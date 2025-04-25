#ifndef __CLI__
#define __CLI__

#include <stdint.h>

typedef struct __attribute__((aligned(8))){
  const uint8_t private_data[24];
}Cli_h;

int8_t cli_init(Cli_h* const self)__attribute__((__nonnull__(1)));
int8_t cli_start(Cli_h* const self)__attribute__((__nonnull__(1)));

#endif // !__CLI__
