//Nguyen Ngo
//Surinder Singh

//Application main

// Standard includes
#include <string.h>
#include <stdint.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"
#include "pin_mux_config.h"
#include "gpio.h"
#include "gpio_if.h"

// Includes Adafruit
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

// Includes test
#include "test.h"

#define APPLICATION_VERSION     "1.1.1"
#define UART_PRINT              Report

//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      1

#define SPI_IF_BIT_RATE  100000
#define TR_BUFF_SIZE     100

#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
//static unsigned char g_ucTxBuff[TR_BUFF_SIZE];
//static unsigned char g_ucRxBuff[TR_BUFF_SIZE];
//static unsigned char ucTxBuffNdx;
//static unsigned char ucRxBuffNdx;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

signed char getMagnitude(unsigned char *pucDataBuf){
	//Since the library function uses unsigned char for the tilt value,
	//We need to convert it to signed char in order to determine if the board is tilted left, right, up, down.
	//Change unsinged char to signed char
	return (signed char)pucDataBuf[0];

}

int getDelta(signed char Magnitude){
	//this function calculates the change in coordinates for the ball based on the tilt value (Magnitude)
	float max = 64; //value to divide the tilt by. can be any arbitrary value.
	//get delta by dividing tilt value by 64
	float delta = ((float)Magnitude) / max;
	//multiply delta by 10 so that the ball will move faster
	delta =delta*10;
	return (int) delta;
}


//*****************************************************************************
//
//! Main function for spi demo application
//!
//! \param none
//!
//! \return None.
//
//*****************************************************************************
void main()
{
	//Initilize launchpad
    BoardInit();

    // Muxing UART and SPI lines. Uses GPIO pin 18 and 53
    PinMuxConfig();

    // Reset SPI
	MAP_SPIReset(GSPI_BASE);

	//Enables the transmit and/or receive FIFOs.
	//Base address is GSPI_BASE, SPI_TX_FIFO || SPI_RX_FIFO are the FIFOs to be enabled
	MAP_SPIFIFOEnable(GSPI_BASE, SPI_TX_FIFO || SPI_RX_FIFO);

    // Configure SPI interface
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_MASTER,SPI_SUB_MODE_0,
                     (SPI_SW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVELOW |
                     SPI_WL_8));

    // Enable SPI for communication
    MAP_SPIEnable(GSPI_BASE);

    // Initialize Adafruit
    Adafruit_Init();

    // Initialising the Terminal.
    InitTerm();
    //Enables and configures the I2C peripheral
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    // Clearing the Terminal.
    ClearTerm();

    // Display the Banner
    Message("\n\n\n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\t\t        CC3200 SPI OLED Application  \n\r");
    Message("\t\t   ********************************************\n\r");
    Message("\n\n\n\r");

    //x axis is in register 0x3
    unsigned char ucRegOffsetX = 0x3;
    //y axis is in register 0x5
    unsigned char ucRegOffsetY = 0x5;
    unsigned char aucRdDataBufX[256];
    unsigned char aucRdDataBufY[256];
    //device address is 0x18
    unsigned char ucDevAddr = 0x18;

    //initialize ball to the middle of OLED display
    int x = 64;
    int y = 64;
    fillScreen(BLACK);
    while(1){
    	//draw the ball at location x and y. This function is defined in test.c
    	drawBall(x,y);
    	//Invokes the I2C driver APIs to write to the specified address
    	//Writes to address 0x18 the x register(0x3). length of data is 1 and stop bit is 0
    	I2C_IF_Write(ucDevAddr,&ucRegOffsetX,1,0);
    	// Invokes the I2C driver APIs to read from the device.
    	//Read from device address 0x18 and stores the read data to the x array buffer.
    	//length of data to be read is 1
    	//This function stores the x tilt angle int aucRdDataBufX[256]
    	I2C_IF_Read(ucDevAddr, &aucRdDataBufX[0], 1);
    	//Invokes the I2C driver APIs to write to the specified address
    	//Writes to address 0x18 the y register(0x5). length of data is 1 and stop bit is 0
    	I2C_IF_Write(ucDevAddr,&ucRegOffsetY,1,0);
    	//Read from device address 0x18 and stores the read data to the y array buffer.
    	//length of data to be read is 1
    	//This function stores the y tilt angle int aucRdDataBufY[256]
    	I2C_IF_Read(ucDevAddr, &aucRdDataBufY[0], 1);
    	//Calculates the change in x for the ball. The bigger the tilt value, the higher the change in x for the ball.
    	//getDelta() is a user defined function
    	int dx = getDelta((signed char) aucRdDataBufX[0]);
    	//Calculates the change in Y for the ball. The bigger the tilt value, the higher the change in Y for the ball.
    	//getDelta() is a user defined function
    	int dy = getDelta((signed char) aucRdDataBufY[0]);
    	//stores the old x and y coordinate so that the ball can be erased later.
    	unsigned char oldx = x;
    	unsigned char oldy = y;
    	//Updates the new x coordinate of the ball by adding the old x coordinate with dx
    	x = x + dx;
    	//if the new coordinate goes above pixel 123, then it is out of bound and so we need to set the x coordinate to 123.
    	//Use value 123 because the ball radius is 4 as defined in test.h
    	//123+ 4 = 127. The whole ball can be drawn on the screen
    	//Similary, if the new coordinate goes belove 4 then it is out of bound and so we need to set the new x coordiante to 4
    	if(x > 123)
    		x = 123;
    	if(x < 4)
    		x = 4;

    	//similar logic as above when the ball goes out of bound for the y axis.
    	y = y+ dy;
    	if(y >123)
    		y = 123;
    	if(y < 4)
    		y = 4;
    	//Erase the ball by coloring it black using the old x and y axis.
    	eraseBall(oldx, oldy);
    	//The OLED crashes when there is no delay between erasing the ball and drawing the new ball
    	delay(2);
    }
}