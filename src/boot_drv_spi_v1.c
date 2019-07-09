/***************************************************************************
 *   Copyright (C) 2005, 2006, 2007, 2008, 2009 by Hilscher GmbH           *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#include <string.h>

#include "boot_drv_spi_v1.h"

#include "netx_io_areas.h"
#include "tools.h"
#include "uprintf.h"


static unsigned char spi_exchange_byte(const SPI_CFG_T *ptCfg __attribute__ ((unused)), unsigned char ucByte)
{
	HOSTDEF(ptSpiArea);
	unsigned long ulValue;


	/* Write a byte to the SPI bus. */
	ptSpiArea->ulSpi_data_register = ucByte | HOSTMSK(spi_data_register_dr_valid0);

	/* Wait until all bytes are clocked out. There will be a byte in the
	 * receive buffer */
	/* TODO: limit the polling time, this will wait forever! On timeout return -1 . */
	do
	{
		ulValue  = ptSpiArea->ulSpi_status_register;
		ulValue &= HOSTMSK(spi_status_register_SR_out_fuel_val);
	} while( ulValue!=0 );

	/* Get the received byte. */
	ucByte = (unsigned char)(ptSpiArea->ulSpi_data_register);
	return ucByte;
}


/*-----------------------------------*/


typedef enum
{
	HAL_SPI_SPEED_0_05MHz   = 1,
	HAL_SPI_SPEED_0_1MHz    = 2,
	HAL_SPI_SPEED_0_2MHz    = 3,
	HAL_SPI_SPEED_0_5MHz    = 4,
	HAL_SPI_SPEED_1_0MHz    = 5,
	HAL_SPI_SPEED_1_25MHz   = 6,
	HAL_SPI_SPEED_2_0MHz    = 7,
	HAL_SPI_SPEED_2_5MHz    = 8,
	HAL_SPI_SPEED_3_3MHz    = 9,
	HAL_SPI_SPEED_5_0MHz    = 10,
	HAL_SPI_SPEED_10_0MHz   = 11,
	HAL_SPI_SPEED_12_5MHz   = 12,
	HAL_SPI_SPEED_16_6MHz   = 13,
	HAL_SPI_SPEED_25_0MHz   = 14,
	HAL_SPI_SPEED_50_0MHz   = 15
} HAL_SPI_SPEED_t;


typedef struct
{
	unsigned long ulSpeed;
	HAL_SPI_SPEED_t tSpiClock;
} SPI_KHZ_TO_CLOCK_T;

/* Speed steps for netX100/500. */
/* NOTE: the speeds *must* be listed in ascending order, i.e. slowest first. */
static const SPI_KHZ_TO_CLOCK_T aulSpeedSteps[] =
{
	{    50, HAL_SPI_SPEED_0_05MHz },
	{   100, HAL_SPI_SPEED_0_1MHz  },
	{   200, HAL_SPI_SPEED_0_2MHz  },
	{   500, HAL_SPI_SPEED_0_5MHz  },
	{  1000, HAL_SPI_SPEED_1_0MHz  },
	{  1250, HAL_SPI_SPEED_1_25MHz },
	{  2000, HAL_SPI_SPEED_2_0MHz  },
	{  2500, HAL_SPI_SPEED_2_5MHz  },
	{  3333, HAL_SPI_SPEED_3_3MHz  },
	{  5000, HAL_SPI_SPEED_5_0MHz  },
	{ 10000, HAL_SPI_SPEED_10_0MHz },
	{ 12500, HAL_SPI_SPEED_12_5MHz },
	{ 16666, HAL_SPI_SPEED_16_6MHz },
	{ 25000, HAL_SPI_SPEED_25_0MHz },
	{ 50000, HAL_SPI_SPEED_50_0MHz }
};

static unsigned long spi_get_device_speed_representation(unsigned int uiSpeedKHz)
{
	unsigned int uiCnt;
	unsigned long ulDevSpeed;
	unsigned long ulMaximumSpeedKhz;


	/* Limit the speed. */
	ulMaximumSpeedKhz = 50000;
	if( uiSpeedKHz>ulMaximumSpeedKhz )
	{
		uiSpeedKHz = ulMaximumSpeedKhz;
	}

	/* Start at the end of the list. There are the largest values. */
	uiCnt = sizeof(aulSpeedSteps)/sizeof(aulSpeedSteps[0]);

	/* Get the minimum matching entry. */
	do
	{
		--uiCnt;
		if( uiSpeedKHz>=aulSpeedSteps[uiCnt].ulSpeed )
		{
			break;
		}
	} while( uiCnt!=0 );

	/* Return the supported speed. */
	ulDevSpeed   = aulSpeedSteps[uiCnt].tSpiClock;
	ulDevSpeed <<= HOSTSRT(spi_control_register_CR_speed);

	return ulDevSpeed;
}



