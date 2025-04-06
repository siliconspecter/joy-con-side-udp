#ifndef REALLOC_OR_THROW_H

#define REALLOC_OR_THROW_H

/**
 * Resizes previously allocated memory, halting execution with an error message
 * should the operation fail.
 * @param block A pointer to the memory to resize.
 * @param size The number of bytes to allocate.
 * @return A pointer to the resized memory.
 */
void *realloc_or_throw(void *const block, const size_t size);

#endif
