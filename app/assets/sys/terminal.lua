-- Import the necessary libraries
sys.import("sys/input")

-- Terminal class definition
Terminal = {}


--- Initializes properties for the Terminal.
-- This is a local function and is not meant to be called externally.
-- @param config Configuration table for the Terminal.
local function _initializeProperties(self, config)

    self.internal = {
        version = "0.1.0",
        commands = {},
        history = {},
        history_index = 0,
        input = " "
    }
    self.text = {
        x = 0,
        y = 0,
        font = "default",
        size = 20,
        value = "",
    }
    self.cursor = {
        x = 0,
        y = 0,
        width = 4,
        height = self.text.size,
        color = sys.gui.color("03a83aff"),
        cursor_blink_rate = 0.5,
        cursor_blink = true,
    }
    self.cursor_visible = true
    self.cursor_position = 0
    self.last_blink_time = sys.time()
    self.buffer = {}
    self.last_key_time = {}
    self.key_repeat_initial_delay = 0.5
end

--- Initializes a new instance of the Terminal.
-- @param config Optional configuration table for the Terminal.
-- @return A new Terminal instance.
function Terminal:new(config)
    config = config or {}
    setmetatable(config, self)
    self.__index = self

    -- Initialize properties
    _initializeProperties(self, config)

    sys.log("terminal initialized")
    return config
end


--- Registers a new command with the Terminal.
-- @param name The name of the command.
-- @param callback The callback function for the command.
function Terminal:register_command(name, callback)
    if not self.internal.commands[name] then
        self.internal.commands[name] = {}
    end
    table.insert(self.internal.commands[name], callback)
end

--- Unregisters a command from the Terminal.
-- @param name The name of the command.
-- @param callback The callback function for the command.
function Terminal:unregister_command(name, callback)
    if not self.internal.commands[name] then
        return
    end
    for i, v in ipairs(self.internal.commands[name]) do
        if v == callback then
            table.remove(self.internal.commands[name], i)
            break
        end
    end
end
--- Resets the command state after execution.
-- This is a local function and is not meant to be called externally.
local function _resetCommandState(self)
    table.insert(self.internal.history, command)
    self.internal.history_index = #self.internal.history + 1
    self.internal.input = ">"
    self.cursor_position = 2
end

--- Executes a command in the Terminal.
-- @param command The command to be executed.
function Terminal:execute_command(command)
    local cmd = string.match(command, "([^ ]+)")
    local args = {}
    for arg in string.gmatch(command, "[^ ]+") do
        table.insert(args, arg)
    end
    table.remove(args, 1)

    if self.internal.commands[cmd] then
        for _, callback in ipairs(self.internal.commands[cmd]) do
            callback(unpack(args))
        end
    else
        self.text.value = self.text.value .. "\nUnknown command: " .. cmd
    end

    _resetCommandState(self)
end



--- Dumps the current state of the Terminal to a string.
-- @return A string representing the state of the Terminal.
function Terminal:dump()
    local str = ""
    for name, callbacks in pairs(self.internal.commands) do
        str = str .. name .. ":\n"
        for _, callback in ipairs(callbacks) do
            str = str .. "    " .. tostring(callback) .. "\n"
        end
    end
    return str
end