static void spi_slave_select(const SPI_CFG_T *ptCfg, int fIsSelected)
{
	HOSTDEF(ptSpiArea);
	unsigned long ulValue;


	/* Get control register contents. */
	ulValue = ptSpiArea->ulSpi_control_register;

	/* Mask out the slave select bits. */
	ulValue &= ~HOSTMSK(spi_control_register_CR_ss);

	/* Mask in the new slave id. */
	if( fIsSelected!=0 )
	{
		ulValue |= 1U << (HOSTSRT(spi_control_register_CR_ss) + ptCfg->uiChipSelect);
	}

	/* Clear in and out FIFO. */
	ulValue |= HOSTMSK(spi_control_register_CR_clr_infifo)|HOSTMSK(spi_control_register_CR_clr_outfifo);

	/* Write back new value. */
	ptSpiArea->ulSpi_control_register = ulValue;
}



static int spi_send_idle_cycles(const SPI_CFG_T *ptCfg __attribute__((unused)), unsigned int sizIdleCycles __attribute__((unused)))
{
	/* Idle cycles are not supported with this unit. */
	return -1;
}



static int spi_send_dummy(const SPI_CFG_T *ptCfg, unsigned int sizDummyBytes)
{
	unsigned char ucIdleChar;


	/* get the idle byte */
	ucIdleChar = ptCfg->ucDummyByte;

	while( sizDummyBytes>0 )
	{
		spi_exchange_byte(ptCfg, ucIdleChar);
		--sizDummyBytes;
	}

	return 0;
}



static int spi_send_data(const SPI_CFG_T *ptCfg, const unsigned char *pucData, unsigned int sizData)
{
	const unsigned char *pucDataEnd;


	/* send data */
	pucDataEnd = pucData + sizData;
	while( pucData<pucDataEnd )
	{
		spi_exchange_byte(ptCfg, *(pucData++));
	}

	return 0;
}



static int spi_receive_data(const SPI_CFG_T *ptCfg, unsigned char *pucData, unsigned int sizData)
{
	unsigned char ucIdleChar;
	unsigned char *pucDataEnd;


	/* get the idle byte */
	ucIdleChar = ptCfg->ucDummyByte;

	/* send data */
	pucDataEnd = pucData + sizData;
	while( pucData<pucDataEnd )
	{
		*pucData = spi_exchange_byte(ptCfg, ucIdleChar);
		++pucData;
	}

	return 0;
}



static int spi_exchange_data(const SPI_CFG_T *ptCfg, unsigned char *pucData, unsigned int sizData)
{
	unsigned char *pucDataEnd;
	unsigned char ucData;


	/* send data */
	pucDataEnd = pucData + sizData;
	while( pucData<pucDataEnd )
	{
		ucData = *pucData;
		*pucData = spi_exchange_byte(ptCfg, ucData);
		++pucData;
	}

	return 0;
}



static int spi_set_new_speed(const SPI_CFG_T *ptCfg __attribute__((unused)), unsigned long ulDeviceSpecificSpeed)
{
	HOSTDEF(ptSpiArea);
	unsigned long ulValue;


	ulDeviceSpecificSpeed &= HOSTMSK(spi_control_register_CR_speed);

	ulValue  = ptSpiArea->ulSpi_control_register;
	ulValue &= ~HOSTMSK(spi_control_register_CR_speed);
	ulValue |= ulDeviceSpecificSpeed;
	ptSpiArea->ulSpi_control_register = ulValue;

	return 0;
}



static void spi_reconfigure_ios(const SPI_CFG_T *ptCfg __attribute__((unused)))
{
}



static int spi_set_bus_width(SPI_CFG_T *ptCfg __attribute__((unused)), SPI_BUS_WIDTH_T tBusWidth)
{
	int iResult;


	/* This interface supports only one bus width: 1bit. */

	if( tBusWidth==SPI_BUS_WIDTH_1BIT )
	{
		uprintf("[BootDrvSpi] New bus width: %d\n", tBusWidth);
		iResult = 0;
	}
	else
	{
		uprintf("[BootDrvSpi] Invalid bus width: %d\n", tBusWidth);
		iResult = -1;
	}

	return iResult;
}


static unsigned long spi_get_device_specific_sqirom_cfg(SPI_CFG_T *ptCfg __attribute__((unused)), unsigned int uiDummyCycles __attribute__((unused)), unsigned int uiAddrBits __attribute__((unused)), unsigned int uiAddressNibbles __attribute__((unused)))
{
	return 0U;
}


static int spi_activate_sqirom(SPI_CFG_T *ptCfg __attribute__((unused)), unsigned long ulSettings __attribute__((unused)))
{
	uprintf("[BootDrvSpi] SQI ROM Unsupported\n");
	return -1;
}


static int spi_deactivate_sqirom(SPI_CFG_T *ptCfg __attribute__((unused)))
{
	return 0;
}


/*-------------------------------------------------------------------------*/


