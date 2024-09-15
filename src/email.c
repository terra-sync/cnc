#include "email.h"
#include <curl/curl.h>
#include <pthread.h>

// Thread-local storage for CURL handles
static pthread_key_t curl_key;
static pthread_once_t curl_key_once = PTHREAD_ONCE_INIT;

// Function to create the thread-local storage key
static void make_curl_key()
{
	pthread_key_create(&curl_key, curl_easy_cleanup);
}

// Function to get a thread-local CURL handle
static CURL *get_curl_handle()
{
	CURL *curl;

	pthread_once(&curl_key_once, make_curl_key);
	curl = pthread_getspecific(curl_key);

	if (curl == NULL) {
		curl = curl_easy_init();
		if (curl != NULL) {
			pthread_setspecific(curl_key, curl);
		}
	}

	return curl;
}

/**
 * Replaces all instances of LF ('\n') with CRLF ("\r\n") in a string.
 *
 * @param input - The input string with LF line endings.
 * @return A newly allocated string with CRLF line endings, or NULL on failure.
 */
char *convert_to_crlf(const char *input)
{
	size_t input_len = strlen(input);
	size_t crlf_count = 0;

	// Count the number of '\n' characters to calculate the new size
	for (size_t i = 0; i < input_len; i++) {
		if (input[i] == '\n') {
			crlf_count++;
		}
	}

	// Allocate memory for the new string
	size_t output_len =
		input_len + crlf_count; // Each '\n' replaced with '\r\n'
	char *output = (char *)malloc(output_len + 1); // +1 for null terminator
	if (!output) {
		return NULL;
	}

	size_t j = 0;
	for (size_t i = 0; i < input_len; i++) {
		if (input[i] == '\n') {
			output[j++] = '\r';
			output[j++] = '\n';
		} else if (input[i] == '\r') {
			// If '\r' is already present, check if it's followed by '\n'
			if (input[i + 1] == '\n') {
				output[j++] = '\r';
				output[j++] = '\n';
				i++; // Skip the next character as it's '\n'
			} else {
				// Standalone '\r', add '\n' to complete CRLF
				output[j++] = '\r';
				output[j++] = '\n';
			}
		} else {
			output[j++] = input[i];
		}
	}
	output[j] = '\0';

	return output;
}

/**
 * Reads the email body from a file, located in `filepath`.
 *
 * @param filepath - A string containing the filepath of the file to read.
 * @return A dynamically allocated string containing the file contents, or NULL on failure.
 */
