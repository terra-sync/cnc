use anyhow::{anyhow, Context, Error};
use log::{info, warn};
use postgres::{Client, NoTls};
use std::process::{Command, Output};

use super::db_operations::DatabaseOperations;
use crate::config::{postgres::Postgres, Config};

pub struct PostgresDB {
    config: Postgres,
    origin_client: Option<Client>,
    target_client: Option<Client>,
}

impl PostgresDB {
    pub fn new(conf: Config) -> Self {
        Self {
            config: conf.postgres,
            origin_client: None,
            target_client: None,
        }
    }
}

impl DatabaseOperations for PostgresDB {
    fn connect(&mut self) -> Result<(), Error> {
        info!("Connecting to {}", self.config.origin_host);
        self.origin_client = Some(
            Client::connect(
                &format!(
                    "host={} user={} password={} port={}",
                    self.config.origin_host,
                    self.config.origin_user,
                    self.config.origin_password,
                    self.config.origin_port,
                ),
                NoTls,
            )
            .context("Cannot connect to Postgres Origin Database")?,
        );
        if let Some(ref mut origin_client) = self.origin_client {
            origin_client
                .simple_query("SELECT 1")
                .context("Cannot ping the Postgres Origin Database")?;
        } else {
            return Err(anyhow::anyhow!("Origin client is not connected"));
        }

        self.target_client = Some(
            Client::connect(
                &format!(
                    "host={} user={} password={} port={}",
                    self.config.target_host,
                    self.config.target_user,
                    self.config.target_password,
                    self.config.target_port,
                ),
                NoTls,
            )
            .context("Cannot connect to Postgres Target Database")?,
        );
        if let Some(ref mut target_client) = self.target_client {
            target_client
                .simple_query("SELECT 1")
                .context("Cannot ping the Postgres Target Database")?;
        } else {
            return Err(anyhow::anyhow!("Target client is not connected"));
        }

        Ok(())
    }

    fn close(&mut self) -> Result<(), Error> {
        info!("Disconnecting from {}", self.config.origin_host);
        if let Some(origin_client) = self.origin_client.take() {
            origin_client.close()?
        }

        if let Some(target_client) = self.target_client.take() {
            target_client.close()?
        }

        Ok(())
    }

    fn replicate(&mut self) -> Result<(), Error> {
        info!(
            "Replicating all data from {} to {}",
            self.config.origin_host, self.config.target_host
        );

        // Ensure the clients are connected
        if self.origin_client.is_none() {
            return Err(anyhow::anyhow!("Origin client is not connected"));
        }
        if self.target_client.is_none() {
            return Err(anyhow::anyhow!("Target client is not connected"));
        }

        let dump_file_path = "/tmp/clean_dump.sql";

        let dump_output: Output = Command::new("pg_dump")
            .env("PGPASSWORD", &self.config.origin_password)
            .arg("-U")
            .arg(&self.config.origin_user)
            .arg("-h")
            .arg(&self.config.origin_host)
            .arg("-p")
            .arg(&self.config.origin_port)
            .arg(&self.config.origin_database)
            .arg("-v")
            .arg("--clean")
            .arg("-f")
            .arg(dump_file_path)
            .output()
            .context("Failed to start pg_dump process")?;

        if !dump_output.stdout.is_empty() {
            info!(
                "pg_dump stdout:\n{}",
                String::from_utf8_lossy(&dump_output.stdout)
            );
        }
        if !dump_output.stderr.is_empty() {
            warn!(
                "pg_dump stderr:\n{}",
                String::from_utf8_lossy(&dump_output.stderr)
            );
        }
        if !dump_output.status.success() {
            return Err(anyhow!("pg_dump process exited with an error"));
        }
        info!("Data successfully dumped to {}", dump_file_path);

        let restore_output: Output = Command::new("psql")
            .arg("-h")
            .arg(&self.config.target_host)
            .arg("-p")
            .arg(&self.config.target_port)
            .arg("-U")
            .arg(&self.config.target_user)
            .arg("-d")
            .arg(&self.config.origin_database)
            .arg("-f")
            .arg(dump_file_path)
            .env("PGPASSWORD", &self.config.target_password)
            .output()
            .context("Failed to start psql process for restoring")?;

        if !restore_output.stdout.is_empty() {
            info!(
                "psql stdout:\n{}",
                String::from_utf8_lossy(&restore_output.stdout)
            );
        }
        if !restore_output.stderr.is_empty() {
            warn!(
                "psql stderr:\n{}",
                String::from_utf8_lossy(&restore_output.stderr)
            );
        }
        if !restore_output.status.success() {
            return Err(anyhow!("psql process exited with an error"));
        }
        info!(
            "Successfully replicated all data from {} to {}",
            self.config.origin_host, self.config.target_host,
        );

        std::fs::remove_file(dump_file_path)?;

        Ok(())
    }
}
