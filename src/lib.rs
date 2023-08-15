pub mod kernel;
pub mod user;
pub mod utils;
pub mod prelude {
    // VOS Prelude
    // Re-export core functionalities for easier access throughout the project.

    pub use crate::kernel::{vfs::VFS, window_manager::WindowManager, graphics_lib::GraphicsLib};
    pub use crate::user::{lua_runtime::LuaRuntime, api::{vfs_api, graphics_api}};
    pub use crate::utils::*;
}