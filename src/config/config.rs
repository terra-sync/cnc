#![allow(clippy::module_inception)]

use serde::Deserialize;

use crate::config::postgres::Postgres;

use super::email::SMTP;

#[derive(Deserialize)]
pub struct Config {
    pub postgres: Postgres,
    pub smtp: Option<SMTP>,
}
