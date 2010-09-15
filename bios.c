/*
 * Copyright(C) 2010 Andrew Powell
 *
 * Copyright(C) 2001-2007 Roderick Colenbrander
 * The original author
 *
 * Copyright(C) 2005 Hans-Frieder Vogt
 * NV40 bios parsing improvements (BIT parsing rewrite + performance table fixes)
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

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "backend.h"
#include "bios.h"
#include "info.h"
#include "crc32.h"
#include "config.h"

#define READ_BYTE(rom, offset) (*(u_char *)(rom + offset))

// This file should now support big endian (imported from config.h)
// NOTICE: Never read or write any type larger than one byte from the rom without these macros

#ifndef NHALE_BIG_ENDIAN
  #define READ_LE_SHORT(rom, offset) (*(u_short *)(rom + offset))
  #define READ_LE_INT(rom, offset)   (*(u_int *)(rom + offset))
  #define WRITE_LE_SHORT(data)    (data)
  #define WRITE_LE_INT(data)      (data)
  #define CRC(x,y,z) crc32_little(x,y,z)
#else
  #define READ_LE_SHORT(rom, offset) (READ_BYTE(rom, offset+1) << 8   | READ_BYTE(rom, offset))
  #define READ_LE_INT(rom, offset)   (READ_LE_SHORT(rom, offset+2) << 16 | READ_LE_SHORT(rom, offset))
  #define WRITE_LE_SHORT(data)    (((data) << 8 & 0xff00)   | ((data) >> 8 & 0x00ff ))
  #define WRITE_LE_INT(data)      (((data) << 24 & 0xff000000) | ((data) << 8 & 0x00ff0000) | ((data) >> 8 & 0x0000ff00) | ((data) >> 24 & 0x000000ff))
  #define CRC(x,y,z) crc32_big(x,y,z)
#endif

// NOTICE: Never divide a value (by a non-power of two) from the rom and store in the nvbios structure.  Because of finite precision we cannot guarantee we can get the original value back.  This is especially important when the user doesn't edit their rom at all and thus no values should change.

// NOTE: Whenever an index is found we should probably check for out of bounds cases before parsing values after it
// NOTE: I use some tokens for table ends but I do not use tokens for table beginnings.  So far I have not been able to find a table leading to the bootup clocks so maybe I should use the token before the bootup clocks to get them.
// TODO: Do pre-6 series parsing and Fermi parsing
// TODO: Find out more about temp tables.  Also, find the softstraps and stuff.
// TODO: Find all bit table versions to make parse_bit safer
// TODO: Integrate dump_bios into write_bios
// NOTE: Only write_bios, read_bios, and print_bios should be defined publicly?
// NOTE: Make an undo stack

// these are macros for the caps member of struct nvbios

enum
{
  TEMP_CORRECTION = (1 << 0),
  FNBST_THLD_1 = (1 << 1),
  FNBST_THLD_2 = (1 << 2),
  CRTCL_THLD_1 = (1 << 3),
  CRTCL_THLD_2 = (1 << 4),
  THRTL_THLD_1 = (1 << 5),
  THRTL_THLD_2 = (1 << 6)
};

struct BitTableHeader
{
  uint8_t version;
  uint8_t start;
  uint8_t entry_size;
  uint8_t num_entries;
};

struct bit_entry
{
  uint8_t id[2]; // first byte is ID, second byte sub-ID?
  uint8_t len[2]; // size of data pointed to by offset
  uint8_t offset[2]; // offset of data
};

struct BitPerformanceTableHeader
{
  uint8_t version;
  uint8_t start;
  uint8_t num_active_entries;
  uint8_t offset;
  uint8_t entry_size;
  uint8_t num_entries;
};

// Read a string from a given offset
void nv_read(struct nvbios *bios, char *str, u_short offset)
{
  // FIXME: I should probably make sure this doesnt read in something too big for the buffer.  Really I need to find out about old signon messages so I can remove nv_read altogether.
  u_short i;
  for(i = 0; bios->rom[offset+i]; i++)
    str[i] = bios->rom[offset+i];
  str[i] = 0;
}

void nv_write(struct nvbios *bios, char *str, u_short offset)
{
  u_short i;
  for(i = 0; bios->rom[offset+i]; i++)
    bios->rom[offset+i] = str[i];
}

void nv_read_segment(struct nvbios *bios, char *str, u_short offset, u_short len)
{
  u_short i;
  for(i = 0; i < len; i++)
    str[i] = bios->rom[offset+i];
  str[i] = 0;
}

void nv_write_segment(struct nvbios *bios, char *str, u_short offset, u_short len)
{
  u_short i;
  for(i = 0; i < len; i++)
    bios->rom[offset+i] = str[i];
}

// static mask
void nv_read_masked_segment(struct nvbios *bios, char *str, u_short offset, u_short len, u_char mask)
{
  u_short i;
  for(i = 0; i < len; i++)
    str[i] = bios->rom[offset+i] ^ mask;
  str[i] = 0;
}

void nv_write_masked_segment(struct nvbios *bios, char *str, u_short offset, u_short len, u_char mask)
{
  u_short i;
  for(i = 0; i < len; i++)
    bios->rom[offset+i] = str[i] ^ mask;
}

// The bios version can be bigger than 4 numbers but it is only stored in a string which is hard to locate?
void bios_version_to_str(char *str, int version)
{
  u_char temp[4]; //temp 0 is msb and temp 3 is lsb

  temp[0] = version >> 24;
  temp[1] = version >> 16;
  temp[2] = version >> 8;
  temp[3] = version;

  sprintf(str, "%02hhX.%02hhX.%02hhX.%02hhX", temp[0], temp[1], temp[2], temp[3]);
}

int str_to_bios_version(char *str)
{
  u_char temp[4]; //temp 0 is msb and temp 3 is lsb
  int version = 0;
  sscanf(str, "%02hhX.%02hhX.%02hhX.%02hhX", temp, temp + 1, temp + 2, temp + 3);
  version = (temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3];
  return version;
}

void nv40_bios_version_to_str(struct nvbios *bios, char *str, short offset)
{
  int version = READ_LE_INT(bios->rom, offset);
  u_char temp[4]; //temp 0 is msb and temp 3 is lsb
  u_char extra = bios->rom[offset+4];

  temp[0] = version >> 24;
  temp[1] = version >> 16;
  temp[2] = version >> 8;
  temp[3] = version;

  sprintf(str, "%02hhX.%02hhX.%02hhX.%02hhX.%02hhX", temp[0], temp[1], temp[2], temp[3], extra);
}

void nv40_str_to_bios_version(struct nvbios *bios, char *str, short offset)
{
  int version = 0;
  u_char temp[4]; //temp 0 is msb and temp 3 is lsb
  u_char extra;

  sscanf(str, "%02hhX.%02hhX.%02hhX.%02hhX.%02hhX", temp, temp + 1, temp + 2, temp + 3, &extra);
  version = (temp[0] << 24) + (temp[1] << 16) + (temp[2] << 8) + temp[3];
  bios->rom[offset+4] = extra;
  *(u_int *)(bios->rom+offset) = WRITE_LE_INT(version);
}

// Parse the GeforceFX performance table
void nv30_parse_performance_table(struct nvbios *bios, int offset, char rnw)
{
  short i;
  u_char start;
  u_char size;

  // read how far away the start is
  start = bios->rom[offset];

  // TODO: find out if there are different versions

  if(rnw)
  {
    bios->perf_entries = bios->rom[offset+2];
    bios->active_perf_entries = bios->perf_entries; // TODO: determine if all entries are active on nv30
  }
  else
    bios->rom[offset+2] = bios->perf_entries;

  size = bios->rom[offset+3];
  offset += start + 1;

  // TODO: Look for the end tag at the end of this perf table
  for(i = 0; i < bios->perf_entries; i++)
  {
    if(i == MAX_PERF_LVLS)
    {
      fprintf(stderr, "Error: Excess performance table entries (internal maximum: %d)\n", MAX_PERF_LVLS);
      break;
    }

    if(rnw)
    {
      bios->perf_lst[i].nvclk =  READ_LE_INT(bios->rom, offset);

      /* The list can contain multiple distinct memory clocks.
      /  Later on the ramcfg register can tell which of the ones is the right one.
      /  But for now assume the first one is correct. It doesn't matter much if the
      /  clocks are a little lower/higher as we mainly use this to detect 3d clocks
      /
      /  Further the clock stored here is the 'real' memory frequency, the effective one
      /  is twice as high. It doesn't seem to be the case for all bioses though. In some effective
      /  and real speed entries existed but this might be patched dumps.
      */
     bios->perf_lst[i].memclk =  READ_LE_INT(bios->rom, offset+4);

      // Move behind the timing stuff to the fanspeed and voltage
      bios->perf_lst[i].fanspeed = bios->rom[offset+54];
      bios->perf_lst[i].voltage = bios->rom[offset+55];
    }
    else
    {
      *(int *)(bios->rom + offset) = WRITE_LE_INT(bios->perf_lst[i].nvclk);
      *(int *)(bios->rom + offset + 4) = WRITE_LE_INT(bios->perf_lst[i].memclk);
      bios->rom[offset+54] = bios->perf_lst[i].fanspeed;
      bios->rom[offset+55] = bios->perf_lst[i].voltage;
    }

    offset += size;
  }
}

