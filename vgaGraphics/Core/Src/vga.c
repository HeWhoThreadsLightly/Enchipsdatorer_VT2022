/*
 * vga.c
 *
 *  Created on: 4 apr. 2022
 *      Author: Orion
 */


#include "vga.h"

/*
http://tinyvga.com/vga-timing/640x400@70Hz

General timing
Screen refresh rate	70 Hz
Vertical refresh	31.46875 kHz
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

enum { horiAria = 640/vgaDownscale};
enum { horiFront = 16/vgaDownscale};
enum { horiSync = 96/vgaDownscale};
enum { horiBack = 48/vgaDownscale};
enum { horiWhole = 800/vgaDownscale};

enum { vertAria = 400};
enum { vertFront = 12};
enum { vertSync = 2};
enum { vertBack = 35};
enum { vertWhole = 449};


Color lineBuff [horiWhole*2] = {0};//double  buffered
Color screenBuff[horiRes*vertRes] = {0};//rows*columns
Color backgroundBuff [horiChar*vertChar] = {0};//rows*columns
Color forgroundBuff [horiChar*vertChar] = {0};//rows*columns
Color charBuff [horiChar*vertChar] = {0};//rows*columns

int vgaCharMode = 0;

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

void clearVisibleAria(Color * lineBuffPart){
	//uses 32 bit mode to clear faster
	//todo replace with DMA mem clear
	uint32_t * begin = (void*)lineBuffPart;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wtautological-compare"
	_Static_assert(&lineBuffPart[horiAria] == lineBuffPart + horiAria, "Line buffer arithmetic broken");
	_Static_assert(horiAria%4==0, "Line buffer not a multiple of 4");
	#pragma GCC diagnostic pop

	uint32_t * end = (void*)&lineBuffPart[horiAria];
	//clear main Visible aria
	while(begin != end){
		*begin = 0;
		++begin;
	}

}

void setVerticalSync(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster
	//todo replace with DMA mem clear
	uint32_t * begin = (void*)lineBuffPart;
	uint32_t * end = (void*)&lineBuffPart[horiWhole];

	//set VerticalSync everywhere
	while(begin != end){
		*begin = 0x80808080;
		++begin;
	}

	begin = (void*)&lineBuffPart[horiAria+horiFront];
	end = (void*)&lineBuffPart[horiAria+horiFront+horiSync];
	//set vertical and Horizontal sync in overlap
	while(begin != end){
		*begin = 0xC0C0C0C0;
		++begin;
	}
}

void setHorizontalSync(Color * lineBuffPart){
	//uses 32 bit accesses to clear faster
	//todo replace with DMA mem clear
	uint32_t * begin = (void*)lineBuffPart;
	uint32_t * end = (void*)&lineBuffPart[horiWhole];

	//clear VerticalSync everywhere / clear entire buffer
	while(begin != end){
		*begin = 0x0;
	}
	begin = (void*)&lineBuffPart[horiAria+horiFront];
	end = (void*)&lineBuffPart[horiAria+horiFront+horiSync];
	//set Horizontal sync
	while(begin != end){
		*begin = 0x40404040;
		++begin;
	}
}

void upscaleBackground(Color * lineBuffPart, const int lineCount){
	//uses 32 bit accesses to upscale faster
	//todo replace with a series of byte to 8 byte mem copies using dma
	//todo remove?
	Color * cBegin = &backgroundBuff[horiRes*lineCount];
	Color * cEnd = cBegin + vertRes;
	uint32_t * lBegin = (void*)lineBuffPart;
	uint32_t * lEnd = (void*)&lineBuffPart[horiAria];
	while(cBegin != cEnd){
		uint32_t colorExtended = cBegin->value;
		colorExtended = colorExtended << 8 | colorExtended;
		colorExtended = colorExtended << 16 | colorExtended;
		* lBegin = colorExtended;
		++lBegin;
		* lBegin = colorExtended;
		++lBegin;
		++cBegin;
	}
	if(lBegin != lEnd){
		//assert(lBegin == lEnd);//todo implement/find run time error reporting
	}
}

void vgaSendBuffer(Color * lineBuffPart){
	//send the currently active buffer to GPIOC output register to 8 byte aligned pins 0-7
	//todo priority replace with memory to peripheral dma transfer
	Color * begin = lineBuffPart;
	Color * end = &lineBuffPart[horiWhole];
	while(begin != end){
		*((volatile Color*) &(GPIOC->ODR)) = *begin;
		++begin;
	}
}



void prepareLine(Color * activeBuffer){
	static int lineCount = vertAria + vertFront + vertSync;//start right after a vertical sync
		if((lineCount&0b1) == 0){//todo there is a odd number of lines in a frame get active buffer from DMA status or boolean toggle
		activeBuffer += horiWhole;
	}
	if(lineCount < vertAria){//render color by upscaling colorBuff and decoding charBuf
		if(vgaCharMode == 1){// render characters in monochrome

		}else if(vgaCharMode == 2){//render characters in color

		}else{// color mode only using the background
			//todo make it so that you can use swap between the 3 buffers
			upscaleBackground(activeBuffer, lineCount);
		}
	}

	else if(lineCount == vertAria){//clear leftover data in buffer 1
		clearVisibleAria(activeBuffer);
	}else if(lineCount == vertAria + 1){//clear leftover data in buffer 2
		clearVisibleAria(activeBuffer);
	}

	else if(lineCount == vertAria + vertFront){//set Vertical Sync in buffer 1
		setVerticalSync(activeBuffer);
	}else if(lineCount == vertAria + vertFront + 1){//set Vertical Sync in buffer 2
		setVerticalSync(activeBuffer);
	}

	else if(lineCount == vertAria + vertFront + vertSync){//set Horizontal Sync in buffer 1
		setHorizontalSync(activeBuffer);
	}else if(lineCount == vertAria + vertFront + vertSync + 1){//set Horizontal Sync in buffer 2
		setHorizontalSync(activeBuffer);
	}

	else if(lineCount == vertWhole){// set line count back to the start
		lineCount = -1;
	}
	++lineCount;
}


void vgaLoop(){
	while(1){
		prepareLine(lineBuff);
		vgaSendBuffer(lineBuff);
		prepareLine(&lineBuff[horiWhole]);
		vgaSendBuffer(&lineBuff[horiWhole]);
	}
}
