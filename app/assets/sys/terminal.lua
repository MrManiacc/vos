-- Import the necessary libraries
sys.import("sys/input")

-- Terminal class definition
Terminal = {}


--- Initializes properties for the Terminal.
-- This is a local function and is not meant to be called externally.
-- @param config Configuration table for the Terminal.
local function _initializeProperties(self, config)

    self.internal = {
        version = "0.1.3b",
        commands = {},
        history = {},
        history_index = 1,
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
        width = 3,
        height = self.text.size - 5,
        color = sys.gui.color("03a83aff"),
        cursor_blink_rate = 0.5,
        cursor_blink = true,
    }
    self.cursor_visible = true
    self.cursor_position = 0
    self.last_blink_time = sys.time()
    self.buffer = {}
    self.last_key_time = {}
    self.key_repeat_initial_delay = 0.66
    self.is_key_repeating = false
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

function Terminal:unregister_all_commands(name)
    self.internal.commands[name] = nil
end

--- Resets the command state after execution.
-- This is a local function and is not meant to be called externally.
local function _resetCommandState(self)
    self.internal.input = ">"
    self.cursor_position = 2
end

function Terminal:clear()
    self.text.value = ""
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
            if #args == 0 then
                callback()
            else
                callback(table.unpack(args))
            end
        end

    else
        self.text.value = self.text.value .. "\nUnknown command: " .. cmd

    end
    -- store the command in the history
    self.internal.history[self.internal.history_index] = cmd
    self.internal.history_index = self.internal.history_index + 1
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

function Terminal:print(text)
    self.text.value = self.text.value .. "\n" .. text
end

