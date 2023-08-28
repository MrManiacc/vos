--
--sys.listen("update", function()
--    local size = sys.window.size()
--    sys.gui.draw_rect(0, 0, size.width, size.height, sys.gui.color("71ab80FA"))
--    --sys.gui.draw_text("Hello World", 20, 20, 20)
--
--    local str = sys.fs_str()
--    local width = sys.gui.text_width(str, 8)
--    sys.gui.draw_text(str, 60, 80, 8)
--end)
--
--
sys.import("sys/libgui")

function main()
    sys.log("Hello World")
end
local click_count = 0

function on_click()
    click_count = click_count + 1
    sys.log("click count: " .. click_count)
end

-- This is the update function that is called every frame
sys.listen("update", function()
    local size = sys.window.size()
    --sys.gui.draw_text("Hello World", 20, 20, 20)
    local str = sys.fs_str()
    local width = sys.gui.text_width(str, 20)
    sys.gui.draw_text(str, size.width / 2 - width / 2, 80, 20)
    button({
        x = 100,
        y = 100,
        text = "Hello World",
        text_size = 20
    }, on_click)
    render_stats(size.width - 300, 0)

    sys.gui.draw_text("Click count: " .. click_count, 100, 20, 20)

end)



