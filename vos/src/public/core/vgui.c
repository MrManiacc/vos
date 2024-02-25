/**
 * Created by jraynor on 2/14/2024.
 */
#include "vinput.h"
#include "vgui.h"
#include "vlogger.h"


/**
 * Created by jraynor on 2/14/2024.
 */
#include "vlogger.h"
#include "nanovg_gl.h"
#include "vmem.h"
#include "filesystem/vfs.h"
#include "kernel/kernel.h"
#include "platform/platform.h"

void input_reset(WindowContext *window_context) {
    memcpy(window_context->input_state->prev_keys, window_context->input_state->keys,
        sizeof(window_context->input_state->keys));
    memcpy(window_context->input_state->prev_buttons, window_context->input_state->buttons,
        sizeof(window_context->input_state->buttons));
}


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    WindowContext *window_context = ((WindowContext *) glfwGetWindowUserPointer(window));
    InputState *input_state = window_context->input_state;

    if (key >= 0 && key < KEYS_MAX_KEYS) {
        input_state->keys[key] = action != GLFW_RELEASE;
    }
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    WindowContext *window_context = ((WindowContext *) glfwGetWindowUserPointer(window));
    // Map GLFW button to buttons enum and update state
    if (button >= 0 && button < BUTTON_MAX_BUTTONS) {
        b8 pressed = action != GLFW_RELEASE;
        window_context->input_state->buttons[button] = pressed;
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    WindowContext *window_context = ((WindowContext *) glfwGetWindowUserPointer(window));
    InputState *input_state = window_context->input_state;
    // Update mouse position
    window_context->input_state->prev_mouse_x = input_state->mouse_x;
    input_state->prev_mouse_y = input_state->mouse_y;
    input_state->mouse_x = (i32) xpos;
    input_state->mouse_y = (i32) ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    WindowContext *window_context = ((WindowContext *) glfwGetWindowUserPointer(window));
    InputState *input_state = window_context->input_state;
    // Update mouse wheel delta
    input_state->mouse_wheel_delta += (i8) yoffset;
}

void window_resize_callback(GLFWwindow *window, int width, int height) {
    WindowContext *window_context = ((WindowContext *) glfwGetWindowUserPointer(window));
    // Update window context
    window_context->width = width;
    window_context->height = height;
}


// Input update is not needed as GLFW handles input in its event loop, but can be used for per-frame updates

// Input API implementations
VAPI b8 input_is_key_down(WindowContext *context, keys key) {
    return context->input_state->keys[key];
}

VAPI b8 input_is_key_up(WindowContext *context, keys key) {
    return !context->input_state->keys[key];
}

VAPI b8 input_is_button_down(WindowContext *context, buttons button) {
    return context->input_state->buttons[button];
}

VAPI b8 input_is_button_pressed(WindowContext *context, buttons button) {
    return context->input_state->buttons[button] && !context->input_state->prev_buttons[button];
}

VAPI void input_get_mouse_position(WindowContext *context, i32 *x, i32 *y) {
    *x = context->input_state->mouse_x;
    *y = context->input_state->mouse_y;
}

VAPI b8 input_is_button_up(WindowContext *context, buttons button) {
    return !context->input_state->buttons[button];
}

VAPI b8 input_is_key_pressed(WindowContext *context, keys key) {
    return context->input_state->keys[key] && !context->input_state->prev_keys[key];
}

VAPI b8 input_is_key_released(WindowContext *context, keys key) {
    return !context->input_state->keys[key] && context->input_state->prev_keys[key];
}

void input_get_scroll_delta(WindowContext *ctx, i8 *x, i8 *y) {
    *x = ctx->input_state->mouse_wheel_delta;
    *y = ctx->input_state->mouse_wheel_delta;
    ctx->input_state->mouse_wheel_delta = 0;
}

b8 window_initialize(WindowContext *window_context, const char *title, int width, int height) {
    window_context->input_state = kallocate(sizeof(InputState), MEMORY_TAG_KERNEL);
    NVGcontext *vg = NULL;
    if (!glfwInit()) {
        verror("Failed to init GLFW.");
        return false;
    }

#ifndef _WIN32 // don't require this on win32, and works with more cards
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#elif __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    // Set window hints and create the window
    window_context->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window_context->window) {
        // Handle error
    }
    if (!window_context->window) {
        verror("Failed to create GLFW window.");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_context->window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        verror("Failed to init GLEW.");
        return -1;
    }
    glGetError();
    vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (vg == null) {
        verror("Failed to init nanovg.");
        return false;
    }
    window_context->vg = vg;
    window_context->width = width;
    window_context->height = height;
    window_context->pixel_ratio = 1.0f;
    glfwSetWindowUserPointer(window_context->window, window_context);
    glfwSetKeyCallback(window_context->window, key_callback);
    glfwSetMouseButtonCallback(window_context->window, mouse_button_callback);
    glfwSetCursorPosCallback(window_context->window, cursor_position_callback);
    glfwSetScrollCallback(window_context->window, scroll_callback);
    glfwSetWindowSizeCallback(window_context->window, window_resize_callback);
    return true;
}

