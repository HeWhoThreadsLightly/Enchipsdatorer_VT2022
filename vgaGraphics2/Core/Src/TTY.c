#include "TTY.h"
#include "vga.h"
#include "graphicsLib.h"
#include "codepage-437-bmp.h"
#include <stdio.h>
#include "main.h"

const char cornerTopLeft = 201;
const char cornerTopRight = 187;
const char cornerBottomLeft = 200;
const char cornerBottomRight = 188;
const char cornerVertical = 186;
const char cornerHorizontal = 205;

void clearScreen(){
	for(int i = 0; i < vertRes; i++){
		for(int j = 0; j < horiRes; j++){
			screenBuff[i*horiRes + j] = ColorWhite;
		}
	}
}

void makeBorders(){
	int colums = horiRes / codepage_437.sprite_hori - 1;
	int rows = vertRes / codepage_437.sprite_vert - 1;

	renderCharOnGrid(cornerTopLeft, 0, 0, ColorTransparent, ColorRainbow, &codepage_437);
	renderCharOnGrid(cornerTopRight, colums, 0, ColorTransparent, ColorRainbow, &codepage_437);
	renderCharOnGrid(cornerBottomLeft, 0, rows, ColorTransparent, ColorRainbow, &codepage_437);
	renderCharOnGrid(cornerBottomRight, colums, rows, ColorTransparent, ColorRainbow, &codepage_437);
	for(int i = 1; i < colums; i++){
		renderCharOnGrid(cornerHorizontal, i, 0, ColorTransparent, ColorRainbow, &codepage_437);
		renderCharOnGrid(cornerHorizontal, i, rows, ColorTransparent, ColorRainbow, &codepage_437);
	}
	for(int i = 1; i < rows; i++){
		renderCharOnGrid(cornerVertical, 0, i, ColorTransparent, ColorRainbow, &codepage_437);
		renderCharOnGrid(cornerVertical, colums, i, ColorTransparent, ColorRainbow, &codepage_437);
	}
}

void runTTY(UART_HandleTypeDef * huart){
	int init = 0;

	char str[81] = { '\0' };
	uint16_t str_len = 0;

	Color background = ColorWhite;
	Color forground = ColorBlack;

	const int colums = horiRes / codepage_437.sprite_hori - 1;
	const int rows = vertRes / codepage_437.sprite_vert - 1;

	int h = 1, v = 1;

	char toPrint;

	char rainbowModeTrigger[] = "rainbow";
	char *rainbowActivationTracker = rainbowModeTrigger;

	while(1){
		HAL_StatusTypeDef resStatus = HAL_UART_Receive(huart, (uint8_t*)&toPrint, 1, HAL_MAX_DELAY);
		uint16_t charCode = toPrint;
		str_len = sprintf(str, "\r\nRecived byte %c code %u HAL %u\r\n", toPrint, charCode, (uint16_t)resStatus);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

		if(init == 0){//clear the screen and draw the window border
			init = 1;
			clearScreen();
			makeBorders();
		}


		if(toPrint == 13){//newline
			v++;
			h = 1;
		}else if(toPrint == 127){//backspace
			h--;
			if(h == 0){//beginning of line
				h = colums - 1;
				v--;
			}
			if(v == 0){//beginning of screen
				v = rows - 1;
			}
			renderCharOnGrid(' ', h, v, background, forground, &codepage_437);
		}else{//print char
			renderCharOnGrid(toPrint, h, v, background, forground, &codepage_437);
			h++;
		}
		if(h >= colums){//line wrap
			v++;
			h = 1;
		}
		if(v >= rows){//screen wrap
			v = 1;
		}

		if(toPrint == *rainbowActivationTracker){//replace all black text with rainbow text
			rainbowActivationTracker++;
			str_len = sprintf(str, "\r\nNext activation char %c step %i\r\n", *rainbowActivationTracker, (rainbowActivationTracker - rainbowModeTrigger));
			HAL_UART_Transmit(huart, (uint8_t*) str, str_len, HAL_MAX_DELAY);
			if(*rainbowActivationTracker == '\0'){
				rainbowActivationTracker = rainbowModeTrigger;
				if(forground.value == ColorRainbow.value){
					forground = ColorBlack;
				}else{
					forground = ColorRainbow;
				}
			}
		}else{
			rainbowActivationTracker = rainbowModeTrigger;
		}
	}
}
