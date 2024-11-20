/* TI includes. */
#include <msp430.h>
#include <stdint.h>
/* ------------ */

/* maix model includes. */
#include "stdio.h"
#include "stdlib.h"
#include "tinymaix.h"

#pragma DATA_SECTION (mdl_data, ".NNmodelData")
#include "model_info.h"
//#include "model.h"
//#include "left_2.0.h"
#include "0305.h"
#include "errno.h"
/* -------------------- */

/* Definition of picture mode and size. */
#define IMAGE_HEIGHT 320
#define IMAGE_WIDTH  240
#define READ_IMAGE_LENGTH  320 // buffer size of readbuff operation
#define SUBARRAY_HEIGHT 160
#define SUBARRAY_WIDTH  80
#define CURRENT_PICTURE_MODE 0x31 // YUYV, 160x120
/* ------------------------------------ */

/* The fram section, for saving picture and load model. */
#pragma PERSISTENT (YUVarr)
uint8_t YUVarr[SUBARRAY_HEIGHT * SUBARRAY_WIDTH] = { 0 };
//#pragma PERSISTENT (LOAD_BUF)
#pragma DATA_SECTION (LOAD_BUF, ".NNmodelBUF")
uint8_t LOAD_BUF[MDL_BUF_LEN] = { 0 };
#include <testpic_1.h>
/* ---------------------------------------------------- */

/* ESP8266 return string buffer. */
#define BUFFER_SIZE 64
#define END_MARKER '\n' // each string ends with it

volatile char rxBuffer[BUFFER_SIZE];
volatile uint8_t rxIndex = 0;
volatile uint8_t rx_OK = 0;
/* ----------------------------- */

/* maix model config. */
static int class_num = PARAM_CLASS_NUM;                          // class number
static const char *labels[] = PARAM_LABELS;                       // class names
//		static int input_w = PARAM_INPUT_W, input_h = PARAM_INPUT_H;               // input resolution
//extern unsigned char pic[PARAM_INPUT_W * PARAM_INPUT_H * PARAM_INPUT_C]; // test input image, hwc,
//#define PREPROCESS_USE_NORMAL_PROCEDURE 0               				   // 0: use fast preprocess, 1: use normal
/* ------------------ */

/* WiFi config. */
#define WIFI_NETWORK_SSID "NAME_OF_WIFI"
#define WIFI_NETWORK_PASS "PASS_OF_WIFI"
#define TCP_SERVER "192.168.18.20" // ipconfig
#define TCP_PORT "2000"
/* ------------ */

/* The prototype of board initialize function. */
void initGPIO(void);
void initClockTo16MHz(void);
void initUART(void);
void initSPI(void);
void initTimer(void); // for maix benchmark
/* ------------------------------------------- */

/* The prototypes of the initialization and connection function of ESP8266. */
void initESPRuntimeConfig(void);
void initESPWiFi(void);
void tcpConnect(void);
/* ------------------------------------------------------------------------ */

/* To let printf via UART(eUSCI A0). */
int fputc(int ch, FILE *f);
int fputs(const char *_ptr, register FILE *_fp);
/* --------------------------------- */

/* The prototypes of delay functions. */
void delayUs(unsigned long us);
void delayMs(unsigned long ms);
void delaySecond(float s);
/* ---------------------------------- */

/* The prototypes of WiFi transmission functions. */
void tx(char *str, uint16_t len);
void send_measurement(uint8_t *data, uint16_t length);
/* ---------------------------------------------- */

/* The prototypes of camera related functions. */
void cameraBegin(void);
unsigned char spiWriteRead(unsigned char val);
void writeReg(uint8_t addr, uint8_t val);
uint8_t readReg(uint8_t addr);
void waitI2cIdle(void);
uint8_t getBit(uint8_t addr, uint8_t bit);
uint32_t readFifoLength(void);
uint32_t readBuff(uint32_t *receivedLength, uint8_t *burstFirstFlag,
				  uint8_t *buff, uint32_t length);
void takePicture(uint8_t cameraFormat, uint8_t cameraResolution,
				 uint32_t *receivedLength, uint8_t *burstFirstFlag,
				 uint32_t delaySec);
