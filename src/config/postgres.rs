//! PostgreSQL database configuration.

use serde::Deserialize;

/// Type of backup to perform
#[derive(Deserialize)]
pub enum BackupType {
    /// Schema-only backup
    Schema,
    /// Full backup including data
    Full,
}

impl Default for BackupType {
    fn default() -> Self {
        Self::Full
    }
}

/// PostgreSQL database configuration settings
#[derive(Deserialize)]
pub struct Postgres {
    /// Whether PostgreSQL replication is enabled
    #[serde(default = "default_enabled")]
    pub enabled: bool,

    /// Source database host
    pub origin_host: String,
    /// Source database user
    pub origin_user: String,
    /// Source database password
    pub origin_password: String,
    /// Source database port
    pub origin_port: String,
    /// Source database name
    pub origin_database: String,

    /// Target database host
    pub target_host: String,
    /// Target database user
    pub target_user: String,
    /// Target database password
    pub target_password: String,
    /// Target database port
    pub target_port: String,
    /// Target database name
    pub target_database: String,

    /// Type of backup to perform
    #[serde(default = "BackupType::default")]
    pub backup_type: BackupType,

    /// Whether to send email notifications
    #[serde(default = "default_email")]
    pub email: bool,
}

// Default value functions
pub fn default_enabled() -> bool {
    true
}

pub fn default_email() -> bool {
    false
}
