//! Main configuration structures.

#![allow(clippy::module_inception)]

use serde::Deserialize;

use crate::config::{email::SMTP, postgres::Postgres};

/// Primary configuration structure containing all settings.
#[derive(Deserialize)]
pub struct Config {
    /// PostgreSQL database configuration
    pub postgres: Postgres,
    /// Optional SMTP email configuration
    pub smtp: Option<SMTP>,
}
