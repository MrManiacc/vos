/**
 * Created by jraynor on 2/14/2024.
 */
#include "vgui.h"
#include "vlogger.h"


/**
 * Created by jraynor on 2/14/2024.
 */
#include "vlogger.h"
#include "nanovg_gl.h"
#include "vinput.h"

// Input state structures
typedef struct {
    b8 keys[KEYS_MAX_KEYS];
    b8 prev_keys[KEYS_MAX_KEYS]; // Add this line
    b8 buttons[BUTTON_MAX_BUTTONS];
    i32 mouse_x, mouse_y;
    i32 prev_mouse_x, prev_mouse_y;
    i8 mouse_wheel_delta;
} InputState;

static InputState g_input_state;

void input_reset(void) {
    memcpy(g_input_state.prev_keys, g_input_state.keys, sizeof(g_input_state.keys));
}


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key >= 0 && key < KEYS_MAX_KEYS) {
        g_input_state.keys[key] = action != GLFW_RELEASE;
    }
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    // Map GLFW button to buttons enum and update state
    if (button >= 0 && button < BUTTON_MAX_BUTTONS) {
        b8 pressed = action != GLFW_RELEASE;
        g_input_state.buttons[button] = pressed;
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
    // Update mouse position
    g_input_state.prev_mouse_x = g_input_state.mouse_x;
    g_input_state.prev_mouse_y = g_input_state.mouse_y;
    g_input_state.mouse_x = (i32) xpos;
    g_input_state.mouse_y = (i32) ypos;
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    // Update mouse wheel delta
    g_input_state.mouse_wheel_delta += (i8) yoffset;
}

void window_resize_callback(GLFWwindow *window, int width, int height) {
    // Update window context
    window_context.width = width;
    window_context.height = height;
}


// Input update is not needed as GLFW handles input in its event loop, but can be used for per-frame updates

// Input API implementations
VAPI b8 input_is_key_down(keys key) {
    return g_input_state.keys[key];
}

VAPI b8 input_is_key_up(keys key) {
    return !g_input_state.keys[key];
}

VAPI b8 input_is_button_down(buttons button) {
    return g_input_state.buttons[button];
}

VAPI void input_get_mouse_position(i32 *x, i32 *y) {
    *x = g_input_state.mouse_x;
    *y = g_input_state.mouse_y;
}

VAPI b8 input_is_button_up(buttons button) {
    return !g_input_state.buttons[button];
}

VAPI b8 input_is_key_pressed(keys key) {
    return g_input_state.keys[key] && !g_input_state.prev_keys[key];
}

VAPI b8 input_is_key_released(keys key) {
    return !g_input_state.keys[key] && g_input_state.prev_keys[key];
}

b8 window_initialize(const char *title, int width, int height) {
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
    window_context.window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window_context.window) {
        // Handle error
    }
    if (!window_context.window) {
        verror("Failed to create GLFW window.");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_context.window);
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
    window_context.vg = vg;
    window_context.width = width;
    window_context.height = height;
    window_context.pixel_ratio = 1.0f;
    glfwSetKeyCallback(window_context.window, key_callback);
    glfwSetMouseButtonCallback(window_context.window, mouse_button_callback);
    glfwSetCursorPosCallback(window_context.window, cursor_position_callback);
    glfwSetScrollCallback(window_context.window, scroll_callback);
    glfwSetWindowSizeCallback(window_context.window, window_resize_callback);
    return true;
}

void window_begin_frame() {
    glfwGetFramebufferSize(window_context.window, &window_context.width, &window_context.height);
    glViewport(0, 0, window_context.width, window_context.height);
    // Clears a nice light purple color.
    glClearColor(0.694f, 0.282f, 0.823f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    nvgBeginFrame(window_context.vg, window_context.width, window_context.height,window_context.width/window_context.height);
}

void window_end_frame() {
    nvgEndFrame(window_context.vg);
    glfwSwapBuffers(window_context.window);
    input_reset(); // Reset input state after processing all events.
    glfwPollEvents();
}

void window_shutdown() {
    nvgDeleteGL3(window_context.vg);
    glfwDestroyWindow(window_context.window);
    glfwTerminate();

    window_context.window = null;
    window_context.vg = null;
    window_context.width = 0;
    window_context.height = 0;
    window_context.pixel_ratio = 0.0f;
}

b8 window_should_close() {
    return glfwWindowShouldClose(window_context.window);
}

b8 gui_load_font(FsPath font_path, const char *font_name) {
    //load our fonts
    FsNode *font = vfs_node_get(font_path);
    if (font == null) {
        verror("Failed to load font");
        return false;
    }
    char *font_data = font->data.file.data;
    u32 font_data_len = font->data.file.size;
    nvgCreateFontMem(window_context.vg, font_name, (unsigned char *) font_data, font_data_len, 0);
    vdebug("Loaded font: %s", font_name)
    return true;
}

void gui_draw_text(const char *text, float x, float y, float size, const char *font_name, NVGcolor color) {
    nvgFontSize(window_context.vg, size);
    nvgFontFace(window_context.vg, font_name);
    nvgFillColor(window_context.vg, color);
    nvgText(window_context.vg, x, y, text, null);
}

void gui_draw_rect(float x, float y, float width, float height, NVGcolor color) {
    nvgBeginPath(window_context.vg);
    nvgRect(window_context.vg, x, y, width, height);
    nvgFillColor(window_context.vg, color);
    nvgFill(window_context.vg);
}

f32 gui_text_width(const char *text, const char *font_name, f32 size) {
    nvgFontSize(window_context.vg, size);
    nvgFontFace(window_context.vg, font_name);
    return nvgTextBounds(window_context.vg, 0, 0, text, null, null);
}

void window_get_size(u32 *width, u32 *height) {
    *width = window_context.width;
    *height = window_context.height;
}
