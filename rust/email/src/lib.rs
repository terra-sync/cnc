use lettre::{
    message::header::ContentType, transport::smtp::authentication::Credentials, Message,
    SmtpTransport, Transport,
};
use std::ffi::CStr;
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn send_email(body: *const c_char) -> i32 {
    tracing_subscriber::fmt::init();

    let c_str = unsafe {
        if body.is_null() {
            return -1; // Return an error code if the input is null
        }
        CStr::from_ptr(body)
    };

    let body_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return -1, // Return an error code if the conversion fails
    };

    let email = Message::builder()
        .from(
            "Charalampos Mitrodimas <charmitro@posteo.net>"
                .parse()
                .unwrap(),
        )
        .to("Charalampos Mitrodimas <trsbisb@gmail.com>"
            .parse()
            .unwrap())
        .subject("Calling Rust from C")
        .header(ContentType::TEXT_PLAIN)
        .body(String::from(body_str))
        .unwrap();

    let creds = Credentials::new("username".to_owned(), "password".to_owned());

    // Open a remote connection to gmail

    let mailer = SmtpTransport::starttls_relay("posteo.de")
        .unwrap()
        .credentials(creds)
        .build();

    // Send the email
    match mailer.send(&email) {
        Ok(_) => {
            println!("Email success.");
            return 0;
        }
        Err(_) => {
            println!("Email failure.");
            return -1;
        }
    }
}
