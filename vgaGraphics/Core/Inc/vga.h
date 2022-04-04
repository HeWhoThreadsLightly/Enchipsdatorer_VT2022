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

enum { horiRes = 80};
enum { vertRes = 50};

extern Color lineBuff [];//double  buffered
extern Color backgroundBuff [];//rows*columns
extern Color forgroundBuff [];//rows*columns
extern Color charBuff [];//rows*columns
extern int vgaCharMode;

void setRed(Color * c, char r);
void setGreen(Color * c, char g);
void setBlue(Color * c, char b);
void setRGB(Color * c, char r, char g, char b);
void setHblank(Color * c);
void setVblank(Color * c);

void clearVisibleAria(Color * lineBuffPart);
void setVerticalSync(Color * lineBuffPart);
void setHorizontalSync(Color * lineBuffPart);




#endif /* INC_VGA_H_ */