void cameraGetPicture(uint32_t *receivedLength, uint8_t *burstFirstFlag);
/* ------------------------------------------- */

/* The prototypes of maix model related functions. */
static tm_err_t layer_cb(tm_mdl_t *mdl, tml_head_t *lh);
static void parse_output(tm_mat_t *outs, int class_num);
/* ----------------------------------------------- */

int main(void)
{
	/* Stop Watchdog timer. */
	WDTCTL = WDTPW | WDTHOLD;

	/* Initialize hardware. */
	initGPIO();
	initClockTo16MHz();
	initUART();
	initSPI();
	initTimer();
	P5IFG &= ~BIT6; // Clear button (P5.6) interrupt flag
	__enable_interrupt();

	/* Initialize camera. */
	cameraBegin();
	writeReg(0x02, 0X07); // keep camera in LPM

	/* Initialize ESP8266 module and connect to the TCP server. */
	initESPRuntimeConfig();
	printf("Connecting WiFi... (%s)\n", WIFI_NETWORK_SSID);
	initESPWiFi();
	printf("WiFi connected. Connecting TCP... (%s, port = %s)\n", TCP_SERVER, TCP_PORT);
	tcpConnect();
	printf("TCP connected. Loading model...\n");

	/* Initialize model. */

	TM_DBGT_INIT();
	TM_PRINTF("Project sleep\n");

	tm_mdl_t mdl;
//	tm_mat_t in_uint8 = { 1, 160, 80, 1, { ((mtype_t*) testpic) } };
    tm_mat_t in_uint8 = {1,80,40,1,{((mtype_t*)YUVarr)}};
	tm_err_t res;
	tm_mat_t in = { 1, 160, 80, 1, { NULL } };
	tm_mat_t outs[1];

	tm_stat((tm_mdlbin_t*) mdl_data);
	res = tm_load(&mdl, mdl_data, LOAD_BUF, layer_cb, &in);
	if (res != TM_OK)
	{
		TM_PRINTF("tm model load err %d\n", res);
		return -1;
	}

	while (1)
	{

		P1OUT |= BIT0;  // Enable P1.0 LED
		LPM4;
		__no_operation();

//		camera low power off
		writeReg(0x02, 0X05);
		P1OUT |= BIT1; // Open P1.1 LED

		const uint8_t CameraResolution = CURRENT_PICTURE_MODE & 0x0f; // Set picture resolution (0x3A, 3:YUYV format, A: 96*96)
		const uint8_t CameraFarmat = (CURRENT_PICTURE_MODE & 0x70) >> 4;
		uint32_t receivedLength = 0;
		uint8_t burstFirstFlag = 0;

		takePicture(CameraFarmat, CameraResolution, &receivedLength, &burstFirstFlag, 5000);

		P1OUT &= ~BIT0;  // Disable P1.0 LED
//		camera low power on
		writeReg(0x02, 0X07);

		cameraGetPicture(&receivedLength, &burstFirstFlag);

		/* Transmit the picture to server via TCP. */
		// Debug
		uint8_t *ptr = YUVarr;

		send_measurement(ptr, SUBARRAY_HEIGHT * SUBARRAY_WIDTH);

		/* Inference start. */

		res = tm_preprocess(&mdl, TMPP_UINT2INT, &in_uint8, &in);

		TM_DBGT_START();
		res = tm_run(&mdl, &in, outs);
		TM_DBGT("tm_run");
		if (res == TM_OK)
			parse_output(outs, class_num);
		else
			TM_PRINTF("tm run error: %d\n", res);

		/* ---------------- */

		P1OUT &= ~BIT1; // Turn off P1.1 LED
	}

	tm_unload(&mdl);
	return 0;
}

