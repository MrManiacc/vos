/**
 * Created by jraynor on 2/14/2024.
 */
#pragma once

#include "defines.h"
#include "gl/glew.h"

#define GLFW_INCLUDE_GLEXT

#include <GLFW/glfw3.h>
#include "nanovg.h"

#define NANOVG_GL3_IMPLEMENTATION

#include "kernel/kernel.h"


/**
 * @class vos_context
 * @brief The vos_context class represents the context for the VOS application.
 *
 * The vos_context class encapsulates the necessary data needed for the VOS application.
 * It includes the GLFW window, NVGcontext, width, height, and pixel ratio of the application.
 */
static struct window_context {
    GLFWwindow *window; // The GLFW window.
    NVGcontext *vg; // The NanoVG context.
    int width, height; // The width and height of the window.
    float pixel_ratio; // The pixel ratio of the window.
} window_context;


/**
 * @brief Initializes the VOS context with the given parameters.
 *
 * @param ctx Pointer to the VOS context to be initialized.
 * @param title The title of the VOS application.
 * @param width The width of the VOS application window.
 * @param height The height of the VOS application window.
 * @return b8 Returns true if the initialization succeeds, otherwise false.
 *
 * This function initializes the VOS context with the given parameters. It sets up the VOS application window
 * with the specified title, width, and height. The VOS context must be allocated and passed as the first parameter.
 *
 * Example usage:
 * @code{.c}
 * vos_context ctx;
 * b8 result = window_initialize(&ctx, "My Application", 800, 600);
 * if (result) {
 *     // Initialization succeeded.
 * } else {
 *     // Initialization failed.
 * }
 * @endcode
 */
b8 window_initialize(const char *title, int width, int height);

/**
 * @brief Starts a new frame for rendering.
 *
 * This function should be called at the beginning of each frame before any rendering calls are made.
 *
 * @param ctx A pointer to the vos_context struct representing the current application context.
 *
 * @see window_end_frame()
 */
void window_begin_frame();


/**
 * @brief Determines whether the window should close.
 *
 * @return True if the window should close, false otherwise.
 */
b8 window_should_close();

/**
 * @brief Get the size of the window
 *
 * This function retrieves the width and height of the window.
 *
 * @param[out] width  A pointer to the variable that will hold the width of the window
 * @param[out] height A pointer to the variable that will hold the height of the window
 *
 * @note The variables pointed by 'width' and 'height' will be modified by this function
 */
void window_get_size(u32  *width, u32 *height);

/**
 * @brief Ends a frame in the vos_context.
 *
 * This function is used to end a frame in the vos_context.
 * It performs any necessary cleanup or operations required for ending the frame.
 *
 * @param ctx The vos_context object.
 */
void window_end_frame();

/**
 * @brief Destroys the VOS context.
 *
 * This function destroys the VOS context and frees any resources associated with it.
 *
 * @param ctx The VOS context to be destroyed.
 */
void window_shutdown();

/**
 * @brief Loads a font from the specified font path and assigns it the given font name.
 *
 * @param font_path The path of the font file to be loaded.
 * @param font_name The name to be assigned to the loaded font.
 *
 * @return Returns 1 if the font is successfully loaded, 0 otherwise.
 *
 * @note This function is used to load a font for further usage in the application.
 *       The font_path parameter should provide the path to the font file,
 *       and the font_name parameter should specify the name to be assigned to the font.
 *       The return value indicates whether the font was successfully loaded or not.
 *       A return value of 1 indicates a successful loading, while 0 represents a failure.
 */
VAPI b8 gui_load_font(FsPath font_path, const char *font_name);

VAPI/**
 * @brief Draws text on the GUI with the specified parameters.
 *
 * This function is responsible for drawing text on the GUI using the specified font, color, size, and position.
 *
 * @param text      The text to be drawn on the GUI.
 * @param x         The x-coordinate of the starting position for drawing the text.
 * @param y         The y-coordinate of the starting position for drawing the text.
 * @param size      The size of the text to be drawn.
 * @param font_name The name of the font to be used for drawing the text.
 * @param color     The color of the text to be drawn.
 */
void gui_draw_text(const char *text, float x, float y, float size, const char *font_name, NVGcolor color);

VAPI/**
 * @brief Draws a rectangle on the GUI with the given coordinates, size, and color.
 *
 * This function is used to draw a rectangle on the GUI using the specified coordin*/
void gui_draw_rect(float x, float y, float width, float height, NVGcolor color);


/**
 * Calculate the width of a given text string using the specified font and size.
 *
 * @param text The text string to measure.
 * @param font_name The name of the font to use for measuring the text.
 * @param size The size of the font to use for measuring the text.
 *
 * @return The calculated width of the text string.
 */
f32 gui_text_width(const char *text, const char *font_name, f32 size);