char *read_mail_body(const char *filepath)
{
	FILE *file = fopen(filepath, "rb");
	if (!file) {
		perror("Cannot open file");
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	rewind(file);

	char *body = (char *)malloc(fsize + 1);
	if (!body) {
		perror("Cannot allocate memory");
		fclose(file);
		return NULL;
	}
	fread(body, 1, fsize, file);
	fclose(file);
	body[fsize] = '\0';
	return body;
}

/**
 * Joins an array of email addresses into a single comma-separated string.
 *
 * @param addresses - Array of email address strings.
 * @param count - Number of email addresses.
 * @return A dynamically allocated string containing the joined email addresses.
 */
char *join_email_addresses(const char **addresses, size_t count)
{
	size_t total_length = 0;
	for (size_t i = 0; i < count; i++) {
		total_length += strlen(addresses[i]) + 2; // For ", "
	}
	if (total_length == 0)
		return NULL;

	char *result = (char *)malloc(total_length + 1);
	if (!result)
		return NULL;

	result[0] = '\0';
	for (size_t i = 0; i < count; i++) {
		strcat(result, addresses[i]);
		if (i < count - 1) {
			strcat(result, ", ");
		}
	}

	return result;
}

/**
 * Callback function for libcurl to read the email data.
 */
struct upload_status {
	size_t bytes_read;
	const char *data;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	size_t max = size * nmemb;
	size_t len = strlen(upload_ctx->data + upload_ctx->bytes_read);

	if (len > max)
		len = max;

	if (len > 0) {
		memcpy(ptr, upload_ctx->data + upload_ctx->bytes_read, len);
		upload_ctx->bytes_read += len;
		return len;
	}

	return 0;
}

int send_email(EmailInfo email_info)
{
	CURL *curl = NULL;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx = { 0 };
	char *body = NULL;
	char *body_crlf = NULL;
	char *payload = NULL;
	char *to_addresses = NULL;
	char *cc_addresses = NULL;
	char *headers_crlf = NULL;
	int result = 0;

	/* Read email body from file */
	body = read_mail_body(email_info.filepath);
	if (!body) {
		fprintf(stderr, "Failed to read email body\n");
		result = -2;
		goto cleanup;
	}

	/* Convert body to use CRLF line endings */
	body_crlf = convert_to_crlf(body);
	if (!body_crlf) {
		fprintf(stderr, "Failed to convert body to CRLF\n");
		result = -1;
		goto cleanup;
	}

	/* Get thread-local CURL handle */
	curl = get_curl_handle();
	if (!curl) {
		fprintf(stderr, "Failed to get CURL handle\n");
		result = -1;
		goto cleanup;
	}

	/* Reset CURL options for this request */
	curl_easy_reset(curl);

	/* Set SMTP server and authentication details */
	curl_easy_setopt(curl, CURLOPT_USERNAME, email_info.smtp_username);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, email_info.smtp_password);
	char smtp_url[256];
	snprintf(smtp_url, sizeof(smtp_url), "smtp://%s:%d",
		 email_info.smtp_host, 587);
	curl_easy_setopt(curl, CURLOPT_URL, smtp_url);

	/* Enable STARTTLS */
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

	/* Set sender and recipients */
	curl_easy_setopt(curl, CURLOPT_MAIL_FROM, email_info.from);
	for (size_t i = 0; i < email_info.to_len; i++) {
		recipients = curl_slist_append(recipients, email_info.to[i]);
	}
	for (size_t i = 0; i < email_info.cc_len; i++) {
		recipients = curl_slist_append(recipients, email_info.cc[i]);
	}
	curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

	/* Build email headers and payload */
	to_addresses = join_email_addresses(email_info.to, email_info.to_len);
	cc_addresses = join_email_addresses(email_info.cc, email_info.cc_len);

	/* Ensure headers use CRLF line endings */
	const char *subject = "Subject: CNC digest\r\n";
	const char *content_type = "Content-Type: text/plain\r\n";
	char from_header[256];
	snprintf(from_header, sizeof(from_header), "From: %s\r\n",
		 email_info.from);

	char headers[2048] = "";
	strcat(headers, from_header);
	if (to_addresses) {
		strcat(headers, "To: ");
		strcat(headers, to_addresses);
		strcat(headers, "\r\n");
	}
	if (cc_addresses) {
		strcat(headers, "Cc: ");
		strcat(headers, cc_addresses);
		strcat(headers, "\r\n");
	}
	strcat(headers, subject);
	strcat(headers, content_type);
	strcat(headers, "\r\n");

	/* Convert headers to use CRLF line endings */
	headers_crlf = convert_to_crlf(headers);
	if (!headers_crlf) {
		fprintf(stderr, "Failed to convert headers to CRLF\n");
		result = -1;
		goto cleanup;
	}

	/* Combine headers and body into payload */
	size_t payload_size = strlen(headers_crlf) + strlen(body_crlf) + 1;
	payload = (char *)malloc(payload_size);
	if (!payload) {
		fprintf(stderr, "Failed to allocate memory for payload\n");
		result = -1;
		goto cleanup;
	}
	strcpy(payload, headers_crlf);
	strcat(payload, body_crlf);

	/* Set up the payload text */
	upload_ctx.bytes_read = 0;
	upload_ctx.data = payload;

	/* Set libcurl options for sending the email */
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
	curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

	/* Send the message */
	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		fprintf(stderr, "libcurl error: %s\n", curl_easy_strerror(res));
		result = -1;
	} else {
		result = 0;
	}

cleanup:
	/* Clean up resources */
	if (body)
		free(body);
	if (body_crlf)
		free(body_crlf);
	if (headers_crlf)
		free(headers_crlf);
	if (payload)
		free(payload);
	if (to_addresses)
		free(to_addresses);
	if (cc_addresses)
		free(cc_addresses);
	if (recipients)
		curl_slist_free_all(recipients);

	return result;
}

// Function to initialize the email sender (call this in your main thread)
int initialize_email_sender()
{
	CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_global_init() failed: %s\n",
			curl_easy_strerror(res));
		return -1;
	}
	return 0;
}

// Function to clean up the email sender (call this in your main thread before exit)
void cleanup_email_sender()
{
	curl_global_cleanup();
}
