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

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);

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

enum { vgaUpscale = 2 };
enum { horiRes = 640/vgaUpscale};
enum { horiFront = 16/vgaUpscale};
enum { horiSync = 96/vgaUpscale};
enum { horiBack = 48/vgaUpscale};
enum { horiWhole = 800/vgaUpscale};

enum { vertRes = 400/vgaUpscale};
enum { vertArea = 400 };
enum { vertFront = 12};
enum { vertSync = 2};
enum { vertBack = 35};
enum { vertWhole = 449};

typedef struct {
	TIM_HandleTypeDef * vgaPixelTimer;
	DMA_HandleTypeDef * vgaCircularDMA;
	DMA_HandleTypeDef * memCopyDMA;
	vgaState state;
	int lineCount;
	int lineUpscale;
	Color * activeBuffer;
	Color * oldBuffer;
	_Alignas(uint32_t) Color lineBuff [horiWhole*2];//double  buffered
	_Alignas(uint32_t) Color screenBuff[horiRes*vertRes];//rows*columns
} vgaData;



HAL_StatusTypeDef memCopy(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef memSet(uint32_t value, uint32_t * DstAddress, uint32_t DataLength);

void registerDebugInterupts(DMA_HandleTypeDef * memCopyDMA, UART_HandleTypeDef * huart2);

void __weak renderLine(Color * screenBuff, Color * lineBuffPart, const int lineCount);
void clearVisibleAria(Color * lineBuffPart);
void setVerticalSync(Color * lineBuffPart);
void setHorizontalSync(Color * lineBuffPart);

vgaData * vgaSetup(
		TIM_HandleTypeDef * vgaPixelTimer,
		DMA_HandleTypeDef * vgaCircularDMA,
		DMA_HandleTypeDef * memCopyDMA);
void vgaStart(vgaData * vga);
void vgaStop(vgaData * vga);


#endif /* INC_VGA_H_ */