// Parse the Geforce6/7/8 performance table
void parse_bit_performance_table(struct nvbios *bios, int offset, char rnw)
{
  u_short i;
  u_char entry_size;
  u_short nvclk_offset, memclk_offset, shader_offset, fanspeed_offset, voltage_offset,delta_offset,lock_offset;

  struct BitPerformanceTableHeader *header = (struct BitPerformanceTableHeader*)(bios->rom + offset);

  /* The first byte contains a version number; based on this we set offsets to interesting entries */
  // Are locks only important to 0x24 versions?
  // NOTE: Look on version 40 end token.
  if(rnw)
    bios->perf_table_version = header->version;

  switch(header->version)
  {
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
      shader_offset = 0;
      fanspeed_offset = 4;
      voltage_offset = 5;
      nvclk_offset = 6;
      delta_offset = 7; //FIXME;
      memclk_offset = 11;
      lock_offset = 13;
      break;
    case 0x25: /* First seen on Geforce 8800GTS bioses */
      lock_offset = 0;
      delta_offset = 0;
      fanspeed_offset = 4;
      voltage_offset = 5;
      nvclk_offset = 8;
      shader_offset = 10;
      memclk_offset = 12;
      break;
    case 0x30: /* First seen on Geforce 8600GT bioses */
    case 0x35: /* First seen on Geforce 8800GT bioses; what else is different? */
      lock_offset = 0;
      delta_offset = 0;
      fanspeed_offset = 6;
      voltage_offset = 7;
      nvclk_offset = 8;
      shader_offset = 10;
      memclk_offset = 12;
      break;
    case 0x40:
      // support will eventually be added here
    default:
      fprintf(stderr, "Error: This performance table version is currently unsupported\n");
      return;
  }

  if(rnw)
    bios->active_perf_entries = header->num_active_entries;
  else
    header->num_active_entries = bios->active_perf_entries;

  if(bios->verbose)
    if(bios->active_perf_entries > MAX_PERF_LVLS)
      fprintf(stderr, "Warning: There seem to be more active performance table entries than internal maximum: %d\n", MAX_PERF_LVLS);

  // +5 contains the number of entries, +4 the size of one in bytes and +3 is some 'offset'
  entry_size = header->offset + header->entry_size * header->num_entries;
  offset += header->start;

  // HACK: My collection of bioses contains a (valid) 6600 bios with two 'bogus' entries at 0x21 (100MHz) and 0x22 (200MHz) these entries aren't the default ones for sure, so skip them until we have a better entry selection algorithm.

  i = 0;
  while(READ_LE_INT(bios->rom,offset) != 0x04104B4D)
  {
    if(i == MAX_PERF_LVLS)
    {
      fprintf(stderr, "Error: Excess performance table entries (internal maximum: %d)\n", MAX_PERF_LVLS);
      break;
    }

    // This is very unlikely
    if(!rnw  && i == bios->perf_entries)
    {
      fprintf(stderr, "Error: Excess performance table entries (rom-based maximum: %d)\n", bios->perf_entries);
      break;
    }

    // On bios version 0x35, this 0x20, 0x21 .. pattern doesn't exist anymore
    // Do the last 4 bits of the first byte tell if an entry is active on 0x35?
    if((header->version < 0x35) && (bios->rom[offset] & 0xf0) != 0x20)
    {
      fprintf(stderr, "Error: Performance table alignment error\n");
      break;
    }

    if(rnw)
    {
      bios->perf_lst[i].fanspeed = bios->rom[offset+fanspeed_offset];
      bios->perf_lst[i].voltage = bios->rom[offset+voltage_offset];

      bios->perf_lst[i].nvclk = READ_LE_SHORT(bios->rom, offset + nvclk_offset);
      bios->perf_lst[i].memclk = READ_LE_SHORT(bios->rom, offset + memclk_offset);

      // !!! Warning this was signed char *
      if(delta_offset && bios->arch & (NV47 | NV49))
        if(bios->rom[offset+delta_offset])
          bios->perf_lst[i].delta = bios->rom[offset+delta_offset+1] / bios->rom[offset+delta_offset];
      /* Geforce8 cards have a shader clock, further the memory clock is at a different offset as well */
      if(shader_offset)
        bios->perf_lst[i].shaderclk = READ_LE_SHORT(bios->rom, offset + shader_offset);
  //   else
  //     bios->perf_lst[i].memclk *= 2;  //FIXME

      if(lock_offset)
        bios->perf_lst[i].lock = bios->rom[offset+lock_offset] & 0x0F;
    }
    else
    {
      bios->rom[offset+fanspeed_offset] = bios->perf_lst[i].fanspeed;
      bios->rom[offset+voltage_offset] = bios->perf_lst[i].voltage;

      *(u_short *)(bios->rom + offset + nvclk_offset) = WRITE_LE_SHORT(bios->perf_lst[i].nvclk);
      *(u_short *)(bios->rom + offset + memclk_offset) = WRITE_LE_SHORT(bios->perf_lst[i].memclk);

      // !!! Warning this was signed char *
      // FIXME: need to figure out how to parse delta first
      //if(delta_offset && bios->arch & (NV47 | NV49))
      //  if(bios->rom[offset+delta_offset])
      //    bios->perf_lst[i].delta = bios->rom[offset+delta_offset+1] / bios->rom[offset+delta_offset];

      /* Geforce8 cards have a shader clock, further the memory clock is at a different offset as well */
      if(shader_offset)
        *(u_short *)(bios->rom + offset + shader_offset) = WRITE_LE_SHORT(bios->perf_lst[i].shaderclk);
  //   else
  //     bios->perf_lst[i].memclk *= 2;  //FIXME

      if(lock_offset)
      {
        bios->rom[offset+lock_offset] &= 0xF0;
        bios->rom[offset+lock_offset] |= bios->perf_lst[i].lock;
      }
    }

    i++;
    offset += entry_size;
  }

  bios->perf_entries = i;
#ifdef DEBUG
  if(rnw)
  {
    printf("perf table version: %X\n", header->version);
    printf("active perf entries: %d\n", header->num_active_entries);
    printf("number of perf entries: %d\n", i);
  }
#endif
}

