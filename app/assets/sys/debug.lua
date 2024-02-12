--function main()
--    sys.log("initializing gui library at " .. sys.path)
--    sys.listen("update", update)
--end

local frame_count = 0

local ui = sys.gui;
local green_color = ui.color("1b1c1bff")
local purple_color = ui.color("bc30d1FA")
local red_color = ui.color("f58142FA")
local start_time = sys.time()

function render_stats(startX, startY)
    local time = sys.time()
    local delta = time - start_time
    start_time = time
    local mouse = sys.input.mouse()
    local fps = math.floor(1 / delta)
    local color

    if (mouse.x > startX and mouse.x < startX + 300 and mouse.y > startY and mouse.y < startY + 200) then
        color = mouse.is_down(0) and red_color or purple_color
    else
        color = ui.color("a1ab80FA")
    end

    ui.draw_rect(startX, startY, 300, 200, color)
    local y = startY + 10
    ui.draw_text("Frame count: " .. frame_count, startX + 10, y, 20, green_color)
    y = y + 25
    ui.draw_text("Frame time: " .. time, startX + 10, y, 20, green_color)
    y = y + 25
    ui.draw_text("Frame delta: " .. fps, startX + 10, y, 20, green_color)
    y = y + 25
    ui.draw_text("Mouse: " .. mouse.x .. ", " .. mouse.y, startX + 10, y, 20, green_color)

    frame_count = frame_count + 1
end
--
--
--

---- This is the update function that is called every frame
function button(config, on_click)
    local mouse = sys.input.mouse()
    local x = config.x
    local y = config.y
    local width = sys.gui.text_width(config.text, config.text_size) + 20
    local height = config.text_size + 20
    local text = config.text
    local text_width = ui.text_width(text, config.text_size)
    local text_size = config.text_size
    local text_x = x + (width / 2) - (text_width / 2)
    local text_y = y + (height / 2) - (text_size / 2)

    local is_hovering = mouse.x > x and mouse.x < x + width and mouse.y > y and mouse.y < y + height

    if (is_hovering) then
        if (mouse.is_pressed(0)) then
            on_click()
        end
        --     Draw offset box for hover
        ui.draw_rect(x - 5, y - 5, width + 10, height + 10, ui.color("000000FF"))
    else
        color = ui.color("71ab80FA")
    end
    ui.draw_rect(x, y, width, height, color)
    ui.draw_text(text, text_x, text_y, text_size)

end

sys.log("initializing gui library at " .. sys.path)