/* Board initialize function. */
void initGPIO(void)
{
	/* Set all GPIO pins to output and low. */
	P1DIR = 0xFF;
	P1OUT = 0x00;
	P2DIR = 0xFF;
	P2OUT = 0x00;
	P3DIR = 0xFF;
	P3OUT = 0x00;
	P4DIR = 0xFF;
	P4OUT = 0x00;
	P5DIR = 0xFF;
	P5OUT = 0x00;
	P6DIR = 0xFF;
	P6OUT = 0x00;
	P7DIR = 0xFF;
	P7OUT = 0x00;
	P8DIR = 0xFF;
	P8OUT = 0x00;
	P9DIR = 0xFF;
	P9OUT = 0x00;
	PADIR = 0xFF;
	PAOUT = 0x00;
	PBDIR = 0xFF;
	PBOUT = 0x00;
	PCDIR = 0xFF;
	PCOUT = 0x00;
	PDDIR = 0xFF;
	PDOUT = 0x00;
	PEDIR = 0xFF;
	PEOUT = 0x00;
	PJDIR = 0xFF;
	PJOUT = 0x00;

	/* Configure eUSCI_A0 UART operation. */
	P2SEL1 |= (BIT0 | BIT1);
	P2SEL0 &= ~(BIT0 | BIT1);

	/* Configure eUSCI_A3 UART operation. */
	P6SEL0 |= (BIT0 | BIT1);
	P6SEL1 &= ~(BIT0 | BIT1);

	/* Configure SPI and CS GPIO. */
	P5SEL1 &= ~(BIT0 | BIT1 | BIT2);        // USCI_B1 SCLK, MOSI,
	P5SEL0 |= (BIT0 | BIT1 | BIT2);         // and MISO pin (primary_module)
	P5DIR |= BIT3;                          // CS P5.3

	/* Configure LEDs. */
	P1DIR |= BIT0 | BIT1;
	P1OUT &= ~(BIT0 | BIT1);

	/* Configure buttons. */
	P5DIR &= ~(BIT6);                         // Set P5.6 to input direction
	P5REN |= BIT6;                            // Select pull-up mode for P5.6
	P5OUT |= BIT6;                            // Pull-up resistor on P5.6
	P5IES |= BIT6;                            // P5.6 Hi to LO edge
	P5IE |= BIT6;                             // P5.6 interrupt enabled

	/* Disable the GPIO power-on default high-impedance mode. */
	PM5CTL0 &= ~LOCKLPM5;
}

void initClockTo16MHz(void) // SMCLK = MCLK = DCO = 16MHz, ACLK = VLOCLK = 10KHz
{
	// Configure one FRAM waitstate as required by the device datasheet for MCLK
	// operation beyond 8MHz _before_ configuring the clock system.
	FRCTL0 = FRCTLPW | NWAITS_1;

	// Clock System Setup
	CSCTL0_H = CSKEY_H;                     // Unlock CS registers
	CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz

	// Set SMCLK = MCLK = DCO, ACLK = VLOCLK
	CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

	// Per Device Errata set divider to 4 before changing frequency to
	// prevent out of spec operation from overshoot transient
	CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4; // Set all corresponding clk sources to divide by 4 for errata
	CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz

	// Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
	__delay_cycles(60);
	CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; // Set all dividers to 1 for 16MHz operation
	CSCTL0_H = 0;                           // Lock CS registers
}

void initUART(void)
{
	/*
	 Baud Rate calculation
	 https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html

	 clockPrescalar = UCAxBRW

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
//	UCA0IE |= UCRXIE;                       // Enable eUSCI_A0 RX interrupt

	/* Configure eUSCI_A3 for UART mode. */
	UCA3CTLW0 = UCSWRST;                    // Put eUSCI in reset
	UCA3CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK

//	/* 115200 bps. */
//	UCA3BRW = 8;
//	UCA3MCTLW |= UCOS16 | UCBRF_10 | 0xF700;
//	UCA3CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
//	UCA3IE |= UCRXIE;                   // Enable eUSCI_A3 RX interrupt for echo

	/* 9600 bps. */
	UCA3BRW = 104;
	UCA3MCTLW |= UCOS16 | UCBRF_2 | 0xB600;
	UCA3CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
	UCA3IE |= UCRXIE;                   // Enable eUSCI_A3 RX interrupt for echo
}

