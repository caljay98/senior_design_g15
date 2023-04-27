// main_task.c
//  this task will handle getting input from the user, updating the display, and
//  outputting waveforms to the pins

#include "main_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "user_input.h"
#include "display.h"
#include <math.h>
#include <string.h>

static bool handle_input_events(INPUTS_t* events);

#define MAX_LED_BRIGHTNESS 50
#define PACK(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (b))
#define GET_R(c) (((c) >> 16) & 0xFF)
#define GET_G(c) (((c) >> 8) & 0xFF)
#define GET_B(c) ((c) & 0xFF)

// which setting the user is hovering on, and if it is selected or not
uint8_t selected_setting = 0;
bool selected = false;
bool pending_change = false;

// pending input events that need to be serviced
INPUTS_t pending_input_events = {0};

// array of all of the different settings
#define FREQ_SETTING 0
#define OUT_MODE_SETTING 1
#define OUT_VOLTAGE_SETTING 2
#define BIAS_VOLTAGE_SETTING 3
#define RUN_TYPE_SETTING 4
SETTING_t settings[NUM_SETTINGS] =
{
		{
				.name = "Freq (Hz)",
				.type = CONTINUOUS,
				.cont_value = 1000,
				.min = 1,
				.max = 125000,
				.num_digits = 6,
				.decimal_loc = 0,
				.current_digit = 0
		},
		{
				.name = "On Time",
				.type = TOGGLE,
				.toggle_value = 0,
				.setting1_name = "LnG",
				.setting2_name = "Shrt"
		},
		{
				.name = "Out V",
				.type = TOGGLE,
				.toggle_value = 0,
				.setting1_name = "0.45V",
				.setting2_name = "5.0V"
		},
		{
				.name = "Bias V",
				.type = CONTINUOUS,
				.cont_value = 0,
				.min = -500,
				.max = 500,
				.num_digits = 3,
				.decimal_loc = 2,
				.current_digit = 0
		},
		{
				.name = "Running",
				.type = TOGGLE,
				.toggle_value = 0,
				.setting1_name = "OFF",
				.setting2_name = "ON"
		}
};

// NOTE: running will never be true in single shot mode
bool running = false;
uint32_t curr_period_100ns = 0;
OUTPUT_TYPE_t curr_out_type = STANDARD;
OUT12_VOLTAGE_t curr_out_voltage = LOW_VOLTAGE_450mV;
float curr_bias_voltage = 0;
bool curr_single = false;

void main_task(void)
{
	// set the neopixel leds to off
	set_neopixel(0, 0, 0, 0);
	set_neopixel(1, 0, 0, 0);
	set_neopixel(2, 0, 0, 0);
	set_neopixel(3, 0, 0, 0);
	send_neo_led_sequence();

	while(1)
	{
		// read in new data from the user. Use that to modify settings and
		// selections on the screen
		get_pending_input_events(&pending_input_events);

		// get all the current values from the settings, checking if there
		// has been a change
		pending_change = handle_input_events(&pending_input_events);

		// set all the inputs to zero as all the pending events have been serviced
		memset(&pending_input_events, 0, sizeof(pending_input_events));

		// if there has been a change in desired configuration, update the
		// outputs to match that. Also change the LEDs
		if (pending_change)
		{
			if (running)
			{
				enable_output_waveform(curr_period_100ns, curr_out_type,
						               curr_out_voltage, curr_bias_voltage,
							           curr_single);
				// set the neopixel LEDs to the correct colors based on the outputs
				if (curr_out_voltage == LOW_VOLTAGE_450mV)
				{
					set_neopixel(0, 0, MAX_LED_BRIGHTNESS, 0);
					set_neopixel(1, 0, MAX_LED_BRIGHTNESS, 0);
				}
				else
				{
					set_neopixel(0, MAX_LED_BRIGHTNESS, 0, 0);
					set_neopixel(1, MAX_LED_BRIGHTNESS, 0, 0);
				}

				// set the last output based on the bias voltage
				if (curr_bias_voltage > 0)
				{
					// orange for positive
					uint8_t set_value = MAX_LED_BRIGHTNESS*(curr_bias_voltage / 5.0);
					set_neopixel(3, set_value, set_value, 0);
				}
				else
				{
					// purple for negative
					uint8_t set_value = MAX_LED_BRIGHTNESS*(-1*curr_bias_voltage / 5.0);
					set_neopixel(3, set_value, 0, set_value);
				}

				// for single shot, show that the output is no longer on once
				// the current cycle is complete
				// TODO

				set_neopixel(2, MAX_LED_BRIGHTNESS, 0, 0);
				send_neo_led_sequence();
			}
			else
			{
				// no longer enabled, turn off all the LEDs
				disable_all_outputs();
				set_neopixel(0, 0, 0, 0);
				set_neopixel(1, 0, 0, 0);
				set_neopixel(2, 0, 0, 0);
				set_neopixel(3, 0, 0, 0);
				send_neo_led_sequence();
			}
		}

		osDelay(10);
	}
}


