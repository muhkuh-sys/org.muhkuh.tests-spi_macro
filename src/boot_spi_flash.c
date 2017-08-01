#include "boot_drv_sqi.h"
#include "boot_drv_spi.h"
#include "boot_spi_flash.h"

#include <string.h>

#include "boot_common.h"
#include "bootblock_oldstyle.h"
#include "options.h"
#include "sha384.h"
#include "spi_macro_player.h"
#include "trace.h"


#define SFDP_MAGIC 0x50444653

#define SFDP_BASIC_PARAMETER_ID 0

/*-------------------------------------*/

#define SPI_FLASH_FLAGS_Forbid_Autodetect    0x00000001U


/* this is the runtime structure */
typedef struct STRUCT_BOOT_SPI_FLASH_HANDLE
{
	SPI_CFG_T tCfg;
	unsigned long ulInitialOffset;
	unsigned long ulOffset;
	unsigned long ulFlags;
	unsigned int uiPageAdrShift;
	unsigned int uiMode_0Fifo_1Rom;
	unsigned long ulSfdpLastAddress;
} BOOT_SPI_FLASH_HANDLE_T;


typedef struct SFDP_HEADER_STRUCT
{
	unsigned int uiJedecID;
	unsigned int uiParameterVersionMajor;
	unsigned int uiParameterVersionMinor;
	unsigned int uiParameterSizeDw;
	unsigned long ulParameterTablePointer;
} SFDP_HEADER_T;


/*-------------------------------------*/


/* This is used to leave any continuous mode. */
static const unsigned char aucFFFF[2] =
{
	0xff, 0xff
};


/*-------------------------------------*/



static unsigned int getMaskLength(unsigned long ulVal)
{
	unsigned long ulMask;
	unsigned int  uiBitCnt;


	ulMask   = 0U;
	uiBitCnt = 0U;

	/*  generate mask and count used bits */
	while( ulMask<ulVal )
	{
		/* shift in one more '1' to the mask value */
		ulMask <<= 1U;
		ulMask  |= 1U;
		/* count the new digit */
		++uiBitCnt;
	}

	return uiBitCnt;
}



static void update_page_address_shift(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	unsigned char ucFlags;
	unsigned int uiPageAdrShift;


	/* Get the new page address shift. */
	uiPageAdrShift = 0U;
	ucFlags  = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucFlags;
	ucFlags &= SPI_FLASH_FLAGS_Atmel_address;
	if( ucFlags!=0U )
	{
		uiPageAdrShift = getMaskLength(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].usAtmelDataflashPageSize);
	}
	ptFlashHandle->uiPageAdrShift = uiPageAdrShift;
}



static int send_read_command(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, unsigned long ulAddress)
{
	unsigned char aucReadCommand[5];
	unsigned char ucFlags;
	unsigned int sizReadCommand;
	unsigned int sizDummyBytes;
	unsigned int sizIdleCycles;
	int iResult;
	unsigned long ulPageNr;
	unsigned long ulByteOffset;
	SPI_BUS_WIDTH_T tBusWidthCommand;
	SPI_BUS_WIDTH_T tBusWidthAddress;
	SPI_BUS_WIDTH_T tBusWidthDummyIdle;
	SPI_BUS_WIDTH_T tBusWidthData;


	ucFlags = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandBusWidths;
	tBusWidthCommand   = (SPI_BUS_WIDTH_T)( ucFlags      & 3U);
	tBusWidthAddress   = (SPI_BUS_WIDTH_T)((ucFlags>>2U) & 3U);
	tBusWidthDummyIdle = (SPI_BUS_WIDTH_T)((ucFlags>>4U) & 3U);
	tBusWidthData      = (SPI_BUS_WIDTH_T)((ucFlags>>6U) & 3U);

	ucFlags = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucFlags;
	if( (ucFlags&SPI_FLASH_FLAGS_Atmel_address)!=0U )
	{
		/* Translate the linear address into the Atmel data flash format. */

		/* Get the page number of the address. */
		ulPageNr = ulAddress / g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].usAtmelDataflashPageSize;

		/* Get the byte ulOffsModuloet of the address. */
		ulByteOffset = ulAddress % g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].usAtmelDataflashPageSize;

		/*  shift the sector number to the right position */
		ulAddress = (ulPageNr << ptFlashHandle->uiPageAdrShift) | ulByteOffset;
	}

	trace_message_ul(TRACEMSG_SpiFlash_NewAddress, ulAddress);

	/* Check if the address does not exceed 16MB for devices with 3 address bytes. */
	if( (ucFlags&SPI_FLASH_FLAGS_4_bit_address)==0U &&
	    ulAddress>=0x01000000U )
	{
		/* The address can not represented by the number of available address bytes. */
		trace_message(TRACEMSG_SpiFlash_AddressExceedsDevice);
		iResult = -1;
	}
	else
	{
		/* Construct the read command. */
		aucReadCommand[0] = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommand;
		sizReadCommand = 1;
		if( (ucFlags&SPI_FLASH_FLAGS_4_bit_address)!=0U )
		{
			aucReadCommand[sizReadCommand] = (unsigned char)((ulAddress >> 24U) & 0xffU);
			++sizReadCommand;
		}
		aucReadCommand[sizReadCommand] = (unsigned char)((ulAddress >> 16U) & 0xffU);
		++sizReadCommand;
		aucReadCommand[sizReadCommand] = (unsigned char)((ulAddress >>  8U) & 0xffU);
		++sizReadCommand;
		aucReadCommand[sizReadCommand] = (unsigned char)( ulAddress         & 0xffU);
		++sizReadCommand;

		/* Select the slave. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 1);

		iResult = ptFlashHandle->tCfg.pfnSetBusWidth(&(ptFlashHandle->tCfg), tBusWidthCommand);
		if( iResult==0 )
		{
			iResult = ptFlashHandle->tCfg.pfnSendData(&(ptFlashHandle->tCfg), aucReadCommand, 1U);
			if( iResult==0 )
			{
				iResult = ptFlashHandle->tCfg.pfnSetBusWidth(&(ptFlashHandle->tCfg), tBusWidthAddress);
				if( iResult==0 )
				{
					iResult = ptFlashHandle->tCfg.pfnSendData(&(ptFlashHandle->tCfg), aucReadCommand+1U, sizReadCommand-1);
					if( iResult==0 )
					{
						sizDummyBytes = (size_t)(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandDummyBytes);
						sizIdleCycles = (size_t)(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandIdleCycles);
						if( (sizDummyBytes+sizIdleCycles)!=0 )
						{
							iResult = ptFlashHandle->tCfg.pfnSetBusWidth(&(ptFlashHandle->tCfg), tBusWidthDummyIdle);
							if( iResult==0 )
							{
								if( sizDummyBytes!=0 )
								{
									iResult = ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), sizDummyBytes);
								}
								if( iResult==0 )
								{
									if( sizIdleCycles!=0 )
									{
										iResult = ptFlashHandle->tCfg.pfnSendIdleCycles(&(ptFlashHandle->tCfg), sizIdleCycles);
									}
								}
							}
						}

						iResult = ptFlashHandle->tCfg.pfnSetBusWidth(&(ptFlashHandle->tCfg), tBusWidthData);
					}
				}
			}
		}
	}

	return iResult;
}



/*-------------------------------------------------------------------------*/


static int change_transport(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	unsigned long ulDeviceSpecificValue;
	int iResult;


	if( ptFlashHandle->uiMode_0Fifo_1Rom==0U )
	{
		/* Set the new speed. */
		ulDeviceSpecificValue = ptFlashHandle->tCfg.pfnGetDeviceSpeedRepresentation(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].uiMaximumSpeedInFifoMode_kHz);
		iResult = ptFlashHandle->tCfg.pfnSetNewSpeed(&(ptFlashHandle->tCfg), ulDeviceSpecificValue);
		if( iResult==0 )
		{
			/* Send the read command. */
			iResult = send_read_command(ptFlashHandle, ptFlashHandle->ulOffset);
		}
	}
	else
	{
		/* Set the new configuration. */
		ulDeviceSpecificValue = g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].ulSqiRomCfg;
		iResult = ptFlashHandle->tCfg.pfnActivateSqiRom(&(ptFlashHandle->tCfg), ulDeviceSpecificValue);
	}

	return iResult;
}



/*-------------------------------------------------------------------------*/


static int transport_spi_flash_get_area(unsigned long *pulData, size_t sizDwords, void *pvUser)
{
	HOSTDEF(ptCryptArea);
	int iResult;
	BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle;
	unsigned char *pucData;
	unsigned int sizData;
	unsigned long *pulCnt;
	unsigned long *pulEnd;
	unsigned long ulData;


	/* Get the pointer to the configuration from the user data. */
	ptFlashHandle = (BOOT_SPI_FLASH_HANDLE_T*)pvUser;

	if( ptFlashHandle->uiMode_0Fifo_1Rom==0U )
	{
		/* Translate the unsigned long arguments to unsigned char. */
		pucData = (unsigned char*)pulData;
		sizData = sizDwords * sizeof(unsigned long);

		/* Increase the offset. */
		ptFlashHandle->ulOffset += sizData;

		/* Read the data. */
		iResult = ptFlashHandle->tCfg.pfnReceiveData(&(ptFlashHandle->tCfg), pucData, sizData, 1);
	}
	else
	{
		/* Get the pointer to the start and the end of the data array. */
		pulCnt  = (unsigned long*)(ptFlashHandle->tCfg.pvSqiRom);
		pulCnt += ptFlashHandle->ulOffset / sizeof(unsigned long);
		pulEnd = pulCnt + sizDwords;

		/* Increase the offset. */
		ptFlashHandle->ulOffset += sizDwords * sizeof(unsigned long);

		/* Copy all requested data. */
		while( pulCnt<pulEnd )
		{
			ulData = *(pulCnt++);
			*(pulData++) = ulData;
			sha384_update_ul(ulData);
		}

		iResult = 0;
	}

	return iResult;
}



static int transport_spi_flash_get_dword(unsigned long *pulData, void *pvUser)
{
	return transport_spi_flash_get_area(pulData, 1, pvUser);
}



static int transport_spi_flash_stop(void *pvUser)
{
	BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle;


	/* Get the pointer to the configuration from the user data. */
	ptFlashHandle = (BOOT_SPI_FLASH_HANDLE_T*)pvUser;

	if( ptFlashHandle->uiMode_0Fifo_1Rom==0U )
	{
		/* Deselect the slave. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		/* Chip select must be low for a device specific time. */
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);
	}

	return 0;
}



