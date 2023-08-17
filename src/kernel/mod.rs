//! Kernel module containing core functionalities of the VOS.


pub mod vfs;
pub mod lua;
pub(crate) mod kern;
pub mod proc;

pub mod sys;

// More kernel functionalities can be added here.
