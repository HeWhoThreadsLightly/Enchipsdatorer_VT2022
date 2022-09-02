/*
 * graphicsLib.c
 *
 *  Created on: 11 apr. 2022
 *      Author: Orion
 */

#include "graphicsLib.h"
#include "codepage-437-bmp.h"
#include "vga.h"


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

int getBitN(uint32_t n, char * buff){
	return (buff[n/8] >> (n%8)) & 0b1;
}

Color combineColors(Color existing, Color new){
	if((existing.value >> 6) & 1){
		return new;//old color is a empty texture use the new color
	}
	switch(new.value>>6){
	case 0b00: return new;//new color is opaque
	case 0b11: return existing;//new color is transparent
	case 0b01://new color is partially opaque or a empty texture
	case 0b10://new color is partially transparent
	default:
		{
			Color t;
			t.value = 0b00101010;//purple error not implemented
			return t;
		}
	}
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