static int transport_spi_flash_restart(void *pvUser)
{
	int iResult;
	BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle;
	unsigned int sizMacro;
	const unsigned char *pucMacro;
	SPI_MACRO_HANDLE_T tSpiMacro;
	SPI_MACRO_CHANGE_TRANSPORT_T tChangeTransport;
	BOOTING_T tResult;
	unsigned long ulValue;


	/* Get the pointer to the configuration from the user data. */
	ptFlashHandle = (BOOT_SPI_FLASH_HANDLE_T*)pvUser;

	/* Reconfigure the IOs. */
	ptFlashHandle->tCfg.pfnReconfigureIos(&(ptFlashHandle->tCfg));

	/* Get the new page address shift. */
	update_page_address_shift(ptFlashHandle);

	/* Is there an activation macro? */
	sizMacro = (size_t)(g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].aucSeqActivate[0]);
	if( sizMacro==0 )
	{
		/* No macro. Do not change the transport. */
		iResult = 0;
		tChangeTransport = SPI_MACRO_CHANGE_TRANSPORT_No_Change;
	}
	else
	{
		pucMacro = g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].aucSeqActivate + 1U;
		spi_macro_initialize(&tSpiMacro, &(ptFlashHandle->tCfg), pucMacro, sizMacro);
		tResult = spi_macro_player_run(&tSpiMacro);
		if( tResult!=BOOTING_Ok )
		{
			iResult = -1;
		}
		else
		{
			/*  Deselect the slave. */
			ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);

			/* Send 1 idle byte. */
			ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

			/* The macro finished with success. Now compare the magic value. */
			ulValue = tSpiMacro.uRxBuffer.aul[0];
			if( (ulValue&BOOTBLOCK_OLDSTYLE_MAGIC_MASK)!=g_t_romloader_options.t_system_config.ulBootBlockOldstyleMagic )
			{
				/* The read value is no magic.
				 * Either the operation did not succeed, or the device does not contain a boot-able image.
				 */
				trace_message_ul(TRACEMSG_BootBlock_NoMagicCookie, ulValue);
				iResult = -1;
			}
			else
			{
				iResult = 0;
				tChangeTransport = tSpiMacro.tChangeTransport;
			}
		}

		/* Do not play the macro twice. */
		g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].aucSeqActivate[0] = 0U;
	}

	if( iResult==0 )
	{
		switch(tChangeTransport)
		{
		case SPI_MACRO_CHANGE_TRANSPORT_No_Change:
			/* Keep the transport the same. */
			break;

		case SPI_MACRO_CHANGE_TRANSPORT_FIFO:
			/* Switch to FIFO mode. */
			ptFlashHandle->uiMode_0Fifo_1Rom = 0U;
			break;

		case SPI_MACRO_CHANGE_TRANSPORT_ROM:
			/* Switch to ROM mode. */
			ptFlashHandle->uiMode_0Fifo_1Rom = 1U;
			break;
		}
		iResult = change_transport(ptFlashHandle);
	}

	return iResult;
}



static int transport_spi_flash_play_spi_macro(unsigned int uiSpiUnitOffset, const unsigned char *pucMacro, unsigned int sizMacro, void *pvUser)
{
	SPI_MACRO_HANDLE_T tSpiMacro;
	int iResult;
	BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle;
	SPI_UNIT_OFFSET_T tSpiUnitOffset;
	PFN_SPI_INIT pfnInit;
	unsigned int uiSpiUnit;
	unsigned int uiChipSelect;
	BOOT_SPI_FLASH_CONFIGURATION_T *ptOpt;
	BOOTING_T tResult;
	SPI_CFG_T tCfg;


	/* Get the pointer to the configuration from the user data. */
	ptFlashHandle = (BOOT_SPI_FLASH_HANDLE_T*)pvUser;

	tSpiUnitOffset = (SPI_UNIT_OFFSET_T)uiSpiUnitOffset;

	uiSpiUnit    = 0U;
	uiChipSelect = 0U;

	/* Is this the current unit? */
	if( tSpiUnitOffset==SPI_UNIT_OFFSET_CURRENT )
	{
		/* This is the current unit.
		 * Do not initialize or deactivate.
		 */
		pfnInit = NULL;
		iResult = 0;
	}
	else
	{
		pfnInit = boot_spi_offset_to_unit(tSpiUnitOffset, &uiSpiUnit, &uiChipSelect);
		if( pfnInit==NULL )
		{
			/* Unknown unit. */
			iResult = -1;
		}
		else if( uiSpiUnit==ptFlashHandle->tCfg.uiUnit && uiChipSelect==ptFlashHandle->tCfg.uiChipSelect )
		{
			/* The offset matches the current unit.
			 * Do not initialize or deactivate.
			 */
			pfnInit = NULL;
			iResult = 0;
		}
		else
		{
			iResult = 0;
		}
	}

	if( iResult==0 )
	{
		/* Must the unit be initialized? */
		if( pfnInit==NULL )
		{
			/* Stop the current read command. */
			iResult = transport_spi_flash_stop(pvUser);
			if( iResult==0 )
			{
				spi_macro_initialize(&tSpiMacro, &tCfg, pucMacro, sizMacro);
				tResult = spi_macro_player_run(&tSpiMacro);
				if( tResult!=BOOTING_Ok )
				{
					iResult = -1;
				}
				else
				{
					iResult = transport_spi_flash_restart(pvUser);
				}
			}
		}
		else
		{
			/* Get the device configuration. */
			ptOpt = g_t_romloader_options.atSpiFlashCfg + ((unsigned int)tSpiUnitOffset);

			/* Initialize the configuration and the unit.*/
			pfnInit(&tCfg, &(ptOpt->tSpiCfg), uiSpiUnit, uiChipSelect);

			spi_macro_initialize(&tSpiMacro, &tCfg, pucMacro, sizMacro);
			tResult = spi_macro_player_run(&tSpiMacro);

			tCfg.pfnDeactivate(&tCfg);

			if( tResult!=BOOTING_Ok )
			{
				iResult = -1;
			}
		}
	}

	return iResult;
}



static int transport_spi_skip(unsigned int sizDwords, void *pvUser)
{
	BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle;
	int iResult;
	unsigned long ulNewOffset;


	/* Expect success. */
	iResult = 0;

	/* Get the pointer to the configuration from the user data. */
	ptFlashHandle = (BOOT_SPI_FLASH_HANDLE_T*)pvUser;

	/* Increment the address. */
	ulNewOffset = ptFlashHandle->ulOffset + sizDwords * sizeof(unsigned long);
	ptFlashHandle->ulOffset = ulNewOffset;

	if( ptFlashHandle->uiMode_0Fifo_1Rom==0U )
	{
		/* Deselect the slave. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		/* Chip select must be low for a device specific time. */
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

		/* Send the read command. */
		iResult = send_read_command(ptFlashHandle, ulNewOffset);
	}

	return iResult;
}



static const HBOOT_TRANSPORT_INTERFACE_T tTransportInterface_SpiFlash =
{
	transport_spi_flash_get_dword,
	transport_spi_flash_get_area,
	transport_spi_flash_stop,
	transport_spi_flash_restart,
	transport_spi_flash_play_spi_macro,
	transport_spi_skip
};



/*-------------------------------------------------------------------------*/


static void terminate_sfdp_reads(BOOT_SPI_FLASH_HANDLE_T *ptCfg)
{
	/*  Deselect the slave. */
	ptCfg->tCfg.pfnSelect(&(ptCfg->tCfg), 0);

	/* Send 1 idle byte. */
	ptCfg->tCfg.pfnSendDummy(&(ptCfg->tCfg), 1);
}



static int read_sfdp_data(BOOT_SPI_FLASH_HANDLE_T *ptCfg, unsigned char *pucBuffer, size_t sizBuffer, unsigned long ulAddress)
{
	unsigned char aucCmd[5];
	int iResult;


	iResult = 0;

	if( ulAddress!=ptCfg->ulSfdpLastAddress )
	{
		terminate_sfdp_reads(ptCfg);

		/* Construct the command. */
		aucCmd[0] = 0x5aU;                                       /* This is the SFDP command. */
		aucCmd[1] = (unsigned char)((ulAddress>>16U) & 0xffU);   /* Address */
		aucCmd[2] = (unsigned char)((ulAddress>> 8U) & 0xffU);   /* Address */
		aucCmd[3] = (unsigned char)( ulAddress       & 0xffU);   /* Address */
		aucCmd[4] = 0x00U;                                       /* Dummy cycle. */

		/* Select the slave. */
		ptCfg->tCfg.pfnSelect(&(ptCfg->tCfg), 1);

		/* Send the command. */
		iResult = ptCfg->tCfg.pfnSendData(&(ptCfg->tCfg), aucCmd, sizeof(aucCmd));
	}

	if( iResult==0 )
	{
		/* Read the data. */
		iResult = ptCfg->tCfg.pfnReceiveData(&(ptCfg->tCfg), pucBuffer, sizBuffer, 0);

		ptCfg->ulSfdpLastAddress = ulAddress + sizBuffer;
	}

	return iResult;
}



static int read_sfdp_header(BOOT_SPI_FLASH_HANDLE_T *ptCfg, unsigned int uiHeaderNr, SFDP_HEADER_T *ptHeader)
{
	unsigned long ulAddress;
	unsigned char aucHeader[8];
	unsigned long ulValue;
	unsigned int uiValue;
	int iResult;


	/* Get the address of the header. */
	ulAddress = 8 + uiHeaderNr * 8;

	/* Read the header. */
	iResult = read_sfdp_data(ptCfg, aucHeader, sizeof(aucHeader), ulAddress);
	if( iResult==0 )
	{
		uiValue  =  (unsigned int)(aucHeader[0]);
		uiValue |= ((unsigned int)(aucHeader[7])) << 8U;
		ptHeader->uiJedecID = uiValue;

		ptHeader->uiParameterVersionMinor = (unsigned int)(aucHeader[1]);
		ptHeader->uiParameterVersionMajor = (unsigned int)(aucHeader[2]);

		ptHeader->uiParameterSizeDw = (unsigned int)(aucHeader[3]);

		ulValue  = ((unsigned long)(aucHeader[4]));
		ulValue |= ((unsigned long)(aucHeader[5])) <<  8U;
		ulValue |= ((unsigned long)(aucHeader[6])) << 16U;
		ptHeader->ulParameterTablePointer = ulValue;
	}

	return iResult;
}


/* Definitions for the JEDEC Basic Flash Parameter table. */
#define OFF_SFDP_BFP_BlockSectorEraseSizes 0U
#define MSK_SFDP_BFP_BlockSectorEraseSizes 0x00000003U
#define SRT_SFDP_BFP_BlockSectorEraseSizes 0U

#define OFF_SFDP_BFP_WriteGranularity 0U
#define MSK_SFDP_BFP_WriteGranularity 0x00000004U
#define SRT_SFDP_BFP_WriteGranularity 2U

#define OFF_SFDP_BFP_VolatileStatusRegisterBlockProtectBits 0U
#define MSK_SFDP_BFP_VolatileStatusRegisterBlockProtectBits 0x00000008U
#define SRT_SFDP_BFP_VolatileStatusRegisterBlockProtectBits 3U

#define OFF_SFDP_BFP_WriteEnableInstructionSelectForWritingToVolatileStatusRegister 0U
#define MSK_SFDP_BFP_WriteEnableInstructionSelectForWritingToVolatileStatusRegister 0x00000010U
#define SRT_SFDP_BFP_WriteEnableInstructionSelectForWritingToVolatileStatusRegister 4U

#define OFF_SFDP_BFP_4KilobyteEraseInstruction 0U
#define MSK_SFDP_BFP_4KilobyteEraseInstruction 0x0000ff00U
#define SRT_SFDP_BFP_4KilobyteEraseInstruction 8U

#define OFF_SFDP_BFP_Supports112FastRead 0U
#define MSK_SFDP_BFP_Supports112FastRead 0x00010000U
#define SRT_SFDP_BFP_Supports112FastRead 16U

#define OFF_SFDP_BFP_AddressBytes 0U
#define MSK_SFDP_BFP_AddressBytes 0x00060000U
#define SRT_SFDP_BFP_AddressBytes 17U

#define OFF_SFDP_BFP_SupportsDoubleTransferRateClocking 0U
#define MSK_SFDP_BFP_SupportsDoubleTransferRateClocking 0x00080000U
#define SRT_SFDP_BFP_SupportsDoubleTransferRateClocking 19U

#define OFF_SFDP_BFP_Supports122FastRead 0U
#define MSK_SFDP_BFP_Supports122FastRead 0x00100000U
#define SRT_SFDP_BFP_Supports122FastRead 20U