// handle_input_events
//  returns true if there is a change to the config that needs to be updated
static bool handle_input_events(INPUTS_t* events)
{
	bool retval = false;
	SETTING_t* curr_setting = settings + selected_setting;
	if (selected)
	{
		// a setting is selected. Changing the dial changes the setting and
		// the navigation buttons exit
		if (events->back_click) selected = false;
		switch (curr_setting->type)
		{
		case CONTINUOUS:
			// clicking the dial will change the digit we are changing
			if (events->select_click)
			{
				curr_setting->current_digit = (curr_setting->current_digit + 1) % curr_setting->num_digits;
			}

			// left and right will increase and decrease that digit
			int32_t change_value = 1;
			for (uint32_t c = 0; c < curr_setting->current_digit; c++) change_value *= 10;
			if (events->left_spin)
			{
				curr_setting->cont_value -= change_value;
				if (curr_setting->cont_value < curr_setting->min) curr_setting->cont_value = curr_setting->min;
				retval = true;
			}
			if (events->right_spin)
			{
				curr_setting->cont_value += change_value;
				if (curr_setting->cont_value > curr_setting->max) curr_setting->cont_value = curr_setting->max;
				retval = true;
			}
			break;

		case TOGGLE:
		default:
			// left, right, or click will all change the setting
			if (events->left_spin || events->right_spin || events->select_click)
			{
				curr_setting->toggle_value = !curr_setting->toggle_value;
				retval = true;
			}
			break;
		}
	}
	else
	{
		// navigating through the menu
		if (events->left_spin)
		{
			if (selected_setting > 0) selected_setting--;
		}
		if (events->right_spin)
		{
			if (selected_setting < NUM_SETTINGS-1) selected_setting++;
		}
		if (events->select_click) selected = true;
	}

	// mode click button will change the output mode
	if (events->mode_click)
	{
		settings[OUT_VOLTAGE_SETTING].toggle_value = !settings[OUT_VOLTAGE_SETTING].toggle_value;
		retval = true;
	}

	if (events->enable_click)
	{
		settings[RUN_TYPE_SETTING].toggle_value = !settings[RUN_TYPE_SETTING].toggle_value;;
		retval = true;
	}

	// convert all of the settings to actual run parameters
	curr_period_100ns = 1e7 / CONVERT_TO_FLOAT(settings[FREQ_SETTING].cont_value, settings[FREQ_SETTING].decimal_loc);
	curr_bias_voltage = CONVERT_TO_FLOAT(settings[BIAS_VOLTAGE_SETTING].cont_value, settings[BIAS_VOLTAGE_SETTING].decimal_loc);;
	curr_out_type = settings[OUT_MODE_SETTING].toggle_value;
	curr_out_voltage = settings[OUT_VOLTAGE_SETTING].toggle_value;
	running = settings[RUN_TYPE_SETTING].toggle_value;
	//curr_single = settings[RUN_TYPE_SETTING].toggle_value;

	return retval;
}


// End of main_task.c
