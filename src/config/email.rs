use anyhow::Result;
use lettre::{
    message::{header::ContentType, Mailbox},
    transport::smtp::authentication::Credentials,
    Message, SmtpTransport, Transport,
};

use serde::Deserialize;

#[derive(Deserialize, Debug)]
pub struct SMTP {
    pub enabled: bool,

    pub username: String,
    pub password: String,
    pub smtp_host: String,
    pub smtp_port: u16,

    pub from: String,
    pub to: Vec<String>,
    pub cc: Option<Vec<String>>,
}

impl SMTP {
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
