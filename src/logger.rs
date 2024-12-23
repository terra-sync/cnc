use env_logger::{Builder, Env};
use std::{
    fs::File,
    io::{self, Write},
};

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

/// A writer that duplicates output to stdout and a log file.
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

#[cfg(test)]
mod tests {
    use env_logger::{Builder, Env};
    use log::{debug, error, info, warn};
    use std::{
        fs::{self, File},
        io::{self, Write},
        thread,
    };

    use super::DualWriter;

    fn init_logger(verbose: bool) {
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

        builder.is_test(true).try_init().unwrap();
    }

    #[test]
    fn test_logger_behavior() {
        init_logger(true);

        // Test logging functionality
        let a = 1;
        let b = 2;

        debug!("checking whether {} + {} = 3", a, b);
        assert_eq!(3, a + b);

        // Test log formatting
        let msg = "Test message";
        info!("{}", msg);
        let log_contents = fs::read_to_string("output.log").expect("Unable to read log file");
        assert!(log_contents.contains("Test message"));

        // Test different log levels
        info!("This is an info message");
        warn!("This is a warning message");
        error!("This is an error message");
        let log_contents = fs::read_to_string("output.log").expect("Unable to read log file");
        assert!(log_contents.contains("This is an info message"));
        assert!(log_contents.contains("This is a warning message"));
        assert!(log_contents.contains("This is an error message"));

        // Test dual output
        info!("Logging to both stdout and file");
        let log_contents = fs::read_to_string("output.log").expect("Unable to read log file");
        assert!(log_contents.contains("Logging to both stdout and file"));

        // Test concurrent logging
        let handles: Vec<_> = (0..10)
            .map(|i| {
                thread::spawn(move || {
                    for _ in 0..10 {
                        info!("Thread {} logging", i);
                    }
                })
            })
            .collect();

        for handle in handles {
            handle.join().unwrap();
        }

        let log_contents = fs::read_to_string("output.log").expect("Unable to read log file");
        for i in 0..10 {
            assert!(log_contents.contains(&format!("Thread {} logging", i)));
        }

        fs::remove_file("output.log").unwrap();
    }
}
