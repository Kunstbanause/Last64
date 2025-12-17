#include "game_state.h"

float get_time_s(void) {
  return (float)((double)get_ticks_us() / 1000000.0);
}
