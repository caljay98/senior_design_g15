// outputs.c
//  This file controls all essential functions for setting the special
//  outputs on the device. Timers should be correctly setup in the IOC
//  file, but this file will mess with the period, duty, and waveforms
//  of the outputs

#include "outputs.h"

// Note: This library assumes the frequency per tick of htim5 and htim2 are both 10MHz
#define TIMER_FREQ_Hz 10000000ul
#define TICK_PERIOD_ns 100
#define CHANGE_OUT4_DUTY_CUTOFF_100ns 500 // 20kHz
#define CHANGE_TIME_STEP_100ns 350 //28.571kHz

// The default time chunks for offsetting the different outputs
#define MAX_TIME_STEP_100ns 50 // 5us
#define SHORT_TIME_PERIOD_100ns 20 // 2us
#define OUT4_LOW_TIME_steps 3

// output 4 has a target fall time of 1us
#define OUT4_FALL_TIME_100ns 10

// tim8 is the master that controls both tim2 and tim5 so they are syncronized
extern TIM_HandleTypeDef htim8;

// OUTPUT 1-3
// tim5 channels 1, 2, and 3 are the first 3 outputs. Channel 1 must be in PWM mode
// and channels 2 and 3 Must be configured in OC Toggle on match with DMA
extern TIM_HandleTypeDef htim5;

// OUTPUT 4
// tim2 channel 3 is neg enable, channel 4 is pos enable. Must be configured in OC
// Toggle on match with DMA
extern TIM_HandleTypeDef htim2;
// DAC controls the voltage level of output4
extern DAC_HandleTypeDef hdac;
// WARNING: When running htim2, Never have channel 3 and channel 4 on at the same time.
// This will cause MOSFET shootthrough

// waveforms for the three channels. These should be calculated mathematically based on
// the desired period, and whether we want to be in short or normal mode. Fall is the
// first number, rise is the second number
#define BUFFER_REPEATS 10
uint32_t chan_2_out[2*BUFFER_REPEATS] = {0};
uint32_t chan_3_out[2*BUFFER_REPEATS] = {0};
uint32_t chan_4_out[2*BUFFER_REPEATS] = {0};

// neopixel LED outputs
// must be configured to be 20MHz for 0.05us resolution, with output 1 defined
// as a PWM with DMA in normal mode. Period length must be 1.5us
#define NEO_TIMER_FREQ_HZ 20000000ul // 20MHz
#define NEO_TICK_PERIOD_ns 50 // 0.05us
#define TIMER_RELOAD_PERIOD_50ns 30 // 1.5us
#define HIGH_BIT_FALL_TIME_50ns 15 // 0.75us
#define LOW_BIT_FALL_TIME_50ns 6 // 0.30us
extern TIM_HandleTypeDef htim1;

// waveform for the 4 neopixel leds
#define NEO_BUFFER_SIZE_PER_LED 24
#define TOTAL_NEO_BUFFER_SIZE NUM_NEO_LEDS*NEO_BUFFER_SIZE_PER_LED
uint16_t neo_out[TOTAL_NEO_BUFFER_SIZE] = {0};
volatile bool send_done = true;

// static functions
static OUTPUT_ERROR_t set_out4_voltage(float voltage);
static void set_all_buffer(uint32_t* array, uint32_t value1, uint32_t value2);


