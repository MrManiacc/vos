-- This is a terminal gui library. It's at the core of the virtual operating system.
-- It's a library that is used to create the terminal gui that allows for many of the
-- system's features to be accessed.
sys.import("sys/input")

-- This is the terminal gui library.
Terminal = {}


-- This is the terminal gui library's constructor.
function Terminal:new(config)
    config = config or {}
    setmetatable(config, self)
    self.__index = self
    self.container = {
        x = function()
            return 0
        end,
        y = function()
            return sys.window.size().height - (sys.window.size().height / 3)
        end,
        width = function()
            return sys.window.size().width
        end,
        height = function()
            return sys.window.size().width / 3
        end,
        color = sys.gui.color("000000FF")
    }
    self.internal = {
        commands = {},
        history = {},
        history_index = 0,
        input = ">"
    }
    self.text = {
        x = 0,
        y = 0,
        font = "default",
        size = 20,
        value = ">",
    }
    self.cursor = {
        x = 0,
        y = 0,
        width = 4,
        height = self.text.size,
        color = sys.gui.color("000000FF"),
        cursor_blink_rate = 0.5,
        cursor_blink = true,
    }
    self.cursor_visible = true
    self.cursor_position = 1 -- This will keep track of where the cursor is in the input
    self.last_blink_time = sys.time() -- Assume sys.time() gives current time
    self.buffer = {}
    self.last_key_time = {}
    self.key_repeat_initial_delay = 0.5 -- 500ms initial delay before repeating
    sys.log("terminal initialized")
    return config
end

-- This is used to register a new command with the terminal.
-- @param name The name of the command.
-- @param callback The callback function for the command.
function Terminal:register_command(name, callback)
    -- Commands should be registered with a name and list of callbacks. we
    -- should append the command to the list of commands.
    if (self.internal.commands[name] == nil) then
        self.internal.commands[name] = {}
    end
    table.insert(self.internal.commands[name], callback)
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

-- Execute a command
function Terminal:execute_command(command)
    local cmd = string.match(command, "([^ ]+)") -- Extract the first word
    local args = {}
    for arg in string.gmatch(command, "[^ ]+") do
        table.insert(args, arg)
    end
    table.remove(args, 1) -- Remove the command itself, leaving only the arguments

    if self.internal.commands[cmd] then
        for _, callback in ipairs(self.internal.commands[cmd]) do
            callback(unpack(args))
        end
    else
        self.text.value = self.text.value .. "\nUnknown command: " .. cmd
    end

    -- Add command to history and reset input
    table.insert(self.internal.history, command)
    self.internal.history_index = #self.internal.history + 1
    self.internal.input = ">"
    self.cursor_position = 2
end


-- Used to dump the terminal state to a string. Dumps all registered commands
-- and their callbacks.
function Terminal:dump()
    local str = ""
    for name, callbacks in pairs(self.internal.commands) do
        str = str .. name .. ":\n"
        for i, callback in ipairs(callbacks) do
            str = str .. "    " .. tostring(callback) .. "\n"
        end
    end
    return str
end

