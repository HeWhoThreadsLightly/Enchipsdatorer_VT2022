/*
 * graphicsLib.c
 *
 *  Created on: 11 apr. 2022
 *      Author: Orion
 */

#include "graphicsLib.h"
#include "codepage-437-bmp.h"
#include "vga.h"

int getBitN(uint32_t n, char * buff){
	return (buff[n/8] >> (n%8)) & 0b1;
}

Color combineColors(Color existing, Color new){
	switch(new.value>>6){
	case 0b00: return new;//new color is opaque
	case 0b11: return existing;//new color is transparent
	case 0b01://new color is partially opaque
	case 0b10://new color is partially transparent
	default:
		{
			Color t;
			t.value = 0b00101010;//purple error not implemented
			return t;
		}
	}
}

void renderChar(char c, int h, int v, Color background, Color forground){
	uint32_t bitOffset = (c/(Codepage_437_width/Codepage_437_char_width))*(Codepage_437_width*Codepage_437_char_hight)
					+ (c%(Codepage_437_width/Codepage_437_char_width))*Codepage_437_char_width;
	char bitBuffer;
	int remainingBits = 0;
	int screenPosition = horiRes*h+v;
	for(int i=0;i<Codepage_437_char_hight;i++){
		for(int j=0;j<Codepage_437_char_width;j++){
			if(remainingBits == 0){
				remainingBits = bitBuffer%8;
				bitBuffer = codepage_437_map[bitOffset/8] >> (8-remainingBits);
			}
			if(screenPosition >= 0){
				if(bitBuffer&1){//use foreground color
					screenBuff[screenPosition] = combineColors(screenBuff[screenPosition], forground);
				}else{//use background color
					screenBuff[screenPosition] = combineColors(screenBuff[screenPosition], background);
				}
			}
			remainingBits--;
			screenPosition++;
		}
		screenPosition += horiRes - Codepage_437_char_width;
		bitOffset += Codepage_437_width - Codepage_437_char_width;
	}
}

void renderString(char * str, int h, int v, Color background, Color forground){
	while(*str != 0){
		renderChar(*str, h, v, background, forground);
		v += Codepage_437_char_width;
		str++;
	}
}

