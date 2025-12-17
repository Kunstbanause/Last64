#include "input_manager.h"

joypad_buttons_t joypad_get_all_pressed(void) {
    joypad_buttons_t combined = {0};
    for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
        joypad_buttons_t b = joypad_get_buttons_pressed((joypad_port_t)i);
        
        combined.a      |= b.a;
        combined.b      |= b.b;
        combined.z      |= b.z;
        combined.start  |= b.start;
        combined.l      |= b.l;
        combined.r      |= b.r;
        combined.d_up   |= b.d_up;
        combined.d_down |= b.d_down;
        combined.d_left |= b.d_left;
        combined.d_right|= b.d_right;
        combined.c_up   |= b.c_up;
        combined.c_down |= b.c_down;
        combined.c_left |= b.c_left;
        combined.c_right|= b.c_right;
    }
    return combined;
}

joypad_buttons_t joypad_get_all_held(void) {
    joypad_buttons_t combined = {0};
    for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
        joypad_buttons_t b = joypad_get_buttons_held((joypad_port_t)i);
        
        combined.a      |= b.a;
        combined.b      |= b.b;
        combined.z      |= b.z;
        combined.start  |= b.start;
        combined.l      |= b.l;
        combined.r      |= b.r;
        combined.d_up   |= b.d_up;
        combined.d_down |= b.d_down;
        combined.d_left |= b.d_left;
        combined.d_right|= b.d_right;
        combined.c_up   |= b.c_up;
        combined.c_down |= b.c_down;
        combined.c_left |= b.c_left;
        combined.c_right|= b.c_right;
    }
    return combined;
}

// Get combined stick input - use first active controller's stick input
joypad_inputs_t joypad_get_all_inputs(void) {
    // Check each port in order, return first active controller found
    for (int i = JOYPAD_PORT_1; i <= JOYPAD_PORT_4; i++) {
        joypad_inputs_t inputs = joypad_get_inputs((joypad_port_t)i);
        
        // Check if this controller has any significant stick input
        if (abs(inputs.stick_x) > 10 || abs(inputs.stick_y) > 10) {
            return inputs; // Return first active controller's inputs
        }
        
        // Also check if any buttons are pressed (in case stick is centered but buttons are used)
        joypad_buttons_t btn = joypad_get_buttons_held((joypad_port_t)i);
        if (btn.a || btn.b || btn.start || btn.d_up || btn.d_down || btn.d_left || btn.d_right) {
            return inputs; // Return first active controller's inputs
        }
    }
    
    // If no active controllers found, return empty inputs
    return (joypad_inputs_t){0};
}
