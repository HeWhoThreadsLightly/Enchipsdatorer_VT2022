/*
 * vga.c
 *
 *  Created on: 4 apr. 2022
 *      Author: Orion
 */


#include "vga.h"
#include "main.h"
#include "graphicsLib.h"
#include <stdio.h>
//#include "codepage-437-bmp.h"

/*
http://tinyvga.com/vga-timing/640x400@70Hz

General timing
Screen refresh rate	70 Hz
Vertical refresh	31.46875 kHz/4/1.2
Pixel freq.	25.175 MHz

hclock 75.5 Mhz / 3 = 25.1666667 megahertz
/ vga upscale


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
_Alignas(uint32_t) Color screenBuff[horiRes] = {0}; //*vertRes] = {0};//rows*columns
int currentLineValidFor = 0;
int nextLineValidFor = 0;
Color * currentLine;
Color * nextLine;

int frameCount = 0;

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
int enteryCount = 0;
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

void registerHUARTvga(UART_HandleTypeDef * huart){
	huartE = huart;
}

void __weak renderLine(Color * lineBuffPart, const int lineCount){
	//both buffers are 32 bit aligned

	//char str[81] = { '\0' };
	//int str_len = sprintf(str, "Rendering line %i\r\n", lineCount);
	//HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	for (int i = 0; i < horiRes; ++i) {
		lineBuffPart[i].value = ( lineCount + i + (frameCount/64 << 8)) % ColorWhite.value;
	}
}


char strDumpBuff[1000];
void dumpBuffer(char * dump, uint32_t bytes, uint32_t * indicatorLength, char * indicator, uint32_t columnSets){
	char * str = strDumpBuff;
	uint32_t nextIndicator;
	if(indicatorLength != NULL){
		nextIndicator = *indicatorLength;
	}
	str += sprintf(str, "      ");
	for(uint32_t i = 0; i < columnSets; i++){
		str += sprintf(str, "0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f    ");
	}
	str += sprintf(str, "\n\r");
	for(uint32_t i = 0; i < bytes;){

		str += sprintf(str, "%3lx ", i / (16));
		for(uint32_t columnSetI = 0; columnSetI < columnSets; columnSetI++){
			for(uint32_t j = i+16; i<j && i < bytes; i++){
				uint32_t tmp = dump[i];
				if(indicatorLength == NULL){
					str += sprintf(str, " %02lx ", tmp);
				}else{
					if(i == nextIndicator){
						indicatorLength++;
						nextIndicator += *indicatorLength;
						indicator++;
						indicator++;
					}
					str += sprintf(str, "%c%02lx%c", *indicator, tmp, *(indicator+1));
				}
			}
			str += sprintf(str, "  ");
			if(str - strDumpBuff > sizeof(strDumpBuff)/2){
				HAL_UART_Transmit(huartE, (uint8_t*) strDumpBuff, str - strDumpBuff, HAL_MAX_DELAY);
				str = strDumpBuff;
			}
		}
		str += sprintf(str, "\n\r");
	}
	str += sprintf(str, "\n\r");
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

void __attribute__((optimize("O3"))) vgaDriver2(){
#ifdef vgaDebug
	int str_len;
	char str[81] = {'\0'};
	static char * ref_str = "null";
#endif
	lineCount++;

	if(lineCount == vertArea + vertFront){//enter vertical sync
#ifdef vgaDebug
		ref_str = "vertical sync start";
#endif
		HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_SET);

	}else if(lineCount == vertArea + vertFront + vertSync){//exit vertical sync
#ifdef vgaDebug
		ref_str = "vertical sync end";
#endif
		HAL_GPIO_WritePin(Vsync_GPIO_Port, Vsync_Pin, GPIO_PIN_RESET);
	}

	if(currentLineValidFor != 0){//reuse the last dma buffer
		currentLineValidFor--;
		if(vgaCircularDMA->Instance->CR & (1 << 19)){
			//vgaCircularDMA->Instance->M0AR = (uint32_t)currentLine;//dma is currently using buffer 1 update buffer 0
		}else{
			//vgaCircularDMA->Instance->M1AR = (uint32_t)currentLine;//dma is currently using buffer 0 update buffer 1
		}
		//Possibly do some calculations during hBlank;
		return;
	}
	//swap buffers
	{
		Color * tmp = currentLine;
		currentLine = nextLine;
		nextLine = tmp;
		currentLineValidFor = nextLineValidFor;
	}
	//load the new current line buffer into dma
	if(vgaCircularDMA->Instance->CR & (1 << 19)){
		//vgaCircularDMA->Instance->M0AR = (uint32_t)currentLine;//dma is currently using buffer 1 update buffer 0
	}else{
		//vgaCircularDMA->Instance->M1AR = (uint32_t)currentLine;//dma is currently using buffer 0 update buffer 1
	}
	//start rendering the next line

	if(lineCount < vertArea){//render visual line
#ifdef vgaDebug
		ref_str = "render line";
#endif

		renderLine(&nextLine[horiWhole-horiRes], lineCount/vgaUpscale);//call user code to render next line
		nextLineValidFor = vgaUpscale;
	}else if(lineCount == vertArea){//last line clear
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&nextLine[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/(sizeof(uint32_t)/sizeof(Color)); i++){//clear the next buffer
			*active32 = 0;
			active32++;
		}
		nextLineValidFor = 1;
	}else if(lineCount == vertArea + 1){//last line clear buffer
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&nextLine[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/(sizeof(uint32_t)/sizeof(Color)); i++){//clear the next buffer
			*active32 = 0;
			active32++;
		}
		nextLineValidFor = vertWhole-vertArea-1;
	}else if(lineCount >= vertWhole){//return to beginning
#ifdef vgaDebug
		ref_str = "next frame";
#endif
		lineCount = -1;
	}
#ifdef vgaDebug
	str_len = sprintf(str, "Line %i\t %s \n\r", lineCount, ref_str);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	uint32_t indicatorLengths[] = {horiFront, horiSync, horiBack, horiRes, horiFront, horiSync, horiBack, horiRes};
	dumpBuffer((char*)lineBuff, horiWhole*2, indicatorLengths, "  []  ||  []  ||EE", 2);
#endif
}

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
		uint32_t * screen32 = (uint32_t*)&screenBuff[(lineCount/vgaUpscale)*horiRes];
		for(uint32_t i = 0; i < horiRes/(sizeof(uint32_t)/sizeof(Color));i++){
			*active32 = *screen32;
			//*active32 = 0x77777777;
			++active32;
			++screen32;
			//activeBuffer[i] = screenBuff[lineCount*vertRes + i];
		}
	}else if(lineCount == vertArea){//last line clear
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&activeBuffer[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/(sizeof(uint32_t)/sizeof(Color)); i++){
			*active32 = 0;
			active32++;
		}
	}else if(lineCount == vertArea + 1){//last line clear buffer
#ifdef vgaDebug
		ref_str = "clear line";
#endif
		uint32_t * active32 = (uint32_t*)&activeBuffer[horiWhole-horiRes];
		for(uint32_t i = 0; i < horiRes/(sizeof(uint32_t)/sizeof(Color)); i++){
			*active32 = 0;
			active32++;
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
		frameCount++;
	}

#ifdef vgaDebug
	str_len = sprintf(str, "Line %i\t %s \n\r", lineCount, ref_str);
	HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	uint32_t indicatorLengths[] = {horiFront, horiSync, horiBack, horiRes, horiFront, horiSync, horiBack, horiRes};
	dumpBuffer((char*)lineBuff, horiWhole*2, indicatorLengths, "  []  ||  []  ||EE", 2);
#endif
}

void vgaHalfCallBack(DMA_HandleTypeDef *_hdma){
	//readyForNextLine++;
	enteryCount++;
	vgaDriver2();
	enteryCount--;
}

void vgaFullCallBack(DMA_HandleTypeDef *_hdma){
	//readyForNextLine++;
	enteryCount++;
	vgaDriver2();
	enteryCount--;
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
	activeBuffer = lineBuff;
	oldBuffer = &lineBuff[horiWhole];
	currentLine = lineBuff;
	nextLine = &lineBuff[horiWhole];

	for(uint32_t i = 0; i < horiWhole; i += 2){//clear all
		activeBuffer[i].value = 0;
		oldBuffer[i].value = 0;
		currentLine[i].value = 0;
		nextLine[i].value = 0;
	}
	Color * currentDisplayPart = &currentLine[horiWhole-horiRes];
	Color * nextDisplayPart = &nextLine[horiWhole-horiRes];
	for(uint32_t i = 0; i < horiWhole; i++){//load test data
		currentDisplayPart[i] = ColorWhite;
		nextDisplayPart[i] = ColorWhite;
	}
	for(uint32_t i = horiFront; i < horiFront + horiSync; i++){//set horizontal sync
		activeBuffer[i] = ColorHsync;
		oldBuffer[i] = ColorHsync;
		currentLine[i] = ColorHsync;
		nextLine[i] = ColorHsync;
	}
	uint32_t indicatorLengths[] = {horiFront, horiSync, horiBack, horiRes, horiFront, horiSync, horiBack, horiRes};
	dumpBuffer((char*)lineBuff, horiWhole*2, indicatorLengths, "  []  ||  []  ||EE", 2);
}

void vgaStart(){

	//dumpBuffer((char*)screenBuff, horiRes*vertRes, NULL, "", 2);
	//HAL_DMA_Init(vgaCircularDMA);
	__HAL_TIM_ENABLE_DMA(vgaPixelTimer, TIM_DMA_UPDATE);
	__HAL_TIM_ENABLE(vgaPixelTimer);

	HAL_StatusTypeDef statusTIM = HAL_TIM_PWM_Start(vgaPixelTimer, TIM_CHANNEL_1);

	//HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_HALFCPLT_CB_ID, vgaHalfCallBack);
	//HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaFullCallBack);ck);
	HAL_StatusTypeDef statusReg1 = HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_M1CPLT_CB_ID, vgaDriver2);
	HAL_StatusTypeDef statusReg2 = HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_CPLT_CB_ID, vgaDriver2);
	HAL_StatusTypeDef statusReg3 = HAL_DMA_RegisterCallback(vgaCircularDMA, HAL_DMA_XFER_ERROR_CB_ID, vgaStop);

	//__HAL_TIM_ENABLE(&htim5);

	//start the circular buffer dma transfer aka vga main loop
	HAL_StatusTypeDef statusDMA = HAL_DMAEx_MultiBufferStart_IT(vgaCircularDMA, (uint32_t)&lineBuff[0], (uint32_t)&(GPIOC->ODR), (uint32_t)&lineBuff[horiWhole], horiWhole);
	//HAL_DMA_Start_IT(vgaCircularDMA, (uint32_t)&lineBuff[0], (uint32_t)&(GPIOC->ODR), horiWhole*2);

	//HAL_DMAEx_MultiBufferStart_IT(hdma, SrcAddress, DstAddress, SecondMemAddress, DataLength);
	//vgaLoop();
}

int vgaErrorCount;
void vgaStop(){
	//todo stop the circular buffer copy
	// write 0 to the vga port
	// remove call backs
	// set the vga state machine in a good state
	return;
	vgaErrorCount++;
	if((vgaErrorCount&0xFFF) == 0xFFF){
		char str[81] = { '\0' };
		int str_len = sprintf(str, "DMA error detected depth %u count %u\r\n", enteryCount, vgaErrorCount);
		HAL_UART_Transmit(huartE, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	}
	return;
	while(1){

	}
}
