use std::sync::{Arc, Mutex};
use crate::prelude::*;
use serde::{Deserialize, Deserializer, Serialize, Serializer};
use crate::kernel::proc::*;

type VfsRef = Arc<Mutex<VFS>>;

type KernelRef = Arc<Mutex<Kernel>>;

type ProcessTableRef = Arc<Mutex<ProcessTable>>;

pub enum KernalError {
    ProcessNotFound(PID),
    ProcessAlreadyLinked(PID, PID),
    ProcessNotLinked(PID),
}

pub struct Kernel {
    /// The virtual file system. This is the file system that the kernel uses to access files.
    /// This is owned by the kernel, and is not owned by any process.
    /// Access will be restricted to the kernel and the root process.
    vfs_ref: VfsRef,
    /// The process table. This is a table of all the processes that are currently running.
    /// This is owned by the kernel, and is not owned by any process.s
    /// Access will be restricted to the kernel and the root process.
    process_table_ref: ProcessTableRef,
}

type ProcessMapRef = Arc<Mutex<ext::HashMap<PID, ProcessHostRef>>>;

struct ProcessTable {
    processes: ProcessMapRef,
    process_id_counter: PID,
}

impl ProcessTable {
    pub fn new() -> Self {
        Self {
            processes: Arc::new(Mutex::new(ext::HashMap::new())),
            process_id_counter: 0,
        }
    }

    pub fn add_process(&mut self, process: ProcessHostRef) -> Result<PID, KernalError> {
        let process_id = self.process_id_counter;
        self.processes.clone().lock().unwrap().insert(process_id.clone(), process);
        self.process_id_counter += 1;
        Ok(process_id)
    }

    pub fn add_child_process(&mut self, process_id: PID, child_process_id: PID) -> Result<(), KernalError> {
        if !self.process_exists(process_id) {
            return Err(KernalError::ProcessNotFound(process_id));
        }
        if !self.process_exists(child_process_id) {
            return Err(KernalError::ProcessNotFound(child_process_id));
        }
        let mut process_context = self.get_process_context(process_id).ok_or(KernalError::ProcessNotFound(process_id))?;
        let child_processes = process_context.lock().unwrap().child_processes.clone();
        if child_processes.contains(&child_process_id) {
            return Err(KernalError::ProcessAlreadyLinked(process_id, child_process_id));
        }
        process_context.lock().unwrap().child_processes.push(child_process_id);

        //update the child process to have the parent process
        let mut child_process_context = self.get_process_context(child_process_id).ok_or(KernalError::ProcessNotFound(child_process_id))?;
        child_process_context.lock().unwrap().parent_process = Some(process_id);
        Ok(())
    }

    pub fn link_process(&mut self, process_id: PID, linked_process_id: PID) -> Result<(), KernalError> {
        if !self.process_exists(process_id) {
            return Err(KernalError::ProcessNotFound(process_id));
        }
        if !self.process_exists(linked_process_id) {
            return Err(KernalError::ProcessNotFound(linked_process_id));
        }
        let mut process_context = self.get_process_context(process_id).ok_or(KernalError::ProcessNotFound(process_id))?;
        let linked_processes = process_context.lock().unwrap().linked_processes.clone();
        if linked_processes.contains(&linked_process_id) {
            return Err(KernalError::ProcessAlreadyLinked(process_id, linked_process_id));
        }
        process_context.lock().unwrap().linked_processes.push(linked_process_id);
        Ok(())
    }

    pub fn unlink_process(&mut self, process_id: PID, linked_process_id: PID) -> Result<(), KernalError> {
        if !self.process_exists(process_id) {
            return Err(KernalError::ProcessNotFound(process_id));
        }
        //TODO: rethink this, we may have a case where we want to unlink a process that doesn't exist.
        if !self.process_exists(linked_process_id) {
            return Err(KernalError::ProcessNotFound(linked_process_id));
        }
        let mut process_context = self.get_process_context(process_id).ok_or(KernalError::ProcessNotFound(process_id))?;
        let linked_processes = process_context.lock().unwrap().linked_processes.clone();
        if !linked_processes.contains(&linked_process_id) {
            return Err(KernalError::ProcessNotLinked(process_id));
        }
        process_context.lock().unwrap().linked_processes.retain(|pid| pid != &linked_process_id);
        Ok(())
    }

    pub fn remove_process(&mut self, process_id: PID) {
        //TODO: we may want to do some kind of reference counting here, so that we can remove a process from the process table, but still have it running.
        self.processes.clone().lock().unwrap().remove(&process_id);
    }

    pub fn get_process(&self, process_id: PID) -> Option<ProcessHostRef> {
        self.processes.clone().lock().unwrap().get(&process_id).map(|process| process.clone())
    }

    pub fn process_exists(&self, process_id: PID) -> bool {
        self.processes.clone().lock().unwrap().contains_key(&process_id)
    }

    pub fn get_process_context(&self, process_id: PID) -> Option<ProcessContextRef> {
        self.processes.clone().lock().unwrap().get(&process_id).map(|process| process.lock().unwrap().process_context.clone())
    }
}

impl Kernel {
    pub fn new() -> Self {
        Self {
            vfs_ref: Arc::new(Mutex::new(VFS::new())),
            process_table_ref: Arc::new(Mutex::new(ProcessTable {
                processes: Arc::new(Mutex::new(ext::HashMap::new())),
                process_id_counter: 0,
            })),
        }
    }
}