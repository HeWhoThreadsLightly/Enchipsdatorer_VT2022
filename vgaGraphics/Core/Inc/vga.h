/*
 * vga.h
 *
 *  Created on: 4 apr. 2022
 *      Author: Orion
 */

#ifndef INC_VGA_H_
#define INC_VGA_H_

#include "main.h"

typedef struct {
	char value;
}Color;

enum { vgaUpscale = 2 };
enum { horiRes = 640/vgaUpscale};
enum { vertRes = 400/vgaUpscale};

extern Color lineBuff [];//double  buffered
extern Color screenBuff [];
extern Color backgroundBuff [];//rows*columns
extern Color forgroundBuff [];//rows*columns
extern Color charBuff [];//rows*columns
extern int vgaCharMode;

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);


extern TIM_HandleTypeDef vgaPixelTimer;
extern DMA_HandleTypeDef vgaCircularDMA;
extern DMA_HandleTypeDef memcopyDMA;

HAL_StatusTypeDef memCopy();
HAL_StatusTypeDef memSet();
void registerDebugInterupts(UART_HandleTypeDef * huart2);

void clearVisibleAria(Color * lineBuffPart);
void setVerticalSync(Color * lineBuffPart);
void setHorizontalSync(Color * lineBuffPart);

void vgaSetup();
void vgaStart();
void vgaStop();


#endif /* INC_VGA_H_ */
