//! Lua Runtime to execute Lua scripts.

// For the sake of example, we'll just outline the basic structure.

use mlua::prelude::*;

pub enum LuaError {
    // Actual error types will go here.
    ScriptExecutionFailed
}

/// Represents the Lua Runtime.
pub struct LuaRuntime {
    // Actual Lua runtime data structures go here.
    // For simplicity, we will leave it empty for now.
    lua: Lua,
}

impl LuaRuntime {
    /// Creates a new LuaRuntime.
    pub fn new() -> Self {
        Self {
            // Initialization goes here.
            lua: LuaRuntime::initialize_lua(),
        }
    }


    fn hello(_: &Lua, name: String) -> LuaResult<()> {
        println!("hello, {}!", name);
        Ok(())
    }

    /// Internally initializes the Lua runtime. This is where you would register
    /// functions, types, etc.
    ///
    /// # Returns
    ///
    /// Returns a Lua instance.
    fn initialize_lua() -> Lua {
        let lua = Lua::new();
        lua.globals().set("hello", lua.create_function(LuaRuntime::hello).unwrap()).unwrap();
        lua
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
            Err(_) => Err(LuaError::ScriptExecutionFailed),
        }
    }
}


#[cfg(test)]
mod tests {
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
    fn hello_test(){
        let lua_runtime = LuaRuntime::new();
        let result = lua_runtime.execute_script("hello('world')");
        assert!(result.is_ok());
    }
}