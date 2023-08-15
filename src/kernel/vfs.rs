use std::collections::{HashMap, HashSet};
use std::time::{SystemTime, UNIX_EPOCH};
use rayon::ThreadPoolBuilder;
use serde::{Serialize, Deserialize};


// ==========================================
// ========= VFS Node Definitions ===========
// ==========================================

/// Represents the type of a VFS node.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum NodeType {
    File(Vec<u8>),
    Directory(HashMap<String, Box<VfsNode>>),
}

/// Intermediate representation of VfsNode for serialization purposes.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VfsNode {
    name: String,
    last_modified: u64,
    node_type: NodeType,
    permissions: Permissions,
}

impl VfsNode {
    /// Get the last modified timestamp of the node.
    ///
    /// # Returns
    ///
    /// The last modified timestamp.
    pub fn last_modified(&self) -> u64 {
        self.last_modified
    }
}

// ==========================================
// ===== User, Group, and Permissions =======
// ==========================================

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub struct User {
    id: u32,
    name: String,
    permissions: u8,
    groups: Vec<Group>,
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub struct Group {
    id: u32,
    name: String,
    permissions: u8,
}

impl User {
    /// Creates a new user with the given name and permissions.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the user.
    /// - `permissions`: The permissions of the user.
    ///
    /// # Returns
    ///
    /// A new User instance.
    pub fn new(name: &str, permissions: u8) -> Self {
        Self {
            id: 0,
            name: name.to_string(),
            permissions,
            groups: vec![],
        }
    }

    pub fn is_member_of(&self, group: &Group) -> bool {
        self.groups.contains(group)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Permissions {
    owner: User,
    group: Group,
    owner_perms: u8,
    // rwx format, e.g., 0b111 for rwx
    group_perms: u8,
    // rwx format
    other_perms: u8,  // rwx format
}

impl Permissions {
    pub fn to_full_permissions(&self) -> u8 {
        self.owner_perms | self.group_perms | self.other_perms
    }

    /// Checks if the given user has read permission.
    ///
    /// # Parameters
    ///
    /// - `user`: The user to check permission for.
    ///
    /// # Returns
    ///
    /// True if the user has read permission, false otherwise.
    pub fn can_read(&self, user: &User) -> bool {
        if user == &self.owner {
            self.owner_perms & 0b100 == 0b100
        } else if user.is_member_of(&self.group) {
            self.group_perms & 0b100 == 0b100
        } else {
            self.other_perms & 0b100 == 0b100
        }
    }

    /// Checks if the given user has write permission.
    ///
    /// # Parameters
    ///
    /// - `user`: The user to check permission for.
    ///
    /// # Returns
    ///
    /// True if the user has write permission, false otherwise.
    pub fn can_write(&self, user: &User) -> bool {
        if user == &self.owner {
            self.owner_perms & 0b010 == 0b010
        } else if user.is_member_of(&self.group) {
            self.group_perms & 0b010 == 0b010
        } else {
            self.other_perms & 0b010 == 0b010
        }
    }

    /// Checks if the given user has execute permission.
    ///
    /// # Parameters
    ///
    /// - `user`: The user to check permission for.
    ///
    /// # Returns
    ///
    /// True if the user has execute permission, false otherwise.
    pub fn can_execute(&self, user: &User) -> bool {
        if user == &self.owner {
            self.owner_perms & 0b001 == 0b001
        } else if user.is_member_of(&self.group) {
            self.group_perms & 0b001 == 0b001
        } else {
            self.other_perms & 0b001 == 0b001
        }
    }

    /// Default root only permissions.
    ///
    /// # Returns
    ///
    /// A Permissions object with root-only access.
    pub fn root_only() -> Self {
        Self {
            owner: User::new("root", 0b111),
            group: Group {
                id: 0,
                name: "root".to_string(),
                permissions: 0b111,
            },
            owner_perms: 0b111,
            group_perms: 0b000,
            other_perms: 0b000,
        }
    }
}

// ==========================================
// ============== Main VFS API ==============
// ==========================================

#[derive(Debug)]
pub struct VFS {
    root: Box<VfsNode>,
    thread_pool: rayon::ThreadPool,
    locked_files: HashSet<String>,
    users: HashMap<String, Box<User>>,
    groups: HashMap<String, Box<Group>>,
}

impl VFS {
    /// Creates a new VFS.
    ///
    /// # Returns
    ///
    /// A new VFS instance.
    pub fn new() -> Self {
        let thread_pool = ThreadPoolBuilder::new().build().unwrap();
        Self {
            root: Box::new((VfsNode {
                name: "root".to_string(),
                last_modified: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
                node_type: NodeType::Directory(HashMap::new()),
                permissions: Permissions::root_only(),
            })),
            thread_pool,
            locked_files: HashSet::new(),
            users: Default::default(),
            groups: Default::default(),
        }
    }

    /// Helper function to validate a path and return the corresponding node.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to validate.
    /// - `user`: The user attempting the operation.
    /// - `permission_check`: The permission check function.
    ///
    /// # Returns
    ///
    /// The VFS node corresponding to the path.
    fn validate_path(&self, path: &str, user: &User, permission_check: fn(&Permissions, &User) -> bool) -> Result<Box<VfsNode>, VfsError> {
        let node = self.find_node(path);
        if node.is_none() {
            return Err(VfsError::InvalidPath);
        }

        let node_guard = node.as_ref().unwrap();
        if !permission_check(&node_guard.permissions, user) {
            return Err(VfsError::PermissionDenied);
        }

        Ok(node.unwrap().clone())
    }

    /// Helper function to find a node based on the path.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to find.
    ///
    /// # Returns
    ///
    /// The VFS node corresponding to the path.
    fn find_node(&self, path: &str) -> Option<Box<VfsNode>> {
        let segments: Vec<&str> = path.split('/').collect();
        let mut current_node = self.root.clone();
        for segment in segments.iter().skip(1) {
            let node = current_node.clone();
            match &node.node_type {
                NodeType::Directory(children) => {
                    if let Some(child) = children.get(*segment) {
                        current_node = child.clone();
                    } else {
                        return None;
                    }
                }
                _ => return None,
            }
        }
        Some(current_node)
    }

    /// Create a new directory in the VFS.
    ///
    /// # Parameters
    ///
    /// - `path`: The path where the directory should be created.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn create_directory(&mut self, path: &str, user: &User) -> Result<(), VfsError> {
        let segments: Vec<&str> = path.split('/').collect();
        let directory_name = segments.last().ok_or(VfsError::InvalidPath)?.to_string();

        let parent_path = &segments[..segments.len() - 1].join("/");
        let parent_node = self.validate_path(parent_path, user, Permissions::can_write)?;

        let new_directory = VfsNode {
            name: directory_name.clone(),
            last_modified: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            node_type: NodeType::Directory(HashMap::new()),
            permissions: Permissions::root_only(),
        };

        let mut parent_guard = parent_node.clone();
        match &mut parent_guard.node_type {
            NodeType::Directory(children) => {
                if children.contains_key(&directory_name) {
                    return Err(VfsError::InvalidOperation);
                }
                children.insert(directory_name, Box::new(new_directory));
                Ok(())
            }
            _ => Err(VfsError::InvalidPath),
        }
    }

    /// Get the size of a directory (including nested content).
    ///
    /// # Parameters
    ///
    /// - `path`: The path to get the size for.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// The size of the directory.
    pub fn get_directory_size(&self, path: &str, user: &User) -> Result<usize, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;

        fn calculate_size(node: &VfsNode) -> usize {
            match &node.node_type {
                NodeType::File(content) => content.len(),
                NodeType::Directory(children) => {
                    children.values().map(|child| {
                        let child_node = child.clone();
                        calculate_size(&child_node)
                    }).sum()
                }
            }
        }

        let node_guard = node.clone();
        Ok(calculate_size(&node_guard))
    }

    /// Rename a file.
    ///
    /// # Parameters
    ///
    /// - `path`: The path of the file to rename.
    /// - `new_name`: The new name for the file.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn rename_file(&mut self, path: &str, new_name: &str, user: &User) -> Result<(), VfsError> {
        let node = self.validate_path(path, user, Permissions::can_write)?;

        let mut node_guard = node.clone();
        node_guard.name = new_name.to_string();

        Ok(())
    }

    /// Read contents from a file.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// The contents of the file.
    pub fn read_file(&self, path: &str, user: &User) -> Result<Vec<u8>, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;
        match &node.clone().node_type {
            NodeType::File(content) => Ok(content.clone()),
            _ => Err(VfsError::InvalidOperation),
        }
    }

    /// Write contents to a file.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file.
    /// - `content`: The content to write.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn write_file(&mut self, path: &str, content: Vec<u8>, user: &User) -> Result<(), VfsError> {
        if self.locked_files.contains(path) {
            return Err(VfsError::FileLocked);
        }

        let node = self.validate_path(path, user, Permissions::can_write)?;
        let mut node_guard = node.clone();
        match &mut node_guard.node_type {
            NodeType::File(file_content) => {
                *file_content = content;
                Ok(())
            }
            _ => Err(VfsError::InvalidOperation),
        }
    }

    /// Deletes a file or directory.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file or directory.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn delete(&mut self, path: &str, user: &User) -> Result<(), VfsError> {
        let segments: Vec<&str> = path.split('/').collect();
        if segments.len() < 2 {
            return Err(VfsError::InvalidOperation);  // Cannot delete root
        }

        let node_name = segments.last().unwrap();
        let parent_path = &segments[..segments.len() - 1].join("/");
        let parent_node = self.validate_path(parent_path, user, Permissions::can_write)?;

        let mut parent_guard = parent_node.clone();
        match &mut parent_guard.node_type {
            NodeType::Directory(children) => {
                children.remove(*node_name);
                Ok(())
            }
            _ => Err(VfsError::InvalidOperation),
        }
    }

    /// Lists the contents of a directory.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the directory.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// A list of names of files and directories within the specified directory.
    pub fn list_directory(&self, path: &str, user: &User) -> Result<Vec<String>, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;
        match &node.clone().node_type {
            NodeType::Directory(children) => Ok(children.keys().cloned().collect()),
            _ => Err(VfsError::InvalidOperation),
        }
    }

    /// Locks a file for editing.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn lock_file(&mut self, path: &str) -> Result<(), VfsError> {
        if self.locked_files.contains(path) {
            Err(VfsError::FileLocked)
        } else {
            self.locked_files.insert(path.to_string());
            Ok(())
        }
    }

    /// Unlocks a file after editing.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn unlock_file(&mut self, path: &str) -> Result<(), VfsError> {
        if self.locked_files.contains(path) {
            self.locked_files.remove(path);
            Ok(())
        } else {
            Err(VfsError::InvalidOperation)
        }
    }

    /// Adds a user to the VFS.
    ///
    /// # Parameters
    ///
    /// - `user`: The user to be added.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn add_user(&mut self, user: User) -> Result<(), VfsError> {
        // For simplicity, we're using a HashSet, but in a real-world scenario, a dedicated database or structure would be used.
        if self.users.contains_key(&user.name) {
            Err(VfsError::InvalidOperation)
        } else {
            self.users.insert(user.name.clone(), Box::new(user));
            Ok(())
        }
    }

    /// Removes a user from the VFS.
    ///
    /// # Parameters
    ///
    /// - `user`: The user to be removed.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn remove_user(&mut self, user: &User) -> Result<(), VfsError> {
        if self.users.remove(user.name.as_str()).is_some() {
            Ok(())
        } else {
            Err(VfsError::InvalidOperation)
        }
    }

    /// Adds a group to the VFS.
    ///
    /// # Parameters
    ///
    /// - `group`: The group to be added.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn add_group(&mut self, group: Group) -> Result<(), VfsError> {
        if self.groups.contains_key(&group.name) {
            Err(VfsError::InvalidOperation)
        } else {
            self.groups.insert(group.name.clone(), Box::new(group));
            Ok(())
        }
    }

    /// Removes a group from the VFS.
    ///
    /// # Parameters
    ///
    /// - `group`: The group to be removed.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn remove_group(&mut self, group: &Group) -> Result<(), VfsError> {
        if self.groups.remove(group.name.as_str()).is_some() {
            Ok(())
        } else {
            Err(VfsError::InvalidOperation)
        }
    }

    /// Changes the permissions of a file or directory.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file or directory.
    /// - `permissions`: The new permissions to be set.
    /// - `user`: The user attempting the operation (should have necessary permissions).
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn change_permissions(&mut self, path: &str, permissions: Permissions, user: &User) -> Result<(), VfsError> {
        let node = self.validate_path(path, user, |p, u| p.owner == *u || u.is_member_of(&p.group))?;
        let mut node_guard = node.clone();

        if node_guard.permissions.owner == *user || user.is_member_of(&node_guard.permissions.group) {
            node_guard.permissions = permissions;
            Ok(())
        } else {
            Err(VfsError::PermissionDenied)
        }
    }
}

#[derive(Debug)]
pub enum VfsError {
    InvalidPath,
    PermissionDenied,
    InvalidOperation,
    FileLocked,
}

// fn main() {
//     // Test and usage code can be added here.
// }

#[cfg(test)]
mod tests {
    use super::*;

    // 1. Initialization Tests
    #[test]
    fn test_new_vfs() {
        let vfs = VFS::new();
        assert_eq!(vfs.root.name, "root", "Root node name does not match");
    }

    #[test]
    fn test_new_vfs_node() {
        let file_node = VfsNode::new_file("test_file".to_string(), vec![0, 1, 2]);
        match &file_node.node_type {
            NodeType::File(data) => assert_eq!(data, &vec![0, 1, 2], "File data does not match"),
            _ => panic!("Expected a file node"),
        }

        let dir_node = VfsNode::new_directory("test_dir".to_string());
        match &dir_node.node_type {
            NodeType::Directory(_) => (),
            _ => panic!("Expected a directory node"),
        }
    }
}
