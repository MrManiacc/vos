---
--- Created by jraynor.
--- DateTime: 8/26/2023 5:04 PM
---
---
--sys.import("/sys/libgui.lua")
----sys.execute("sys/libgui.lua")
--
--
--local config = {
--    x = 300,
--    y = 20,
--    text = "Hello World",
--    text_size = 20,
--    color = sys.gui.color("71ab80FA")
--}
--
--local click_count = 0
--sys.listen("update", function()
--    local size = sys.window.size()
--    sys.gui.draw_rect(0, 0, size.width, size.height, config.color)
--    render_stats(size.width - 300, 0)
--    button(config, function()
--        click_count = click_count + 1
--        config.text = "Clicked " .. click_count .. " times"
--    end)
--    local str = sys.fs_str()
--    local width = sys.gui.text_width(str, 8)
--    sys.gui.draw_text(str, 20, 20, 8)
--
--
--
--end)
--
--
--print("Hello World")


sys.listen("update", function()
    local size = sys.window.size()
    sys.gui.draw_rect(0, 0, size.width, size.height, sys.gui.color("71ab80FA"))
    sys.gui.draw_text("Hello World", 20, 20, 20)
end)


