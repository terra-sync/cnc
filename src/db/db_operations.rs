use anyhow::Result;

/// Trait defining basic operations for interacting with a database.
///
/// Types implementing this trait can be used with the `DBOperations` struct for managing database logic.
pub trait DatabaseOperations {
    /// Establishes a connection to the database.
    fn connect(&mut self) -> Result<(), anyhow::Error>;

    /// Closes the database connection.
    fn close(&mut self) -> Result<(), anyhow::Error>;

    /// Replicates the database state.
    fn replicate(&mut self) -> Result<(), anyhow::Error>;
}