void initSPI(void)
{
	// Configure USCI_B1 for SPI operation
	UCB1CTLW0 = UCSWRST;                    // **Put state machine in reset**
	//Clock Polarity: The inactive state is high
	//MSB First, 8-bit, Master, 3-pin mode, Synchronous, SMCLK
	UCB1CTLW0 |= UCCKPH | UCMSB | UCSYNC | UCMST | UCSSEL__SMCLK;
	UCB1BRW = 0x20;                         // /2
	UCB1CTLW0 &= ~UCSWRST;                  // **Initialize USCI state machine**
	P5OUT &= ~BIT3;                         // Set output low to CS pin
}

void initTimer()
{
	TA0CTL = TASSEL__ACLK | MC__STOP | TACLR;
	TA0CCR0 = 0xFFFF;
	TA0CTL |= TAIE;
}
/* -------------------------- */

/* The initialization and connection function of ESP8266. */
void initESPRuntimeConfig(void)
{
	/* "AT+UART_DEF=9600,8,1,0,0" If you want to change baudrate of ESP8266 */
	/* ESP8266 runtime config. */
	__disable_interrupt(); 	// To prevent module reset data interrupt
	tx("AT+RST\r\n", 0);
	delaySecond(1);
	__enable_interrupt();
	tx("ATE0\r\n", 0);        // Disable echo
	delayMs(100);
	tx("AT+CIPMUX=0\r\n", 0); // Single-Connection Mode
	delayMs(100);
}

void initESPWiFi(void)
{
	/* Flash Wifi-Mode: As client. */
	tx("AT+CWMODE_CUR=1\r\n", 0);
	delayMs(100);

	/* Flash ssid credentials. */
    while (1)
    {
    	tx("AT+CWQAP\r\n", 0); // Close former connection
    	delayMs(10);
		tx("AT+CWJAP_CUR=\"", 0);
		tx(WIFI_NETWORK_SSID, 0);
		tx("\",\"", 0);
		tx(WIFI_NETWORK_PASS, 0);
		tx("\"\r\n", 0);
		delaySecond(4);

		// Wait for 10 seconds, checking rx_OK every 1s
		for (uint8_t i = 0; i < 10; i++)
		{
			delaySecond(1); // Wait 1 second
			if (rx_OK) // If connection is successful
			{
				rx_OK = 0;
				return; // Exit the function
			}
			else
			{
				printf(".");
			}
		}

		printf("retry...\n");
	}
}

void tcpConnect(void)
{
	/* tcp server connect. */
	do
	{
		tx("AT+CIPCLOSE\r\n", 0); // Close former connection
		delayMs(10);
		tx("AT+CIPSTART=\"TCP\",\"", 0); // Establish TCP connection
		tx(TCP_SERVER, 0);
		tx("\",", 0);
		tx(TCP_PORT, 0);
		tx("\r\n", 0);
		delaySecond(2);
	}
	while (!rx_OK);

	tx("AT+CIPMODE=1\r\n", 0); // UART-WiFi Passthrough mode, use "AT+CIPSEND" to enable this feature
	delayMs(100);
}
/* ------------------------------------------------------ */

/* To let printf via UART (eUSCI A0). */
int fputc(int ch, FILE *f)
{
	while (!(UCA0IFG & UCTXIFG))
		;
	UCA0TXBUF = ch & 0xff;
	return ch;
}

int fputs(const char *_ptr, register FILE *_fp)
{
	unsigned int i, len;

	len = strlen(_ptr);

	for (i = 0; i < len; i++)
	{
		while (!(UCA0IFG & UCTXIFG))
			;
		UCA0TXBUF = _ptr[i] & 0xff;
	}
	return len;
}
/* ---------------------------------- */

/* Delay functions. */
void delayUs(unsigned long us)
{
	while (us > 0)
	{
		__delay_cycles(16); // MCLK = 16 MHz
		us--;
	}
}

void delayMs(unsigned long ms)
{
	while (ms > 0)
	{
		__delay_cycles(16000);
		ms--;
	}
}
void delaySecond(float s)
{
	delayMs((unsigned long) (s * 1000));
}
/* ---------------- */