function Terminal:redraw()
    local currentTime = sys.time()
    local currentTime = sys.time()

    -- Generic function to handle key repeat behavior
    local function handleKeyRepeat(key, action)
        if sys.input.key(key).is_pressed then
            self.last_key_time[key] = currentTime
        elseif sys.input.key(key).is_down then
            local elapsed = currentTime - (self.last_key_time[key] or 0)
            if elapsed > self.key_repeat_initial_delay then
                action()
            end
        end
    end

    -- Handle left arrow (move cursor left)
    handleKeyRepeat(keys.KEY_LEFT, function()
        if self.cursor_position > 2 then
            self.cursor_position = self.cursor_position - 1
        end
    end)

    -- Handle right arrow (move cursor right)
    handleKeyRepeat(keys.KEY_RIGHT, function()
        if self.cursor_position < #self.text.value + 1 then
            self.cursor_position = self.cursor_position + 1
        end
    end)

    -- Handle backspace
    handleKeyRepeat(keys.KEY_BACKSPACE, function()
        if self.cursor_position > 2 then
            local beforeCursor = string.sub(self.text.value, 1, self.cursor_position - 2)
            local afterCursor = string.sub(self.text.value, self.cursor_position)
            self.text.value = beforeCursor .. afterCursor
            self.cursor_position = self.cursor_position - 1
        end
    end)

    if (self.cursor_visible) then
        sys.gui.draw_rect(self.cursor.x, self.cursor.y, self.cursor.width, self.cursor.height, self.cursor.color)
    end

    -- Handle enter/return
    if sys.input.key(keys.KEY_ENTER).is_pressed then
        local command = string.sub(self.text.value, 2)
        print("command: " .. command)
        self:execute_command(command)
        self.text.value = ">"
    end

    for i = 32, 126 do
        if (sys.input.key(i).is_pressed) then
            local char = string.char(i)
            local beforeCursor = string.sub(self.text.value, 1, self.cursor_position - 1)
            local afterCursor = string.sub(self.text.value, self.cursor_position)
            self.text.value = beforeCursor .. char .. afterCursor
            self.cursor_position = self.cursor_position + 1
        end
    end

    -- Handle left arrow (move cursor left)
    if sys.input.key(keys.KEY_LEFT).is_pressed and self.cursor_position > 2 then
        self.cursor_position = self.cursor_position - 1
    end

    -- Handle right arrow (move cursor right)
    if sys.input.key(keys.KEY_RIGHT).is_pressed and self.cursor_position < #self.text.value + 1 then
        self.cursor_position = self.cursor_position + 1
    end

    -- Handle backspace
    if sys.input.key(keys.KEY_BACKSPACE).is_pressed then
        if self.cursor_position > 2 then
            local beforeCursor = string.sub(self.text.value, 1, self.cursor_position - 2)
            local afterCursor = string.sub(self.text.value, self.cursor_position)
            self.text.value = beforeCursor .. afterCursor
            self.cursor_position = self.cursor_position - 1
        end
    end

    -- Handle up arrow (previous command)
    if sys.input.key(keys.KEY_UP).is_pressed and self.internal.history_index > 1 then
        self.internal.history_index = self.internal.history_index - 1
        self.internal.input = ">" .. self.internal.history[self.internal.history_index]
        self.text.value = self.internal.input
        self.cursor_position = #self.text.value + 1
    end

    -- Handle down arrow (next command)
    if sys.input.key(keys.KEY_DOWN).is_pressed and self.internal.history_index < #self.internal.history then
        self.internal.history_index = self.internal.history_index + 1
        self.internal.input = ">" .. self.internal.history[self.internal.history_index]
        self.text.value = self.internal.input
        self.cursor_position = #self.text.value + 1
    end

    -- Render the text buffer and input
    local startY = self.container.y() + self.container.height() - self.text.size
    for i = #self.buffer, 1, -1 do
        sys.gui.draw_text(self.buffer[i], self.text.x, startY, self.text.size)
        startY = startY - self.text.size
        if startY < self.container.y() then
            break
        end
    end

    -- Render the input
    sys.gui.draw_text(self.text.value, self.text.x, self.container.y() + self.container.height() - self.text.size, self.text.size)

    -- Recalculate cursor position
    self.cursor.x = self.text.x + sys.gui.text_width(string.sub(self.text.value, 1, self.cursor_position - 1), self.text.size)

    -- Draw cursor
    if self.cursor.cursor_blink then
        sys.gui.draw_rect(self.cursor.x, self.container.y() + self.container.height() - self.text.size, self.cursor.width, self.cursor.height, self.cursor.color)
    end

    --    Always make sure the > is the first character
    if (string.sub(self.text.value, 1, 1) ~= ">") then
        self.text.value = ">" .. self.text.value
    end

    --    Disallow cursor from going past the > character
    if (self.cursor_position < 2) then
        self.cursor_position = 2
    end

    -- Handle cursor blinking
    if currentTime - self.last_blink_time > self.cursor.cursor_blink_rate then
        self.cursor.cursor_blink = not self.cursor.cursor_blink
        self.last_blink_time = currentTime
    end
end

