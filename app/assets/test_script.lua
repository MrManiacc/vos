local ns = kernel.namespace("test")

local color = kernel.call("boot.color", 255, 0, 0, 255)

ns.define("render(f64;pointer)void", function(delta, pointer)
    print("Render", delta, pointer)
    kernel.call("boot.rect", pointer, 0.0, 0.0, 100.0, 100.0, color)
end)

kernel.call("boot.register_render", "test.render")
