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

//#define READ_SHORT(rom,offset) (*(u_short *)(rom + offset))
//#define READ_INT(rom, offset) (*(u_int *)(rom + offset))

#define READ_BYTE(rom, offset)  (rom[offset]&0xff)

// This file should now support big endian (imported from config.h)
#ifndef NHALE_BIG_ENDIAN
  #define READ_SHORT(rom, offset) (READ_BYTE(rom, offset+1) << 8   | READ_BYTE(rom, offset))
  #define READ_INT(rom, offset)   (READ_SHORT(rom, offset+2) << 16 | READ_SHORT(rom, offset))
  #define WRITE_LE_SHORT(data)    (data)
  #define WRITE_LE_INT(data)      (data)
  #define CRC(x,y,z) crc32_little(x,y,z)
#else
  #define READ_SHORT(rom, offset) (READ_BYTE(rom, offset) << 8   | READ_BYTE(rom, offset +1))
  #define READ_INT(rom, offset)   (READ_SHORT(rom, offset) << 16 | READ_SHORT(rom, offset+2))
  #define WRITE_LE_SHORT(data)    (((data) << 8 & 0xff00)   | ((data) >> 8 & 0x00ff ))
  #define WRITE_LE_INT(data)      (((data) << 24 & 0xff000000) | ((data) << 8 & 0x00ff0000) | ((data) >> 8 & 0x0000ff00) | ((data) >> 24 & 0x000000ff))
  #define CRC(x,y,z) crc32_big(x,y,z)
#endif

// NOTE: Whenever an index is found we should probably check for out of bounds cases before parsing values after it
// TODO: Do pre-6 series parsing and Fermi parsing

// TODO: Don't assume Nvidia always conforms to a logical format because they don't.
// NOTICE: Never divide a value from the rom and store in the nvbios structure.  Because of finite precision we cannot guarantee we can get the original value back.  This is especially important when the user doesn't edit their rom at all and thus no values should change.

enum
{
  FNBST_THLD_1 = (1 << 0),
  FNBST_THLD_2 = (1 << 1),
  CRTCL_THLD_1 = (1 << 2),
  CRTCL_THLD_2 = (1 << 3),
  THRTL_THLD_1 = (1 << 4),
  THRTL_THLD_2 = (1 << 5)
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
  uint8_t id[2]; /* first byte is ID, second byte sub-ID? */
  uint8_t len[2]; /* size of data pointed to by offset */
  uint8_t offset[2]; /* offset of data */
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

/* Parse the GeforceFX performance table */
void nv30_read_performance_table(struct nvbios *bios, int offset)
{
  short i;
  u_char start;
  u_char size;
//  int tmp = 0;

  /* read how far away the start is */
  start = bios->rom[offset];
  // !!! Warning this was a signed char *rom
  bios->perf_entries = bios->rom[offset+2];
  size = bios->rom[offset + 3];

  offset += start + 1;

  // TODO: Look for the end tag at the end of this perf table
  for(i = 0; i < bios->perf_entries; i++)
  {
    bios->perf_lst[i].active = 1; // TODO: determine if all entries are active on nv30
    bios->perf_lst[i].nvclk =  READ_INT(bios->rom, offset);

    /* The list can contain multiple distinct memory clocks.
    /  Later on the ramcfg register can tell which of the ones is the right one.
    /  But for now assume the first one is correct. It doesn't matter much if the
    /  clocks are a little lower/higher as we mainly use this to detect 3d clocks
    /
    /  Further the clock stored here is the 'real' memory frequency, the effective one
    /  is twice as high. It doesn't seem to be the case for all bioses though. In some effective
    /  and real speed entries existed but this might be patched dumps.
    */
    bios->perf_lst[i].memclk =  READ_INT(bios->rom, offset+4);

    /* Move behind the timing stuff to the fanspeed and voltage */
    bios->perf_lst[i].fanspeed = bios->rom[offset + 54];
    bios->perf_lst[i].voltage = bios->rom[offset + 55];

    offset += size;
  }
}

void nv30_write_performance_table(struct nvbios *bios, int offset)
{
  short i;
  u_char start;
  u_char size;
//  int tmp = 0;

  /* read how far away the start is */
  start = bios->rom[offset];
  // !!! Warning this was a signed char *rom
  bios->rom[offset+2] = bios->perf_entries;
  size = bios->rom[offset + 3];

  offset += start + 1;

  for(i = 0; i < bios->perf_entries; i++)
  {
    *(int *)(bios->rom + offset) = WRITE_LE_INT(bios->perf_lst[i].nvclk);

    /* The list can contain multiple distinct memory clocks.
    /  Later on the ramcfg register can tell which of the ones is the right one.
    /  But for now assume the first one is correct. It doesn't matter much if the
    /  clocks are a little lower/higher as we mainly use this to detect 3d clocks
    /
    /  Further the clock stored here is the 'real' memory frequency, the effective one
    /  is twice as high. It doesn't seem to be the case for all bioses though. In some effective
    /  and real speed entries existed but this might be patched dumps.
    */
    *(int *)(bios->rom + offset + 4) = WRITE_LE_INT(bios->perf_lst[i].memclk);

    /* Move behind the timing stuff to the fanspeed and voltage */
    bios->rom[offset + 54] = bios->perf_lst[i].fanspeed;
    bios->rom[offset + 55] = bios->perf_lst[i].voltage;

    offset += size;
  }
}

/* Convert the bios version which is stored in a numeric way to a string.
/  On NV40 bioses it is stored in 5 numbers instead of 4 which was the
/  case on old cards. The bios version on old cards could be bigger than
/  4 numbers too but that version was only stored in a string which was
/  hard to locate. On NV40 cards the version is stored in a string too,
/  for which the offset can be found at +3 in the 'S' table.
*/
void nv40_bios_version_to_str(struct nvbios *bios, char *str, short offset)
{
  int version = READ_INT(bios->rom, offset);
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

/* Parse the Geforce6/7/8 performance table */
void read_bit_performance_table(struct nvbios *bios, int offset)
{
  u_short i;
  u_char entry_size;
  u_short nvclk_offset, memclk_offset, shader_offset, fanspeed_offset, voltage_offset,delta_offset,lock_offset;

  struct BitPerformanceTableHeader *header = (struct BitPerformanceTableHeader*)(bios->rom+offset);

  /* The first byte contains a version number; based on this we set offsets to interesting entries */
  // Are locks only important to 0x24 versions?

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
    default:
      printf("Error: This performance table version is currently unsupported\n");
      return;
  }

  if(bios->verbose)
    if(header->num_active_entries > MAX_PERF_LVLS)
      printf("Warning: There seem to be more active performance table entries than built-in maximum: %d\n", MAX_PERF_LVLS);

  // +5 contains the number of entries, +4 the size of one in bytes and +3 is some 'offset'
  entry_size = header->offset + header->entry_size * header->num_entries;
  offset += header->start;

  // HACK: My collection of bioses contains a (valid) 6600 bios with two 'bogus' entries at 0x21 (100MHz) and 0x22 (200MHz) these entries aren't the default ones for sure, so skip them until we have a better entry selection algorithm.
  // FIXME: READ_SHORT(bios->rom, offset+nvclk_offset) > 200)

  i = 0;
  while(READ_INT(bios->rom,offset) != 0x04104B4D)
  {
    if(i == MAX_PERF_LVLS)
    {
      printf("Error: Excess performance table entries\n");
      break;
    }

    // On bios version 0x35, this 0x20, 0x21 .. pattern doesn't exist anymore
    // Do the last 4 bits of the first byte tell if an entry is active on 0x35?
    if ( (header->version != 0x35) && (bios->rom[offset] & 0xf0) != 0x20)
    {
      printf("Error: Performance table alignment error\n");
      break;
    }

    bios->perf_lst[i].active = (i < header->num_active_entries);
    bios->perf_lst[i].fanspeed = bios->rom[offset+fanspeed_offset];
    bios->perf_lst[i].voltage = bios->rom[offset+voltage_offset];

    bios->perf_lst[i].nvclk = READ_SHORT(bios->rom, offset+nvclk_offset);
    bios->perf_lst[i].memclk = READ_SHORT(bios->rom, offset+memclk_offset);

    // !!! Warning this was signed char *
    if(bios->arch & (NV47 | NV49))
      if(bios->rom[offset+delta_offset])
        bios->perf_lst[i].delta = bios->rom[offset+delta_offset+1]/bios->rom[offset+delta_offset];
    /* Geforce8 cards have a shader clock, further the memory clock is at a different offset as well */
    if(bios->arch & NV5X)
      bios->perf_lst[i].shaderclk = READ_SHORT(bios->rom, offset+shader_offset);
 //   else
 //     bios->perf_lst[i].memclk *= 2;  //FIXME

    if(bios->arch & NV4X)
      bios->perf_lst[i].lock = bios->rom[offset + lock_offset] & 0xF;

    i++;
    offset += entry_size;
  }
  bios->perf_entries = i;
#ifdef DEBUG
  printf("perf table version: %X\n", header->version);
  printf("active perf entries: %d\n", header->num_active_entries);
  printf("number of perf entries: %d\n", i);
#endif
}

void write_bit_performance_table(struct nvbios *bios, int offset)
{
  u_short i;
  u_char entry_size;
  u_short nvclk_offset, memclk_offset, shader_offset, fanspeed_offset, voltage_offset,delta_offset,lock_offset;

  struct BitPerformanceTableHeader *header = (struct BitPerformanceTableHeader*)(bios->rom+offset);

  /* The first byte contains a version number; based on this we set offsets to interesting entries */

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
    default:
      printf("Error: This performance table version is currently unsupported\n");
      return;
  }

