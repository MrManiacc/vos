---
--- Created by jraynor.
--- DateTime: 8/26/2023 5:04 PM
---
function main()
    sys.log("initializing gui library")
    sys.listen("update", update)
end

local frame_count = 0

local ui = sys.gui;
local color = ui.color("71ab80FA")
local start_time = sys.time()

function update()
    local time = sys.time()
    local delta = time - start_time
    start_time = time
    local fps = math.floor(1 / delta)
    ui.draw_rect(40, 40, 250, 100, color)
    local y = 50
    ui.draw_text("Frame count: " .. frame_count, 50, y, 20)
    y = y + 25
    ui.draw_text("Frame time: " .. time, 50, y, 20)
    y = y + 25
    ui.draw_text("Frame delta: " .. fps, 50, y, 20)


    frame_count = frame_count + 1
end

