/*
 * graphicsLib.c
 *
 *  Created on: 11 apr. 2022
 *      Author: Orion
 */

#include "graphicsLib.h"
#include "codepage-437-bmp.h"
#include "vga.h"
#include "math.h"

const Color ColorTransparant = {0xC000};
const Color ColorHsync = {0x1000};
const Color ColorWhite = {0xFFF};
const Color ColorBlack = {0x000};
const Color ColorRed = {0xF00};
const Color ColorGreen = {0x0F0};
const Color ColorBlue = {0x00F};

void setRed(Color * c, char r){
	uint16_t R = r;
	c->value = (c->value & ~0xF00) | (R & 0xF) << 8;
}
void setGreen(Color * c, char g){
	uint16_t G = g;
	c->value = (c->value & ~0XF0) | (G & 0XF) << 4;
}
void setBlue(Color * c, char b){
	uint16_t B = b;
	c->value = (c->value & ~0XF) | (B & 0XF);
}
void setRGB(Color * c, char r, char g, char b){
	uint16_t R = r & 0xF;
	uint16_t G = g & 0xF;
	uint16_t B = b & 0xF;
	c->value = R << 8 | G << 4 | B;
}
void setHblank(Color * c){
	c->value = 1 << 12;
}

void setVblank(Color * c){
	c->value = 0;
}

int getBitN(uint32_t n, char * buff){
	return (buff[n/8] >> (n%8)) & 0b1;
}

Color combineColors(Color existing, Color new){
	if((new.value >> 14) == 0b00){//new color is transparent
		return existing;
	}
	if((new.value >> 14) == 0b11){//new color is opaque and overwrites the old
		new.value &= ~(0b11 << 14); //clear the transparency value
		return new;
	}
	float r = ((existing.value >> 8) & 0xF) * ((existing.value >> 8) & 0xF) + ((new.value >> 8) & 0xF) * ((new.value >> 8) & 0xF);
	float g = ((existing.value >> 4) & 0xF) * ((existing.value >> 4) & 0xF) + ((new.value >> 4) & 0xF) * ((new.value >> 4) & 0xF);
	float b = ((existing.value >> 0) & 0xF) * ((existing.value >> 0) & 0xF) + ((new.value >> 0) & 0xF) * ((new.value >> 0) & 0xF);
	uint16_t R = sqrtf(r);
	uint16_t G = sqrtf(g);
	uint16_t B = sqrtf(b);
	if(R > 0xF) R = 0xF;
	if(G > 0xF) G = 0xF;
	if(B > 0xF) B = 0xF;
	Color ret;
	ret.value = R << 8 | G << 4 | B;
	return ret;
}

void renderChar(char c, int h, int v, const Color background, const Color forground, const Sprite_map * font){
	if(h < -font->sprite_hori || h >= horiRes
		|| v < -font->sprite_vert || v >= vertRes) return;//sprite is outside visible area

	const uint8_t * data = font->data;
	uint8_t bitBuffer;
	uint8_t bitsRemaining;
	{
		uint32_t bitOffset = c * font->sprite_hori * font->sprite_vert + font->sprite_hori - 1;// 8 + 2
		data += bitOffset / 8; // 0b10xx
		bitsRemaining = 8 - bitOffset % 8;
		bitBuffer = *data >> (8 - bitsRemaining + 1);
	}
	for(int vpx = 0; vpx <  font->sprite_vert; vpx++){
		for(int hpx = 0; hpx <  font->sprite_hori; hpx++){
			if(bitsRemaining == 0){//out of pixel data load more
				bitBuffer = *data;
				data++;
				bitsRemaining = 8;
			}
			if(		!(v + vpx < 0 || v + vpx >= vertRes) &&//row is outside screen
					!(h + hpx < 0 || h + hpx >= horiRes)){ //pixel is outside screen
				uint32_t pos = (v + vpx) * horiRes + h + hpx;
				screenBuff[pos] = combineColors(screenBuff[pos], bitBuffer&1 ? forground : background);
			}
			bitsRemaining--;
			bitBuffer >>= 1;
		}
	}
}

void renderString(char * str, int h, int v, const Color background, const Color forground, const Sprite_map * font){
	while(*str != 0){
		renderChar(*str, h, v, background, forground, font);
		h += font->sprite_hori;
		str++;
	}
}


void renderCharOnGrid(char c, int h, int v, const Color background, const Color forground, const Sprite_map * font){
	renderChar(c, h*font->sprite_hori, v*font->sprite_vert, background, forground, font);
}

void renderStringOnGrid(char * str, int h, int v, const Color background, const Color forground, const Sprite_map * font){
	renderString(str, h*font->sprite_hori, v*font->sprite_vert, background, forground, font);
}
