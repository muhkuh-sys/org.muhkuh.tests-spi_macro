/***************************************************************************
 *   Copyright (C) 2005, 2006, 2007, 2008, 2009 by Hilscher GmbH           *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#include <string.h>

#include "spi_macro_player.h"
#include "uprintf.h"



/*-------------------------------------------------------------------------*/


static int get_cs_mode(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_MACRO_CHIP_SELECT_MODE_T *ptMode)
{
	int iResult;
	unsigned char ucMode;
	SPI_MACRO_CHIP_SELECT_MODE_T tMode;


	/* Expect an invalid CS mode. */
	iResult = -1;

	ucMode = *((ptSpiMacro->pucMacroCnt)++);
	tMode = (SPI_MACRO_CHIP_SELECT_MODE_T)ucMode;
	switch(tMode)
	{
	case SMCS_NNN:
	case SMCS_SNN:
	case SMCS_SDN:
	case SMCS_SDD:
		iResult = 0;
		break;
	}
	if( iResult!=0 )
	{
		uprintf("Invalid CS mode: %d\n", ucMode);
	}
	else
	{
		*ptMode = tMode;
	}

	return iResult;
}



static int get_condition(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_MACRO_CONDITION_T *ptCondition)
{
	int iResult;
	unsigned int uiCondition;
	SPI_MACRO_CONDITION_T tCondition;


	/* Be pessimistic. */
	iResult = -1;

	uiCondition = (unsigned int)(*((ptSpiMacro->pucMacroCnt)++));
	tCondition = (SPI_MACRO_CONDITION_T)uiCondition;
	switch(tCondition)
	{
	case SPI_MACRO_CONDITION_Always:
	case SPI_MACRO_CONDITION_Equal:
	case SPI_MACRO_CONDITION_NotEqual:
	case SPI_MACRO_CONDITION_Zero:
	case SPI_MACRO_CONDITION_NotZero:
		iResult = 0;
	}
	if( iResult!=0 )
	{
		uprintf("[SpiMacro] Invalid condition: %d\n", uiCondition);
	}
	else
	{
		*ptCondition = tCondition;
	}

	return iResult;
}



static int is_condition_true(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_MACRO_CONDITION_T tCondition)
{
	unsigned long ulFlags;
	int iConditionIsTrue;


	/* Expect the condition not to be true. */
	iConditionIsTrue = 0;

	ulFlags = ptSpiMacro->ulFlags;

	switch(tCondition)
	{
	case SPI_MACRO_CONDITION_Always:
		/* Jump always. */
		iConditionIsTrue = 1;
		break;

	case SPI_MACRO_CONDITION_Equal:
		/* Jump if equal. */
		ulFlags &= (unsigned long)SPI_MACRO_FLAGS_Equal;
		if( ulFlags!=0 )
		{
			/* Execute the jump. */
			iConditionIsTrue = 1;
		}
		break;

	case SPI_MACRO_CONDITION_NotEqual:
		/* Jump if not equal. */
		ulFlags &= (unsigned long)SPI_MACRO_FLAGS_Equal;
		if( ulFlags==0 )
		{
			/* Execute the jump. */
			iConditionIsTrue = 1;
		}
		break;

	case SPI_MACRO_CONDITION_Zero:
		/* Jump if the last result was zero. */
		ulFlags &= (unsigned long)SPI_MACRO_FLAGS_Zero;
		if( ulFlags!=0 )
		{
			/* The last result was zero. Execute the jump. */
			iConditionIsTrue = 1;
		}
		break;

	case SPI_MACRO_CONDITION_NotZero:
		/* Jump if the last result was not zero. */
		ulFlags &= (unsigned long)SPI_MACRO_FLAGS_Zero;
		if( ulFlags==0 )
		{
			/* The last result was not zero. Execute the jump. */
			iConditionIsTrue = 1;
		}
		break;
	}

	return iConditionIsTrue;
}


/*-------------------------------------------------------------------------*/


