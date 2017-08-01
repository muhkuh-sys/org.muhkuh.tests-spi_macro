/***************************************************************************
 *   Copyright (C) 2005, 2006, 2007, 2008, 2009 by Hilscher GmbH           *
 *                                                                         *
 *   Author: Christoph Thelen (cthelen@hilscher.com)                       *
 *                                                                         *
 *   Redistribution or unauthorized use without expressed written          *
 *   agreement from the Hilscher GmbH is forbidden.                        *
 ***************************************************************************/


#include <stddef.h>


#ifndef __TOOLS_H__
#define __TOOLS_H__


#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))

#define QUAD_ID(a,b,c,d) ((unsigned long)(((unsigned char)(a))|(((unsigned char)(b))<<8U)|(((unsigned char)(c))<<16U)|(((unsigned char)(d))<<24U)))

typedef union ADR_UNION
{
	unsigned long ul;
	unsigned char *puc;
	unsigned short *pus;
	unsigned long *pul;
} ADR_T;

typedef union CADR_UNION
{
	unsigned long ul;
	const unsigned char *puc;
	const unsigned short *pus;
	const unsigned long *pul;
} CADR_T;

typedef union VADR_UNION
{
	unsigned long ul;
	volatile unsigned char *puc;
	volatile unsigned short *pus;
	volatile unsigned long *pul;
} VADR_T;


#define __IRQ_LOCK__ {__asm__ volatile ("cpsid   i, #0x13");}
#define __IRQ_UNLOCK__ {__asm__ volatile ("cpsie   i, #0x13");}

#endif  /* __TOOLS_H__ */
