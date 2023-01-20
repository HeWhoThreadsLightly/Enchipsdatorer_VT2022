/*
 * RetiredCode.cpp
 *
 *  Created on: May 24, 2022
 *      Author: Orion
 */

#include "RetiredCode.h"
#include "main.h"
#include "memFunc.h"
#include "vga.h"

void dumpLine(){
	char str[81] = {'0'};
	int str_len = 0;

	for(int i = 0; i < 40; i++){
		//int tmp = screenBuff[i].value;
		//str_len = sprintf(str, "%02x ", tmp);
		//HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	}
	str_len = sprintf(str, "\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}

int printStatus(HAL_StatusTypeDef status){
	char str[81] = {'0'};
	int str_len = 0;
	/*
	typedef enum
	{
	  HAL_OK       = 0x00U,
	  HAL_ERROR    = 0x01U,
	  HAL_BUSY     = 0x02U,
	  HAL_TIMEOUT  = 0x03U
	} HAL_StatusTypeDef;//*/
	switch(status){
	case HAL_OK:str_len = sprintf(str, "HAL_OK\r\n");break;
	case HAL_ERROR:str_len = sprintf(str, "HAL_ERROR\r\n");break;
	case HAL_BUSY:str_len = sprintf(str, "HAL_BUSY\r\n");break;
	case HAL_TIMEOUT:str_len = sprintf(str, "HAL_TIMEOUT\r\n");break;
	default:str_len = sprintf(str, "HAL_Unknown\r\n");break;
	}
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	return status != HAL_OK;

}

int printState(HAL_DMA_StateTypeDef state){
	char str[81] = {'0'};
	int str_len = 0;
	//HAL_DMA_STATE_RESET             = 0x00U,  /*!< DMA not yet initialized or disabled */
	//HAL_DMA_STATE_READY             = 0x01U,  /*!< DMA initialized and ready for use   */
	//HAL_DMA_STATE_BUSY              = 0x02U,  /*!< DMA process is ongoing              */
	//HAL_DMA_STATE_TIMEOUT           = 0x03U,  /*!< DMA timeout state                   */
	//HAL_DMA_STATE_ERROR             = 0x04U,  /*!< DMA error state                     */
	//HAL_DMA_STATE_ABORT             = 0x05U,  /*!< DMA Abort state                     */
	//}HAL_DMA_StateTypeDef;
	switch(state){
	case HAL_DMA_STATE_RESET:str_len = sprintf(str, "DMA not yet initialized or disabled\r\n");break;
	case HAL_DMA_STATE_READY:str_len = sprintf(str, "DMA initialized and ready for use\r\n");break;
	case HAL_DMA_STATE_BUSY:str_len = sprintf(str, "DMA process is ongoing\r\n");break;
	case HAL_DMA_STATE_TIMEOUT:str_len = sprintf(str, "DMA timeout state\r\n");break;
	case HAL_DMA_STATE_ERROR:str_len = sprintf(str, "DMA error state\r\n");break;
	case HAL_DMA_STATE_ABORT:str_len = sprintf(str, "DMA Abort state\r\n");break;
	default:str_len = sprintf(str, "DMA_Unknown\r\n");break;

	}
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	return state != HAL_DMA_STATE_READY;
}

/*
void memFunDebug(){
	char str[81] = {'0'};
	int str_len = 0;

	dumpLine();
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	//*
	str_len = sprintf(str, "\r\n\r\nTesting Memset\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	printStatus(old_memSet(0, (uint32_t*)&screenBuff[0], 2));//testingMemset
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

	str_len = sprintf(str, "\r\n\r\nTesting Memcopy\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	printStatus(old_memCopy((uint32_t*)&screenBuff[8], (uint32_t*)&screenBuff[0], 2));//testingMemcopy
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

	str_len = sprintf(str, "\r\n\r\nTesting Memset\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	printStatus(old_memSet(0xff, (uint32_t*)&screenBuff[0], 2));//testingMemset
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

}
//*/
