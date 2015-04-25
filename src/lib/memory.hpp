#pragma once


namespace NewCfr {
#define FREE_POINTER(ptr)                                                      \
  {                                                                            \
    if (ptr) {                                                                 \
      free(static_cast<void *>(ptr));                                          \
      ptr = nullptr;                                                           \
    }                                                                          \
  }
#define DELETE_POINTER(ptr)                                                    \
  {                                                                            \
    if (ptr) {                                                                 \
      delete ptr;                                                              \
      ptr = nullptr;                                                           \
    }                                                                          \
  }
#define DELETE_ARRAY(array)                                                    \
  {                                                                            \
    if (array) {                                                               \
      delete[] array;                                                          \
      array = nullptr;                                                         \
    }                                                                          \
  }
}
