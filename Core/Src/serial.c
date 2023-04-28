/*
 * serial.c
 *
 *  Created on: Apr 27, 2023
 *      Author: sebas
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "usbd_cdc_if.h"
#include "main_task.h"
#include "user_input.h"
#include "serial.h"
#include "cmsis_os.h"

#define BUFFER_SIZE 		250
#define FRAME_DELIMITER     0x7E
#define ESCAPE              0x7D
#define XOR_VALUE           0x20


extern osMessageQId vComHandle;
extern SETTING_t settings[NUM_SETTINGS];
extern bool pending_gui_change;



uint8_t recvBuffer[BUFFER_SIZE];

uint32_t decodeFrame(uint8_t* decoded, uint8_t* encoded, uint32_t numBytes)
{
    bool isEscaped = false;
    uint32_t decodedIndex = 0;

    for (uint32_t i = 0; i < numBytes; i++)
    {
        uint8_t curByte = encoded[i];

        if (isEscaped)
        {
            decoded[decodedIndex++] = curByte ^ XOR_VALUE;
            isEscaped = false;
        }
        else if (curByte == ESCAPE)
        {
            isEscaped = true;
        }
        else
        {
            decoded[decodedIndex++] = curByte;
        }
    }

    // Return the number of decoded bytes
    return decodedIndex;
}

#define FRAME_DELIMITER 0x7E
#define ESCAPE 0x7D
#define XOR_VALUE 0x20

size_t escape_data(const uint8_t *input_data, size_t input_length, uint8_t *escaped_frame, size_t escaped_frame_size) {
    uint8_t escape_bytes[] = {FRAME_DELIMITER, ESCAPE};
    size_t escape_bytes_count = sizeof(escape_bytes) / sizeof(escape_bytes[0]);

    size_t escaped_index = 0;

    // Add the starting frame delimiter
    if (escaped_index < escaped_frame_size) {
        escaped_frame[escaped_index++] = FRAME_DELIMITER;
    } else {
        return 0; // Not enough space in the output buffer
    }

    for (size_t i = 0; i < input_length; ++i) {
        uint8_t byte = input_data[i];
        int is_escape_byte = 0;

        for (size_t j = 0; j < escape_bytes_count; ++j) {
            if (byte == escape_bytes[j]) {
                is_escape_byte = 1;
                break;
            }
        }

        if (is_escape_byte) {
            if (escaped_index < escaped_frame_size) {
                escaped_frame[escaped_index++] = ESCAPE;
            } else {
                return 0; // Not enough space in the output buffer
            }

            if (escaped_index < escaped_frame_size) {
                escaped_frame[escaped_index++] = byte ^ XOR_VALUE;
            } else {
                return 0; // Not enough space in the output buffer
            }
        } else {
            if (escaped_index < escaped_frame_size) {
                escaped_frame[escaped_index++] = byte;
            } else {
                return 0; // Not enough space in the output buffer
            }
        }
    }

    // Add the ending frame delimiter
    if (escaped_index < escaped_frame_size) {
        escaped_frame[escaped_index++] = FRAME_DELIMITER;
    } else {
        return 0; // Not enough space in the output buffer
    }

    return escaped_index;
}



void parseMessage(uint8_t *encoded, uint32_t encodedBytes)
{
    // Allocate a buffer for the decoded array
    uint8_t msg[BUFFER_SIZE] = { 0 };

    // Decode the array
    uint32_t numBytes = decodeFrame(msg, encoded, encodedBytes);

    if (numBytes == 1 && msg[0] == 0xAA)
    {
    	sendCurrentConfiguration();
    }

    if (numBytes != 9)
    {
        return;
    }

    uint32_t freqCount = msg[3] << 24 | msg[2] << 16 | msg[1] << 8 | msg[0];
    bool onTime = (msg[4] == 0x00);
    bool outVoltage = (msg[5] != 0x00);
    int32_t biasVRaw = (msg[7] << 8 | msg[6]) - 5000;
    bool running = (msg[8] != 0x00);
    settings[0].cont_value = (int32_t)freqCount;
    settings[1].toggle_value = onTime;
    settings[2].toggle_value = outVoltage;
    settings[3].cont_value = biasVRaw;
    settings[4].toggle_value = running;
    INPUTS_t events = {0};
    handle_input_events(&events);
    pending_gui_change = true;
}

void sendCurrentConfiguration()
{
	uint8_t curConfig[200] = {0};
	uint8_t escapedCurConfig[200] = {0};
	// Cast to unsigned to deal with sign extension
	uint32_t freqCount = (uint32_t)settings[0].cont_value;

	curConfig[0] = freqCount & 0xFF;
	curConfig[1] = (freqCount >> 8) & 0xFF;
	curConfig[2] = (freqCount >> 16) & 0xFF;
	curConfig[3] = (freqCount >> 24) & 0xFF;
	curConfig[4] = (settings[1].toggle_value == 0x00);
	curConfig[5] = (settings[2].toggle_value == 0x00);
	uint16_t biasVRaw = settings[3].cont_value + 5000;
	curConfig[6] = biasVRaw & 0xFF;
	curConfig[7] = (biasVRaw >> 8) & 0xFF;
	curConfig[8] = (settings[4].toggle_value == 0x00);

	uint32_t numBytes = escape_data(curConfig, 9, escapedCurConfig, 200);

	CDC_Transmit_FS(escapedCurConfig, numBytes);
}

void runSerial()
{
	static uint32_t recvIdx = 0;
	while (xQueueReceive(vComHandle, &recvBuffer[recvIdx], 0) == pdPASS && recvIdx < BUFFER_SIZE)
	{
		// If just received FRAME_DELIMITER indicates beginning of new message
		if (recvBuffer[recvIdx] == FRAME_DELIMITER)
		{
			if (recvIdx == 0)
			{
				// Since recvIdx == 0 no data has been received. This means that the FRAME_DELIMITER that was
				// received was due to the start delimiter
				continue;
			}
			else
			{
				// Data received. FRAME_DELIMITER that was received was due to end delimiter. Parse message
				parseMessage(recvBuffer, recvIdx);
				// Reset recvIdx to allow for reading of new message
				recvIdx = 0;
			}
		}
		else
		{
			// Still receiving new data. Increment recvIdx
			recvIdx++;
		}
	}
}
