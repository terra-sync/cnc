//! Email notification functionality using SMTP.

use anyhow::Result;
use lettre::{
    message::{header::ContentType, Mailbox},
    transport::smtp::authentication::Credentials,
    Message, SmtpTransport, Transport,
};

use serde::Deserialize;

/// SMTP configuration for sending email notifications.
#[derive(Deserialize, Debug)]
pub struct SMTP {
    /// Whether email notifications are enabled
    pub enabled: bool,
    /// SMTP username
    pub username: String,
    /// SMTP password
    pub password: String,
    /// SMTP server hostname
    pub smtp_host: String,
    /// SMTP server port
    pub smtp_port: u16,
    /// Email sender address
    pub from: String,
    /// List of recipient email addresses
    pub to: Vec<String>,
    /// Optional list of CC recipient addresses
    pub cc: Option<Vec<String>>,
}

impl SMTP {
    /// Sends an email with the given subject and body.
    ///
    /// # Arguments
    ///
    /// * `subject` - Email subject line
    /// * `body` - Email body content
    ///
    /// # Returns
    ///
    /// Returns `Ok(())` if the email was sent successfully, or an error if sending failed.
    pub fn send_email(&self, subjet: String, body: String) -> Result<()> {
        let from_address: Mailbox = self.from.parse()?;
        let to_addresses: Vec<Mailbox> = self
            .to
            .iter()
            .filter_map(|email| email.parse().ok())
            .collect();

        let mut builder = Message::builder().from(from_address).subject(subjet);
        for to_addr in &to_addresses {
            builder = builder.to(to_addr.clone());
        }

        if let Some(cc_list) = &self.cc {
            for email in cc_list {
                if let Ok(cc_addr) = email.parse() {
                    builder = builder.cc(cc_addr);
                }
            }
        }

        let email = builder.header(ContentType::TEXT_PLAIN).body(body)?;
        let creds = Credentials::new(self.username.to_owned(), self.password.to_owned());

        let mailer = SmtpTransport::relay(&self.smtp_host)
            .unwrap()
            .credentials(creds)
            .build();

        match mailer.send(&email) {
            Ok(_) => Ok(()),
            Err(e) => Err(anyhow::anyhow!("Could not send email: {e:?}")),
        }
    }
}