  if(bios->verbose)
    if(header->num_active_entries > MAX_PERF_LVLS)
      printf("Warning: There seem to be more active performance table entries than built-in maximum: %d\n", MAX_PERF_LVLS);

  // +5 contains the number of entries, +4 the size of one in bytes and +3 is some 'offset'
  entry_size = header->offset + header->entry_size * header->num_entries;
  offset += header->start;

  // HACK: My collection of bioses contains a (valid) 6600 bios with two 'bogus' entries at 0x21 (100MHz) and 0x22 (200MHz) these entries aren't the default ones for sure, so skip them until we have a better entry selection algorithm.
  // FIXME: READ_SHORT(bios->rom, offset+nvclk_offset) > 200)

  i = 0;
  while(READ_INT(bios->rom,offset) != 0x04104B4D)
  {
    if(i == MAX_PERF_LVLS)
    {
      printf("Error: Excess performance table entries\n");
      break;
    }

    // On bios version 0x35, this 0x20, 0x21 .. pattern doesn't exist anymore
    // Do the last 4 bits of the first byte tell if an entry is active on 0x35?
    if ( (header->version != 0x35) && (bios->rom[offset] & 0xf0) != 0x20)
    {
      printf("Error: Performance table alignment error\n");
      break;
    }

    bios->rom[offset+fanspeed_offset] = bios->perf_lst[i].fanspeed;
    bios->rom[offset+voltage_offset] = bios->perf_lst[i].voltage;

    *(u_short *)(bios->rom + offset + nvclk_offset) = WRITE_LE_SHORT(bios->perf_lst[i].nvclk);
    *(u_short *)(bios->rom + offset + memclk_offset) = WRITE_LE_SHORT(bios->perf_lst[i].memclk);

    // !!! Warning this was signed char *
    // FIXME: need to figure out how to parse delta first
    //if(bios->arch & (NV47 | NV49))
    //  if(bios->rom[offset+delta_offset])
    //    bios->perf_lst[i].delta = bios->rom[offset+delta_offset+1]/bios->rom[offset+delta_offset];

    /* Geforce8 cards have a shader clock, further the memory clock is at a different offset as well */
    if(bios->arch & NV5X)
      *(u_short *)(bios->rom + offset + shader_offset) = WRITE_LE_SHORT(bios->perf_lst[i].shaderclk);
 //   else
 //     bios->perf_lst[i].memclk *= 2;  //FIXME

    if(bios->arch & NV4X)
      bios->rom[offset + lock_offset] |= bios->perf_lst[i].lock;

    i++;
    offset += entry_size;
  }
}

