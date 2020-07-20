/*---------------------------------------------------------------------------
  Author : Christoph Thelen

           Hilscher GmbH, Copyright (c) 2006, All Rights Reserved

           Redistribution or unauthorized use without expressed written
           agreement from the Hilscher GmbH is forbidden
---------------------------------------------------------------------------*/


#include <string.h>

#include "boot_drv_sqi.h"

#include "netx_io_areas.h"
#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
#       include "portcontrol.h"
#endif
#include "tools.h"
#include "uprintf.h"



#define MAXIMUM_TRANSACTION_SIZE_BYTES 0x80000


#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
/* This tables shows the port control indices for the SQI0 pins.
 * There is only CS0. All other chip selects are not routed to the outside.
 */
static const unsigned short ausPortcontrol_Index_SQI0_CS0[6] =
{
	PORTCONTROL_INDEX( 1,  5),      // SQI0_CS0N
	PORTCONTROL_INDEX( 1,  0),      // SQI0_CLK
	PORTCONTROL_INDEX( 1,  2),      // SQI0_MISO
	PORTCONTROL_INDEX( 1,  1),      // SQI0_MOSI
	PORTCONTROL_INDEX( 1,  3),      // SQI0_SIO2
	PORTCONTROL_INDEX( 1,  4),      // SQI0_SIO3
};


/* This tables shows the port control indices for the SQI1 pins.
 * There is only CS0. All other chip selects are not routed to the outside.
 */
static const unsigned short ausPortcontrol_Index_SQI1_CS0[6] =
{
	PORTCONTROL_INDEX(10,  8),      // SQI1_CS0N
	PORTCONTROL_INDEX(10,  3),      // SQI1_CLK
	PORTCONTROL_INDEX(10,  5),      // SQI1_MISO
	PORTCONTROL_INDEX(10,  4),      // SQI1_MOSI
	PORTCONTROL_INDEX(10,  6),      // SQI1_SIO2
	PORTCONTROL_INDEX(10,  7),      // SQI1_SIO3
};
#endif


static unsigned char qsi_exchange_byte(const SPI_CFG_T *ptCfg, unsigned char uiByte)
{
	HOSTADEF(SQI) * ptSqi;
	unsigned long ulValue;
	unsigned char ucByte;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	/* Set mode to "full duplex". */
	ulValue  = ptCfg->ulTrcBase;
	ulValue |= 3 << HOSTSRT(sqi_tcr_duplex);
	/* Start the transfer. */
	ulValue |= HOSTMSK(sqi_tcr_start_transfer);
	ptSqi->ulSqi_tcr = ulValue;

	/* Send byte. */
	ptSqi->ulSqi_dr = uiByte;

	/* Wait for one byte in the FIFO. */
	do
	{
		ulValue  = ptSqi->ulSqi_sr;
		ulValue &= HOSTMSK(sqi_sr_busy);
	} while( ulValue!=0 );

	/* Grab byte. */
	ucByte = (unsigned char)(ptSqi->ulSqi_dr);
	return ucByte;
}

//---------------------------------------------------------------------------


static unsigned long qsi_get_device_speed_representation(unsigned int uiSpeed)
{
	unsigned long ulDevSpeed;
	unsigned long ulInputFilter;


	/* ulSpeed is in kHz. */

	/* Limit speed to upper border. */
	if( uiSpeed>50000 )
	{
		uiSpeed = 50000;
	}

	/* Convert speed to "multiply add value". */
	ulDevSpeed  = uiSpeed * 4096;

	/* NOTE: do not round up here. */
	ulDevSpeed /= 100000;

	/* Use input filtering? */
	ulInputFilter = 0;
	if( ulDevSpeed<=0x0200 )
	{
		ulInputFilter = HOSTMSK(sqi_cr0_filter_in);
	}

	/* Shift to register position. */
	ulDevSpeed <<= HOSTSRT(sqi_cr0_sck_muladd);

	/* Add filter bit. */
	ulDevSpeed |= ulInputFilter;

	return ulDevSpeed;
}



static void qsi_slave_select(const SPI_CFG_T *ptCfg, int fIsSelected)
{
	HOSTADEF(SQI) * ptSqi;
	unsigned long uiChipSelect;
	unsigned long ulValue;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	/* Get the chip select value. */
	uiChipSelect  = 0;
	if( fIsSelected!=0 )
	{
		uiChipSelect  = ptCfg->uiChipSelect << HOSTSRT(sqi_cr1_fss);
		uiChipSelect &= HOSTMSK(sqi_cr1_fss);
	}

	/* Get control register contents. */
	ulValue  = ptSqi->aulSqi_cr[1];

	/* Mask out the slave select bits. */
	ulValue &= ~HOSTMSK(sqi_cr1_fss);

	/* Mask in the new slave ID. */
	ulValue |= uiChipSelect;

	/* Write back new value. */
	ptSqi->aulSqi_cr[1] = ulValue;
}



