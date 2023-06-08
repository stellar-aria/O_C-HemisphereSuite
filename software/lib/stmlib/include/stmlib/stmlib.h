#pragma once

#include <stddef.h>
#include <stdint.h>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#define CLIP(x)                 \
  do {                          \
    if (x < -32767) x = -32767; \
    if (x > 32767) x = 32767;   \
  } while (0)

#define CONSTRAIN(var, min, max) \
  do {                           \
    if (var < (min)) {           \
      var = (min);               \
    } else if (var > (max)) {    \
      var = (max);               \
    }                            \
  } while (0)

