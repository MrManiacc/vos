/**
 * Created by jraynor on 2/27/2024.
 */
#include <nanovg.h>

#include "defines.h"
#include "platform/platform.h"

VAPI NVGcolor *color(u32 r, u32 g, u32 b, u32 a) {
    //TODO track this memory and free it when the context is destroyed
    NVGcolor *color = platform_allocate(sizeof(NVGcolor), false);
    *color = nvgRGBA(r, g, b, a);
    return color;
}

VAPI void rect(NVGcontext *vg, f32 x, f32 y, f32 w, f32 h, const NVGcolor *color) {
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    nvgFillColor(vg, *color);
    nvgFill(vg);
    nvgClosePath(vg);
}
