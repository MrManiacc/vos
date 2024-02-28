local color = boot.color(22, 23, 196, 255)

function render(delta, vg)
    --print("Render", delta, pointer)
    kernel.call("boot.rect", vg, 0.0, 0.0, 100.0, 100.0, color)
    boot.rect(vg, 100.0, 100.0, 100.0, 100.0, color)
    print("Render", delta, vg)
end




local ns = kernel.namespace("test")
ns.define("render(f64;pointer)void", render)

print(test)

function on_start()
    boot.renderer("test.render")
end