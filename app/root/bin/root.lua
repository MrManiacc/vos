local color = boot.color(22, 23, 196, 255)

kernel.namespace("test")
      .define("render(f64;pointer)void",
        function(delta, vg)
            --print("Render", delta, pointer)
            kernel.call("boot.rect", vg, 0.0, 0.0, 100.0, 100.0, color)
            boot.rect(vg, 100.0, 100.0, 100.0, 100.0, color)
            --print("Render", delta, vg)
        end)
      .define("tests()void",
        function()
            print("Test2")
        end)


function on_start()
    boot.renderer(test.render)
    test.tests()

end

