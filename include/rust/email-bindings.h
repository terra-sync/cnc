#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Represents the necessary information to send an email.
 * Struct is designed to be compatible with C, allowing for easy FFI.
 */
typedef struct EmailInfo {
	const char *from;
	const char *const *to;
	uintptr_t to_len;
	const char *const *cc;
	uintptr_t cc_len;
	const char *body;
	const char *smtp_host;
	const char *smtp_username;
	const char *smtp_password;
} EmailInfo;

/**
 * Sends an email based on the provided `EmailInfo`.
 * Initializes logging, constructs the email message, and sends it using SMTP.
 *
 * # Arguments
 *
 * * `email_info` - Struct containing information about the email to be sent.
 *
 * # Returns
 *
 * Returns `0` on success, `-1` on failure.
 * # Example Usage in C
 *
 * This example demonstrates how to construct and pass an `EmailInfo` struct from C,
 * calling the `send_email` function exposed by Rust.
 *
 * ```c
 * #include <stdio.h>
 * #include "path/to/generated_header.h" // Include the generated header for Rust FFI
 *
 * int main() {
 *     // Email information
 *     const char* from = "sender@example.com";
 *     const char* to[] = {"recipient1@example.com", "recipient2@example.com"};
 *     size_t to_len = 2; // Number of recipients
 *     const char* cc[] = {"recipient1@example.com", "recipient2@example.com"};
 *     size_t cc_len = 2; // Number of recipients
 *     const char* body = "Hello from C! This is the email body.";
 *     const char *smtp_host = "posteo.de";
 *     const char *smtp_username = "username";
 *     const char *smtp_password = "password";
 *
 *     // Construct the EmailInfo struct as defined in Rust, adapted for use in C
 *     EmailInfo email_info = {
 *             .from = from,
 *             .to = to,
 *             .to_len = to_len,
 *             .cc = cc,
 *             .cc_len = cc_len,
 *             .body = body,
 *             .smtp_host = smtp_host,
 *             .smtp_username = smtp_username,
 *             .smtp_password = smtp_password,
 *     };
 *
 *     // Call the `send_email` function and capture the return value
 *     int result = send_email(email_info);
 * ```
 */
int32_t send_email(struct EmailInfo email_info);
