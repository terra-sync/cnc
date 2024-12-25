//! Trait definitions for database operations.

use anyhow::Result;

/// Trait defining basic operations for interacting with a database.
pub trait DatabaseOperations {
    /// Establishes a connection to the database.
    ///
    /// # Returns
    ///
    /// Returns `Ok(())` if connection is successful, or an error if it fails.
    fn connect(&mut self) -> Result<(), anyhow::Error>;

    /// Closes the database connection.
    ///
    /// # Returns
    ///
    /// Returns `Ok(())` if disconnection is successful, or an error if it fails.
    fn close(&mut self) -> Result<(), anyhow::Error>;

    /// Replicates the database state from source to target.
    ///
    /// # Returns
    ///
    /// Returns `Ok(())` if replication is successful, or an error if it fails.
    fn replicate(&mut self) -> Result<(), anyhow::Error>;
}
