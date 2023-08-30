#pragma once
#include "defines.h"
typedef struct AssetWatcher AssetWatcher;

AssetWatcher *asset_watcher_initialize(const char *path, b8 *running);

void asset_watcher_shutdown(AssetWatcher *watcher);