void read_bit_temperature_table(struct nvbios *bios, int offset)
{
  short i;
  struct BitTableHeader *header = (struct BitTableHeader*)(bios->rom+offset);

  // NOTE: is temp monitoring enable bit at offset + 0x5?
  //  FF = temp monitoring off; 00 = temp monitoring on?

  offset += header->start;
  for(i = 0; i < header->num_entries; i++)
  {
    u_char id = bios->rom[offset];
    short value = READ_SHORT(bios->rom, offset+1);
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
        printf("0x1: (%0x) %d 0x%0x\n", value, (value>>9) & 0x7f, value & 0x3ff);
#endif
        if(!(value & 0x8f))
          bios->temp_correction = value >> 9;
        if((value & 0x8f) == 0)
          bios->sensor_cfg.temp_correction = (value>>9) & 0x7f;
        break;
      /* An id of 4 seems to correspond to a temperature threshold but 5, 6 and 8 have similar values, what are they? */
      case 0x4: //this appears to be critical threshold
        if(bios->caps & CRTCL_THLD_2)
        {
          if(bios->verbose)printf("Unknown critical temperature threshold\n");
        }
        else if(bios->caps & CRTCL_THLD_1)
        {
          bios->crtcl_ext_thld = (value >> 4) & 0x1ff;
          bios->caps |= CRTCL_THLD_2;
        }
        else
        {
          bios->crtcl_int_thld = (value >> 4) & 0x1ff;
          bios->caps |= CRTCL_THLD_1;
        }
        break;
      case 0x5:   //this appears to be throttling threshold (permanent?)
        if(bios->caps & THRTL_THLD_2)
        {
          if(bios->verbose)printf("Unknown throttle temperature threshold\n");
        }
        else if(bios->caps & THRTL_THLD_1)
        {
          bios->thrtl_ext_thld = (value >> 4) & 0x1ff;
          bios->caps |= THRTL_THLD_2;
        }
        else
        {
          bios->thrtl_int_thld = (value >> 4) & 0x1ff;
          bios->caps |= THRTL_THLD_1;
        }
        break;
      case 0x6:   // what is this? Temporary throttle threshold?
        break;
      case 0x8:   //this appears to be fan boost threshold
        if(bios->caps & FNBST_THLD_2)
        {
          if(bios->verbose)printf("Unknown fanboost temperature threshold\n");
        }
        else if(bios->caps & FNBST_THLD_1)
        {
          bios->fnbst_ext_thld = (value >> 4) & 0x1ff;
          bios->caps |= FNBST_THLD_2;
        }
        else
        {
          bios->fnbst_int_thld = (value >> 4) & 0x1ff;
          bios->caps |= FNBST_THLD_1;
        }
        break;
      case 0x10:
        bios->sensor_cfg.diode_offset_mult = value;
        break;
      case 0x11:
        bios->sensor_cfg.diode_offset_div = value;
        break;
      case 0x12:
        bios->sensor_cfg.slope_mult = value;
        break;
      case 0x13:
        bios->sensor_cfg.slope_div = value;
        break;
#if DEBUG
      default:
        printf("0x%x: %x\n", id, value);
#endif
    }
    offset += header->entry_size;
  }
#if DEBUG
  printf("temperature table version: %#x\n", header->version);
  printf("correction: %d\n", bios->sensor_cfg.temp_correction);
  printf("offset: %.3f\n", (float)bios->sensor_cfg.diode_offset_mult / (float)bios->sensor_cfg.diode_offset_div);
  printf("slope: %.3f\n", (float)bios->sensor_cfg.slope_mult / (float)bios->sensor_cfg.slope_div);
#endif
}

void write_bit_temperature_table(struct nvbios *bios, int offset)
{
  short i;
  struct BitTableHeader *header = (struct BitTableHeader*)(bios->rom+offset);
  short old_caps = bios->caps;

  // NOTE: is temp monitoring enable bit at offset + 0x5?
  //  FF = temp monitoring off; 00 = temp monitoring on?

  offset += header->start;
  for(i = 0; i < header->num_entries; i++)
  {
    u_char id = bios->rom[offset];
    short value = READ_SHORT(bios->rom, offset+1);

    switch(id)
    {
      case 0x1:  //FIXME: check table for multiple 1's to not set two different bits (incorporate this in read)
        if(!(value & 0x8f))
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->temp_correction << 9);
        break;
      /* An id of 4 seems to correspond to a temperature threshold but 5, 6 and 8 have similar values, what are they? */
      case 0x4: //this appears to be critical threshold
        if(bios->caps & CRTCL_THLD_1)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->crtcl_int_thld << 4);
          bios->caps &= ~CRTCL_THLD_1;
        }
        else if(bios->caps & CRTCL_THLD_2)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->crtcl_ext_thld << 4);
          bios->caps &= ~CRTCL_THLD_2;
        }
        else
        {
          if(bios->verbose)printf("Unknown fanboost temperature threshold\n");
        }
        break;
      case 0x5:   //this appears to be throttling threshold (permanent?)
        if(bios->caps & THRTL_THLD_1)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->thrtl_int_thld << 4);
          bios->caps &= ~THRTL_THLD_1;
        }
        else if(bios->caps & THRTL_THLD_2)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->thrtl_ext_thld << 4);
          bios->caps &= ~THRTL_THLD_2;
        }
        else
        {
          if(bios->verbose)printf("Unknown fanboost temperature threshold\n");
        }
        break;
      case 0x6:   // what is this? Temporary throttle threshold?
        break;
      case 0x8:   //this appears to be fan boost threshold
        if(bios->caps & FNBST_THLD_1)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->fnbst_int_thld << 4);
          bios->caps &= ~FNBST_THLD_1;
        }
        else if(bios->caps & FNBST_THLD_2)
        {
          *(u_short *)(bios->rom + offset + 1) |= WRITE_LE_SHORT(bios->fnbst_ext_thld << 4);
          bios->caps &= ~FNBST_THLD_2;
        }
        else
        {
          if(bios->verbose)printf("Unknown fanboost temperature threshold\n");
        }
        break;
    }
    offset += header->entry_size;
  }
  bios->caps = old_caps;
}