void parse_bit_temperature_table(struct nvbios *bios, int offset, char rnw)
{
  short i;
  struct BitTableHeader *header = (struct BitTableHeader*)(bios->rom + offset);
  int old_caps;

  if(rnw)
    bios->temp_table_version = header->version;
  // versions: 0x20, 0x21, 0x23
  // new perf40 versions: 0x24, 0x40 ? This is with bit offset 0x10 in read_bit_structure

  // NOTE: is temp monitoring enable bit at offset + 0x5?  Its really in the switch-case 1 below
  // ? Force offset + 5 to 0xFF (or 0x10?) to disable temp monitoring (7800 aftermarket cooling issue?)
  // Force offset + 5 to 0x00 to enable temp monitoring for 6600 series

  int critical_thld_caps[2] = { CRTCL_THLD_1, CRTCL_THLD_2 };
  int throttle_thld_caps[2] = { THRTL_THLD_1, THRTL_THLD_2 };
  int fanboost_thld_caps[2] = { FNBST_THLD_1, FNBST_THLD_2 };

  int *thld_caps;
  u_short *int_thld, *ext_thld;
  int j = rnw; // this is either 0 or 1

  // Are there any tokens on the temp table?
  if(!rnw)
    old_caps = bios->caps;

  offset += header->start;
  for(i = 0; i < header->num_entries; i++)
  {
    u_char id = bios->rom[offset];
    short value = READ_LE_SHORT(bios->rom, offset + 1);
    //printf("%02X: %02X %02X masked: %d\n", bios->rom[offset], bios->rom[offset+1], bios->rom[offset+2], (value>>4)&0x1ff);

    switch(id)
    {
      /* The temperature section can store settings for more than just the builtin sensor.
      /  The value of 0x0 sets the channel for which the values below are meant. Right now
      /  we ignore this as we only use option 0x10-0x13 which are specific to the builtin sensor.
      /  Further what do 0x33/0x34 contain? Those appear on Geforce7300/7600/7900 cards.
      */
      case 0x1:
#if DEBUG
        if(rnw)
          printf("0x1: (%0x) %d 0x%0x\n", value, (value>>9) & 0x7f, value & 0x3ff);
#endif
        if(rnw)
          if((value & 0x8f) == 0)
            bios->sensor_cfg.temp_correction = (value>>9) & 0x7f;

        if(!(value & 0x8f))
        {
          if(bios->caps & TEMP_CORRECTION)
          {
            if(rnw)
            {
              if(bios->verbose)
                fprintf(stderr, "Unknown temperature correction\n");
            }
            else
            {
              *(u_short *)(bios->rom + offset + 1) &= WRITE_LE_SHORT(~0xe0);
              *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->temp_correction << 9);
              bios->caps &= ~TEMP_CORRECTION;
            }
          }
          else
          {
            if(rnw)
            {
              bios->temp_correction = value >> 9;
              bios->caps |= TEMP_CORRECTION;
            }
            else
            {
              if(bios->verbose)
                fprintf(stderr, "Unknown temperature correction\n");
            }
          }
        }
        break;
      case 0x4:   // this appears to be critical threshold
      case 0x5:   // this appears to be throttling threshold (permanent?)
      case 0x6:   // I think this is fanboost threshold
      case 0x8:   // this appears to be fanboost threshold
        if(id == 0x4)
        {
          thld_caps = critical_thld_caps;
          int_thld = &bios->crtcl_int_thld;
          ext_thld = &bios->crtcl_ext_thld;
        }
        else if(id == 0x5)
        {
          thld_caps = throttle_thld_caps;
          int_thld = &bios->thrtl_int_thld;
          ext_thld = &bios->thrtl_ext_thld;
        }
        else
        {
          thld_caps = fanboost_thld_caps;
          int_thld = &bios->fnbst_int_thld;
          ext_thld = &bios->fnbst_ext_thld;
        }

        if(bios->caps & thld_caps[j])
        {
          if(rnw)
          {
            if(bios->verbose)
              fprintf(stderr, "Unknown temperature threshold for id  %d\n", id);
          }
          else
          {
            *(u_short *)(bios->rom + offset + 1) &= WRITE_LE_SHORT(~0x1ff0);
            *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(*int_thld << 4);
            bios->caps &= ~thld_caps[0];
          }
        }
        else if(bios->caps & thld_caps[1-j])
        {
          if(rnw)
          {
            *ext_thld = (value >> 4) & 0x1ff;
            bios->caps |= thld_caps[1];
          }
          else
          {
            *(u_short *)(bios->rom + offset + 1) &= WRITE_LE_SHORT(~0x1ff0);
            *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(*ext_thld << 4);
            bios->caps &= ~thld_caps[1];
          }
        }
        else
        {
          if(rnw)
          {
            *int_thld = (value >> 4) & 0x1ff;
            bios->caps |= thld_caps[0];
          }
          else
          {
            if(bios->verbose)
              fprintf(stderr, "Unknown temperature threshold for id  %d\n", id);
          }
        }
        break;
      case 0x10:
        if(rnw)
          bios->sensor_cfg.diode_offset_mult = value;
        break;
      case 0x11:
        if(rnw)
          bios->sensor_cfg.diode_offset_div = value;
        break;
      case 0x12:
        if(rnw)
          bios->sensor_cfg.slope_mult = value;
        break;
      case 0x13:
        if(rnw)
          bios->sensor_cfg.slope_div = value;
        break;
#if DEBUG
      default:
        if(rnw)
          printf("0x%x: %x\n", id, value);
#endif
    }
    offset += header->entry_size;
  }

  if(!rnw)
    bios->caps = old_caps;

#if DEBUG
  if(rnw)
  {
    printf("temperature table version: %#x\n", header->version);
    printf("correction: %d\n", bios->sensor_cfg.temp_correction);
    printf("offset: %.3f\n", (float)bios->sensor_cfg.diode_offset_mult / (float)bios->sensor_cfg.diode_offset_div);
    printf("slope: %.3f\n", (float)bios->sensor_cfg.slope_mult / (float)bios->sensor_cfg.slope_div);
  }
#endif
}

/* Read the voltage table for nv30/nv40/nv50 cards */
void parse_voltage_table(struct nvbios *bios, int offset, char rnw)
{
  u_char entry_size=0;
  u_char start=0;
  u_int i;
  u_short end_tag;
  u_char version;
  u_short active_offset = 0;
  // NOTE: I may be showing one or two more volt_entries than there actually are

  version = bios->rom[offset];

  if(rnw)
    bios->volt_table_version = version;

  switch(version)
  {
    case 0x10: // This version has no standard end_tag
      start = 5;
      entry_size = bios->rom[offset+1];
      active_offset = 2;
      break;
    case 0x11: // 0x11 table is always empty?
    case 0x12:
      start = 5;
      entry_size = bios->rom[offset+1];
      end_tag = 0x4D49;
      active_offset = 2;
      break;
    case 0x20:
      start = bios->rom[offset+1];
      entry_size = bios->rom[offset+3];
      end_tag = 0x4D49;
      active_offset = 2;
      break;
    case 0x30:
      start = bios->rom[offset+1];
      entry_size = bios->rom[offset+2];
      end_tag = 0x0424;
      active_offset = 3;
      break;
    case 0x40:
    default:
      printf("Currently unsupported voltage table version\n");
      return;
  }

  if(rnw)
  {
    bios->volt_mask = bios->rom[offset+start-1];
    bios->active_volt_entries = bios->rom[offset+active_offset];
  }
  else
    bios->rom[offset+active_offset] = bios->active_volt_entries;

#ifdef DEBUG
  if(rnw)
  {
    printf("voltage table version: %X\n", bios->rom[offset]);
    printf("number of volt entries: %d\n", bios->volt_entries);
  }
#endif

  if(bios->verbose)
    if(bios->active_volt_entries > MAX_VOLT_LVLS)
      fprintf(stderr, "Warning: There seem to be more active voltage table entries than internal maximum: %d\n", MAX_VOLT_LVLS);

  offset += start;
//  for(i = 0; i < bios->volt_entries; i++)
  i = 0;
  while((version != 0x10 && READ_LE_SHORT(bios->rom, offset) != end_tag) || (version == 0x10 && i < 7))
  {
    if(i == MAX_VOLT_LVLS)
    {
      fprintf(stderr, "Error: Excess voltage table entries (internal maximum: %d)\n", MAX_VOLT_LVLS);
      break;
    }

    if(rnw)
    {
      bios->volt_lst[i].voltage = bios->rom[offset];
      bios->volt_lst[i].VID = bios->rom[offset+1];
    }
    else
    {
      bios->rom[offset] = bios->volt_lst[i].voltage;
      bios->rom[offset+1] = bios->volt_lst[i].VID;
    }

    i++;
    offset += entry_size;
  }

  if(rnw)
    bios->volt_entries = i;
}

void parse_string_table(struct nvbios *bios, int offset, int length, char rnw)
{
  u_short off;
  u_char i, len;

  if(length != 0x15)
  {
    fprintf(stderr, "Error: Unknown String Table\n");
    return;
  }

  // Read the inverted Engineering Release string
  // The string is after the Copyright string on NV4X and after the VESA Rev on NV5X
  if(bios->arch & NV4X)
    off = READ_LE_SHORT(bios->rom, offset + 0x06) + bios->rom[offset+0x08] + 0x1;
  else
    off = READ_LE_SHORT(bios->rom, offset + 0x12) + bios->rom[offset+0x14];

  if(rnw)
    nv_read_masked_segment(bios, bios->str[7], off, 0x2E, 0xFF);
  else
    nv_write_masked_segment(bios, bios->str[7], off, 0x2E, 0xFF);

  for(i = 0; i < 7; i++)
  {
    off = READ_LE_SHORT(bios->rom, offset);
    len = bios->rom[offset+2];

    if(rnw)
      nv_read_segment(bios, bios->str[i], off, len);
    else
      nv_write_segment(bios, bios->str[i], off, len);

    offset += 3;
  }
}

void nv5_parse(struct nvbios *bios, u_short nv_offset, char rnw)
{
  /* Go to the position containing the offset to the card name, it is 30 away from NV. */
  int offset = READ_LE_SHORT(bios->rom, nv_offset + 30);

  if(rnw)
    nv_read(bios, bios->str[0], offset);
  else
    nv_write(bios, bios->str[0], offset);
}

void nv30_parse(struct nvbios *bios, u_short nv_offset, char rnw)
{
  u_short init_offset = 0;
  u_short perf_offset=0;
  u_short volt_offset=0;

  int offset = READ_LE_SHORT(bios->rom, nv_offset + 30);

  if(rnw)
    nv_read(bios, bios->str[0],offset);
  else
    nv_write(bios, bios->str[0],offset);

  init_offset = READ_LE_SHORT(bios->rom, nv_offset + 0x4d);

  volt_offset = READ_LE_SHORT(bios->rom, nv_offset + 0x98);
  parse_voltage_table(bios, volt_offset, rnw);

  perf_offset = READ_LE_SHORT(bios->rom, nv_offset + 0x94);
  nv30_parse_performance_table(bios, perf_offset, rnw);
}

