--sys.import("sys/terminal")
--sys.import("sys/debug")
sys.import("sys/gui")

local style_header = {
    font_size = 30,
    text_color = Colors.White,
    align = Alignment.CENTER | Alignment.MIDDLE,
    padding = 10,
    stand_color = Colors.DarkGray,
    hover_color = Colors.Gray,
    click_color = Colors.LightGray,
    border_radius = 5
}

local button = Button:new(10, 10, "testing")
button:setStyle(style_header)
button:on("onClick", function()
    sys.log.info("Button clicked")
end)

function drawUI()
    button:process()
end

sys.listen("update", drawUI)



--
--local term = Terminal:new()
----
--local showing_stats = false
----
--term:register_command("test", function()
--    term:print("testing")
--end)
----
----term:register_command("path", function()
----    term:print(sys.path)
----end)
----
----term:register_command("clear", function()
----    term:clear()
----end)
----
----term:register_command("exit", function()
----    sys.exit()
----end)
----
--term:register_command("help", function()
--    term:print("Commands:")
--    term:print("  test")
--    term:print("  clear")
--    term:print("  exit")
--    term:print("  path")
--    term:print("  debug")
--    term:print("  stats")
--end)
----
--term:register_command("fs", function()
--    local str = sys.fs_str()
--    term:print(str)
--end)
--
--term:register_command("stats", function()
--    showing_stats = not showing_stats
--end)
--
----term:register_command("debug", function()
----    local str = sys.fs_str()
----    term:print(str)
----end)
--
--sys.listen("update", function()
--    term:redraw()
--    if showing_stats then
--        render_stats(10, 10)
--    end
--end)

sys.log.warn("boot.lua loaded")