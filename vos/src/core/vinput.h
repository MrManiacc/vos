/**
 * @file input.h
 * @author Travis Vroman (travis@kohiengine.com)
 * @brief This file contains everything having to do with input on deskop
 * environments from keyboards and mice. Gamepads and touch controls will
 * likely be handled separately at a future date.
 * @version 1.0
 * @date 2022-01-10
 *
 * @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2022
 *
 */
#pragma once

#include "defines.h"
#include "vgui.h"
#include "GLFW/glfw3.h"


/**
 * @brief Represents available mouse buttons.
 */
typedef enum buttons {
    /** @brief The left mouse button */
    BUTTON_LEFT,
    /** @brief The right mouse button */
    BUTTON_RIGHT,
    /** @brief The middle mouse button (typically the wheel) */
    BUTTON_MIDDLE,
    BUTTON_MAX_BUTTONS
} buttons;

/**
 * @brief Represents available keyboard keys.
 */
typedef enum keys {
    /** @brief The backspace key. */
    KEY_BACKSPACE = GLFW_KEY_BACKSPACE,
/** @brief The enter key. */
    KEY_ENTER = GLFW_KEY_ENTER,
/** @brief The tab key. */
    KEY_TAB = GLFW_KEY_TAB,
/** @brief The shift key. */
    KEY_SHIFT = -1, // Use GLFW_KEY_LEFT_SHIFT or GLFW_KEY_RIGHT_SHIFT
/** @brief The Control/Ctrl key. */
    KEY_CONTROL = -1, // Use GLFW_KEY_LEFT_CONTROL or GLFW_KEY_RIGHT_CONTROL

/** @brief The pause key. */
    KEY_PAUSE = GLFW_KEY_PAUSE,
/** @brief The Caps Lock key. */
    KEY_CAPITAL = GLFW_KEY_CAPS_LOCK,

/** @brief The Escape key. */
    KEY_ESCAPE = GLFW_KEY_ESCAPE,

// GLFW does not have direct equivalents for KEY_CONVERT, KEY_NONCONVERT, KEY_ACCEPT, KEY_MODECHANGE
    KEY_CONVERT = -1,
    KEY_NONCONVERT = -1,
    KEY_ACCEPT = -1,
    KEY_MODECHANGE = -1,

/** @brief The spacebar key. */
    KEY_SPACE = GLFW_KEY_SPACE,
/** @brief The page up key. */
    KEY_PAGEUP = GLFW_KEY_PAGE_UP,
/** @brief The page down key. */
    KEY_PAGEDOWN = GLFW_KEY_PAGE_DOWN,
/** @brief The end key. */
    KEY_END = GLFW_KEY_END,
/** @brief The home key. */
    KEY_HOME = GLFW_KEY_HOME,
/** @brief The left arrow key. */
    KEY_LEFT = GLFW_KEY_LEFT,
/** @brief The up arrow key. */
    KEY_UP = GLFW_KEY_UP,
/** @brief The right arrow key. */
    KEY_RIGHT = GLFW_KEY_RIGHT,
/** @brief The down arrow key. */
    KEY_DOWN = GLFW_KEY_DOWN,

// GLFW does not have direct equivalents for KEY_SELECT, KEY_PRINT, KEY_EXECUTE
    KEY_SELECT = -1,
    KEY_PRINT = -1,
    KEY_EXECUTE = -1,

/** @brief The Print Screen key. */
    KEY_PRINTSCREEN = GLFW_KEY_PRINT_SCREEN,
/** @brief The insert key. */
    KEY_INSERT = GLFW_KEY_INSERT,
/** @brief The delete key. */
    KEY_DELETE = GLFW_KEY_DELETE,

// GLFW does not have a direct equivalent for KEY_HELP
    KEY_HELP = -1,

// For alphanumeric keys, GLFW uses the ASCII values directly:
/** @brief The 0 key */
    KEY_0 = '0',
/** @brief The 1 key */
    KEY_1 = '1',
// ... all the way to
/** @brief The Z key. */
    KEY_Z = 'Z',

/** @brief The left Windows/Super key. */
    KEY_LSUPER = GLFW_KEY_LEFT_SUPER,
/** @brief The right Windows/Super key. */
    KEY_RSUPER = GLFW_KEY_RIGHT_SUPER,
/** @brief The applications key. */
    KEY_APPS = -1, // GLFW does not have a direct equivalent

/** @brief The sleep key. */
    KEY_SLEEP = -1, // GLFW does not have a direct equivalent

// For numpad keys, GLFW also has direct equivalents:
/** @brief The numberpad 0 key. */
    KEY_NUMPAD0 = GLFW_KEY_KP_0,
// ... all the way to
/** @brief The numberpad divide key. */
    KEY_DIVIDE = GLFW_KEY_KP_DIVIDE,

// Function keys:
/** @brief The F1 key. */
    KEY_F1 = GLFW_KEY_F1,
// ... all the way to
/** @brief The F24 key. */
    KEY_F24 = GLFW_KEY_F24,

/** @brief The number lock key. */
    KEY_NUMLOCK = GLFW_KEY_NUM_LOCK,
/** @brief The scroll lock key. */
    KEY_SCROLL = GLFW_KEY_SCROLL_LOCK,

// GLFW does not have a direct equivalent for KEY_NUMPAD_EQUAL
    KEY_NUMPAD_EQUAL = GLFW_KEY_KP_EQUAL,

// Modifier keys are specifically left and right in GLFW:
/** @brief The left shift key. */
    KEY_LSHIFT = GLFW_KEY_LEFT_SHIFT,
/** @brief The right shift key. */
    KEY_RSHIFT = GLFW_KEY_RIGHT_SHIFT,
/** @brief The left control key. */
    KEY_LCONTROL = GLFW_KEY_LEFT_CONTROL,
/** @brief The right control key. */
    KEY_RCONTROL = GLFW_KEY_RIGHT_CONTROL,
/** @brief The left alt key. */
    KEY_LALT = GLFW_KEY_LEFT_ALT,
/** @brief The right alt key. */
    KEY_RALT = GLFW_KEY_RIGHT_ALT,

// For the rest of the keys, GLFW uses direct equivalents:
/** @brief The semicolon key. */
    KEY_SEMICOLON = GLFW_KEY_SEMICOLON,

/** @brief The apostrophe/single-quote key */
    KEY_APOSTROPHE = GLFW_KEY_APOSTROPHE,
// No need for KEY_QUOTE as an alias in GLFW context, use KEY_APOSTROPHE directly

/** @brief The equal/plus key. */
    KEY_EQUAL = GLFW_KEY_EQUAL,
/** @brief The comma key. */
    KEY_COMMA = GLFW_KEY_COMMA,
/** @brief The minus key. */
    KEY_MINUS = GLFW_KEY_MINUS,
/** @brief The period key. */
    KEY_PERIOD = GLFW_KEY_PERIOD,
/** @brief The slash key. */
    KEY_SLASH = GLFW_KEY_SLASH,

/** @brief The grave key. */
    KEY_GRAVE = GLFW_KEY_GRAVE_ACCENT,

/** @brief The left (square) bracket key e.g. [{ */
    KEY_LBRACKET = GLFW_KEY_LEFT_BRACKET,
/** @brief The pipe/backslash key */
    KEY_PIPE = GLFW_KEY_BACKSLASH, // Also for backslash
/** @brief The right (square) bracket key e.g. ]} */
    KEY_RBRACKET = GLFW_KEY_RIGHT_BRACKET,
    KEYS_MAX_KEYS = GLFW_KEY_LAST
} keys;


