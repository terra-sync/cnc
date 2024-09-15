#ifndef EMAIL_H_
#define EMAIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Represents the necessary information to send an email.
 * Struct is designed to be compatible with the Rust `EmailInfo` struct.
 */
typedef struct {
	const char *from;
	const char **to;
	size_t to_len;
	const char **cc;
	size_t cc_len;
	const char *filepath;

	const char *smtp_host;
	const char *smtp_username;
	const char *smtp_password;
} EmailInfo;

int send_email(EmailInfo email_info);
int initialize_email_sender();

#endif // EMAIL_H_