#define OFF_SFDP_BFP_Supports144FastRead 0U
#define MSK_SFDP_BFP_Supports144FastRead 0x00200000U
#define SRT_SFDP_BFP_Supports144FastRead 21U

#define OFF_SFDP_BFP_Supports114FastRead 0U
#define MSK_SFDP_BFP_Supports114FastRead 0x00400000U
#define SRT_SFDP_BFP_Supports114FastRead 22U

#define OFF_SFDP_BFP_FlashMemoryDensity 1U
#define MSK_SFDP_BFP_FlashMemoryDensity 0xffffffffU
#define SRT_SFDP_BFP_FlashMemoryDensity 0U

#define OFF_SFDP_BFP_144FastReadNumberOfWaitStates 2
#define MSK_SFDP_BFP_144FastReadNumberOfWaitStates 0x0000001f
#define SRT_SFDP_BFP_144FastReadNumberOfWaitStates 0

#define OFF_SFDP_BFP_144FastReadNumberOfModeClocks 2
#define MSK_SFDP_BFP_144FastReadNumberOfModeClocks 0x000000e0
#define SRT_SFDP_BFP_144FastReadNumberOfModeClocks 5

#define OFF_SFDP_BFP_144FastReadInstruction 2
#define MSK_SFDP_BFP_144FastReadInstruction 0x0000ff00
#define SRT_SFDP_BFP_144FastReadInstruction 8

#define OFF_SFDP_BFP_114FastReadNumberOfWaitStates 2
#define MSK_SFDP_BFP_114FastReadNumberOfWaitStates 0x001f0000
#define SRT_SFDP_BFP_114FastReadNumberOfWaitStates 16

#define OFF_SFDP_BFP_114FastReadNumberOfModeClocks 2
#define MSK_SFDP_BFP_114FastReadNumberOfModeClocks 0x00e00000
#define SRT_SFDP_BFP_114FastReadNumberOfModeClocks 21

#define OFF_SFDP_BFP_114FastReadInstruction 2
#define MSK_SFDP_BFP_114FastReadInstruction 0xff000000
#define SRT_SFDP_BFP_114FastReadInstruction 24

#define OFF_SFDP_BFP_112FastReadNumberOfWaitStates 3
#define MSK_SFDP_BFP_112FastReadNumberOfWaitStates 0x0000001fU
#define SRT_SFDP_BFP_112FastReadNumberOfWaitStates 0

#define OFF_SFDP_BFP_112FastReadNumberOfModeClocks 3
#define MSK_SFDP_BFP_112FastReadNumberOfModeClocks 0x000000e0
#define SRT_SFDP_BFP_112FastReadNumberOfModeClocks 5

#define OFF_SFDP_BFP_112FastReadInstruction 3
#define MSK_SFDP_BFP_112FastReadInstruction 0x0000ff00
#define SRT_SFDP_BFP_112FastReadInstruction 8

#define OFF_SFDP_BFP_122FastReadNumberOfWaitStates 3
#define MSK_SFDP_BFP_122FastReadNumberOfWaitStates 0x001f0000
#define SRT_SFDP_BFP_122FastReadNumberOfWaitStates 16

#define OFF_SFDP_BFP_122FastReadNumberOfModeClocks 3
#define MSK_SFDP_BFP_122FastReadNumberOfModeClocks 0x00e00000
#define SRT_SFDP_BFP_122FastReadNumberOfModeClocks 21

#define OFF_SFDP_BFP_122FastReadInstruction 3
#define MSK_SFDP_BFP_122FastReadInstruction 0xff000000
#define SRT_SFDP_BFP_122FastReadInstruction 24

#define OFF_SFDP_BFP_Supports222FastRead 4U
#define MSK_SFDP_BFP_Supports222FastRead 0x00000001U
#define SRT_SFDP_BFP_Supports222FastRead 0U

#define OFF_SFDP_BFP_Supports444FastRead 4U
#define MSK_SFDP_BFP_Supports444FastRead 0x00000010U
#define SRT_SFDP_BFP_Supports444FastRead 4U

#define OFF_SFDP_BFP_222FastReadNumberOfWaitStates 5U
#define MSK_SFDP_BFP_222FastReadNumberOfWaitStates 0x001f0000
#define SRT_SFDP_BFP_222FastReadNumberOfWaitStates 16

#define OFF_SFDP_BFP_222FastReadNumberOfModeClocks 5U
#define MSK_SFDP_BFP_222FastReadNumberOfModeClocks 0x00e00000
#define SRT_SFDP_BFP_222FastReadNumberOfModeClocks 21

#define OFF_SFDP_BFP_222FastReadInstruction 5U
#define MSK_SFDP_BFP_222FastReadInstruction 0xff000000
#define SRT_SFDP_BFP_222FastReadInstruction 24

#define OFF_SFDP_BFP_444FastReadNumberOfWaitStates 6U
#define MSK_SFDP_BFP_444FastReadNumberOfWaitStates 0x001f0000
#define SRT_SFDP_BFP_444FastReadNumberOfWaitStates 16

#define OFF_SFDP_BFP_444FastReadNumberOfModeClocks 6U
#define MSK_SFDP_BFP_444FastReadNumberOfModeClocks 0x00e00000
#define SRT_SFDP_BFP_444FastReadNumberOfModeClocks 21

#define OFF_SFDP_BFP_444FastReadInstruction 6U
#define MSK_SFDP_BFP_444FastReadInstruction 0xff000000
#define SRT_SFDP_BFP_444FastReadInstruction 24

#define OFF_SFDP_BFP_SectorType1Size 7U
#define MSK_SFDP_BFP_SectorType1Size 0x000000ffU
#define SRT_SFDP_BFP_SectorType1Size 0U

#define OFF_SFDP_BFP_SectorType1Instruction 7U
#define MSK_SFDP_BFP_SectorType1Instruction 0x0000ff00U
#define SRT_SFDP_BFP_SectorType1Instruction 8U

#define OFF_SFDP_BFP_SectorType2Size 7U
#define MSK_SFDP_BFP_SectorType2Size 0x00ff0000U
#define SRT_SFDP_BFP_SectorType2Size 16U

#define OFF_SFDP_BFP_SectorType2Instruction 7U
#define MSK_SFDP_BFP_SectorType2Instruction 0xff000000U
#define SRT_SFDP_BFP_SectorType2Instruction 24U

#define OFF_SFDP_BFP_SectorType3Size 8U
#define MSK_SFDP_BFP_SectorType3Size 0x000000ffU
#define SRT_SFDP_BFP_SectorType3Size 0U

#define OFF_SFDP_BFP_SectorType3Instruction 8U
#define MSK_SFDP_BFP_SectorType3Instruction 0x0000ff00U
#define SRT_SFDP_BFP_SectorType3Instruction 8U

#define OFF_SFDP_BFP_SectorType4Size 8U
#define MSK_SFDP_BFP_SectorType4Size 0x00ff0000U
#define SRT_SFDP_BFP_SectorType4Size 16U

#define OFF_SFDP_BFP_SectorType4Instruction 8U
#define MSK_SFDP_BFP_SectorType4Instruction 0xff000000U
#define SRT_SFDP_BFP_SectorType4Instruction 24U

#define OFF_SFDP_BFP_MultiplierFromTypicalEraseTimeToMaximumEraseTime 9U
#define MSK_SFDP_BFP_MultiplierFromTypicalEraseTimeToMaximumEraseTime 0x0000000fU
#define SRT_SFDP_BFP_MultiplierFromTypicalEraseTimeToMaximumEraseTime 0U

#define OFF_SFDP_BFP_SectorType1EraseTypicalTime 9U
#define MSK_SFDP_BFP_SectorType1EraseTypicalTime 0x000007f0U
#define SRT_SFDP_BFP_SectorType1EraseTypicalTime 4U

#define OFF_SFDP_BFP_SectorType2EraseTypicalTime 9U
#define MSK_SFDP_BFP_SectorType2EraseTypicalTime 0x0003f800U
#define SRT_SFDP_BFP_SectorType2EraseTypicalTime 11U

#define OFF_SFDP_BFP_SectorType3EraseTypicalTime 9U
#define MSK_SFDP_BFP_SectorType3EraseTypicalTime 0x01fc0000U
#define SRT_SFDP_BFP_SectorType3EraseTypicalTime 18U

#define OFF_SFDP_BFP_SectorType4EraseTypicalTime 9U
#define MSK_SFDP_BFP_SectorType4EraseTypicalTime 0xfe000000U
#define SRT_SFDP_BFP_SectorType4EraseTypicalTime 25U

#define OFF_SFDP_BFP_MultiplierFromTypicalTimeToMaxTimeForPageOrByteProgram 10U
#define MSK_SFDP_BFP_MultiplierFromTypicalTimeToMaxTimeForPageOrByteProgram 0x0000000fU
#define SRT_SFDP_BFP_MultiplierFromTypicalTimeToMaxTimeForPageOrByteProgram 0U

#define OFF_SFDP_BFP_PageSize 10U
#define MSK_SFDP_BFP_PageSize 0x000000f0U
#define SRT_SFDP_BFP_PageSize 4U

#define OFF_SFDP_BFP_PageProgramTypicalTime 10U
#define MSK_SFDP_BFP_PageProgramTypicalTime 0x00003f00U
#define SRT_SFDP_BFP_PageProgramTypicalTime 8U

#define OFF_SFDP_BFP_ByteProgramTypicalTimeFirstByte 10U
#define MSK_SFDP_BFP_ByteProgramTypicalTimeFirstByte 0x0007c000U
#define SRT_SFDP_BFP_ByteProgramTypicalTimeFirstByte 14U

#define OFF_SFDP_BFP_ByteProgramTypicalTimeAdditionalByte 10U
#define MSK_SFDP_BFP_ByteProgramTypicalTimeAdditionalByte 0x00f80000U
#define SRT_SFDP_BFP_ByteProgramTypicalTimeAdditionalByte 19U

#define OFF_SFDP_BFP_ChipEraseTypicalTime 10U
#define MSK_SFDP_BFP_ChipEraseTypicalTime 0xef000000U
#define SRT_SFDP_BFP_ChipEraseTypicalTime 24U

#define OFF_SFDP_BFP_ProhibitedOperationsDuringProgramSuspend 11U
#define MSK_SFDP_BFP_ProhibitedOperationsDuringProgramSuspend 0x0000000fU
#define SRT_SFDP_BFP_ProhibitedOperationsDuringProgramSuspend 0U

#define OFF_SFDP_BFP_ProhibitedOperationsDuringEraseSuspend 11U
#define MSK_SFDP_BFP_ProhibitedOperationsDuringEraseSuspend 0x000000f0U
#define SRT_SFDP_BFP_ProhibitedOperationsDuringEraseSuspend 4U

#define OFF_SFDP_BFP_ProgramResumeToSuspendInterval 11U
#define MSK_SFDP_BFP_ProgramResumeToSuspendInterval 0x00001e00U
#define SRT_SFDP_BFP_ProgramResumeToSuspendInterval 9U

#define OFF_SFDP_BFP_SuspendInProgressProgramMaxLatency 11U
#define MSK_SFDP_BFP_SuspendInProgressProgramMaxLatency 0x000fe000U
#define SRT_SFDP_BFP_SuspendInProgressProgramMaxLatency 13U

#define OFF_SFDP_BFP_EraseResumeToSuspendInterval 11U
#define MSK_SFDP_BFP_EraseResumeToSuspendInterval 0x00f00000U
#define SRT_SFDP_BFP_EraseResumeToSuspendInterval 20U

