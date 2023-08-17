use crate::prelude::*;


// ==========================================
// ========= VFS Node Definitions ===========
// ==========================================


/// A node within the virtual file system (VFS).
///
/// The virtual file system (VFS) is structured as a tree of nodes, where each node represents either a file or a directory.
/// This enum provides the differentiation between a file (which contains binary data) and a directory (which can have child nodes).
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
#[derive(Debug, ext::Serialize, ext::Deserialize)]
pub struct VfsNode {
    name: String,
    last_modified: u64,
    children: ArcMutexHashMap,
    contents: Vec<u8>,
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

    pub fn new(name: String) -> Self {
        Self {
            name,
            last_modified: ext::SystemTime::now().duration_since(ext::UNIX_EPOCH).unwrap().as_secs(),
            children: ArcMutexHashMap::new(),
            contents: vec![],
            permissions: Permissions::public_perms(),
        }
    }

    pub fn get_child(&self, name: &str) -> Option<ext::Arc<ext::Mutex<VfsNode>>> {
        let lock = self.children.0.lock().unwrap();
        let result = lock.get(name).map(|node| node.0.clone());
        result
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
#[derive(Debug, Clone, ext::Serialize, ext::Deserialize)]
pub struct User {
    id: u16,
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
#[derive(Debug, Clone, ext::Serialize, ext::Deserialize, PartialEq, Eq, Hash)]
pub struct Group {
    id: u16,
    name: String,
    permissions: u8,
}


impl Group {
    pub fn new(name: &str, permissions: u8, id: u16) -> Self {
        Self {
            id,
            name: name.to_string(),
            permissions,
        }
    }
}

impl ArcMutexVecGroup {
    pub fn new() -> Self {
        Self(ext::Arc::new(ext::Mutex::new(vec![])))
    }
}

impl Default for User {
    fn default() -> Self {
        Self {
            id: 0,
            name: "root".to_string(),
            permissions: 0b111,
            groups: ArcMutexVecGroup::new(),
        }
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

    pub fn is_member_of_any(&self, group_ids: &Vec<u16>) -> bool {
        let groups = self.groups.0.lock().unwrap();
        for group_id in group_ids {
            if groups.contains(group_id) {
                return true;
            }
        }
        false
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
#[derive(Debug, Clone, ext::Serialize, ext::Deserialize)]
pub struct Permissions {
    owner_id: u16,
    group_ids: Vec<u16>,
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
    pub fn new(owner_id: u16, group_ids: Vec<u16>, owner_perms: u8, group_perms: u8, other_perms: u8) -> Self {
        Self {
            owner_id,
            group_ids,
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
        if user.id == self.owner_id {
            self.owner_perms & 0b100 == 0b100
        } else if user.is_member_of_any(&self.group_ids) {
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
        if user.id == self.owner_id {
            self.owner_perms & 0b010 == 0b010
        } else if user.is_member_of_any(&self.group_ids) {
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
        if user.id == self.owner_id {
            self.owner_perms & 0b001 == 0b001
        } else if user.is_member_of_any(&self.group_ids) {
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
    pub fn private_perms_for_root() -> Self {
        Self {
            owner_id: 0,
            group_ids: vec![],
            owner_perms: 0b111,
            group_perms: 0b000,
            other_perms: 0b000,
        }
    }

    pub fn public_perms() -> Self {
        Self {
            owner_id: 0,
            group_ids: vec![],
            owner_perms: 0b111,
            group_perms: 0b111,
            other_perms: 0b111,
        }
    }

    pub fn private_perms_for(user: Box<User>) -> Self {
        Self {
            owner_id: user.id,
            group_ids: vec![],
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
    root: ArcMutexVfsNode,
    locked_files: ext::HashSet<String>,
    users: ext::HashMap<String, Box<User>>,
    groups: ext::HashMap<String, Box<Group>>,
}

impl VFS {
    /// Creates a new VFS.
    ///
    /// # Returns
    ///
    /// A new VFS instance.
    pub fn new() -> Self {
        Self {
            root: ArcMutexVfsNode::new(VfsNode {
                name: "/".to_string(),
                last_modified: ext::SystemTime::now().duration_since(ext::UNIX_EPOCH).unwrap().as_secs(),
                children: ArcMutexHashMap::new(),
                contents: vec![],
                permissions: Permissions::public_perms(),
            }),
            locked_files: ext::HashSet::new(),
            users: Default::default(),
            groups: Default::default(),
        }
    }

    pub fn new_ref() -> ext::Arc<ext::Mutex<Self>> {
        return ext::Arc::new(ext::Mutex::new(VFS::new()));
    }

    /// Checks if a file or directory exists.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file or directory.
    ///
    /// # Returns
    ///
    /// True if the file or directory exists, false otherwise.
    pub fn exists(&self, path: &str, user: Box<User>) -> bool {
        let node = self.validate_path(path, user, Permissions::can_read);
        node.is_ok()
    }

    pub fn serialize_to_file(&self, path: &str) -> Result<(), VfsError> {
        let serialized = bincode::serialize(&self.root).unwrap();
        std::fs::write(path, serialized).unwrap();
        Ok(())
    }

    pub fn deserialize_from_file(&mut self, path: &str) -> Result<(), VfsError> {
        let serialized = std::fs::read(path).unwrap();
        self.root = bincode::deserialize(&serialized).unwrap();
        Ok(())
    }

    pub fn try_deserialize_from_file(&mut self, path: &str) -> Result<(), VfsError> {
        let serialized = std::fs::read(path);
        if serialized.is_err() {
            return Err(VfsError::InvalidOperation);
        }
        self.root = bincode::deserialize(&serialized.unwrap()).unwrap();
        Ok(())
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
    fn validate_path(&self, path: &str, user: Box<User>, permission_check: fn(&Permissions, Box<User>) -> bool) -> Result<ext::Arc<ext::Mutex<VfsNode>>, VfsError> {
        //if path is empty or root, return root
        if path == "" || path == "/" {
            return Ok(self.root.0.clone());
        }
        let node = self.find_node(path);
        if node.is_err() {
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
    // Updated find_node method
    fn find_node(&self, path: &str) -> Result<ext::Arc<ext::Mutex<VfsNode>>, VfsError> {
        //if path is empty or root, return root
        if path == "" || path == "/" {
            return Ok(self.root.0.clone());
        }
        let components: Vec<&str> = path.split('/').filter(|s| !s.is_empty()).collect();
        let mut current_node: Option<ext::Arc<ext::Mutex<VfsNode>>> = Some(self.root.0.clone());

        for component in components {
            let children = current_node.as_ref().unwrap().lock().unwrap().children.clone();
            if !children.0.lock().unwrap().contains_key(component) {
                return Err(VfsError::InvalidPath);
            }
            current_node = Some(children.0.lock().unwrap().get(component).unwrap().0.clone());
        }

        Ok(current_node.unwrap())
    }

    pub fn create_file(&self, path: &str, permissions: Permissions) -> Result<ext::Arc<ext::Mutex<VfsNode>>, VfsError> {
        println!("Creating file: {}", path);
        if path == "" || path == "/" || path.contains("//") || !path.starts_with("/") {
            return Err(VfsError::InvalidPath);
        }
        let path = ext::Path::new(path);
        // let parent_path = path.split('/').take(path.split('/').count() - 1).collect::<Vec<&str>>().join("/");
        let parent_path = path.parent().unwrap().to_str().unwrap();
        let parent_node = self.find_node(&parent_path);

        let binding = parent_node.unwrap();
        let node_guard = binding.lock().unwrap();
        let name = path.file_name().unwrap().to_str().unwrap();
        //create our file node
        let new_node = ArcMutexVfsNode::new(VfsNode {
            name: name.to_string(),
            last_modified: ext::SystemTime::now().duration_since(ext::UNIX_EPOCH).unwrap().as_secs(),
            children: ArcMutexHashMap::new(),
            contents: vec![],
            permissions,
        });
        let new_node_binding = new_node.0.clone();
        let binding = node_guard.children.clone();
        let mut child_lock = binding.0.lock().unwrap();
        child_lock.insert(path.file_name().unwrap().to_str().unwrap().to_string(), new_node);
        return Ok(new_node_binding);
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
        let node = self.validate_path(path, user.clone(), Permissions::can_read)?;
        let node_guard = node.lock().unwrap();
        //recursivley get the size of the directory

        let mut size = 0;
        for child in node_guard.children.0.lock().unwrap().values() {
            let child_guard = child.0.lock().unwrap();
            size += self.get_directory_size(&format!("{}/{}", path, child_guard.name), user.clone())?;
        }

        Ok(size)
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
        Ok(node.clone().lock().unwrap().contents.clone())
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
        Ok(node_guard.contents = content)
    }

    /// Reads a file from native file system and writes it to the VFS.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the on the native file system.
    /// - `vfs_path`: The path to the file in the VFS.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn read_file_from_disk(&mut self, path: &str, vfs_path: &str) -> Result<(), VfsError> {
        let content = std::fs::read(path).unwrap();
        let node = self.create_file(vfs_path, Permissions::public_perms())?;
        let mut node_guard = node.lock().unwrap();
        Ok(node_guard.contents = content)
    }

    /// Writes a file from the VFS to the native file system.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file on the native file system.
    /// - `vfs_path`: The path to the file in the VFS.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn write_file_to_disk(&mut self, path: &str, vfs_path: &str) -> Result<(), VfsError> {
        let node = self.find_node(vfs_path)?;
        let node_guard = node.lock().unwrap();
        std::fs::write(path, node_guard.contents.clone()).unwrap();
        Ok(())
    }

    /// Copies a file from one location to another.
    ///
    /// # Parameters
    ///
    /// - `path`: The path to the file to copy.
    /// - `new_path`: The path to the new file.
    ///
    /// # Returns
    ///
    /// Result indicating the operation's success.
    pub fn copy_file(&mut self, _path: &str, new_path: &str, user: Box<User>) -> Result<(), VfsError> {
        let path = ext::Path::new(_path);
        let new_path = ext::Path::new(new_path);
        let node = self.validate_path(_path, user.clone(), Permissions::can_read)?;
        let node_guard = node.lock().unwrap();
        let new_node = ArcMutexVfsNode::new(VfsNode {
            name: path.file_name().unwrap().to_str().unwrap().to_string(),
            last_modified: ext::SystemTime::now().duration_since(ext::UNIX_EPOCH).unwrap().as_secs(),
            children: ArcMutexHashMap::new(),
            contents: node_guard.contents.clone(),
            permissions: node_guard.permissions.clone(),
        });
        let parent_path = new_path.parent().unwrap().to_str().unwrap();
        let parent_node = self.find_node(&parent_path);
        let binding = parent_node.unwrap();
        let node_guard = binding.lock().unwrap();
        let name = new_path.file_name().unwrap().to_str().unwrap().to_string();
        let binding = node_guard.children.clone();
        let mut child_lock = binding.0.lock().unwrap();
        child_lock.insert(name, new_node);
        Ok(())
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
    pub fn delete(&mut self, path: &str, user: Box<User>) -> Result<ArcMutexVfsNode, VfsError> {
        //validate the path permissions for the user
        let validated_lookup = self.validate_path(path, user.clone(), Permissions::can_write);
        if validated_lookup.is_err() {
            return Err(validated_lookup.err().unwrap());
        }
        let segments: Vec<&str> = path.split('/').collect();
        if segments.len() < 2 {
            return Err(VfsError::InvalidOperation);  // Cannot delete root
        }
        let node_name = segments.last().unwrap();
        let parent_path = &segments[..segments.len() - 1].join("/");
        let parent_node = self.validate_path(parent_path, user, Permissions::can_write)?;
        let node_guard = parent_node.lock().unwrap();
        let mut child_guard = node_guard.children.0.lock().unwrap();

        let result = Ok(child_guard.remove(node_name.to_string().as_str()).ok_or(VfsError::InvalidOperation)?);
        result
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
        let mut children = vec![];
        for child in node_guard.children.0.lock().unwrap().values() {
            let child_guard = child.0.lock().unwrap();
            children.push(child_guard.name.clone());
        }
        Ok(children)
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
    pub fn add_user(&mut self, user: User) -> Result<Box<User>, VfsError> {
        // For simplicity, we're using a HashSet, but in a real-world scenario, a dedicated database or structure would be used.
        if self.users.contains_key(&user.name) {
            Err(VfsError::InvalidOperation)
        } else {
            let mut user = Box::new(user);
            self.users.insert(user.name.clone(), user.clone());
            //update the user id
            if user.id != 0 {
                return Ok(user);
            }
            user.id = self.users.len() as u16 - 1;
            Ok(user.clone())
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
            owner_id: 0,
            group_ids: vec![],
            owner_perms: 0b111,
            group_perms: 0b000,
            other_perms: 0b000,
        }  // Assuming Permissions has no fields
    }
}

#[derive(Debug)]
pub struct ArcMutexVfsNode(ext::Arc<ext::Mutex<VfsNode>>);

impl ArcMutexVfsNode {
    pub fn new(node: VfsNode) -> Self {
        Self(ext::Arc::new(ext::Mutex::new(node)))
    }
}

impl ext::Serialize for ArcMutexVfsNode {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: ext::Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> ext::Deserialize<'de> for ArcMutexVfsNode {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: ext::Deserializer<'de>,
    {
        let node = VfsNode::deserialize(deserializer)?;
        Ok(ArcMutexVfsNode(ext::Arc::new(ext::Mutex::new(node))))
    }
}

// Newtype wrappers
#[derive(Debug, Clone)]
pub struct ArcMutexHashMap(ext::Arc<ext::Mutex<ext::HashMap<String, ArcMutexVfsNode>>>);

impl ArcMutexHashMap {
    pub fn new() -> Self {
        Self(ext::Arc::new(ext::Mutex::new(ext::HashMap::new())))
    }
}

#[derive(Debug, Clone)]
pub struct ArcMutexVecGroup(ext::Arc<ext::Mutex<Vec<u16>>>);

impl ext::Serialize for ArcMutexHashMap {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: ext::Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> ext::Deserialize<'de> for ArcMutexHashMap {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: ext::Deserializer<'de>,
    {
        let map = ext::HashMap::<String, ArcMutexVfsNode>::deserialize(deserializer)?;
        Ok(ArcMutexHashMap(ext::Arc::new(ext::Mutex::new(map))))
    }
}

impl ext::Serialize for ArcMutexVecGroup {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: ext::Serializer,
    {
        self.0.lock().unwrap().serialize(serializer)
    }
}

impl<'de> ext::Deserialize<'de> for ArcMutexVecGroup {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: ext::Deserializer<'de>,
    {
        let vec = Vec::<u16>::deserialize(deserializer)?;
        Ok(ArcMutexVecGroup(ext::Arc::new(ext::Mutex::new(vec))))
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
        let group = Group::new("Admin", 0b111, 1);
        assert_eq!(group.name, "Admin");
        assert_eq!(group.permissions, 0b111);
    }

    #[test]
    fn test_vfs_node_file_creation() {
        let file_node = VfsNode {
            name: "file.txt".to_string(),
            last_modified: 1629139200, // Arbitrary timestamp
            children: ArcMutexHashMap::new(),
            contents: vec![],
            permissions: Permissions::default(), // Assuming a Permissions struct with a default implementation
        };
        assert_eq!(file_node.name, "file.txt");
        assert_eq!(file_node.last_modified, 1629139200);
    }

    // 1. Initialization Tests
    #[test]
    fn test_new_vfs() {
        let vfs = VFS::new();
        assert_eq!(vfs.root.0.lock().unwrap().name, "/", "Root node name does not match");

        let child_lock = vfs.root.0.lock().unwrap();
        let root_children = child_lock.children.0.lock().unwrap();
        assert_eq!(root_children.len(), 0, "Root node should not have any children");
    }

    #[test]
    fn test_new_file_node() {
        let vfs = VFS::new();
        // assert_eq!(vfs.root.0.lock().unwrap().name, "/", "Root node name does not match");
        let perms = Permissions::default();
        let result = vfs.create_file("/blah", perms);
        assert!(result.is_ok(), "File was not created, error: {:?}", result.err());
        //
        let child_lock = vfs.root.0.lock().unwrap();
        let root_children = child_lock.children.0.lock().unwrap();
        assert_eq!(root_children.len(), 1, "Root node should have one child");
    }

    #[test]
    fn test_nested_file_node() {
        let vfs = VFS::new();
        // assert_eq!(vfs.root.0.lock().unwrap().name, "/", "Root node name does not match");
        let result = vfs.create_file("/blah", Permissions::default());
        assert!(result.is_ok(), "File was not created, error: {:?}", result.err());
        let result = vfs.create_file("/blah/blah2", Permissions::default());
        assert!(result.is_ok(), "File was not created, error: {:?}", result.err());
    }


    #[test]
    fn test_file_no_permission_delete() {
        let mut vfs = VFS::new();
        let root_user = vfs.add_user(User::new("root", 0b111)).unwrap();
        let regular_user = vfs.add_user(User::new("regular", 0b100)).unwrap();
        vfs.create_file("/test", Permissions::private_perms_for_root()).unwrap();
        let result = vfs.delete("/test", regular_user);
        assert!(result.is_err(), "Got error {:?} when deleting file", result.err());
        let result = vfs.delete("/test", root_user);
        assert!(result.is_ok(), "Got error {:?} when deleting file", result.err());
    }

    #[test]
    fn test_serialization() {
        let mut vfs = VFS::new();
        let _root_user = vfs.add_user(User::new("root", 0b111)).unwrap();
        let _regular_user = vfs.add_user(User::new("regular", 0b100)).unwrap();
        vfs.create_file("/test", Permissions::private_perms_for_root()).unwrap();
        vfs.serialize_to_file("test.bin").unwrap();
        let mut new_vfs = VFS::new();
        new_vfs.deserialize_from_file("test.bin").unwrap();
        assert!(new_vfs.find_node("/test").is_ok(), "Failed to deserialize file");
    }

    #[test]
    fn test_writing_of_file() {
        let mut vfs = VFS::new();
        let root_user = vfs.add_user(User::new("root", 0b111)).unwrap();

        let result = vfs.create_file("/test", Permissions::private_perms_for_root());
        assert!(result.is_ok(), "Got error {:?} when creating file", result.err());
        let result = vfs.write_file("/test", vec![1, 2, 3], root_user);
        assert!(result.is_ok(), "Got error {:?} when writing to file", result.err());
    }

    #[test]
    fn test_reading_of_file() {
        let mut vfs = VFS::new();
        let root_user = vfs.add_user(User::new("root", 0b111)).unwrap();

        let result = vfs.create_file("/test", Permissions::private_perms_for_root());
        assert!(result.is_ok(), "Got error {:?} when creating file", result.err());
        let result = vfs.write_file("/test", vec![1, 2, 3], root_user.clone());
        assert!(result.is_ok(), "Got error {:?} when writing to file", result.err());

        let result = vfs.read_file("/test", root_user);

        assert!(result.is_ok(), "Got error {:?} when reading file", result.err());

        let contents = result.unwrap();
        assert_eq!(contents, vec![1, 2, 3], "File contents do not match");
    }

    #[test]
    fn test_reading_file_from_disk() {
        let mut vfs = VFS::new();
        let user = vfs.add_user(User::new("root", 0b111)).unwrap();
        //If file doesn't exists on disk, this will fail so we need to create it and add "hello, from the disk!" to it
        if std::fs::metadata("tests/test.txt").is_err() {
            std::fs::write("tests/test.txt", "hello, from the disk!").unwrap();
        }
        vfs.read_file_from_disk("tests/test.txt", "/test.txt").unwrap();

        let result = vfs.read_file("/test.txt", user);
        assert!(result.is_ok(), "Got error {:?} when reading file", result.err());

        let contents = result.unwrap();
        //the contents of test.txt are "hello, from the disk!"
        assert_eq!(contents, vec![104, 101, 108, 108, 111, 44, 32, 102, 114, 111, 109, 32, 116, 104, 101, 32, 100, 105, 115, 107, 33], "File contents do not match");

        //test that we can write to the file
        vfs.write_file_to_disk("test_2.txt", "/test.txt").unwrap();
    }

    #[test]
    fn test_copying_file() {
        let mut vfs = VFS::new();

        let _user = vfs.add_user(User::new("root", 0b111)).unwrap();
        vfs.read_file_from_disk("tests/test.txt", "/test.txt").unwrap();

        vfs.copy_file("/test.txt", "/test_2.txt", _user.clone()).unwrap();

        let result = vfs.read_file("/test_2.txt", _user);
        assert!(result.is_ok(), "Got error {:?} when reading file", result.err());

        let contents = result.unwrap();
        //the contents of test.txt are "hello, from the disk!"
        assert_eq!(contents, vec![104, 101, 108, 108, 111, 44, 32, 102, 114, 111, 109, 32, 116, 104, 101, 32, 100, 105, 115, 107, 33], "File contents do not match");
    }
}

