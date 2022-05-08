/*
 * vga.c
 *
 *  Created on: 4 apr. 2022
 *      Author: Orion
 */


#include "vga.h"
#include "main.h"
#include <stdio.h>
//#include "codepage-437-bmp.h"

/*
http://tinyvga.com/vga-timing/640x400@70Hz

General timing
Screen refresh rate	70 Hz
Vertical refresh	31.46875 kHz/4/1.2
Pixel freq.	25.175 MHz

Horizontal timing (line)
Polarity of horizontal sync pulse is negative.
Scanline part	Pixels	/4	Time [µs]
Visible area	640		160	25.422045680238
Front porch		16		4	0.63555114200596
Sync pulse		96		24	3.8133068520357
Back porch		48		12	1.9066534260179
Whole line		800		200	31.777557100298

Vertical timing (frame)
Polarity of vertical sync pulse is positive.
Frame part		Lines	/4	Time [ms]
Visible area	400		100	12.711022840119
Front porch		12			0.38133068520357
Sync pulse		2			0.063555114200596
Back porch		35			1.1122144985104
Whole frame		449			14.268123138034
 *
 */

//VGA resolution constants
//enum { horiRes = 640/vgaUpscale};//defined in header
enum { horiFront = 16/vgaUpscale};
enum { horiSync = 96/vgaUpscale};
enum { horiBack = 48/vgaUpscale};
enum { horiWhole = 800/vgaUpscale};

//enum { vertRes = 400/vgaUpscale};//defined in header
enum { vertArea = 400 };
enum { vertFront = 12};
enum { vertSync = 2};
enum { vertBack = 35};
enum { vertWhole = 449};

//main buffers
_Alignas(uint32_t) Color lineBuff[horiWhole*2] = {0};//double  buffered
_Alignas(uint32_t) Color screenBuff[horiRes*vertRes] = {0};//rows*columns
//todo remove and replace with a setCharAtXY function

//hardware interfaces
TIM_HandleTypeDef * vgaPixelTimer;
DMA_HandleTypeDef * vgaCircularDMA;
DMA_HandleTypeDef * memCopyDMA;

//state machine state defined in vgaSetup
int lineCount;//start right after a vertical sync
int lineUpscale;//copy old buffer if non zero
int readyForNextLine;
vgaState state;
Color * activeBuffer;
Color * oldBuffer;

void checkAsserts(){//check memory layout assumptions
	//32 bit accesses is used to sped up dma transfers alignment of start and end is needed to avoid manual copying of leading and trailing bytes
	_Static_assert((int)&lineBuff % sizeof(uint32_t) == 0, "Line buff is not aligned for uint32 accesses");
	_Static_assert((int)&screenBuff % sizeof(uint32_t) == 0, "Screen buff is not aligned for uint32 accesses");

	//checks that 32 bit mode works for all subsets of the line buffer
	_Static_assert(horiRes % sizeof(uint32_t) == 0, "Horizontal area is not 32 bit aligned");
	_Static_assert(horiFront % sizeof(uint32_t) == 0, "Horizontal front is not 32 bit aligned");
	_Static_assert(horiSync % sizeof(uint32_t) == 0, "Horizontal sync is not 32 bit aligned");
	_Static_assert(horiBack % sizeof(uint32_t) == 0, "Horizontal back is not 32 bit aligned");
	_Static_assert(horiWhole % sizeof(uint32_t) == 0, "Horizontal whole line is not 32 bit aligned");

	_Static_assert(horiWhole == horiRes+horiFront+horiSync+horiBack, "Horizontal vga configuration does not sum up");
	_Static_assert(vertWhole == vertArea+vertFront+vertSync+vertBack, "Vertical vga configuration does not sum up");
}

void setRed(Color * c, char r){
	c->value = (c->value & 0b11001111) | r << 4;
}
void setGreen(Color * c, char g){
	c->value = (c->value & 0b11110011) | g;
}
void setBlue(Color * c, char b){
	c->value = (c->value & 0b11111100) | b;
}
void setRGB(Color * c, char r, char g, char b){
	c->value = r << 4 | g << 2 | b;
}
void setHblank(Color * c){
	c->value = 0b0100000;
}
void setVblank(Color * c){
	c->value = 0b1000000;
}

