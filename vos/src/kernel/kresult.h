/**
 * Created by jraynor on 8/26/2023.
 */
#pragma once
#include "defines.h"

/**
 * This is used to mark the result of a kernel operation.
 * All error codes are > KERNEL_SUCCESS.
 */
typedef enum KernelCode {
  /**
   * The kernel vfs user limit has been reached.
   * @data the max number of users that can be created (u32)
   */
  KERNEL_VFS_USER_LIMIT_REACHED = -7,
  /**
   * The kernel id pool has overflowed.
   * @data the max id that was generated (u32)
   */
  KERNEL_ID_POOL_OVERFLOW = -6,
  /**
   * The kernel has already been initialized.
   * @data no data is returned
   */
  KERNEL_ALREADY_INITIALIZED = -5,
  /**
   * The kernel has already been shutdown.
   * @data no data is returned
   */
  KERNEL_ALREADY_SHUTDOWN = -4,
  /**
   * The kernel has not been initialized.
   * @data no data is returned
   */
  KERNEL_CALL_BEFORE_INIT = -3,
  /**
   * The process was not found.
   * @data a pointer to the process id that was not found
   */
  KERNEL_PROCESS_NOT_FOUND = -2,
  /**
   * The operation failed due to a lack of memory.
   * @data total memory used
   */
  KERNEL_ERROR_OUT_OF_MEMORY = -1,

  /**
   * A general kernel error occurred.
   * @data a general error message
   */
  KERNEL_ERROR = 0,

  /**
   * The operation was successful. This allows KernelCode to be used as a boolean. 0 is SUCCESS, all other values are errors.
   * @data may be NULL depending on the operation, may pass a message.
   */
  KERNEL_SUCCESS = 1,

  /**
   * A kernel process was successfully created.
   * This is a success code.
   * @data A pointer to the process id.
   */
  KERNEL_PROCESS_CREATED = 2,
  /**
   * A kernel vfs was successfully created.
   * @date A pointer to the FileSystem
   */
  KERNEL_VFS_CREATED = 3,
  /**
   * A user  was successfully created.
   * @date A pointer to the User
   */
  KERNEL_VFS_USER_CREATED = 4,
  /**
   * A group  was successfully created.
   * @date A pointer to the Group
   */
  KERNEL_VFS_GROUP_CREATED = 5,
  /**
   * A node  was successfully created.
   * @date A pointer to the Node
   */
  KERNEL_VFS_NODE_CREATED = 6,
  /**
   * A node  was successfully read.
   * @date A char* to the read data
   */
  KERNEL_VFS_NODE_READ = 7,

  /**
   * A node  was successfully list of nodes.
   * @date A Node** to the list of nodes
   */
  KERNEL_VFS_NODE_LIST = 8,
  /**
   * A node  was successfully symlinked.
   * @date A Node* to the symlink
   */
  KERNEL_SYM_LINK_CREATED = 9,

} KernelCode;

/**
 * Allows us to return a result from a kernel operation.
 * @param code The error code.
 * @param data The data to return.
 */
typedef struct KernelResult {
  // The error code of the operation.
  KernelCode code;
  // Can be used to return data from a kernel operation, may be NULL depending on the operation.
  void *data;
} KernelResult;

/**
 * Determines if the kernel operation was successful.
 * @param code The error code.
 * @return TRUE if the operation was successful, else FALSE.
 */
b8 is_kernel_success(KernelCode code);

/**
 * Gets the result message for a kernel operation.
 * @param result The result of the kernel operation.
 * @return
 */
const char *get_kernel_result_message(KernelResult result);