
#include "portcontrol.h"

#include "netx_io_areas.h"


#if ASIC_TYP==ASIC_TYP_NETX4000_RELAXED
static const unsigned short ausPortControlDefault[] =
{
	/* P00 */
	PORTCONTROL_CONFIGURATION(0,0,REEMUX_DRV_04MA,REEMUX_UDC_PULLUP50K),  /* RDY */
	PORTCONTROL_CONFIGURATION(0,0,REEMUX_DRV_04MA,REEMUX_UDC_PULLUP50K),  /* RUN */
	HOSTDFLT(PORTCONTROL_P0_2),  /* RST_OUT */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P01 */
	HOSTDFLT(PORTCONTROL_P1_0),    /* P1_0 SQI_CLK */
	HOSTDFLT(PORTCONTROL_P1_1),    /* P1_1 SQI_MOSI */
	HOSTDFLT(PORTCONTROL_P1_2),    /* P1_2 SQI_MISO */
	HOSTDFLT(PORTCONTROL_P1_3),    /* P1_3 SQI_SIO2 */
	HOSTDFLT(PORTCONTROL_P1_4),    /* P1_4 SQI_SIO3 */
	HOSTDFLT(PORTCONTROL_P1_5),    /* P1_5 SQI_CS0N */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P02 */
	HOSTDFLT(PORTCONTROL_P2_0),    /* P2_0 I2C_SCL */
	HOSTDFLT(PORTCONTROL_P2_1),    /* P2_1 I2C_SDA */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P03 */
	0xFFFF,
	HOSTDFLT(PORTCONTROL_P3_1),    /* P3_1  MMIO1 */
	HOSTDFLT(PORTCONTROL_P3_2),    /* P3_2  MMIO2 */
	HOSTDFLT(PORTCONTROL_P3_3),    /* P3_3  MMIO3 */
	HOSTDFLT(PORTCONTROL_P3_4),    /* P3_4  MMIO4 */
	HOSTDFLT(PORTCONTROL_P3_5),    /* P3_5  MMIO5 */
	HOSTDFLT(PORTCONTROL_P3_6),    /* P3_6  MMIO6 */
	HOSTDFLT(PORTCONTROL_P3_7),    /* P3_7  MMIO7 */
	HOSTDFLT(PORTCONTROL_P3_8),    /* P3_8  MMIO8 */
	HOSTDFLT(PORTCONTROL_P3_9),    /* P3_9  MMIO9 */
	HOSTDFLT(PORTCONTROL_P3_10),   /* P3_10 MMIO10 */
	HOSTDFLT(PORTCONTROL_P3_11),   /* P3_11 MMIO11 */
	HOSTDFLT(PORTCONTROL_P3_12),   /* P3_12 MMIO12 */
	HOSTDFLT(PORTCONTROL_P3_13),   /* P3_13 MMIO13 */
	HOSTDFLT(PORTCONTROL_P3_14),   /* P3_14 MMIO14 */
	HOSTDFLT(PORTCONTROL_P3_15),   /* P3_15 MMIO15 */

	/* P04 */
	HOSTDFLT(PORTCONTROL_P4_0),    /* P4_0  MMIO16 */
	HOSTDFLT(PORTCONTROL_P4_1),    /* P4_1  MMIO17 */
	HOSTDFLT(PORTCONTROL_P4_2),    /* P4_2  MMIO18 */
	HOSTDFLT(PORTCONTROL_P4_3),    /* P4_3  MMIO19 */
	HOSTDFLT(PORTCONTROL_P4_4),    /* P4_4  MMIO20 */
	HOSTDFLT(PORTCONTROL_P4_5),    /* P4_5  MMIO21 */
	HOSTDFLT(PORTCONTROL_P4_6),    /* P4_6  MMIO22 */
	HOSTDFLT(PORTCONTROL_P4_7),    /* P4_7  MMIO23 */
	HOSTDFLT(PORTCONTROL_P4_8),    /* P4_8  MMIO24 */
	HOSTDFLT(PORTCONTROL_P4_9),    /* P4_9  MMIO25 */
	HOSTDFLT(PORTCONTROL_P4_10),   /* P4_10 MMIO26 */
	HOSTDFLT(PORTCONTROL_P4_11),   /* P4_11 MMIO27 */
	HOSTDFLT(PORTCONTROL_P4_12),   /* P4_12 MMIO28 */
	HOSTDFLT(PORTCONTROL_P4_13),   /* P4_13 MMIO29 */
	HOSTDFLT(PORTCONTROL_P4_14),   /* P4_14 MMIO30 */
	HOSTDFLT(PORTCONTROL_P4_15),   /* P4_15 MMIO31 */

	/* P05 */
	HOSTDFLT(PORTCONTROL_P5_0),    /* P5_0  MMIO32 */
	HOSTDFLT(PORTCONTROL_P5_1),    /* P5_1  MMIO33 */
	HOSTDFLT(PORTCONTROL_P5_2),    /* P5_2  MMIO34 */
	HOSTDFLT(PORTCONTROL_P5_3),    /* P5_3  MMIO35 */
	HOSTDFLT(PORTCONTROL_P5_4),    /* P5_4  MMIO36 */
	HOSTDFLT(PORTCONTROL_P5_5),    /* P5_5  MMIO37 */
	HOSTDFLT(PORTCONTROL_P5_6),    /* P5_6  MMIO38 */
	HOSTDFLT(PORTCONTROL_P5_7),    /* P5_7  MMIO39 */
	HOSTDFLT(PORTCONTROL_P5_8),    /* P5_8  MMIO40 */
	HOSTDFLT(PORTCONTROL_P5_9),    /* P5_9  MMIO41 */
	HOSTDFLT(PORTCONTROL_P5_10),   /* P5_10 MMIO42 */
	HOSTDFLT(PORTCONTROL_P5_11),   /* P5_11 MMIO43 */
	HOSTDFLT(PORTCONTROL_P5_12),   /* P5_12 MMIO44 */
	HOSTDFLT(PORTCONTROL_P5_13),   /* P5_13 MMIO45 */
	HOSTDFLT(PORTCONTROL_P5_14),   /* P5_14 MMIO46 */
	HOSTDFLT(PORTCONTROL_P5_15),   /* P5_15 MMIO47 */

	/* P06 */
	HOSTDFLT(PORTCONTROL_P6_0),    /* P6_0  MMIO48 */
	HOSTDFLT(PORTCONTROL_P6_1),    /* P6_1  MMIO49 */
	HOSTDFLT(PORTCONTROL_P6_2),    /* P6_2  MMIO50 */
	HOSTDFLT(PORTCONTROL_P6_3),    /* P6_3  MMIO51 */
	HOSTDFLT(PORTCONTROL_P6_4),    /* P6_4  MMIO52 */
	HOSTDFLT(PORTCONTROL_P6_5),    /* P6_5  MMIO53 */
	HOSTDFLT(PORTCONTROL_P6_6),    /* P6_6  MMIO54 */
	HOSTDFLT(PORTCONTROL_P6_7),    /* P6_7  MMIO55 */
	HOSTDFLT(PORTCONTROL_P6_8),    /* P6_8  MMIO56 */
	HOSTDFLT(PORTCONTROL_P6_9),    /* P6_9  MMIO57 */
	HOSTDFLT(PORTCONTROL_P6_10),   /* P6_10 MMIO58 */
	HOSTDFLT(PORTCONTROL_P6_11),   /* P6_11 MMIO59 */
	HOSTDFLT(PORTCONTROL_P6_12),   /* P6_12 MMIO60 */
	HOSTDFLT(PORTCONTROL_P6_13),   /* P6_13 MMIO61 */
	HOSTDFLT(PORTCONTROL_P6_14),   /* P6_14 MMIO62 */
	HOSTDFLT(PORTCONTROL_P6_15),   /* P6_15 MMIO63 */

	/* P07 */
	HOSTDFLT(PORTCONTROL_P7_0),    /* P7_0  HIF_A17 */
	HOSTDFLT(PORTCONTROL_P7_1),    /* P7_1  HIF_D0 */
	HOSTDFLT(PORTCONTROL_P7_2),    /* P7_2  HIF_D1 */
	HOSTDFLT(PORTCONTROL_P7_3),    /* P7_3  HIF_D2 */
	HOSTDFLT(PORTCONTROL_P7_4),    /* P7_4  HIF_D3 */
	HOSTDFLT(PORTCONTROL_P7_5),    /* P7_5  HIF_D4 */
	HOSTDFLT(PORTCONTROL_P7_6),    /* P7_6  HIF_D5 */
	HOSTDFLT(PORTCONTROL_P7_7),    /* P7_7  HIF_D6 */
	HOSTDFLT(PORTCONTROL_P7_8),    /* P7_8  HIF_D7 */
	HOSTDFLT(PORTCONTROL_P7_9),    /* P7_9  HIF_D8 */
	HOSTDFLT(PORTCONTROL_P7_10),   /* P7_10 HIF_D9 */
	HOSTDFLT(PORTCONTROL_P7_11),   /* P7_11 HIF_D10 */
	HOSTDFLT(PORTCONTROL_P7_12),   /* P7_12 HIF_D11 */
	HOSTDFLT(PORTCONTROL_P7_13),   /* P7_13 HIF_D12 */
	HOSTDFLT(PORTCONTROL_P7_14),   /* P7_14 HIF_D13 */
	HOSTDFLT(PORTCONTROL_P7_15),   /* P7_15 HIF_D14 */

	/* P08 */
	HOSTDFLT(PORTCONTROL_P8_0),    /* P8_0  HIF_D15 */
	HOSTDFLT(PORTCONTROL_P8_1),    /* P8_1  HIF_D16 */
	HOSTDFLT(PORTCONTROL_P8_2),    /* P8_2  HIF_D17 */
	HOSTDFLT(PORTCONTROL_P8_3),    /* P8_3  HIF_D18 */
	HOSTDFLT(PORTCONTROL_P8_4),    /* P8_4  HIF_D19 */
	HOSTDFLT(PORTCONTROL_P8_5),    /* P8_5  HIF_D20 */
	HOSTDFLT(PORTCONTROL_P8_6),    /* P8_6  HIF_D21 */
	HOSTDFLT(PORTCONTROL_P8_7),    /* P8_7  HIF_D22 */
	HOSTDFLT(PORTCONTROL_P8_8),    /* P8_8  HIF_D23 */
	HOSTDFLT(PORTCONTROL_P8_9),    /* P8_9  HIF_D24 */
	HOSTDFLT(PORTCONTROL_P8_10),   /* P8_10 HIF_D25 */
	HOSTDFLT(PORTCONTROL_P8_11),   /* P8_11 HIF_D26 */
	HOSTDFLT(PORTCONTROL_P8_12),   /* P8_12 HIF_D27 */
	HOSTDFLT(PORTCONTROL_P8_13),   /* P8_13 HIF_D28 */
	HOSTDFLT(PORTCONTROL_P8_14),   /* P8_14 HIF_D29 */
	HOSTDFLT(PORTCONTROL_P8_15),   /* P8_15 HIF_D30 */

	/* P09 */
	HOSTDFLT(PORTCONTROL_P9_0),    /* P9_0  MEM_SDCLK */
	HOSTDFLT(PORTCONTROL_P9_1),    /* P9_1  HIF_A0 */
	HOSTDFLT(PORTCONTROL_P9_2),    /* P9_2  HIF_A1 */
	HOSTDFLT(PORTCONTROL_P9_3),    /* P9_3  HIF_A2 */
	HOSTDFLT(PORTCONTROL_P9_4),    /* P9_4  HIF_A3 */
	HOSTDFLT(PORTCONTROL_P9_5),    /* P9_5  HIF_A4 */
	HOSTDFLT(PORTCONTROL_P9_6),    /* P9_6  HIF_A5 */
	HOSTDFLT(PORTCONTROL_P9_7),    /* P9_7  HIF_A6 */
	HOSTDFLT(PORTCONTROL_P9_8),    /* P9_8  HIF_A7 */
	HOSTDFLT(PORTCONTROL_P9_9),    /* P9_9  HIF_A8 */
	HOSTDFLT(PORTCONTROL_P9_10),   /* P9_10 HIF_A9 */
	HOSTDFLT(PORTCONTROL_P9_11),   /* P9_11 HIF_A10 */
	HOSTDFLT(PORTCONTROL_P9_12),   /* P9_12 HIF_A11 */
	HOSTDFLT(PORTCONTROL_P9_13),   /* P9_13 HIF_A12 */
	HOSTDFLT(PORTCONTROL_P9_14),   /* P9_14 HIF_A13 */
	HOSTDFLT(PORTCONTROL_P9_15),   /* P9_15 HIF_A14 */

	/* P10 */
	HOSTDFLT(PORTCONTROL_P10_0),   /*  */
	HOSTDFLT(PORTCONTROL_P10_1),   /*  */
	HOSTDFLT(PORTCONTROL_P10_2),   /*  */
	HOSTDFLT(PORTCONTROL_P10_3),   /*  */
	HOSTDFLT(PORTCONTROL_P10_4),   /*  */
	HOSTDFLT(PORTCONTROL_P10_5),   /*  */
	HOSTDFLT(PORTCONTROL_P10_6),   /*  */
	HOSTDFLT(PORTCONTROL_P10_7),   /*  */
	HOSTDFLT(PORTCONTROL_P10_8),   /*  */
	HOSTDFLT(PORTCONTROL_P10_9),   /*  */
	HOSTDFLT(PORTCONTROL_P10_10),  /*  */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P11 */
	HOSTDFLT(PORTCONTROL_P11_0),   /*  */
	HOSTDFLT(PORTCONTROL_P11_1),   /*  */
	HOSTDFLT(PORTCONTROL_P11_2),   /*  */
	HOSTDFLT(PORTCONTROL_P11_3),   /*  */
	HOSTDFLT(PORTCONTROL_P11_4),   /*  */
	HOSTDFLT(PORTCONTROL_P11_5),   /*  */
	HOSTDFLT(PORTCONTROL_P11_6),   /*  */
	HOSTDFLT(PORTCONTROL_P11_7),   /*  */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P12 */
	HOSTDFLT(PORTCONTROL_P12_0),   /*  */
	HOSTDFLT(PORTCONTROL_P12_1),   /*  */
	HOSTDFLT(PORTCONTROL_P12_2),   /*  */
	HOSTDFLT(PORTCONTROL_P12_3),   /*  */
	HOSTDFLT(PORTCONTROL_P12_4),   /*  */
	HOSTDFLT(PORTCONTROL_P12_5),   /*  */
	HOSTDFLT(PORTCONTROL_P12_6),   /*  */
	HOSTDFLT(PORTCONTROL_P12_7),   /*  */
	HOSTDFLT(PORTCONTROL_P12_8),   /*  */
	HOSTDFLT(PORTCONTROL_P12_9),   /*  */
	HOSTDFLT(PORTCONTROL_P12_10),   /*  */
	HOSTDFLT(PORTCONTROL_P12_11),   /*  */
	HOSTDFLT(PORTCONTROL_P12_12),   /*  */
	HOSTDFLT(PORTCONTROL_P12_13),   /*  */
	HOSTDFLT(PORTCONTROL_P12_14),   /*  */
	HOSTDFLT(PORTCONTROL_P12_15),   /*  */

	/* P13 */
	HOSTDFLT(PORTCONTROL_P13_0),   /*  */
	HOSTDFLT(PORTCONTROL_P13_1),   /*  */
	HOSTDFLT(PORTCONTROL_P13_2),   /*  */
	HOSTDFLT(PORTCONTROL_P13_3),   /*  */
	HOSTDFLT(PORTCONTROL_P13_4),   /*  */
	HOSTDFLT(PORTCONTROL_P13_5),   /*  */
	HOSTDFLT(PORTCONTROL_P13_6),   /*  */
	HOSTDFLT(PORTCONTROL_P13_7),   /*  */
	HOSTDFLT(PORTCONTROL_P13_8),   /*  */
	HOSTDFLT(PORTCONTROL_P13_9),   /*  */
	HOSTDFLT(PORTCONTROL_P13_10),  /*  */
	HOSTDFLT(PORTCONTROL_P13_11),  /*  */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P14 */
	HOSTDFLT(PORTCONTROL_P14_0),   /*  */
	HOSTDFLT(PORTCONTROL_P14_1),   /*  */
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P15 */
	HOSTDFLT(PORTCONTROL_P15_0),   /*  */
	HOSTDFLT(PORTCONTROL_P15_1),   /*  */
	HOSTDFLT(PORTCONTROL_P15_2),   /*  */
	HOSTDFLT(PORTCONTROL_P15_3),   /*  */
	HOSTDFLT(PORTCONTROL_P15_4),   /*  */
	HOSTDFLT(PORTCONTROL_P15_5),   /*  */
	HOSTDFLT(PORTCONTROL_P15_6),   /*  */
	HOSTDFLT(PORTCONTROL_P15_7),   /*  */
	HOSTDFLT(PORTCONTROL_P15_8),   /*  */
	HOSTDFLT(PORTCONTROL_P15_9),   /*  */
	HOSTDFLT(PORTCONTROL_P15_10),  /*  */
	HOSTDFLT(PORTCONTROL_P15_11),  /*  */
	HOSTDFLT(PORTCONTROL_P15_12),  /*  */
	HOSTDFLT(PORTCONTROL_P15_13),  /*  */
	0xFFFF,
	0xFFFF,

	/* P16 */
	HOSTDFLT(PORTCONTROL_P16_0),
	HOSTDFLT(PORTCONTROL_P16_1),
	HOSTDFLT(PORTCONTROL_P16_2),
	HOSTDFLT(PORTCONTROL_P16_3),
	HOSTDFLT(PORTCONTROL_P16_4),
	HOSTDFLT(PORTCONTROL_P16_5),
	HOSTDFLT(PORTCONTROL_P16_6),
	HOSTDFLT(PORTCONTROL_P16_7),
	HOSTDFLT(PORTCONTROL_P16_8),
	HOSTDFLT(PORTCONTROL_P16_9),
	HOSTDFLT(PORTCONTROL_P16_10),
	HOSTDFLT(PORTCONTROL_P16_11),
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P17 */
	HOSTDFLT(PORTCONTROL_P17_0),
	0xFFFF,
	HOSTDFLT(PORTCONTROL_P17_2),
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,

	/* P18 */
	HOSTDFLT(PORTCONTROL_P18_0),
	0xFFFF,
	HOSTDFLT(PORTCONTROL_P18_2)
};
#endif



