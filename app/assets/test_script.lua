local ns = kernel.namespace("test")

function _init_self()
    --kernel.call("boot.render", 0.5, nil)
    --local result = kernel.call("boot.tell_secret", "James")
end

ns.define("say_hello(string)void", function(name)
    print("Hello, " .. name)
end)
