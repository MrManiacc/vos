sys.import("sys/input")
UIComponent = {}
Alignment = {
    -- Horizontal align
    LEFT = 1 << 0, -- Default, align text horizontally to left.
    CENTER = 1 << 1, -- Align text horizontally to center.
    RIGHT = 1 << 2, -- Align text horizontally to right.
    -- Vertical align
    TOP = 1 << 3, -- Align text vertically to top.
    MIDDLE = 1 << 4, -- Align text vertically to middle.
    BOTTOM = 1 << 5, -- Align text vertically to bottom.
    BASELINE = 1 << 6 -- Default, align text vertically to baseline.
}

Colors = {
    White = mui.color("FFFFFF"),
    Black = mui.color("000000"),
    Red = mui.color("FF0000"),
    Green = mui.color("00FF00"),
    Blue = mui.color("0000FF"),
    Yellow = mui.color("FFFF00"),
    Cyan = mui.color("00FFFF"),
    Magenta = mui.color("FF00FF"),
    Gray = mui.color("808080"),
    DarkGray = mui.color("404040"),
    LightGray = mui.color("C0C0C0"),
    Orange = mui.color("FFA500"),
    Brown = mui.color("A52A2A"),
    Pink = mui.color("FFC0CB"),
    Purple = mui.color("800080"),
}
function UIComponent:new(x, y)
    local obj = {
        x = x,
        y = y,
        style = {},
        events = {
            onHoverStart = nil,
            onHoverStop = nil,
            onClick = nil,
            onDrag = nil,
        },
    }
    setmetatable(obj, self)
    self.__index = self
    return obj
end

function UIComponent:setStyle(style)
    self.style = style
end

function UIComponent:on(event, callback)
    self.events[event] = callback
end

function UIComponent:triggerEvent(event, ...)
    if self.events[event] then
        self.events[event](self, ...)
    end
end

Button = UIComponent:new()

function Button:new(x, y, text)
    local obj = UIComponent.new(self, x, y)
    obj.text = text
    obj.width = 0 -- Will be calculated
    obj.height = 0 -- Will be calculated
    obj.isHovered = false
    obj.isPressed = false
    return obj
end

function Button:process()
    local function draw(self)
        -- Button specific drawing logic, including event handling
        local bounds = mui.measure_text(self.text, self.style.font_size or 20)
        self.width = bounds.w + (self.style.padding or 5) * 2
        self.height = bounds.h + (self.style.padding or 5) * 2

        local col = self.style.stand_color or Colors.DarkGray
        if self.isHovered then
            col = self.style.hover_color or Colors.Gray
        end
        if self.isPressed then
            col = self.style.click_color or Colors.LightGray
        end

        mui.draw_rect(self.x, self.y, self.width, self.height, col)
        mui.draw_text(self.text, self.x + self.width / 2, self.y + self.height / 2, self.style.font_size or 20, self.style.text_color or Colors.White, self.style.align or (Alignment.CENTER | Alignment.MIDDLE))

    end


    -- Calculate the width and height of the button based on the text and style
    local bounds = mui.measure_text(self.text, self.style.font_size or 20)
    self.width = bounds.w + (self.style.padding or 5) * 2
    self.height = bounds.h + (self.style.padding or 5) * 2

    local mouseX, mouseY = get_mouse_pos()

    -- Check if mouse is over the button
    local isHovered = mouseX >= self.x and mouseX <= (self.x + self.width) and mouseY >= self.y and mouseY <= (self.y + self.height)

    -- Update hover state and trigger events
    if isHovered and not self.isHovered then
        self.isHovered = true
        self:triggerEvent("onHoverStart")
    elseif not isHovered and self.isHovered then
        self.isHovered = false
        self:triggerEvent("onHoverStop")
    end

    -- Check for click
    local isPressed = is_mouse_pressed(0) -- Assuming button 0 is the left mouse button
    local isReleased = is_mouse_released(0)

    if isHovered and isPressed then
        self.isPressed = true
        self:triggerEvent("onClick")
    end

    if self.isPressed and isReleased then
        self.isPressed = false
    end

    -- Draw the button
    draw(self)
end

Label = UIComponent:new()

function Label:new(x, y, text)
    local obj = UIComponent.new(self, x, y)
    obj.text = text
    return obj
end

function Label:process()
    local function draw(self)
        mui.draw_text(self.text, self.x, self.y, self.style.font_size or 20, self.style.text_color or Colors.White, self.style.align or (Alignment.LEFT | Alignment.BASELINE))
    end
    draw(self)
end