void portcontrol_apply(const unsigned short *pusIndex, const unsigned short *pusConfiguration, unsigned int sizConfiguration)
{
	const unsigned short *pusIndexCnt;
	const unsigned short *pusIndexEnd;
	const unsigned short *pusConfigurationCnt;
	unsigned long ulConfiguration;
	unsigned long ulOffset;
	volatile unsigned long *pulPortControl;


	pulPortControl = (volatile unsigned long*)HOSTADDR(PORTCONTROL);

	pusIndexCnt = pusIndex;
	pusIndexEnd = pusIndex + sizConfiguration;
	pusConfigurationCnt = pusConfiguration;
	while( pusIndexCnt<pusIndexEnd )
	{
		/* Get the value. */
		ulOffset = (unsigned long)(*(pusIndexCnt++));
		ulConfiguration = (unsigned long)(*(pusConfigurationCnt++));

		if( ulConfiguration!=PORTCONTROL_SKIP && ulOffset!=PORTCONTROL_SKIP )
		{
			/* Write the configuration. */
			pulPortControl[ulOffset] = ulConfiguration;
		}
	}
}



void portcontrol_apply_mmio(const unsigned char *pucMmioIndex, const unsigned short *pusConfiguration, unsigned int sizConfiguration)
{
	const unsigned char *pucMmioIndexCnt;
	const unsigned char *pucMmioIndexEnd;
	const unsigned short *pusConfigurationCnt;
	unsigned long ulConfiguration;
	unsigned long ulOffset;
	volatile unsigned long *pulPortControl;


	pulPortControl = (volatile unsigned long*)HOSTADDR(PORTCONTROL);

	pucMmioIndexCnt = pucMmioIndex;
	pucMmioIndexEnd = pucMmioIndex + sizConfiguration;
	pusConfigurationCnt = pusConfiguration;
	while( pucMmioIndexCnt<pucMmioIndexEnd )
	{
		ulOffset = (unsigned long)(*(pucMmioIndexCnt++));
		ulConfiguration = (unsigned long)(*(pusConfigurationCnt++));

		if( ulOffset!=0xffU && ulConfiguration!=PORTCONTROL_SKIP )
		{
			/* MMIO0 is at 16,11.
			 * From MMIO1 on the pins start at 3,0 and continue sequentially. */
			if( ulOffset==0 )
			{
				ulOffset += PORTCONTROL_INDEX(16,11);
			}
			else
			{
				ulOffset += PORTCONTROL_INDEX( 3, 0);
			}

			/* Write the configuration. */
			pulPortControl[ulOffset] = ulConfiguration;
		}
	}
}



