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

//enum { vgaUpscale = 8 };//note vga up scale looses last 2 pixels in 32 bit copy
enum { horiRes = 640/vgaUpscale};
enum { vertRes = 400/vgaUpscale};

//main buffers
extern Color lineBuff [];//double  buffered = horiWhole*2
extern Color screenBuff [];//horiRes*vertRes

//hardware interfaces
extern TIM_HandleTypeDef * vgaPixelTimer;
extern DMA_HandleTypeDef * vgaCircularDMA;
extern DMA_HandleTypeDef * memCopyDMA;

//state machine state defined in vgaSetup
extern int lineCount;//start right after a vertical sync
extern int lineUpscale;//copy old buffer if non zero
extern int readyForNextLine;
extern Color * activeBuffer;
extern Color * oldBuffer;

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);



HAL_StatusTypeDef old_memCopy(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef old_memSet(uint32_t value, uint32_t * DstAddress, uint32_t DataLength);

void registerDebugInterupts(DMA_HandleTypeDef * toDebug);
void registerHUART(UART_HandleTypeDef * huart);

void clearVisibleAria(Color * lineBuffPart);
void setVerticalSync(Color * lineBuffPart);
void setHorizontalSync(Color * lineBuffPart);

void vgaLoop();
void vgaSetup(
		TIM_HandleTypeDef * vgaPixelTimer_,
		DMA_HandleTypeDef * vgaCircularDMA_,
		DMA_HandleTypeDef * memCopyDMA_,
		GPIO_TypeDef* Vsync_GPIO_Port_,
		uint16_t Vsync_Pin_);
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


extern vgaState state;
#endif /* INC_VGA_H_ */
