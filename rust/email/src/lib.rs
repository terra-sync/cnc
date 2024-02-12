use lettre::{
	message::header::ContentType,
	transport::smtp::authentication::Credentials, Message,
	SmtpTransport, Transport,
};
use std::{ffi::CStr, os::raw::c_char, slice};

/// Represents the necessary information to send an email.
/// Struct is designed to be compatible with C, allowing for easy FFI.
#[repr(C)]
pub struct EmailInfo {
	from: *const c_char,
	to: *const *const c_char,
	to_len: usize,
	cc: *const c_char,
	body: *const c_char,

	smtp_host: *const c_char,
	smtp_username: *const c_char,
	smtp_password: *const c_char,
}

/// Converts a C-style string to a Rust `String`.
/// Returns `Ok(String)` if conversion is successful, `Err(i32)` if the input string is null.
///
/// # Arguments
///
/// * `str_` - A pointer to the null-terminated C-style string to convert.
fn c_char_to_string(str_: *const c_char) -> Result<String, i32> {
	unsafe {
		if str_.is_null() {
			return Err(-1);
		}
		Ok(CStr::from_ptr(str_).to_string_lossy().into_owned())
	}
}

/// Sends an email based on the provided `EmailInfo`.
/// Initializes logging, constructs the email message, and sends it using SMTP.
///
/// # Arguments
///
/// * `email_info` - Struct containing information about the email to be sent.
///
/// # Returns
///
/// Returns `0` on success, `-1` on failure.
/// # Example Usage in C
///
/// This example demonstrates how to construct and pass an `EmailInfo` struct from C,
/// calling the `send_email` function exposed by Rust.
///
/// ```c
/// #include <stdio.h>
/// #include "path/to/generated_header.h" // Include the generated header for Rust FFI
///
/// int main() {
///     // Email information
///     const char* from = "sender@example.com";
///     const char* to[] = {"recipient1@example.com", "recipient2@example.com"};
///     size_t to_len = 2; // Number of recipients
///     const char* cc = "cc@example.com";
///     const char* body = "Hello from C! This is the email body.";
///     const char *smtp_host = "posteo.de";
///     const char *smtp_username = "username";
///     const char *smtp_password = "password";
///
///     // Construct the EmailInfo struct as defined in Rust, adapted for use in C
///     EmailInfo email_info = {
///             .from = from,
///             .to = to,
///             .to_len = to_len,
///             .cc = cc,
///             .body = body,
///             .smtp_host = smtp_host,
///             .smtp_username = smtp_username,
///             .smtp_password = smtp_password,
///     };
///
///     // Call the `send_email` function and capture the return value
///     int result = send_email(email_info);
/// ```
#[no_mangle]
pub extern "C" fn send_email(email_info: EmailInfo) -> i32 {
	tracing_subscriber::fmt::init();

	let from_str = c_char_to_string(email_info.from).unwrap();
	let cc_str = c_char_to_string(email_info.cc).unwrap();
	let body_str = c_char_to_string(email_info.body).unwrap();

	// Convert the 'to' C string array to a Vec of Rust Strings
	let to_strs: Vec<String> = unsafe {
		if email_info.to.is_null() {
			return -1;
		}
		slice::from_raw_parts(email_info.to, email_info.to_len)
			.iter()
			.map(|&c_str| {
				CStr::from_ptr(c_str)
					.to_string_lossy()
					.into_owned()
			})
			.collect()
	};

	let mut email_builder = Message::builder()
		.from(from_str.parse().unwrap())
		.subject("Calling Rust from C")
		.header(ContentType::TEXT_PLAIN);

	for addr in to_strs.iter() {
		email_builder = email_builder.to(addr.parse().unwrap());
	}

	match c_char_to_string(email_info.cc) {
		Ok(str_) => {
			email_builder =
				email_builder.cc(str_.parse().unwrap());
		},
		Err(_) => println!("No `cc` recipient provided."),
	}

	let email = email_builder
		.cc(cc_str.parse().unwrap())
		.body(body_str)
		.unwrap();

	let creds = Credentials::new(
		c_char_to_string(email_info.smtp_username).unwrap(),
		c_char_to_string(email_info.smtp_password).unwrap(),
	);

	let host_str =
		c_char_to_string(email_info.smtp_host).unwrap();
	let mailer = SmtpTransport::starttls_relay(&host_str)
		.unwrap()
		.credentials(creds)
		.build();

	match mailer.send(&email) {
		Ok(_) => {
			println!("Email success.");
			0
		},
		Err(_) => {
			println!("Email failure.");
			-1
		},
	}
}
