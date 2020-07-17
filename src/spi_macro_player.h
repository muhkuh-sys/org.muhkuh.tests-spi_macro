/***************************************************************************
 *   Copyright (C) 2014 by Hilscher GmbH                                   *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#include "boot_spi.h"


#ifndef __SPI_MACRO_PLAYER_H__
#define __SPI_MACRO_PLAYER_H__


/*-------------------------------------------------------------------------*/

typedef enum SPI_MACRO_COMMAND_ENUM
{
	SMC_RECEIVE  = 0,
	SMC_SEND     = 1,
	SMC_IDLE     = 2,
	SMC_DUMMY    = 3,
	SMC_JUMP     = 4,
	SMC_CHTR     = 5,
	SMC_CMP      = 6,
	SMC_MASK     = 7,
	SMC_MODE     = 8,
	SMC_ADR      = 9,
	SMC_FAIL     = 10
} SPI_MACRO_COMMAND_T;

/*-------------------------------------------------------------------------*/


typedef enum SPI_MACRO_CHIP_SELECT_MODE_ENUM
{
	SMCS_NNN     = 0,
	SMCS_SNN     = 1,
	SMCS_SDN     = 2,
	SMCS_SDD     = 3
} SPI_MACRO_CHIP_SELECT_MODE_T;


typedef enum SPI_MACRO_FLAGS_ENUM
{
	SPI_MACRO_FLAGS_Equal = 1,
	SPI_MACRO_FLAGS_Zero  = 2
} SPI_MACRO_FLAGS_T;


typedef enum SPI_MACRO_CHANGE_TRANSPORT_ENUM
{
	SPI_MACRO_CHANGE_TRANSPORT_FIFO       = 0,
	SPI_MACRO_CHANGE_TRANSPORT_ROM        = 1
} SPI_MACRO_CHANGE_TRANSPORT_T;


typedef enum SPI_MACRO_CONDITION_ENUM
{
	SPI_MACRO_CONDITION_Always    = 0,
	SPI_MACRO_CONDITION_Equal     = 1,
	SPI_MACRO_CONDITION_NotEqual  = 2,
	SPI_MACRO_CONDITION_Zero      = 3,
	SPI_MACRO_CONDITION_NotZero   = 4
} SPI_MACRO_CONDITION_T;


typedef struct SPI_MACRO_HANDLE_STRUCT
{
	SPI_CFG_T *ptCfg;
	const unsigned char *pucMacroStart;
	const unsigned char *pucMacroCnt;
	const unsigned char *pucMacroEnd;

	unsigned long ulTotalTimeoutMs;

	unsigned long ulFlags;

	union
	{
		unsigned long aul[64];
		unsigned char auc[64*sizeof(unsigned long)];
	} uRxBuffer;
} SPI_MACRO_HANDLE_T;



int spi_macro_initialize(SPI_MACRO_HANDLE_T *ptSpiMacro, SPI_CFG_T *ptCfg, const unsigned char *pucMacro, unsigned int sizMacro);
int spi_macro_player_run(SPI_MACRO_HANDLE_T *ptSpiMacro);


#endif  /* __SPI_MACRO_PLAYER_H__ */

