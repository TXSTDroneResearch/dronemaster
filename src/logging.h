#ifndef _LOGGING_H_
#define _LOGGING_H_

#ifdef _DEBUG
#define PERROR(...)                                                    \
  do {                                                                 \
    fprintf(stderr, "%s:%d:%s(): ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(stderr, __VA_ARGS__);                                      \
  } while (0)

#define PINFO(...)                                            \
  do {                                                        \
    printf("%s:%d:%s(): ", __FILE__, __LINE__, __FUNCTION__); \
    printf(__VA_ARGS__);                                      \
  } while (0)
#else
#define PERROR(...)
#define PINFO(...)
#endif

#endif  // _LOGGING_H_