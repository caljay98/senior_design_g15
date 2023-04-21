// user_input.c
//  file that handles all of the button and encoder input from the user, only
//  outputting if there is a new event in each of the inputs


#include "user_input.h"
#include "main.h"

typedef struct
{
	GPIO_TypeDef* port;
	uint16_t pin;
	bool pressed;
} BUTTON_t;

BUTTON_t back =
{
		.port = BACK_BUT_GPIO_Port,
		.pin = BACK_BUT_Pin,
		.pressed = false,
};
BUTTON_t enable =
{
		.port = ENABLE_BUT_GPIO_Port,
		.pin = ENABLE_BUT_Pin,
		.pressed = false,
};
BUTTON_t mode =
{
		.port = MODE_BUT_GPIO_Port,
		.pin = MODE_BUT_Pin,
		.pressed = false,
};
BUTTON_t select =
{
		.port = SELECT_BUT_GPIO_Port,
		.pin = SELECT_BUT_Pin,
		.pressed = false,
};

#define TIME_BETWEEN_SPINS_ms 20

// encoder timer stuff. Must be configured to input capture direct mode, 100khz
uint32_t enc1_last;
uint32_t enc2_last;
uint32_t last_spin;
bool new_enc1_event = false;
bool new_enc2_event = false;

static bool handle_button(BUTTON_t* button);

void get_pending_input_events(INPUTS_t* inputs)
{
	inputs->back_click = handle_button(&back);
	inputs->enable_click = handle_button(&enable);
	inputs->mode_click = handle_button(&mode);
	inputs->select_click = handle_button(&select);

	if (new_enc1_event && new_enc2_event)
	{
		if (enc1_last > enc2_last)
		{
			inputs->left_spin = true;
		}
		else
		{
			inputs->right_spin = true;
		}
		new_enc1_event = false;
		new_enc2_event = false;
		last_spin = HAL_GetTick();
	}
}

// handle_button
//  will handle the logic for one button, return true only on the pressing edge
static bool handle_button(BUTTON_t* button)
{
	if (!button->pressed && !HAL_GPIO_ReadPin(button->port, button->pin))
	{
		button->pressed = true;
		return true;
	}
	else if (HAL_GPIO_ReadPin(button->port, button->pin))
	{
		button->pressed = false;
	}

	return false;
}

// interrupt for the encoder
// TODO this is not great, sometimes gives both
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (HAL_GetTick() - last_spin <= TIME_BETWEEN_SPINS_ms) return;

	if (GPIO_Pin == ENC_1_Pin && !new_enc1_event)
	{
		enc1_last = HAL_GetTick();
		new_enc1_event = true;
	}
	if (GPIO_Pin == ENC_2_Pin && !new_enc2_event)
	{
		enc2_last = HAL_GetTick();
		new_enc2_event = true;
	}
}


// End of user_input.c