void parse_bit_structure(struct nvbios *bios, u_int bit_offset, char rnw)
{
  u_short offset;
  u_short entry_length, entry_offset;
  u_short bit_table_version = 0;

  struct bit_entry *entry = (struct bit_entry *)(bios->rom + bit_offset + 4); // skip ["BIT"\0]

  // read the entries
  while (entry->id[0] || entry->id[1])
  {
    // Big endian support
    entry_length = READ_LE_SHORT(entry->len, 0);
    entry_offset = READ_LE_SHORT(entry->offset, 0);

    switch (entry->id[0])  // TODO: To be completely safe I should make sure each case is entered at most 1 time
    {
      case 0  : //bit table version
        if(bios->verbose)
        {
          if(entry_length == 0x060C)
            printf("BIT table version : %X.%X%02X\n", (entry_offset & 0x00F0) >> 4, entry_offset & 0x000F, (entry_offset & 0xFF00) >> 8);
          else  // unknown because entry size isn't 0x6 and start isn't 0xC away
            fprintf(stderr, "Warning: Unknown BIT table\n");
        }

        bit_table_version = entry_offset;

        if(rnw)
          bios->bit_table_version = bit_table_version;
        break;
      case 'B': // bios version (1), boot text display time
        if(rnw)
        {
          nv40_bios_version_to_str(bios, bios->version[0], entry_offset);
          bios->text_time = READ_LE_SHORT(bios->rom, entry_offset + 0x0a);
        }
        else
        {
          nv40_str_to_bios_version(bios, bios->version[0], entry_offset);
          *(u_short *)(bios->rom + entry_offset + 0x0a) = WRITE_LE_SHORT(bios->text_time);
        }
        break;
      case 'C': // Configuration table; it contains at least PLL parameters
        offset = READ_LE_SHORT(bios->rom, entry_offset + 0x8);
        //if(rnw)
        //  parse_bit_pll_table(bios, offset); //this function can only read
        break;
      case 'I': // Init table
        offset = READ_LE_SHORT(bios->rom, entry_offset);
        //if(rnw)
        //  parse_bit_init_script_table(bios, offset, entry_length); //this function can only read
        break;
      case 'P': // Performance table, Temperature table, and Voltage table
        // NOTE: Make sure perf table has not moved in version 0x4413.  This would affect perf & temp table versions
        // NOTE: Why is Fermi 0x4512?
        // TODO: dont use version, use arch?
        // TODO: test the rebranded G96's
        offset = READ_LE_SHORT(bios->rom, entry_offset);
        parse_bit_performance_table(bios, offset, rnw);

        // test: again not sure yet
        // NOTE: I need to change the GT200 perf40 arch's so this doesnt look so hacky
        // NOTE: One test that always works so far is if perf_table_version == 0x40
        if(bios->arch & (GF100 | UNKNOWN) || bit_table_version == 0x4413)
        {
          offset = READ_LE_SHORT(bios->rom, entry_offset + 0x0c);
          parse_voltage_table(bios,offset, rnw);

          // test: not sure this is correct
          offset = READ_LE_SHORT(bios->rom, entry_offset + 0x10); // potential offsets: 0x4, 0x8, 0x10, 0x18
          parse_bit_temperature_table(bios, offset, rnw);
        }
        else
        {
          offset = READ_LE_SHORT(bios->rom, entry_offset + 0x0c);
          parse_bit_temperature_table(bios, offset, rnw);

          offset = READ_LE_SHORT(bios->rom, entry_offset + 0x10);
          parse_voltage_table(bios, offset, rnw);
        }
        break;
      case 'S': // table with string references
        parse_string_table(bios, entry_offset, entry_length, rnw);
        break;
      case 'i': // bios version(2), bios build date, board id, hierarchy id
        if(rnw)
        {
          nv40_bios_version_to_str(bios, bios->version[1], entry_offset);
          bios->board_id = READ_LE_SHORT(bios->rom, entry_offset + 0x0b);
          nv_read_segment(bios, bios->build_date,entry_offset + 0x0f, 8);
          bios->hierarchy_id = bios->rom[entry_offset+0x24];
        }
        else
        {
          nv40_str_to_bios_version(bios, bios->version[1], entry_offset);
          *(u_short *)(bios->rom + entry_offset + 0x0b) = WRITE_LE_SHORT(bios->board_id);
          nv_write_segment(bios, bios->build_date,entry_offset + 0x0f, 8);
          bios->rom[entry_offset+0x24] = bios->hierarchy_id;
        }
        break;
    }
    entry++;
  }
}

int parse_bios(struct nvbios *bios, char rnw)
{
  u_short bit_offset;
  u_short nv_offset;
  u_short pcir_offset;

  u_char pcir_tag[5] = "PCIR";
  u_char nv_tag[5] = "\xFF\x7FNV";
  u_char bit_tag[4] = "BIT";

  // TODO: ? Maybe I should remove the arch unknown tests since its really just based on table versions

  // Does pcir_offset + 20 == 1 indicate BMP?
  if(rnw)
  {
    bios->subven_id = READ_LE_SHORT(bios->rom, 0x54);
    bios->subsys_id = READ_LE_SHORT(bios->rom, 0x56);
    nv_read_segment(bios, bios->mod_date, 0x38, 8);

    pcir_offset = locate_segment(bios, pcir_tag, 0, 4);

    bios->device_id = READ_LE_SHORT(bios->rom, pcir_offset + 6);
    get_card_name(bios->device_id, bios->adapter_name);
    bios->arch = get_gpu_arch(bios->device_id);
    get_subvendor_name(bios->subven_id, bios->vendor_name);

    if(bios->arch & UNKNOWN)
      fprintf(stderr, "Warning: attempting to parse unknown architecture.\n");

    /* We are dealing with a card that only contains the BMP structure */
    if(bios->arch <= NV3X)
    {
      int version;
      /* The main offset starts with "0xff 0x7f NV" */
      nv_offset = locate_segment(bios, nv_tag, 0, 4);

      bios->major = bios->rom[nv_offset+5];
      bios->minor = bios->rom[nv_offset+6];

      /* Go to the bios version */
      /* Not perfect for bioses containing 5 numbers */
      version = READ_LE_INT(bios->rom, nv_offset + 10);
      bios_version_to_str(bios->version[0], version);

      if(bios->arch & NV3X)
        nv30_parse(bios, nv_offset, rnw);
      else
        nv5_parse(bios, nv_offset, rnw);
    }
    else  // use newest methods on unknown architectures
    {
      /* For NV40 card the BIT structure is used instead of the BMP structure (last one doesn't exist anymore on 6600/6800le cards). */
      bit_offset = locate_segment(bios, bit_tag, 0, 3);

      parse_bit_structure(bios, bit_offset, rnw);
    }
  }
  else
  {
    *(u_short *)(bios->rom + 0x54) = WRITE_LE_SHORT(bios->subven_id);
    *(u_short *)(bios->rom + 0x56) = WRITE_LE_SHORT(bios->subsys_id);
    nv_write_segment(bios, bios->mod_date, 0x38, 8);

    pcir_offset = locate_segment(bios, pcir_tag, 0, 4);

    *(u_short *)(bios->rom + pcir_offset + 6) = WRITE_LE_SHORT(bios->device_id);

    if(bios->arch & UNKNOWN && !bios->force)
    {
      printf("Error: Bios writing is unsupported on UNKNOWN architectures.\n");
      printf("       Use -f or --force if you are sure you know what you are doing\n");
      return 0;
    }

    // We are dealing with a card that only contains the BMP structure
    if(bios->arch <= NV3X)
    {
      // The main offset starts with "0xff 0x7f NV"
      nv_offset = locate_segment(bios, nv_tag, 0, 4);

      // Go to the bios version
      // Not perfect for bioses containing 5 numbers
      int version = str_to_bios_version(bios->version[0]);
      *(u_int *)(bios->rom + nv_offset + 10) = WRITE_LE_INT(version);

      if(bios->arch & NV3X)
        nv30_parse(bios, nv_offset, rnw);
      else
        nv5_parse(bios, nv_offset, rnw);
    }
    else  // use newest methods on unknown architectures
    {
      bit_offset = locate_segment(bios, bit_tag, 0, 3);

      parse_bit_structure(bios, bit_offset, rnw);
    }

    // Recompute checksum for filesaves and CRC for user viewing purposes only
    u_int i;
    for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
      bios->checksum = bios->checksum + bios->rom[i];

    if(!bios->no_correct_checksum)
      bios->rom[bios->rom_size-1] = bios->rom[bios->rom_size-1] - bios->checksum;

    bios->crc = CRC(0, bios->rom, bios->rom_size);
    bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

    if(!verify_bios(bios))
      return 0;

  }
  return 1;
}

