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
#include "systime.h"
#include "uprintf.h"



/*-------------------------------------------------------------------------*/


static int get_cs_mode(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_MACRO_CHIP_SELECT_MODE_T *ptMode, const char **ppcName)
{
	int iResult;
	unsigned char ucMode;
	SPI_MACRO_CHIP_SELECT_MODE_T tMode;
	const char *pcName;


	/* Expect an invalid CS mode. */
	iResult = -1;
	pcName = NULL;

	ucMode = *((ptSpiMacro->pucMacroCnt)++);
	tMode = (SPI_MACRO_CHIP_SELECT_MODE_T)ucMode;
	switch(tMode)
	{
	case SMCS_NNN:
		iResult = 0;
		pcName = "NNN";
		break;

	case SMCS_SNN:
		iResult = 0;
		pcName = "SNN";
		break;

	case SMCS_SDN:
		iResult = 0;
		pcName = "SDN";
		break;

	case SMCS_SDD:
		iResult = 0;
		pcName = "SDD";
		break;
	}
	if( iResult!=0 )
	{
		uprintf("[ERROR] Invalid CS mode: %d\n", ucMode);
	}
	else
	{
		*ptMode = tMode;
		*ppcName = pcName;
	}

	return iResult;
}



static int get_condition(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_MACRO_CONDITION_T *ptCondition, const char **ppcName)
{
	int iResult;
	unsigned int uiCondition;
	SPI_MACRO_CONDITION_T tCondition;
	const char *pcName;


	/* Be pessimistic. */
	iResult = -1;
	pcName = NULL;

	uiCondition = (unsigned int)(*((ptSpiMacro->pucMacroCnt)++));
	tCondition = (SPI_MACRO_CONDITION_T)uiCondition;
	switch(tCondition)
	{
	case SPI_MACRO_CONDITION_Always:
		iResult = 0;
		pcName = "always";
		break;

	case SPI_MACRO_CONDITION_Equal:
		iResult = 0;
		pcName = "equal";
		break;

	case SPI_MACRO_CONDITION_NotEqual:
		iResult = 0;
		pcName = "not equal";
		break;

	case SPI_MACRO_CONDITION_Zero:
		iResult = 0;
		pcName = "zero";
		break;

	case SPI_MACRO_CONDITION_NotZero:
		iResult = 0;
		pcName = "not zero";
		break;
	}
	if( iResult!=0 )
	{
		uprintf("[ERROR] Invalid condition: %d\n", uiCondition);
	}
	else
	{
		*ptCondition = tCondition;
		*ppcName = pcName;
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



static void show_flags(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	unsigned long ulFlags;
	char cFlagE;
	char cFlagZ;


	ulFlags = ptSpiMacro->ulFlags;
	cFlagE = ' ';
	cFlagZ = ' ';
	if( (ulFlags & ((unsigned long)SPI_MACRO_FLAGS_Equal))!=0 )
	{
		cFlagE = 'E';
	}
	if( (ulFlags & ((unsigned long)SPI_MACRO_FLAGS_Zero))!=0 )
	{
		cFlagZ = 'Z';
	}
	uprintf("[SpiMacro] Flags: %c%c\n", cFlagE, cFlagZ);
}


/*-------------------------------------------------------------------------*/


static int SMC_Handler_Send(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned int sizBytes;
	SPI_MACRO_CHIP_SELECT_MODE_T tCsMode;
	const char *pcCsModeName;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode, &pcCsModeName);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] CMD: Send, CS mode: %s, Length: %d bytes\n", pcCsModeName, sizBytes);

		if( (ptSpiMacro->pucMacroCnt + sizBytes)>(ptSpiMacro->pucMacroEnd) )
		{
			uprintf("[ERROR] SEND command: invalid length.\n");
			iResult = -1;
		}
		else
		{
			/* Write the data to the trace buffer. */
			uprintf("[SpiMacro] Send data:\n");
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
				uprintf("[ERROR] SEND command: transfer error\n");
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
	const char *pcCsModeName;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode, &pcCsModeName);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);
		uprintf("[SpiMacro] CMD: Receive, CS mode: %s, Length: %d bytes\n", pcCsModeName, sizBytes);

		sizBuffer = sizeof(ptSpiMacro->uRxBuffer);
		if( sizBytes>sizBuffer )
		{
			uprintf("[ERROR] The read request with %d bytes exceeds the buffer with %d bytes.\n", sizBytes, sizBuffer);
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
				uprintf("[ERROR] RECEIVE command: transfer error\n");
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
	const char *pcCsModeName;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode, &pcCsModeName);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizCycles = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] CMD: Idle, CS mode: %s, Length: %d bytes\n", pcCsModeName, sizCycles);

		if( tCsMode!=SMCS_NNN )
		{
			/* Select the slave. */
			ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
		}

		/* Send the idle cycles. */
		iResult = ptSpiMacro->ptCfg->pfnSendIdleCycles(ptSpiMacro->ptCfg, sizCycles);
		if( iResult!=0 )
		{
			uprintf("[ERROR] IDLE command: transfer error\n");
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
	const char *pcCsModeName;


	/* Get the chip select mode. */
	iResult = get_cs_mode(ptSpiMacro, &tCsMode, &pcCsModeName);
	if( iResult==0 )
	{
		/* Get the number of bytes to send. */
		sizBytes = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] CMD: Dummy, CS mode: %s, Length: %d bytes\n", pcCsModeName, sizBytes);

		if( tCsMode!=SMCS_NNN )
		{
			/* Select the slave. */
			ptSpiMacro->ptCfg->pfnSelect(ptSpiMacro->ptCfg, 1);
		}

		/* Send the dummy bytes. */
		iResult = ptSpiMacro->ptCfg->pfnSendDummy(ptSpiMacro->ptCfg, sizBytes);
		if( iResult!=0 )
		{
			uprintf("[ERROR] DUMMY command: transfer error\n");
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
	const char *pcModeName;


	/* Expect an invalid mode. */
	iResult = -1;
	pcModeName = NULL;

	/* Extract the mode from the command. */
	ulValue = (unsigned long)(*((ptMacroCfg->pucMacroCnt)++));
	tMode = (SPI_BUS_WIDTH_T)ulValue;
	switch( tMode )
	{
	case SPI_BUS_WIDTH_1BIT:
		iResult = 0;
		pcModeName = "1 bit";
		break;

	case SPI_BUS_WIDTH_2BIT:
		iResult = 0;
		pcModeName = "2 bit";
		break;

	case SPI_BUS_WIDTH_4BIT:
		iResult = 0;
		pcModeName = "4 bit";
		break;
	}

	if( iResult!=0 )
	{
		uprintf("[ERROR] MODE command: Invalid mode: %d\n", ulValue);
	}
	else
	{
		uprintf("[SpiMacro] CMD: Mode, Mode: %s\n", pcModeName);

		iResult = ptMacroCfg->ptCfg->pfnSetBusWidth(ptMacroCfg->ptCfg, tMode);
		if( iResult!=0 )
		{
			uprintf("[ERROR] Failed to set new mode.\n");
		}
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
	const char *pcName;


	/* Be pessimistic. */
	iResult = -1;

	/* Get the condition. */
	iResult = get_condition(ptSpiMacro, &tCondition, &pcName);
	if( iResult==0 )
	{
		/* Get the address. */
		ucAddress = *((ptSpiMacro->pucMacroCnt)++);

		uprintf("[SpiMacro] CMD: Jump, Condition: %s, Address: 0x%02x\n", pcName, ucAddress);

		pucAddress = ptSpiMacro->pucMacroStart + ucAddress;
		if( pucAddress>ptSpiMacro->pucMacroEnd )
		{
			uprintf("[ERROR] JUMP command: the address point exceeds the size of the macro!\n");
			iResult = -1;
		}
		else
		{
			show_flags(ptSpiMacro);

			iConditionIsTrue = is_condition_true(ptSpiMacro, tCondition);
			if( iConditionIsTrue==0 )
			{
				uprintf("[SpiMacro] The condition is not true.\n");
			}
			else
			{
				ptSpiMacro->pucMacroCnt = pucAddress;
				uprintf("[SpiMacro] The condition is true. Jump to 0x%02x\n", ucAddress);
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
		uprintf("[ERROR] The CMP command with %d bytes exceeds the buffer with %d bytes.\n", ulLength, sizBuffer);
		iResult = -1;
	}
	else if( (ptSpiMacro->pucMacroCnt + ulLength)>ptSpiMacro->pucMacroEnd )
	{
		uprintf("[ERROR] CMP command: invalid size\n");
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
	uprintf("[SpiMacro] CMD: Mask Length: %d bytes\n", ulLength);

	sizBuffer = sizeof(ptSpiMacro->uRxBuffer);
	if( ulLength>sizBuffer )
	{
		uprintf("[ERROR] The MASK command with %d bytes exceeds the buffer with %d bytes.\n", ulLength, sizBuffer);
		iResult = -1;
	}
	else if( (ptSpiMacro->pucMacroCnt + ulLength)>ptSpiMacro->pucMacroEnd )
	{
		uprintf("[ERROR] MASK command: invalid size\n");
	}
	else
	{
		/* Write the mask to the trace buffer. */
		uprintf("[SpiMacro] Mask data:\n");
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
		uprintf("[SpiMacro] Mask result:\n");
		hexdump(ptSpiMacro->uRxBuffer.auc, ulLength);
		show_flags(ptSpiMacro);
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
	const char *pcName;


	/* Get the condition. */
	iResult = get_condition(ptSpiMacro, &tCondition, &pcName);
	if( iResult==0 )
	{
		uprintf("[SpiMacro] CMD: Fail, Condition: %s\n", pcName);
		show_flags(ptSpiMacro);

		iConditionIsTrue = is_condition_true(ptSpiMacro, tCondition);
		if( iConditionIsTrue==0 )
		{
			uprintf("[SpiMacro] The condition is not true.\n");
		}
		else
		{
			iResult = -1;
			uprintf("[ERROR] The condition is true. Failed!\n");
		}
	}

	return iResult;
}

/*-------------------------------------------------------------------------*/


int spi_macro_initialize(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_CFG_T *ptCfg, const unsigned char *pucMacro, unsigned int sizMacro)
{
	int iResult;
	unsigned long ulTimeout;


	/* The macro must have at least 4 bytes for the timeout. */
	if( sizMacro<4 )
	{
		iResult = -1;
	}
	else
	{
		ulTimeout  =  (unsigned long)pucMacro[0];
		ulTimeout |= ((unsigned long)pucMacro[1]) <<  8U;
		ulTimeout |= ((unsigned long)pucMacro[2]) << 16U;
		ulTimeout |= ((unsigned long)pucMacro[3]) << 24U;

		pucMacro += 4U;
		sizMacro -= 4U;

		/* Initialize the SPI macro handle. */
		ptSpiMacro->ptCfg = ptCfg;
		ptSpiMacro->pucMacroStart = pucMacro;
		ptSpiMacro->pucMacroCnt = pucMacro;
		ptSpiMacro->pucMacroEnd = pucMacro + sizMacro;
		ptSpiMacro->ulFlags = 0;
		ptSpiMacro->ulTotalTimeoutMs = ulTimeout;

		iResult = 0;
	}

	return iResult;
}



int spi_macro_player_run(SPI_MACRO_HANDLE_T *ptSpiMacro)
{
	int iResult;
	unsigned char ucCmd;
	SPI_MACRO_COMMAND_T tCmd;
	unsigned long ulTotalTimeoutMs;
	unsigned long ulTimerStart;
	int iIsElapsed;
	unsigned int uiCntCmd;
	const char *pcCmdNames[11] = {
	"RECEIVE",
	"SEND",
	"IDLE",
	"DUMMY",
	"JUMP",
	"CHTR",
	"CMP",
	"MASK",
	"MODE",
	"ADR",
	"FAIL"  
	};

	/* Be optimistic. */
	iResult = 0;

	/* Starting value of command count. */
	uiCntCmd = 1;

	/* Get the timeout for the macro. */
	ulTotalTimeoutMs = ptSpiMacro->ulTotalTimeoutMs;
	/* Get the timer value at the start of the macro. */
	ulTimerStart = systime_get_ms();

	/* Run over the complete macro sequence. */
	while( ptSpiMacro->pucMacroCnt<ptSpiMacro->pucMacroEnd )
	{
		/* Get the next command. */
		ucCmd = *((ptSpiMacro->pucMacroCnt)++);
		tCmd = (SPI_MACRO_COMMAND_T)ucCmd;


		uprintf("[DEBUG] command: %s (macro id: 0x%02x) - (Command number: %d)\n",pcCmdNames[ucCmd], ucCmd,uiCntCmd);

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
			uprintf("[ERROR] Unknown command with the id: 0x%02x\n", ucCmd);
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
				uprintf("[ERROR] Failed to execute the command: %s - (command number: %d).\n", pcCmdNames[ucCmd],uiCntCmd);
				break;
			}

			/* Is the timeout enabled? (!=0) */
			if( ulTotalTimeoutMs!=0 )
			{
				/* Did the timer run out? */
				iIsElapsed = systime_elapsed(ulTimerStart, ulTotalTimeoutMs);
				if( iIsElapsed!=0 )
				{
					uprintf("[ERROR] The timeout of %dms for the complete macro ran out. Stopping the macro.\n", ulTotalTimeoutMs);
					iResult = -1;
					break;
				}
			}
		}

		uiCntCmd++;
	}

	return iResult;
}