#define OFF_SFDP_BFP_SuspendInProgressEraseMaxLatency 11U
#define MSK_SFDP_BFP_SuspendInProgressEraseMaxLatency 0xef000000U
#define SRT_SFDP_BFP_SuspendInProgressEraseMaxLatency 24U

#define OFF_SFDP_BFP_ProgramResumeInstruction 12U
#define MSK_SFDP_BFP_ProgramResumeInstruction 0x000000ffU
#define SRT_SFDP_BFP_ProgramResumeInstruction 0U

#define OFF_SFDP_BFP_ProgramSuspendInstruction 12U
#define MSK_SFDP_BFP_ProgramSuspendInstruction 0x0000ff00U
#define SRT_SFDP_BFP_ProgramSuspendInstruction 8U

#define OFF_SFDP_BFP_ResumeInstruction 12U
#define MSK_SFDP_BFP_ResumeInstruction 0x00ff0000U
#define SRT_SFDP_BFP_ResumeInstruction 16U

#define OFF_SFDP_BFP_SuspendInstruction 12U
#define MSK_SFDP_BFP_SuspendInstruction 0xff000000U
#define SRT_SFDP_BFP_SuspendInstruction 24U

#define OFF_SFDP_BFP_StatusRegisterPollingDeviceBusy 13U
#define MSK_SFDP_BFP_StatusRegisterPollingDeviceBusy 0x000000fcU
#define SRT_SFDP_BFP_StatusRegisterPollingDeviceBusy 2U

#define OFF_SFDP_BFP_ExitDeepPowerdownToNextOperationDelay 13U
#define MSK_SFDP_BFP_ExitDeepPowerdownToNextOperationDelay 0x00007f00U
#define SRT_SFDP_BFP_ExitDeepPowerdownToNextOperationDelay 8U

#define OFF_SFDP_BFP_ExitDeepPowerdownInstruction 13U
#define MSK_SFDP_BFP_ExitDeepPowerdownInstruction 0x007f8000U
#define SRT_SFDP_BFP_ExitDeepPowerdownInstruction 15U

#define OFF_SFDP_BFP_EnterDeepPowerdownInstruction 13U
#define MSK_SFDP_BFP_EnterDeepPowerdownInstruction 0x7f800000U
#define SRT_SFDP_BFP_EnterDeepPowerdownInstruction 23U

#define OFF_SFDP_BFP_DeepPowerdownSupported 13U
#define MSK_SFDP_BFP_DeepPowerdownSupported 0x80000000U
#define SRT_SFDP_BFP_DeepPowerdownSupported 31U

#define OFF_SFDP_BFP_444ModeDisableSequences 14U
#define MSK_SFDP_BFP_444ModeDisableSequences 0x0000000fU
#define SRT_SFDP_BFP_444ModeDisableSequences 0U

#define OFF_SFDP_BFP_444ModeEnableSequences 14U
#define MSK_SFDP_BFP_444ModeEnableSequences 0x000001f0U
#define SRT_SFDP_BFP_444ModeEnableSequences 4U

#define OFF_SFDP_BFP_044ModeSupported 14U
#define MSK_SFDP_BFP_044ModeSupported 0x00000200U
#define SRT_SFDP_BFP_044ModeSupported 9U

#define OFF_SFDP_BFP_044ModeExitMethod 14U
#define MSK_SFDP_BFP_044ModeExitMethod 0x0000fc00U
#define SRT_SFDP_BFP_044ModeExitMethod 10U

#define OFF_SFDP_BFP_044ModeEntryMethod 14U
#define MSK_SFDP_BFP_044ModeEntryMethod 0x000f0000U
#define SRT_SFDP_BFP_044ModeEntryMethod 16U

#define OFF_SFDP_BFP_QuadEnableRequirements 14U
#define MSK_SFDP_BFP_QuadEnableRequirements 0x00700000U
#define SRT_SFDP_BFP_QuadEnableRequirements 20U

#define OFF_SFDP_BFP_HOLDAndWPDisable 14U
#define MSK_SFDP_BFP_HOLDAndWPDisable 0x00800000U
#define SRT_SFDP_BFP_HOLDAndWPDisable 23U

#define OFF_SFDP_BFP_VolatileOrNonVolatileRegisterAndWriteEnableInstructionForStatusRegister1 15U
#define MSK_SFDP_BFP_VolatileOrNonVolatileRegisterAndWriteEnableInstructionForStatusRegister1 0x0000007fU
#define SRT_SFDP_BFP_VolatileOrNonVolatileRegisterAndWriteEnableInstructionForStatusRegister1 0U

#define OFF_SFDP_BFP_SoftResetAndRescueSequenceSupport 15U
#define MSK_SFDP_BFP_SoftResetAndRescueSequenceSupport 0x00003f00U
#define SRT_SFDP_BFP_SoftResetAndRescueSequenceSupport 8U

#define OFF_SFDP_BFP_Exit4ByteAddressing 15U
#define MSK_SFDP_BFP_Exit4ByteAddressing 0x00ffc000U
#define SRT_SFDP_BFP_Exit4ByteAddressing 14U

#define OFF_SFDP_BFP_Enter4ByteAddressing 15U
#define MSK_SFDP_BFP_Enter4ByteAddressing 0xff000000U
#define SRT_SFDP_BFP_Enter4ByteAddressing 24U



struct SFDP_MODE_ATTRIBUTES_STRUCT;

typedef BOOTING_T (*PFN_SFDP_SETUP_T)(BOOT_SPI_FLASH_HANDLE_T *ptCfg, const unsigned long *pulParameters);

typedef struct SFDP_MODE_ATTRIBUTES_STRUCT
{
	unsigned long ulSupports_OFF;
	unsigned long ulSupports_MSK;

	PFN_SFDP_SETUP_T pfnSfdpSetup;

	/* This is only for debugging. */
	unsigned long ulModeID;
} SFDP_MODE_ATTRIBUTES_T;



static BOOTING_T process_boot_image(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	BOOTBLOCK_OLDSTYLE_U_T tBootBlock;
	BOOTING_T tResult;
	int iResult;
	unsigned long ulCheckSum;
	unsigned long ulValue;
	unsigned int sizHashDwords;


	/* Receive the boot block. */
	iResult = tTransportInterface_SpiFlash.pfnGetArea(tBootBlock.aul, sizeof(BOOTBLOCK_OLDSTYLE_U_T)/sizeof(unsigned long), ptFlashHandle);
	if( iResult!=0 )
	{
		tResult = BOOTING_Transfer_Error;
	}
	else
	{
		/* does the header have a valid checksum and a valid signature? */
		ulCheckSum = bootblock_oldstyle_bootblock_checksum(tBootBlock.aul);
		if( ulCheckSum!=0 )
		{
			trace_message(TRACEMSG_BootBlock_ChecksumError);
			tResult = BOOTING_Header_Checksum_Invalid;
		}
		else if( (tBootBlock.s.ulMagic&BOOTBLOCK_OLDSTYLE_MAGIC_MASK)!=g_t_romloader_options.t_system_config.ulBootBlockOldstyleMagic )
		{
			trace_message_ul(TRACEMSG_BootBlock_NoMagicCookie, tBootBlock.s.ulMagic);
			tResult = BOOTING_Cookie_Invalid;
		}
		else if( tBootBlock.s.ulSignature!=BOOTBLOCK_HBOOT_SIGNATURE )
		{
			trace_message_ul(TRACEMSG_BootBlock_InvalidSignature, tBootBlock.s.ulSignature);
			tResult = BOOTING_Signature_Invalid;
		}
		else
		{
			/*
			 * The boot block is OK!
			 */

			/* Get the hash size. */
			ulValue  = tBootBlock.aul[7];
			ulValue &= 0x0000000fU;
			sizHashDwords = ((size_t)ulValue) + 1U;

			/* process a streamed HBoot image */
			tResult = boot_process_hboot_image(&tTransportInterface_SpiFlash, ptFlashHandle, sizHashDwords);
		}
	}

	return tResult;
}