/* WiFi transmission functions. */
void tx(char *str, uint16_t len)
{
	unsigned int i;
	if(len == 0)
	{
		len = strlen(str);
	}
	for (i = 0; i < len; i++)
	{
		while (!(UCA3IFG & UCTXIFG))
			;
		UCA3TXBUF = str[i];
	}
}

void send_measurement(uint8_t *data, uint16_t length)
{
	tx("AT+CIPSEND\r\n", 0); // From now on, data received from UART will be transmitted to the server automatically.
	delayMs(300);

	tx("[PHOTO_START]", 0);
//	delayMs(300);
//    uint16_t sent = 0;
//    while (sent < length) {
//        uint16_t batch_size = (length - sent) > 1024 ? 1024 : (length - sent);
//        tx((char*)(data + sent), batch_size);
//        sent += batch_size;
//        delayMs(300); // batch sent delay
//    }
	tx((char*)(data), length);
	tx("[PHOTO_END]", 0);

	printf("===Debug: picture sent. (sent byte = %d)\r\n", length);
	delayMs(500);

	// When receiving a packet that contains only "+++",
	// the UART-WiFi passthrough transmission process will be stopped.
	// Then please wait at least 1 second before sending next AT command.
	tx("+++", 0);
	delaySecond(1);

//	tx("AT+CIPSEND=7\r\n", 0);
//	delayMs(50);
//	tx("[START]", 7);
//	delayMs(10);
//
//	uint16_t sent = 0;
//	char buffer[20];
//	while (sent < length) {
//		uint16_t batch_size = (length - sent) > 1024 ? 1024 : (length - sent);
//		snprintf(buffer, sizeof(buffer), "AT+CIPSEND=%d\r\n", batch_size);
//		tx(buffer, 0);
//		delayMs(100);
//		tx((char*)(data + sent), batch_size);
//		sent += batch_size;
//		delayMs(500); // batch sent delay
//	}
//
//	tx("AT+CIPSEND=5\r\n", 0);
//	delayMs(50);
//	tx("[END]", 5);
//	delayMs(10);

}
/* ---------------------------- */

/* Camera related functions. */

void cameraBegin(void)
{
// reset cpld and camera
	writeReg(0x07, (1 << 6));
	waitI2cIdle();

// CAM_REG_DEBUG_DEVICE_ADDRESS, deviceAddress
	writeReg(0x0A, 0x78);
	waitI2cIdle();
}

unsigned char spiWriteRead(unsigned char val)
{
	unsigned char rt = 0;

	UCB1TXBUF = val;
	while ((UCB1STAT & UCBUSY))
		;
	rt = UCB1RXBUF;
	while ((UCB1STAT & UCBUSY))
		;

	return rt;
}

void writeReg(uint8_t addr, uint8_t val)
{
// CS pin LO
	P5OUT &= ~BIT3;

	spiWriteRead(addr | 0x80);
	spiWriteRead(val);

// CS pin HI
	P5OUT |= BIT3;

	delayMs(1);
}

uint8_t readReg(uint8_t addr)
{
	uint8_t val;
// CS pin LO
	P5OUT &= ~BIT3;

	spiWriteRead(addr & 0x7F);
	val = spiWriteRead(0x00);
	val = spiWriteRead(0x00);

// CS pin HI
	P5OUT |= BIT3;
	return val;
}

void waitI2cIdle(void)
{
	while ((readReg(0x44) & 0X03) != (1 << 1))
	{
		delayMs(2);
	}
}

uint8_t getBit(uint8_t addr, uint8_t bit)
{
	uint8_t temp;
	temp = readReg(addr);
	temp = temp & bit;
	return temp;
}

uint32_t readFifoLength()
{
	uint32_t len1, len2, len3, length = 0;
	len1 = readReg(0x45);
	len2 = readReg(0x46);
	len3 = readReg(0x47);
	length = ((len3 << 16) | (len2 << 8) | len1) & 0xffffff;
	return length;
}

