
// where do they come in? Without this std::min() fails to compile
#undef min
#undef max

#include <assert.h>
#include "types.h"

// 0:off (sync, stall), 1:on (async, no stall)
#define ASYNC_LOAD 0
