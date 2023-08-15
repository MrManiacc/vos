
//! Graphics functions exposed to Lua scripts.

use crate::kernel::graphics_lib::GraphicsLib;

/// Draws a triangle using the graphics library.
///
/// # Parameters
/// 
/// - `graphics`: The GraphicsLib instance.
///
/// # Returns
///
/// Returns a Result indicating the success or failure of the drawing operation.
pub fn draw_triangle(graphics: &GraphicsLib) -> Result<(), &'static str> {
    graphics.draw_triangle()
}
