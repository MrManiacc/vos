sys.import("sys/terminal")

local term = Terminal:new()

term:register_command("test", function()
    sys.log("test command")
end)

sys.listen("update", function()
    term:redraw()
end)
