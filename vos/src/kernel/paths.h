#pragma once

#include "defines.h"

/**
 * Moves the current directory to the given path. If no root path is given, the current working directory will be used.
 * Root directory can only be set once. It will be set by the first call to this function.
 *
 * The provided path can relative or absolute. If the path is relative, it will be relative to the current working directory.
 *
 * ‼️This must be called at least once before any other path functions are called.‼️
 */
void path_move(char *path);

/**
 * Gets the current working directory.
 * @return The current working directory.
 */
char *path_root_directory();

/**
 * Gets the current directory.
 * @return The current directory.
 */
char *path_current_directory();


/**
 * Normalizes a path, this will convert windows style paths to unix style paths.
 * @param path
 * @return
 */
char *path_normalize(char *path);

/**
 * Normalizes a path, this will convert windows style paths to unix style paths.
 * Will also convert relative paths to absolute paths.
 * @param path The path to normalize.
 * @return The normalized path.
 */
char *path_relative(char *path);

/**
 * Gets the absolute path from a relative path.
 * @param path The relative path.
 * @return The absolute path.
 */
char *path_absolute(char *path);

/**
 * Gets the parent directory of a path.
 * @param path The path to get the parent directory from.
 * @return The parent directory of the path.
 */
char *path_parent_directory(char *path);

/**
 * Gets the file name from a path.
 * @param path The path to get the file name from.
 * @return The file name from the path.
 */
char *path_file_name(char *path);

/**
 * Gets the file extension from a path.
 * @param path The path to get the file extension from.
 * @return The file extension from the path.
 */
char *path_file_extension(char *path);

/**
 * Gets the file name without the extension from a path.
 * @param path The path to get the file name without the extension from.
 * @return The file name without the extension from the path.
 */
char *path_file_name_without_extension(char *path);


