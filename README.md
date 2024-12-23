# CNC-RS

CNC-RS is a Rust-based application for PostgreSQL database replication, configurable through a TOML file and a command-line interface (CLI). It features a robust logging system and optional email notifications to streamline database operations and ensure reliability.

## Features

- **PostgreSQL Replication**: Efficiently replicates data between PostgreSQL instances using `pg_dump` and `psql`.
- **Logging**: Provides detailed logs, including timestamps and log levels, with output redirected to both the console and a log file (`output.log`).
- **Email Notifications**: Configurable email notifications for replication success or failure using SMTP settings.
- **Flexible Backup Options**: Supports full and schema-only backups.
- **CLI and Configuration File**: Combines CLI options with a TOML configuration file for flexible operation and overrides.

---

## Installation

### Using `cargo install`

Install it using Cargo:

```bash
cargo install cnc-rs
```

### From Source

1. Clone the repository:
   ```bash
   git clone https://github.com/terra-sync/cnc
   cd cnc-rs
   ```

2. Install globally using Cargo:
   ```bash
   cargo install --path .
   ```

---

## Usage

### Command-Line Options

```bash
cnc <path-to-config.toml> [--verbose] [--email <true|false>]
```

- `--verbose`: Enables detailed logging to the console and `output.log`.
- `--email`: Overrides the email notification setting in the configuration file.

### Example

```bash
cnc ./config.toml --verbose --email true
```

---

## Configuration

CNC-RS requires a configuration file in TOML format. Below is an example:

```toml
[postgres]
enabled = true
origin_host = "localhost"
origin_user = "postgres"
origin_password = "password"
origin_port = "5432"
origin_database = "source_db"
target_host = "localhost"
target_user = "postgres"
target_password = "password"
target_port = "5432"
target_database = "target_db"
backup_type = "Full" # Options: "Full" or "Schema"

[smtp]
enabled = true
username = "your_email_username"
password = "your_email_password"
smtp_host = "smtp.example.com"
smtp_port = 587
from = "your_email@example.com"
to = ["recipient1@example.com", "recipient2@example.com"]
cc = ["cc_recipient@example.com"]
```

### Sections

- **[postgres]**:
  - `enabled`: Enables or disables PostgreSQL replication.
  - `origin_*`: Details for the source database.
  - `target_*`: Details for the target database.
  - `backup_type`: Type of backup (`Full` or `Schema`).

- **[smtp]**:
  - `enabled`: Enables or disables email notifications.
  - `username`, `password`: SMTP credentials.
  - `smtp_host`, `smtp_port`: SMTP server details.
  - `from`: Email sender.
  - `to`: List of recipient emails.
  - `cc`: List of CC recipient emails (optional).

---

## Logging

Logs are generated in both the console and a file named `output.log`. Each log entry includes:
- A timestamp
- Log level (e.g., `INFO`, `WARN`, `ERROR`)
- Log message

Enable detailed logs using the `--verbose` flag.

---

## Development

To contribute or modify this project:

1. Clone the repository:
   ```bash
   git clone https://github.com/terra-sync/cnc
   ```

2. Build the project:
   ```bash
   cargo build
   ```

3. Run tests:
   ```bash
   cargo test
   ```

---

## Example Workflow

1. Create a configuration file (e.g., `config.toml`) based on the example above.
2. Run the application with verbose logging and email notifications:
   ```bash
   cnc ./config.toml --verbose --email true
   ```
3. Review the logs in `output.log` for replication details.
4. Check your email for notifications if SMTP is enabled.

---

## License

This project is licensed under the terms of both the MIT license and
the GPL-3.0. See the [LICENSE-MIT](LICENSE-MIT) and
[LICENSE-GPL-3.0](LICENSE-GPL-3.0) files for details.

---

## Acknowledgments

- **[env_logger](https://crates.io/crates/env_logger)**: For logging configuration.
- **[lettre](https://crates.io/crates/lettre)**: For SMTP email support.
- **[postgres](https://crates.io/crates/postgres)**: For interacting with PostgreSQL databases.
- **[clap](https://crates.io/crates/clap)**: For building the CLI.
- **[serde](https://crates.io/crates/serde)**: For configuration deserialization.

---

## Contributing

###  Mailing Lists

For bugs, patches, or help, please reach out through our [groups.io](https://groups.io/g/cnc):
* Post: cnc@groups.io
* Subscribe: cnc+subscribe@groups.io
* Unsubscribe: cnc+unsubscribe@groups.io
* Group Owner: cnc+owner@groups.io
* Help: cnc+help@groups.io
