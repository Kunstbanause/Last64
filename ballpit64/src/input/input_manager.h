#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <libdragon.h>

// Combined input functions - allow any controller port to control the game
joypad_buttons_t joypad_get_all_pressed(void);
joypad_buttons_t joypad_get_all_held(void);
joypad_inputs_t joypad_get_all_inputs(void);

#endif // INPUT_MANAGER_H
