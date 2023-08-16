use serde::{Deserialize, Serialize};
use crate::kernel::utils::*;
///
/// A `Group` has an ID, name, and permissions. Users can be members of groups to
/// inherit group permissions.
///
/// # Fields
/// - `id`: The unique identifier of the group.
/// - `name`: The name of the group.
/// - `permissions`: The permissions granted to the group.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct User {
    id: u16,
    name: String,
    permissions: u8,
    groups: IdListRef,
}



