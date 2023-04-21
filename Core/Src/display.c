// display.c
//  this file will take in the current state of all of the settings as well
//  as the current cursor and display it all on the screen

#include "display.h"
#include "ssd1306.h"
#include "cmsis_os.h"
#include "main_task.h"
#include "string.h"
#include "stdio.h"

#define MAX_STR_LEN 256
#define CHARS_IN_ROW 18
#define FONT_HEIGHT 10
#define FONT_WIDTH

extern bool running;
extern SETTING_t settings[NUM_SETTINGS];
extern uint8_t selected_setting;
extern bool selected;
extern bool pending_change;

static void display_setting(SETTING_t* setting, bool on_setting, bool selected);

void display_task(void)
{
	// init stuff for display
	SSD1306_Init();
	SSD1306_InvertDisplay(false);
	SSD1306_Clear();
	while(1)
	{
		// NOTE: these are designed to only write over the same parts of the screen
		// so we dont need to constantly clear the screen
		// display the different settings
		SSD1306_GotoXY(0, 0);
		display_setting(settings, (selected_setting == 0), selected);

		SSD1306_GotoXY(0, 12);
		display_setting(settings+1, (selected_setting == 1), selected);

		SSD1306_GotoXY(0, 24);
		display_setting(settings+2, (selected_setting == 2), selected);

		SSD1306_GotoXY(0, 36);
		display_setting(settings+3, (selected_setting == 3), selected);

		SSD1306_GotoXY(0, 48);
		display_setting(settings+4, (selected_setting == 4), selected);

		SSD1306_UpdateScreen();
		osDelay(1);
	}
}

static void display_setting(SETTING_t* setting, bool on_setting, bool selected)
{
	char str[MAX_STR_LEN];
	char* temp_char;
	int8_t counter;
	uint8_t num_spaces;
	uint16_t saved_x1;
	uint16_t saved_x2;
	strncpy(str, setting->name, MAX_SETTING_NAME_SIZE);
	SSD1306_Puts(str, &Font_7x10, !(on_setting && !selected));
	SSD1306_Putc(':', &Font_7x10, 1);

	switch(setting->type)
	{
	case CONTINUOUS:
		// put the correct number of spaces to make the value right justified
		num_spaces = CHARS_IN_ROW;
		num_spaces -= strnlen(setting->name, MAX_SETTING_NAME_SIZE);
		num_spaces -= setting->num_digits + 2;
		num_spaces -= (setting->decimal_loc != 0);
		for (int8_t c = 0; c < num_spaces; c++)
		{
			SSD1306_Putc(' ', &Font_7x10, 1);
		}

		// we want to leave room for a negative sign if this is negative so we dont need to clear
		// the entire screen
		if (setting->cont_value < 0)
		{
			sprintf(str, "%0*ld", setting->num_digits + 1, setting->cont_value);
		}
		else
		{
			sprintf(str, " %0*ld", setting->num_digits, setting->cont_value);
		}
		temp_char = str;
		counter = setting->num_digits; // extra because there is a space or negative in front
		while (*temp_char != '\0')
		{
			// if this is the selected one make it a different color
			SSD1306_Putc(*temp_char, &Font_7x10, !(on_setting && selected && counter == setting->current_digit));

			// if the decimal goes here put it here
			if (counter == setting->decimal_loc && counter != 0)
			{
				SSD1306_Putc('.', &Font_7x10, 1);
			}

			temp_char++;
			counter--;
		}
		break;

	case TOGGLE:
		// put the correct number of spaces to make the value right justified
		num_spaces = CHARS_IN_ROW;
	    num_spaces -= strnlen(setting->name, MAX_SETTING_NAME_SIZE);
	    num_spaces -= strnlen(setting->setting1_name, MAX_TOGGLE_SELECTION_SIZE);
	    num_spaces -= strnlen(setting->setting2_name, MAX_TOGGLE_SELECTION_SIZE);
	    num_spaces -= 2;
		for (int8_t c = 0; c < num_spaces; c++)
		{
			SSD1306_Putc(' ', &Font_7x10, 1);
		}

		// output the two setting names, also draw boxes arount the values
		saved_x1 = SSD1306_GetX();
		SSD1306_Puts(setting->setting1_name, &Font_7x10, !(on_setting && selected && !setting->toggle_value));
		saved_x2 = SSD1306_GetX();
		SSD1306_Putc(' ', &Font_7x10, 1);
		SSD1306_DrawRectangle(saved_x1-1, SSD1306_GetY()-1, saved_x2-saved_x1+1, FONT_HEIGHT, !setting->toggle_value);

		saved_x1 = SSD1306_GetX();
		SSD1306_Puts(setting->setting2_name, &Font_7x10, !(on_setting && selected && setting->toggle_value));
		SSD1306_DrawRectangle(saved_x1-1, SSD1306_GetY()-1, SSD1306_GetX()-saved_x1+1, FONT_HEIGHT, setting->toggle_value);
		break;
	}
}

// End of display.c
