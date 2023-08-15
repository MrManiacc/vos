use std::any::{Any, TypeId};
use std::collections::{HashMap, HashSet};
use std::time::{SystemTime, UNIX_EPOCH};
use std::sync::{Arc, Mutex};
use rayon::ThreadPoolBuilder;
use serde::{Serialize, Deserialize, Serializer, Deserializer};


// ==========================================
// ========= VFS Node Definitions ===========
// ==========================================


/// A node within the virtual file system (VFS).
///
/// The virtual file system (VFS) is structured as a tree of nodes, where each node represents either a file or a directory.
/// This enum provides the differentiation between a file (which contains binary data) and a directory (which can have child nodes).

///
/// A node in the virtual file system can either be a file with binary data
/// or a directory which can contain other `VfsNode` objects.
///
/// # Variants
/// - `File`: Represents a file node with binary data.
/// - `Directory`: Represents a directory node with a hashmap of child nodes.
#[derive(Debug, Serialize, Deserialize)]
pub enum NodeType {
    File(Vec<u8>),
    Directory(ArcMutexHashMap),
}

impl NodeType {
    fn type_id(&self) -> TypeId {
        TypeId::of::<Self>()
    }
}

/// Represents a node within the virtual file system (VFS).
///
/// Each node in the VFS is characterized by its name, last modified timestamp, type (file or directory), and permissions.
/// The node can either be a file (with associated binary data) or a directory (which can contain child nodes).

///
/// A `VfsNode` can either be a file or a directory. It contains metadata
/// such as its name, last modified timestamp, and permissions.
///
/// # Fields
/// - `name`: The name of the node.
/// - `last_modified`: The last modified timestamp of the node.
/// - `node_type`: The type of the node (file or directory).
/// - `permissions`: The permissions associated with the node.
#[derive(Debug, Serialize, Deserialize)]
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

    pub fn new(name: String, node_type: NodeType) -> Self {
        Self {
            name,
            last_modified: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
            node_type,
            permissions: Permissions::all_users(),
        }
    }
}

// ==========================================
// ===== User, Group, and Permissions =======
// ==========================================


/// Represents a user within the virtual file system (VFS).
///
/// Each user in the VFS has a unique identifier, name, associated permissions, and can be part of multiple groups.
/// Users are essential entities in the VFS, enabling differentiated access and operations based on permissions.

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
    id: u32,
    name: String,
    permissions: u8,
    groups: ArcMutexVecGroup,
}


/// Represents a user within the virtual file system (VFS).
///
/// Each user in the VFS has a unique identifier, name, associated permissions, and can be part of multiple groups.
/// Users are essential entities in the VFS, enabling differentiated access and operations based on permissions.

///
/// A `Group` has an ID, name, and permissions. Users can be members of groups to
/// inherit group permissions.
///
/// # Fields
/// - `id`: The unique identifier of the group.
/// - `name`: The name of the group.
/// - `permissions`: The permissions granted to the group.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub struct Group {
    id: u32,
    name: String,
    permissions: u8,
}

impl ArcMutexVecGroup {
    pub fn new() -> Self {
        Self(Arc::new(Mutex::new(vec![])))
    }
}

impl User {
    /// Represents a user in the virtual file system.
    ///
    /// A `User` has an ID, name, permissions, and can be a member of multiple groups.
    ///
    /// # Fields
    /// - `id`: The unique identifier of the user.
    /// - `name`: The name of the user.
    /// - `permissions`: The permissions granted to the user.
    /// - `groups`: A list of groups the user is a member of.
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
            groups: ArcMutexVecGroup::new(),
        }
    }

    pub fn is_member_of(&self, group: &Group) -> bool {
        self.groups.0.lock().unwrap().contains(group)
    }
}


/// Represents the permissions associated with a file or directory in the VFS.
///
/// Permissions in the VFS are modelled after the UNIX style permissions system, where permissions are categorized as:
/// 1. Owner Permissions
/// 2. Group Permissions
/// 3. Other Permissions
///
/// Each category has a set of permissions represented in binary form, where:
/// - Read Permission is represented by the value: 0b100
/// - Write Permission is represented by the value: 0b010
/// - Execute Permission is represented by the value: 0b001
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Permissions {
    owner: Box<User>,
    group: Box<Group>,
    // permissions in the linux style (rwxrwxrwx)
    owner_perms: u8,
    group_perms: u8,
    other_perms: u8,
}

