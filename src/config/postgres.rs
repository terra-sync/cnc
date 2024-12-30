use serde::Deserialize;

#[derive(Deserialize)]
pub enum BackupType {
    Schema,
    Full,
}

impl Default for BackupType {
    fn default() -> Self {
        Self::Full
    }
}

#[derive(Deserialize)]
pub struct Postgres {
    #[serde(default = "Postgres::default_enabled")]
    pub enabled: bool,

    pub origin_host: String,
    pub origin_user: String,
    pub origin_password: String,
    pub origin_port: String,
    pub origin_database: String,

    pub target_host: String,
    pub target_user: String,
    pub target_password: String,
    pub target_port: String,
    pub target_database: String,

    #[serde(default = "BackupType::default")]
    pub backup_type: BackupType,

    #[serde(default = "Postgres::default_email")]
    pub email: bool,
}

impl Postgres {
    // Default value functions
    pub fn default_enabled() -> bool {
        true
    }

    pub fn default_email() -> bool {
        false
    }
}
