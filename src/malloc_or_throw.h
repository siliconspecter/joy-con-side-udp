#ifndef MALLOC_OR_THROW_H

#define MALLOC_OR_THROW_H

#include <stdlib.h>

/**
 * Allocates memory, halting execution with an error message should the
 * allocation fail.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory.
 */
void *malloc_or_throw(const size_t size);

#endif
