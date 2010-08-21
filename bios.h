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

struct voltage
{
  unsigned char VID;
  float voltage;
};

struct performance
{
  unsigned char active;
  unsigned short nvclk;
  int delta;  //FIXME
  unsigned short memclk;
  unsigned short shaderclk;
  unsigned char fanspeed;
  unsigned char lock;
  float voltage;
};

struct vco
{
  unsigned int minInputFreq, maxInputFreq;
  unsigned int minFreq, maxFreq;
  unsigned char minN, maxN;
  unsigned char minM, maxM;
};

struct pll
{
  unsigned int reg;
  unsigned char var1d;
  unsigned char var1e;
  struct vco VCO1;
  struct vco VCO2;
};

struct sensor
{
  int slope_div;
  int slope_mult;
  int diode_offset_div;
  int diode_offset_mult;
  int temp_correction;
};

enum { MAX_PERF_LVLS = 0x4, MAX_VOLT_LVLS = MAX_PERF_LVLS };

struct nvbios
{
  unsigned char rom[NV_PROM_SIZE]; // raw data from bios
  unsigned int rom_size; //rom_size could be NV_PROM_SIZE or less (multiple of 512 bits)
  unsigned char checksum;
  unsigned int crc;
  unsigned int fake_crc;  //TODO: remove this
  short caps;
  char no_correct_checksum; //do not correct the checksum on file save
  char verbose;
  char pramin_priority;
  uint32_t arch;

  unsigned short subven_id;
  unsigned short subsys_id;
  unsigned short board_id;
  unsigned short device_id;
  unsigned char hierarchy_id;
  unsigned char major; // non-modifiable
  unsigned char minor; // non-modifiable
  char build_date[9];
  char mod_date[9];
  char adapter_name[64];
  char vendor_name[64];
  char str[8][256];
  char version[2][20];

  short temp_correction; //seems like this should be signed
  unsigned short fnbst_int_thld;
  unsigned short fnbst_ext_thld;
  unsigned short thrtl_int_thld;
  unsigned short thrtl_ext_thld;
  unsigned short crtcl_int_thld;
  unsigned short crtcl_ext_thld;

  unsigned short text_time;


  unsigned short volt_entries;
  short volt_mask; // non-modifiable
  struct voltage volt_lst[MAX_VOLT_LVLS];

  unsigned short perf_entries;
  struct performance perf_lst[MAX_PERF_LVLS];

  unsigned short pll_entries; // non-displayable, non-modifiable
  struct pll pll_lst[16];     // non-displayable, non-modifiable

  struct sensor sensor_cfg;   // non-displayable, non-modifiable

  /* Cache the 'empty' PLLs, this is needed for PLL calculation */
  unsigned int mpll;         // non-displayable, non-modifiable
  unsigned int nvpll;        // non-displayable, non-modifiable
  unsigned int spll;         // non-displayable, non-modifiable

  /* Used to cache the NV4x pipe_cfg register */
  unsigned int pipe_cfg;     // non-displayable, non-modifiable
};

void nv_read(struct nvbios *, char *, u_short);
void nv_write(struct nvbios *bios, char *, u_short);
void nv_read_masked_segment(struct nvbios *, char *, u_short, u_short, u_char);
void nv_write_masked_segment(struct nvbios *, char *, u_short, u_short, u_char);
void bios_version_to_str(char *, int);
int str_to_bios_version(char *);
void nv30_read_performance_table(struct nvbios *, int);
void nv30_write_performance_table(struct nvbios *, int);
void nv40_bios_version_to_str(struct nvbios *, char *, short);
void nv40_str_to_bios_version(struct nvbios *, char *, short);
void read_bit_performance_table(struct nvbios *, int);
void write_bit_performance_table(struct nvbios *, int);
void read_bit_temperature_table(struct nvbios *, int);
void write_bit_temperature_table(struct nvbios *, int);
void read_voltage_table(struct nvbios *, int);
void write_voltage_table(struct nvbios *, int);
void read_string_table(struct nvbios *, int, int);
void write_string_table(struct nvbios *, int, int);
void nv5_read(struct nvbios *, u_short);
void nv5_write(struct nvbios *, u_short);
void nv30_read(struct nvbios *, u_short);
void nv30_write(struct nvbios *, u_short);
void read_bit_structure(struct nvbios *, u_int);
void write_bit_structure(struct nvbios *, u_int);
u_int locate(struct nvbios *, char *, int);
u_int locate_segment(struct nvbios *, u_char *, u_short, u_short);
u_int locate_masked_segment(struct nvbios *, u_char *, u_char *, u_short, u_short);
u_int get_rom_size(struct nvbios *);
int verify_bios(struct nvbios *);
int read_bios(struct nvbios *, const char *);
int write_bios(struct nvbios *, const char *);
int dump_bios(struct nvbios *, const char *);
void parse_bios(struct nvbios *);
int load_bios_file(struct nvbios *, const char *);
int load_bios_pramin(struct nvbios *);
int load_bios_prom(struct nvbios *);
void print_bios_info(struct nvbios *);
int set_speaker(struct nvbios *, char);
int disable_print(struct nvbios *, char);

int bit_init_script_table_get_next_entry(struct nvbios *, int);
void read_bit_init_script_table(struct nvbios *, int, int);
void read_bit_pll_table(struct nvbios *, u_short);