static void spi_deactivate(const SPI_CFG_T *ptCfg __attribute__((unused)))
{
	HOSTDEF(ptSpiArea);
	unsigned long ulValue;


	/* Soft reset SPI and clear both FIFOs. */
	ulValue  = HOSTMSK(spi_control_register_CR_softreset);
	ulValue |= HOSTMSK(spi_control_register_CR_clr_infifo);
	ulValue |= HOSTMSK(spi_control_register_CR_clr_outfifo);
	ptSpiArea->ulSpi_control_register = ulValue;

	/* Disable the SPI interface. */
	ptSpiArea->ulSpi_control_register = 0;

	/* Reset status bits. */
	ptSpiArea->ulSpi_status_register = 0;

	/* Do not use IRQs. */
	ptSpiArea->ulSpi_interrupt_control_register = 0;
}



int boot_drv_spi_init_v1(SPI_CFG_T *ptCfg, const BOOT_SPI_CONFIGURATION_T *ptSpiCfg, unsigned int uiSpiUnit, unsigned int uiChipSelect)
{
	HOSTDEF(ptSpiArea);
	unsigned long ulValue;
	int iResult;


	iResult = 0;

	ptCfg->pvArea = NULL;
	ptCfg->pvSqiRom = NULL;
	ptCfg->ulSpeedFifoKhz = ptSpiCfg->ulSpeedFifoKhz;            /* initial device speed in kHz */
	ptCfg->ucDummyByte = ptSpiCfg->ucDummyByte;     /* the idle configuration */
	ptCfg->uiIdleConfiguration = 0;
	ptCfg->tMode = ptSpiCfg->ucMode;                         /* bus mode */
	ptCfg->uiUnit = uiSpiUnit;                      /* the unit */
	ptCfg->uiChipSelect = uiChipSelect;                      /* chip select */

	/* set the function pointers */
	ptCfg->pfnSelect = spi_slave_select;
	ptCfg->pfnExchangeByte = spi_exchange_byte;
	ptCfg->pfnSendIdleCycles = spi_send_idle_cycles;
	ptCfg->pfnSendDummy = spi_send_dummy;
	ptCfg->pfnSendData = spi_send_data;
	ptCfg->pfnReceiveData = spi_receive_data;
	ptCfg->pfnExchangeData = spi_exchange_data;
	ptCfg->pfnSetNewSpeed = spi_set_new_speed;
	ptCfg->pfnGetDeviceSpeedRepresentation = spi_get_device_speed_representation;
	ptCfg->pfnReconfigureIos = spi_reconfigure_ios;
	ptCfg->pfnSetBusWidth = spi_set_bus_width;
	ptCfg->pfnGetDeviceSpecificSqiRomCfg = spi_get_device_specific_sqirom_cfg;
	ptCfg->pfnActivateSqiRom = spi_activate_sqirom;
	ptCfg->pfnDeactivateSqiRom = spi_deactivate_sqirom;
	ptCfg->pfnDeactivate = spi_deactivate;

	/* Soft reset SPI and clear both FIFOs. */
	ulValue  = HOSTMSK(spi_control_register_CR_softreset);
	ulValue |= HOSTMSK(spi_control_register_CR_clr_infifo);
	ulValue |= HOSTMSK(spi_control_register_CR_clr_outfifo);
	ptSpiArea->ulSpi_control_register = ulValue;

	/* Configure the SPI interface. */
	ulValue  = HOSTMSK(spi_control_register_CR_read);                /* Enable reading data from SPI_MISO. */
	ulValue |= HOSTMSK(spi_control_register_CR_write);               /* Enable writing data to SPI_MOSI. */
	ulValue |= HOSTMSK(spi_control_register_CR_ms);                  /* Set the unit to master mode. */
	ulValue |= (7<<HOSTSRT(spi_control_register_CR_burst));          /* Set the burst size to the maximum. */
	ulValue |= (0<<HOSTSRT(spi_control_register_CR_burstdelay));     /* No burst delay. */
	ulValue |= HOSTMSK(spi_control_register_CR_en);                  /* Enable the interface. */
	ulValue |= spi_get_device_speed_representation(ptCfg->ulSpeedFifoKhz);  /* clock divider for SCK */

	/* Set the clock polarity. */
	/* Mode 2 and 3 have cpol=1 . */
	if( (ptCfg->tMode==SPI_MODE2) || (ptCfg->tMode==SPI_MODE3) )
	{
		ulValue |= HOSTMSK(spi_control_register_CR_cpol);
	}

	/* Set the clock phase. */
	/* Mode 0 and 2 have ncpha=1 . */
	if( (ptCfg->tMode==SPI_MODE0) || (ptCfg->tMode==SPI_MODE2) )
	{
		ulValue |= HOSTMSK(spi_control_register_CR_ncpha);
	}

	/* Write value to control register. */
	ptSpiArea->ulSpi_control_register = ulValue;

	/* Reset status bits. */
	ptSpiArea->ulSpi_status_register = 0;

	/* Do not use IRQs. */
	ptSpiArea->ulSpi_interrupt_control_register = 0;


	/* transfer control base is unused in this driver */
	ptCfg->ulTrcBase = 0;

	return iResult;
}
