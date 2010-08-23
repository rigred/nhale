/*
 * Copyright(C) 2010 Andrew Powell
 *
 * Copyright(C) 2001-2007 Roderick Colenbrander
 * The original author
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

// TODO: restructure NVXY defines using top 22 bits as select lines for NVX and bottom 10 bits as NVX1,NVX2, etc

#include <stdint.h>

/* PCI stuff */
#if 0
#define PCI_DEVICE_ID 0x2 /* 16-bit */
#define PCI_SUBSYSTEM_VENDOR_ID 0x2c /* 16-bit */
#define PCI_SUBSYSTEM_ID 0x2e /* 16-bit */
#endif

enum
{
  NV_PMC_BOOT_0 = 0x0,
  NV_PMC_BOOT_0_REVISION_MINOR = 0xf,
  NV_PMC_BOOT_0_REVISION_MAJOR =  0xf0, /* in general A or B, on pre-NV10 it was different */
  NV_PMC_BOOT_0_REVISION_MASK = 0xff,
  NV_PDISPLAY_OFFSET = 0x610000,
  NV_PDISPLAY_SIZE = 0x10000,
  NV_PDISPLAY_SOR0_REGS_BRIGHTNESS = 0xc084,
  NV_PDIPSLAY_SOR0_REGS_BRIGHTNESS_CONTROL_ENABLED =  0x80000000,
  NV_PRAMIN_OFFSET = 0x00700000,
  NV_PRAMIN_SIZE = 0x00100000,
  NV_PROM_OFFSET = 0x300000,
  NV_PROM_SIZE = 0x10000
};

enum
{
  NV5 = (1<<0),
  NV10 = (1<<1),
  NV17 = (1<<2),
  NV1X = (NV10 | NV17),
  NV20 = (1<<3),
  NV25 = (1<<4),
  NV2X = (NV20 | NV25),
  NV30 = (1<<5),
  NV31 = (1<<6),
  NV35 = (1<<7),
  NV3X = (NV30 | NV31 | NV35),
  NV40 = (1<<8),
  NV41 = (1<<9),
  NV43 = (1<<10),
  NV44 = (1<<11),
  NV46 = (1<<12),
  NV47 = (1<<13),
  NV49 = (1<<14),
  NV4B = (1<<15),
  C51  = (1<<16),
  G78 = (1<<17),
  NV4X = (NV40 | NV41 | NV43 | NV44 | NV46 | NV47 | NV49 | NV4B | C51 | G78),
  NV50 = (1<<18),
  G84 = (1<<19),
  G86 = (1<<20),
  G92 = (1<<21),
  G94 = (1<<22),
  G96 = (1<<23),
  GT200 = (1<<24),
  NV5X = (NV50 | G84 | G86 | G92 | G94 | G96 | GT200),
  GF100 = (1<<25),
  NV6X = (GF100)
};

typedef struct {
  unsigned int reg_address;
  char *dev_name; // /dev/mem or /dev/nvidiaX
  uint32_t arch; // Architecture NV10, NV15, NV20 ..; for internal use only as we don't list all architectures
  unsigned short device_id;
  char adapter_name[64];

  volatile unsigned int *PDISPLAY; // NV50 display registers
  volatile unsigned int *PMC;
  volatile unsigned int *PRAMIN;
  volatile unsigned char *PROM; // Nvidia bios
} NVCard;

enum { MAX_CARDS = 0x4 };
enum { UNKNOWN = 0x0 };

extern NVCard* nv_card;