static BOOTING_T sfdp10_generate_macro(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters, unsigned long ulModeBits, unsigned char ucIdleCycles, unsigned char ucOpcode, unsigned char ucBusWidths)
{
	BOOTING_T tResult;
	SPI_BUS_WIDTH_T tBusWidthCommand;
	SPI_BUS_WIDTH_T tBusWidthAddress;
	SPI_BUS_WIDTH_T tBusWidthData;
	unsigned char aucMacro[16];
	unsigned char *pucData;
	unsigned long ulValue;
	unsigned long ulAddressBytes;
	unsigned long ulSizeExponent;
	unsigned int sizMacro;
	SPI_MACRO_HANDLE_T tSpiMacro;
	unsigned int uiAddressBits;
	unsigned int uiAddressNibbles;
	int iResult;
	unsigned long ulDeviceSpecificValue;
	unsigned int uiFlashSupportsXip;
	unsigned char ucModeData;
	unsigned char ucFlags;


	/* No flags set by default. */
	ucFlags = 0;

	tBusWidthCommand = (SPI_BUS_WIDTH_T)( ucBusWidths      & 3U);
	tBusWidthAddress = (SPI_BUS_WIDTH_T)((ucBusWidths>>2U) & 3U);
	/* NOTE: the bus width for the dummy/idle phase is not used here. */
	tBusWidthData    = (SPI_BUS_WIDTH_T)((ucBusWidths>>6U) & 3U);

	/* Does the flash support XIP and can these settings be used for XIP?
	 * Both the address and the data must be 4 bit, and there must be 8 mode bits.
	 */
	if( (ptFlashHandle->tCfg.pvSqiRom!=NULL) &&
	    (tBusWidthAddress==SPI_BUS_WIDTH_4BIT) &&
	    (tBusWidthData==SPI_BUS_WIDTH_4BIT) &&
	    (ulModeBits==8) )
	{
		/* Yes, this unit supports XIP. */
		uiFlashSupportsXip = 1;
		/* Try to activate continuous mode to access the flash with the SQIROM unit. */
		ucModeData = 0xa5;
	}
	else
	{
		/* No, this unit can not be used with XIP. */
		uiFlashSupportsXip = 0;
		/* Do not activate continuous mode. This flash does not support SQIROM anyway. */
		ucModeData = 0x00;
	}

	/*
	 * Get the number of bits for the internal address calculation.
	 */
	ulValue   = pulParameters[OFF_SFDP_BFP_FlashMemoryDensity];
	ulValue  &= MSK_SFDP_BFP_FlashMemoryDensity;
	ulValue >>= SRT_SFDP_BFP_FlashMemoryDensity;
	if( (ulValue&0x80000000U)==0 )
	{
		ulSizeExponent = 0U;
		while( (1U<<ulSizeExponent)<ulValue )
		{
			++ulSizeExponent;
		}
	}
	else
	{
		/* Bits 0..30 are the size exponent. */
		ulSizeExponent = ulValue & 0x7fffffffU;
	}
	/* Size exponents smaller than 19 are not supported, but it does not hurt to use a larger address space. */
	if( ulSizeExponent<19 )
	{
		ulSizeExponent = 19;
	}
	/* Size exponents larger than 25 are not supported, but it does not hurt to use a smaller address space. */
	else if( ulSizeExponent>25 )
	{
		ulSizeExponent = 25;
	}
	uiAddressBits = ulSizeExponent + 1;


	/*
	 * Get the number of 4bit nibbles for an address in a command.
	 */
	ulAddressBytes   = pulParameters[OFF_SFDP_BFP_AddressBytes];
	ulAddressBytes  &= MSK_SFDP_BFP_AddressBytes;
	ulAddressBytes >>= SRT_SFDP_BFP_AddressBytes;
	if( ulAddressBytes==2 )
	{
		uiAddressNibbles = 8;
		ucFlags |= SPI_FLASH_FLAGS_4_bit_address;
	}
	else
	{
		uiAddressNibbles = 6;
	}


	/*
	 * Construct the macro to read the first 32 bit.
	 */
	pucData = aucMacro;

	/* Always start with 1bit. */
	*(pucData++) = SMC_MODE | ((unsigned char)SPI_BUS_WIDTH_1BIT);

	/* Send 16 bits of 1 to exit any continuous read commands. */
	*(pucData++) = SMC_SEND_SDD | 1;
	*(pucData++) = 0xffU;
	*(pucData++) = 0xffU;
	
	/* Switch the bus width for the command phase. */
	if( tBusWidthCommand!=SPI_BUS_WIDTH_1BIT )
	{
		*(pucData++) = SMC_MODE | ((unsigned char)tBusWidthCommand);
	}

	/* Chip select, send 1 byte. */
	*(pucData++) = SMC_SEND_SNN;

	/* Send the instruction. */
	*(pucData++) = ucOpcode;

	/* Switch the bus width for the address phase. */
	if( tBusWidthCommand!=tBusWidthAddress )
	{
		*(pucData++) = SMC_MODE | ((unsigned char)tBusWidthAddress);
	}

	/* Send the address bytes. */
	ulValue = ptFlashHandle->ulInitialOffset;
	if( uiAddressNibbles==8 )
	{
		/* Use 4 address bytes. */
		*(pucData++) = SMC_SEND_NNN | 3;
		*(pucData++) = (unsigned char)((ulValue >> 24U) & 0xffU);
	}
	else
	{
		/* Use 3 address bytes. */
		*(pucData++) = SMC_SEND_NNN | 2;
	}
	*(pucData++) = (unsigned char)((ulValue >> 16U) & 0xffU);
	*(pucData++) = (unsigned char)((ulValue >>  8U) & 0xffU);
	*(pucData++) = (unsigned char)( ulValue         & 0xffU);

	/* Send the mode bits. */
	if( ulModeBits!=0 )
	{
		*(pucData++) = (unsigned char)(SMC_SEND_NNN | ((ulModeBits/8)-1));
		ulValue = ulModeBits;
		do
		{
			*(pucData++) = ucModeData;
			ulValue -= 8;
		} while( ulValue!=0 );
	}

	/* Switch the bus width for the data phase. */
	if( tBusWidthAddress!=tBusWidthData )
	{
		*(pucData++) = SMC_MODE | ((unsigned char)tBusWidthData);
	}

	/* Send the dummy bits. */
	if( ucIdleCycles!=0 )
	{
		*(pucData++) = (unsigned char)(SMC_IDLE_NNN | (ucIdleCycles-1U));
	}

	/* Receive 4 bytes. */
	*(pucData++) = SMC_RECEIVE_SDD | 0x3U;

	/* Run the macro. */
	sizMacro = (unsigned int)(pucData - aucMacro);
	spi_macro_initialize(&tSpiMacro, &(ptFlashHandle->tCfg), aucMacro, sizMacro);
	tResult = spi_macro_player_run(&tSpiMacro);
	if( tResult==BOOTING_Ok )
	{
		/* The macro finished with success. Now compare the magic value. */
		ulValue  = tSpiMacro.uRxBuffer.aul[0];
		if( (ulValue&BOOTBLOCK_OLDSTYLE_MAGIC_MASK)!=g_t_romloader_options.t_system_config.ulBootBlockOldstyleMagic )
		{
			/* The read value is no magic.
			 * Either the operation did not succeed, or the device does not contain a boot-able image.
			 */
			trace_message_ul(TRACEMSG_BootBlock_NoMagicCookie, ulValue);
			tResult = BOOTING_Cookie_Invalid;
		}
		else
		{
			/* Does the unit support XIP and can these settings be used for XIP?
			 * Both the address and the data must be 4 bit, and there must be 8 mode bits.
			 */
			if( uiFlashSupportsXip!=0 )
			{
				trace_message(TRACEMSG_SpiFlash_UseXip);

				/*  Deselect the slave. */
				ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);

				/* Send 1 idle byte. */
				ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

				/* Activate the SQI ROM area. */
				ulDeviceSpecificValue = ptFlashHandle->tCfg.pfnGetDeviceSpecificSqiRomCfg(&(ptFlashHandle->tCfg), ucIdleCycles, uiAddressBits, uiAddressNibbles);
				if( ulDeviceSpecificValue==0U )
				{
					tResult = BOOTING_Setup_Error;
				}
				else
				{
					/* Store the configuration in the current settings block. */
					g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].ulSqiRomCfg = ulDeviceSpecificValue;

					/* Activate the SQI ROM area. */
					iResult = ptFlashHandle->tCfg.pfnActivateSqiRom(&(ptFlashHandle->tCfg), ulDeviceSpecificValue);
					if( iResult!=0 )
					{
						tResult = BOOTING_Setup_Error;
					}
					else
					{
						/* Use the ROM transport. */
						ptFlashHandle->uiMode_0Fifo_1Rom = 1U;
					}
				}
			}
			else
			{
				trace_message(TRACEMSG_SpiFlash_UseSqi);

				/* No XIP possible with this settings. Use the FIFO transport. */
				ptFlashHandle->uiMode_0Fifo_1Rom = 0U;

				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommand = ucOpcode;
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandBusWidths = ucBusWidths;
				/* Translate the mode bits to dummy bytes by dividing them by 8. */
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandDummyBytes = (unsigned char)(ulModeBits >> 3U);
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandIdleCycles = ucIdleCycles;
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucFlags = ucFlags;

				/* Send the read command. */
				iResult = send_read_command(ptFlashHandle, ptFlashHandle->ulInitialOffset);
				if( iResult!=0 )
				{
					tResult = BOOTING_Transfer_Error;
				}
			}

			/* Reset the offset to the initial value. */
			ptFlashHandle->ulOffset = ptFlashHandle->ulInitialOffset;

			/* Process the image. */
			tResult = process_boot_image(ptFlashHandle);

			if( ptFlashHandle->uiMode_0Fifo_1Rom==1U )
			{
				/* Disable the SQI ROM. */
				ptFlashHandle->tCfg.pfnDeactivateSqiRom(&(ptFlashHandle->tCfg));
			}
		}
	}

	if( tResult!=BOOTING_Ok )
	{
		/* Switch to 1 bit communication. */
		ptFlashHandle->tCfg.pfnSetBusWidth(&(ptFlashHandle->tCfg), SPI_BUS_WIDTH_1BIT);

		/*  Deselect the slave. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);

		/* Send 1 idle byte. */
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

		/* Send 0xff 0xff here to quit any continuous read. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 1);
		ptFlashHandle->tCfg.pfnSendData(&(ptFlashHandle->tCfg), aucFFFF, sizeof(aucFFFF));
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_444(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_444FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_444FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_444FastReadNumberOfModeClocks;
	ulModeBits  *= 4;

	ulValue   = pulParameters[OFF_SFDP_BFP_444FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_444FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_444FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_444FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_444FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_444FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_4BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary. */
	if( (ulModeBits&7)==4 && ucIdleCycles>=2 )
	{
		/* Add one cycle from the dummy cycles to the mode bits. */
		trace_message(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits);
		--ucIdleCycles;
		ulModeBits += 4;
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_144(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_144FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_144FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_144FastReadNumberOfModeClocks;
	ulModeBits  *= 4;

	ulValue   = pulParameters[OFF_SFDP_BFP_144FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_144FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_144FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_144FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_144FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_144FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_1BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary. */
	if( (ulModeBits&7)==4 && ucIdleCycles>=2 )
	{
		/* Add one cycle from the dummy cycles to the mode bits. */
		trace_message(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits);
		--ucIdleCycles;
		ulModeBits += 4;
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_114(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_114FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_114FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_114FastReadNumberOfModeClocks;

	ulValue   = pulParameters[OFF_SFDP_BFP_114FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_114FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_114FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_114FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_114FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_114FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_1BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_1BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_1BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_4BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary. */
	if( (ulModeBits&7)==4 && ucIdleCycles>=2 )
	{
		/* Add one cycle from the dummy cycles to the mode bits. */
		trace_message(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits);
		--ucIdleCycles;
		ulModeBits += 4;
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_222(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;
	unsigned char ucModeBitsFillUp;
	unsigned char ucIdleCyclesTaken;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_222FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_222FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_222FastReadNumberOfModeClocks;
	ulModeBits  *= 2;

	ulValue   = pulParameters[OFF_SFDP_BFP_222FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_222FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_222FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_222FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_222FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_222FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_2BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary.
	 * This is only possible for 2, 4 or 6 left over bits.
	 */
	if( (ulModeBits&1)==0 && (ulModeBits&6)!=0 )
	{
		/* Get the number of additional cycles needed form a complete byte. */
		ucModeBitsFillUp = (unsigned char)(8U - (ulModeBits & 7U));

		/* Convert this to dummy cycles. */
		ucIdleCyclesTaken = ucModeBitsFillUp >> 1U;

		/* Can this number of clocks be taken from the dummy cycles?
		 * There must be some dummy cycles left to allow I/O switching.
		 */
		if( ucIdleCycles>ucIdleCyclesTaken )
		{
			/* Add one cycle from the dummy cycles to the mode bits. */
			ulValue  =  (unsigned long)ucIdleCyclesTaken;
			ulValue |= ((unsigned long)ucModeBitsFillUp) << 8U;
			trace_message_ul(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits, ulValue);
			ucIdleCycles = (unsigned char)(ucIdleCycles - ucIdleCyclesTaken);
			ulModeBits += ucModeBitsFillUp;
		}
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_122(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;
	unsigned char ucModeBitsFillUp;
	unsigned char ucIdleCyclesTaken;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_122FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_122FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_122FastReadNumberOfModeClocks;
	ulModeBits  *= 2;

	ulValue   = pulParameters[OFF_SFDP_BFP_122FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_122FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_122FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_122FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_122FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_122FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_1BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary.
	 * This is only possible for 2, 4 or 6 left over bits.
	 */
	if( (ulModeBits&1)==0 && (ulModeBits&6)!=0 )
	{
		/* Get the number of additional cycles needed form a complete byte. */
		ucModeBitsFillUp = (unsigned char)(8U - (ulModeBits & 7U));

		/* Convert this to dummy cycles. */
		ucIdleCyclesTaken = ucModeBitsFillUp >> 1U;

		/* Can this number of clocks be taken from the dummy cycles?
		 * There must be some dummy cycles left to allow I/O switching.
		 */
		if( ucIdleCycles>ucIdleCyclesTaken )
		{
			/* Add one cycle from the dummy cycles to the mode bits. */
			ulValue  =  (unsigned long)ucIdleCyclesTaken;
			ulValue |= ((unsigned long)ucModeBitsFillUp) << 8U;
			trace_message_ul(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits, ulValue);
			ucIdleCycles = (unsigned char)(ucIdleCycles - ucIdleCyclesTaken);
			ulModeBits += ucModeBitsFillUp;
		}
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static BOOTING_T sfdp10_setup_112(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, const unsigned long *pulParameters)
{
	BOOTING_T tResult;
	unsigned long ulValue;
	unsigned long ulModeBits;
	unsigned char ucIdleCycles;
	unsigned char ucOpcode;
	unsigned char ucBusWidths;
	unsigned char ucModeBitsFillUp;
	unsigned char ucIdleCyclesTaken;


	/* This is a V1.0 SFDP table. There is no information how to activate
	 * 4 bit output. Just try if it works out of the box.
	 */

	ulModeBits   = pulParameters[OFF_SFDP_BFP_112FastReadNumberOfModeClocks];
	ulModeBits  &= MSK_SFDP_BFP_112FastReadNumberOfModeClocks;
	ulModeBits >>= SRT_SFDP_BFP_112FastReadNumberOfModeClocks;

	ulValue   = pulParameters[OFF_SFDP_BFP_112FastReadNumberOfWaitStates];
	ulValue  &= MSK_SFDP_BFP_112FastReadNumberOfWaitStates;
	ulValue >>= SRT_SFDP_BFP_112FastReadNumberOfWaitStates;
	ucIdleCycles = (unsigned char)ulValue;

	ulValue   = pulParameters[OFF_SFDP_BFP_112FastReadInstruction];
	ulValue  &= MSK_SFDP_BFP_112FastReadInstruction;
	ulValue >>= SRT_SFDP_BFP_112FastReadInstruction;
	ucOpcode = (unsigned char)ulValue;

	ulValue  =  (unsigned long)SPI_BUS_WIDTH_1BIT;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_1BIT) << 2U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_1BIT) << 4U;
	ulValue |= ((unsigned long)SPI_BUS_WIDTH_2BIT) << 6U;
	ucBusWidths = (unsigned char)ulValue;

	/* Try to round the mode bits up to a byte boundary.
	 * This is only possible for 2, 4 or 6 left over bits.
	 */
	if( (ulModeBits&1)==0 && (ulModeBits&6)!=0 )
	{
		/* Get the number of additional cycles needed form a complete byte. */
		ucModeBitsFillUp = (unsigned char)(8U - (ulModeBits & 7U));

		/* Convert this to dummy cycles. */
		ucIdleCyclesTaken = ucModeBitsFillUp >> 1U;

		/* Can this number of clocks be taken from the dummy cycles?
		 * There must be some dummy cycles left to allow I/O switching.
		 */
		if( ucIdleCycles>ucIdleCyclesTaken )
		{
			/* Add one cycle from the dummy cycles to the mode bits. */
			ulValue  =  (unsigned long)ucIdleCyclesTaken;
			ulValue |= ((unsigned long)ucModeBitsFillUp) << 8U;
			trace_message_ul(TRACEMSG_SpiFlash_ConvertDummyCycleToModeBits, ulValue);
			ucIdleCycles = (unsigned char)(ucIdleCycles - ucIdleCyclesTaken);
			ulModeBits += ucModeBitsFillUp;
		}
	}

	if( (ulModeBits&7)!=0 )
	{
		/* The number of mode clocks is no complete byte. */
		trace_message(TRACEMSG_SpiFlash_ModeBitsAreNoByte);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		tResult = sfdp10_generate_macro(ptFlashHandle, pulParameters, ulModeBits, ucIdleCycles, ucOpcode, ucBusWidths);
	}

	return tResult;
}



static const SFDP_MODE_ATTRIBUTES_T atSfdpModes[] =
{
	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports444FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports444FastRead,
		.pfnSfdpSetup = sfdp10_setup_444,
		.ulModeID = 0x0444
	},
	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports144FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports144FastRead,
		.pfnSfdpSetup = sfdp10_setup_144,
		.ulModeID = 0x0144
	},
	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports114FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports114FastRead,
		.pfnSfdpSetup = sfdp10_setup_114,
		.ulModeID = 0x0114
	},

	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports222FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports222FastRead,
		.pfnSfdpSetup = sfdp10_setup_222,
		.ulModeID = 0x0222
	},
	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports122FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports122FastRead,
		.pfnSfdpSetup = sfdp10_setup_122,
		.ulModeID = 0x0122
	},
	{
		.ulSupports_OFF = OFF_SFDP_BFP_Supports112FastRead,
		.ulSupports_MSK = MSK_SFDP_BFP_Supports112FastRead,
		.pfnSfdpSetup = sfdp10_setup_112,
		.ulModeID = 0x0112
	}
};




static int parse_jedec_flash_parameters_v1_0(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, unsigned long ulAddress, unsigned int sizDwords)
{
	union UNION_FLASH_PARAMETERS_U
	{
		unsigned long aul[9];
		unsigned char auc[9*sizeof(unsigned long)];
	} uFlashParameters;
	BOOTING_T tResult;
	int iResult;
	int iContinueTrying;
	const SFDP_MODE_ATTRIBUTES_T *ptModeCnt;
	const SFDP_MODE_ATTRIBUTES_T *ptModeEnd;
	unsigned long ulValue;


	/* Clear the table. */
	memset(&uFlashParameters, 0, sizeof(uFlashParameters));

	/* Read the parameter table. */
	iResult = read_sfdp_data(ptFlashHandle, uFlashParameters.auc, sizDwords*sizeof(unsigned long), ulAddress);
	terminate_sfdp_reads(ptFlashHandle);
	if( iResult==0 )
	{
		ptModeCnt = atSfdpModes;
		ptModeEnd = atSfdpModes + (sizeof(atSfdpModes)/sizeof(atSfdpModes[0]));
		while( ptModeCnt<ptModeEnd )
		{
			/* Check for the mode. */
			ulValue  = uFlashParameters.aul[ptModeCnt->ulSupports_OFF];
			ulValue &= ptModeCnt->ulSupports_MSK;
			if( ulValue!=0 )
			{
				trace_message_ul(TRACEMSG_SpiFlash_TryMode, ptModeCnt->ulModeID);
				
				/* Try to setup the communication for this mode and look for a boot image. */
				tResult = ptModeCnt->pfnSfdpSetup(ptFlashHandle, uFlashParameters.aul);
				trace_message_ul(TRACEMSG_SpiFlash_ProbeResult, tResult);

				/* Set the result of the complete operation. */
				iResult = tResult;

				/* Decide if other modes should be probed.
				 * In general the decision is based on how much of the image was processed yet.
				 * If the error occurred in the header, try another mode.
				 * If the error occurred in the chunks, do not try another mode.
				 *
				 * The default is to continue trying.
				 */
				iContinueTrying = 1;
				switch(tResult)
				{
				case BOOTING_Ok:
					/* All OK, the image could be booted.
					 * Do not try again. It will not get better.
					 */
					iContinueTrying = 0;
					break;

				case BOOTING_Not_Allowed:
					/* This mode seems to be not allowed, try another one. */
					break;

				case BOOTING_Setup_Error:
					/* Failed to set up the controller, try with different settings. */
					break;

				case BOOTING_Transfer_Error:
					/* This is a general error for SPI. Do not try again. */
					iContinueTrying = 0;
					break;

				case BOOTING_Cookie_Invalid:
					/* The image has an invalid cookie.
					 * This could be a transfer error due to wrong settings.
					 * Try again with different ones.
					 */
					break;

				case BOOTING_Signature_Invalid:
					/* The image has an invalid signature.
					 * This could be a transfer error due to wrong settings.
					 * Try again with different ones.
					 */
					break;

				case BOOTING_Header_Checksum_Invalid:
					/* The header checksum is invalid.
					 * This could be a transfer error due to wrong settings.
					 * Try again with different ones.
					 */
					break;

				case BOOTING_Image_Processing_Errors:
					/* The header was processed, and the image contains errors.
					 * This will most likely not change with different settings.
					 * Do not try again.
					 */
					iContinueTrying = 0;
					break;

				case BOOTING_Secure_Error:
					/* Do not try again after a security error. */
					iContinueTrying = 0;
					break;
				}
				if( iContinueTrying==0 )
				{
					break;
				}
			}
			++ptModeCnt;
		}
	}

	return iResult;
}



typedef union SFDP_ID_UNION
{
	unsigned long aul[2];
	unsigned char auc[2*sizeof(unsigned long)];
} SFDP_ID_T;



static int spi_flash_autodetect(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	SFDP_ID_T uSfdpId;
	int iResult;
	unsigned int uiHeaderMax;
	unsigned int uiHeaderCnt;
	SFDP_HEADER_T tHeader;
	unsigned int uiID;
	unsigned long ulAddress_HeaderV1_0_crippled;
	unsigned long ulAddress_HeaderV1_0;
	unsigned long ulAddress_HeaderV1_1;
	unsigned long ulAddress_HeaderV1_later;
	size_t sizHeaderV1_0_crippled;
	unsigned long ulDeviceSpecificSpeed;


	sizHeaderV1_0_crippled = 0;
	ulAddress_HeaderV1_0_crippled = 0;
	ulAddress_HeaderV1_0 = 0;
	ulAddress_HeaderV1_1 = 0;
	ulAddress_HeaderV1_later = 0;

	/* Set the maximum allowed speed for FIFO operations. */
	ulDeviceSpecificSpeed = ptFlashHandle->tCfg.pfnGetDeviceSpeedRepresentation(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].uiMaximumSpeedInFifoMode_kHz);
	iResult = ptFlashHandle->tCfg.pfnSetNewSpeed(&(ptFlashHandle->tCfg), ulDeviceSpecificSpeed);
	if( iResult==0 )
	{
		/* Read the header. */
		iResult = read_sfdp_data(ptFlashHandle, uSfdpId.auc, sizeof(uSfdpId), 0);
		if( iResult==0 )
		{
			if( uSfdpId.aul[0]!=SFDP_MAGIC )
			{
				terminate_sfdp_reads(ptFlashHandle);
				trace_message(TRACEMSG_SpiFlash_NoSfdp);
				iResult = -1;
			}
			else
			{
				/* This code knows only SFDP V1.x . Another major
				 * version than 1 indicates an incompatible structure.
				 */
				if( uSfdpId.auc[5]==0x01U )
				{
					/* Get the number of headers.
					 * This is 0 based.
					 */
					uiHeaderMax = ((unsigned int)(uSfdpId.auc[6])) + 1;
					uiHeaderCnt = 0;
					do
					{
						iResult = read_sfdp_header(ptFlashHandle, uiHeaderCnt, &tHeader);
						if( iResult==0 )
						{
							trace_message_data(TRACEMSG_SpiFlash_SfdpHeader, &tHeader, sizeof(SFDP_HEADER_T));

							/* The first entry is always a "Serial Flash Basics" table. */
							if( uiHeaderCnt==0 )
							{
								uiID = SFDP_BASIC_PARAMETER_ID;
							}
							else
							{
								/* The ID of a V1.0 SFDP table is 1 byte. */
								uiID = tHeader.uiJedecID & 0xffU;
							}

							if( uiID==SFDP_BASIC_PARAMETER_ID )
							{
								if( tHeader.uiParameterVersionMajor==1 &&
								    tHeader.uiParameterVersionMinor==1 &&
								    tHeader.uiParameterSizeDw>=16 )
								{
									ulAddress_HeaderV1_1 = tHeader.ulParameterTablePointer;
								}
								else if( tHeader.uiParameterVersionMajor==1 &&
									 tHeader.uiParameterVersionMinor>=1 &&
									 tHeader.uiParameterSizeDw>=16 )
								{
									ulAddress_HeaderV1_later = tHeader.ulParameterTablePointer;
								}
								/* The complete V1.0 header has 9 DWORDS. */
								else if( tHeader.uiParameterVersionMajor==1 &&
									 tHeader.uiParameterVersionMinor==0 )
								{
									if( tHeader.uiParameterSizeDw>=9 )
									{
										ulAddress_HeaderV1_0 = tHeader.ulParameterTablePointer;
									}
									/* Some older V1.0 headers are not complete. */
									else if( tHeader.uiParameterSizeDw>=4 )
									{
										sizHeaderV1_0_crippled = tHeader.uiParameterSizeDw;
										ulAddress_HeaderV1_0_crippled = tHeader.ulParameterTablePointer;
									}
								}
							}
						}
						else
						{
							/* Failed to read the header. */
							break;
						}

						/* Next header. */
						++uiHeaderCnt;
					} while( uiHeaderCnt<uiHeaderMax );

					if( iResult==0 )
					{
						/* Prefer the V1.0 header. */
						if( ulAddress_HeaderV1_0!=0 )
						{
							trace_message_ul(TRACEMSG_SpiFlash_UseSfdpV10, ulAddress_HeaderV1_0);
							iResult = parse_jedec_flash_parameters_v1_0(ptFlashHandle, ulAddress_HeaderV1_0, 9);
						}
						else if( ulAddress_HeaderV1_0_crippled!=0 )
						{
							trace_message_ul(TRACEMSG_SpiFlash_UseSfdpV10Crippled, ulAddress_HeaderV1_0_crippled);
							iResult = parse_jedec_flash_parameters_v1_0(ptFlashHandle, ulAddress_HeaderV1_0_crippled, sizHeaderV1_0_crippled);
						}
						else if( ulAddress_HeaderV1_1!=0 )
						{
							trace_message_ul(TRACEMSG_SpiFlash_UseSfdpV11, ulAddress_HeaderV1_1);
							iResult = parse_jedec_flash_parameters_v1_0(ptFlashHandle, ulAddress_HeaderV1_1, 9);
						}
						else if( ulAddress_HeaderV1_later!=0 )
						{
							trace_message_ul(TRACEMSG_SpiFlash_UseSfdpV1later, ulAddress_HeaderV1_later);
							iResult = parse_jedec_flash_parameters_v1_0(ptFlashHandle, ulAddress_HeaderV1_later, 9);
						}
						else
						{
							/* No matching header found. */
							trace_message(TRACEMSG_SpiFlash_NoUseableSfdpFound);
							iResult = -1;
						}
					}
				}
			}
		}
	}

	return iResult;
}


/*-------------------------------------------------------------------------*/


static BOOTING_T spi_flash_fallback_boot(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	int iResult;
	BOOTING_T tResult;
	unsigned long ulDeviceSpecificSpeed;


	trace_message_ul(TRACEMSG_SpiFlash_TryMode, 0x0111);

	/* Reset the offset to the initial value. */
	ptFlashHandle->ulOffset = ptFlashHandle->ulInitialOffset;

	/* Set the maximum allowed speed for FIFO operations. */
	ulDeviceSpecificSpeed = ptFlashHandle->tCfg.pfnGetDeviceSpeedRepresentation(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].uiMaximumSpeedInFifoMode_kHz);
	iResult = ptFlashHandle->tCfg.pfnSetNewSpeed(&(ptFlashHandle->tCfg), ulDeviceSpecificSpeed);
	if( iResult!=0 )
	{
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		/* Clear the SPI bus. */
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

		/* Send 0xff 0xff here to quit any continuous read. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 1);
		ptFlashHandle->tCfg.pfnSendData(&(ptFlashHandle->tCfg), aucFFFF, sizeof(aucFFFF));
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

		/* Send the read command. */
		iResult = send_read_command(ptFlashHandle, ptFlashHandle->ulOffset);
		if( iResult!=0 )
		{
			tResult = BOOTING_Transfer_Error;
		}
		else
		{
			tResult = process_boot_image(ptFlashHandle);
		}

		/* Deselect the slave. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		/* Chip select must be low for a device specific time. */
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);
	}

	return tResult;
}



static void update_speed_limits(unsigned long ulData)
{
	unsigned long ulValue;


	/* Set the speed limit fields from the first header DWORD.
	 * The first 16 bits are the FIFO access limit in MHz.
	 * The last 16 bits are the ROM access limit in MHz.
	 *
	 * The values are only valid if they are not 0.
	 */
	ulValue  = ulData;
	ulValue &= 0x0000ffffU;
	if( ulValue!=0U )
	{
		/* Convert the value from MHz to kHz. */
		ulValue *= 1000U;
		g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].uiMaximumSpeedInFifoMode_kHz = ulValue;
		trace_message_ul(TRACEMSG_SpiFlash_NewFifoSpeed, ulValue);
	}

	ulValue   = ulData;
	ulValue >>= 16U;
	ulValue  &= 0x0000ffffU;
	if( ulValue!=0U )
	{
		/* Convert the value from MHz to kHz. */
		ulValue *= 1000U;
		g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].uiMaximumSpeedInRomMode_kHz = ulValue;
		trace_message_ul(TRACEMSG_SpiFlash_NewRomSpeed, ulValue);
	}
}



static int spi_flash_search_magic(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, unsigned long *pulMagic)
{
	union
	{
		unsigned char auc[16];
		unsigned long aul[16/sizeof(unsigned long)];
	} uMagicSeek;
	unsigned char *pucCnt;
	unsigned char *pucEnd;
	unsigned char *pucHit;
	unsigned char *pucDst;
	unsigned long ulMagic;
	int iResult;
	unsigned char ucDataOffset;
	unsigned int sizBytesLeft;
	unsigned int uiAddressBytes;
	unsigned long ulOffset;


	/* Clear the SPI bus. */
	ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 10);

	/* Select the slave. */
	ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 1);

	/* Invalidate the magic. */
	ulMagic = 0U;

	/* Clear the complete send buffer. */
	memset(uMagicSeek.auc, 0, sizeof(uMagicSeek));
	pucCnt = uMagicSeek.auc;

	/* Write the read command to the start of the buffer. */
	*(pucCnt++) = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommand;

	/* Continue with the offset. */
	ulOffset = ptFlashHandle->ulInitialOffset;

	/* Get the number of address bytes. */
	uiAddressBytes = 3U;
	if( (g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucFlags&SPI_FLASH_FLAGS_4_bit_address)!=0 )
	{
		uiAddressBytes = 4U;
		*(pucCnt++) = (unsigned char)((ulOffset>>24U) & 0xffU);
	}
	*(pucCnt++) = (unsigned char)((ulOffset>>16U) & 0xffU);
	*(pucCnt++) = (unsigned char)((ulOffset>> 8U) & 0xffU);
	*(pucCnt++) = (unsigned char)( ulOffset       & 0xffU);

	/* Send the read command. */
	iResult = ptFlashHandle->tCfg.pfnExchangeData(&(ptFlashHandle->tCfg), uMagicSeek.auc, sizeof(uMagicSeek), 0);
	if( iResult==0 )
	{
		/* Search for the magic. */
		ulMagic = uMagicSeek.aul[0];
		pucCnt = uMagicSeek.auc;
		pucEnd = uMagicSeek.auc + sizeof(uMagicSeek);
		pucHit = NULL;
		do
		{
			if( (ulMagic&BOOTBLOCK_OLDSTYLE_MAGIC_MASK)==g_t_romloader_options.t_system_config.ulBootBlockOldstyleMagic )
			{
				pucHit = pucCnt;
				break;
			}
			else
			{
				++pucCnt;
				if( pucCnt<pucEnd )
				{
					ulMagic >>= 8U;
					ulMagic  |= ((unsigned long)(*pucCnt)) << 24U;
				}
			}
		} while( pucCnt<pucEnd );

		/* Found the magic? */
		if( pucHit!=NULL )
		{
			ucDataOffset = (unsigned char)(pucHit - uMagicSeek.auc);
			/* The difference of the pointers include...
			 *  1) the read command: 1 byte
			 *  2) address bytes (3 or 4)
			 *  3) the data itself. The end pointer is still
			 *     pointing to the last byte of the magic: 3 bytes
			 *
			 *  This makes a minimum of 4 bytes plus the address bytes. Smaller offsets
			 *  violate the 3 elements in the list above.
			 */
			if( ucDataOffset<(4U+uiAddressBytes) )
			{
				iResult = -1;
			}
			else
			{
				/* Get the number of additional bytes between
				 * the command with the address bytes and the
				 * data.
				 * Take all of these bytes as dummy bytes as the SPI interface does not support idle bytes.
				 */
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandDummyBytes = (unsigned char)(ucDataOffset - (4U + uiAddressBytes));
				g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommandIdleCycles = 0U;
				trace_message_uc(TRACEMSG_SpiFlash_FoundMagicAtOffset, ucDataOffset);

				/* Copy 4 bytes to the start of the buffer.
				 * If not enough bytes are left in the buffer, read more bytes from the flash.
				 */
				sizBytesLeft = 4U;
				pucDst = uMagicSeek.auc;
				/* pucCnt is still on the last byte of the magic. */
				++pucCnt;
				while( sizBytesLeft!=0U )
				{
					if( pucCnt<pucEnd )
					{
						*(pucDst++) = *(pucCnt++);
						--sizBytesLeft;
					}
					else
					{
						break;
					}
				}
				/* Need more data from the flash? */
				if( sizBytesLeft!=0 )
				{
					/* Read the the rest of the bytes. */
					iResult = ptFlashHandle->tCfg.pfnReceiveData(&(ptFlashHandle->tCfg), pucDst, sizBytesLeft, 0);
				}
				if( iResult==0 )
				{
					update_speed_limits(uMagicSeek.aul[0]);
				}
			}
		}
		else
		{
			/* No -> the detection failed. */
			trace_message(TRACEMSG_SpiFlash_NoMagicFound);
			iResult = -1;

		}
	}

	/* Deselect the slave. */
	ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
	/* Chip select must be low for a device specific time. */
	ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

	*pulMagic = ulMagic;
	return iResult;
}