--- Handles key input for the Terminal.
-- This is a local function and is not meant to be called externally.
local function _handleKeyInput(self)
    local currentTime = sys.time()
    -- Initialize key_repeat_rate if not done already
    self.key_repeat_rate = self.key_repeat_rate or {}

    -- Generic function to handle key repeat behavior
    local function handleKeyRepeat(key, action)
        if sys.input.key(key).is_pressed then
            self.last_key_time[key] = currentTime
            self.key_repeat_rate[key] = self.key_repeat_initial_delay
            action()  -- immediate action on key press
        elseif sys.input.key(key).is_down then
            local elapsed = currentTime - (self.last_key_time[key] or 0)
            if elapsed > self.key_repeat_rate[key] then
                action()  -- repeated action on key hold
                self.last_key_time[key] = currentTime
                self.key_repeat_rate[key] = 0.025  -- subsequent repeats should be faster
            end
        end
    end

    for i = 32, 126 do
        if (sys.input.key(i).is_pressed) then
            local char = string.char(i)
            local beforeCursor = string.sub(self.internal.input, 1, self.cursor_position)
            local afterCursor = string.sub(self.internal.input, self.cursor_position + 1)
            self.internal.input = beforeCursor .. char .. afterCursor
            self.cursor_position = self.cursor_position + 1
        end
    end
    -- Handle left arrow (move cursor left)
    handleKeyRepeat(keys.KEY_LEFT, function()
        if self.cursor_position > 0 then
            self.cursor_position = self.cursor_position - 1
        end
    end)

    -- Handle right arrow (move cursor right)
    handleKeyRepeat(keys.KEY_RIGHT, function()
        if self.cursor_position < #self.internal.input then
            self.cursor_position = self.cursor_position + 1
        end
    end)

    -- Handle backspace
    handleKeyRepeat(keys.KEY_BACKSPACE, function()
        if self.cursor_position >= 1 then
            local start = string.sub(self.internal.input, 1, self.cursor_position - 1)
            local finish = string.sub(self.internal.input, self.cursor_position + 1, #self.internal.input)
            self.internal.input = start .. finish
            self.cursor_position = self.cursor_position - 1
        end
    end)

    -- Handle delete
    handleKeyRepeat(keys.KEY_DELETE, function()
        if self.cursor_position < #self.internal.input then  -- Modified this line to ensure we're not at the end
            local start = string.sub(self.internal.input, 1, self.cursor_position)
            local finish = string.sub(self.internal.input, self.cursor_position + 2)  -- Skip the character to the right of the cursor
            self.internal.input = start .. finish
        end
    end)

    -- Handle home
    handleKeyRepeat(keys.KEY_HOME, function()
        self.cursor_position = 0
    end)

    -- Handle end
    handleKeyRepeat(keys.KEY_END, function()
        self.cursor_position = #self.internal.input + 1
    end)

    -- Handle page up
    handleKeyRepeat(keys.KEY_PAGE_UP, function()
    end)

end

-- Renders the header of the Terminal.
-- This is a local function and is not meant to be called externally.
local function _renderHeader(self)
    local size = sys.window.size()
    local x = 0
    local y = math.floor(size.height - (size.height / 3))
    local width = size.width
    local height = size.height / 3
    sys.gui.draw_rect(x, y + 1, width, height, sys.gui.color("474747FF"))
    sys.gui.draw_rect(x + 10, y + 10, width - 20, height - 20, sys.gui.color("363534ff"))
    sys.gui.draw_text("Terminus", x + 20, y + 15, 20, sys.gui.color("FFFFFFFF"))
    sys.gui.draw_text("v" .. self.internal.version, sys.gui.text_width("Terminus", 20) + 25, y + 20, 15, sys.gui.color("03a83aff"))
    sys.gui.draw_rect(x + 10, y + 40, width - 20, height - 50, sys.gui.color("1b1c1bff"))
end

--- Renders the current input of the Terminal.
-- This is a local function and is not meant to be called externally.
local function _renderCursor(self)
    local size = sys.window.size()
    local x = 0
    local y = math.floor(size.height - (size.height / 3))
    local height = size.height / 3
    local text_x = x + 20
    local text_y = y + height - 40
    local text_size = 20

    -- Calculate the width of the text up to the cursor's position
    local input_upto_cursor = string.sub(self.internal.input, 1, self.cursor_position )
    local width_upto_cursor = sys.gui.text_width(input_upto_cursor, text_size)

    local cursor_x = text_x + width_upto_cursor
    local cursor_y = text_y
    local cursor_width = self.cursor.width
    local cursor_height = self.cursor.height
    local cursor_color = self.cursor.color

    sys.gui.draw_text(self.internal.input, text_x, text_y, text_size, sys.gui.color("FFFFFFFF"))

    if self.cursor.cursor_blink then
        sys.gui.draw_rect(cursor_x, cursor_y, cursor_width, cursor_height, cursor_color)
    end
end


--- Renders the text buffer of the Terminal.
-- This is a local function and is not meant to be called externally.
local function _renderBuffer(self)
--    draws the input on the first line,
--    draws the text.valu on all other lines split by \n
end



--- Handles cursor blinking for the Terminal.
-- This is a local function and is not meant to be called externally.
local function _handleCursorBlinking(self)
    local currentTime = sys.time()
    if currentTime - self.last_blink_time > self.cursor.cursor_blink_rate then
        self.cursor.cursor_blink = not self.cursor.cursor_blink
        self.last_blink_time = currentTime
    end
end

--- Redraws the Terminal.
function Terminal:redraw()
    _handleKeyInput(self)
    _renderHeader(self)
    _renderBuffer(self)
    _renderCursor(self)
    _handleCursorBlinking(self)
end

-- End of Terminal class
