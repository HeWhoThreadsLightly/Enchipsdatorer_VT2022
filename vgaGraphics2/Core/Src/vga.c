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

GPIO_TypeDef* Vsync_GPIO_Port;
uint16_t Vsync_Pin;

//state machine state defined in vgaSetup
int lineCount;//start right after a vertical sync
int lineUpscale;//copy old buffer if non zero
int readyForNextLine;
int vgaCantKeepUpp = 0;
vgaState state;
Color * activeBuffer;
Color * oldBuffer;

//debug logging
UART_HandleTypeDef * huartE = NULL;

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

void vgaCopyAndSetCallBack(DMA_HandleTypeDef *_hdma);



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
	char str[81] = { '\0' };
	int str_len = sprintf(str, "start mem copy\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
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
	char str[81] = { '\0' };
	int str_len = sprintf(str, "start mem set\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	//CLEAR_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(memCopyDMA, (uint32_t)&setVal, (uint32_t)DstAddress, DataLength);
}


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
	char * err = "Default";
	uint32_t errorCode = HAL_DMA_GetError(vgaCircularDMA);
	if(errorCode == HAL_DMA_ERROR_NONE){
		int str_len = sprintf(str, "DMA %s\r\n", "No error");
		HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	}
	while(errorCode){
		switch(errorCode){
		case HAL_DMA_ERROR_NONE            : err = "No error";break;    /*!< No error                               */
		case HAL_DMA_ERROR_TE              : err = "Transfer error";break;    /*!< Transfer error                         */
		case HAL_DMA_ERROR_FE              : err = "FIFO error";break;    /*!< FIFO error                             */
		case HAL_DMA_ERROR_DME             : err = "Direct Mode error";break;    /*!< Direct Mode error                      */
		case HAL_DMA_ERROR_TIMEOUT         : err = "Timeout error";break;    /*!< Timeout error                          */
		case HAL_DMA_ERROR_PARAM           : err = "Parameter error";break;    /*!< Parameter error                        */
		case HAL_DMA_ERROR_NO_XFER         : err = "Abort requested with no Xfer ongoing";break;    /*!< Abort requested with no Xfer ongoing   */
		case HAL_DMA_ERROR_NOT_SUPPORTED   : err = "Not supported mode";break;    /*!< Not supported mode                     */
		}

		int str_len = sprintf(str, "DMA %s\r\n", err);
		HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
		errorCode &= errorCode - 1;
	};

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



void registerDebugInterupts(DMA_HandleTypeDef * toDebug){
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_CPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_HALFCPLT_CB_ID, vga_DMA_XFER_HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_M1CPLT_CB_ID, vga_DMA_XFER_M1CPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_M1HALFCPLT_CB_ID, vga_DMA_XFER_M1HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ERROR_CB_ID, vga_DMA_XFER_ERROR_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_ABORT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ALL_CB_ID, vga_DMA_XFER_ALL_CB_ID);
}

void registerHUART(UART_HandleTypeDef * huart){
	huartE = huart;
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
	//HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_CPLT_CB_ID, setVerticalSyncP2);
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
	old_memSet(0, (uint32_t*)lineBuffPart, horiWhole/4);
	//set Horizontal sync
	old_memSet(0x40404040, (uint32_t*)&lineBuffPart[horiFront], horiSync/4);
}

void setHorizontalSyncP1(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//clear VerticalSync everywhere / clear entire buffer
	old_memSet(0, (uint32_t*)lineBuffPart, horiWhole/4);
}

void setHorizontalSyncP2(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster

	//set Horizontal sync
	old_memSet(0x40404040, (uint32_t*)&lineBuffPart[horiFront], horiSync/4);
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


void printVgaState(){
	char * ref_str;
	char str[81] = { '\0' };
	int str_len = 0;

	switch(state){
	case sDecideNext:ref_str = "sDecideNext";break;
	case sRenderLine:ref_str = "sRenderLine";break;
	case sDoneRenderLine:ref_str = "sDoneRenderLine";break;
	case sCopyLastLine:ref_str = "sCopyLastLine";break;
	case sDoneCopylastLine:ref_str = "sDoneCopylastLine";break;

	case sExitVisible1:ref_str = "sExitVisible1";break;
	case sDoneExitVisible1:ref_str = "sDoneExitVisible1";break;
	case sExitVisible2:ref_str = "sExitVisible2";break;
	case sDoneExitVisible2:ref_str = "sDoneExitVisible2";break;

	case sSetVsync1P1:ref_str = "sSetVsync1P1";break;
	case sSetVsync1P2:ref_str = "sSetVsync1P2";break;
	case sDoneSetVsync1:ref_str = "sDoneSetVsync1";break;
	case sSetVsync2P1:ref_str = "sSetVsync2P1";break;
	case sSetVsync2P2:ref_str = "sSetVsync2P2";break;
	case sDoneSetVsync2:ref_str = "sDoneSetVsync2";break;

	case sSetHsync1P1:ref_str = "sSetHsync1P1";break;
	case sSetHsync1P2:ref_str = "sSetHsync1P2";break;
	case sDoneSetHsync1:ref_str = "sDoneSetHsync1";break;
	case sSetHsync2P1:ref_str = "sSetHsync2P1";break;
	case sSetHsync2P2:ref_str = "sSetHsync2P2";break;
	case sDoneSetHsync2:ref_str = "sDoneSetHsync2";break;

	case sEndBuffer:ref_str = "sEndBuffer";break;
	default:ref_str = "Unknown";break;
	}
	str_len = sprintf(str, "Line %i\t vga state %s \t", lineCount, ref_str);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}

void vgaStateMachine(int activatedFromCircularBuffer){

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
		printVgaState();
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
				HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_SET);
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
				HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_RESET);
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

