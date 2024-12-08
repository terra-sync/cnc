#![allow(clippy::module_inception)]

use serde::Deserialize;

use crate::config::postgres::Postgres;

#[derive(Deserialize)]
pub struct Config {
    pub postgres: Postgres,
}
