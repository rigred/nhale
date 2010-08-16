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
  unsigned int rom_size; //rom_size could be NV_PROM_SIZE or less (multiple of 512 bits)
  unsigned char rom[NV_PROM_SIZE]; // raw data from bios
  unsigned char checksum;
  unsigned int crc;
  unsigned int fake_crc;  //TODO: remove this

  unsigned short subven_id;
  unsigned short subsys_id;
  unsigned short board_id;
  unsigned short device_id;
  unsigned char hierarchy_id;
  unsigned char major;
  unsigned char minor;
  char build_date[9];
  char mod_date[9];
  char adapter_name[64];
  char vendor_name[64];
  char str[8][256];
  char version[2][20];

  short caps;
  char no_correct_checksum; //do not correct the checksum on file save

  short temp_correction; //seems like this should be signed
  unsigned short fnbst_int_thld;
  unsigned short fnbst_ext_thld;
  unsigned short thrtl_int_thld;
  unsigned short thrtl_ext_thld;
  unsigned short crtcl_int_thld;
  unsigned short crtcl_ext_thld;

  unsigned short text_time;


  short volt_entries;
  short volt_mask;
  struct voltage volt_lst[MAX_VOLT_LVLS];

  short perf_entries;
  struct performance perf_lst[MAX_PERF_LVLS];

  short pll_entries;
  struct pll pll_lst[16];

  struct sensor sensor_cfg;

  /* Cache the 'empty' PLLs, this is needed for PLL calculation */
  unsigned int mpll;
  unsigned int nvpll;
  unsigned int spll;

  unsigned int pipe_cfg; /* Used to cache the NV4x pipe_cfg register */
};

void nv_read(struct nvbios *, char *, unsigned short);
void nv_read_masked_segment(struct nvbios *, char *, unsigned short, unsigned short, unsigned char);
void bios_version_to_str(char *, int);
void parse_nv30_performance_table(struct nvbios *, int);
void nv40_bios_version_to_str(struct nvbios *, char *, short);
int bit_init_script_table_get_next_entry(struct nvbios *, int);
void parse_bit_init_script_table(struct nvbios *, int, int);
void parse_bit_performance_table(struct nvbios *, int);
void parse_bit_pll_table(struct nvbios *, unsigned short);
void parse_bit_temperature_table(struct nvbios *, int);
void parse_voltage_table(struct nvbios *, int);
void parse_string_table(struct nvbios *, int, int);
void nv5_parse(struct nvbios *, unsigned short);
void nv30_parse(struct nvbios *, unsigned short);
void parse_bit_structure(struct nvbios *, unsigned int);
unsigned int locate(struct nvbios *, char *, int);
unsigned int locate_segment(struct nvbios *, unsigned char *, unsigned short, unsigned short);
unsigned int locate_masked_segment(struct nvbios *, unsigned char *, unsigned char *, unsigned short, unsigned short);
unsigned int get_rom_size(struct nvbios *);
int verify_bios(struct nvbios *);
int read_bios(struct nvbios *, const char *);
int dump_bios(struct nvbios *, const char *);
int load_bios_file(struct nvbios *, const char *);
int load_bios_pramin(struct nvbios *);
int load_bios_prom(struct nvbios *);
void parse_bios(struct nvbios *);
void print_bios_info(struct nvbios *);
void print_nested_func_names(int, const char *);
int set_speaker(struct nvbios *, char);
int disable_print(void);
