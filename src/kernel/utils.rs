use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use crate::kernel::vfs::VfsNode;

#[derive(Debug)]
pub struct Amw<T>(Arc<Mutex<T>>);

impl<T> Amw<T> {
    pub fn new(data: T) -> Self {
        Amw(Arc::new(Mutex::new(data)))
    }

    pub fn lock(&self) -> std::sync::LockResult<std::sync::MutexGuard<'_, T>> {
        self.0.lock()
    }
}

impl<T> Clone for Amw<T> {
    fn clone(&self) -> Self {
        Amw(self.0.clone())
    }
}

impl<T: Serialize> Serialize for Amw<T> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer,
    {
        let locked_data = self.lock().unwrap();
        locked_data.serialize(serializer)
    }
}

impl<'de, T: Deserialize<'de>> Deserialize<'de> for Amw<T> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: Deserializer<'de>,
    {
        let data = T::deserialize(deserializer)?;
        Ok(Amw::new(data))
    }
}

pub type NodeRef = Amw<VfsNode>;
pub type NodeMap = Amw<HashMap<String, NodeRef>>;
pub type IdListRef = Amw<Vec<u16>>;