static int spi_flash_read_magic(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle, unsigned long *pulMagic)
{
	union
	{
		unsigned char  auc[8];
		unsigned short aus[8/sizeof(unsigned short)];
		unsigned long  aul[8/sizeof(unsigned long)];
	} uDataBuffer;
	unsigned long ulMagic;
	int iResult;


	/* Invalidate the magic. */
	ulMagic = 0;

	/* Clear the SPI bus. */
	ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 10);

	iResult = send_read_command(ptFlashHandle, ptFlashHandle->ulInitialOffset);
	if( iResult==0 )
	{
		/* Read the the magic cookie and the speed limits. */
		iResult = ptFlashHandle->tCfg.pfnReceiveData(&(ptFlashHandle->tCfg), uDataBuffer.auc, sizeof(uDataBuffer), 0);
		if( iResult==0 )
		{
			ulMagic = uDataBuffer.aul[0];
			if( (ulMagic&BOOTBLOCK_OLDSTYLE_MAGIC_MASK)==g_t_romloader_options.t_system_config.ulBootBlockOldstyleMagic )
			{
				update_speed_limits(uDataBuffer.aul[1]);
			}
			else
			{
				trace_message(TRACEMSG_SpiFlash_NoMagicFound);
				iResult = -1;
			}
		}
	}

	/* Deselect the slave. */
	ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
	/* Chip select must be low for a device specific time. */
	ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

	*pulMagic = ulMagic;
	return iResult;
}



