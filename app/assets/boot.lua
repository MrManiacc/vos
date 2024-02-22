sys.import("sys/terminal")
--sys.import("sys/debug")
--
local term = Terminal:new()
--
--local showing_stats = false
--
term:register_command("test", function()
    term:print("testing")
end)
--
--term:register_command("path", function()
--    term:print(sys.path)
--end)
--
--term:register_command("clear", function()
--    term:clear()
--end)
--
--term:register_command("exit", function()
--    sys.exit()
--end)
--
--term:register_command("help", function()
--    term:print("Commands:")
--    term:print("  test")
--    term:print("  clear")
--    term:print("  exit")
--    term:print("  path")
--    term:print("  debug")
--    term:print("  stats")
--end)
--
--term:register_command("debug", function()
--    local str = sys.fs_str()
--    term:print(str)
--end)
--
--term:register_command("stats", function()
--    showing_stats = not showing_stats
--end)
--
--term:register_command("debug", function()
--    local str = sys.fs_str()
--    term:print(str)
--end)
--
--system.listen("update", function()
--    term:redraw()
--    if showing_stats then
--        render_stats(10, 10)
--    end
--end)
--


local function draw_screen()
    term:redraw()
end

sys.listen("update", draw_screen)
sys.log.warn("boot.lua loaded")