impl Permissions {
    /// Creates a new Permissions instance.
    ///
    ///
    /// # Parameters
    ///
    /// - `owner`: The owner of the file.
    /// - `group`: The group of the file.
    /// - 'permissions': The permissions of the file.
    ///
    /// # Returns
    ///
    /// A new Permissions instance.
    pub fn new(owner: Box<User>, group: Box<Group>, owner_perms: u8, group_perms: u8, other_perms: u8) -> Self {
        Self {
            owner,
            group,
            owner_perms,
            group_perms,
            other_perms,
        }
    }

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
    pub fn can_read(&self, user: Box<User>) -> bool {
        if user.id == *&self.owner.id {
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
    pub fn can_write(&self, user: Box<User>) -> bool {
        if user.id == *&self.owner.id {
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
    pub fn can_execute(&self, user: Box<User>) -> bool {
        if user.id == *&self.owner.id {
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
            owner: Box::from(User::new("root", 0b111)),
            group: Box::from(Group {
                id: 0,
                name: "root".to_string(),
                permissions: 0b111,
            }),
            owner_perms: 0b111,
            group_perms: 0b000,
            other_perms: 0b000,
        }
    }


    pub fn all_users() -> Self {
        Self {
            owner: Box::from(User::new("root", 0b111)),
            group: Box::from(Group {
                id: 0,
                name: "root".to_string(),
                permissions: 0b111,
            }),
            owner_perms: 0b111,
            group_perms: 0b111,
            other_perms: 0b111,
        }
    }
}

// ==========================================
// ============== Main VFS API ==============
// ==========================================

#[derive(Debug)]
pub struct VFS {
    root: ArcMutexVfsNode,
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
            root: ArcMutexVfsNode::new(VfsNode {
                name: "root".to_string(),
                last_modified: SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs(),
                node_type: NodeType::Directory(ArcMutexHashMap::new()),
                permissions: Permissions::all_users(),
            }),
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
    fn validate_path(&mut self, path: &str, user: Box<User>, permission_check: fn(&Permissions, Box<User>) -> bool) -> Result<Arc<Mutex<VfsNode>>, VfsError> {
        //if path is empty or root, return root
        if path == "" || path == "/" {
            return Ok(self.root.0.clone());
        }
        let node = self.find_node(path);
        if node.is_none() {
            return Err(VfsError::InvalidPath);
        }

        let node_guard = node.unwrap();
        if !permission_check(&node_guard.lock().unwrap().permissions, user) {
            return Err(VfsError::PermissionDenied);
        }
        Ok(node_guard)
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
    fn find_node(&mut self, path: &str) -> Option<Arc<Mutex<VfsNode>>> {
        let segments: Vec<&str> = path.split('/').collect();

        let mut current_node = self.root.0.clone();

        for segment in segments.iter().skip(1) {
            let next_node = {
                let current_node_guard = current_node.lock().unwrap();
                match &current_node_guard.node_type {
                    NodeType::Directory(children) => {
                        let children_guard = children.0.lock().unwrap();
                        if let Some(child) = children_guard.get(*segment) {
                            Some(Arc::clone(&child.0))
                        } else {
                            None
                        }
                    }
                    _ => None,
                }
            };

            match next_node {
                Some(node) => current_node = node,
                None => return None,
            }
        }

        Some(current_node)
    }
    pub fn open(&self, path: &str, user: u8) -> Result<Arc<Mutex<VfsNode>>, &'static str> {
        let segments: Vec<&str> = path.split('/').collect();

        if segments.is_empty() {
            return Err("Invalid path");
        }

        let mut current_node = Arc::clone(&self.root.0);

        for segment in segments.iter() {
            // Use a temporary variable to store the next node to avoid borrow checker issues
            let next_node: Option<Arc<Mutex<VfsNode>>>;

            {
                let node_guard = current_node.lock().unwrap();

                // Check if current node is a directory
                if node_guard.node_type.type_id() != NodeType::Directory.type_id() {
                    return Err("Invalid path");
                }

                match &node_guard.node_type {
                    NodeType::Directory(children) => {
                        let children_guard = children.0.lock().unwrap();
                        if let Some(child) = children_guard.get(*segment) {
                            next_node = Some(Arc::clone(&child.0));
                        } else {
                            return Err("Invalid path");
                        }
                    }
                    _ => return Err("Invalid path"),
                }
            }

            // Outside of the lock scope, update the current_node
            current_node = next_node.unwrap();
        }

        let node_guard = current_node.lock().unwrap();

        if !permission_check(&node_guard.permissions.to_full_permissions(), user) {
            return Err("Permission denied");
        }

        Ok(Arc::clone(&current_node))
    }

    /// Creats a directory. If the directory already exists, returns an error.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the directory.
    /// - `user`: The user attempting the operation.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn create_directory(&self, path: &str, user: u8) -> Result<(), &'static str> {
        let segments: Vec<&str> = path.split('/').collect();

        if segments.len() < 2 {
            return Err("Path too short");
        }

        let mut current_node = Arc::clone(&self.root.0);

        for segment in segments.iter().take(segments.len() - 1) {
            let next_node = {
                let node_guard = current_node.lock().unwrap();
                match &node_guard.node_type {
                    NodeType::Directory(children) => {
                        children.0.lock().unwrap().get(*segment).map(|child| Arc::clone(&child.0))
                    }
                    _ => None,
                }
            };

            if let Some(node) = next_node {
                current_node = node;
            } else {
                return Err("Invalid path");
            }
        }


        let mut node_guard = current_node.lock().unwrap();

        if node_guard.node_type.type_id() != NodeType::Directory.type_id() {
            return Err("Not a directory");
        }

        if !permission_check(&node_guard.permissions.to_full_permissions(), user) {
            return Err("Permission denied");
        }

        match &mut node_guard.node_type {
            NodeType::Directory(children) => {
                if children.0.lock().unwrap().contains_key(segments.last().cloned().unwrap()) {
                    return Err("Directory already exists");
                }

                let new_dir = VfsNode::new(segments.last().unwrap().to_string(), NodeType::Directory(ArcMutexHashMap::new()));
                children.0.lock().unwrap().insert(segments.last().unwrap().to_string(), ArcMutexVfsNode::new(new_dir));
            }
            _ => return Err("Not a directory"),
        };

        Ok(())
    }


    ///Get the size of a directory (including nested content).
    ///
    ///# Parameters
    ///
    ///- `path`: The path to get the size for.
    ///- `user`: The user attempting the operation.
    ///
    ///# Returns
    ///
    ///The size of the directory.
    pub fn get_directory_size(&mut self, path: &str, user: Box<User>) -> Result<usize, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;
        let node_guard = node.lock().unwrap();
        match &node_guard.node_type {
            NodeType::Directory(children) => {
                let children_guard = children.0.lock().unwrap();
                let mut size = 0;
                for child in children_guard.values() {
                    let guard = child.0.lock().unwrap();
                    let user = guard.permissions.owner.clone();
                    size += self.get_directory_size(&guard.name, user.clone())?;
                }
                Ok(size)
            }
            _ => Err(VfsError::InvalidOperation),
        }
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
    pub fn rename_file(&mut self, path: &str, new_name: &str, user: Box<User>) -> Result<(), VfsError> {
        let node = self.validate_path(path, user, Permissions::can_write)?;
        node.lock().unwrap().name = new_name.to_string();
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
    pub fn read_file(&mut self, path: &str, user: Box<User>) -> Result<Vec<u8>, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;
        let node_guard = node.lock().unwrap();
        match &node_guard.node_type {
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
    pub fn write_file(&mut self, path: &str, content: Vec<u8>, user: Box<User>) -> Result<(), VfsError> {
        if self.locked_files.contains(path) {
            return Err(VfsError::FileLocked);
        }

        let node = self.validate_path(path, user, Permissions::can_write)?;
        let mut node_guard = node.lock().unwrap();

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
    pub fn delete(&mut self, path: &str, user: Box<User>) -> Result<(), VfsError> {
        let segments: Vec<&str> = path.split('/').collect();
        if segments.len() < 2 {
            return Err(VfsError::InvalidOperation);  // Cannot delete root
        }

        let node_name = segments.last().unwrap();
        let parent_path = &segments[..segments.len() - 1].join("/");
        let parent_node = self.validate_path(parent_path, user, Permissions::can_write)?;
        let mut node_guard = parent_node.lock().unwrap();
        match &mut node_guard.node_type {
            NodeType::Directory(children) => {
                children.0.lock().unwrap().remove(*node_name);
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
    pub fn list_directory(&mut self, path: &str, user: Box<User>) -> Result<Vec<String>, VfsError> {
        let node = self.validate_path(path, user, Permissions::can_read)?;
        let node_guard = node.lock().unwrap();
        match &node_guard.node_type {
            NodeType::Directory(children) => Ok(children.0.lock().unwrap().keys().cloned().collect()),
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
    pub fn change_permissions(&mut self, path: &str, permissions: Permissions, user: Box<User>) -> Result<(), VfsError> {
        let node = self.validate_path(path, user, Permissions::can_write)?;
        node.lock().unwrap().permissions = permissions;
        Ok(())
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

impl Default for Permissions {
    fn default() -> Self {
        // TODO: Provide a proper default implementation
        Self {
            owner: Box::new(User {
                id: 0,
                name: "".to_string(),
                permissions: 0,
                groups: ArcMutexVecGroup::new(),
            }),
            group: Box::new(Group {
                id: 0,
                name: "".to_string(),
                permissions: 0,
            }),
            owner_perms: 0,
            group_perms: 0,
            other_perms: 0,
        }  // Assuming Permissions has no fields
    }
}

#[derive(Debug)]
pub struct ArcMutexVfsNode(Arc<Mutex<VfsNode>>);

impl ArcMutexVfsNode {
    pub fn new(node: VfsNode) -> Self {
        Self(Arc::new(Mutex::new(node)))
    }
}


impl Serialize for ArcMutexVfsNode {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for ArcMutexVfsNode {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
    {
        let node = VfsNode::deserialize(deserializer)?;
        Ok(ArcMutexVfsNode(Arc::new(Mutex::new(node))))
    }
}

// Newtype wrappers
#[derive(Debug, Clone)]
pub struct ArcMutexHashMap(Arc<Mutex<HashMap<String, ArcMutexVfsNode>>>);

impl ArcMutexHashMap {
    pub fn new() -> Self {
        Self(Arc::new(Mutex::new(HashMap::new())))
    }
}

#[derive(Debug, Clone)]
pub struct ArcMutexVecGroup(Arc<Mutex<Vec<Group>>>);


impl Serialize for ArcMutexHashMap {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for ArcMutexHashMap {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
    {
        let map = HashMap::<String, ArcMutexVfsNode>::deserialize(deserializer)?;
        Ok(ArcMutexHashMap(Arc::new(Mutex::new(map))))
    }
}

impl Serialize for ArcMutexVecGroup {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for ArcMutexVecGroup {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
    {
        let vec = Vec::<Group>::deserialize(deserializer)?;
        Ok(ArcMutexVecGroup(Arc::new(Mutex::new(vec))))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_user_creation() {
        let user = User::new("Alice", 0b111);
        assert_eq!(user.name, "Alice");
        assert_eq!(user.permissions, 0b111);
        assert!(user.groups.0.lock().unwrap().is_empty());
    }

    #[test]
    fn test_group_creation() {
        let group = Group {
            id: 1,
            name: "Admin".to_string(),
            permissions: 0b111,
        };
        assert_eq!(group.name, "Admin");
        assert_eq!(group.permissions, 0b111);
    }

    #[test]
    fn test_vfs_node_file_creation() {
        let file_node = VfsNode {
            name: "file.txt".to_string(),
            last_modified: 1629139200, // Arbitrary timestamp
            node_type: NodeType::File(b"Hello World".to_vec()),
            permissions: Permissions::default(), // Assuming a Permissions struct with a default implementation
        };
        assert_eq!(file_node.name, "file.txt");
        if let NodeType::File(data) = &file_node.node_type {
            assert_eq!(data, &b"Hello World".to_vec());
        } else {
            panic!("Node type is not File");
        }
    }

    // 1. Initialization Tests
    #[test]
    fn test_new_vfs() {
        let vfs = VFS::new();
        assert_eq!(vfs.root.0.lock().unwrap().name, "root", "Root node name does not match");
    }

    #[test]
    fn test_new_vfs_node() {
        let node = VfsNode {
            name: "test".to_string(),
            last_modified: 0,
            node_type: NodeType::File(vec![]),
            permissions: Permissions::root_only(),
        };
        assert_eq!(node.name, "test", "Node name does not match");
    }

    // 2. Directory Tests
    #[test]
    fn test_create_directory() {
        let mut vfs = VFS::new();
        let user = User::new("test", 0b111);
        let sudoer = Group {
            id: 0,
            name: "sudo".to_string(),
            permissions: 0b111,
        };
        let permissions = Permissions::new(Box::from(user.clone()), Box::from(sudoer), 0b111, 0b111, 0b111);
        vfs.create_directory("/test", 0b111).unwrap();
        assert!(vfs.find_node("/test").is_some(), "Directory was not created");
    }
}


/// Check if a user has the required permissions.
///
/// # Parameters
/// * `permissions` - Permissions of the node.
/// * `user` - The user whose permissions are being checked.
///
/// # Returns
/// `true` if the user has the required permissions, `false` otherwise.
fn permission_check(permissions: &u8, user: u8) -> bool {
    *permissions & user != 0
}
