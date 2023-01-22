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

//enum { vgaUpscale = 4 };
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
extern int lineCount;
extern Color * activeBuffer;
extern Color * oldBuffer;

void registerHUARTvga(UART_HandleTypeDef * huart);

void vgaLoop();
void vgaSetup(
		TIM_HandleTypeDef * vgaPixelTimer_,
		DMA_HandleTypeDef * vgaCircularDMA_,
		DMA_HandleTypeDef * memCopyDMA_,
		GPIO_TypeDef* Vsync_GPIO_Port_,
		uint16_t Vsync_Pin_);
void vgaStart();
void vgaStop();

#endif /* INC_VGA_H_ */