/* Read the voltage table for nv30/nv40/nv50 cards */
void read_voltage_table(struct nvbios *bios, int offset)
{
  u_char entry_size=0;
  u_char start=0;
  u_int i;

  /* In case of the first voltage table revision, there was no start pointer? */
  switch(bios->rom[offset])
  {
    case 0x10:
    case 0x12:
       start = 5;
      entry_size = bios->rom[offset+1];
      // !!! Warning this was u_char *
      bios->volt_mask = bios->rom[offset+4];
      break;
    default: // 11, 20, 24?  I'm guessing 11 doesnt belong here. I need to test the others too.
             // Note that 24 only appears on newer roms with performance table version 0x40
      start = bios->rom[offset+1];
      entry_size = bios->rom[offset+3];

      /* The VID mask is stored right before the start of the first entry? */
      // !!! Warning this was u_char *
      bios->volt_mask = bios->rom[offset+start -1];
  }

  // !!! Warning this was u_char *
  bios->rom[offset+2] = bios->volt_entries;

  if(bios->verbose)
    if(bios->volt_entries > MAX_VOLT_LVLS)
      printf("Warning: There seem to be more voltage table entries than built-in maximum: %d\n", MAX_VOLT_LVLS);

  offset += start;
  for(i = 0; i < bios->volt_entries; i++)
  {
    if(i == MAX_VOLT_LVLS)
    {
      printf("Error: Excess voltage table entries\n");
      break;
    }

    bios->volt_lst[i].voltage = bios->rom[offset];
    bios->volt_lst[i].VID = bios->rom[offset + 1];
    offset += entry_size;
  }
#ifdef DEBUG
  printf("voltage table version: %X\n", bios->rom[offset]);
  printf("number of volt entries: %d\n", bios->volt_entries);
#endif
}

void write_voltage_table(struct nvbios *bios, int offset)
{
  u_char entry_size=0;
  u_char start=0;
  u_int i;

  /* In case of the first voltage table revision, there was no start pointer? */
  switch(bios->rom[offset])
  {
    case 0x10:
    case 0x12:
      start = 5;
      entry_size = bios->rom[offset+1];
      break;
    default: // 11, 20, 24?  I'm guessing 11 doesnt belong here. I need to test the others too.
             // Note that 24 only appears on newer roms with performance table version 0x40
      start = bios->rom[offset+1];
      entry_size = bios->rom[offset+3];
  }

  // !!! Warning this was u_char *
  bios->rom[offset+2] = bios->volt_entries;

  if(bios->verbose)
    if(bios->volt_entries > MAX_VOLT_LVLS)
      printf("Warning: There seem to be more voltage table entries than built-in maximum: %d\n", MAX_VOLT_LVLS);

  offset += start;
  for(i = 0; i < bios->volt_entries; i++)
  {
    if(i == MAX_VOLT_LVLS)
    {
      printf("Error: Excess voltage table entries\n");
      break;
    }

    bios->rom[offset] = bios->volt_lst[i].voltage;
    bios->rom[offset + 1] = bios->volt_lst[i].VID;
    offset += entry_size;
  }
}

void read_string_table(struct nvbios *bios, int offset, int length)
{
  u_short off;
  u_char i, len;

  if(length != 0x15)
  {
    printf("Error: Unknown String Table\n");
    return;
  }

  // Read the inverted Engineering Release string
  // The string is after the Copyright string on NV4X and after the VESA Rev on NV5X
  if(bios->arch & NV4X)
    off = READ_SHORT(bios->rom, offset + 0x06) + bios->rom[offset+0x08] + 0x1;
  else if(bios->arch & NV5X)
    off = READ_SHORT(bios->rom, offset + 0x12) + bios->rom[offset+0x14];

  nv_read_masked_segment(bios, bios->str[7], off, 0x2E, 0xFF);

  for(i = 0; i < 7; i++)
  {
    off = READ_SHORT(bios->rom, offset);
    len = bios->rom[offset + 2];
    nv_read_segment(bios, bios->str[i], off, len);
    offset += 3;
  }
}

void write_string_table(struct nvbios *bios, int offset, int length)
{
  u_short off;
  u_char i, len;

  if(length != 0x15)
  {
    printf("Error: Unknown String Table\n");
    return;
  }

  if(bios->arch & NV4X)
    off = READ_SHORT(bios->rom, offset + 0x06) + bios->rom[offset+0x08] + 0x1;
  else if(bios->arch & NV5X)
    off = READ_SHORT(bios->rom, offset + 0x12) + bios->rom[offset+0x14];

  nv_write_masked_segment(bios, bios->str[7], off, 0x2E, 0xFF);

  for(i = 0; i < 7; i++)
  {
    off = READ_SHORT(bios->rom, offset);
    len = bios->rom[offset + 2];
    nv_write_segment(bios, bios->str[i], off, len);
    offset += 3;
  }
}

// TODO: Either add functionality or remove support for this
void nv5_read(struct nvbios *bios, u_short nv_offset)
{
  /* Go to the position containing the offset to the card name, it is 30 away from NV. */
  int offset = READ_SHORT(bios->rom, nv_offset + 30);
  nv_read(bios, bios->str[0], offset);
}

void nv5_write(struct nvbios *bios, u_short nv_offset)
{
  int offset = READ_SHORT(bios->rom, nv_offset + 30);
  nv_write(bios, bios->str[0], offset);
}

void nv30_read(struct nvbios *bios, u_short nv_offset)
{
  u_short init_offset = 0;
  u_short perf_offset=0;
  u_short volt_offset=0;

  int offset = READ_SHORT(bios->rom, nv_offset + 30);
  nv_read(bios, bios->str[0],offset);

  init_offset = READ_SHORT(bios->rom, nv_offset + 0x4d);

  volt_offset = READ_SHORT(bios->rom, nv_offset + 0x98);
  read_voltage_table(bios, volt_offset);

  perf_offset = READ_SHORT(bios->rom, nv_offset + 0x94);
  nv30_read_performance_table(bios, perf_offset);
}

void nv30_write(struct nvbios *bios, u_short nv_offset)
{
  u_short init_offset = 0;
  u_short perf_offset=0;
  u_short volt_offset=0;

  int offset = READ_SHORT(bios->rom, nv_offset + 30);
  nv_write(bios, bios->str[0],offset);

  init_offset = READ_SHORT(bios->rom, nv_offset + 0x4d);

  volt_offset = READ_SHORT(bios->rom, nv_offset + 0x98);
  write_voltage_table(bios, volt_offset);

  perf_offset = READ_SHORT(bios->rom, nv_offset + 0x94);
  nv30_write_performance_table(bios, perf_offset);
}