uint32_t readBuff(uint32_t *receivedLength, uint8_t *burstFirstFlag,
				  uint8_t *buff, uint32_t length)
{
	if ((*receivedLength) == 0 || (length == 0))
	{
		return 0;
	}

	if ((*receivedLength) < length)
	{
		length = (*receivedLength);
	}

// CS pin LO
	P5OUT &= ~BIT3;

//setFifoBurst(camera);
	spiWriteRead(0x3C);

	if ((*burstFirstFlag) == 0)
	{
		(*burstFirstFlag) = 1;
		spiWriteRead(0x00);
	}

	uint32_t count = length;
	while (count > 0)
	{
		uint8_t temp = spiWriteRead(0x00);

		if (count % 2 == 0) // length must be even, we only need the Y (luminance) part (YUYV format)
		{
			(*buff) = temp;
			buff++;
		}
		count--;
	}

	(*receivedLength) -= (length);

// CS pin HI
	P5OUT |= BIT3;
	return length;
}

void takePicture(uint8_t cameraFormat, uint8_t cameraResolution,
				 uint32_t *receivedLength, uint8_t *burstFirstFlag,
				 uint32_t delaySec)
{
	writeReg(0x20, cameraFormat);
	waitI2cIdle(); // Wait I2c Idle
	writeReg(0x21, (0 << 7) | cameraResolution);
	waitI2cIdle(); // Wait I2c Idle

	delayMs(delaySec); //needed

// Clear Fifo Flag
	writeReg(0x04, 0x01);
// start capture
	writeReg(0x04, 0x02);
	while (getBit(0x44, 0x04) == 0)
		;
	(*receivedLength) = readFifoLength();
	(*burstFirstFlag) = 0;
	return;
}

void cameraGetPicture(uint32_t *receivedLength, uint8_t *burstFirstFlag)
{
	uint8_t buff[READ_IMAGE_LENGTH] = { 0 };

// calculate the starting point of cropping
	int startRow = (IMAGE_HEIGHT - SUBARRAY_HEIGHT) / 2;
	int startCol = (IMAGE_WIDTH - SUBARRAY_WIDTH) / 2;
	int index = 0;

// receive the pixels and save the central part
	for (int i = 0; i < IMAGE_HEIGHT; i++)
	{
		readBuff(receivedLength, burstFirstFlag, buff, 2*READ_IMAGE_LENGTH);
		for (int j = 0; j < IMAGE_WIDTH; j++)
		{

			// if current pixel is in the central part, save it.
			if (i >= startRow&& i < startRow + SUBARRAY_HEIGHT &&
			j >= startCol && j < startCol + SUBARRAY_WIDTH)
			{
				YUVarr[index] = buff[j];
				index++;
			}
		}
	}

//	// Debug (print array to UART)
//	for (int var = 0; var < SUBARRAY_HEIGHT * SUBARRAY_WIDTH; ++var)
//	{
//		if (!(UCA0IE & UCTXIE))
//		{
//			//Poll for transmit interrupt flag
//			while (!(UCA0IFG & UCTXIFG))
//				;
//		}
//		UCA0TXBUF = YUVarr[var];
//	}
}
/* ------------------------- */

/* maix model related functions. */
static tm_err_t layer_cb(tm_mdl_t *mdl, tml_head_t *lh)
{   //dump middle result
	uint16_t h = lh->out_dims[1];
	uint16_t w = lh->out_dims[2];
	uint16_t ch = lh->out_dims[3];
	mtype_t *output = TML_GET_OUTPUT(mdl, lh);
	TM_PRINTF("Layer %d callback ========\n", mdl->layer_i);
#if 1
	for (uint16_t y = 0; y < h; y++)
	{
		TM_PRINTF("[");
		for (uint16_t x = 0; x < w; x++)
		{
			TM_PRINTF("[");
			for (uint16_t c = 0; c < ch; c++)
			{
#if TM_MDL_TYPE == TM_MDL_FP32
                TM_PRINTF("%.3f,", output[(y*w+x)*ch+c]);
            #else
				TM_PRINTF("%.3f,", TML_DEQUANT(lh,output[(y*w+x)*ch+c]));
#endif
				if (c > 10)
				{
					TM_PRINTF(" ... ");
					break;
				}
			}
			TM_PRINTF("],\n");
			if (x > 10)
			{
				TM_PRINTF(" ... ");
				break;
			}
		}
		TM_PRINTF("],\n\n");
		if (y > 2)
		{
			TM_PRINTF(" .\n .\n .\n\n");
			break;
		}
	}
	TM_PRINTF("\n");


#endif
	return TM_OK;
}