static int qsi_send_idle_cycles(const SPI_CFG_T *ptCfg, unsigned int sizIdleCycles)
{
	HOSTADEF(SQI) * ptSqi;
	unsigned long ulValue;
	unsigned int sizChunkTransaction;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	while( sizIdleCycles!=0 )
	{
		/* Limit the number of cycles to the maximum transaction size. */
		sizChunkTransaction = sizIdleCycles;
		if( sizChunkTransaction>((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U) )
		{
			sizChunkTransaction = ((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U);
		}

		/* Set mode to "send dummy". */
		ulValue  = ptCfg->ulTrcBase;
		ulValue |= 0 << HOSTSRT(sqi_tcr_duplex);
		/* Set the transfer size. */
		ulValue |= (sizChunkTransaction-1) << HOSTSRT(sqi_tcr_transfer_size);
		/* Clear the output bits. */
		ulValue &= ~(HOSTMSK(sqi_tcr_tx_oe)|HOSTMSK(sqi_tcr_tx_out));
		/* Start the transfer. */
		ulValue |= HOSTMSK(sqi_tcr_start_transfer);
		ptSqi->ulSqi_tcr = ulValue;

		/* Wait until the transfer is done. */
		do
		{
			ulValue  = ptSqi->ulSqi_sr;
			ulValue &= HOSTMSK(sqi_sr_busy);
		} while( ulValue!=0 );

		sizIdleCycles -= sizChunkTransaction;
	}

	return 0;
}



static int qsi_send_dummy(const SPI_CFG_T *ptCfg, unsigned int sizDummyBytes)
{
	HOSTADEF(SQI) * ptSqi;
	unsigned long ulValue;
	unsigned int sizChunkTransaction;
	unsigned int sizChunkFifo;
	unsigned char ucDummyByte;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);
	ucDummyByte = ptCfg->ucDummyByte;

	while( sizDummyBytes!=0 )
	{
		/* Limit the number of bytes by the maximum transaction size. */
		sizChunkTransaction = sizDummyBytes;
		if( sizChunkTransaction>((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U) )
		{
			sizChunkTransaction = ((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U);
		}
		sizDummyBytes -= sizChunkTransaction;

		/* Set the mode to "send". */
		ulValue  = ptCfg->ulTrcBase;
		ulValue |= 2 << HOSTSRT(sqi_tcr_duplex);
		/* Set the transfer size. */
		ulValue |= (sizChunkTransaction - 1U) << HOSTSRT(sqi_tcr_transfer_size);
		/* Start the transfer. */
		ulValue |= HOSTMSK(sqi_tcr_start_transfer);
		ptSqi->ulSqi_tcr = ulValue;

		/* Check the mode. */
		ulValue  = ptCfg->ulTrcBase;
		ulValue &= HOSTMSK(sqi_tcr_mode);
		if( ulValue==0 )
		{
			/* Mode 0 : the FIFO size is 8 bit. */
			while( sizChunkTransaction!=0 )
			{
				ulValue   = ptSqi->ulSqi_sr;
				ulValue  &= HOSTMSK(spi_sr_tx_fifo_level);
				ulValue >>= HOSTSRT(spi_sr_tx_fifo_level);
				/* The FIFO has 16 entries. Get the number of free entries from the fill level. */
				ulValue = 16 - ulValue;

				/* Try to fill up the complete FIFO... */
				sizChunkFifo = ulValue;
				/* .. but limit this by the number of bytes left to send. */
				if( sizChunkFifo>sizChunkTransaction )
				{
					sizChunkFifo = sizChunkTransaction;
				}

				sizChunkTransaction -= sizChunkFifo;
				while( sizChunkFifo!=0 )
				{
					/* Send byte */
					ptSqi->ulSqi_dr = (unsigned long)ucDummyByte;
					--sizChunkFifo;
				}
			}
		}
		else
		{
			/* DSI/QSI mode : the FIFO size is 32 bit */
			do
			{
				/* collect a DWORD */
				sizChunkFifo = 4;
				if( sizChunkFifo>sizChunkTransaction )
				{
					sizChunkFifo = sizChunkTransaction;
				}
				sizChunkTransaction -= sizChunkFifo;

				/* wait for space in the FIFO */
				do
				{
					ulValue  = ptSqi->ulSqi_sr;
					ulValue &= HOSTMSK(sqi_sr_rx_fifo_full);
				} while( ulValue!=0 );

				/* send DWORD */
				ulValue  =  (unsigned long)ucDummyByte;
				ulValue |= ((unsigned long)ucDummyByte) <<  8U;
				ulValue |= ((unsigned long)ucDummyByte) << 16U;
				ulValue |= ((unsigned long)ucDummyByte) << 24U;
				ptSqi->ulSqi_dr = ulValue;
			} while( sizChunkTransaction!=0 );
		}

		/* wait until the transfer is done */
		do
		{
			ulValue  = ptSqi->ulSqi_sr;
			ulValue &= HOSTMSK(sqi_sr_busy);
		} while( ulValue!=0 );
	}

	return 0;
}



static int qsi_send_data(const SPI_CFG_T *ptCfg, const unsigned char *pucData, unsigned int sizData)
{
	HOSTADEF(SQI) * ptSqi;
	unsigned long ulValue;
	unsigned int uiShiftCnt;
	unsigned long ulSend;
	unsigned int sizChunkTransaction;
	unsigned int sizChunkFifo;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	while( sizData!=0 )
	{
		/* Limit the number of bytes by the maximum transaction size. */
		sizChunkTransaction = sizData;
		if( sizChunkTransaction>((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U) )
		{
			sizChunkTransaction = ((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U);
		}
		sizData -= sizChunkTransaction;

		/* Set the mode to "send". */
		ulValue  = ptCfg->ulTrcBase;
		ulValue |= 2 << HOSTSRT(sqi_tcr_duplex);
		/* Set the transfer size. */
		ulValue |= (sizChunkTransaction - 1U) << HOSTSRT(sqi_tcr_transfer_size);
		/* Start the transfer. */
		ulValue |= HOSTMSK(sqi_tcr_start_transfer);
		ptSqi->ulSqi_tcr = ulValue;

		/* Check the mode. */
		ulValue  = ptCfg->ulTrcBase;
		ulValue &= HOSTMSK(sqi_tcr_mode);
		if( ulValue==0 )
		{
			/* Mode 0 : the FIFO size is 8 bit. */
			while( sizChunkTransaction!=0 )
			{
				ulValue   = ptSqi->ulSqi_sr;
				ulValue  &= HOSTMSK(spi_sr_tx_fifo_level);
				ulValue >>= HOSTSRT(spi_sr_tx_fifo_level);
				/* The FIFO has 16 entries. Get the number of free entries from the fill level. */
				ulValue = 16 - ulValue;

				/* Try to fill up the complete FIFO... */
				sizChunkFifo = ulValue;
				/* .. but limit this by the number of bytes left to send. */
				if( sizChunkFifo>sizChunkTransaction )
				{
					sizChunkFifo = sizChunkTransaction;
				}

				sizChunkTransaction -= sizChunkFifo;
				while( sizChunkFifo!=0 )
				{
					/* Send byte */
					ptSqi->ulSqi_dr = *(pucData++);
					--sizChunkFifo;
				}
			}
		}
		else
		{
			/* DSI/QSI mode : the FIFO size is 32 bit */
			do
			{
				/* collect a DWORD */
				ulSend = 0;
				uiShiftCnt = 0;
				do
				{
					ulSend |= ((unsigned long)(*(pucData++))) << (uiShiftCnt<<3U);
					++uiShiftCnt;
					--sizChunkTransaction;
				} while( sizChunkTransaction!=0 && uiShiftCnt<4 );

				/* wait for space in the FIFO */
				do
				{
					ulValue  = ptSqi->ulSqi_sr;
					ulValue &= HOSTMSK(sqi_sr_rx_fifo_full);
				} while( ulValue!=0 );
				/* send DWORD */
				ptSqi->ulSqi_dr = ulSend;
			} while( sizChunkTransaction!=0 );
		}

		/* wait until the transfer is done */
		do
		{
			ulValue  = ptSqi->ulSqi_sr;
			ulValue &= HOSTMSK(sqi_sr_busy);
		} while( ulValue!=0 );
	}

	return 0;
}



static int qsi_receive_data(const SPI_CFG_T *ptCfg, unsigned char *pucData, unsigned int sizData)
{
	HOSTADEF(SQI) * ptSqi;
	ADR_T tAdr;
	unsigned long ulValue;
	unsigned int sizChunkTransaction;
	unsigned int sizChunkFifo;
	unsigned int sizBytes;
	unsigned char ucReceivedChar;
	unsigned long ulReceivedDw;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	tAdr.puc = pucData;

	while( sizData!=0 )
	{
		/* Limit the number of bytes by the maximum transaction size. */
		sizChunkTransaction = sizData;
		if( sizChunkTransaction>((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U) )
		{
			sizChunkTransaction = ((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U);
		}
		sizData -= sizChunkTransaction;

		/* Set mode to "receive". */
		ulValue  = ptCfg->ulTrcBase;
		ulValue |= 1 << HOSTSRT(sqi_tcr_duplex);
		/* Set the transfer size. */
		ulValue |= (sizChunkTransaction - 1) << HOSTSRT(sqi_tcr_transfer_size);
		/* Start the transfer. */
		ulValue |= HOSTMSK(sqi_tcr_start_transfer);
		ptSqi->ulSqi_tcr = ulValue;

		/* Check the mode. */
		if( (ptCfg->ulTrcBase&HOSTMSK(sqi_tcr_mode))==0 )
		{
			/* Mode 0 : the FIFO size is 8 bit. */

			/* Use an optimized DWORD receive if...
			 *  1) the destination pointer is DWORD aligned
			 *  2) the size is a multiple of DWORDs
			 */
			if( (tAdr.ul & 3U)==0U && (sizChunkTransaction & 3U)==0U )
			{
				/* Initialize the shift register for the bytes. */
				ulReceivedDw = 0U;

				while( sizChunkTransaction!=0 )
				{
					/* Get the fill level of the FIFO. */
					ulValue   = ptSqi->ulSqi_sr;
					ulValue  &= HOSTMSK(spi_sr_rx_fifo_level);
					ulValue >>= HOSTSRT(spi_sr_rx_fifo_level);

					/* Limit the chunk by the number of bytes to transfer. */
					sizChunkFifo = ulValue;
					if( sizChunkFifo>sizChunkTransaction )
					{
						sizChunkFifo = sizChunkTransaction;
					}

					sizChunkTransaction -= sizChunkFifo;
					while( sizChunkFifo!=0 )
					{
						/* Grab a byte. */
						ucReceivedChar = (unsigned char)(ptSqi->ulSqi_dr & 0xffU);
						ulReceivedDw >>= 8U;
						ulReceivedDw  |= ((unsigned long)ucReceivedChar) << 24U;
						++tAdr.puc;
						if( (tAdr.ul&3U)==0U )
						{
							*(tAdr.pul-1U) = ulReceivedDw;
						}

						--sizChunkFifo;
					}
				}
			}
			else
			{
				while( sizChunkTransaction!=0 )
				{
					/* Get the fill level of the FIFO. */
					ulValue   = ptSqi->ulSqi_sr;
					ulValue  &= HOSTMSK(spi_sr_rx_fifo_level);
					ulValue >>= HOSTSRT(spi_sr_rx_fifo_level);

					/* Limit the chunk by the number of bytes to transfer. */
					sizChunkFifo = ulValue;
					if( sizChunkFifo>sizChunkTransaction )
					{
						sizChunkFifo = sizChunkTransaction;
					}

					sizChunkTransaction -= sizChunkFifo;
					while( sizChunkFifo!=0 )
					{
						/* Grab a byte. */
						ucReceivedChar = (unsigned char)(ptSqi->ulSqi_dr & 0xffU);
						*(tAdr.puc++) = ucReceivedChar;

						--sizChunkFifo;
					}
				}
			}
		}
		else
		{
			/* DSI/QSI mode : the FIFO size is 32 bit. */

			/* Use an optimized DWORD receive if...
			 *  1) the destination pointer is DWORD aligned
			 *  2) the size is a multiple of DWORDs
			 */
			if( (tAdr.ul & 3U)==0U && (sizChunkTransaction & 3U)==0U )
			{
				/* Initialize the shift register for the bytes. */
				ulReceivedDw = 0U;

				while( sizChunkTransaction!=0 )
				{
					/* Get the fill level of the FIFO. */
					ulValue   = ptSqi->ulSqi_sr;
					ulValue  &= HOSTMSK(spi_sr_rx_fifo_level);
					ulValue >>= HOSTSRT(spi_sr_rx_fifo_level);

					/* Limit the chunk by the number of bytes to transfer. */
					sizChunkFifo = ulValue * sizeof(unsigned long);
					if( sizChunkFifo>sizChunkTransaction )
					{
						sizChunkFifo = sizChunkTransaction;
					}

					sizChunkTransaction -= sizChunkFifo;
					while( sizChunkFifo!=0 )
					{
						/* Get the DWORD. */
						ulValue = ptSqi->ulSqi_dr;
						*(tAdr.pul++) = ulValue;

						sizChunkFifo -= 4U;
					}
				}
			}
			else
			{
				while( sizChunkTransaction!=0 )
				{
					/* Get the fill level of the FIFO. */
					ulValue   = ptSqi->ulSqi_sr;
					ulValue  &= HOSTMSK(spi_sr_rx_fifo_level);
					ulValue >>= HOSTSRT(spi_sr_rx_fifo_level);

					/* Limit the chunk by the number of bytes to transfer. */
					sizChunkFifo = ulValue * sizeof(unsigned long);
					if( sizChunkFifo>sizChunkTransaction )
					{
						sizChunkFifo = sizChunkTransaction;
					}

					sizChunkTransaction -= sizChunkFifo;
					while( sizChunkFifo!=0 )
					{
						/* Get the DWORD. */
						ulValue = ptSqi->ulSqi_dr;

						sizBytes = 4;
						if( sizBytes>sizChunkFifo )
						{
							sizBytes = sizChunkFifo;
						}

						sizChunkFifo -= sizBytes;
						while( sizBytes!=0 )
						{
							ucReceivedChar = (unsigned char)(ulValue & 0xffU);
							*(tAdr.puc++) = ucReceivedChar;
							ulValue >>= 8U;

							--sizBytes;
						}
					}
				}
			}
		}
	}

	return 0;
}



static int qsi_exchange_data(const SPI_CFG_T *ptCfg, unsigned char *pucData, unsigned int sizData)
{
	HOSTADEF(SQI) * ptSqi;
	int iResult;
	unsigned long ulValue;
	unsigned int sizChunkTransaction;
	unsigned int sizChunkFifo;
	unsigned char *pucDataRx;
	unsigned char *pucDataTx;
	unsigned int sizDataRx;
	unsigned int sizDataTx;
	unsigned char ucReceivedChar;


	/* Be optimistic. */
	iResult = 0;

	/* Exchanging data works only in 1bit full-duplex mode. */
	if( (ptCfg->ulTrcBase&HOSTMSK(sqi_tcr_mode))!=0 )
	{
		uprintf("[BootDrvSqi] Invalid bus width for 'exchange_data'.\n");
		iResult = -1;
	}
	else
	{
		ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

		pucDataRx = pucData;
		pucDataTx = pucData;

		while( sizData!=0 )
		{
			/* Limit the number of bytes by the maximum transaction size. */
			sizChunkTransaction = sizData;
			if( sizChunkTransaction>((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U) )
			{
				sizChunkTransaction = ((HOSTMSK(sqi_tcr_transfer_size)<<HOSTSRT(sqi_tcr_transfer_size))+1U);
			}
			sizData -= sizChunkTransaction;

			/* Set mode to "send and receive". */
			ulValue  = ptCfg->ulTrcBase;
			ulValue |= 3 << HOSTSRT(sqi_tcr_duplex);
			/* Set the transfer size. */
			ulValue |= (sizChunkTransaction - 1U) << HOSTSRT(sqi_tcr_transfer_size);
			/* Start the transfer. */
			ulValue |= HOSTMSK(sqi_tcr_start_transfer);
			ptSqi->ulSqi_tcr = ulValue;

			/* The requested amount of data must be send and received. */
			sizDataRx = sizChunkTransaction;
			sizDataTx = sizChunkTransaction;

			/* Mode 0 : the FIFO size is 8 bit. */
			do
			{
				/*
				 * Send data.
				 */
				/* Get the fill level of the transmit FIFO. */
				ulValue   = ptSqi->ulSqi_sr;
				ulValue  &= HOSTMSK(spi_sr_tx_fifo_level);
				ulValue >>= HOSTSRT(spi_sr_tx_fifo_level);
				/* The FIFO has 16 entries. Get the number of free entries from the fill level. */
				ulValue = 16U - ulValue;

				/* Try to fill up the complete FIFO... */
				sizChunkFifo = ulValue;
				/* .. but limit this by the number of bytes left to send. */
				if( sizChunkFifo>sizDataTx )
				{
					sizChunkFifo = sizDataTx;
				}

				sizDataTx -= sizChunkFifo;
				while( sizChunkFifo!=0 )
				{
					/* Send byte */
					ptSqi->ulSqi_dr = *(pucDataTx++);
					--sizChunkFifo;
				}


				/*
				 * Receive data.
				 */
				/* Get the fill level of the receive FIFO. */
				ulValue   = ptSqi->ulSqi_sr;
				ulValue  &= HOSTMSK(spi_sr_rx_fifo_level);
				ulValue >>= HOSTSRT(spi_sr_rx_fifo_level);

				/* Limit the chunk by the number of bytes to transfer. */
				sizChunkFifo = ulValue;
				if( sizChunkFifo>sizDataRx )
				{
					sizChunkFifo = sizDataRx;
				}

				sizDataRx -= sizChunkFifo;
				while( sizChunkFifo!=0 )
				{
					/* Grab a byte. */
					ucReceivedChar = (unsigned char)(ptSqi->ulSqi_dr & 0xffU);
					*(pucDataRx++) = ucReceivedChar;

					--sizChunkFifo;
				}
			} while( (sizDataRx | sizDataTx)!=0 );
		}
	}

	return iResult;
}



static int qsi_set_new_speed(const SPI_CFG_T *ptCfg, unsigned long ulDeviceSpecificSpeed)
{
	HOSTADEF(SQI) * ptSqi;
	int iResult;
	unsigned long ulValue;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	/* Expect error. */
	iResult = 1;

	/* All irrelevant bits must be 0. */
	if( (ulDeviceSpecificSpeed&(~(HOSTMSK(sqi_cr0_sck_muladd)|HOSTMSK(sqi_cr0_filter_in))))!=0 )
	{
		uprintf("[BootDrvSqi] Invalid new device specific speed: 0x%08x\n", ulDeviceSpecificSpeed);
	}
	else if( ulDeviceSpecificSpeed==0 )
	{
		uprintf("[BootDrvSqi] The device specific new speed must not be 0.\n");
	}
	else
	{
		uprintf("[BootDrvSqi] New device specific speed: 0x%08x\n", ulDeviceSpecificSpeed);

		ulValue  = ptSqi->aulSqi_cr[0];
		ulValue &= ~(HOSTMSK(sqi_cr0_sck_muladd)|HOSTMSK(sqi_cr0_filter_in));
		ulValue |= ulDeviceSpecificSpeed;
		ptSqi->aulSqi_cr[0] = ulValue;

		/* All OK! */
		iResult = 0;
	}

	return iResult;
}



#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
static void qsi_reconfigure_ios(const SPI_CFG_T *ptCfg)
{
	unsigned int uiSqiUnit;
	const unsigned short *pusPortControlIndex;


	/* Get the SQI unit number. */
	uiSqiUnit = ptCfg->uiUnit;

	/* Get the port control table for the unit. */
	pusPortControlIndex = NULL;
	if( uiSqiUnit==0 )
	{
		pusPortControlIndex = ausPortcontrol_Index_SQI0_CS0;
	}
	else if( uiSqiUnit==1 )
	{
		pusPortControlIndex = ausPortcontrol_Index_SQI1_CS0;
	}

	if( pusPortControlIndex!=NULL )
	{
		/* Set up the port control unit. */
		portcontrol_apply(pusPortControlIndex, ptCfg->ausPortControl, sizeof(ptCfg->ausPortControl)/sizeof(ptCfg->ausPortControl[0]));
	}
}
#else
static void qsi_reconfigure_ios(const SPI_CFG_T *ptCfg __attribute__((unused)))
{
}
#endif


static int qsi_set_bus_width(SPI_CFG_T *ptCfg, SPI_BUS_WIDTH_T tBusWidth)
{
	HOSTADEF(SQI) * ptSqi;
	int iResult;
	unsigned long ulTcrMode;
	unsigned long ulSioCfg;
	unsigned long ulValue;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	iResult = -1;
	switch(tBusWidth)
	{
	case SPI_BUS_WIDTH_1BIT:
		ulTcrMode = 0;
		ulSioCfg = 0;
		iResult = 0;
		break;

	case SPI_BUS_WIDTH_2BIT:
		ulTcrMode = 1;
		ulSioCfg = 0;
		iResult = 0;
		break;

	case SPI_BUS_WIDTH_4BIT:
		ulTcrMode = 2;
		ulSioCfg = 1;
		iResult = 0;
		break;
	}

	if( iResult==0 )
	{
		uprintf("[BootDrvSqi] Set new bus width: %d\n", tBusWidth);

		/* Set the new SIO configuration. */
		ulValue  = ptSqi->aulSqi_cr[0];
		ulValue &= ~HOSTMSK(sqi_cr0_sio_cfg);
		ulValue |= ulSioCfg << HOSTSRT(sqi_cr0_sio_cfg);
		ptSqi->aulSqi_cr[0] = ulValue;

		ulValue  = ptCfg->ulTrcBase;
		ulValue &= ~HOSTMSK(sqi_tcr_mode);
		ulValue |= ulTcrMode << HOSTSRT(sqi_tcr_mode);;
		ptCfg->ulTrcBase = ulValue;
	}
	else
	{
		uprintf("[BootDrvSqi] Invalid bus width: %d\n", tBusWidth);
	}

	return iResult;
}



#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
#       define SQIROMCFG_DUMMY_CYCLES_MAXIMUM 15
#       define SQIROMCFG_ADDRESS_NIBBLES_MINIMUM 5
#       define SQIROMCFG_ADDRESS_NIBBLES_MAXIMUM 8
#       define SQIROMCFG_ADDRESS_BITS_MINIMUM 20
#       define SQIROMCFG_ADDRESS_BITS_MAXIMUM 26
#       define SQIROMCFG_FREQUENCY_MAXIMUM_KHZ 142000U
#elif ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
#       define SQIROMCFG_DUMMY_CYCLES_MAXIMUM 15
#       define SQIROMCFG_ADDRESS_NIBBLES_MINIMUM 5
#       define SQIROMCFG_ADDRESS_NIBBLES_MAXIMUM 8
#       define SQIROMCFG_ADDRESS_BITS_MINIMUM 20
#       define SQIROMCFG_ADDRESS_BITS_MAXIMUM 26
#       define SQIROMCFG_FREQUENCY_MAXIMUM_KHZ 133000U
#elif ASIC_TYP==ASIC_TYP_NETX56
#       define SQIROMCFG_DUMMY_CYCLES_MAXIMUM 7
#       define SQIROMCFG_ADDRESS_NIBBLES_MINIMUM 5
#       define SQIROMCFG_ADDRESS_NIBBLES_MAXIMUM 8
#       define SQIROMCFG_ADDRESS_BITS_MINIMUM 20
#       define SQIROMCFG_ADDRESS_BITS_MAXIMUM 24
#       define SQIROMCFG_FREQUENCY_MAXIMUM_KHZ 133000U
#endif

static unsigned long qsi_get_device_specific_sqirom_cfg(SPI_CFG_T *ptCfg, unsigned int uiDummyCycles, unsigned int uiAddressBits, unsigned int uiAddressNibbles)
{
	unsigned long ulDeviceSpecificValue;
	unsigned long ulFreqKHz;
	unsigned long ulClockDivider;


	/* Get the maximum frequency for the ROM mode. */
	ulFreqKHz = ptCfg->ulSpeedSqiRomKhz;

	if( uiDummyCycles>SQIROMCFG_DUMMY_CYCLES_MAXIMUM )
	{
		/* The number of dummy cycles exceed the capabilities of the hardware. */
		uprintf("[BootDrvSqi] Invalid dummy cycles for SQI ROM: %d\n", uiDummyCycles);
		ulDeviceSpecificValue = 0;
	}
	else if( (uiAddressNibbles<SQIROMCFG_ADDRESS_NIBBLES_MINIMUM) || (uiAddressNibbles>SQIROMCFG_ADDRESS_NIBBLES_MAXIMUM) )
	{
		/* The address bits can not be realized by the hardware. */
		uprintf("[BootDrvSqi] Invalid address bits for SQI ROM: %d\n", uiAddressNibbles);
		ulDeviceSpecificValue = 0;
	}
	else
	{
		if( uiAddressBits<SQIROMCFG_ADDRESS_BITS_MINIMUM )
		{
			uiAddressBits = SQIROMCFG_ADDRESS_BITS_MINIMUM;
		}
		else if( uiAddressBits>SQIROMCFG_ADDRESS_BITS_MAXIMUM )
		{
			uiAddressBits = SQIROMCFG_ADDRESS_BITS_MAXIMUM;
		}

		/* Limit the frequency to the allowed maximum. */
		if( ulFreqKHz>SQIROMCFG_FREQUENCY_MAXIMUM_KHZ )
		{
			ulFreqKHz = SQIROMCFG_FREQUENCY_MAXIMUM_KHZ;
		}

#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
		/* In the regdef the following formula is specified:
		 *
		 *   t_sck = (clk_div_val+1)*1.0ns
		 *
		 * Converted to the frequency this results in
		 *
		 *   1 / t_sck = 1 / ((clk_div_val + 1) * 1.0ns)
		 *       f_sck = 1 / 1.0ns * 1 / (clk_div_val + 1)
		 *       f_sck = 1000000000Hz *  1 / (clk_div_val + 1)
		 *       f_sck = 1000000kHz / (clk_div_val + 1)
		 *
		 * This results in the clock divider:
		 *
		 *   clk_div_val + 1 =  1000000kHz / freq
		 *   clk_div_val     = (1000000kHz / freq) - 1
		 */
		ulClockDivider  = 1000000U;
		ulClockDivider /= ulFreqKHz;
		ulClockDivider -= 1U;
		/* The value must not be smaller than 6. */
		if( ulClockDivider<6U )
		{
			ulClockDivider = 6U;
		}
#elif ASIC_TYP==ASIC_TYP_NETX56 || ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
		/* In the regdef the following formula is specified:
		 *
		 *   t_sck = (clk_div_val+3)*2.5ns
		 *
		 * Converted to the frequency this results in
		 *
		 *   1 / t_sck = 1 / ((clk_div_val + 3) * 2.5ns)
		 *       f_sck = 1 / 2.5ns * 1 / (clk_div_val + 3)
		 *       f_sck = 400000000Hz *  1 / (clk_div_val + 3)
		 *       f_sck = 400000kHz / (clk_div_val + 3)
		 *
		 * This results in the clock divider:
		 *
		 *   clk_div_val + 3 =  400000kHz / freq
		 *   clk_div_val     = (400000kHz / freq) - 3
		 */
		ulClockDivider  = 400000U;
		ulClockDivider /= ulFreqKHz;
		ulClockDivider -= 3U;
#endif

		/* SFDP does not provide any speed information for read operation. Use 50MHz. */
		ulDeviceSpecificValue  = ulClockDivider << HOSTSRT(sqi_sqirom_cfg_clk_div_val);

		/* Set the minimum high time for the chip select signal to 1 clock. */
		ulDeviceSpecificValue |= 0 << HOSTSRT(sqi_sqirom_cfg_t_csh);

		/* Set the dummy cycles. */
		ulDeviceSpecificValue |= uiDummyCycles << HOSTSRT(sqi_sqirom_cfg_dummy_cycles);

		/* Set the default mode byte as the command. */
		ulDeviceSpecificValue |= 0xa5 << HOSTSRT(sqi_sqirom_cfg_cmd_byte);

		/* Set the number of address bits for internal calculation.  */
		ulDeviceSpecificValue |= (uiAddressBits-20U) << HOSTSRT(sqi_sqirom_cfg_addr_bits);

		/* Set the number of address nibbles. */
		ulDeviceSpecificValue |= (uiAddressNibbles-5U) << HOSTSRT(sqi_sqirom_cfg_addr_nibbles);

		/* The command is here the mode byte. Send the address before the mode byte. */
		ulDeviceSpecificValue |= HOSTMSK(sqi_sqirom_cfg_addr_before_cmd);
		ulDeviceSpecificValue |= HOSTMSK(sqi_sqirom_cfg_enable);

		uprintf("[BootDrvSqi] New SQI ROM configuration: 0x%08x\n", ulDeviceSpecificValue);
	}

	return ulDeviceSpecificValue;
}



static int qsi_activate_sqirom(SPI_CFG_T *ptCfg, unsigned long ulSettings)
{
	HOSTADEF(SQI) * ptSqi;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);
	ptSqi->ulSqi_sqirom_cfg = ulSettings;

	uprintf("[BootDrvSqi] SQI ROM activated.\n");

	return 0;
}



static int qsi_deactivate_sqi_rom(SPI_CFG_T *ptCfg)
{
	HOSTADEF(SQI) * ptSqi;


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);
	ptSqi->ulSqi_sqirom_cfg = 0;

	uprintf("[BootDrvSqi] SQI ROM deactivated.\n");

	return 0;
}


/*-------------------------------------------------------------------------*/


static void qsi_deactivate(const SPI_CFG_T *ptCfg)
{
	HOSTADEF(SQI) * ptSqi;
#if ASIC_TYP==ASIC_TYP_NETX56
	HOSTDEF(ptAsicCtrlArea);
#endif
	unsigned long ulValue;
#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
	unsigned int uiSqiUnit;
	const unsigned short *pusPortControlIndex;
#endif


	ptSqi = (HOSTADEF(SQI) *)(ptCfg->pvArea);

	/* Deactivate IRQs. */
	ptSqi->ulSqi_irq_mask = 0;
	/* Clear all pending IRQs. */
	ulValue  = HOSTMSK(sqi_irq_clear_RORIC);
	ulValue |= HOSTMSK(sqi_irq_clear_RTIC);
	ulValue |= HOSTMSK(sqi_irq_clear_RXIC);
	ulValue |= HOSTMSK(sqi_irq_clear_TXIC);
	ulValue |= HOSTMSK(sqi_irq_clear_rxneic);
	ulValue |= HOSTMSK(sqi_irq_clear_rxfic);
	ulValue |= HOSTMSK(sqi_irq_clear_txeic);
	ulValue |= HOSTMSK(sqi_irq_clear_trans_end);
	ptSqi->ulSqi_irq_clear = ulValue;

	/* Deactivate DMAs. */
	ptSqi->ulSqi_dmacr = 0;

	/* Deactivate XIP. */
	ptSqi->ulSqi_sqirom_cfg = 0;

	ptSqi->ulSqi_tcr = 0;
	ptSqi->ulSqi_pio_oe = 0;
	ptSqi->ulSqi_pio_out = 0;

	/* Deactivate the unit. */
	ptSqi->aulSqi_cr[0] = 0;
	ptSqi->aulSqi_cr[1] = 0;

#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
	/* Restore the default settings for the port control unit. */
	uiSqiUnit = ptCfg->uiUnit;
	pusPortControlIndex = NULL;
	if( uiSqiUnit==0 )
	{
		pusPortControlIndex = ausPortcontrol_Index_SQI0_CS0;
	}
	else if( uiSqiUnit==1 )
	{
		pusPortControlIndex = ausPortcontrol_Index_SQI1_CS0;
	}
	if( pusPortControlIndex!=NULL )
	{
		portcontrol_restore(pusPortControlIndex, sizeof(ausPortcontrol_Index_SQI0_CS0)/sizeof(ausPortcontrol_Index_SQI0_CS0[0]));
	}
#endif

#if ASIC_TYP==ASIC_TYP_NETX56
	/* This is netX56 specific: Enable the SQI ROM pins. */
	ulValue  = ptAsicCtrlArea->ulIo_config2;
	ulValue &= ~HOSTMSK(io_config2_sel_sqi);
	ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;
	ptAsicCtrlArea->ulIo_config2 = ulValue;
#endif

}


int boot_drv_sqi_init(SPI_CFG_T *ptCfg, const BOOT_SPI_CONFIGURATION_T *ptSpiCfg, unsigned int uiSqiUnit, unsigned int uiChipSelect)
{
#if ASIC_TYP==ASIC_TYP_NETX500
#       error "netX500 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX50
#       error "netX50 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX10
#       error "netX10 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX56
	HOSTDEF(ptSqiArea);
	HOSTDEF(ptAsicCtrlArea);
	HOSTADEF(SQI) * ptSqi;

#elif ASIC_TYP==ASIC_TYP_NETX6
#       error "netX6 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
	HOSTDEF(ptSQI0Area);
	HOSTDEF(ptSQI1Area);
	HOSTADEF(SQI) * ptSqi;
	const unsigned short *pusPortControlIndex;

#elif ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
	HOSTDEF(ptSqiArea);
	HOSTADEF(SQI) * ptSqi;

#elif ASIC_TYP==ASIC_TYP_NETX90_MPW_APP
#       error "netX90 MPW APP is not yet supported"

#else
#       error "Invalid ASIC_TYP!"
#endif

	void *pvSqiRom;
	unsigned long ulValue;
	int iResult;
	unsigned int uiIdleCfg;


	ptSqi = NULL;

#if ASIC_TYP==ASIC_TYP_NETX500
#       error "netX500 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX50
#       error "netX50 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX10
#       error "netX10 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX56
	if( uiSqiUnit==0 )
	{
		ptSqi = ptSqiArea;
		pvSqiRom = (unsigned long*)Addr_NX56_sqirom;
	}

#elif ASIC_TYP==ASIC_TYP_NETX6
#       error "netX6 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
	if( uiSqiUnit==0 )
	{
		ptSqi = ptSQI0Area;
		pvSqiRom = (unsigned long*)HOSTADDR(NX2RAP_SQIROM0);
		pusPortControlIndex = ausPortcontrol_Index_SQI0_CS0;
	}
	else if( uiSqiUnit==1 )
	{
		ptSqi = ptSQI1Area;
		pvSqiRom = (unsigned long*)HOSTADDR(NX2RAP_SQIROM1);
		pusPortControlIndex = ausPortcontrol_Index_SQI1_CS0;
	}

#elif ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
	if( uiSqiUnit==0 )
	{
		ptSqi = ptSqiArea;
		pvSqiRom = (unsigned long*)HOSTADDR(sqirom);
	}

#elif ASIC_TYP==ASIC_TYP_NETX90_MPW_APP
#       error "netX90 MPW APP is not yet supported"

#else
#       error "Invalid ASIC_TYP!"
#endif

	if( ptSqi==NULL )
	{
		/* Error: the unit is invalid! */
		uprintf("[BootDrvSqi] Invalid unit: %d\n", uiSqiUnit);
		iResult = -1;
	}
	else
	{
		ptCfg->pvArea = ptSqi;
		ptCfg->pvSqiRom = pvSqiRom;
		ptCfg->ulSpeedFifoKhz = ptSpiCfg->ulSpeedFifoKhz;       /* Speed for the FIFO mode in kHz. */
		ptCfg->ulSpeedSqiRomKhz = ptSpiCfg->ulSpeedSqiRomKhz;   /* Speed for SQI ROM mode in kHz. */
		ptCfg->ucDummyByte = ptSpiCfg->ucDummyByte;             /* The idle configuration. */
		ptCfg->uiIdleConfiguration = (unsigned int)(ptSpiCfg->ucIdleConfiguration);
		ptCfg->tMode = ptSpiCfg->ucMode;                        /* Bus mode. */
		ptCfg->uiUnit = uiSqiUnit;                              /* the unit */
		ptCfg->uiChipSelect = 1U<<uiChipSelect;                 /* Chip select. */

		/* Set the function pointers. */
		ptCfg->pfnSelect = qsi_slave_select;
		ptCfg->pfnExchangeByte = qsi_exchange_byte;
		ptCfg->pfnSendIdleCycles = qsi_send_idle_cycles;
		ptCfg->pfnSendDummy = qsi_send_dummy;
		ptCfg->pfnSendData = qsi_send_data;
		ptCfg->pfnReceiveData = qsi_receive_data;
		ptCfg->pfnExchangeData = qsi_exchange_data;
		ptCfg->pfnSetNewSpeed = qsi_set_new_speed;
		ptCfg->pfnGetDeviceSpeedRepresentation = qsi_get_device_speed_representation;
		ptCfg->pfnReconfigureIos = qsi_reconfigure_ios;
		ptCfg->pfnSetBusWidth = qsi_set_bus_width;
		ptCfg->pfnGetDeviceSpecificSqiRomCfg = qsi_get_device_specific_sqirom_cfg;
		ptCfg->pfnActivateSqiRom = qsi_activate_sqirom;
		ptCfg->pfnDeactivateSqiRom = qsi_deactivate_sqi_rom;
		ptCfg->pfnDeactivate = qsi_deactivate;

#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
		/* Set up the port control unit. */
		portcontrol_apply(pusPortControlIndex, ptSpiCfg->ausPortControl, sizeof(ptSpiCfg->ausPortControl)/sizeof(ptSpiCfg->ausPortControl[0]));
#endif

		/* Do not use IRQs in boot loader. */
		ptSqi->ulSqi_irq_mask = 0;
		/* Clear all pending IRQs. */
		ulValue  = HOSTMSK(sqi_irq_clear_RORIC);
		ulValue |= HOSTMSK(sqi_irq_clear_RTIC);
		ulValue |= HOSTMSK(sqi_irq_clear_RXIC);
		ulValue |= HOSTMSK(sqi_irq_clear_TXIC);
		ulValue |= HOSTMSK(sqi_irq_clear_rxneic);
		ulValue |= HOSTMSK(sqi_irq_clear_rxfic);
		ulValue |= HOSTMSK(sqi_irq_clear_txeic);
		ulValue |= HOSTMSK(sqi_irq_clear_trans_end);
		ptSqi->ulSqi_irq_clear = ulValue;

		/* Do not use DMAs. */
		ptSqi->ulSqi_dmacr = 0;

		/* Do not use XIP. */
		ptSqi->ulSqi_sqirom_cfg = 0;

		/* Set 8 bits. */
		ulValue  = 7 << HOSTSRT(sqi_cr0_datasize);
		/* Set speed and filter. */
		ulValue |= qsi_get_device_speed_representation(ptCfg->ulSpeedFifoKhz);
		/* Start in SPI mode: use only IO0 and IO1 for transfer. */
		ulValue |= 0 << HOSTSRT(sqi_cr0_sio_cfg);
		/* Set the clock polarity.  */
		if( (ptCfg->tMode==SPI_MODE2) || (ptCfg->tMode==SPI_MODE3) )
		{
			ulValue |= HOSTMSK(sqi_cr0_sck_pol);
		}
		/* Set the clock phase. */
		if( (ptCfg->tMode==SPI_MODE1) || (ptCfg->tMode==SPI_MODE3) )
		{
			ulValue |= HOSTMSK(sqi_cr0_sck_phase);
		}
		ptSqi->aulSqi_cr[0] = ulValue;


		/* Set the chip select to manual mode. */
		ulValue  = HOSTMSK(sqi_cr1_fss_static);
		/* Manual transfer start. */
		ulValue |= HOSTMSK(sqi_cr1_spi_trans_ctrl);
		/* Enable the interface. */
		ulValue |= HOSTMSK(sqi_cr1_sqi_en);
		/* Clear both FIFOs. */
		ulValue |= HOSTMSK(sqi_cr1_rx_fifo_clr)|HOSTMSK(sqi_cr1_tx_fifo_clr);
		ptSqi->aulSqi_cr[1] = ulValue;


		uiIdleCfg = ptCfg->uiIdleConfiguration;
		
		/* Set transfer control base. */
		ulValue  = HOSTMSK(sqi_tcr_ms_bit_first);
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO1_OE)!=0 )
		{
			ulValue |= HOSTMSK(sqi_tcr_tx_oe);
		}
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO1_OUT)!=0 )
		{
			ulValue |= HOSTMSK(sqi_tcr_tx_out);
		}
		ptCfg->ulTrcBase = ulValue;
		ptSqi->ulSqi_tcr = ulValue;

		ulValue = 0;
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO2_OUT)!=0 )
		{
			ulValue |= HOSTMSK(sqi_pio_out_sio2);
		}
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO3_OUT)!=0 )
		{
			ulValue |= HOSTMSK(sqi_pio_out_sio3);
		}
		ptSqi->ulSqi_pio_out = ulValue;

		ulValue = 0;
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO2_OE)!=0 )
		{
			ulValue |= HOSTMSK(sqi_pio_oe_sio2);
		}
		if( (uiIdleCfg&MSK_SQI_CFG_IDLE_IO3_OE)!=0 )
		{
			ulValue |= HOSTMSK(sqi_pio_oe_sio3);
		}
		ptSqi->ulSqi_pio_oe = ulValue;

#if ASIC_TYP==ASIC_TYP_NETX56
		/* This is netX56 specific: Enable the SQI ROM pins. */
		ulValue  = ptAsicCtrlArea->ulIo_config2;
		ulValue |= HOSTMSK(io_config2_sel_sqi);
		ptAsicCtrlArea->ulAsic_ctrl_access_key = ptAsicCtrlArea->ulAsic_ctrl_access_key;
		ptAsicCtrlArea->ulIo_config2 = ulValue;
#endif

		iResult = 0;
	}

	return iResult;
}
