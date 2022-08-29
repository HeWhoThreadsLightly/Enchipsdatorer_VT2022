/*
 * dmaDebug.h
 *
 *  Created on: May 24, 2022
 *      Author: Orion
 */

#ifndef SRC_DMADEBUG_H_
#define SRC_DMADEBUG_H_

#include "main.h"

void registerDebugInterupts(DMA_HandleTypeDef * toDebug);
void registerHUARTdmaDebug(UART_HandleTypeDef * huart);

#endif /* SRC_DMADEBUG_H_ */
