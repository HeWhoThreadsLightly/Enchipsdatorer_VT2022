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

extern Color lineBuff [];//double  buffered = horiWhole*2
extern Color screenBuff [];//horiRes*vertRes

extern TIM_HandleTypeDef * vgaPixelTimer;
extern DMA_HandleTypeDef * vgaCircularDMA;
extern DMA_HandleTypeDef * memcopyDMA;

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);



HAL_StatusTypeDef memCopy(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef memSet(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength);

void registerDebugInterupts(UART_HandleTypeDef * huart2);

void clearVisibleAria(Color * lineBuffPart);
void setVerticalSync(Color * lineBuffPart);
void setHorizontalSync(Color * lineBuffPart);

void vgaSetup();
void vgaStart();
void vgaStop();

typedef enum {
	sDecideNext,
	sRenderLine,
	sDoneRenderLine,
	sCopyLastLine,
	sDoneCopylastLine,

	sExitVisible1,
	sDoneExitVisible1,
	sExitVisible2,
	sDoneExitVisible2,

	sSetVsync1P1,
	sSetVsync1P2,
	sDoneSetVsync1,
	sSetVsync2P1,
	sSetVsync2P2,
	sDoneSetVsync2,

	sSetHsync1P1,
	sSetHsync1P2,
	sDoneSetHsync1,
	sSetHsync2P1,
	sSetHsync2P2,
	sDoneSetHsync2,

	sEndBuffer,
} vgaState;

#endif /* INC_VGA_H_ */