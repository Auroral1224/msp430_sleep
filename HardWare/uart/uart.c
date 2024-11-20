/*
 * This file is part of the Arducam SPI Camera project.
 *
 * Copyright 2021 Arducam Technology co., Ltd. All Rights Reserved.
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 */
#include "uart.h"
#include <msp430.h>

/* for printf using UART. */

int fputc(int ch, FILE *f)
{
//	EUSCI_A_UART_transmitData(EUSCI_A0_BASE, ch & 0xff);
	//If interrupts are not used, poll for flags

	if (!(UCA0IE & UCTXIE))
	{
		//Poll for transmit interrupt flag
		while (!(UCA0IFG & UCTXIFG))
			;
	}
	UCA0TXBUF = ch;
	return ch;
}

int fputs(const char *_ptr, register FILE *_fp)
{
	unsigned int len = strlen(_ptr);

	while (len > 0)
	{
		if (!(UCA0IE & UCTXIE))
		{
			//Poll for transmit interrupt flag
			while (!(UCA0IFG & UCTXIFG))
				;
		}
		UCA0TXBUF = (*_ptr);
		_ptr++;
		len--;
	}

	return strlen(_ptr);
}

/* Initialize UART. */
void initUART(void)
{
	/*
	 Baud Rate calculation
	 https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html

	 clockPrescalar = UCA3BRW

	 firstModReg = UCBRF_x

	 secondModReg = UCBRS_x = 0x??00 (HEX)

	 overSampling = UCOS16 = 1
	 */

	/* Configure eUSCI_A0 for UART mode. */
	UCA0CTLW0 = UCSWRST;                    // Put eUSCI in reset
	UCA0CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK

	/* 115200 bps. */
	UCA0BRW = 8;
	UCA0MCTLW |= UCOS16 | UCBRF_10 | 0xF700;
	UCA0CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
	UCA0IE |= UCRXIE;                       // Enable USCI_A3 RX interrupt
}

/* for 96*96 YUYV B&W format, other commands are deleted for minimize memory usage. */

extern uint8_t YUVarr[];

void framWriteBuffer(uint8_t *fram, uint8_t *buff, uint32_t length)
{
	while (length > 0)
	{
		*(fram) = *(buff);
		fram++;
		buff += 2;
		length -= 2;
	}
	return;
}

void uartTestWriteBuffer(uint8_t *buff, uint32_t length)
{
	while (length > 0)
	{
		if (!(UCA0IE & UCTXIE))
		{
			//Poll for transmit interrupt flag
			while (!(UCA0IFG & UCTXIFG))
				;
		}
		UCA0TXBUF = (*buff);
		buff += 2;
		length -= 2;
	}
	return;
}

void cameraGetPicture(ArducamCamera *camera)
{
	uint8_t *fram_addr = YUVarr;
	uint8_t buff[READ_IMAGE_LENGTH] = { 0 };
	uint8_t rtLength = 0;

	while (camera->receivedLength)
	{
		rtLength = readBuff(camera, buff, READ_IMAGE_LENGTH);
		framWriteBuffer(fram_addr, buff, rtLength);
		uartTestWriteBuffer(buff, rtLength);
		fram_addr += rtLength / 2;
	}
}

