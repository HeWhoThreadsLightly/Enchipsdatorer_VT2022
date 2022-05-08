/*
 * memFunc.h
 *
 *  Created on: 8 maj 2022
 *      Author: Orion
 */

#ifndef INC_MEMFUNC_H_
#define INC_MEMFUNC_H_

#include <stdio.h>
#include "main.h"

HAL_StatusTypeDef memCopy32(DMA_HandleTypeDef * memtomemDMA, uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef memSet32(DMA_HandleTypeDef * memtomemDMA, uint32_t value, uint32_t * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef memCopy(DMA_HandleTypeDef * memtomemDMA, void * SrcAddress, void * DstAddress, uint32_t DataLength);
HAL_StatusTypeDef memSet(DMA_HandleTypeDef * memtomemDMA, uint32_t value, void * DstAddress, uint32_t DataLength);


#endif /* INC_MEMFUNC_H_ */