void read_bit_structure(struct nvbios *bios, u_int bit_offset)
{
  u_short offset;
  u_short entry_length, entry_offset;
  char unknown_entry;
  FILE *fp;
  enum { ENABLE_BIT_LOG = 1 };

  if(ENABLE_BIT_LOG)
    fp = fopen("bit_table_log.txt", "w");

  struct bit_entry *entry = (struct bit_entry *)(bios->rom + bit_offset + 4);

  /* read the entries */
  while (entry->id[0] || entry->id[1])
  {
    // Big endian support
    entry_length = READ_SHORT(entry->len, 0);
    entry_offset = READ_SHORT(entry->offset, 0);

    unknown_entry = ' ';
    switch (entry->id[0])
    {
      case 0  : //bit table version
        if(bios->verbose)
        {
          if(entry_length == 0x060C)
            printf("BIT table version : %X.%X%02X\n", (entry_offset & 0x00F0) >> 4, entry_offset & 0x000F, (entry_offset & 0xFF00) >> 8);
          else
            printf("Warning: Unknown BIT table\n");
        }
        break;
      case 'B': // bios version (1), boot text display time
        nv40_bios_version_to_str(bios, bios->version[0], entry_offset);
        bios->text_time = READ_SHORT(bios->rom, entry_offset+0xA);
        break;
      case 'C': // Configuration table; it contains at least PLL parameters
        offset = READ_SHORT(bios->rom, entry_offset + 0x8);
        //read_bit_pll_table(bios, offset);
        break;
      case 'I': // Init table
        offset = READ_SHORT(bios->rom, entry_offset);
        //read_bit_init_script_table(bios, offset, entry_length);
        break;
      case 'P': // Performance table, Temperature table, and Voltage table
        offset = READ_SHORT(bios->rom, entry_offset);
        read_bit_performance_table(bios, offset);

        offset = READ_SHORT(bios->rom, entry_offset + 0xc);
        read_bit_temperature_table(bios, offset);

        offset = READ_SHORT(bios->rom, entry_offset + 0x10);
        read_voltage_table(bios, offset);
        break;
      case 'S': // table with string references
        read_string_table(bios, entry_offset, entry_length);
        break;
      case 'i': // bios version(2), bios build date, board id, hierarchy id
          nv40_bios_version_to_str(bios, bios->version[1], entry_offset);
          bios->board_id = READ_SHORT(bios->rom, entry_offset + 0xB);
          nv_read_segment(bios, bios->build_date,entry_offset + 0xF, 8);
          bios->hierarchy_id = bios->rom[entry_offset + 0x24];
        break;
      default:
        if(ENABLE_BIT_LOG)
          unknown_entry = '*';
    }

    //NOTE: 2 has a value for my rom and all my mobile 79xx series roms.  Is this something to do with delta?
    if(ENABLE_BIT_LOG)
      if(fp)
        if(entry->id[0])
        {
          int i;
          fprintf(fp, "%c%c", entry->id[0], unknown_entry);
          fprintf(fp, "| %X | %02d | %04X - %04X", entry->id[1], entry_length, entry_offset, entry_offset + entry_length - 1);

          for(i = 0; i < entry_length; i++)
          {
            if(i && !(i % 16))
              fprintf(fp,"\n                        ");
            fprintf(fp, " | %02X", bios->rom[entry_offset+i]);
          }
          fprintf(fp, "\n");
        }
    entry++;
  }

  if(ENABLE_BIT_LOG)
    if(fp)
      fclose(fp);
}

void write_bit_structure(struct nvbios *bios, u_int bit_offset)
{
  u_short offset;
  u_short entry_length, entry_offset;

  struct bit_entry *entry = (struct bit_entry *)(bios->rom + bit_offset + 4); // skip BIT

  while (entry->id[0] || entry->id[1])
  {
    // Big endian support
    entry_length = READ_SHORT(entry->len, 0);
    entry_offset = READ_SHORT(entry->offset, 0);

    switch (entry->id[0])
    {
      case 0  : //bit table version
        if(bios->verbose)
        {
          if(entry_length != 0x060C)
            printf("Warning: Unknown BIT table\n");
        }
        break;
      case 'B': // bios version (1), boot text display time
        nv40_str_to_bios_version(bios, bios->version[0], entry_offset);
        *(u_short *)(bios->rom + entry_offset + 0xA) = WRITE_LE_SHORT(bios->text_time);
        break;
      case 'C': // Configuration table; it contains at least PLL parameters
        offset = READ_SHORT(bios->rom, entry_offset + 0x8);
        //read_bit_pll_table(bios, offset);
        break;
      case 'I': // Init table
        offset = READ_SHORT(bios->rom, entry_offset);
        //read_bit_init_script_table(bios, offset, entry_length);
        break;
      case 'P': // Performance table, Temperature table, and Voltage table
        offset = READ_SHORT(bios->rom, entry_offset);
        write_bit_performance_table(bios, offset);

        offset = READ_SHORT(bios->rom, entry_offset + 0xc);
        write_bit_temperature_table(bios, offset);

        offset = READ_SHORT(bios->rom, entry_offset + 0x10);
        write_voltage_table(bios, offset);
        break;
      case 'S': // table with string references
        write_string_table(bios, entry_offset, entry_length);
        break;
      case 'i': // bios version(2), bios build date, board id, hierarchy id
          nv40_str_to_bios_version(bios, bios->version[1], entry_offset);
          *(u_short *)(bios->rom + entry_offset + 0xB) = WRITE_LE_SHORT(bios->board_id);
          nv_write_segment(bios, bios->build_date,entry_offset + 0xF, 8);
          bios->rom[entry_offset + 0x24] = bios->hierarchy_id;
        break;
    }
    entry++;
  }
}

