/***************************************************************************
 *   Copyright (C) 2019 by Hilscher GmbH                                   *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#ifndef __BOOT_DRV_SPI_V1_H__
#define __BOOT_DRV_SPI_V1_H__

#include "boot_spi.h"

/*-------------------------------------*/


int boot_drv_spi_init_v1(SPI_CFG_T *ptCfg, const BOOT_SPI_CONFIGURATION_T *ptSpiCfg, unsigned int uiSpiUnit, unsigned int uiChipSelect);


/*-------------------------------------*/


#endif  /* __BOOT_DRV_SPI_V1_H__ */

