// main_task.h

#ifndef MAIN_TASK_H
#define MAIN_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "outputs.h"

#define MAX_SETTING_NAME_SIZE 128
#define MAX_TOGGLE_SELECTION_SIZE 8
#define NUM_SETTINGS 5

#define CONVERT_TO_FLOAT(value, dec) (((float)value)/pow(10, (dec)))

// stuff to define all of the different settings
typedef enum
{
	CONTINUOUS = 0,
	TOGGLE = 1
} SETTING_TYPE_t;

typedef struct
{
	char name[MAX_SETTING_NAME_SIZE]; // should include trailing spaces to make the output look good
	SETTING_TYPE_t type;

	// only used if this is a continuous setting. NOTE: fixed point
	int32_t cont_value; // fixed point base10 with decimal_loc
	int32_t min;        // fixed point base10 with decimal_loc
	int32_t max;        // fixed point base10 with decimal_loc
	uint8_t decimal_loc;
	uint8_t num_digits;
	uint8_t current_digit;

	// only used if this is a toggle setting
	uint8_t toggle_value;
	char setting1_name[MAX_TOGGLE_SELECTION_SIZE];
	char setting2_name[MAX_TOGGLE_SELECTION_SIZE];

} SETTING_t;


// function prototypes
void main_task(void);


#endif // MAIN_TASK_H
