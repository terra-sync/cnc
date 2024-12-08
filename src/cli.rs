use clap::Parser;
use std::path::PathBuf;

#[derive(Parser)]
#[command(version, about, long_about = None)]
pub struct Cli {
    pub config_file: PathBuf,

    #[arg(long)]
    pub verbose: bool,
}