typedef struct InputState {
    b8 keys[KEYS_MAX_KEYS];
    b8 prev_keys[KEYS_MAX_KEYS]; // Add this line
    b8 buttons[BUTTON_MAX_BUTTONS];
    i32 mouse_x, mouse_y;
    i32 prev_mouse_x, prev_mouse_y;
    i8 mouse_wheel_delta;
} InputState;

typedef struct WindowContext WindowContext;
/**
 * @brief Initializes the input system. Call twice; once to obtain memory requirement (passing
 * state = 0), then a second time passing allocated memory to state.
 *
 * @param memory_requirement The required size of the state memory.
 * @param state Either 0 or the allocated block of state memory.
 * @param config Ignored.
 * @returns True on success; otherwise false.
 */
b8 input_system_initialize(u64 *memory_requirement, void *state, void *config);

/**
 * @brief Shuts the input system down.
 * @param state A pointer to the system state.
 */
void input_system_shutdown(void *state);

/**
 * @brief Updates the input system every frame.
 * @param p_frame_data A constant pointer to the current frame's data.
 * NOTE: Does not use system manager update because it must be called at end of a frame.
 */
void input_reset(WindowContext *ctx);

// keyboard input

/**
 * @brief Indicates if the given key is currently pressed down.
 * @param key They key to be checked.
 * @returns True if currently pressed; otherwise false.
 */
