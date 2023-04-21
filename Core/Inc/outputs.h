// outputs.h


#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdint.h>
#include <stdbool.h>
#include <main.h>


typedef enum
{
	STANDARD = 0,    // standard waveform with the pulses matching the period
	SHORT = 1,       // short waveform with the 2us pulse timings on output 1 and 2
} OUTPUT_TYPE_t;

typedef enum
{
	NO_MOD = 0,
	CHANGE_OUT4_DUTY = 1,
	CHANGE_TIME_STEP = 2
} HIGH_SPEED_MODIFICATION_t;

typedef enum
{
	LOW_VOLTAGE_450mV = 0,
	HIGH_VOLTAGE_5000mV = 1
} OUT12_VOLTAGE_t;

typedef enum
{
	OUT_SUCCESS = 0,
	OUT_BAD_ENUM = 1,
	OUT_HAL_CONFIG_ERR = 2,
	OUT_BAD_RANGE = 3,
	OUT_NOT_READY = 4
} OUTPUT_ERROR_t;

#define NUM_NEO_LEDS 4

OUTPUT_ERROR_t enable_output_waveform(uint32_t period_100ns,
		                              OUTPUT_TYPE_t out_type,
									  OUT12_VOLTAGE_t out_voltage,
									  float target_bias_voltage_V,
									  bool single_shot);
void disable_all_outputs();
int8_t set_neopixel(uint8_t led_num, uint8_t red, uint8_t grn, uint8_t blu);
void send_neo_led_sequence(void);

#endif // #ifndef OUTPUTS_H