// enable_output_waveform
//  This will configure the 4 outputs to the correct values in order to output
//  the parameters that are passed in
OUTPUT_ERROR_t enable_output_waveform(uint32_t period_100ns,
		                              OUTPUT_TYPE_t out_type,
									  OUT12_VOLTAGE_t out_voltage,
									  float target_bias_voltage_V,
									  bool single_shot)
{
	TIM_OC_InitTypeDef sConfigOC = {0};
	uint32_t out1_fall_time = 0;
	uint32_t out2_rise_time = 0;
	uint32_t out2_fall_time = 0;
	uint32_t out3_rise_time = 0;
	uint32_t out3_fall_time = 0;
	uint32_t out4_rise_time = 0;
	uint32_t out4_fall_time = 0;
	OUTPUT_ERROR_t err = OUT_SUCCESS;
	uint32_t time_step_100ns;
	HIGH_SPEED_MODIFICATION_t speed_mod = NO_MOD;

	// wait for the current cycle to be finished before reconfiguring
	// TODO

	// disable all outputs so there is no strange behavior when switching values
	disable_all_outputs();

	// set the correct voltage on output4
	err = set_out4_voltage(target_bias_voltage_V);
	if (err != OUT_SUCCESS) return err;

	// configure out1 and out2 to the correct voltage by setting the relay
	if (out_voltage == LOW_VOLTAGE_450mV)
	{
		HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, SET);
		HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin, SET);
	}
	else
	{
		HAL_GPIO_WritePin(RELAY_1_GPIO_Port, RELAY_1_Pin, RESET);
		HAL_GPIO_WritePin(RELAY_2_GPIO_Port, RELAY_2_Pin, RESET);
	}

	// set the correct period for tim2 and tim5
	htim2.Init.Period = period_100ns - 1;
	htim5.Init.Period = period_100ns - 1;
	htim8.Init.Period = period_100ns - 1;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK ||
		HAL_TIM_Base_Init(&htim5) != HAL_OK ||
		HAL_TIM_Base_Init(&htim8) != HAL_OK) return OUT_HAL_CONFIG_ERR;

	// find if there needs to be a modification because the frequency is too high
	if (period_100ns < CHANGE_OUT4_DUTY_CUTOFF_100ns) speed_mod = CHANGE_OUT4_DUTY;
	if (period_100ns < CHANGE_TIME_STEP_100ns) speed_mod = CHANGE_TIME_STEP;
	switch(speed_mod)
	{
	case NO_MOD:
		time_step_100ns = MAX_TIME_STEP_100ns;
		out4_fall_time = ((period_100ns % 2) ? ((period_100ns / 2) - 1) : (period_100ns / 2));
		break;

	case CHANGE_OUT4_DUTY:
		time_step_100ns = MAX_TIME_STEP_100ns;
		out4_fall_time = period_100ns - (OUT4_LOW_TIME_steps * time_step_100ns);
		break;

	case CHANGE_TIME_STEP:
		time_step_100ns = (uint32_t)(MAX_TIME_STEP_100ns * ((float)period_100ns / CHANGE_TIME_STEP_100ns));
		out4_fall_time = period_100ns - (OUT4_LOW_TIME_steps * time_step_100ns);
		break;

	default: return OUT_BAD_ENUM;
	}

	// shift everything to be based off of the output 1 timer, meaning the start
	// of output 4 is two time steps after the start
	out4_rise_time = (2*time_step_100ns);
	out4_fall_time += (2*time_step_100ns);
	out2_rise_time = (1*time_step_100ns);
	out3_rise_time = out4_rise_time;
	out3_fall_time = out4_fall_time - (2*time_step_100ns);
	switch (out_type)
	{
	case STANDARD:
		out1_fall_time = out4_fall_time - (3*time_step_100ns);
		out2_fall_time = out4_fall_time - (2*time_step_100ns);
		break;

	case SHORT:
		out1_fall_time = SHORT_TIME_PERIOD_100ns;
		out2_fall_time = out2_rise_time + SHORT_TIME_PERIOD_100ns;
		break;

	default: return OUT_BAD_ENUM;
	}

	// take the rise and fall times and turn them into the actual DMA and PWM
	// prescaler values
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = out1_fall_time;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) return OUT_HAL_CONFIG_ERR;
    set_all_buffer(chan_2_out, out2_rise_time, out2_fall_time);
    set_all_buffer(chan_3_out, out3_rise_time, out3_fall_time);
    set_all_buffer(chan_4_out, out4_rise_time, out4_fall_time);

	// TODO some code to make single-shot work

    // TODO the first of the series of shots does not work great

    // ensure the channels start in the correct state (low/high)
    // TODO

	// prime the first three outputs to start. These will not start until tim8 is started
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
	HAL_TIM_OC_Start_DMA(&htim5, TIM_CHANNEL_2, chan_2_out, 2*BUFFER_REPEATS);
	HAL_TIM_OC_Start_DMA(&htim5, TIM_CHANNEL_3, chan_3_out, 2*BUFFER_REPEATS);

	if (target_bias_voltage_V <= 0)
	{
		HAL_TIM_OC_Start_DMA(&htim2, TIM_CHANNEL_3, chan_4_out, 2*BUFFER_REPEATS);
	}
	else
	{
		HAL_TIM_OC_Start_DMA(&htim2, TIM_CHANNEL_4, chan_4_out, 2*BUFFER_REPEATS);
	}


	HAL_TIM_Base_Start(&htim8);

	return OUT_SUCCESS;
}

