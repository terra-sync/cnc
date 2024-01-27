#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "email.h"
#include "config.h"

extern config_t *ini_config;

// Prepare payload
char date_header[256];
char to_header[256];
char from_header[256];
char subject_header[256];
char body_content[1024];

const char *payload_text[] = { date_header, to_header,	    from_header,
			       "Subject: ", subject_header, "\r\n",
			       "\r\n",	    body_content,   ".\r\n",
			       NULL };

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
		return 0;
	}

	data = payload_text[upload_ctx->lines_read];

	if (data) {
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;
		return len;
	}

	return 0;
}

/**
 * send_email - Send an email using SMTP
 * @to: The recipient of the email
 * @subject: The subject of the email
 * @body: The body of the email
 *
 * Returns:
 *  0 on success
 *  -1 if memory allocation fails
 *  -2 if curl initialization fails
 *  -3 if curl_easy_perform fails.
 */
int send_email(const char *to, const char *subject, const char *body)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx = { 0 };

	char *from = strdup(ini_config->smtp_config->username);
	if (from == NULL) {
		return -1;
	}

	size_t url_length =
		strlen("smtp://") + strlen(ini_config->smtp_config->smtp_host) +
		strlen(":") + strlen(ini_config->smtp_config->smtp_port) + 1;
	char *url = malloc(url_length);
	if (url == NULL) {
		free(from);
		return -1;
	} else {
		snprintf(url, url_length, "smtp://%s:%s",
			 ini_config->smtp_config->smtp_host,
			 ini_config->smtp_config->smtp_port);
	}

	char *username = strdup(ini_config->smtp_config->username);
	if (username == NULL) {
		free(from);
		free(url);
		return -1;
	}

	char *password = strdup(ini_config->smtp_config->password);
	if (password == NULL) {
		free(from);
		free(url);
		free(username);
		return -1;
	}

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_USERNAME, username);
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password);

		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from);
		recipients = curl_slist_append(recipients, to);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		time_t now = time(NULL);
		struct tm *tm = localtime(&now);
		strftime(date_header, sizeof(date_header),
			 "Date: %a, %d %b %Y %H:%M:%S %z\r\n", tm);

		snprintf(to_header, sizeof(to_header), "To: %s\r\n", to);
		snprintf(from_header, sizeof(from_header), "From: %s\r\n",
			 from);
		snprintf(subject_header, sizeof(subject_header), "%s\r\n",
			 subject);
		snprintf(body_content, sizeof(body_content), "%s\r\n", body);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			free(from);
			free(url);
			free(username);
			free(password);
			return -3; // curl_easy_perform failed
		}

		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
	} else {
		free(from);
		free(url);
		free(username);
		free(password);
		return -2; // curl initialization failed
	}

	free(from);
	free(url);
	free(username);
	free(password);

	return 0; // Success
}