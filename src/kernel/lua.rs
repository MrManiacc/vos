//! Lua Runtime to execute Lua scripts.

// For the sake of example, we'll just outline the basic structure.

use mlua::{Function, Table, UserData};
use mlua::prelude::*;


/// Represents the Lua Runtime.
pub struct LuaRuntime {
    // Actual Lua runtime data structures go here.
    // For simplicity, we will leave it empty for now.
    pub(crate) lua: Lua,
}

impl LuaRuntime {
    // Creates a new LuaRuntime.
    pub fn new() -> Self {
        Self {
            // Initialization goes here.
            lua: Lua::new(),
        }
    }

    pub fn create_function<'lua, A, R, F>(&'lua self, function: F) -> Result<Function<'lua>, LuaError>
        where
            A: FromLuaMulti<'lua>,
            R: IntoLuaMulti<'lua>,
            F: 'static + Send + Sync + Fn(&'lua Lua, A) -> Result<R, LuaError>,
    {
        self.lua.create_function(function)
    }

    pub fn create_table(&self) -> Table {
        self.lua.create_table().unwrap()
    }

    pub fn add_table(&self, name: &str, table: Table) {
        self.lua.globals().set(name, table).unwrap();
    }

    /// Adds a function to the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the function.
    /// - `function`: The function to be added.
    ///
    /// # Returns
    ///
    /// Returns a Result indicating the success or failure of the function addition.
    pub fn add_function(&self, name: &str, function: Function) {
        self.lua.globals().set(name, function).unwrap();
    }

    /// Gets a string from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the string to get.
    ///
    /// # Returns
    ///
    /// Returns the string.
    pub fn get_string(&self, name: &str) -> String {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_f64(&self, name: &str) -> f64 {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_f32(&self, name: &str) -> f32 {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_i64(&self, name: &str) -> i64 {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_i32(&self, name: &str) -> i32 {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a table from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the table to get.
    ///
    /// # Returns
    ///
    /// Returns the table.
    pub fn get_table(&self, name: &str) -> Table {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a boolean from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the boolean to get.
    ///
    /// # Returns
    ///
    /// Returns the boolean.
    pub fn get_bool(&self, name: &str) -> bool {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_usize(&self, name: &str) -> usize {
        self.lua.globals().get(name).unwrap()
    }

    /// Gets a number from the Lua runtime.
    ///
    /// # Parameters
    ///
    /// - `name`: The name of the number to get.
    ///
    /// # Returns
    ///
    /// Returns the number.
    pub fn get_u64(&self, name: &str) -> u64 {
        self.lua.globals().get(name).unwrap()
    }

    /// Executes a Lua script.
    ///
    /// # Parameters
    ///
    /// - `script`: The Lua script to be executed.
    ///
    /// # Returns
    ///
    /// Returns a Result indicating the success or failure of the script execution.
    pub fn execute_script(&self, script: &str) -> Result<(), LuaError> {
        let result = self.lua.load(script).exec();
        match result {
            Ok(_) => Ok(()),
            Err(_) => Err(result.unwrap_err()),
        }
    }
}


#[cfg(test)]
mod tests {
    use crate::kernel::vfs::User;
    use crate::prelude::VFS;
    use super::*;

    #[test]
    fn test_lua_runtime() {
        let lua_runtime = LuaRuntime::new();
        let result = lua_runtime.execute_script("meaning = 43");
        assert!(result.is_ok());
        let result = lua_runtime.execute_script("meaning = meaning + 1");
        assert!(result.is_ok());
        let result = lua_runtime.execute_script("assert(meaning == 44)");
        assert!(result.is_ok());
    }


    #[test]
    fn test_lua_runtime_error() {
        let lua_runtime = LuaRuntime::new();
        let result = lua_runtime.execute_script("meaning = meaning + 1");
        assert!(result.is_err());
    }


    #[test]
    fn test_lua_initialization() {
        let lua_runtime = LuaRuntime::new();
        lua_runtime.add_function("good", lua_runtime.lua.create_function(|_, ()| {
            println!("good day to you!");
            Ok(())
        }).unwrap());

        let _ = lua_runtime.execute_script("good()");
    }

    #[test]
    fn test_for_lua_std_library() {
        let lua_runtime = LuaRuntime::new();
        let result = lua_runtime.execute_script("print('hello world')");
        assert!(result.is_ok());
    }

    #[test]
    fn test_get_string() {
        let lua_runtime = LuaRuntime::new();
        let result = lua_runtime.execute_script("meaning = 'hello'");
        assert!(result.is_ok());
        let result = lua_runtime.get_string("meaning");
        assert_eq!(result, "hello");
    }

    #[test]
    fn test_loading_lua_from_vfs() {
        let mut vfs = VFS::new();
        //first check if file exists on file system using std::fs
        //if it does not exist, read it from the disk
        //then deserialize it into the vfs

        if std::fs::metadata("scripts/os.bin").is_ok() {
            vfs.deserialize_from_file("scripts/os.bin").expect("Failed to deserialize from file");
        }
        let lua_runtime = LuaRuntime::new();
        let user = vfs.add_user(User::new("test", 0b111)).unwrap();

        if !vfs.exists("test.lua", user.clone()) {
            vfs.read_file_from_disk("scripts/test.lua", "/test.lua").unwrap();
        }
        assert!(vfs.exists("test.lua", user.clone()));
        let script = vfs.read_file("test.lua", user.clone()).unwrap();
        //convert the script to a string
        let script = String::from_utf8(script).unwrap();
        let result = lua_runtime.execute_script(script.as_str());
        assert!(result.is_ok());

        vfs.serialize_to_file("scripts/os.bin").expect("Failed to serialize to file");
    }
}