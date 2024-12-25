//! Logging functionality for the database replication tool.

use env_logger::{Builder, Env};
use std::{
    fs::File,
    io::{self, Write},
};

/// Initializes the logging system with optional verbose mode and file output.
///
/// # Arguments
///
/// * `verbose` - When true, logs are written to both stdout and a file named "output.log".
///               When false, logs are only written to stdout.
///
/// # Example
///
/// ```
/// use crate::logger::init_logger;
///
/// init_logger(true); // Enable verbose logging
/// ```
pub fn init_logger(verbose: bool) {
    let env = Env::default()
        .filter_or("MY_LOG_LEVEL", "info")
        .write_style("MY_LOG_STYLE");

    let mut builder = Builder::from_env(env);

    builder.format(move |buf, record| {
        let timestamp = buf.timestamp();
        writeln!(
            buf,
            "[{timestamp}] {level}: {message}",
            level = record.level(),
            message = record.args()
        )
    });

    if verbose {
        // Redirect logs to both stdout and a file
        let log_file = File::create("output.log").expect("Unable to create log file");
        let dual_writer = DualWriter {
            stdout: io::stdout(),
            log_file,
        };

        builder.target(env_logger::Target::Pipe(Box::new(dual_writer)));
    }

    builder.init();
}

/// A writer implementation that duplicates output to both stdout and a log file.
///
/// This struct is used internally by the logger when verbose mode is enabled.
pub struct DualWriter {
    stdout: io::Stdout,
    log_file: File,
}

impl Write for DualWriter {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.stdout.lock().write_all(buf)?;
        self.log_file.write(buf)
    }

    fn flush(&mut self) -> io::Result<()> {
        self.stdout.lock().flush()?;
        self.log_file.flush()
    }
}
