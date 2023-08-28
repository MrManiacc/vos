-- This is a terminal gui library. It's at the core of the virtual operating system.
-- It's a library that is used to create the terminal gui that allows for many of the
-- system's features to be accessed.


-- This is the terminal gui library.
Terminal = {}


-- This is the terminal gui library's constructor.
function Terminal:new(config)
    config = config or {}
    setmetatable(config, self)
    self.__index = self
    self.commands = {}
    self.history = {}
    self.history_index = 0
    self.input = ""
    self.cursor = 0
    self.cursor_visible = true
    self.cursor_timer = 0
    self.cursor_blink_rate = 0.5
    self.cursor_blink = true
    sys.log("terminal initialized")
    return config
end


-- This is used to register a new command with the terminal.
-- @param name The name of the command.
-- @param callback The callback function for the command.
function Terminal:register_command(name, callback)
    -- Commands should be registered with a name and list of callbacks. we
    -- should append the command to the list of commands.
    if (self.commands[name] == nil) then
        self.commands[name] = {}
    end
    table.insert(self.commands[name], callback)
end


-- This is used to unregister a command from the terminal.
-- @param name The name of the command.
-- @param callback The callback function for the command.
function Terminal:unregister_command(name, callback)
    -- Commands should be registered with a name and list of callbacks. we
    -- should append the command to the list of commands.
    if (self.commands[name] == nil) then
        return
    end
    for i, v in ipairs(self.commands[name]) do
        if (v == callback) then
            table.remove(self.commands[name], i)
            return
        end
    end
end

-- Used to dump the terminal state to a string. Dumps all registered commands
-- and their callbacks.
function Terminal:dump()
    local str = ""
    for name, callbacks in pairs(self.commands) do
        str = str .. name .. ":\n"
        for i, callback in ipairs(callbacks) do
            str = str .. "    " .. tostring(callback) .. "\n"
        end
    end
    return str
end