u_int locate(struct nvbios *bios, char *str, int offset)
{
  u_int i;
  u_int size = strlen(str);

  if(!size)
    return 0;

  for(i = offset; i <= bios->rom_size - size; i++)
    if(!memcmp(str, bios->rom + i, size))
      return i;
  return 0;
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

// Verify if we are dealing with a valid bios image
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
  size_offset = 0x10 + READ_SHORT(bios->rom, 0x18);
  offset_based_size = READ_SHORT(bios->rom, size_offset);
  if(index_based_size != offset_based_size)
  {
    printf("Error: Rom size validation failure\n");
    return 0;
  }

  // PCIR tag test
  if(!(pcir_offset = locate(bios, "PCIR", 0)))
  {
    printf("Error: Could not find \"PCIR\" string\n");
    return 0;
  }

  device_id = READ_SHORT(bios->rom, pcir_offset + 6);

  // Fail if the bios is not from an Nvidia card
  if(READ_SHORT(bios->rom, pcir_offset + 4) != 0x10de)
  {
    printf("Error: Could not find Nvidia signature\n");
    return 0;
  }

  /* We are dealing with a card that only contains the BMP structure */
  if(get_gpu_arch(device_id) & (NV5 | NV1X | NV2X | NV3X))
  {
    /* The main offset starts with "0xff 0x7f NV" */
    if(!(nv_offset = locate(bios, "\xff\x7fNV", 0)))
    {
      printf("Error: Could not find \"FF7FNV\" string\n");
      return 0;
    }

    /* We don't support old bioses. Mainly some old tnt1 models */
    // !!! Warning this was signed char *
    if(bios->rom[nv_offset + 5] < 5)
    {
      printf("Error: This card/rom is too old\n");
      return 0;
    }
  }
  else
  {
  // For NV40 card the BIT structure is used instead of the BMP structure (last one doesn't exist anymore on 6600/6800le cards).
    if(!locate(bios, "BIT", pcir_offset))
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

  if(bios->verbose)
    printf("------------------------------------\n%s\n------------------------------------\n", __func__);

  // TODO: Compare opcodes/data in pramin roms to see what has changed
  // Loading from ROM might fail on laptops as sometimes GPU BIOS is hidden in the System BIOS?
  // TODO: use device id (or EEPROM id) to determine whether to read from pramin (old) or prom (new) first

  if(filename)
  {
    if(!load_bios_file(bios, filename))
    {
      printf("Error: File '%s' is either invalid or does not exist\n", filename);
      return 0;
    }
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

  parse_bios(bios);

  return 1;
}

int write_bios(struct nvbios *bios, const char *filename)
{
  if(bios->verbose)
    printf("------------------------------------\n%s\n------------------------------------\n", __func__);

  u_short bit_offset;
  u_short nv_offset;
  u_short pcir_offset;

  *(u_short *)(bios->rom + 0x54) = WRITE_LE_SHORT(bios->subven_id);
  *(u_short *)(bios->rom + 0x56) = WRITE_LE_SHORT(bios->subsys_id);
  nv_write_segment(bios, bios->mod_date, 0x38, 8);

  pcir_offset = locate(bios, "PCIR", 0);

  *(u_short *)(bios->rom + pcir_offset + 6) = WRITE_LE_SHORT(bios->device_id);

  if(bios->arch == UNKNOWN && !bios->force)
  {
    printf("Error: Bios writing is unsupported on UNKNOWN architectures.\n");
    printf("       Use -f or --force if you are sure you know what you are doing\n");
    return 0;
  }

  // We are dealing with a card that only contains the BMP structure
  if(bios->arch & (NV5 | NV1X | NV2X | NV3X))
  {
    // The main offset starts with "0xff 0x7f NV"
    nv_offset = locate(bios, "\xff\x7fNV", 0);

    // Go to the bios version
    // Not perfect for bioses containing 5 numbers
    int version = str_to_bios_version(bios->version[0]);
    *(u_int *)(bios->rom + nv_offset + 10) = WRITE_LE_INT(version);

    if(bios->arch & NV3X)
      nv30_write(bios, nv_offset);
    else
      nv5_write(bios, nv_offset);
  }
  else  // use newest methods on unknown architectures
  {
    // For NV40 card the BIT structure is used instead of the BMP structure (last one doesn't exist anymore on 6600/6800le cards).
    bit_offset = locate(bios, "BIT", 0);

    write_bit_structure(bios, bit_offset);
  }

  if(!verify_bios(bios))
    return 0;

  // More verification
  struct nvbios bios_cpy;
  memset(&bios_cpy, 0, sizeof(struct nvbios));
  memcpy(bios_cpy.rom, bios->rom, NV_PROM_SIZE);
  bios_cpy.rom_size = bios->rom_size;
  bios_cpy.checksum = bios->checksum;
  bios_cpy.crc = bios->crc;
  bios_cpy.fake_crc = bios->fake_crc;
  bios_cpy.no_correct_checksum = bios->no_correct_checksum;
  bios_cpy.pramin_priority = bios->pramin_priority;
  bios_cpy.force = bios->force;

  // keep verbose off during our internal check
  parse_bios(&bios_cpy);

  // now restore the verbose state so we can compare
  bios_cpy.verbose = bios->verbose;

  if(memcmp(&bios_cpy, bios, sizeof(struct nvbios)))
  {
    printf("Error: Internal copy verification error\n");
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
  {
    printf("Error: Unable to dump the rom image\n");
    return 0;
  }

  // Recompute checksum
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  if(!bios->no_correct_checksum)
  {
    if(bios->verbose)
      if(bios->checksum)
        printf("Correcting checksum\n");
    bios->rom[bios->rom_size - 1] = bios->rom[bios->rom_size - 1] - bios->checksum;
  }

  for(i = 0; i < bios->rom_size; i++)
    fprintf(fp, "%c", bios->rom[i]);
  fclose(fp);

  if(bios->verbose)
    printf("Bios outputted to file '%s'\n", filename);
  return 1;
}

void parse_bios(struct nvbios *bios)
{
  u_short bit_offset;
  u_short nv_offset;
  u_short pcir_offset;

  bios->caps = 0;

  // Does pcir_offset + 20 == 1 indicate BMP?

  bios->subven_id = READ_SHORT(bios->rom, 0x54);
  bios->subsys_id = READ_SHORT(bios->rom, 0x56);
  nv_read_segment(bios, bios->mod_date, 0x38, 8);

  pcir_offset = locate(bios, "PCIR", 0);

  bios->device_id = READ_SHORT(bios->rom, pcir_offset + 6);
  get_card_name(bios->device_id, bios->adapter_name);
  bios->arch = get_gpu_arch(bios->device_id);
  get_vendor_name(bios->subven_id, bios->vendor_name);

  if(bios->arch == UNKNOWN)
    printf("Warning: attempting to parse unknown architecture.\n");

  /* We are dealing with a card that only contains the BMP structure */
  if(bios->arch & (NV5 | NV1X | NV2X | NV3X))
  {
    int version;

    /* The main offset starts with "0xff 0x7f NV" */
    nv_offset = locate(bios, "\xff\x7fNV", 0);

    //TODO: Make sure this is right later
    //bios->major = (char)bios->rom[nv_offset + 5];
    //bios->minor = (char)bios->rom[nv_offset + 6];
    bios->major = bios->rom[nv_offset+5];
    bios->minor = bios->rom[nv_offset+6];

    /* Go to the bios version */
    /* Not perfect for bioses containing 5 numbers */
    version = READ_INT(bios->rom, nv_offset + 10);
    bios_version_to_str(bios->version[0], version);

    if(bios->arch & NV3X)
      nv30_read(bios, nv_offset);
    else
      nv5_read(bios, nv_offset);
  }
  else  // use newest methods on unknown architectures
  {

  /* For NV40 card the BIT structure is used instead of the BMP structure (last one doesn't exist anymore on 6600/6800le cards). */
    bit_offset = locate(bios, "BIT", 0);

    read_bit_structure(bios, bit_offset);
  }
}

/* Load the bios image from a file */
int load_bios_file(struct nvbios *bios, const char* filename)
{
  int fd = 0;
  u_char *rom;
  struct stat stbuf;
  u_int size, proj_file_size;

  stat(filename, &stbuf);
  size = stbuf.st_size;

  if((fd = open(filename, O_RDONLY)) == -1)
    return 0;

  rom = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
  if(!rom)
    return 0;

  /* Copy bios data */
  memcpy(bios->rom, rom, size);

  /* Close the bios */
  close(fd);

  proj_file_size = get_rom_size(bios);

  if(size != proj_file_size)
  {
    printf("Error: The file size %d B does not match the projected file size %d B\n", size, proj_file_size);
    return 0;
  }

  bios->rom_size = size;

  u_int i;
  // Ignore checksum because user might just want us to fix the checksum
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  if(bios->checksum)
    printf("Warning: The checksum is incorrect\n");

  // Ignore CRC on file read as we dont know if the file is for a physically connected card
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
  if(!nv_card->arch)
    return 0;

  /* On NV5x cards we need to let pramin point to the bios */
  if (nv_card->arch & NV5X)
  {
    uint32_t vbios_vram = (nv_card->PDISPLAY[0x9f04/4] & ~0xff) << 8;

    if (!vbios_vram)
      vbios_vram = (nv_card->PMC[0x1700/4] << 16) + 0xf0000;

    old_bar0_pramin = nv_card->PMC[0x1700/4];
    nv_card->PMC[0x1700/4] = (vbios_vram >> 16);
  }

  /* Copy bios data */
  rom = (u_char*)nv_card->PRAMIN;
  memcpy(bios->rom, rom, NV_PROM_SIZE);

  if (nv_card->arch & NV5X)
    nv_card->PMC[0x1700/4] = old_bar0_pramin;

  bios->rom_size = get_rom_size(bios);

  u_int i;
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  if(bios->checksum)
    return 0;

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
  enum { STABLE_COUNT = 7 };
  u_int max_delay = STABLE_COUNT;

  /* enable bios parsing; on some boards the display might turn off */
  nv_card->PMC[0x1850/4] = 0x0;

  // TODO: perhaps use the identified EEPROM to find the number of delays (faster but less flexible)

  // Very simple software debouncer for stable output
  for(i = 0; i < NV_PROM_SIZE; i++)
  {
    delay=0;
    bios->rom[i] = nv_card->PROM[i];
    for(j = 0; j < STABLE_COUNT; j++)
    {
      delay++;
      if(bios->rom[i] != nv_card->PROM[i])
      {
        bios->rom[i] = nv_card->PROM[i];
        j = -1;
      }
    }
    if(delay > max_delay)max_delay = delay;
  }

  if(bios->verbose)printf("This EEPROM probably requires %d delays\n", max_delay - STABLE_COUNT);

  /* disable the rom; if we don't do it the screens stays black on some cards */
  nv_card->PMC[0x1850/4] = 0x1;

  bios->rom_size = get_rom_size(bios);

  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  if(bios->checksum)
    return 0;

  // TODO: Find the stamped CRC in a register
  bios->crc = CRC(0, bios->rom, bios->rom_size);
  bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

  return verify_bios(bios);
}

void print_bios_info(struct nvbios *bios)
{
  // Recompute checksum and CRC
  u_int i;
  for(i = 0, bios->checksum = 0; i < bios->rom_size; i++)
    bios->checksum = bios->checksum + bios->rom[i];

  bios->crc = CRC(0, bios->rom, bios->rom_size);
  bios->fake_crc = CRC(0, bios->rom, NV_PROM_SIZE);

  printf("\nAdapter           : %s\n", bios->adapter_name);
  printf("Subvendor         : %s\n", bios->vendor_name);
  printf("File size         : %u%s KB  (%u B)\n", bios->rom_size/1024, bios->rom_size%1024 ? ".5" : "", bios->rom_size);
  printf("Checksum-8        : %02X\n", bios->checksum);
  printf("~CRC32            : %08X\n", bios->crc);
//  printf("~Fake CRC         : %08X\n", bios->fake_crc);
//  printf("CRC32?            : %08X\n", ~bios->crc);
//  printf("Fake CRC?         : %08X\n", ~bios->fake_crc);
  printf("Version [1]       : %s\n", bios->version[0]);

  if(bios->arch & (NV4X | NV5X))
    printf("Version [2]       : %s\n", bios->version[1]);

  printf("Device ID         : %04X\n", bios->device_id);
  printf("Subvendor ID      : %04X\n", bios->subven_id);
  printf("Subsystem ID      : %04X\n", bios->subsys_id);

  if(bios->arch & (NV4X | NV5X))
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

  if(bios->arch & (NV4X | NV5X))
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

  printf("\n");

  if(bios->arch <= NV3X)
    printf("BMP version: %x.%x\n", bios->major, bios->minor);

  //TODO: print delta

  char shader_num[21], lock_nibble[8];

  if(bios->perf_entries)
    printf("Perf lvl | Active |  Gpu Freq %s|  Mem Freq | Voltage | Fan  %s\n", bios->arch & NV5X ? "| Shad Freq " : "", bios->arch & NV4X ? "| Lock " : "");

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
        printf("Warning: Core clock or Memory clock is too high.  Masking clks...\n");
      display_nvclk &= 0xFFFF;
      display_memclk &= 0xFFFF;
    }

    /* The voltage is stored in multiples of 10mV, scale it to V */
    float display_voltage = (float)bios->perf_lst[i].voltage / 100.0;
    printf("%8d |    %s | %5u MHz%s | %5u MHz | %1.2f V  | %3d%%%s\n", i, bios->perf_lst[i].active ? "Yes" : "No ", display_nvclk, shader_num, display_memclk, display_voltage, bios->perf_lst[i].fanspeed, lock_nibble);
  }

  if(bios->volt_entries)
    printf("\nVID mask: %x\n", bios->volt_mask);

  for(i = 0; i < bios->volt_entries; i++)
  {
    /* The voltage is stored in multiples of 10mV, scale it to V */
    float display_voltage = (float)bios->volt_lst[i].voltage / 100.0;
    printf("Voltage level %d: %1.2fV, VID: %x\n", i, display_voltage, bios->volt_lst[i].VID);
  }

  printf("\nTemparature compensation         : %d\n", bios->temp_correction);
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

  if(locate_masked_segment(bios, toggle_string, mask, first_offset+1, 5))
  {
    printf("Error: found potential speaker %s multiple times\n", state? "enable" : "disable");
    return 0;
  }

  // Here is where the AL register's previous value is reset and rewritten?
  second_offset = locate_segment(bios, reset_string, first_offset+5, 3);

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

  if(bios->verbose)printf(" + ROM EDIT : Successfully %s speaker\n", state ? "enabled" : "disabled");

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
      printf("'%c'\t%08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5));
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
        int base = READ_INT(bios->rom, offset+1);
        int number = bios->rom[offset+5];

        printf("'%c'\tbase: %08x number: %d\n", id, base, number);
        for(i=0; i<number; i++)
          printf("'%c'\t %08x: %08x\n", id, base+4*i, READ_INT(bios->rom, offset+6 + 4*i));
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
      printf("'%c'\t%08x %08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5), READ_INT(bios->rom, offset+9));
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
      printf("'%c'\t%08x %08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5), READ_INT(bios->rom, offset+9));
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
      printf("'%c'\tCRTC reg: %x CRTC index: %x AND-mask: %x OR-with: %x\n", id, READ_SHORT(bios->rom, offset+1), bios->rom[offset+3], bios->rom[offset+4], bios->rom[offset+5]);
#endif
      offset += 6;
      break;
    case 'y': /* 0x79 */
#if DEBUG
      /* +1 = register, +5 = clock */
      printf("'%c'\t%08x %08x (%dMHz)\n", id, READ_INT(bios->rom, offset+1), READ_SHORT(bios->rom, offset+5), READ_SHORT(bios->rom, offset+5)/100);
#endif
      offset += 7;
      break;
    case 'z': /* 0x7a: INIT_ZM_REG */
#if DEBUG
      /* +1 = register, +5 = value */
      printf("'%c'\t%08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5));
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
        printf("'%c'\treg: %08x size: %d number: %d", id, READ_INT(bios->rom, offset+1), size, number);
        /* why times 2? */
        for(i=0; i<number*size*2; i++)
          printf(" %08x", READ_INT(bios->rom, offset + 7 + i));
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
      printf("'%c'\t%08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5));
#endif
      offset += 18;
      break;
    case 0x97: /* 0x97 */
#if DEBUG
      printf("'%c'\t%08x %08x\n", id, READ_INT(bios->rom, offset+1), READ_INT(bios->rom, offset+5));
#endif
      offset += 13;
      break;
    default:
      printf("Unhandled init script entry with id '%c' at %04x\n", id, offset);
      return 0;
  }

  return offset;
}


void read_bit_init_script_table(struct nvbios *bios, int init_offset, int len)
{
  int offset;
//  int done=0;
  u_char id;

  /* Table 1 */
  offset = READ_SHORT(bios->rom, init_offset);

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
      int reg = READ_INT(bios->rom, offset+1);
      u_int val = READ_INT(bios->rom, offset+5);
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
  offset = READ_SHORT(bios->rom, init_offset);

  for(i=0; i<=len; i+=2)
  {
    /* Not all tables have to exist */
    if(!offset)
    {
      init_offset += 2;
      offset = READ_SHORT(bios->rom, init_offset);
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
    offset = READ_SHORT(bios->rom, init_offset);
  }
#endif

}

/* Parse the table containing pll programming limits */
void read_bit_pll_table(struct nvbios *bios, u_short offset)
{
  struct BitTableHeader *header = (struct BitTableHeader*)(bios->rom+offset);
  int i;

  offset += header->start;
  for(i=0; i<header->num_entries; i++)
  {
    /* Each type of pll (corresponding to a certain register) has its own limits */
    bios->pll_lst[i].reg = READ_INT(bios->rom, offset);

    /* Minimum/maximum frequency each VCO can generate */
    bios->pll_lst[i].VCO1.minFreq = READ_SHORT(bios->rom, offset+0x4)*1000;
    bios->pll_lst[i].VCO1.maxFreq = READ_SHORT(bios->rom, offset+0x6)*1000;
    bios->pll_lst[i].VCO2.minFreq = READ_SHORT(bios->rom, offset+0x8)*1000;
    bios->pll_lst[i].VCO2.maxFreq = READ_SHORT(bios->rom, offset+0xa)*1000;

    /* Minimum/maximum input frequency for each VCO */
    bios->pll_lst[i].VCO1.minInputFreq = READ_SHORT(bios->rom, offset+0xc)*1000;
    bios->pll_lst[i].VCO1.maxInputFreq = READ_SHORT(bios->rom, offset+0xe)*1000;
    bios->pll_lst[i].VCO2.minInputFreq = READ_SHORT(bios->rom, offset+0x10)*1000;
    bios->pll_lst[i].VCO2.maxInputFreq = READ_SHORT(bios->rom, offset+0x12)*1000;

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
    printf("register: %#08x\n", READ_INT(bios->rom, offset));

    /* Minimum/maximum frequency each VCO can generate */
    printf("minVCO_1: %d\n", READ_SHORT(bios->rom, offset+0x4));
    printf("maxVCO_1: %d\n", READ_SHORT(bios->rom, offset+0x6));
    printf("minVCO_2: %d\n", READ_SHORT(bios->rom, offset+0x8));
    printf("maxVCO_2: %d\n", READ_SHORT(bios->rom, offset+0xa));

    /* Minimum/maximum input frequency for each VCO */
    printf("minVCO_1_in: %d\n", READ_SHORT(bios->rom, offset+0xc));
    printf("minVCO_2_in: %d\n", READ_SHORT(bios->rom, offset+0xe));
    printf("maxVCO_1_in: %d\n", READ_SHORT(bios->rom, offset+0x10));
    printf("maxVCO_2_in: %d\n", READ_SHORT(bios->rom, offset+0x12));

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
