// /**
//  * Created by jraynor on 2/25/2024.
//  */
//
// #include <nanovg.h>
//
// #include "kern/kernel.h"
// #include "core/vlogger.h"
// // initialized automatically
// static Kernel *_kernel = null;
//
// VAPI i32 fib(const i32 n) {
//     if (n <= 1) {
//         return n;
//     }
//     return fib(n - 1) + fib(n - 2);
// }
//
//
// VAPI void render(const f64 delta, NVGcontext *vg) {
//     //Draws and animated a circle
//     nvgBeginPath(vg);
//     nvgCircle(vg, 400, 300, 100);
//     nvgFillColor(vg, nvgRGB(255, 0, 0));
//     nvgFill(vg);
//     nvgClosePath(vg);
//     nvgBeginPath(vg);
//     nvgCircle(vg, 400, 300, 100);
//     nvgStrokeColor(vg, nvgRGB(0, 0, 255));
//     nvgStroke(vg);
//     nvgClosePath(vg);
//     nvgBeginPath(vg);
//     nvgCircle(vg, 400, 300, 100);
//     nvgFillColor(vg, nvgRGB(0, 255, 0));
//     nvgFill(vg);
//     nvgClosePath(vg);
// }
//
// VAPI b8 prime(const i32 n) {
//     for (int i = 2; i <= n / 2; i++) {
//         if (n % i == 0) {
//             return false;
//         }
//     }
//     return true;
// }
//
//
// VAPI i32 fib_from_script(const i32 n) {
//     Process *process = kernel_process_find(_kernel, "test_script.lua");
//     if (process == null) {
//         verror("Failed to find process");
//         return -1;
//     }
//     const Function *script_fib = kernel_process_function_lookup(process, (FunctionSignature){
//         .name = "fib",
//         .arg_count = 1,
//         .return_type = FUNCTION_TYPE_I32,
//         .args[0] = FUNCTION_TYPE_I32
//     });
//
//     if (script_fib == null) {
//         verror("Failed to find test script function");
//         return -1;
//     }
//     const FunctionResult result = kernel_process_function_call(script_fib, n);
//     return result.data.i32;
// }
//
//
// kernel_define_driver();
