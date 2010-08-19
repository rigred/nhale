/*
 * Copyright(C) 2010 Andrew Powell
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

#include <stdlib.h>
#include <stdio.h>

#include "backend.h"
#include "back_linux.h"
#include "bios.h"

//hacker.c/developer.c to try test byte ptr's and call's?

//NOTICE, NOTE, FIXME, TODO
//TODO: Use uintn_t's where necessary.
//TODO: Try to access PCI BIOS on my machine?
//TODO: Look at xf86 qt 7.6 (or 7.4?) mouse cursor lag issue?
//TODO: Show MXM version
//TODO: print the BDF PCI info?
//TODO: read EEPROM ID and map to name; chip is SPI, use spi (write) to probe
//TODO: Assert NV_PROM_SIZE <= Physical EEPROM size
//TODO: Use CRC for verification on PRAMIN and PROM dumps
//TODO: Determine memory manufacturer?
//TODO: make a g_debug_print
//TODO: expand bios caps to indicate what values the bios has
//NOTE: 0x100 seems to be important value for nvflash parsing on my 3600M
//TODO: Try to find hidden rom in system bios on laptops with no video bios stored on PROM (integrate with flashrom)

NVCard *nv_card;

int main(void)
{
  int i, num_cards;
  NVCard cards[MAX_CARDS];
  struct nvbios bios;
  char filename[13];

  num_cards = probe_devices(cards);

  for(i = 0; i < num_cards; i++)
  {
    nv_card = cards + i;
    map_mem(nv_card->dev_name);

    if(read_bios(&bios, NULL))
    {
      print_bios_info(&bios);
      sprintf(filename, "dump%X.rom", i);
      //set_speaker(&bios,0);
      if(write_bios(&bios, filename))
        printf("Bios outputted to file '%s'\n", filename);
    }

    unmap_mem();
  }

  return 0;
}