char strDumpBuff[1200];
void dumpBuffer(Color * activeBuffer){
	char * str = strDumpBuff;
	uint32_t columnSets = 2;

	str += sprintf(str, "Line %i\n\r", lineCount);
	str += sprintf(str, "    ");
	for(uint32_t i = 0; i < columnSets; i++){
		str += sprintf(str, "0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   ");
	}
	str += sprintf(str, "\n\r");
	for(uint32_t i = 0; i < horiWhole;){

		str += sprintf(str, "%3lx ", i / (16*columnSets));
		for(uint32_t columnSetI = 0; columnSetI < columnSets; columnSetI++){
			for(uint32_t j = i+16; i<j && i < horiWhole; i++){
				uint32_t tmp = activeBuffer[i].value;
				str += sprintf(str, "%02lx ", tmp);
			}
			str += sprintf(str, "  ");
		}
		str += sprintf(str, "\n\r");
	}
	str += sprintf(str, "\n\r");
	str += sprintf(str, "                                            ");
	//str += sprintf(str, "\0");
	HAL_UART_Transmit(huartE, (uint8_t*) strDumpBuff, str - strDumpBuff, HAL_MAX_DELAY);
}

/*
//enum { vertRes = 400/vgaUpscale};//defined in header
enum { vertArea = 400 };
enum { vertFront = 12};
enum { vertSync = 2};
enum { vertBack = 35};
enum { vertWhole = 449};
 */
//#define vgaDebug
void __attribute__((optimize("O3"))) vgaDriver(){
	{
		Color * tmp = activeBuffer;
		activeBuffer = oldBuffer;
		oldBuffer = tmp;
	}
#ifdef vgaDebug
	int str_len;
	char str[81] = {'\0'};
	static char * ref_str = "null";
#endif
	lineCount++;
	if(lineCount < vertArea){//send line
#ifdef vgaDebug
		ref_str = "render line";
#endif
		//renderLine(activeBuffer, lineCount);
		//while(HAL_DMA_PollForTransfer(memCopyDMA, HAL_DMA_FULL_TRANSFER, 100)){HAL_Delay(1);};
		uint32_t * active32 = (uint32_t*)&activeBuffer[horiWhole-horiRes];
		uint32_t * screen32 = (uint32_t*)&screenBuff[lineCount*vertRes];
		for(uint32_t i = 0; i < horiRes/4;i++){
			*active32 = *screen32;
			++active32;
			++screen32;
			//activeBuffer[i] = screenBuff[lineCount*vertRes + i];
		}

	}else if(lineCount == vertArea){//last line clear
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&activeBuffer[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/4; i++){
			*active32 = 0;
		}
	}else if(lineCount == vertArea + 1){//last line clear buffer
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&activeBuffer[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/4; i++){
			*active32 = 0;
		}
	}else if(lineCount == vertArea + vertFront){//enter vertical sync todo check for of by one error
#ifdef vgaDebug
		ref_str = "vertical sync start";
#endif
		HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_SET);

	}else if(lineCount == vertArea + vertFront + vertSync){//exit vertical sync
#ifdef vgaDebug
		ref_str = "vertical sync end";
#endif
		HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_RESET);

	}else if(lineCount >= vertWhole){//return to beginning
#ifdef vgaDebug
		ref_str = "next frame";
#endif
		lineCount = -1;
	}
	//dumpBuffer(activeBuffer);
#ifdef vgaDebug
	str_len = sprintf(str, "Line %i\t %s \t", lineCount, ref_str);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