static int spi_flash_detect_magic(BOOT_SPI_FLASH_HANDLE_T *ptFlashHandle)
{
	int iResult;
	unsigned char ucFlags;
	unsigned long ulMagic;


	/* Should the data offset be auto-detected? */
	ucFlags  = g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucFlags;
	ucFlags &= SPI_FLASH_FLAGS_Forbid_Autodetection_of_Dummy_and_Idle;
	if( ucFlags==0U )
	{
		/* Yes, use auto-detection to find the magic. */

		/* Send 0xff 0xff here to quit any continuous read. */
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 1);
		ptFlashHandle->tCfg.pfnSendData(&(ptFlashHandle->tCfg), aucFFFF, sizeof(aucFFFF));
		ptFlashHandle->tCfg.pfnSelect(&(ptFlashHandle->tCfg), 0);
		ptFlashHandle->tCfg.pfnSendDummy(&(ptFlashHandle->tCfg), 1);

		trace_message_uc(TRACEMSG_SpiFlash_AutoDetectMagic, g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].ucReadCommand);
		iResult = spi_flash_search_magic(ptFlashHandle, &ulMagic);
	}
	else
	{
		/* No auto-detection, there is a valid command and offset already. */
		trace_message_data(TRACEMSG_SpiFlash_FixedMagicPosition, g_t_romloader_options.atSpiFlashCfg+SPI_UNIT_OFFSET_CURRENT, sizeof(BOOT_SPI_FLASH_CONFIGURATION_T));
		iResult = spi_flash_read_magic(ptFlashHandle, &ulMagic);
	}

	if( iResult==0 )
	{
		trace_message_ul(TRACEMSG_BootBlock_MagicCookieFound, ulMagic);

		ulMagic &= BOOTBLOCK_OLDSTYLE_MAGIC_PARAMETER_MASK;
		if( ulMagic==BOOTBLOCK_OLDSTYLE_MAGIC_PARAMETER_NOAUTO )
		{
			/* Forbid auto detection with SFDP. */
			ptFlashHandle->ulFlags |= SPI_FLASH_FLAGS_Forbid_Autodetect;
		}
	}

	return iResult;
}



