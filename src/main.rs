use anyhow::{anyhow, Context, Result};
use clap::Parser;
use cnc_rs::db::{db_operations::DatabaseOperations, postgresdb::PostgresDB};
use cnc_rs::logger::init_logger;

use cnc_rs::{cli::Cli, config::Config};

use std::fs::{self};

fn main() -> Result<()> {
    let cli = Cli::parse();
    init_logger(cli.verbose);

    let mut config: Config = toml::from_str(&fs::read_to_string(&cli.config_file)?)
        .context("Failed to parse configuration file")?;

    // Override config fields if flags are found;
    if let Some(smtp) = config.smtp.as_mut() {
        if let Some(email_flag) = cli.email {
            smtp.enabled = email_flag;
        }
        println!("{:?}", config.smtp);
    }
    let mut databases: Vec<Box<dyn DatabaseOperations>> = Vec::new();
    if config.postgres.enabled {
        databases.push(Box::new(PostgresDB::new(config)));
    }

    if databases.len().eq(&0) {
        return Err(anyhow!("No databases are enabled in the configuration"));
    }

    for mut db in databases {
        db.connect().context("Failed to connect to database")?;
        db.replicate().context("Failed to replicate databases")?;
        db.close().context("Failed to close database")?;
    }

    Ok(())
}
