sys.import("sys/terminal")
sys.import("sys/debug")

local term = Terminal:new()

local showing_stats = false

term:register_command("test", function()
    term:print("testing")
end)

term:register_command("clear", function()
    term:clear()
end)

term:register_command("exit", function()
    sys.exit()
end)

term:register_command("help", function()
    term:print("Commands:")
    term:print("  test")
    term:print("  clear")
    term:print("  exit")
end)

term:register_command("debug", function()
    local str = sys.fs_str()
    term:print(str)
end)

term:register_command("stats", function()
    showing_stats = not showing_stats
end)

sys.listen("update", function()
    term:redraw()
    if showing_stats then
        render_stats(10, 10)
    end
end)
