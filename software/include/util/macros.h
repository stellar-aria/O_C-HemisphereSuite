#ifndef UTIL_MACROS_H_
#define UTIL_MACROS_H_

#include "stmlib/stmlib.h"
#include <string.h>

#ifndef SWAP
#define SWAP(a, b)   \
  do {               \
    typeof(a) t = a; \
    a = b;           \
    b = t;           \
  } while (0)
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#endif  // UTIL_MACROS_H_
