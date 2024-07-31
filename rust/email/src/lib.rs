use lettre::{
	message::header::ContentType,
	transport::smtp::authentication::Credentials, Message,
	SmtpTransport, Transport,
};
use std::{ffi::CStr, fs, io, os::raw::c_char, slice};

/// Represents the necessary information to send an email.
/// Struct is designed to be compatible with C, allowing for easy
/// FFI.
#[repr(C)]
pub struct EmailInfo {
	from: *const c_char,
	to: *const *const c_char,
	to_len: usize,
	cc: *const *const c_char,
	cc_len: usize,
	filepath: *const c_char,

	smtp_host: *const c_char,
	smtp_username: *const c_char,
	smtp_password: *const c_char,
}

/// Converts a C-style string to a Rust `String`.
/// Returns `Ok(String)` if conversion is successful, `Err(i32)`
/// if the input string is null.
///
/// # Arguments
///
/// * `str_` - A pointer to the null-terminated C-style string to
///   convert.
fn c_char_to_string(str_: *const c_char) -> Result<String, i32> {
	unsafe {
		if str_.is_null() {
			return Err(-1);
		}
		Ok(CStr::from_ptr(str_).to_string_lossy().into_owned())
	}
}

/// Reads the email body from a file, located in `filepath`.
///
/// # Arguments
///
/// * `filepath` - A String containing the filepath of the file
///   we want to read.
fn read_mail_body(filepath: &str) -> Result<String, io::Error> {
	fs::read_to_string(filepath)
}

/// Sends an email based on the provided `EmailInfo`.
/// Initializes logging, constructs the email message, and sends
/// it using SMTP.
///
/// # Arguments
///
/// * `email_info` - Struct containing information about the
///   email to be sent.
///
/// # Returns
///
/// Returns `0` on success, `-1` on failure.
/// # Example Usage in C
///
/// This example demonstrates how to construct and pass an
/// `EmailInfo` struct from C, calling the `send_email` function
/// exposed by Rust.
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
///     const char* cc[] = {"recipient1@example.com", "recipient2@example.com"};
///     size_t cc_len = 2; // Number of recipients
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
///             .cc_len = cc_len,
///             .filepath = filepath,
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
	let from_str = match c_char_to_string(email_info.from) {
		Ok(s) => s,
		Err(_) => return -1,
	};

	let filepath_str =
		match c_char_to_string(email_info.filepath) {
			Ok(s) => s,
			Err(_) => return -1,
		};

	let body = match read_mail_body(&filepath_str) {
		Ok(body) => body,
		Err(error) => {
			println!("{:?}", error);
			return -2;
		},
	};

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

	let cc_strs: Option<Vec<String>> = unsafe {
		if !email_info.cc.is_null() {
			let vec = slice::from_raw_parts(
				email_info.cc,
				email_info.cc_len,
			)
			.iter()
			.map(|&c_str| {
				CStr::from_ptr(c_str)
					.to_string_lossy()
					.into_owned()
			})
			.collect();
			Some(vec)
		} else {
			None
		}
	};

	let mut email_builder = match from_str.parse() {
		Ok(parsed_from) => Message::builder()
			.from(parsed_from)
			.subject("CNC digest")
			.header(ContentType::TEXT_PLAIN),
		Err(_) => return -1,
	};

	for addr in to_strs.iter() {
		match addr.parse() {
			Ok(parsed_to) => {
				email_builder = email_builder.to(parsed_to);
			},
			Err(_) => return -1,
		}
	}

	if let Some(cc_strs) = cc_strs {
		for cc_addr in cc_strs.iter() {
			match cc_addr.parse() {
				Ok(parsed_cc) => {
					email_builder = email_builder.cc(parsed_cc);
				},
				Err(_) => return -1,
			}
		}
	};

	let email = match email_builder.body(body) {
		Ok(email) => email,
		Err(_) => return -1,
	};

	let smtp_username =
		match c_char_to_string(email_info.smtp_username) {
			Ok(s) => s,
			Err(_) => return -1,
		};

	let smtp_password =
		match c_char_to_string(email_info.smtp_password) {
			Ok(s) => s,
			Err(_) => return -1,
		};

	let creds = Credentials::new(smtp_username, smtp_password);

	let host_str = match c_char_to_string(email_info.smtp_host) {
		Ok(s) => s,
		Err(_) => return -1,
	};

	let mailer = match SmtpTransport::starttls_relay(&host_str) {
		Ok(builder) => builder.credentials(creds).build(),
		Err(_) => return -1,
	};

	match mailer.send(&email) {
		Ok(_) => 0,
		Err(_) => -1,
	}
}
