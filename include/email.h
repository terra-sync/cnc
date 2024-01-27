#ifndef EMAIL_H
#define EMAIL_H

#include <curl/curl.h>

// Struct representing the status of an email upload.
struct upload_status {
	int lines_read; // Number of lines read in the email body.
};


int send_email(const char *to, const char *subject, const char *body);

#endif