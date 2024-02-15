sys.import("sys/terminal")
sys.import("sys/debug")

local term = Terminal:new()

local showing_stats = false

term:register_command("test", function()
    term:print("testing")
end)

term:register_command("path", function()
    term:print(sys.path)
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

-- Registeres a short mini game that can be played in the terminal
term:register_command("game", function()
    local game = {
        x = 10,
        y = 10,
        width = 10,
        height = 10,
        speed = 20
    }

    term:register_command("left", function()
        game.x = game.x - game.speed
    end)

    term:register_command("right", function()
        game.x = game.x + game.speed
    end)

    term:register_command("up", function()
        game.y = game.y - game.speed
    end)

    term:register_command("down", function()
        game.y = game.y + game.speed
    end)

    term:register_command("speed", function(speed)
        game.speed = tonumber(speed)
    end)

    term:register_command("size", function(width, height)
        game.width = tonumber(width)
        game.height = tonumber(height)
    end)

    term:register_command("start", function()
        sys.listen("update", function()
            sys.gui.draw_rect(game.x, game.y, game.width, game.height, 0, 255, 0, 255)
        end)
    end)

    term:register_command("stop", function()

    end)

    term:register_command("clear", function()
        term:clear()
    end)

    term:register_command("help", function()
        term:print("Commands:")
        term:print("  left")
        term:print("  right")
        term:print("  up")
        term:print("  down")
        term:print("  speed <number>")
        term:print("  size <width> <height>")
        term:print("  start")
        term:print("  stop")
        term:print("  clear")
    end)

    term:register_command("debug", function()
        local str = sys.fs_str()
        term:print(str)
    end)

    term:register_command("stats", function()
        showing = not showing
    end)
end)

sys.listen("update", function()
    term:redraw()
    if showing_stats then
        render_stats(10, 10)
    end
end)

print("Hello World!")