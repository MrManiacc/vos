pub mod kernel;

pub mod prelude {
    // VOS Prelude
    // Re-export core functionalities for easier access throughout the project.
    //various modules
    pub mod ext {
        pub use std::collections::{HashMap, HashSet};
        pub use std::path::Path;
        pub use std::time::{SystemTime, UNIX_EPOCH};
        pub use std::sync::{Arc, Mutex};
        pub use serde::{Serialize, Deserialize, Serializer, Deserializer};
    }

    pub mod lua {
        pub use mlua::*;
    }


    pub use crate::kernel::{vfs::VFS, lua::LuaRuntime, kern::Kernel, proc::ProcessHost, proc::Process};
}