/**
 * @brief copies 4byte aligned data
 *
 * @param SrcAddress copies from source address incrementing
 *
 * @param DstAddress copies to destination address incrementing
 *
 * @param DataLength number of unit32_t to be copied
 */
HAL_StatusTypeDef old_memCopy(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength){

	memCopyDMA->Init.PeriphInc = DMA_PINC_ENABLE;
	if (HAL_DMA_Init(memCopyDMA) != HAL_OK) {
		Error_Handler();
	}
	//SET_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(memCopyDMA, (uint32_t)SrcAddress, (uint32_t)DstAddress, DataLength);
}

/**
 * @brief sets 4byte aligned data
 *
 * @param value value to be written
 *
 * @param DstAddress destination address incrementing
 *
 * @param DataLength number of unit32_t to be written
 */
HAL_StatusTypeDef old_memSet(uint32_t value, uint32_t * DstAddress, uint32_t DataLength){
	static volatile uint32_t setVal = 0;
	setVal = value;
	memCopyDMA->Init.PeriphInc = DMA_PINC_DISABLE;
	if (HAL_DMA_Init(memCopyDMA) != HAL_OK) {
		Error_Handler();
	}
	//CLEAR_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(memCopyDMA, (uint32_t)&setVal, (uint32_t)DstAddress, DataLength);
}

UART_HandleTypeDef * huartE;
//HAL_DMA_XFER_CPLT_CB_ID         = 0x00U,  /*!< Full transfer     */
void vga_DMA_XFER_CPLT_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "Full transfer\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_HALFCPLT_CB_ID     = 0x01U,  /*!< Half Transfer     */
void vga_DMA_XFER_HALFCPLT_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "Half Transfer\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_M1CPLT_CB_ID       = 0x02U,  /*!< M1 Full Transfer  */
void vga_DMA_XFER_M1CPLT_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "M1 Full Transfer\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_M1HALFCPLT_CB_ID   = 0x03U,  /*!< M1 Half Transfer  */
void vga_DMA_XFER_M1HALFCPLT_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "M1 Half Transfer\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_ERROR_CB_ID        = 0x04U,  /*!< Error             */
void vga_DMA_XFER_ERROR_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "DMA Error\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_ABORT_CB_ID        = 0x05U,  /*!< Abort             */
void vga_DMA_XFER_ABORT_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "DMA Abort\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_ALL_CB_ID          = 0x06U   /*!< All               */
void vga_DMA_XFER_ALL_CB_ID(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "Full transfer\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}



void registerDebugInterupts(UART_HandleTypeDef * t_huartE){
	huartE = t_huartE;
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_CPLT_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_HALFCPLT_CB_ID, vga_DMA_XFER_HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_M1CPLT_CB_ID, vga_DMA_XFER_M1CPLT_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_M1HALFCPLT_CB_ID, vga_DMA_XFER_M1HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_ERROR_CB_ID, vga_DMA_XFER_ERROR_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_ABORT_CB_ID);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_ALL_CB_ID, vga_DMA_XFER_ALL_CB_ID);
}

void clearVisibleArea(Color * lineBuffPart){
	//uses 32 bit mode to clear faster
	old_memSet(0, (uint32_t*)&lineBuffPart[horiFront+horiSync+horiBack], horiRes/4);
}

void setVerticalSync(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//set VerticalSync everywhere
	old_memSet(0x80808080, (uint32_t*)lineBuffPart, horiWhole/4);
	//set vertical and Horizontal sync in overlap
	old_memSet(0xC0C0C0C0, (uint32_t*)&lineBuffPart[horiFront], horiSync/4);
}

void setVerticalSyncP1(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//set VerticalSync everywhere
	old_memSet(0x80808080, (uint32_t*)lineBuffPart, horiWhole/4);
}

void setVerticalSyncP2(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//set vertical and Horizontal sync in overlap
	old_memSet(0xC0C0C0C0, (uint32_t*)&lineBuffPart[horiFront], horiSync/4);
}