static int SMC_Handler_Send(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned int sizBytes;
	SPI_MACRO_CHIP_SELECT_MODE_T tCsMode;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] Send %d\n", sizBytes);

		if( (ptSpiMacro->pucMacroCnt + sizBytes)>(ptSpiMacro->pucMacroEnd) )
		{
			uprintf("[SpiMacro] InvalidSize\n");
			iResult = -1;
		}
		else
		{
			/* Write the data to the trace buffer. */
			uprintf("[SpiMacro] Data:\n");
			hexdump(ptSpiMacro->pucMacroCnt, sizBytes);

			if( tCsMode!=SMCS_NNN )
			{
				/* Select the slave. */
				ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
			}

			/* Send the command. */
			iResult = ptSpiMacro->ptCfg->pfnSendData(ptSpiMacro->ptCfg, ptSpiMacro->pucMacroCnt, sizBytes);
			if( iResult!=0 )
			{
				uprintf("[SpiMacro] Transfer error\n");
			}
			else
			{
				ptSpiMacro->pucMacroCnt += sizBytes;

				if( tCsMode==SMCS_SDN || tCsMode==SMCS_SDD )
				{
					/*  Deselect the slave. */
					ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 0);

					if( tCsMode==SMCS_SDD )
					{
						/* Send 1 dummy byte. */
						ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, 1);
					}
				}
			}
		}
	}

	return iResult;
}



static int SMC_Handler_Receive(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned int sizBytes;
	unsigned int sizBuffer;
	SPI_MACRO_CHIP_SELECT_MODE_T tCsMode;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);
		uprintf("[SpiMacro] Receive %d\n", sizBytes);

		sizBuffer = sizeof(ptSpiMacro->uRxBuffer);
		if( sizBytes>sizBuffer )
		{
			uprintf("[SpiMacro] The read request with %d bytes exceeds the buffer with %d bytes.\n", sizBytes, sizBuffer);
			iResult = -1;
		}
		else
		{
			if( tCsMode!=SMCS_NNN )
			{
				/* Select the slave. */
				ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
			}

			/* Receive the data. */
			iResult = ptSpiMacro->ptCfg->pfnReceiveData(ptSpiMacro->ptCfg, ptSpiMacro->uRxBuffer.auc, sizBytes);
			if( iResult!=0 )
			{
				uprintf("[SpiMacro] Transfer Error\n");
			}
			else
			{
				uprintf("[SpiMacro] Data\n");
				hexdump(ptSpiMacro->uRxBuffer.auc, sizBytes);

				if( tCsMode==SMCS_SDN || tCsMode==SMCS_SDD )
				{
					/*  De-select the slave. */
					ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 0);

					if( tCsMode==SMCS_SDD )
					{
						/* Send 1 dummy byte. */
						ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, 1);
					}
				}
			}
		}
	}

	return iResult;
}



static int SMC_Handler_Idle(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned int sizCycles;
	SPI_MACRO_CHIP_SELECT_MODE_T tCsMode;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizCycles = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] Idle %d\n", sizCycles);

		if( tCsMode!=SMCS_NNN )
		{
			/* Select the slave. */
			ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
		}

		/* Send the idle cycles. */
		iResult = ptSpiMacro->ptCfg->pfnSendIdleCycles(ptSpiMacro->ptCfg, sizCycles);
		if( iResult!=0 )
		{
			uprintf("[SpiMacro] Transfer Error\n");
		}
		else
		{
			if( tCsMode==SMCS_SDN || tCsMode==SMCS_SDD )
			{
				/*  Deselect the slave. */
				ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 0);

				if( tCsMode==SMCS_SDD )
				{
					/* Send 1 dummy byte. */
					ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, 1);
				}
			}
		}
	}

	return iResult;
}



static int SMC_Handler_Dummy(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned int sizBytes;
	SPI_MACRO_CHIP_SELECT_MODE_T tCsMode;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] Dummy %d\n", sizBytes);

		if( tCsMode!=SMCS_NNN )
		{
			/* Select the slave. */
			ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
		}

		/* Send the dummy bytes. */
		iResult = ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, sizBytes);
		if( iResult!=0 )
		{
			uprintf("[SpiMacro] Transfer Error\n");
		}
		else
		{
			if( tCsMode==SMCS_SDN || tCsMode==SMCS_SDD )
			{
				/*  Deselect the slave. */
				ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 0);

				if( tCsMode==SMCS_SDD )
				{
					/* Send 1 dummy byte. */
					ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, 1);
				}
			}
		}
	}

	return iResult;
}