// disable_all_outputs
//  Turns off and resets all outputs
void disable_all_outputs(void)
{
	HAL_TIM_Base_Stop(&htim8);
	HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_1);
	HAL_TIM_OC_Stop_DMA(&htim5, TIM_CHANNEL_2);
	HAL_TIM_OC_Stop_DMA(&htim5, TIM_CHANNEL_3);
	HAL_TIM_OC_Stop_DMA(&htim2, TIM_CHANNEL_3);
	HAL_TIM_OC_Stop_DMA(&htim2, TIM_CHANNEL_4);
	HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
	__HAL_TIM_SET_COUNTER(&htim5, 0);
	__HAL_TIM_SET_COUNTER(&htim8, 0);
}

int8_t set_neopixel(uint8_t led_num, uint8_t red, uint8_t grn, uint8_t blu)
{
	// make sure the last send is done before modifying
	if (!send_done) return OUT_NOT_READY;
	if (led_num >= NUM_NEO_LEDS) return OUT_BAD_RANGE;

	// these LEDs are in the opposite order of the outputs
	led_num = NUM_NEO_LEDS - led_num - 1;

	// pack the color into one bit sequence
	uint32_t bits = (uint32_t)grn << 16 | (uint32_t)red << 8 | blu;

	uint16_t* this_bit = neo_out + (led_num*NEO_BUFFER_SIZE_PER_LED);
	for (int8_t c = NEO_BUFFER_SIZE_PER_LED-1; c >= 0; c--)
	{
		*this_bit = (bits & (0b1 << c)) ? HIGH_BIT_FALL_TIME_50ns: LOW_BIT_FALL_TIME_50ns;
		this_bit++;
	}

	return OUT_SUCCESS;
}

// TODO sometimes this does not work and incorrect values
void send_neo_led_sequence(void)
{
	TIM_OC_InitTypeDef sConfigOC = {0};
	// make sure the sending is done
	if (!send_done) return;

	// make sure the first value in the preload register is 0 to ensure there is
	// no effect from the first cycle before the DMA kicks in
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)neo_out, TOTAL_NEO_BUFFER_SIZE);
	send_done = false;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim1)
	{
		HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
		send_done = true;
	}
}

// set_out4_voltage
//  sets the voltage for output 4 using the DA
static OUTPUT_ERROR_t set_out4_voltage(float voltage)
{
	// TODO need to do the math on setting the actual voltage on the output
	// of the circuit to match the desired voltage
	if (voltage < 0) voltage *= -1;
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, voltage*4096/3.3);
	HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
	return OUT_SUCCESS;
}

// set_all_buffer
//  fills the inputed buffer with the two values repeated
static void set_all_buffer(uint32_t* array, uint32_t value1, uint32_t value2)
{
	for (uint32_t c = 0; c < BUFFER_REPEATS; c++)
	{
		array[2*c] = value1;
		array[2*c+1] = value2;
	}
}

// End of outputs.c