static void parse_output(tm_mat_t *outs, int class_num)
{
	tx("AT+CIPSEND\r\n", 0); // Re-enable UART-WiFi passthrough mode
	delayMs(300);

	tx("[ANS_START]", 0);
//	delayMs(100);

	tm_mat_t out = outs[0];
	float *data = out.dataf;
	float maxp = -1;
	int maxi = -1;

	// JSON format
	char str_result[50] = {0};
	tx("{\r\n", 0);
	for (int i = 0; i < class_num; i++)
	{
		char comma = i == class_num - 1 ? NULL : ',';
		snprintf(str_result, sizeof(str_result), "\t\"%s\": %.3f%c\r\n", labels[i], data[i], comma);
		tx(str_result, 0);

		printf("\"%s\": %.3f\r\n", labels[i], data[i]);

		if (data[i] > maxp)
		{
			maxi = i;
			maxp = data[i];
		}
	}
	tx("\r\n}", 0);

	TM_PRINTF("### Result: %s, prob %.3f\n", labels[maxi], maxp);
//	delayMs(100);

	tx("[ANS_END]", 0);
	delayMs(300);
	tx("+++", 0); // exit UART-WiFi passthrough mode
	delaySecond(1);
	return;
}
/* ----------------------------- */

/* eUSCI_A3 interrupt service routine for echo */

int tx_ok(const char *cmd)
{
	if (strstr(cmd, "OK") != NULL)
	{
		return 1;
	}
	return 0;
}

#pragma vector=EUSCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
{
	switch (__even_in_range(UCA3IV, USCI_UART_UCTXCPTIFG))
	{
		case USCI_NONE:
			break;
		case USCI_UART_UCRXIFG:
			while (!(UCA3IFG & UCTXIFG))
				;
			if (rxIndex < BUFFER_SIZE - 1)
			{
				char rx = UCA3RXBUF;  // read byte
				rxBuffer[rxIndex++] = rx;

				if (rx == END_MARKER)
				{
					rxBuffer[rxIndex] = '\0';  // add the end sign of string
					rxIndex = 0;  // reset index
					if (strstr(rxBuffer, "OK") != NULL)
						rx_OK = 1;
					else
						rx_OK = 0;
				}
			}
			break;
		case USCI_UART_UCTXIFG:
			break;
		case USCI_UART_UCSTTIFG:
			break;
		case USCI_UART_UCTXCPTIFG:
			break;
		default:
			break;
	}
}
/* ------------------------------------------- */

/* Port 5 interrupt service routine */
#pragma vector = PORT5_VECTOR
__interrupt void port5_isr_handler(void)
{
	switch (__even_in_range(P5IV, P5IV__P5IFG7))
	{
		case P5IV__NONE:
			break;          // Vector  0:  No interrupt
		case P5IV__P5IFG0:
			break;          // Vector  2:  P5.0 interrupt flag
		case P5IV__P5IFG1:
			break;          // Vector  4:  P5.1 interrupt flag
		case P5IV__P5IFG2:
			break;          // Vector  6:  P5.2 interrupt flag
		case P5IV__P5IFG3:
			break;          // Vector  8:  P5.3 interrupt flag
		case P5IV__P5IFG4:
			break;          // Vector  10:  P5.4 interrupt flag
		case P5IV__P5IFG5:
			break;          // Vector  12:  P5.5 interrupt flag
		case P5IV__P5IFG6:  // Vector  14:  P5.6 interrupt flag
			LPM4_EXIT;
			break;
		case P5IV__P5IFG7:
			break;          // Vector  16:  P5.7 interrupt flag
		default:
			break;
	}
}
/* -------------------------------- */
