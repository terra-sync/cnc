//! Command-line interface definitions.

use clap::Parser;
use std::path::PathBuf;

/// Command-line argument structure.
#[derive(Parser)]
#[command(version, about, long_about = None)]
pub struct Cli {
    /// Path to the configuration file
    pub config_file: PathBuf,

    /// Enable verbose logging
    #[arg(long)]
    pub verbose: bool,

    /// Override email notification setting
    #[arg(long)]
    pub email: Option<bool>,
}