void portcontrol_restore(const unsigned short *pusIndex, unsigned int sizConfiguration)
{
	const unsigned short *pusIndexCnt;
	const unsigned short *pusIndexEnd;
	unsigned long ulConfiguration;
	unsigned long ulOffset;
	volatile unsigned long *pulPortControl;


	pulPortControl = (volatile unsigned long*)HOSTADDR(PORTCONTROL);

	pusIndexCnt = pusIndex;
	pusIndexEnd = pusIndex + sizConfiguration;
	while( pusIndexCnt<pusIndexEnd )
	{
		/* Get the value. */
		ulOffset = (unsigned long)(*(pusIndexCnt++));
		ulConfiguration = (unsigned long)(ausPortControlDefault[ulOffset]);

		if( ulConfiguration!=PORTCONTROL_SKIP && ulOffset!=PORTCONTROL_SKIP )
		{
			/* Write the configuration. */
			pulPortControl[ulOffset] = ulConfiguration;
		}
	}
}



void portcontrol_restore_mmio(const unsigned char *pucMmioIndex, unsigned int sizConfiguration)
{
	const unsigned char *pucMmioIndexCnt;
	const unsigned char *pucMmioIndexEnd;
	unsigned long ulConfiguration;
	unsigned long ulOffset;
	volatile unsigned long *pulPortControl;


	pulPortControl = (volatile unsigned long*)HOSTADDR(PORTCONTROL);

	pucMmioIndexCnt = pucMmioIndex;
	pucMmioIndexEnd = pucMmioIndex + sizConfiguration;
	while( pucMmioIndexCnt<pucMmioIndexEnd )
	{
		/* Get the value. */
		ulOffset = (unsigned long)(*(pucMmioIndexCnt++));
		if( ulOffset!=0xffU )
		{
			/* MMIO0 is at 16,11.
			 * From MMIO1 on the pins start at 3,0 and continue sequentially. */
			if( ulOffset==0 )
			{
				ulOffset += PORTCONTROL_INDEX(16,11);
			}
			else
			{
				ulOffset += PORTCONTROL_INDEX( 3, 0);
			}

			ulConfiguration = (unsigned long)(ausPortControlDefault[ulOffset]);
			if( ulConfiguration!=PORTCONTROL_SKIP )
			{
				/* Write the configuration. */
				pulPortControl[ulOffset] = ulConfiguration;
			}
		}
	}
}