static int SMC_Handler_Mode(SPI_MACRO_HANDLE_T *ptMacroCfg)
{
	int iResult;
	unsigned long ulValue;
	SPI_BUS_WIDTH_T tMode;


	/* Extract the mode from the command. */
	ulValue = (unsigned long)(*((ptMacroCfg->pucMacroCnt)++));
	tMode = (SPI_BUS_WIDTH_T)ulValue;

	uprintf("[SpiMacro] Mode %d\n", tMode);

	iResult = ptMacroCfg->ptCfg->pfnSetBusWidth(ptMacroCfg->ptCfg, tMode);
	if( iResult!=0 )
	{
		uprintf("[SpiMacro] Transfer Error\n");
	}

	return iResult;
}



static int SMC_Handler_Jump(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	SPI_MACRO_CONDITION_T tCondition;
	unsigned char ucAddress;
	const unsigned char *pucAddress;
	int iConditionIsTrue;


	/* Be pessimistic. */
	iResult = -1;

	/* Get the condition. */
	iResult = get_condition(ptSpiMacro, &tCondition);
	if( iResult==0 )
	{
		uprintf("[SpiMacro] Jump %d\n", tCondition);

		/* Get the address. */
		ucAddress = *((ptSpiMacro->pucMacroCnt)++);
		pucAddress = ptSpiMacro->pucMacroStart + ucAddress;
		if( pucAddress<=ptSpiMacro->pucMacroEnd )
		{
			iConditionIsTrue = is_condition_true(ptSpiMacro, tCondition);
			if( iConditionIsTrue!=0 )
			{
				ptSpiMacro->pucMacroCnt = pucAddress;
				uprintf("[SpiMacro] Jump 0x%08x\n", (unsigned long)pucAddress);
			}
		}
	}

	return iResult;
}



static int SMC_Handler_RequestChangeOfTransport(SPI_MACRO_HANDLE_T *ptMacroCfg)
{
	int iResult;
	unsigned long ulValue;
	SPI_MACRO_CHANGE_TRANSPORT_T tChangeTransport;
	SPI_CFG_T *ptSpiCfg;
	unsigned long ulDeviceSpecificSqiRomConfiguration;
	unsigned int uiDummyCycles;
	unsigned int uiAddrBits;
	unsigned int uiAddressNibbles;


	/* Extract the new transport. */
	ulValue = (unsigned long)(*((ptMacroCfg->pucMacroCnt)++));
	tChangeTransport = (SPI_MACRO_CHANGE_TRANSPORT_T)ulValue;

	uprintf("[SpiMacro] Request change of transport: %d\n", tChangeTransport);

	/* Check if the value is a valid transport. */
	iResult = -1;
	switch(tChangeTransport)
	{
	case SPI_MACRO_CHANGE_TRANSPORT_FIFO:
	case SPI_MACRO_CHANGE_TRANSPORT_ROM:
		iResult = 0;
		break;
	}

	if( iResult==0 )
	{
		/* Yes, this is a valid transport. */
		uprintf("[SpiMacro] Transport %d\n", tChangeTransport);

		ptSpiCfg = ptMacroCfg->ptCfg;
		switch(tChangeTransport)
		{
		case SPI_MACRO_CHANGE_TRANSPORT_FIFO:
			iResult = ptSpiCfg->pfnDeactivateSqiRom(ptSpiCfg);
			break;

		case SPI_MACRO_CHANGE_TRANSPORT_ROM:
			uiDummyCycles = (unsigned int)(*((ptMacroCfg->pucMacroCnt)++));
			uiAddrBits = (unsigned int)(*((ptMacroCfg->pucMacroCnt)++));
			uiAddressNibbles = (unsigned int)(*((ptMacroCfg->pucMacroCnt)++));

			ulDeviceSpecificSqiRomConfiguration = ptSpiCfg->pfnGetDeviceSpecificSqiRomCfg(ptSpiCfg, uiDummyCycles, uiAddrBits, uiAddressNibbles);
			if( ulDeviceSpecificSqiRomConfiguration==0 )
			{
				iResult = -1;
			}
			else
			{
				iResult = ptSpiCfg->pfnActivateSqiRom(ptSpiCfg, ulDeviceSpecificSqiRomConfiguration);
			}
			break;
		}
	}

	return iResult;
}



