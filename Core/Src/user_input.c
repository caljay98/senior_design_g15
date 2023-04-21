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


static volatile unsigned char state;
static int32_t count = 0;
void get_pending_input_events(INPUTS_t* inputs)
{
	inputs->back_click = handle_button(&back);
	inputs->enable_click = handle_button(&enable);
	inputs->mode_click = handle_button(&mode);
	inputs->select_click = handle_button(&select);

	static int32_t lastCount = 0;
	if (lastCount > count)
	{
		// Left spin
		inputs->left_spin = true;
		lastCount--;
	}
	else if (lastCount < count)
	{
		// Right spin
		inputs->right_spin = true;
		lastCount++;
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



// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

#define R_START 0x0

#ifdef HALF_STEP
// Use the half-step state table (emits a code at 00 and 11)
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif


// interrupt for the encoder
// TODO this is not great, sometimes gives both
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Grab state of input pins.
	  unsigned char pinstate = (HAL_GPIO_ReadPin(ENC_1_GPIO_Port, ENC_1_Pin) << 1) | HAL_GPIO_ReadPin(ENC_2_GPIO_Port, ENC_2_Pin);
	  // Determine new state from the pins and state table.
	  state = ttable[state & 0xf][pinstate];
	  // Return emit bits, ie the generated event.
	  unsigned char result = state & 0x30;

	  if (result == DIR_CW)
	  {
		  count++;
	  }
	  else if (result == DIR_CCW)
	  {
		  count--;
	  }
}


// End of user_input.c

