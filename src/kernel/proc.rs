use std::any::Any;
use std::sync::{Arc, Mutex};
use std::ops::Deref;

pub type PID = u32;

pub type ProcessRef = Arc<Mutex<dyn Process>>;

pub type ProcessContextRef = Arc<Mutex<ProcessContext>>;

pub type ProcessHostRef = Arc<Mutex<ProcessHost>>;

pub struct ProcessHost {
    process_ref: ProcessRef,
    pub(crate) process_context: ProcessContextRef,
    process_name: String,
}

pub struct ProcessRunCommands {
    commands: Vec<Box<dyn FnOnce(&dyn Any) -> ()>>,
}

impl ProcessHost {
    pub fn new(process_name: String, process_ref: ProcessRef) -> Self {
        Self {
            process_ref,
            process_context: ProcessContextRef::new(Mutex::new(ProcessContext {
                linked_processes: Vec::new(),
                child_processes: Vec::new(),
                parent_process: None,
                process_id: 0,
            })),
            process_name,
        }
    }

    pub fn run(&self) {
        let locked_process_ref = self.process_ref.lock().unwrap();
        let locked_context = self.process_context.lock().unwrap();
        locked_process_ref.run(&locked_context);
    }
}

/// Stores all the information about a process. This is the context of a process. We can use this to link to the process's memory,
/// or to the process's file descriptors, or to the process's threads, etc.
pub struct ProcessContext {
    pub(crate) linked_processes: Vec<PID>,
    pub(crate) child_processes: Vec<PID>,
    pub(crate) parent_process: Option<PID>,
    pub(crate) process_id: PID,
}

pub trait Process {
    fn run(&self, context: &ProcessContext);
}