u_int locate_segment(struct nvbios *bios, u_char *str, u_short offset, u_short len)
{
  u_int i;

  if(!len)
    return 0;

  for(i = offset; i <= bios->rom_size - len; i++)
    if(!memcmp(str, bios->rom + i, len))
      return i;
  return 0;
}

// dynamic mask
u_int locate_masked_segment(struct nvbios *bios, u_char *str, u_char *mask, u_short offset, u_short len)
{
  u_int i,j;

  if(!len)
    return 0;

  for(i = offset; i <= bios->rom_size - len; i++)
  {
    for(j = 0; j < len; j++)
      if((bios->rom[i+j] & mask[j]) != (str[j] & mask[j]))
        break;
    if(j == len)
      return i;
  }
  return 0;
}

// Determine actual rom size
u_int get_rom_size(struct nvbios *bios)
{
  return bios->rom[2] << 9;
}

#if DEBUG

NVCard *nv_card;

int main(int argc, char **argv)
{
  struct nvbios bios, bios_cpy;
  memset(&bios, 0, sizeof(struct nvbios));
  memset(&bios_cpy, 0, sizeof(struct nvbios));
  bios.verbose = 1;

  if(!read_bios(&bios, "bios.rom"))
    return -1;

  print_bios_info(&bios);

  bios_cpy = bios;
  write_bios(&bios, NULL); // This prints an error when dumping but ignore it

  if(memcmp(&bios, &bios_cpy, sizeof(struct nvbios)))
    printf("Write bios critical error\n");

  return 0;
}


#endif

// Verify that we are dealing with a valid bios image
int verify_bios(struct nvbios *bios)
{
  u_short pcir_offset,nv_offset,device_id;
  u_short index_based_size, offset_based_size;
  u_short size_offset;

  // Signature test: All bioses start with this '0x55 0xAA'
  if((bios->rom[0] != 0x55) || (bios->rom[1] != 0xAA))
  {
    printf("Error: ROM signature failure\n");
    return 0;
  }

  // EEPROMS are getting bigger and bigger.  Maybe one day Nvidia will take advantage of this and NV_PROM_SIZE will need to be replaced by a variable that reads the physical EEPROM size or bios->rom[2]
  // The reason we are doing this check is that we mmap NV_PROM_SIZE and use it as the max size in a few places
  if(bios->rom_size > NV_PROM_SIZE)
  {
    printf("Error: This rom is too big\n");
    return 0;
  }

  // Size test
  index_based_size = bios->rom[2];
  size_offset = 0x10 + READ_LE_SHORT(bios->rom, 0x18);
  offset_based_size = READ_LE_SHORT(bios->rom, size_offset);
  if(index_based_size != offset_based_size)
  {
    printf("Error: Rom size validation failure\n");
    return 0;
  }

  // PCIR tag test
  u_char pcir_tag[5] = "PCIR";
  if(!(pcir_offset = locate_segment(bios, pcir_tag, 0, 4)))
  {
    printf("Error: Could not find \"PCIR\" string\n");
    return 0;
  }

  device_id = READ_LE_SHORT(bios->rom, pcir_offset + 6);

  // Fail if the bios is not from an Nvidia card
  if(READ_LE_SHORT(bios->rom, pcir_offset + 4) != 0x10de)
  {
    printf("Error: Could not find Nvidia signature\n");
    return 0;
  }

  /* We are dealing with a card that only contains the BMP structure */
  if(get_gpu_arch(device_id) & (NV5 | NV1X | NV2X | NV3X))
  {
    /* The main offset starts with "0xff 0x7f N V" */
    u_char nv_tag[5] = "\xFF\x7FNV";
    if(!(nv_offset = locate_segment(bios, nv_tag, 0, 4)))
    {
      printf("Error: Could not find \"FF7FNV\" string\n");
      return 0;
    }

    /* We don't support old bioses. Mainly some old tnt1 models */
    // !!! Warning this was signed char *
    if(bios->rom[nv_offset+5] < 5)
    {
      printf("Error: This card/rom is too old\n");
      return 0;
    }
  }
  else
  {
  // For NV40 card the BIT structure is used instead of the BMP structure (last one doesn't exist anymore on 6600/6800le cards).
    u_char bit_tag[4] = "BIT";
    if(!locate_segment(bios, bit_tag, pcir_offset, 3))
    {
      printf("Error: Could not find \"BIT\" string\n");
      return 0;
    }
  }

  return 1;
}

int read_bios(struct nvbios *bios, const char *filename)
{
  int (*load_bios[2])(struct nvbios *) = { &load_bios_prom, &load_bios_pramin };
  int i = bios->pramin_priority; //this is either 0 or 1

  if(!bios)
    return 0;

  if(bios->verbose)
    printf("------------------------------------\n%s\n------------------------------------\n", __func__);

  // TODO: Compare opcodes/data in pramin roms to see what has changed
  // Loading from PROM might fail on laptops as sometimes GPU BIOS is hidden in the System BIOS?
  // TODO: use device id (or EEPROM id) to determine whether to read from pramin (old) or prom (new) first

  if(filename)
  {
    if(!load_bios_file(bios, filename))
      return 0;
  }
  else
  {
    if(!(*load_bios[i])(bios)) //either prom or pramin read
    {
      if(!(*load_bios[1-i])(bios)) // the opposite of what is above
      {
        printf("Error: Unable to shadow the video bios from PROM or PRAMIN\n");
        return 0;
      }
    }
  }

  // Do not exit on this condition as the user may just want to save their bios and not edit it
  if(!parse_bios(bios, 1))
    fprintf(stderr, "Warning: Unable to parse the bios\n");

  return 1;
}

int write_bios(struct nvbios *bios, const char *filename)
{
  if(!bios)
    return 0;

  if(bios->verbose)
    printf("------------------------------------\n%s\n------------------------------------\n", __func__);

  if(!parse_bios(bios, 0) && !bios->force)        // write the (potentially edited) bios content to the rom
  {
    printf("Error: An error occured in writing the bios so output has been disabled\n");
    printf("       Use -f or --force if you are sure you know what you are doing\n");

    return 0;
  }

  // Internal verification:
  //    Detect if the user added a reserved string or a reserved end tag to the rom in an illegal place
  //    Detect if the programmer's read and write functions are not inverses of one another

  struct nvbios bios_cpy;                         // create a new bios struct
  memset(&bios_cpy, 0, sizeof(struct nvbios));    // clear all bios content
  memcpy(bios_cpy.rom, bios->rom, NV_PROM_SIZE);  // copy rom data from old bios struct to new bios struct
  bios_cpy.rom_size = bios->rom_size;             // copy some other struct bios members that parse_bios will not set
  bios_cpy.force = bios->force;

  if(!parse_bios(&bios_cpy, 1) && !bios->force)   // re-read the bios
  {
    printf("Error: An error occured in parsing the edited bios so output has been disabled\n");
    printf("       Use -f or --force if you are sure you know what you are doing\n");

    return 0;
  }

  // Copy all other struct bios members so the bioses can be compared
  bios_cpy.checksum = bios->checksum;
  bios_cpy.crc = bios->crc;
  bios_cpy.fake_crc = bios->fake_crc;
  bios_cpy.no_correct_checksum = bios->no_correct_checksum;
  bios_cpy.pramin_priority = bios->pramin_priority;
  bios_cpy.verbose = bios->verbose;

  if(memcmp(&bios_cpy, bios, sizeof(struct nvbios))) // compare the bioses
  {
    printf("Error: Unable to reparse the edited bios to get the appropriate struct bios members\n");
    return 0;
  }

  return dump_bios(bios, filename);
}

int dump_bios(struct nvbios *bios, const char *filename)
{
  FILE *fp = NULL;
  u_int i;

  //  NOTE: nvflash lets you flash the 64K Pramin with invalid checksum*

  fp = fopen(filename, "w+");
  if(!fp)
    return 0;

  for(i = 0; i < bios->rom_size; i++)
    fprintf(fp, "%c", bios->rom[i]);

  fclose(fp);

  if(bios->verbose)
    printf("Bios outputted to file '%s'\n", filename);

  return 1;
}

