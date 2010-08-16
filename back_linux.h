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

int check_driver(void);
int probe_devices(NVCard *);
int IsVideoCard(unsigned short);
int32_t pciReadLong(unsigned short, long);
int map_mem(const char *);
void unmap_mem(void);
void *map_dev_mem(int, unsigned long, unsigned long);
void unmap_dev_mem(unsigned long, unsigned long);