#endif
}

void vgaHalfCallBack(DMA_HandleTypeDef *_hdma){
	//prepareLine(lineBuff, &lineBuff[horiWhole]);
	//readyForNextLine++;
	//vga_DMA_XFER_HALFCPLT_CB_ID();
	vgaDriver();
	//vgaStateMachine(1);
}

void vgaFullCallBack(DMA_HandleTypeDef *_hdma){
	//prepareLine(&lineBuff[horiWhole], lineBuff);
	//readyForNextLine++;
	//vga_DMA_XFER_CPLT_CB_ID();
	vgaDriver();
	//vgaStateMachine(1);
}

void vgaCopyAndSetCallBack(DMA_HandleTypeDef *_hdma){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "memCopyDMA\r\n");
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	vgaStateMachine(0);
}

void reportOVerspeed(){
	char str[81] = { '\0' };
	int str_len = sprintf(str, "VGA can't keep up %i\r\n", vgaCantKeepUpp);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}

void vgaLoop(){
	while(1){
		if(readyForNextLine){
			vgaDriver();
			if(readyForNextLine > 1){
				vgaCantKeepUpp++;
				//reportOVerspeed();
			}
			readyForNextLine--;
		}
	}
}

void vgaSetup(
		TIM_HandleTypeDef * vgaPixelTimer_,
		DMA_HandleTypeDef * vgaCircularDMA_,
		DMA_HandleTypeDef * memCopyDMA_,
		GPIO_TypeDef* Vsync_GPIO_Port_,
		uint16_t Vsync_Pin_
		){

	vgaPixelTimer = vgaPixelTimer_;
	vgaCircularDMA = vgaCircularDMA_;
	memCopyDMA = memCopyDMA_;

	Vsync_GPIO_Port = Vsync_GPIO_Port_;
	Vsync_Pin = Vsync_Pin_;

	lineCount = 0; //vertArea + vertFront - 1 - 2;//start right after a vertical sync
	lineUpscale = 0;//copy old buffer if non zero
	readyForNextLine = 1;
	state = sSetVsync1P1;
	activeBuffer = lineBuff;
	oldBuffer = &lineBuff[horiWhole];

	for(uint32_t i = 0; i < horiWhole; i += 2){//clear all
		activeBuffer[i].value = 0;
		oldBuffer[i].value = 0;
	}
	for(uint32_t i = 0; i < horiRes; i++){//load test data
		activeBuffer[i].value = 0x0;
		oldBuffer[i].value = 0x00;
	}
	for(uint32_t i = horiFront; i < horiFront + horiSync; i++){//set horizontal sync
		activeBuffer[i].value = 0x80;
		oldBuffer[i].value = 0x80;
	}
}



void vgaStart(){
	//HAL_DMA_Init(vgaCircularDMA);
	__HAL_TIM_ENABLE_DMA(vgaPixelTimer, TIM_DMA_UPDATE);
	__HAL_TIM_ENABLE(vgaPixelTimer);

	HAL_TIM_PWM_Start(vgaPixelTimer, TIM_CHANNEL_1);

	HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_HALFCPLT_CB_ID, vgaHalfCallBack);
	HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaFullCallBack);
	HAL_DMA_RegisterCallback(memCopyDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaCopyAndSetCallBack);

	//__HAL_TIM_ENABLE(&htim5);

	//HAL_TIM_Base_Init(vgaPixelTimer);
	//HAL_TIM_Base_Start_IT(vgaPixelTimer);
	//HAL_TIM_Base_Start_DMA(htim, pData, Length);//used to load the timer with new times each time it triggers not our usecase

	//prepare the buffer with the first two lines
	//vgaStateMachine(1);
	//vgaStateMachine(1);

	//start the circular buffer dma transfer aka vga main loop
	HAL_DMA_Start_IT(vgaCircularDMA, (uint32_t)&lineBuff[0], (uint32_t)&(GPIOC->ODR), horiWhole*2);
	//__HAL_TIM_ENABLE_DMA(vgaPixelTimer, TIM_DMA_UPDATE);//transfer error
    //__HAL_TIM_ENABLE_DMA(vgaPixelTimer, TIM_DMA_CC3);//no effect
	//__HAL_TIM_ENABLE_DMA(vgaPixelTimer, TIM_DMA_TRIGGER);//no effect


	//HAL_DMAEx_MultiBufferStart_IT(hdma, SrcAddress, DstAddress, SecondMemAddress, DataLength);
	//vgaLoop();
}

void vgaStop(){
	//todo stop the circular buffer copy
	// write 0 to the vga port
	// remove call backs
	// set the vga state machine in a good state
}
