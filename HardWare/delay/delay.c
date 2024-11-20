/*
 * This file is part of the Arducam SPI Camera project.
 *
 * Copyright 2021 Arducam Technology co., Ltd. All Rights Reserved.
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 */
#include "delay.h"
#include "intrinsics.h"

void delayUs(unsigned long us)
{
	while(us > 0)
	{
		__delay_cycles(16); // MCLK = 16 MHz
		us--;
	}
}
void delayMs(unsigned long ms)
{
    while(ms > 0)
    {
    	__delay_cycles(16000);
    	ms--;
    }
}
