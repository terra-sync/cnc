use anyhow::{anyhow, Context, Error};
use chrono::Local;
use log::{info, warn};
use postgres::{Client, NoTls};
use std::process::{Command, Output};

use super::db_operations::DatabaseOperations;
use crate::config::{email::SMTP, postgres::Postgres, Config};

pub struct PostgresDB {
    config_postgres: Postgres,
    config_email: Option<SMTP>,
    origin_client: Option<Client>,
    target_client: Option<Client>,
}

impl PostgresDB {
    pub fn new(conf: Config) -> Self {
        Self {
            config_postgres: conf.postgres,
            config_email: conf.smtp,
            origin_client: None,
            target_client: None,
        }
    }
}

impl DatabaseOperations for PostgresDB {
    fn connect(&mut self) -> Result<(), Error> {
        info!("Connecting to {}", self.config_postgres.origin_host);
        self.origin_client = Some(
            Client::connect(
                &format!(
                    "host={} user={} password={} port={}",
                    self.config_postgres.origin_host,
                    self.config_postgres.origin_user,
                    self.config_postgres.origin_password,
                    self.config_postgres.origin_port,
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
                    self.config_postgres.target_host,
                    self.config_postgres.target_user,
                    self.config_postgres.target_password,
                    self.config_postgres.target_port,
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
        info!("Disconnecting from {}", self.config_postgres.origin_host);
        if let Some(origin_client) = self.origin_client.take() {
            origin_client.close()?
        }

        if let Some(target_client) = self.target_client.take() {
            target_client.close()?
        }

        Ok(())
    }

    fn replicate(&mut self) -> Result<(), Error> {
        let mut output_log = String::new();

        info!(
            "Replicating all data from {} to {}",
            self.config_postgres.origin_host, self.config_postgres.target_host
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
            .env("PGPASSWORD", &self.config_postgres.origin_password)
            .arg("-U")
            .arg(&self.config_postgres.origin_user)
            .arg("-h")
            .arg(&self.config_postgres.origin_host)
            .arg("-p")
            .arg(&self.config_postgres.origin_port)
            .arg(&self.config_postgres.origin_database)
            .arg("-v")
            .arg("--clean")
            .arg("-f")
            .arg(dump_file_path)
            .output()
            .context("Failed to start pg_dump process")?;

        if !dump_output.stdout.is_empty() {
            let stdout = String::from_utf8_lossy(&dump_output.stdout);
            info!("pg_dump stdout:\n{}", stdout);
            output_log.push_str(&format!("\npg_dump stdout:\n{}", stdout));
        }
        if !dump_output.stderr.is_empty() {
            let stderr = String::from_utf8_lossy(&dump_output.stderr);
            warn!("pg_dump stderr:\n{}", stderr);
            output_log.push_str(&format!("\npg_dump stderr:\n{}", stderr));
        }
        if !dump_output.status.success() {
            return Err(anyhow!("pg_dump process exited with an error"));
        }

        let dump_msg = format!("Data successfully dumped to {}\n", dump_file_path);
        info!("{}", dump_msg);
        output_log.push_str(&dump_msg);

        let restore_output: Output = Command::new("psql")
            .arg("-h")
            .arg(&self.config_postgres.target_host)
            .arg("-p")
            .arg(&self.config_postgres.target_port)
            .arg("-U")
            .arg(&self.config_postgres.target_user)
            .arg("-d")
            .arg(&self.config_postgres.origin_database)
            .arg("-f")
            .arg(dump_file_path)
            .env("PGPASSWORD", &self.config_postgres.target_password)
            .output()
            .context("Failed to start psql process for restoring")?;
        if !restore_output.stdout.is_empty() {
            let stdout = String::from_utf8_lossy(&restore_output.stdout);
            info!("psql stdout:\n{}", stdout);
            output_log.push_str(&format!("\npsql stdout:\n{}", stdout));
        }
        if !restore_output.stderr.is_empty() {
            let stderr = String::from_utf8_lossy(&restore_output.stderr);
            warn!("psql stderr:\n{}", stderr);
            output_log.push_str(&format!("\npsql stderr:\n{}", stderr));
        }
        if !restore_output.status.success() {
            return Err(anyhow!("psql process exited with an error"));
        }

        let success_msg = format!(
            "Successfully replicated all data from {} to {}\n",
            self.config_postgres.origin_host, self.config_postgres.target_host,
        );
        info!("{}", success_msg);
        output_log.push_str(&success_msg);

        std::fs::remove_file(dump_file_path)?;

        if let Some(smtp_config) = &self.config_email {
            if smtp_config.enabled {
                smtp_config
                    .send_email(format!("POSTGRES Replication {}", Local::now()), output_log)?;
            }
        }
        Ok(())
    }
}