VAPI b8 input_is_key_down(WindowContext *ctx, keys key);


/**
 * @brief Indicates if the given key is currently pressed down.
 * @param key They key to be checked.
 * @returns True if currently pressed; otherwise false.
 */
VAPI b8 input_is_key_pressed(WindowContext *ctx, keys key);

/**
 * @brief Checks if a key is currently released.
 *
 * @param key The key to be checked.
 * @return Returns true if the key is released; returns false otherwise.
 */
VAPI b8 input_is_key_released(WindowContext *ctx, keys key);

/**
*
*/
VAPI b8 input_is_key_up(WindowContext *ctx, keys key);


/**
 * @brief Sets the state for the given key.
 * @param key The key to be processed.
 * @param pressed Indicates whether the key is currently pressed.
 */
void input_process_key(WindowContext *ctx, keys key, b8 pressed);

// mouse input

/**
 * @brief Indicates if the given mouse button is currently pressed.
 * @param button The button to check.
 * @returns True if currently pressed; otherwise false.
 */
VAPI b8 input_is_button_down(WindowContext *ctx, buttons button);

/**
 * @brief Indicates if the given mouse button is currently released.
 * @param button The button to check.
 * @returns True if currently released; otherwise false.
 */
VAPI b8 input_is_button_up(WindowContext *ctx, buttons button);

/**
 * @brief Indicates if the given mouse button was previously pressed in the last frame.
 * @param button The button to check.
 * @returns True if previously pressed; otherwise false.
 */
VAPI b8 input_was_button_down(WindowContext *ctx, buttons button);

/**
 * @brief Indicates if the given mouse button was previously released in the last frame.
 * @param button The button to check.
 * @returns True if previously released; otherwise false.
 */
VAPI b8 input_was_button_up(WindowContext *ctx, buttons button);

/**
 * @brief Indicates if the mouse is currently being dragged with the provided button
 * being held down.
 *
 * @param button The button to check.
 * @returns True if dragging; otherwise false.
 */
VAPI b8 input_is_button_dragging(WindowContext *ctx, buttons button);

/**
 * @brief Obtains the current mouse position.
 * @param x A pointer to hold the current mouse position on the x-axis.
 * @param y A pointer to hold the current mouse position on the y-axis.
 */
VAPI void input_get_mouse_position(WindowContext *ctx, i32 *x, i32 *y);

/**
 * @brief Obtains the previous mouse position.
 * @param x A pointer to hold the previous mouse position on the x-axis.
 * @param y A pointer to hold the previous mouse position on the y-axis.
 */
VAPI void input_get_previous_mouse_position(WindowContext *ctx, i32 *x, i32 *y);

/**
 * @brief Sets the press state of the given mouse button.
 * @param button The mouse button whose state to set.
 * @param pressed Indicates if the mouse button is currently pressed.
 */
void input_process_button(WindowContext *ctx, buttons button, b8 pressed);

/**
 * @brief Sets the current position of the mouse to the given x and y positions.
 * Also updates the previous position beforehand.
 */
void input_process_mouse_move(WindowContext *ctx, i16 x, i16 y);

/**
 * @brief Processes mouse wheel scrolling.
 * @param z_delta The amount of scrolling which occurred on the z axis (mouse wheel)
 */
void input_process_mouse_wheel(WindowContext *ctx, i8 z_delta);


VAPI void input_get_scroll_delta(WindowContext *ctx, i8 *x, i8 *y);
/**
 * @brief Returns a string representation of the provided key. Ex. "tab" for the tab key.
 *
 * @param key
 * @return const char*
 */
VAPI const char *input_keycode_str(WindowContext *ctx, keys key);

/**
 * @brief Pushes a new keymap onto the keymap stack, making it the active keymap.
 * A copy of the keymap is taken when pushing onto the stack.
 *
 * @param map A constant pointer to the keymap to be pushed.
 */
VAPI void input_keymap_push(const struct keymap *map);

/**
 * @brief Attempts to pop the top-most keymap from the stack, if there is one.
 *
 * @return True if a keymap was popped; otherwise false.
 */
VAPI b8 input_keymap_pop(void);