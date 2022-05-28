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


#endif /* INC_GRAPHICSLIB_H_ */
