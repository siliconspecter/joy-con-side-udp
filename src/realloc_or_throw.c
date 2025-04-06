#include <stdlib.h>
#include "realloc_or_throw.h"
#include "throw.h"

void *realloc_or_throw(void *const block, const size_t size)
{
  void *const output = realloc(block, size);

  if (output == NULL)
  {
    throw("Failed to realloc %u byte%s.", size, size == 1 ? "" : "s");
  }

  return output;
}