/* Load the bios image from a file */
int load_bios_file(struct nvbios *bios, const char* filename)
{
  int fd = 0;
  u_char *rom;
  struct stat stbuf;
  u_int size, proj_file_size;

  stat(filename, &stbuf);

  if((stbuf.st_mode & S_IFMT) == S_IFDIR)
  {
    printf("Error: %s is a directory, not a file\n", filename);
    return 0;
  }

  size = stbuf.st_size;

  if(size < 3)
  {
    printf("Error: %s has invalid file size\n", filename);
    return 0;
  }

  if((fd = open(filename, O_RDONLY)) == -1)
  {
    printf("Error: Cannot access file %s\n", filename);
    return 0;
  }

  rom = (u_char *)mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
  if(!rom)
    return 0;

  /* Copy bios data */
  memcpy(bios->rom, rom, size);

  munmap(rom, size);

  /* Close the bios */
  close(fd);

  proj_file_size = get_rom_size(bios);

  // NOTE: Should I add --force here?
  if(size != proj_file_size)
  {
    printf("Error: The file size %d B does not match the projected file size %d B\n", size, proj_file_size);
    return 0;
  }

  bios->rom_size = size;

  u_int i;
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  if(bios->checksum)
    fprintf(stderr, "Warning: File %s has an incorrect checksum\n", filename);

  // CRC check not implemented because we are unsure file corresponds to physically connected GPU.
  bios->crc = CRC(0, bios->rom, bios->rom_size);
  bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

  return verify_bios(bios);
}

/* Load the bios from video memory. Note it might not be cached there at all times. */
int load_bios_pramin(struct nvbios *bios)
{
  u_char *rom;
  uint32_t old_bar0_pramin = 0;

  /* Don't use this on unknown cards because we don't know if it needs PRAMIN fixups. */
  if(!nv_card->arch && !bios->force)
  {
    printf("Error: Reading the bios from videocard memory is disabled on unknown architectures\n");
    printf("       Use -f or --force if you are sure you know what you are doing\n");
    return 0;
  }

  /* On NV5x cards we need to let pramin point to the bios */
  if(nv_card->arch > NV4X)
  {
    uint32_t vbios_vram = (nv_card->PDISPLAY[0x9f04/4] & ~0xff) << 8;

    if(!vbios_vram)
      vbios_vram = (nv_card->PMC[0x1700/4] << 16) + 0xf0000;

    old_bar0_pramin = nv_card->PMC[0x1700/4];
    nv_card->PMC[0x1700/4] = (vbios_vram >> 16);
  }

  /* Copy bios data */
  rom = (u_char*)nv_card->PRAMIN;
  memcpy(bios->rom, rom, NV_PROM_SIZE);

  if(nv_card->arch > NV4X)
    nv_card->PMC[0x1700/4] = old_bar0_pramin;

  bios->rom_size = get_rom_size(bios);

  u_int i;
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  // I do not currently allow --force here.
  if(bios->checksum)
  {
    printf("Error: Incorrect checksum read from PRAMIN\n");
    return 0;
  }

  // TODO: Find the stamped CRC in a register
  bios->crc = CRC(0, bios->rom, bios->rom_size);
  bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

  return verify_bios(bios);
}

/* Load the video bios from the ROM. Note laptops might not have a ROM which can be accessed from the GPU */
int load_bios_prom(struct nvbios *bios)
{
  u_int i,j;
  u_int delay;
  enum { STABLE_COUNT = 7 , MAX_ALLOWED_DELAY = STABLE_COUNT * 3};
  u_int max_delay = STABLE_COUNT;

  /* enable bios parsing; on some boards the display might turn off */
  nv_card->PMC[0x1850/4] = 0x0;

  // TODO: perhaps use the identified EEPROM to find the number of delays (faster but less flexible)

  // Very simple software debouncer for stable output
  for(i = 0; i < NV_PROM_SIZE; i++)
  {
    delay = 0;
    bios->rom[i] = nv_card->PROM[i];

    for(j = 0; j < STABLE_COUNT; j++)
    {
      if(delay == MAX_ALLOWED_DELAY)
      {
        printf("Error: Timeout occurred while waiting for stable PROM output\n");
        return 0;
      }

      if(bios->rom[i] != nv_card->PROM[i])
      {
        bios->rom[i] = nv_card->PROM[i];
        j = -1;
      }

      delay++;
    }

    if(delay > max_delay)
      max_delay = delay;
  }

  if(bios->verbose)
    printf("This EEPROM probably requires %d delays\n", max_delay - STABLE_COUNT);

  /* disable the rom; if we don't do it the screens stays black on some cards */
  nv_card->PMC[0x1850/4] = 0x1;

  bios->rom_size = get_rom_size(bios);

  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  // I do not currently allow --force here.
  if(bios->checksum)
  {
    printf("Error: Incorrect checksum read from PROM\n");
    return 0;
  }

  // TODO: Find the stamped CRC in a register
  bios->crc = CRC(0, bios->rom, bios->rom_size);
  bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

  return verify_bios(bios);
}

void print_bios_info(struct nvbios *bios)
{
  if(!bios)
    return;
  // TODO: I either need to call parse_bios again before this or call parse_bios after every edit
  u_int i;

  printf("\nAdapter           : %s\n", bios->adapter_name);
  printf("Vendor            : Nvidia\n");  //currently its impossible for this to be anything else b/c of verify_bios
  printf("Subvendor         : %s\n", bios->vendor_name);
  printf("File size         : %u%s KB  (%u B)\n", bios->rom_size/1024, bios->rom_size%1024 ? ".5" : "", bios->rom_size);
  printf("Checksum-8        : %02X\n", bios->checksum);
  printf("~CRC32            : %08X\n", bios->crc);
//  printf("~Fake CRC         : %08X\n", bios->fake_crc);
//  printf("CRC32?            : %08X\n", ~bios->crc);
//  printf("Fake CRC?         : %08X\n", ~bios->fake_crc);
  printf("Version [1]       : %s\n", bios->version[0]);

  if(bios->arch > NV3X)
    printf("Version [2]       : %s\n", bios->version[1]);

  printf("Device ID         : %04X\n", bios->device_id);
  printf("Subvendor ID      : %04X\n", bios->subven_id);
  printf("Subsystem ID      : %04X\n", bios->subsys_id);

  if(bios->arch > NV3X)
  {
    printf("Board ID          : %04X\n", bios->board_id);
    printf("Hierarchy ID      : ");
    switch(bios->hierarchy_id)
    {
      case 0:
        printf("None\n");
        break;
      case 1:
        printf("Normal Board\n");
        break;
      case 2:
      case 3:
      case 4:
      case 5:
        printf("Switch Port %u\n", bios->hierarchy_id - 2);
        break;
      default:
        printf("%X\n", bios->hierarchy_id);
    }
    printf("Build Date        : %s\n", bios->build_date);
  }

  printf("Modification Date : %s\n", bios->mod_date);
  printf("Sign-on           : %s", bios->str[0]);

  if(bios->arch > NV3X)
  {
    printf("Version           : %s", bios->str[1]);
    printf("Copyright         : %s", bios->str[2]);
    printf("OEM               : %s\n", bios->str[3]);
    printf("VESA Vendor       : %s\n", bios->str[4]);
    printf("VESA Name         : %s\n", bios->str[5]);
    printf("VESA Revision     : %s\n", bios->str[6]);
    printf("Release           : %s", bios->str[7]);
    printf("Text time         : %u ms\n", bios->text_time);
  }
  else
    printf("BMP version: %x.%x\n", bios->major, bios->minor);

  //TODO: print delta

  char shader_num[21], lock_nibble[8];

  if(bios->perf_entries)
    printf("\nPerf lvl | Active |  Gpu Freq %s|  Mem Freq | Voltage | Fan  %s\n", bios->arch & NV5X ? "| Shad Freq " : "", bios->arch & NV4X ? "| Lock " : "");

  for(i = 0; i < bios->perf_entries; i++)
  {
      /* For now assume the first memory entry is the right one; should be fixed as some bioses contain various different entries */
    shader_num[0] = lock_nibble[0] = 0;

    u_int display_nvclk = bios->perf_lst[i].nvclk;
    u_int display_memclk = bios->perf_lst[i].memclk;

    if(bios->arch & NV5X)
      sprintf(shader_num, " | %5u MHz", bios->perf_lst[i].shaderclk);
    if(bios->arch & NV4X)
      sprintf(lock_nibble, " | %4X", bios->perf_lst[i].lock);
    if(bios->arch & NV3X)
    {
      display_nvclk /= 100;
      display_memclk /= 50;
      if(display_nvclk > 0xFFFF || display_memclk > 0xFFFF)
        fprintf(stderr, "Warning: Core clock or Memory clock is too high.  Masking clks...\n");
      display_nvclk &= 0xFFFF;
      display_memclk &= 0xFFFF;
    }

    /* The voltage is stored in multiples of 10mV, scale it to V */
    float display_voltage = (float)bios->perf_lst[i].voltage / 100.0;
    printf("%8d |    %s | %5u MHz%s | %5u MHz | %1.2f V  | %3d%%%s\n", i, i < bios->active_perf_entries ? "Yes" : "No ", display_nvclk, shader_num, display_memclk, display_voltage, bios->perf_lst[i].fanspeed, lock_nibble);
  }

  if(bios->volt_entries)
  {
    printf("\nVID mask: %02X\n", bios->volt_mask);
    printf("\nVolt lvl | Active | Voltage | VID\n");
  }

  for(i = 0; i < bios->volt_entries; i++)
  {
    /* The voltage is stored in multiples of 10mV, scale it to V */
    float display_voltage = (float)bios->volt_lst[i].voltage / 100.0;
    printf("%8d |    %s | %1.2f V  | %02X\n", i, i < bios->active_volt_entries ? "Yes" : "No ", display_voltage, bios->volt_lst[i].VID);
  }

  printf("\n");

  if(bios->caps & TEMP_CORRECTION)
    printf("Temparature compensation         : %d\n", bios->temp_correction);

  if(bios->caps & FNBST_THLD_1)
    printf("Fanboost internal threshold      : %u\n", bios->fnbst_int_thld);

  if(bios->caps & FNBST_THLD_2)
    printf("Fanboost external threshold      : %u\n", bios->fnbst_ext_thld);

  if(bios->caps & THRTL_THLD_1)
    printf("Throttle internal threshold      : %u\n", bios->thrtl_int_thld);

  if(bios->caps & THRTL_THLD_2)
    printf("Throttle external threshold      : %u\n", bios->thrtl_ext_thld);

  if(bios->caps & CRTCL_THLD_1)
    printf("Critical internal threshold      : %u\n", bios->crtcl_int_thld);

  if(bios->caps & CRTCL_THLD_2)
    printf("Critical external threshold      : %u\n", bios->crtcl_ext_thld);

  printf("\n");
}

