#ifndef __MAIN_H__
#define __MAIN_H__


#include "netx_test.h"
#include "boot_spi.h"


#define SPI_MACRO_MAX_SIZE 0x80


typedef struct TEST_PARAMETER_STRUCT
{
	BOOT_SPI_CONFIGURATION_T tSpiConfiguration;
	unsigned int uiUnit;
	unsigned int uiChipSelect;
	unsigned int sizSpiMacro;
	unsigned char aucSpiMacro[SPI_MACRO_MAX_SIZE];
} TEST_PARAMETER_T;


TEST_RESULT_T test_main(const TEST_PARAMETER_T *ptParameter);

#endif  /* __MAIN_H__ */
