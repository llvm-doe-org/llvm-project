// Header file to demonstrate where unique OpenACC may reside

/// Print a generic message string for OpenACC from libomptarget or a plugin RTL
#define OPENACC_MESSAGE0(_str)                                                         \
  do {                                                                         \
    fprintf(stderr, "OpenACC (via Libomptarget) message: %s\n", _str);              \
  } while (0)

// Print a printf formatting string message for OpenACC from libomptarget or a plugin RTL
#define OPENACC_MESSAGE(_str, ...)                                                     \
  do {                                                                         \
    fprintf(stderr, "OpenACC (via Libomptarget) message: " _str "\n", __VA_ARGS__); \
  } while (0)

