// user_input.h


#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdbool.h>
#include <stdint.h>

// struct for all of the different inputs
typedef struct
{
	bool left_spin;
	bool right_spin;
	bool select_click;
	bool back_click;
	bool mode_click;
	bool enable_click;
} INPUTS_t;

void get_pending_input_events(INPUTS_t* inputs);

#endif // USER_INPUT_H