static int SMC_Handler_Cmp(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned long ulLength;
	unsigned int sizBuffer;
	unsigned long ulLengthCnt;
	unsigned char *pucBuffer;
	unsigned long ulEqualFlag;
	unsigned char ucData0;
	unsigned char ucData1;
	unsigned long ulFlags;


	/* Be pessimistic. */
	iResult = -1;

	/* Get the length. */
	ulLength = *((ptSpiMacro->pucMacroCnt)++);
	uprintf("[SpiMacro] CMP %d\n", ulLength);

	sizBuffer = sizeof(ptSpiMacro->uRxBuffer);
	if( ulLength>sizBuffer )
	{
		uprintf("[SpiMacro] The 'CMP' command with %d bytes exceeds the buffer with %d bytes.\n", ulLength, sizBuffer);
		iResult = -1;
	}
	else if( (ptSpiMacro->pucMacroCnt + ulLength)>ptSpiMacro->pucMacroEnd )
	{
		uprintf("[SpiMacro] Invalid size\n");
	}
	else
	{
		/* Write the value to the trace buffer. */
		uprintf("[SpiMacro] CMP data\n");
		hexdump(ptSpiMacro->pucMacroCnt, ulLength);

		pucBuffer = ptSpiMacro->uRxBuffer.auc;
		ulEqualFlag = (unsigned long)SPI_MACRO_FLAGS_Equal;
		ulLengthCnt = ulLength;
		while( ulLengthCnt!=0 )
		{
			ucData0 = *(pucBuffer++);
			ucData1 = *((ptSpiMacro->pucMacroCnt)++);

			if( ucData0!=ucData1 )
			{
				ulEqualFlag = 0;
			}
			--ulLengthCnt;
		}

		ulFlags  = ptSpiMacro->ulFlags;
		ulFlags &= ~((unsigned long)SPI_MACRO_FLAGS_Equal);
		ulFlags |= ulEqualFlag;
		ptSpiMacro->ulFlags = ulFlags;

		iResult = 0;
	}

	return iResult;
}



static int SMC_Handler_Mask(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned long ulLength;
	unsigned int sizBuffer;
	unsigned long ulLengthCnt;
	unsigned char *pucBuffer;
	unsigned long ulZeroFlag;
	unsigned char ucData;
	unsigned long ulFlags;


	/* Be pessimistic. */
	iResult = -1;

	/* Extract the length. */
	ulLength = *((ptSpiMacro->pucMacroCnt)++);
	uprintf("[SpiMacro] Mask %d\n", ulLength);

	sizBuffer = sizeof(ptSpiMacro->uRxBuffer);
	if( ulLength>sizBuffer )
	{
		uprintf("[SpiMacro] The 'MASK' command with %d bytes exceeds the buffer with %d bytes.\n", ulLength, sizBuffer);
		iResult = -1;
	}
	else if( (ptSpiMacro->pucMacroCnt + ulLength)>ptSpiMacro->pucMacroEnd )
	{
		uprintf("[SpiMacro] Invalid size\n");
	}
	else
	{
		/* Write the mask to the trace buffer. */
		uprintf("[SpiMacro] Mask:\n");
		hexdump(ptSpiMacro->pucMacroCnt, ulLength);

		pucBuffer = ptSpiMacro->uRxBuffer.auc;
		ulZeroFlag = (unsigned long)SPI_MACRO_FLAGS_Zero;
		ulLengthCnt = ulLength;
		while( ulLengthCnt!=0 )
		{
			ucData  = *pucBuffer;
			ucData &= *((ptSpiMacro->pucMacroCnt)++);
			*(pucBuffer++) = ucData;

			if( ucData!=0 )
			{
				ulZeroFlag = 0;
			}
			--ulLengthCnt;
		}

		ulFlags  = ptSpiMacro->ulFlags;
		ulFlags &= ~((unsigned long)SPI_MACRO_FLAGS_Zero);
		ulFlags |= ulZeroFlag;
		ptSpiMacro->ulFlags = ulFlags;

		iResult = 0;

		/* Write the result to the trace buffer. */
		uprintf("[SpiMacro] Data:\n");
		hexdump(ptSpiMacro->uRxBuffer.auc, ulLength);
	}

	return iResult;
}



