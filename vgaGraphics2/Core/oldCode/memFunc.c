/*
 * memFunc.c
 *
 *  Created on: 8 maj 2022
 *      Author: Orion
 */
#include "memFunc.h"
#include <stdio.h>
#include "main.h"


/**
 * @brief copies 4byte aligned data
 *
 * @param SrcAddress copies from source address incrementing
 *
 * @param DstAddress copies to destination address incrementing
 *
 * @param DataLength number of unit32_t to be copied
 */
HAL_StatusTypeDef old_memCopy(uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength){

	hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	char str[81] = { '\0' };
	int str_len = sprintf(str, "start mem copy\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	//SET_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (uint32_t)SrcAddress, (uint32_t)DstAddress, DataLength);
}

/**
 * @brief sets 4byte aligned data
 *
 * @param value value to be written
 *
 * @param DstAddress destination address incrementing
 *
 * @param DataLength number of unit32_t to be written
 */
HAL_StatusTypeDef old_memSet(uint32_t value, uint32_t * DstAddress, uint32_t DataLength){
	static volatile uint32_t setVal = 0;
	setVal = value;
	hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_DISABLE;
	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	char str[81] = { '\0' };
	int str_len = sprintf(str, "start mem set\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	//CLEAR_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (uint32_t)&setVal, (uint32_t)DstAddress, DataLength);
}


/**
 * @brief copies 4byte aligned data
 *
 * @param SrcAddress copies from source address incrementing
 *
 * @param DstAddress copies to destination address incrementing
 *
 * @param DataLength number of unit32_t to be copied
 */
HAL_StatusTypeDef memCopy32(DMA_HandleTypeDef * memtomemDMA, uint32_t * SrcAddress, uint32_t * DstAddress, uint32_t DataLength){

	memtomemDMA->Init.PeriphInc = DMA_PINC_ENABLE;
	if (HAL_DMA_Init(memtomemDMA) != HAL_OK) {
		Error_Handler();
	}
	//SET_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(memtomemDMA, (uint32_t)SrcAddress, (uint32_t)DstAddress, DataLength);
}

/**
 * @brief sets 4byte aligned data
 *
 * @param value value to be written
 *
 * @param DstAddress destination address incrementing
 *
 * @param DataLength number of unit32_t to be written
 */
HAL_StatusTypeDef memSet32(DMA_HandleTypeDef * memtomemDMA, uint32_t value, uint32_t * DstAddress, uint32_t DataLength){
	static volatile uint32_t setVal = 0;
	setVal = value;
	memtomemDMA->Init.PeriphInc = DMA_PINC_DISABLE;
	if (HAL_DMA_Init(memtomemDMA) != HAL_OK) {
		Error_Handler();
	}
	//CLEAR_BIT(vgaCircularDMA.Instance->CR, DMA_MINC_ENABLE);
	return HAL_DMA_Start_IT(memtomemDMA, (uint32_t)&setVal, (uint32_t)DstAddress, DataLength);
}

uint32_t roundUp(uint32_t numberToRound, uint32_t multipel){
	if(multipel == 0) return numberToRound;

	uint32_t reminder = numberToRound % multipel;
	if(reminder == 0) return numberToRound;

	return numberToRound + multipel - reminder;
}

uint32_t roundDown(uint32_t numberToRound, uint32_t multipel){
	if(multipel == 0) return numberToRound;

	uint32_t reminder = numberToRound % multipel;
	if(reminder == 0) return numberToRound;

	return numberToRound - reminder;
}

typedef struct splitPointersResult{
	uint8_t * beginHead;
	uint8_t * endHead;
	uint32_t * beginMain;
	uint32_t * endMain;
	uint8_t * beginTail;
	uint8_t * endTail;
} splitPointersResult;

splitPointersResult splitPointers(uint8_t * begin, uint8_t * end, const uint32_t size){
	splitPointersResult ret;
	static_assert(sizeof(uint32_t) == sizeof(void*));

	ret.beginHead = begin;
	ret.endHead = (uint8_t*)roundUp((uint32_t)begin, size);
	ret.beginMain = (uint32_t*)ret.endHead;

	ret.endTail = end;
	ret.beginTail = (uint8_t*)roundDown((uint32_t)end, size);
	ret.endMain = (uint32_t*) ret.beginTail;
	return ret;
}

HAL_StatusTypeDef memCopy(DMA_HandleTypeDef * memtomemDMA, void * SrcAddress, void * DstAddress, uint32_t DataLength){
	uint32_t srcAlignment;
	uint32_t dstAlignment;
	switch ((uint32_t)SrcAddress % 4) {
	case 0: {
		srcAlignment = sizeof(uint32_t);
		memtomemDMA->Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
		break;
	}
	case 2: {
		srcAlignment = sizeof(uint16_t);
		memtomemDMA->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
		break;
	}
	case 1:
	case 3: {
		srcAlignment = sizeof(uint8_t);
		memtomemDMA->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		break;
	}
	}
	switch ((uint32_t)DstAddress % 4) {
	case 0: {
		dstAlignment = sizeof(uint32_t);
		memtomemDMA->Init.MemDataAlignment = DMA_PDATAALIGN_WORD;
		break;
	}
	case 2: {
		dstAlignment = sizeof(uint16_t);
		memtomemDMA->Init.MemDataAlignment = DMA_PDATAALIGN_HALFWORD;
		break;
	}
	case 1:
	case 3: {
		dstAlignment = sizeof(uint8_t);
		memtomemDMA->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		break;
	}
	}
	splitPointersResult src = splitPointers(SrcAddress, SrcAddress + DataLength, srcAlignment);
	splitPointersResult dst = splitPointers(DstAddress, DstAddress + DataLength, dstAlignment);

	return memCopy32(memtomemDMA, src.beginMain, dst.beginMain, (uint8_t*)src.endMain - (uint8_t*)src.beginMain);
}

HAL_StatusTypeDef memSet(DMA_HandleTypeDef * memtomemDMA, uint32_t value, void * DstAddress, uint32_t DataLength){
	//HAL_DMAEx_MultiBufferStart_IT(hdma, SrcAddress, DstAddress, SecondMemAddress, DataLength);
	//HAL_DMAEx_ChangeMemory(hdma, Address, memory);

	return memSet32(memtomemDMA, value, DstAddress, DataLength);
}



