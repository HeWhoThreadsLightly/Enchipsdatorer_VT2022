/*
 * dmaDebug.c
 *
 *  Created on: May 24, 2022
 *      Author: Orion
 */

#include "dmaDebug.h"
#include "main.h"
#include <stdio.h>

UART_HandleTypeDef * huartD;
char str_d[81] = { '\0' };
int str_len_d = 0;
DMA_HandleTypeDef * toDebug_m;
//HAL_DMA_XFER_CPLT_CB_ID         = 0x00U,  /*!< Full transfer     */
void vga_DMA_XFER_CPLT_CB_ID(){
	str_len_d = sprintf(str_d, "Full transfer\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}

//HAL_DMA_XFER_HALFCPLT_CB_ID     = 0x01U,  /*!< Half Transfer     */
void vga_DMA_XFER_HALFCPLT_CB_ID(){
	str_len_d = sprintf(str_d, "Half Transfer\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}

//HAL_DMA_XFER_M1CPLT_CB_ID       = 0x02U,  /*!< M1 Full Transfer  */
void vga_DMA_XFER_M1CPLT_CB_ID(){
	str_len_d = sprintf(str_d, "M1 Full Transfer\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}

//HAL_DMA_XFER_M1HALFCPLT_CB_ID   = 0x03U,  /*!< M1 Half Transfer  */
void vga_DMA_XFER_M1HALFCPLT_CB_ID(){
	str_len_d = sprintf(str_d, "M1 Half Transfer\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}

//HAL_DMA_XFER_ERROR_CB_ID        = 0x04U,  /*!< Error             */
void vga_DMA_XFER_ERROR_CB_ID(){
	char * err = "Default";
	uint32_t errorCode = HAL_DMA_GetError(toDebug_m);
	if(errorCode == HAL_DMA_ERROR_NONE){
		str_len_d = sprintf(str_d, "DMA %s\r\n", "No error");
		HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
	}
	while(errorCode){
		switch(errorCode){
		case HAL_DMA_ERROR_NONE            : err = "No error";break;    /*!< No error                               */
		case HAL_DMA_ERROR_TE              : err = "Transfer error";break;    /*!< Transfer error                         */
		case HAL_DMA_ERROR_FE              : err = "FIFO error";break;    /*!< FIFO error                             */
		case HAL_DMA_ERROR_DME             : err = "Direct Mode error";break;    /*!< Direct Mode error                      */
		case HAL_DMA_ERROR_TIMEOUT         : err = "Timeout error";break;    /*!< Timeout error                          */
		case HAL_DMA_ERROR_PARAM           : err = "Parameter error";break;    /*!< Parameter error                        */
		case HAL_DMA_ERROR_NO_XFER         : err = "Abort requested with no Xfer ongoing";break;    /*!< Abort requested with no Xfer ongoing   */
		case HAL_DMA_ERROR_NOT_SUPPORTED   : err = "Not supported mode";break;    /*!< Not supported mode                     */
		}

		str_len_d = sprintf(str_d, "DMA %s\r\n", err);
		HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
		errorCode &= errorCode - 1;
	};

}
//HAL_DMA_XFER_ABORT_CB_ID        = 0x05U,  /*!< Abort             */
void vga_DMA_XFER_ABORT_CB_ID(){
	str_len_d = sprintf(str_d, "DMA Abort\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}
//HAL_DMA_XFER_ALL_CB_ID          = 0x06U   /*!< All               */
void vga_DMA_XFER_ALL_CB_ID(){
	str_len_d = sprintf(str_d, "Full transfstrer\r\n");
	HAL_UART_Transmit(huartD, (uint8_t*) str_d, str_len_d, HAL_MAX_DELAY);
}

void registerDebugInterupts(DMA_HandleTypeDef * toDebug){
	toDebug_m = toDebug;
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_CPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_HALFCPLT_CB_ID, vga_DMA_XFER_HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_M1CPLT_CB_ID, vga_DMA_XFER_M1CPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_M1HALFCPLT_CB_ID, vga_DMA_XFER_M1HALFCPLT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ERROR_CB_ID, vga_DMA_XFER_ERROR_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ABORT_CB_ID, vga_DMA_XFER_ABORT_CB_ID);
	HAL_DMA_RegisterCallback(toDebug, HAL_DMA_XFER_ALL_CB_ID, vga_DMA_XFER_ALL_CB_ID);
}

void registerHUARTdmaDebug(UART_HandleTypeDef * huart){
	huartD = huart;
}
