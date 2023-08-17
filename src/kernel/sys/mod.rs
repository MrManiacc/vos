pub mod win_proc;

use mlua::prelude::LuaError;
use crate::prelude::*;
pub use win_proc::*;

pub fn register_to_lua(lua: &LuaRuntime) -> Result<(), LuaError> {
    let table = lua.create_table();
    table.set("Window", lua.create_function(|_, ()| Ok(Window::default()))?)?;
    lua.add_table("sys", table);
    Ok(())
}


#[cfg(test)]
mod tests {
    use super::*;
    use crate::kernel::lua::LuaRuntime;

    #[test]
    fn test_lua_runtime() {
        let lua_runtime = LuaRuntime::new();
        register_to_lua(&lua_runtime).unwrap();
        let result = lua_runtime.execute_script(r#"
            local window = sys.Window();
            window.title = "Hello, World!";
            window.width = 800;
            window.height = 600;
            print(window.title .. " area " .. window:diagonal());
        "#);

        assert!(result.is_ok(), "Error: {:?}", result.unwrap_err());
    }
}