void setHorizontalSync(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//clear VerticalSync everywhere / clear entire buffer
	old_memSet(0, (uint32_t*)lineBuffPart, horiWhole);
	//set Horizontal sync
	old_memSet(0x40404040, (uint32_t*)&lineBuffPart[horiFront], horiSync);
}

void setHorizontalSyncP1(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//clear VerticalSync everywhere / clear entire buffer
	old_memSet(0, (uint32_t*)lineBuffPart, horiWhole);
}

void setHorizontalSyncP2(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//set Horizontal sync
	old_memSet(0x40404040, (uint32_t*)&lineBuffPart[horiFront], horiSync);
}

void __weak renderLine(Color * lineBuffPart, const int lineCount){
	//both buffers are 32 bit aligned

	char str[81] = { '\0' };
	int str_len = sprintf(str, "Rendering line %i\r\n", lineCount);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	//copy the current line of the screen buffer in to the line buffer
	old_memCopy((uint32_t*)&screenBuff[horiRes*lineCount], (uint32_t *)&lineBuffPart[horiFront+horiSync+horiBack], horiRes/4);
}

void copyLastLine(Color * activeBuffer, const Color * oldBuffer){
	//both buffers are 32 bit aligned
	old_memCopy((uint32_t*)&oldBuffer[horiFront+horiSync+horiBack], (uint32_t *)&activeBuffer[horiFront+horiSync+horiBack], horiRes/4);
}



void vgaStateMachine(int activatedFromCircularBuffer){

	while(HAL_DMA_PollForTransfer(memCopyDMA, HAL_DMA_FULL_TRANSFER, 100) != HAL_OK){
		//error triggered early should still be in user code

	}

	if(activatedFromCircularBuffer){
		if(!readyForNextLine){
			//we ran out of time rendering the last line
			//todo log error
		}
		readyForNextLine = 0;
		lineCount++;
		Color * tmp = activeBuffer;
		activeBuffer = oldBuffer;
		oldBuffer = tmp;

	}

	while(1){
		switch(state){
		//Render screen
		case sDecideNext:{
			if(lineCount < vertArea){
				state = sRenderLine;
			}else{
				state = sExitVisible1;
			}
			break;
		}
		case sRenderLine:{
			renderLine(activeBuffer, lineCount);//render line by copying from screenBuff
			lineUpscale = 1;
			state = sDoneRenderLine;
			return;
		}
		case sDoneRenderLine:{
			state = vgaUpscale==1?sDecideNext:sCopyLastLine;
			readyForNextLine = 1;
			return;
		}
		case sCopyLastLine:{
			//we are upscaling and can save recourses by copying last buffer
			//would be faster if we could use a fifo queue of dma transfers instead of a circular buffer
			copyLastLine(activeBuffer, oldBuffer);
			state = sDoneCopylastLine;
			return;
		}
		case sDoneCopylastLine:{
			lineUpscale++;
			if(lineUpscale == vgaUpscale){//waits in current state until we need to render a new line and can't reuse old buffers
				lineUpscale = 0;
				state = sDecideNext;
				readyForNextLine = 1;
			}
			return;
		}
		//Exit visible area
		case sExitVisible1:{
			clearVisibleArea(activeBuffer);//clear leftover data in buffer 1
			state = sDoneExitVisible1;
			return;
		}
		case sDoneExitVisible1:{
			state = sExitVisible2;
			readyForNextLine = 1;
			return;
		}
		case sExitVisible2:{
			clearVisibleArea(activeBuffer);//clear leftover data in buffer 2
			state = sDoneExitVisible2;
			return;
		}
		case sDoneExitVisible2:{
			state = sSetVsync1P1;
			readyForNextLine = 1;
			return;
		}
		//Vertical sync
		case sSetVsync1P1:{
			if(lineCount == vertArea + vertFront){//wait until vertical sync starts
				setVerticalSyncP1(activeBuffer);//set vertical sync in buffer 1
				state = sSetVsync1P2;
			}else{
				readyForNextLine = 1;
			}
			return;
		}
		case sSetVsync1P2:{
			setVerticalSyncP2(activeBuffer);
			state = sDoneSetVsync1;
			return;
		}
		case sDoneSetVsync1:{
			state = sSetVsync2P1;
			readyForNextLine = 1;
			return;
		}
		case sSetVsync2P1:{
			setVerticalSyncP1(activeBuffer);//set Vertical Sync in buffer 2
			state = sSetVsync2P2;
			return;
		}
		case sSetVsync2P2:{
			setVerticalSyncP2(activeBuffer);
			state = sDoneSetVsync2;
			return;
		}
		case sDoneSetVsync2:{
			state = sSetHsync1P1;
			readyForNextLine = 1;
			return;
		}
		//Horizontal sync
		case sSetHsync1P1:{
			if(lineCount == vertArea + vertFront + vertSync){//wait until vertical sync ends
				setHorizontalSyncP1(activeBuffer);//set horizontal sync in buffer 2
				state = sSetHsync1P2;
			}else{
				readyForNextLine = 1;
			}
			return;
		}
		case sSetHsync1P2:{
			setHorizontalSyncP2(activeBuffer);
			state = sDoneSetHsync1;
			return;
		}
		case sDoneSetHsync1:{
			state = sSetHsync2P1;
			readyForNextLine = 1;
			return;
		}
		case sSetHsync2P1:{
			setVerticalSyncP1(activeBuffer);//set Horizontal Sync in buffer 2
			state = sSetHsync2P2;
			return;
		}
		case sSetHsync2P2:{
			setVerticalSyncP2(activeBuffer);
			state = sDoneSetHsync2;
			return;
		}
		case sDoneSetHsync2:{
			state = sEndBuffer;
			readyForNextLine = 1;
			return;
		}
		case sEndBuffer:{
			if(lineCount == vertWhole){//wait until end of the screen
				state = sDecideNext;
				lineCount = -1;// set line count back to the start
			}
			readyForNextLine = 1;
			return;
		}
	}


	}
}

