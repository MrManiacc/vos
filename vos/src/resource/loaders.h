/**
 * Created by jraynor on 2/12/2024.
 */
#pragma once

#include "resource/resource.h"

/**
 * @brief The folder loader is a special loader that will load a folder as a resource.
 * This is useful for loading a folder of assets.
 */
resource_loader *folder_loader();

/**
* @brief The script loader is a special loader that will load a lua file as a resource.
*/
resource_loader *script_loader();

/**
* @brief The binary loader is a special loader that will load a binary file as a resource.
*/
resource_loader *binary_loader();

/**
* @brief The image loader is a special loader that will load an image file as a resource.
*/
resource_loader *image_loader();

/**
 * @brief The audio loader is a special loader that will load an audio file as a resource.
 */
void register_loaders() {
    resource_register_loader(folder_loader());
    resource_register_loader(script_loader());
    resource_register_loader(binary_loader());
    resource_register_loader(image_loader());
}