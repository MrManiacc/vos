#[derive(Debug, PartialEq, Eq, Clone)]
pub struct Process();

pub struct ProcessHost {
    pub processes: Vec<Process>,
    pub current_process: usize,
    pub current_pid: usize,
}

