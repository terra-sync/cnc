pub mod cli;
pub mod config;
pub mod db;
pub mod logger;

use anyhow::{Context, Result};
use clap::Parser;
use db::{db_operations::DatabaseOperations, postgresdb::PostgresDB};
use logger::init_logger;

use crate::{cli::Cli, config::Config};

use std::fs::{self};

fn main() -> Result<()> {
    let cli = Cli::parse();
    init_logger(cli.verbose);

    let config: Config = toml::from_str(&fs::read_to_string(&cli.config_file)?)
        .context("Failed to parse configuration file")?;

    let mut databases: Vec<Box<dyn DatabaseOperations>> = Vec::new();
    if config.postgres.enabled {
        databases.push(Box::new(PostgresDB::new(config)));
    }

    for mut db in databases {
        db.connect().context("Failed to connect to database")?;
        db.replicate().context("Failed to replicate databases")?;
        db.close().context("Failed to close database")?;
    }

    Ok(())
}