void window_begin_frame(WindowContext *window_context) {
    glfwGetFramebufferSize(window_context->window, &window_context->width, &window_context->height);
    glViewport(0, 0, window_context->width, window_context->height);
    // Clears a nice light purple color.
    glClearColor(0.694f, 0.282f, 0.823f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(window_context->vg, window_context->width, window_context->height,
        window_context->width / window_context->height);
}

void window_end_frame(WindowContext *window_context) {
    nvgEndFrame(window_context->vg);
    glfwSwapBuffers(window_context->window);
    input_reset(window_context); // Reset input state after processing all events.
    glfwPollEvents();
}

void window_shutdown(WindowContext *window_context) {
    nvgDeleteGL3(window_context->vg);
    glfwDestroyWindow(window_context->window);
    glfwTerminate();

    window_context->window = null;
    window_context->vg = null;
    window_context->width = 0;
    window_context->height = 0;
    window_context->pixel_ratio = 0.0f;
}

b8 window_should_close(WindowContext *window_context) {
    return glfwWindowShouldClose(window_context->window);
}

b8 gui_load_font(struct Kernel *kernel, const char *font_path, const char *font_name) {
    //load our fonts
    // WindowContext *window_context = kernel->window_context;
    // FsNode *font = vfs_node_get(kernel->fs_context, font_path);
    // if (font == null) {
    // verror("Failed to load font");
    // return false;
    // }
    // char *font_data = font->data.file.data;
    // u32 font_data_len = font->data.file.size;
    // nvgCreateFontMem(window_context->vg, font_name, (unsigned char *) font_data, font_data_len, 0);
    // vdebug("Loaded font: %s", font_name)
    return true;
}

void gui_draw_text(WindowContext *window_context, const char *text, float x, float y, float size, const char *font_name,
    NVGcolor color, i32 alignment) {
    nvgFontSize(window_context->vg, size);
    nvgFontFace(window_context->vg, font_name);
    nvgFillColor(window_context->vg, color);
    nvgTextAlign(window_context->vg, alignment);
    nvgText(window_context->vg, x, y, text, null);
}


void gui_draw_line(WindowContext *window_context, float x1, float y1, float x2, float y2, float size, NVGcolor color) {
    nvgBeginPath(window_context->vg);
    nvgMoveTo(window_context->vg, x1, y1);
    nvgLineTo(window_context->vg, x2, y2);
    nvgStrokeWidth(window_context->vg, size);
    nvgStrokeColor(window_context->vg, color);
    nvgStroke(window_context->vg);
}

void gui_scissor(WindowContext *window_context, float x, float y, float width, float height) {
    nvgScissor(window_context->vg, x, y, width, height);
}

void gui_reset_scissor(WindowContext *window_context) {
    nvgResetScissor(window_context->vg);
}

void gui_draw_rect(WindowContext *window_context, float x, float y, float width, float height, NVGcolor color) {
    nvgBeginPath(window_context->vg);
    nvgRect(window_context->vg, x, y, width, height);
    nvgFillColor(window_context->vg, color);
    nvgFill(window_context->vg);
}

f32 gui_text_width(WindowContext *window_context, const char *text, const char *font_name, f32 size) {
    nvgFontSize(window_context->vg, size);
    nvgFontFace(window_context->vg, font_name);
    return nvgTextBounds(window_context->vg, 0, 0, text, null, null);
}

void window_get_size(WindowContext *window_context, u32 *width, u32 *height) {
    *width = window_context->width;
    *height = window_context->height;
}

void gui_get_text_bounds(WindowContext *window_context, const char *text, const char *font_name, f32 size, float *bounds) {
    nvgFontSize(window_context->vg, size);
    nvgFontFace(window_context->vg, font_name);
    nvgTextBounds(window_context->vg, 0, 0, text, null, bounds);
}

void gui_draw_rounded_rect(WindowContext *window_context, float x, float y, float width, float height, float radius,
    NVGcolor color) {
    nvgBeginPath(window_context->vg);
    nvgRoundedRect(window_context->vg, x, y, width, height, radius);
    nvgFillColor(window_context->vg, color);
    nvgFill(window_context->vg);
}