// Disable/Enable PCM motherboard speaker access
// Disable: state = 0 ; Enable; state = 1
int set_speaker(struct nvbios *bios, char state)
{
  u_short first_offset;
  u_short second_offset;

  // AL is either OR'ed with 3 (opcode 0C03) or AND'ed with ~3 (opcode 24FC)
  // 0xE661 is writing AL value to port 61 (pc speaker) which connects or disconnects the speaker to a PIT timer based on AL's value.
  // The PIT has been previously configured but I do not include the PIT config in the search because NVIDIA could use different frequency pulses (PWM)
  u_char toggle_string[5] = { 0x50, 0x0C & 0x24, 0x00, 0xE6, 0x61 };
  u_char reset_string[3] = { 0x58, 0xE6, 0x61 };
  u_char mask[5] = { 0xFF, 0x0C & 0x24, 0x00, 0xFF, 0xFF };

  first_offset = locate_masked_segment(bios, toggle_string, mask, 0, 5);

  if(!first_offset)
  {
    printf("Error: could not find write to port 61 (PC Speaker)\n");
    return 0;
  }

  if(locate_masked_segment(bios, toggle_string, mask, first_offset + 1, 5))
  {
    printf("Error: found potential speaker %s multiple times\n", state? "enable" : "disable");
    return 0;
  }

  // Here is where the AL register's previous value is reset and rewritten?
  second_offset = locate_segment(bios, reset_string, first_offset + 5, 3);

  if(!second_offset)
  {
    printf("Error: could not find reset of port 61 (PC Speaker)\n");
    return 0;
  }

  if(second_offset - first_offset != 0x0B)
  {
    printf("Error: offsets may have changed.  Contact developer\n");
    return 0;
  }

  //I could NOP write to port 61 but I think this way is more confidently reversed.
  if(state)
  {
    //opcode for OR byte
    bios->rom[first_offset+1] = 0x0C;
    //operand 0x03 = enable speaker
    bios->rom[first_offset+2] |= 0x03;
  }
  else
  {
    // opcode for AND byte
    bios->rom[first_offset+1] = 0x24;
    // operand ~0x03 = disable speaker
    bios->rom[first_offset+2] &= 0xFC;
  }

  if(bios->verbose)
    printf(" + ROM EDIT : Successfully %s speaker\n", state ? "enabled" : "disabled");

  return 1;
}

int set_print(struct nvbios *bios, char state)
{
  return 0;
}