void vgaHalfCallBack(DMA_HandleTypeDef *_hdma){
	//prepareLine(lineBuff, &lineBuff[horiWhole]);
	vgaStateMachine(1);
}

void vgaFullCallBack(DMA_HandleTypeDef *_hdma){
	//prepareLine(&lineBuff[horiWhole], lineBuff);
	vgaStateMachine(1);
}

void vgaCopyAndSetCallBack(DMA_HandleTypeDef *_hdma){
	vgaStateMachine(0);
}


void vgaSetup(
		TIM_HandleTypeDef * vgaPixelTimer_,
		DMA_HandleTypeDef * vgaCircularDMA_,
		DMA_HandleTypeDef * memCopyDMA_){
	vgaPixelTimer = vgaPixelTimer_;
	vgaCircularDMA = vgaCircularDMA_;
	memCopyDMA = memCopyDMA_;

	lineCount = vertArea + vertFront + vertSync - 1;//start right after a vertical sync
	lineUpscale = 0;//copy old buffer if non zero
	readyForNextLine = 1;
	state = sSetVsync1P1;
	activeBuffer = lineBuff;
	oldBuffer = &lineBuff[horiWhole];
}

void vgaStart(){
	HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_HALFCPLT_CB_ID, vgaHalfCallBack);
	HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaFullCallBack);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaCopyAndSetCallBack);

	//__HAL_TIM_ENABLE_DMA(vgaPixelTimer, *vgaCircularDMA);
	//vgaPixelTimer->Instance->DIER |= hdma_tim5_up;
	//__HAL_TIM_ENABLE(&htim5);
	HAL_TIM_Base_Start(vgaPixelTimer);

	//prepare the buffer with the first two lines
	vgaStateMachine(1);
	vgaStateMachine(1);
	//start the circular buffer dma transfer aka vga main loop
	HAL_DMA_Start_IT(vgaCircularDMA, (uint32_t)&lineBuff[0], (uint32_t)&(GPIOC->ODR), horiWhole*2);
}

void vgaStop(){
	//todo stop the circular buffer copy
	// write 0 to the vga port
	// remove call backs
	// set the vga state machine in a good state
}
