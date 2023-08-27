---
--- Created by jraynor.
--- DateTime: 8/26/2023 5:04 PM
---
---
sys.import("/sys/libgui.lua")
--sys.execute("sys/libgui.lua")


local config = {
    x = 20,
    y = 20,
    text = "Hello World",
    text_size = 20,
    color = sys.gui.color("71ab80FA")
}

local click_count = 0
sys.listen("update", function()
    local size = sys.window.size()
    sys.gui.draw_rect(0, 0, size.width, size.height, config.color)
    render_stats(size.width - 300, 0)
    button(config, function()
        click_count = click_count + 1
        config.text = "Clicked " .. click_count .. " times"
    end)
end)