--- Handles key input for the Terminal.
-- This is a local function and is not meant to be called externally.
local function _handleKeyInput(self)

    -- Initialize key_repeat_rate if not done already
    self.key_repeat_rate = self.key_repeat_rate or {}

    -- Generic function to handle key repeat behavior
    local function handleKeyRepeat(key, action)
        -- Calls the first action immediately when the key is pressed.
        -- stores that time in last_key_time
        if sys.input.key(key).is_pressed then
            if not self.last_key_time[key] then
                self.last_key_time[key] = sys.time()
                self.is_key_repeating = true -- Key repeat starts
                action()
            end
        end
        -- starts a timer to call the action again after the initial delay
        if sys.input.key(key).is_down then
            if not self.key_repeat_rate[key] then
                self.key_repeat_rate[key] = self.key_repeat_initial_delay
            end
            if sys.time() - self.last_key_time[key] > self.key_repeat_rate[key] then
                self.key_repeat_rate[key] = self.key_repeat_rate[key] * 0.33
                action()
            end
        end
        -- resets the key repeat rate when the key is released
        if sys.input.key(key).is_released then
            self.key_repeat_rate[key] = self.key_repeat_initial_delay
            self.last_key_time[key] = nil
            self.is_key_repeating = false -- Key repeat stops
        end
    end

    for i = 32, 126 do
        if (sys.input.key(i).is_pressed) then
            local char = string.char(i)
            -- Check for Shift key state; assume sys.input.key() can check it
            -- You might need to adjust this based on your actual input handling system
            local isShiftPressed = sys.input.key(keys.KEY_LEFT_SHIFT).is_down or sys.input.key(keys.KEY_SHIFT_RIGHT).is_down
            if not isShiftPressed then
                char = string.lower(char)
            end
            local beforeCursor = string.sub(self.internal.input, 1, self.cursor_position)
            local afterCursor = string.sub(self.internal.input, self.cursor_position + 1)
            self.internal.input = beforeCursor .. char .. afterCursor
            self.cursor_position = self.cursor_position + 1
        end
    end

    local isControlPressed = sys.input.key(keys.KEY_LEFT_CONTROL).is_down or sys.input.key(keys.KEY_RIGHT_CONTROL).is_down

    -- Handle Enter key
    handleKeyRepeat(keys.KEY_ENTER, function()
        if #self.internal.input > 0 then
            local command = self.internal.input
            self:execute_command(command)
            --self.text.value = self.text.value .. "\n" .. command
            self.internal.input = ""
            self.cursor_position = 0
        end
    end)
    -- Check for Control key state

    -- Handle right arrow with Control for jumping to the next space
    handleKeyRepeat(keys.KEY_RIGHT, function()
        if isControlPressed then
            local foundPos = string.find(self.internal.input, " ", self.cursor_position + 1)
            if foundPos then
                self.cursor_position = foundPos
            else
                self.cursor_position = #self.internal.input -- Move to end if no space found
            end
        else
            if self.cursor_position < #self.internal.input then
                self.cursor_position = self.cursor_position + 1
            end
        end
    end)

    -- Handle left arrow with Control for jumping to the previous space
    handleKeyRepeat(keys.KEY_LEFT, function()
        if isControlPressed then
            local startPos = self.cursor_position - 1
            local foundPos = startPos > 0 and string.find(string.reverse(self.internal.input:sub(1, startPos)), " ") or nil
            if foundPos then
                self.cursor_position = startPos - foundPos + 1
            else
                self.cursor_position = 0 -- Move to start if no space found
            end
        else
            if self.cursor_position > 0 then
                self.cursor_position = self.cursor_position - 1
            end
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
        if self.cursor_position < #self.internal.input then
            -- Modified this line to ensure we're not at the end
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

    -- Handle up arrow (previous command in history)
    handleKeyRepeat(keys.KEY_UP, function()
        print(self.internal.history_index)
        if self.internal.history_index > 1 then
            self.internal.history_index = self.internal.history_index - 1
            self.internal.input = self.internal.history[self.internal.history_index]
            self.cursor_position = #self.internal.input
        end
    end)

    -- Handle down arrow (next command in history)
    handleKeyRepeat(keys.KEY_DOWN, function()
        if self.internal.history_index < #self.internal.history then
            self.internal.history_index = self.internal.history_index + 1
            self.internal.input = self.internal.history[self.internal.history_index]
            self.cursor_position = #self.internal.input
        end
    end)


end

-- Renders the header of the Terminal.
-- This is a local function and is not meant to be called externally.
local function _renderHeader(self)
    local size = sys.window.size()
    local x = 0
    local y = math.floor(size.height - (size.height / 3)) + 10
    local width = size.width
    local height = size.height / 3
    sys.gui.draw_rect(x, y - 12, width, height, sys.gui.color("474747FF"))
    sys.gui.draw_rect(x + 10, y + 10, width - 20, height - 20, sys.gui.color("363534ff"))
    sys.gui.draw_rect(x + 10, y + 40, width - 20, height - 50, sys.gui.color("1b1c1bff"))
end

--- Renders the current input of the Terminal.
-- This is a local function and is not meant to be called externally.
local function _renderCursor(self)
    local size = sys.window.size()
    local x = 0
    local height = size.height / 3
    local y = math.floor(size.height - height) + 10
    local text_size = 20
    local text_x = x + 20
    local text_y = y + height - text_size

    -- Calculate the width of the text up to the cursor's position
    local input_upto_cursor = string.sub(self.internal.input, 1, self.cursor_position)
    local width_upto_cursor = sys.gui.text_width(input_upto_cursor, text_size)

    local cursor_x = text_x + width_upto_cursor
    local cursor_y = text_y - text_size + 7
    local cursor_width = self.cursor.width
    local cursor_height = self.cursor.height
    local cursor_color = self.cursor.color

    sys.gui.draw_text(self.internal.input, text_x, text_y, text_size, sys.gui.color("FFFFFFFF"))

    if self.cursor.cursor_blink then
        sys.gui.draw_rect(cursor_x, cursor_y, cursor_width, cursor_height, cursor_color)
    end
end

local function _renderBuffer(self)
    local size = sys.window.size()  -- Get the current window size
    local x = 0
    local height = size.height / 3
    local y = math.floor(size.height - height) + 25
    local text_x = x + 20
    local cursor_base_y = y + height - 30  -- Calculate based on the cursor's relative position
    local text_size = 20
    
    local lines = {}
    for line in self.text.value:gmatch("([^\n]*)\n?") do
        table.insert(lines, line)
    end

    -- Ensure the text buffer starts right above the cursor and moves up
    local buffer_start_y = cursor_base_y - #lines * text_size
    for i, line in ipairs(lines) do
        -- Calculate Y position for each line, starting from buffer_start_y
        local line_y = buffer_start_y + (i - 1) * text_size
        -- Don't render if the line is outside the visible area
        if line_y > y then
            sys.gui.draw_text(line, text_x, line_y, text_size, sys.gui.color("FaFaFaFF"))
        end
        -- Adjust drawing to ensure it's within the visible area
    end

    local width = size.width
    -- draws shadow for the header
    sys.gui.draw_rect(x + 15, y + 5, width - 25, 40, sys.gui.color("000000ff"))
    -- draw the overlay
    sys.gui.draw_rect(x + 10, y + 5, width - 25, 35, sys.gui.color("1b1c1bff"))
    sys.gui.draw_text("Terminus", x + 20, y + 30, 30, sys.gui.color("FFFFFFFF"))
    sys.gui.draw_text("v" .. self.internal.version, sys.gui.text_width("Terminus", 20) + 25, y + 40, 12, sys.gui.color("03a83aff"))
end


--- Handles cursor blinking for the Terminal.
-- This is a local function and is not meant to be called externally.
local function _handleCursorBlinking(self)
    local currentTime = sys.time()
    if self.is_key_repeating then
        self.cursor.cursor_blink = true -- Keep the cursor always visible during key repeat
    else
        if currentTime - self.last_blink_time > self.cursor.cursor_blink_rate then
            self.cursor.cursor_blink = not self.cursor.cursor_blink
            self.last_blink_time = currentTime
        end
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
