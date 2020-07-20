
#include "main.h"

#include "buffer.h"
#include "netx_io_areas.h"
#include "rdy_run.h"
#include "spi_macro_player.h"
#include <string.h>
#include "systime.h"
#include "uprintf.h"
#include "version.h"

#include "boot_drv_spi_v1.h"
#include "boot_drv_spi_v2.h"
#include "boot_drv_sqi.h"


/*-------------------------------------------------------------------------*/


static int open_driver(unsigned int uiUnit, unsigned int uiChipSelect, const BOOT_SPI_CONFIGURATION_T *ptSpiConfiguration, SPI_CFG_T *ptSpiCfg)
{
	int iResult;


	/* Be pessimistic. */
	iResult = -1;

#if ASIC_TYP==ASIC_TYP_NETX500
	/* netX500 has 1 hardware unit.
	 */
	if( uiUnit==0 )
	{
		iResult = boot_drv_spi_init_v1(ptSpiCfg, ptSpiConfiguration, uiUnit, uiChipSelect);
	}

#elif ASIC_TYP==ASIC_TYP_NETX50
	/* netX50 has 2 hardware units, which can do 1 bit SPI. This is mapped
	 * to units 0 and 1.
	 */
	if( (uiUnit==0) || (uiUnit==1) )
	{
		iResult = boot_drv_spi_init_v2(ptSpiCfg, ptSpiConfiguration, uiUnit, uiChipSelect);
	}

#elif ASIC_TYP==ASIC_TYP_NETX10
#       error "netX10 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX56
	/* netX56 has 2 hardware units. The first one can do 1, 2 and 4 bit
	 * SPI. It is mapped to unit 0. The second one can do 1 bit SPI. It
	 * is mapped to unit 1.
	 */
	if( uiUnit==0 )
	{
		iResult = boot_drv_sqi_init(ptSpiCfg, ptSpiConfiguration, 0, uiChipSelect);
	}
	else if( uiUnit==1 )
	{
		iResult = boot_drv_spi_init_v2(ptSpiCfg, ptSpiConfiguration, 0, uiChipSelect);
	}

#elif ASIC_TYP==ASIC_TYP_NETX6
#       error "netX6 is not yet supported"

#elif ASIC_TYP==ASIC_TYP_NETX4000_RELAXED || ASIC_TYP==ASIC_TYP_NETX4000
	/* netX4000 has 2 hardware units. The first one can do 1, 2 and 4 bit
	 * SPI. It is mapped to unit 0 and 1. The second one can do 1 bit SPI. It
	 * is mapped to unit 2.
	 */
	if( (uiUnit==0) || (uiUnit==1) )
	{
		iResult = boot_drv_sqi_init(ptSpiCfg, ptSpiConfiguration, 0, uiChipSelect);
	}
	else if( uiUnit==2 )
	{
		iResult = boot_drv_spi_init_v2(ptSpiCfg, ptSpiConfiguration, 0, uiChipSelect);
	}

#elif ASIC_TYP==ASIC_TYP_NETX90_MPW || ASIC_TYP==ASIC_TYP_NETX90
	/* netX90 has 3 hardware units. The first one can do 1, 2 and 4 bit
	 * SPI. It is mapped to unit 0. The second one can do 1 bit SPI. It
	 * is mapped to unit 1 and unit 2.
	 */
	if( uiUnit==0 )
	{
		iResult = boot_drv_sqi_init(ptSpiCfg, ptSpiConfiguration, 0, uiChipSelect);
	}
#if 0
	/* Not yet... */
	else if( (uiUnit==1) || (uiUnit==2) )
	{
		iResult = boot_drv_spi_init_v2(ptSpiCfg, ptSpiConfiguration, uiUnit-1, uiChipSelect);
	}
#endif

#else
#       error "Invalid ASIC_TYP!"
#endif

	return iResult;
}



TEST_RESULT_T test_main(const TEST_PARAMETER_T *ptParameter)
{
	TEST_RESULT_T tResult;
	int iResult;
	SPI_CFG_T tSpiCfg;
	SPI_MACRO_HANDLE_T tSpiMacro;


	/* Be pessimistic. */
	tResult = TEST_RESULT_ERROR;

	systime_init();

	uprintf("\f. *** SPI flash macro test by cthelen@hilscher.com ***\n");
	uprintf("V" VERSION_ALL "\n\n");

	/* Switch all LEDs off. */
	rdy_run_setLEDs(RDYRUN_OFF);

	if( ptParameter==NULL )
	{
		uprintf("No parameters provided!\n");
	}
	else
	{
		iResult = open_driver(ptParameter->uiUnit, ptParameter->uiChipSelect, &(ptParameter->tSpiConfiguration), &tSpiCfg);
		if( iResult!=0 )
		{
			uprintf("Failed to initialize the SPI driver.\n");
		}
		else
		{
			uprintf("Driver initialized.\n");

			iResult = spi_macro_initialize(&tSpiMacro, &tSpiCfg, ptParameter->aucSpiMacro, ptParameter->sizSpiMacro);
			if( iResult!=0 )
			{
				uprintf("Failed to initialize the SPI macro.\n");
			}
			else
			{
				iResult = spi_macro_player_run(&tSpiMacro);
				if( iResult!=0 )
				{
					uprintf("Failed to run the SPI macro.\n");
				}
				else
				{
					uprintf("SPI macro OK!\n");
					tResult = TEST_RESULT_OK;
				}
			}
		}
	}


	/* Switch the SYS LED to green if the test was successful. Switch it
	 * to red if an error occurred.
	 */
	if( tResult==TEST_RESULT_OK )
	{
		rdy_run_setLEDs(RDYRUN_GREEN);
	}
	else
	{
		rdy_run_setLEDs(RDYRUN_YELLOW);
	}

	/* Return the result. */
	return tResult;
}