static BOOTING_T boot_spi_flash(PFN_SPI_INIT pfnInit, unsigned int uiSpiUnit, unsigned int uiSpiDevice, unsigned int uiChipSelect)
{
	BOOTING_T tResult;
	BOOT_SPI_FLASH_HANDLE_T tFlashHandle;
	int iResult;
	unsigned long ulValue;
	BOOT_SQIROM_CONFIGURATION_T *ptSqiCfg;
	unsigned int sizMacro;
	int iUnitHasMultiIo;
	BOOT_SPI_FLASH_CONFIGURATION_T tSettingsBackupForFallback;


	/* Copy the options to the current configuration. */
	memcpy(g_t_romloader_options.atSpiFlashCfg + SPI_UNIT_OFFSET_CURRENT, g_t_romloader_options.atSpiFlashCfg + uiSpiDevice, sizeof(BOOT_SPI_FLASH_CONFIGURATION_T));

	/* Copy the SQIROM configurations for units supporting this transport. */
	ptSqiCfg = NULL;
	sizMacro = 0U;
	iUnitHasMultiIo = 0;
	switch( (SPI_UNIT_OFFSET_T)uiSpiDevice )
	{
	case SPI_UNIT_OFFSET_CURRENT:
		/* This should not happen. */
		break;

	case SPI_UNIT_OFFSET_SQI0_CS0:
		ptSqiCfg = g_t_romloader_options.atSqiRomOptions + SQIROM_UNIT_OFFSET_SQIROM0_CS0;
		iUnitHasMultiIo = 1;
		break;

	case SPI_UNIT_OFFSET_SQI1_CS0:
		ptSqiCfg = g_t_romloader_options.atSqiRomOptions + SQIROM_UNIT_OFFSET_SQIROM1_CS0;
		iUnitHasMultiIo = 1;
		break;

	case SPI_UNIT_OFFSET_SPI0_CS0:
	case SPI_UNIT_OFFSET_SPI0_CS1:
	case SPI_UNIT_OFFSET_SPI0_CS2:
	case SPI_UNIT_OFFSET_SPI1_CS0:
	case SPI_UNIT_OFFSET_SPI1_CS1:
	case SPI_UNIT_OFFSET_SPI1_CS2:
		/* The SPI units do not have a SQIROM part. */
		break;
	}

	if( ptSqiCfg!=NULL )
	{
		memcpy(g_t_romloader_options.atSqiRomOptions+SQIROM_UNIT_OFFSET_CURRENT, ptSqiCfg, sizeof(BOOT_SQIROM_CONFIGURATION_T));
		sizMacro = (unsigned int)(g_t_romloader_options.atSqiRomOptions[SQIROM_UNIT_OFFSET_CURRENT].aucSeqActivate[0]);
	}

	/* Initialize the configuration and the unit.*/
	ulValue = g_t_romloader_options.atSpiFlashCfg[SQIROM_UNIT_OFFSET_CURRENT].ulInitialOffset;
	trace_message_ul(TRACEMSG_SpiFlash_InitialOffset, ulValue);
	tFlashHandle.ulInitialOffset = ulValue;
	tFlashHandle.ulOffset = ulValue;
	tFlashHandle.ulFlags  = 0U;
	tFlashHandle.uiMode_0Fifo_1Rom = 0U;
	/* Initialize the last used address.
	 * This is for the function reading the SFDP options. */
	tFlashHandle.ulSfdpLastAddress = 0xffffffffU;
	iResult = pfnInit(&(tFlashHandle.tCfg), &(g_t_romloader_options.atSpiFlashCfg[SPI_UNIT_OFFSET_CURRENT].tSpiCfg), uiSpiUnit, uiChipSelect);
	if( iResult!=0 )
	{
		trace_message(TRACEMSG_SpiFlash_DriverInitFailed);
		tResult = BOOTING_Setup_Error;
	}
	else
	{
		update_page_address_shift(&tFlashHandle);

		if( sizMacro!=0U )
		{
			iResult = transport_spi_flash_restart(&tFlashHandle);
			if( iResult!=0 )
			{
				tResult = BOOTING_Setup_Error;
			}
			else
			{
				tResult = process_boot_image(&tFlashHandle);
			}
		}
		else
		{
			/* Detect the magic cookie and read the following 32 bits
			 * as speed limits for FIFO and ROM.
			 */
			iResult = spi_flash_detect_magic(&tFlashHandle);
			if( iResult==0 )
			{
				/* Is auto-detection allowed? */
				ulValue  = tFlashHandle.ulFlags;
				ulValue &= SPI_FLASH_FLAGS_Forbid_Autodetect;
				if( (ulValue!=0) || (iUnitHasMultiIo==0) )
				{
					/* The image requests that no auto-detection is used,
					 * or the unit does not support multi-IO (in this case auto detection makes no sense).
					 * All parameters must be provided in the chunks.
					 */
					trace_message(TRACEMSG_SpiFlash_ImageForbidsAutoDetection);
					tResult = spi_flash_fallback_boot(&tFlashHandle);
				}
				else
				{
					/* Save the current settings. They are basically working. */
					memcpy(&tSettingsBackupForFallback, g_t_romloader_options.atSpiFlashCfg + SPI_UNIT_OFFSET_CURRENT, sizeof(BOOT_SPI_FLASH_CONFIGURATION_T));

					tResult = spi_flash_autodetect(&tFlashHandle);
					/* Try the fall-back mode if the auto-detected values did not work.
					 * A security error is fatal.
					 */
					if( tResult!=BOOTING_Ok && tResult!=BOOTING_Secure_Error )
					{
						/* Restore the settings which allowed to read the magic cookie. */
						memcpy(g_t_romloader_options.atSpiFlashCfg + SPI_UNIT_OFFSET_CURRENT, &tSettingsBackupForFallback, sizeof(BOOT_SPI_FLASH_CONFIGURATION_T));

						tResult = spi_flash_fallback_boot(&tFlashHandle);
					}
				}
			}
			else
			{
				tResult = BOOTING_Cookie_Invalid;
			}
		}

		/* Deactivate the unit. */
		tFlashHandle.tCfg.pfnDeactivate(&(tFlashHandle.tCfg));
	}

	return tResult;
}



/* The shell controls the search for an alternative boot image.
 *
 * If alternative boot images are disabled, it acts as a transparent wrapper to boot_spi_flash .
 *
 * Otherwise boot_spi_flash is called with different initial offsets:
 *
 *  1)   0x0FF000
 *  2)   0x1FF000
 *  3)   0x3FF000
 *  4)   0x7FF000
 *  5)   0xFFF000
 *  6) 0x01FFF000
 *  7) 0x03FFF000
 *  8) 0x07FFF000
 *  9) 0x0FFFF000
 * 10) 0x1FFFF000
 * 11) 0x3FFFF000
 * 12) 0x7FFFF000
 * 13) 0xFFFFF000
 *
 * For a device with 3 bit addressing this results in read requests to the following offsets:
 *
 *  1) 0x0FF000
 *  2) 0x1FF000
 *  3) 0x3FF000
 *  4) 0x7FF000
 *  5) 0xFFF000
 *  6) 0x01FFF0
 *  7) 0x03FFF0
 *  8) 0x07FFF0
 *  9) 0x0FFFF0
 * 10) 0x1FFFF0
 * 11) 0x3FFFF0
 * 12) 0x7FFFF0
 * 13) 0xFFFFF0
 *
 * Offsets 6 - 13 are not nice, but can not be suppressed without dropping 4bit support or the hard requirement of SFDP.
 *
 * For a device with 4 bit addressing the following offsets are probed:
 *
 *  1) 0x0FF00000
 *  2) 0x1FF00000
 *  3) 0x3FF00000
 *  4) 0x7FF00000
 *  5) 0xFFF00000
 *  6) 0x01FFF000
 *  7) 0x03FFF000
 *  8) 0x07FFF000
 *  9) 0x0FFFF000
 * 10) 0x1FFFF000
 * 11) 0x3FFFF000
 * 12) 0x7FFFF000
 * 13) 0xFFFFF000
 *
 * The first 5 offsets are also not nice here.
 *
 * Both sequences should be described in the reference guides.
 */
BOOTING_T boot_spi_flash_shell(PFN_SPI_INIT pfnInit, unsigned int uiSpiUnit, unsigned int uiSpiDevice, unsigned int uiChipSelect)
{
	BOOTING_T tResult;
	unsigned long ulOffsetRs12;
	unsigned long ulOffset;
	BOOT_SPI_FLASH_CONFIGURATION_T *ptSpiFlash;


	/* Use standard or alternative boot image? */
	if( g_t_romloader_options.t_system_config.ucBootImage_Standard0_Alternative1==0 )
	{
		/* Use the standard image. */
		tResult = boot_spi_flash(pfnInit, uiSpiUnit, uiSpiDevice, uiChipSelect);
	}
	else
	{
		/* Search for an alternative image. */
		ptSpiFlash = g_t_romloader_options.atSpiFlashCfg + uiSpiDevice;

		/* Start searching at 1MB.
		 * To prevent an overflow, omit the lower 12 bits in the
		 * counter. Shift the result up later.
		 */
		ulOffsetRs12 = 0x00100U;
		do
		{
			ulOffset   = ulOffsetRs12 - 1U;
			ulOffset <<= 12U;
			ptSpiFlash->ulInitialOffset = ulOffset;

			/* Try 4 bit addressing if necessary. */
			if( ulOffset>=0x01000000 )
			{
				ptSpiFlash->ucFlags |= SPI_FLASH_FLAGS_4_bit_address;
			}

			/* Try to boot from the new offset. */
			tResult = boot_spi_flash(pfnInit, uiSpiUnit, uiSpiDevice, uiChipSelect);
			if( tResult==BOOTING_Secure_Error )
			{
				break;
			}

			/* Move to the next offset. */
			ulOffsetRs12 <<= 1U;
		} while( ulOffsetRs12<=0x100000 );
	}

	return tResult;
}
