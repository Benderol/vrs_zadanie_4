/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);


/* Space for your global variables. */

#define STATUS_PERIOD_MS     2000
#define MAX_STRING_LENGTH    35

volatile uint32_t g_ms = 0;

letter_count_ letterCount;

static char rxString[MAX_STRING_LENGTH + 1];
static uint8_t rxLength = 0;
static uint8_t collecting = 0;
static volatile uint8_t stringReady = 0;

static char txMessage[128];


int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* Space for your local variables, callback registration ...*/

  uint32_t lastMs = 0;
  int used;
  int load;

  USART2_RegisterCallback(proccesDmaData);
  SysTick_Config(SystemCoreClock / 1000);

  while (1)
  {
	  /* Periodic transmission of information about DMA Rx buffer state.
	   * Transmission frequency - 0.5Hz.
	   * Message format - "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	   * Example message (what I wish to see in terminal) - Buffer capacity: 1000 bytes, occupied memory: 231 bytes, load [in %]: 23.1%
	   */

	  /* Valid text string information transmission.
	   * Transmission frequency - when new valid string is received.
	   * Message format - "Valid string: %s, lower-case: %d, upper-case: %d"
	   * Example message (what I wish to see in terminal) - Valid string: Platn15uborZnakov, lower-case: 13, upper-case: 2
	   */

	  if(g_ms - lastMs >= STATUS_PERIOD_MS)
	  {
		  lastMs = g_ms;

		  used = DMA_USART2_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		  load = used * 1000 / DMA_USART2_BUFFER_SIZE;

		  while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7));

		  sprintf(txMessage, "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %%]: %d.%d%%\r\n",
				  DMA_USART2_BUFFER_SIZE, used, load / 10, load % 10);

		  USART2_PutBuffer((uint8_t *)txMessage, strlen(txMessage));
	  }

	  if(stringReady)
	  {
		  stringReady = 0;

		  while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7));

		  sprintf(txMessage, "Valid string: %s, lower-case: %d, upper-case: %d\r\n",
				  rxString, letterCount.small_letter, letterCount.capital_letter);

		  USART2_PutBuffer((uint8_t *)txMessage, strlen(txMessage));
	  }
  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign)
{
	/* Process received data */

	int i;

	if(sign == '#')
	{
		collecting = 1;
		rxLength = 0;
		return;
	}

	if(collecting == 0)
	{
		return;
	}

	if(sign == '$')
	{
		rxString[rxLength] = 0;

		letterCount.small_letter = 0;
		letterCount.capital_letter = 0;

		for(i = 0; i < rxLength; i++)
		{
			if(rxString[i] >= 'a' && rxString[i] <= 'z') letterCount.small_letter++;
			if(rxString[i] >= 'A' && rxString[i] <= 'Z') letterCount.capital_letter++;
		}

		collecting = 0;
		stringReady = 1;
		return;
	}

	if(rxLength >= MAX_STRING_LENGTH)
	{
		collecting = 0;
		return;
	}

	rxString[rxLength] = sign;
	rxLength++;
}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