static int SMC_Handler_Adr(SPI_MACRO_HANDLE_T *ptMacroCfg __attribute__((unused)))
{
	/* FIXME: Add the ADR handler. */
	return -1;
}



static int SMC_Handler_Fail(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	SPI_MACRO_CONDITION_T tCondition;
	int iConditionIsTrue;


	/* Get the condition. */
	iResult = get_condition(ptSpiMacro, &tCondition);
	if( iResult==0 )
	{
		uprintf("[SpiMacro] Fail %d\n", tCondition);

		iConditionIsTrue = is_condition_true(ptSpiMacro, tCondition);
		if( iConditionIsTrue!=0 )
		{
			iResult = -1;
			uprintf("[SpiMacro] Failed\n");
		}
	}

	return iResult;
}

/*-------------------------------------------------------------------------*/


void spi_macro_initialize(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_CFG_T *ptCfg, const unsigned char *pucMacro, unsigned int sizMacro)
{
	/* Initialize the SPI macro handle. */
	ptSpiMacro->ptCfg = ptCfg;
	ptSpiMacro->pucMacroStart = pucMacro;
	ptSpiMacro->pucMacroCnt = pucMacro;
	ptSpiMacro->pucMacroEnd = pucMacro + sizMacro;
	ptSpiMacro->ulFlags = 0;
}



int spi_macro_player_run(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned char ucCmd;
	SPI_MACRO_COMMAND_T tCmd;


	/* Be optimistic. */
	iResult = 0;

	/* Run over the complete macro sequence. */
	while( ptSpiMacro->pucMacroCnt<ptSpiMacro->pucMacroEnd )
	{
		/* Get the next command. */
		ucCmd = *((ptSpiMacro->pucMacroCnt)++);
		tCmd = (SPI_MACRO_COMMAND_T)ucCmd;

		/* Expect an unknown command. */
		iResult = -1;

		switch(tCmd)
		{
		case SMC_RECEIVE:
		case SMC_SEND:
		case SMC_IDLE:
		case SMC_DUMMY:
		case SMC_JUMP:
		case SMC_CHTR:
		case SMC_CMP:
		case SMC_MASK:
		case SMC_MODE:
		case SMC_ADR:
		case SMC_FAIL:
			iResult = 0;
			break;
		}

		if( iResult!=0 )
		{
			uprintf("[SpiMacro] Invalid command: 0x%02x\n", ucCmd);
			break;
		}
		else
		{
			switch(tCmd)
			{
			case SMC_RECEIVE:
				iResult = SMC_Handler_Receive(ptSpiMacro);
				break;

			case SMC_SEND:
				iResult = SMC_Handler_Send(ptSpiMacro);
				break;

			case SMC_IDLE:
				iResult = SMC_Handler_Idle(ptSpiMacro);
				break;

			case SMC_DUMMY:
				iResult = SMC_Handler_Dummy(ptSpiMacro);
				break;

			case SMC_JUMP:
				iResult = SMC_Handler_Jump(ptSpiMacro);
				break;

			case SMC_CHTR:
				iResult = SMC_Handler_RequestChangeOfTransport(ptSpiMacro);
				break;

			case SMC_CMP:
				iResult = SMC_Handler_Cmp(ptSpiMacro);
				break;

			case SMC_MASK:
				iResult = SMC_Handler_Mask(ptSpiMacro);
				break;

			case SMC_MODE:
				iResult = SMC_Handler_Mode(ptSpiMacro);
				break;

			case SMC_ADR:
				iResult = SMC_Handler_Adr(ptSpiMacro);
				break;

			case SMC_FAIL:
				iResult = SMC_Handler_Fail(ptSpiMacro);
				break;
			}

			if( iResult!=0 )
			{
				uprintf("Failed to execute the command.\n");
				break;
			}
		}
	}

	return iResult;
}


