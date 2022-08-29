/*
 * graphicsLib.h
 *
 *  Created on: 11 apr. 2022
 *      Author: Orion
 */

#ifndef INC_GRAPHICSLIB_H_
#define INC_GRAPHICSLIB_H_

#include "vga.h"

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);

typedef struct Sprite{
	uint8_t sprite_width;
	uint8_t sprite_hight;
	uint8_t * data;
} Sprite;

typedef struct Sprite_map{
	uint8_t sprite_hori;
	uint8_t sprite_vert;
	uint16_t sprite_count;
	uint8_t data[];
} Sprite_map;

#endif /* INC_GRAPHICSLIB_H_ */