/* Init script tables contain dozens of entries containing commands to initialize
/  the card. There are lots of different commands each having a different 'id' useally
/  most entries also have a different size. The task of this function is to move to the
/  next entry in the table.
*/
// Warning this was a signed char *rom
int bit_init_script_table_get_next_entry(struct nvbios *bios, int offset)
{
  u_char id = bios->rom[offset];
//  int i=0;

  switch(id)
  {
    case '2': /* 0x32 */
      offset += 43;
      break;
    case '3': /* 0x33: INIT_REPEAT */
      offset += 2;
      break;
    case '6': /* 0x36: INIT_REPEAT_END */
      offset += 1;
      break;
    case '7': /* 0x37 */
      offset += 11;
      break;
    case '8': /* 0x38: INIT_NOT */
      offset += 1;
      break;
    case '9': /* 0x39 */
      offset += 2;
      break;
    case 'J': /* 0x4A */
      offset += 43;
      break;
    case 'K': /* 0x4B */
#if DEBUG
      /* +1 = PLL register, +5 = value */
      printf("'%c'\t%08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5));
#endif
      offset += 9;
      break;
    case 'M': /* 0x4D: INIT_ZM_I2C_BYTE */
#if DEBUG
      printf("'%c'\ti2c bytes: %x\n", id, bios->rom[offset+3]);
#endif
      offset += 4 + bios->rom[offset+3]*2;
      break;
    case 'Q': /* 0x51 */
      offset += 5 + bios->rom[offset+4];
      break;
    case 'R': /* 0x52 */
      offset += 4;
      break;
    case 'S': /* 0x53: INIT_ZM_CR */
#if DEBUG
      /* +1 CRTC index (8-bit)
      /  +2 value (8-bit)
      */
      printf("'%c'\tCRTC index: %x value: %x\n", id, bios->rom[offset+1], bios->rom[offset+2]);
#endif
      offset += 3;
      break;
    case 'T': /* 0x54 */
      offset += 2 + bios->rom[offset+1] * 2;
      break;
    case 'V': /* 0x56 */
      offset += 3;
      break;
    case 'X': /* 0x58 */
#if DEBUG
      {
        int i;
        /* +1 register base (32-bit)
        /  +5 number of values (8-bit)
        /  +6 value (32-bit) to regbase+4
        /  .. */
        int base = READ_LE_INT(bios->rom, offset+1);
        int number = bios->rom[offset+5];

        printf("'%c'\tbase: %08x number: %d\n", id, base, number);
        for(i=0; i<number; i++)
          printf("'%c'\t %08x: %08x\n", id, base+4*i, READ_LE_INT(bios->rom, offset+6 + 4*i));
      }
#endif
      offset += 6 + bios->rom[offset+5] * 4;
      break;
    case '[': /* 0x5b */
      offset += 3;
      break;
    case '_': /* 0x5F */
      offset += 22;
      break;
    case 'b': /* 0x62 */
      offset += 5;
      break;
    case 'c': /* 0x63 */
      offset +=1;
      break;
    case 'e': /* 0x65: INIT_RESET */
#if DEBUG
      /* +1 register (32-bit)
      /  +5 value (32-bit)
      /  +9 value (32-bit)
      */
      printf("'%c'\t%08x %08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5), READ_LE_INT(bios->rom, offset+9));
#endif
      offset += 13;
      break;
    case 'i': /* 0x69 */
      offset += 5;
      break;
    case 'k': /* 0x6b: INIT_SUB */
#if DEBUG
      printf("'%c' executing SUB: %x\n", id, bios->rom[offset+1]);
#endif
      offset += 2;
      break;
    case 'n': /* 0x6e */
#if DEBUG
      /* +1 = register, +5 = AND-mask, +9 = value */
      printf("'%c'\t%08x %08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5), READ_LE_INT(bios->rom, offset+9));
#endif
      offset += 13;
      break;
    case 'o': /* 0x6f */
      offset += 2;
      break;
    case 'q': /* 0x71: quit */
      offset += 1;
      break;
    case 'r': /* 0x72: INIT_RESUME */
      offset += 1;
      break;
    case 't': /* 0x74 */
      offset += 3;
      break;
    case 'u': /* 0x75: INIT_CONDITION */
#if DEBUG
      printf("'%c'\t condition: %d\n", id, bios->rom[offset+1]);
#endif
      offset += 2;
      break;
    case 'v': /* 0x76: INIT_IO_CONDITION */
#if DEBUG
      printf("'%c'\t IO condition: %d\n", id, bios->rom[offset+1]);
#endif
      offset += 2;
      break;
    case 'x': /* 0x78: INIT_INDEX_IO */
#if DEBUG
      /* +1 CRTC reg (16-bit)
      /  +3 CRTC index (8-bit)
      /  +4 AND-mask (8-bit)
      /  +5 OR-with (8-bit)
      */
      printf("'%c'\tCRTC reg: %x CRTC index: %x AND-mask: %x OR-with: %x\n", id, READ_LE_SHORT(bios->rom, offset+1), bios->rom[offset+3], bios->rom[offset+4], bios->rom[offset+5]);
#endif
      offset += 6;
      break;
    case 'y': /* 0x79 */
#if DEBUG
      /* +1 = register, +5 = clock */
      printf("'%c'\t%08x %08x (%dMHz)\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_SHORT(bios->rom, offset+5), READ_LE_SHORT(bios->rom, offset+5)/100);
#endif
      offset += 7;
      break;
    case 'z': /* 0x7a: INIT_ZM_REG */
#if DEBUG
      /* +1 = register, +5 = value */
      printf("'%c'\t%08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5));
#endif
      offset += 9;
      break;
    case 0x8e: /* 0x8e */
      /* +1 what is this doing? */
      offset += 1;
      break;
    case 0x8f: /* 0x8f: INIT_ZM_REG */
#if DEBUG
      {
        int i;
        /* +1 register 
        /  +5 = length of sequence (?)
        /  +6 = num entries
        */
        int size = bios->rom[offset+5];
        int number = bios->rom[offset+6];
        printf("'%c'\treg: %08x size: %d number: %d", id, READ_LE_INT(bios->rom, offset+1), size, number);
        /* why times 2? */
        for(i=0; i<number*size*2; i++)
          printf(" %08x", READ_LE_INT(bios->rom, offset + 7 + i));
        printf("\n");
      }
#endif
      offset += bios->rom[offset+6] * 32 + 7;
      break;
    case 0x90: /* 0x90 */
      offset += 9;
      break;
    case 0x91: /* 0x91 */
#if DEBUG
      /* +1 = pll register, +5 = ?, +9 = ?, +13 = ? */
      printf("'%c'\t%08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5));
#endif
      offset += 18;
      break;
    case 0x97: /* 0x97 */
#if DEBUG
      printf("'%c'\t%08x %08x\n", id, READ_LE_INT(bios->rom, offset+1), READ_LE_INT(bios->rom, offset+5));
#endif
      offset += 13;
      break;
    default:
      printf("Unhandled init script entry with id '%c' at %04x\n", id, offset);
      return 0;
  }

  return offset;
}


void parse_bit_init_script_table(struct nvbios *bios, int init_offset, int len)
{
  int offset;
//  int done=0;
  u_char id;

  /* Table 1 */
  offset = READ_LE_SHORT(bios->rom, init_offset);

  /* For pipeline modding purposes we cache 0x1540 and for PLL generation the PLLs */
  id = bios->rom[offset];
  while(id != 'q')
  {
    offset = bit_init_script_table_get_next_entry(bios, offset);
    /* Break out of the loop if we find an unknown entry id */
    if(!offset)
      break;
    id = bios->rom[offset];

    if(id == 'z')
    {
      int reg = READ_LE_INT(bios->rom, offset+1);
      u_int val = READ_LE_INT(bios->rom, offset+5);
      switch(reg)
      {
        case 0x1540:
          bios->pipe_cfg = val;
          break;
        case 0x4000:
          bios->nvpll = val;
          break;
        case 0x4020:
          bios->mpll = val;
          break;
      }
    }
  }

#if DEBUG /* Read all init tables and print some debug info */
  int i;
/* Table 1 */
  offset = READ_LE_SHORT(bios->rom, init_offset);

  for(i=0; i<=len; i+=2)
  {
    /* Not all tables have to exist */
    if(!offset)
    {
      init_offset += 2;
      offset = READ_LE_SHORT(bios->rom, init_offset);
      continue;
    }

    printf("Init script table %d\n", i/2+1);
    id = bios->rom[offset];

    while(id != 'q')
    {
      /* Break out of the loop if we find an unknown entry id */
      if(!offset)
        break;

      if(!(id == 'K' || id == 'n' || id == 'x' || id == 'y' || id == 'z'))
        printf("'%c' (%x)\n", id, id);
      offset = bit_init_script_table_get_next_entry(bios, offset);
      id = bios->rom[offset];
    }

    /* Pointer to next init table */
    init_offset += 2;
    /* Get location of next table */
    offset = READ_LE_SHORT(bios->rom, init_offset);
  }
#endif

}

/* Parse the table containing pll programming limits */
void parse_bit_pll_table(struct nvbios *bios, u_short offset)
{
  struct BitTableHeader *header = (struct BitTableHeader*)(bios->rom+offset);
  int i;

  offset += header->start;
  for(i=0; i<header->num_entries; i++)
  {
    /* Each type of pll (corresponding to a certain register) has its own limits */
    bios->pll_lst[i].reg = READ_LE_INT(bios->rom, offset);

    /* Minimum/maximum frequency each VCO can generate */
    bios->pll_lst[i].VCO1.minFreq = READ_LE_SHORT(bios->rom, offset+0x4)*1000;
    bios->pll_lst[i].VCO1.maxFreq = READ_LE_SHORT(bios->rom, offset+0x6)*1000;
    bios->pll_lst[i].VCO2.minFreq = READ_LE_SHORT(bios->rom, offset+0x8)*1000;
    bios->pll_lst[i].VCO2.maxFreq = READ_LE_SHORT(bios->rom, offset+0xa)*1000;

    /* Minimum/maximum input frequency for each VCO */
    bios->pll_lst[i].VCO1.minInputFreq = READ_LE_SHORT(bios->rom, offset+0xc)*1000;
    bios->pll_lst[i].VCO1.maxInputFreq = READ_LE_SHORT(bios->rom, offset+0xe)*1000;
    bios->pll_lst[i].VCO2.minInputFreq = READ_LE_SHORT(bios->rom, offset+0x10)*1000;
    bios->pll_lst[i].VCO2.maxInputFreq = READ_LE_SHORT(bios->rom, offset+0x12)*1000;

    /* Low and high values for the dividers and multipliers */
    bios->pll_lst[i].VCO1.minN = bios->rom[offset+0x14];
    bios->pll_lst[i].VCO1.maxN = bios->rom[offset+0x15];
    bios->pll_lst[i].VCO1.minM = bios->rom[offset+0x16];
    bios->pll_lst[i].VCO1.maxM = bios->rom[offset+0x17];
    bios->pll_lst[i].VCO2.minN = bios->rom[offset+0x18];
    bios->pll_lst[i].VCO2.maxN = bios->rom[offset+0x19];
    bios->pll_lst[i].VCO2.minM = bios->rom[offset+0x1a];
    bios->pll_lst[i].VCO2.maxM = bios->rom[offset+0x1b];

    bios->pll_lst[i].var1d = bios->rom[offset+0x1d];;
    bios->pll_lst[i].var1e = bios->rom[offset+0x1e];

#if DEBUG
    printf("register: %#08x\n", READ_LE_INT(bios->rom, offset));

    /* Minimum/maximum frequency each VCO can generate */
    printf("minVCO_1: %d\n", READ_LE_SHORT(bios->rom, offset+0x4));
    printf("maxVCO_1: %d\n", READ_LE_SHORT(bios->rom, offset+0x6));
    printf("minVCO_2: %d\n", READ_LE_SHORT(bios->rom, offset+0x8));
    printf("maxVCO_2: %d\n", READ_LE_SHORT(bios->rom, offset+0xa));

    /* Minimum/maximum input frequency for each VCO */
    printf("minVCO_1_in: %d\n", READ_LE_SHORT(bios->rom, offset+0xc));
    printf("minVCO_2_in: %d\n", READ_LE_SHORT(bios->rom, offset+0xe));
    printf("maxVCO_1_in: %d\n", READ_LE_SHORT(bios->rom, offset+0x10));
    printf("maxVCO_2_in: %d\n", READ_LE_SHORT(bios->rom, offset+0x12));

    /* Low and high values for the dividers and multipliers */
    printf("N1_low: %d\n", bios->rom[offset+0x14]);
    printf("N1_high: %d\n", bios->rom[offset+0x15]);
    printf("M1_low: %d\n", bios->rom[offset+0x16]);
    printf("M1_high: %d\n", bios->rom[offset+0x17]);
    printf("N2_low: %d\n", bios->rom[offset+0x18]);
    printf("N2_high: %d\n", bios->rom[offset+0x19]);
    printf("M2_low: %d\n", bios->rom[offset+0x1a]);
    printf("M2_high: %d\n", bios->rom[offset+0x1b]);

    /* What's the purpose of these? */
    printf("1c: %d\n", bios->rom[offset+0x1c]);
    printf("1d: %d\n", bios->rom[offset+0x1d]);
    printf("1e: %d\n", bios->rom[offset+0x1e]);
    printf("\n");
#endif

    bios->pll_entries = i+1;
    offset += header->entry_size;
  }
}
