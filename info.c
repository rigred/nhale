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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend.h"
#include "info.h"

struct pci_ids
{
  short id;
  const char *name;
};

// NOTE: To order this list and get new names for cards I can empty this list and run new_card script. However, I do not like a few of Nvidia's names for cards.
static const struct pci_ids ids[] =
{
  { 0x0020, "Nvidia Riva TnT" },
  { 0x0028, "Nvidia Riva TnT 2 Pro" },
  { 0x002a, "Nvidia Riva TnT 2" },
  { 0x002b, "Nvidia Riva TnT 2" },
  { 0x00a0, "Nvidia Riva TnT 2 Aladdin (Integrated)" },
  { 0x002c, "Nvidia Riva TnT 2 VANTA" },
  { 0x002d, "Nvidia Riva TnT 2 M64" },
  { 0x002e, "Nvidia Riva TnT 2 VANTA" },
  { 0x002f, "Nvidia Riva TnT 2 VANTA" },
  { 0x0029, "Nvidia Riva TnT 2 Ultra" },
  { 0x0100, "Nvidia Geforce 256 SDR" },
  { 0x0101, "Nvidia Geforce 256 DDR" },
  { 0x0103, "Nvidia Quadro" },
  { 0x0110, "Nvidia Geforce 2 MX/MX400" },
  { 0x0111, "Nvidia Geforce 2 MX 100/200" },
  { 0x0112, "Nvidia Geforce 2 GO" },
  { 0x0113, "Nvidia Quadro 2 MXR/EX/GO" },
  { 0x01a0, "Nvidia Geforce 2 MX integrated" },
  { 0x0150, "Nvidia Geforce 2 GTS/PRO" },
  { 0x0151, "Nvidia Geforce 2 Ti" },
  { 0x0152, "Nvidia Geforce 2 Ultra" },
  { 0x0153, "Nvidia Quadro 2 Pro" },
  { 0x0170, "Nvidia Geforce 4 MX460" },
  { 0x0171, "Nvidia Geforce 4 MX440" },
  { 0x0172, "Nvidia Geforce 4 MX420" },
  { 0x0173, "Nvidia Geforce 4 MX440 SE" },
  { 0x0174, "Nvidia Geforce 4 440 Go" },
  { 0x0175, "Nvidia Geforce 4 420 Go" },
  { 0x0176, "Nvidia Geforce 4 420 Go 32M" },
  { 0x0177, "Nvidia Geforce 4 460 Go" },
  { 0x0178, "Nvidia Quadro 4 550 XGL" },
  { 0x0179, "Nvidia Geforce 4 440 Go 64M" },
  { 0x017a, "Nvidia Quadro 4 200/400 NVS" },
  { 0x017b, "Nvidia Quadro 4 550 XGL" },
  { 0x017c, "Nvidia Quadro 4 500 GoGL" },
  { 0x017d, "Nvidia Geforce 410 Go" },
  { 0x0180, "Nvidia Geforce 4 MX440 8X" },
  { 0x0181, "Nvidia Geforce 4 MX440 8X" },
  { 0x0182, "Nvidia Geforce 4 MX440SE 8X" },
  { 0x0185, "Nvidia Geforce 4 MX4000" },
  { 0x0183, "Nvidia Geforce 4 MX420 8X" },
  { 0x0186, "Nvidia Geforce 4 Go" },
  { 0x0187, "Nvidia Geforce 4 488 Go" },
  { 0x0188, "Nvidia Quadro 4 580 XGL" },
  { 0x0189, "Nvidia GeForce4 MX with AGP8X (MAC)"},
  { 0x018a, "Nvidia Quadro 4 280 NVS" },
  { 0x018b, "Nvidia Quadro 4 380 XGL" },
  { 0x018c, "Nvidia Quadro NVS 50 PCI" },
  { 0x018d, "Nvidia Geforce 4 448 Go" },
  { 0x01f0, "Nvidia Geforce 4 MX integrated" },
  { 0x0200, "Nvidia Geforce 3" },
  { 0x0201, "Nvidia Geforce 3 Titanium 200" },
  { 0x0202, "Nvidia Geforce 3 Titanium 500" },
  { 0x0203, "Nvidia Quadro DCC" },
  { 0x0250, "Nvidia Geforce 4 Ti 4600" },
  { 0x0251, "Nvidia Geforce 4 Ti 4400" },
  { 0x0252, "Nvidia GeForce4 Ti" },
  { 0x0253, "Nvidia Geforce 4 Ti 4200" },
  { 0x0258, "Nvidia Quadro 4 900 XGL" },
  { 0x0259, "Nvidia Quadro 4 750 XGL" },
  { 0x025a, "Nvidia Quadro 4 600 XGL" },
  { 0x025b, "Nvidia Quadro 4 700 XGL" },
  { 0x0280, "Nvidia Geforce 4 Ti 4800" },
  { 0x0281, "Nvidia Geforce 4 Ti 4200 8X" },
  { 0x0282, "Nvidia Geforce 4 Ti 4800SE" },
  { 0x0286, "Nvidia Geforce 4 4000 GO" },
  { 0x0288, "Nvidia Quadro 4 980 XGL" },
  { 0x0289, "Nvidia Quadro 4 780 XGL" },
  { 0x028c, "Nvidia Quadro 4 700 GoGL" },
  { 0x0300, "Nvidia GeforceFX 5800" },
  { 0x0301, "Nvidia GeforceFX 5800 Ultra" },
  { 0x0302, "Nvidia GeforceFX 5800" },
  { 0x0308, "Nvidia QuadroFX 2000" },
  { 0x0309, "Nvidia QuadroFX 1000" },
  { 0x030a, "Nvidia ICE FX 2000" },
  { 0x0311, "Nvidia GeforceFX 5600 Ultra" },
  { 0x0312, "Nvidia GeforceFX 5600" },
  { 0x0313, "Nvidia NV31" },
  { 0x0314, "Nvidia GeforceFX 5600XT" },
  { 0x0316, "Nvidia NV31" },
  { 0x0317, "Nvidia NV31" },
  { 0x0318, "Nvidia NV31GL" },
  { 0x0319, "Nvidia NV31GL" },
  { 0x031a, "Nvidia GeforceFX Go 5600" },
  { 0x031b, "Nvidia GeforceFX Go 5650" },
  { 0x031c, "Nvidia QuadroFX Go700" },
  { 0x031d, "Nvidia NV31" },
  { 0x031e, "Nvidia NV31GL" },
  { 0x031f, "Nvidia NV31GL" },
  { 0x0320, "Nvidia GeForce FX 5200" },
  { 0x0321, "Nvidia GeforceFX 5200 Ultra" },
  { 0x0322, "Nvidia GeforceFX 5200" },
  { 0x0323, "Nvidia GeforceFX 5200LE" },
  { 0x0324, "Nvidia GeforceFX Go 5200" },
  { 0x0325, "Nvidia GeforceFX Go 5250" },
  { 0x0326, "Nvidia GeforceFX 5500" },
  { 0x0327, "Nvidia GeForce FX 5100" },
  { 0x0328, "Nvidia GeForceFX Go5200 32M/64M" },
  { 0x0329, "Nvidia GeForce FX 5200 (Mac)" },
  { 0x032a, "Nvidia Quadro NVS 280 PCI" },
  { 0x032b, "Nvidia QuadroFX 500" },
  { 0x032c, "Nvidia GeforceFX Go5300" },
  { 0x032d, "Nvidia GeforceFX Go5100" },
  { 0x032f, "Nvidia NV34GL" },
  { 0x0330, "Nvidia GeforceFX 5900 Ultra" },
  { 0x0331, "Nvidia GeforceFX 5900" },
  { 0x0332, "Nvidia GeforceFX 5900XT" },
  { 0x0333, "Nvidia GeforceFX 5950 Ultra" },
  { 0x0334, "Nvidia GeforceFX 5900ZT" },
  { 0x0338, "Nvidia QuadroFX 3000" },
  { 0x033f, "Nvidia GeforceFX 700" },
  { 0x0341, "Nvidia GeforceFX 5700 Ultra" },
  { 0x0342, "Nvidia GeforceFX 5700" },
  { 0x0343, "Nvidia GeforceFX 5700LE" },
  { 0x0344, "Nvidia GeforceFX 5700VE" },
  { 0x0345, "Nvidia NV36" },
  { 0x0347, "Nvidia GeforceFX Go5700" },
  { 0x0348, "Nvidia GeforceFX Go5700" },
  { 0x0349, "Nvidia NV36" },
  { 0x034b, "Nvidia NV36" },
  { 0x034c, "Nvidia Quadro FX Go1000" },
  { 0x034e, "Nvidia QuadroFX 1100" },
  { 0x034f, "Nvidia NV36GL" },
  { 0x02a0, "Nvidia Xbox GPU" },
  { 0x0040, "Nvidia Geforce 6800 Ultra" },
  { 0x0041, "Nvidia Geforce 6800" },
  { 0x0042, "Nvidia Geforce 6800LE" },
  { 0x0043, "Nvidia Geforce 6800XE" },
  { 0x0044, "Nvidia Geforce 6800XT" },
  { 0x0045, "Nvidia Geforce 6800GT" },
  { 0x0046, "Nvidia Geforce 6800GT" },
  { 0x0047, "Nvidia Geforce 6800GS" },
  { 0x0048, "Nvidia Geforce 6800XT" },
  { 0x0049, "Nvidia NV40GL" },
  { 0x004d, "Nvidia QuadroFX 4400" },
  { 0x004e, "Nvidia QuadroFX 4000" },
  { 0x00c0, "Nvidia Geforce 6800GS" },
  { 0x00c1, "Nvidia Geforce 6800" },
  { 0x00c2, "Nvidia Geforce 6800LE" },
  { 0x00c3, "Nvidia Geforce 6800XT" },
  { 0x00c8, "Nvidia Geforce Go 6800" },
  { 0x00c9, "Nvidia Geforce Go 6800Ultra" },
  { 0x00cc, "Nvidia QuadroFX Go 1400" },
  { 0x00cd, "Nvidia QuadroFX 3350/4000SDI" },
  { 0x00ce, "Nvidia QuadroFX 1400" },
  { 0x00f0, "Nvidia Geforce 6800/Ultra" },
  { 0x00f1, "Nvidia Geforce 6600/GT" },
  { 0x00f2, "Nvidia Geforce 6600" },
  { 0x00f3, "Nvidia Geforce 6200" },
  { 0x00f4, "Nvidia Geforce 6600LE" },
  { 0x00f5, "Nvidia Geforce 7800GS" },
  { 0x00f6, "Nvidia Geforce 6800GS/XT" },
  { 0x00f8, "Nvidia QuadroFX 3400" },
  { 0x00f9, "Nvidia Geforce 6800 Ultra" },
  { 0x00fa, "Nvidia GeforcePCX 5750" },
  { 0x00fb, "Nvidia GeforcePCX 5900" },
  { 0x00fc, "Nvidia GeforcePCX 5300 / Quadro FX330" },
  { 0x00fd, "Nvidia Quadro NVS280/FX330" },
  { 0x00fe, "Nvidia QuadroFX 1300" },
  { 0x00ff, "Nvidia GeforcePCX 4300" },
  { 0x0140, "Nvidia Geforce 6600GT" },
  { 0x0141, "Nvidia Geforce 6600" },
  { 0x0142, "Nvidia Geforce 6600LE" },
  { 0x0143, "Nvidia NV43" },
  { 0x0144, "Nvidia Geforce Go 6600" },
  { 0x0145, "Nvidia Geforce 6610XL" },
  { 0x0146, "Nvidia GeForce Go 6600TE/6200TE" },
  { 0x0147, "Nvidia Geforce 6700XL" },
  { 0x0148, "Nvidia Geforce Go 6600" },
  { 0x0149, "Nvidia Geforce Go 6600GT" },
  { 0x014a, "Nvidia NV43" },
  { 0x014b, "Nvidia NV43" },
  { 0x014c, "Nvidia QuadroFX 550" },
  { 0x014d, "Nvidia QuadroFX 550" },
  { 0x014e, "Nvidia QuadroFX 540" },
  { 0x014f, "Nvidia Geforce 6200" },
  { 0x0160, "Nvidia NV44" },
  { 0x0161, "Nvidia Geforce 6200 TurboCache" },
  { 0x0162, "Nvidia Geforce 6200SE TurboCache" },
  { 0x0163, "Nvidia NV44" },
  { 0x0164, "Nvidia Geforce Go 6200" },
  { 0x0165, "Nvidia Quadro NVS 285" },
  { 0x0166, "Nvidia Geforce Go 6400" },
  { 0x0167, "Nvidia Geforce Go 6200" },
  { 0x0168, "Nvidia Geforce Go 6400" },
  { 0x0169, "Nvidia Geforce 6250" },
  { 0x016a, "Nvidia Geforce 7100GS" },
  { 0x016b, "Nvidia NV44GLM" },
  { 0x016c, "Nvidia NV44GLM" },
  { 0x016d, "Nvidia NV44GLM" },
  { 0x016e, "Nvidia NV44GLM" },
  { 0x0210, "Nvidia NV48" },
  { 0x0211, "Nvidia Geforce 6800" },
  { 0x0212, "Nvidia Geforce 6800LE" },
  { 0x0215, "Nvidia Geforce 6800GT" },
  { 0x0218, "Nvidia Geforce 6800XT" },
  { 0x021d, "Nvidia NV48" },
  { 0x021e, "Nvidia NV48" },
  { 0x0220, "Nvidia Unknown NV44" },
  { 0x0221, "Nvidia Geforce 6200" },
  { 0x0222, "Nvidia Geforce 6200 A-LE" },
  { 0x0228, "Nvidia NV44M" },
  { 0x0240, "Nvidia Geforce 6150" },
  { 0x0241, "Nvidia Geforce 6150LE" },
  { 0x0242, "Nvidia Geforce 6100" },
  { 0x0243, "Nvidia C51" },
  { 0x0244, "Nvidia Geforce Go 6150" },
  { 0x0245, "Nvidia Quadro NVS 210S / Geforce 6150LE" },
  { 0x0246, "Nvidia C51" },
  { 0x0247, "Nvidia Geforce Go 6100" },
  { 0x0248, "Nvidia C51" },
  { 0x0249, "Nvidia C51" },
  { 0x024a, "Nvidia C51" },
  { 0x024b, "Nvidia C51" },
  { 0x024c, "Nvidia C51" },
  { 0x024d, "Nvidia C51" },
  { 0x024e, "Nvidia C51" },
  { 0x024f, "Nvidia C51" },
  { 0x02dd, "Nvidia Unknown NV4x" },
  { 0x02de, "Nvidia Unknown NV4x" },
  { 0x0090, "Nvidia G70" },
  { 0x0091, "Nvidia Geforce 7800GTX" },
  { 0x0092, "Nvidia Geforce 7800GT" },
  { 0x0093, "Nvidia Geforce 6800GS" },
  { 0x0094, "Nvidia G70" },
  { 0x0095, "Nvidia GeForce 7800 SLI" },
  { 0x0098, "Nvidia G70" },
  { 0x0099, "Nvidia Geforce Go 7800GTX" },
  { 0x009c, "Nvidia G70" },
  { 0x009d, "Nvidia QuadroFX 4500" },
  { 0x009e, "Nvidia G70GL" },
  { 0x0290, "Nvidia Geforce 7900GTX" },
  { 0x0291, "Nvidia Geforce 7900GT" },
  { 0x0292, "Nvidia Geforce 7900GS" },
  { 0x0293, "Nvidia Geforce 7950GX2" },
  { 0x0294, "Nvidia Geforce 7950GX2" },
  { 0x0295, "Nvidia Geforce 7950GT" },
  { 0x0296, "Nvidia G71" },
  { 0x0297, "Nvidia Geforce Go 7950GTX" },
  { 0x0298, "Nvidia Geforce Go 7900GS" },
  { 0x0299, "Nvidia Geforce Go 7900GTX" },
  { 0x029a, "Nvidia QuadroFX 2500M" },
  { 0x029b, "Nvidia QuadroFX 1500M" },
  { 0x029c, "Nvidia QuadroFX 5500" },
  { 0x029d, "Nvidia QuadroFX 3500" },
  { 0x029e, "Nvidia QuadroFX 1500" },
  { 0x029f, "Nvidia QuadroFX 4500 X2" },
  { 0x038b, "Nvidia GeForce 7650 GS" },
  { 0x0390, "Nvidia Geforce 7650GS" },
  { 0x0391, "Nvidia Geforce 7600GT" },
  { 0x0392, "Nvidia Geforce 7600GS" },
  { 0x0393, "Nvidia Geforce 7300GT" },
  { 0x0394, "Nvidia Geforce 7600LE" },
  { 0x0395, "Nvidia Geforce 7300GT" },
  { 0x0397, "Nvidia Geforce Go 7700" },
  { 0x0398, "Nvidia Geforce Go 7600" },
  { 0x0399, "Nvidia Geforce Go 7600GT" },
  { 0x039a, "Nvidia Quadro NVS 300M" },
  { 0x039b, "Nvidia Geforce Go 7900SE" },
  { 0x039c, "Nvidia QuadroFX 550M" },
  { 0x039e, "Nvidia QuadroFX 560" },
  { 0x02e0, "Nvidia Geforce 7600GT" },
  { 0x02e1, "Nvidia Geforce 7600GS" },
  { 0x02e2, "Nvidia Geforce 7300GT" },
  { 0x02e3, "Nvidia GeForce 7900 GS" },
  { 0x02e4, "Nvidia Geforce 7950GT" },
  { 0x01d0, "Nvidia GeForce 7350 LE" },
  { 0x01d1, "Nvidia Geforce 7300LE" },
  { 0x01d2, "Nvidia GeForce 7550 LE" },
  { 0x01d3, "Nvidia Geforce 7300SE" },
  { 0x01d4, "Nvidia GeForce Go 7350" },
  { 0x01d5, "Nvidia Entry Graphics" },
  { 0x01d6, "Nvidia GeForce Go 7200" },
  { 0x01d7, "Nvidia Geforce Go 7300" },
  { 0x01d8, "Nvidia Geforce Go 7400" },
  { 0x01d9, "Nvidia Geforce Go 7400GS" },
  { 0x01da, "Nvidia Quadro NVS 110M" },
  { 0x01db, "Nvidia Quadro NVS 120M" },
  { 0x01dc, "Nvidia QuadroFX 350M" },
  { 0x01dd, "Nvidia Geforce 7500LE" },
  { 0x01de, "Nvidia QuadroFX 350" },
  { 0x01df, "Nvidia Geforce 7300GS" },
  { 0x04c0, "Nvidia G78" },
  { 0x04c1, "Nvidia G78" },
  { 0x04c2, "Nvidia G78" },
  { 0x04c3, "Nvidia G78" },
  { 0x04c4, "Nvidia G78" },
  { 0x04c5, "Nvidia G78" },
  { 0x04c6, "Nvidia G78" },
  { 0x04c7, "Nvidia G78" },
  { 0x04c8, "Nvidia G78" },
  { 0x04c9, "Nvidia G78" },
  { 0x04ca, "Nvidia G78" },
  { 0x04cb, "Nvidia G78" },
  { 0x04cc, "Nvidia G78" },
  { 0x04cd, "Nvidia G78" },
  { 0x04ce, "Nvidia G78" },
  { 0x04cf, "Nvidia G78" },
  { 0x0190, "Nvidia Geforce 8800" },
  { 0x0191, "Nvidia Geforce 8800GTX" },
  { 0x0192, "Nvidia Geforce 8800" },
  { 0x0193, "Nvidia Geforce 8800GTS" },
  { 0x0194, "Nvidia Geforce 8800Ultra" },
  { 0x0197, "Nvidia Geforce 8800" },
  { 0x019a, "Nvidia G80-875" },
  { 0x019d, "Nvidia QuadroFX 5600" },
  { 0x019e, "Nvidia QuadroFX 4600" },
  { 0x0400, "Nvidia Geforce 8600GTS" },
  { 0x0401, "Nvidia Geforce 8600GT" },
  { 0x0402, "Nvidia Geforce 8600GT" },
  { 0x0403, "Nvidia Geforce 8600GS" },
  { 0x0404, "Nvidia Geforce 8400GS" },
  { 0x0405, "Nvidia Geforce 9500M GS" },
  { 0x0406, "Nvidia Geforce NB9P-GE" },
  { 0x0407, "Nvidia Geforce 8600M GT" },
  { 0x0408, "Nvidia Geforce 8600M GTS" },
  { 0x0409, "Nvidia Geforce 8700M GT" },
  { 0x040a, "Nvidia Quadro NVS 370M" },
  { 0x040b, "Nvidia Quadro NVS 320M" },
  { 0x040c, "Nvidia QuadroFX 570M" },
  { 0x040d, "Nvidia QuadroFX 1600M" },
  { 0x040e, "Nvidia QuadroFX 570" },
  { 0x040f, "Nvidia QuadroFX 1700" },
  { 0x0410, "Nvidia GeForce GT 330" },
  { 0x0420, "Nvidia Geforce 8400SE" },
  { 0x0421, "Nvidia Geforce 8500GT" },
  { 0x0422, "Nvidia Geforce 8400GS" },
  { 0x0423, "Nvidia Geforce 8300GS" },
  { 0x0424, "Nvidia Geforce 8400GS" },
  { 0x0425, "Nvidia Geforce 8600M GS" },
  { 0x0426, "Nvidia Geforce 8400M GT" },
  { 0x0427, "Nvidia Geforce 8400M GS" },
  { 0x0428, "Nvidia Geforce 8400M G" },
  { 0x0429, "Nvidia Quadro NVS 140M" },
  { 0x042a, "Nvidia Quadro NVS 130M" },
  { 0x042b, "Nvidia Quadro NVS 135M" },
  { 0x042c, "Nvidia GeForce 9400 GT" },
  { 0x042d, "Nvidia Quadro FX 360M" },
  { 0x042e, "Nvidia Geforce 9300M G" },
  { 0x042f, "Nvidia Quadro NVS 290" },
  { 0x03d0, "Nvidia Geforce 6100 nForce 430"},
  { 0x03d1, "Nvidia Geforce 6100 nForce 405"},
  { 0x03d2, "Nvidia Geforce 6100 nForce 400"},
  { 0x03d3, "Nvidia MCP61" },
  { 0x03d4, "Nvidia MCP61" },
  { 0x03d5, "Nvidia Geforce 6100 nForce 420"},
  { 0x03d6, "Nvidia GeForce 7025 /Nvidia nForce 630a" },
  { 0x03d7, "Nvidia MCP61" },
  { 0x03d8, "Nvidia MCP61" },
  { 0x03d9, "Nvidia MCP61" },
  { 0x03da, "Nvidia MCP61" },
  { 0x03db, "Nvidia MCP61" },
  { 0x03dc, "Nvidia MCP61" },
  { 0x03dd, "Nvidia MCP61" },
  { 0x03de, "Nvidia MCP61" },
  { 0x03df, "Nvidia MCP61" },
  { 0x0530, "Nvidia GeForce 7190M / nForce 650M" },
  { 0x0531, "Nvidia GeForce 7150M / nForce 630M" },
  { 0x0532, "Nvidia MCP67M" },
  { 0x0533, "Nvidia GeForce 7000M / nForce 610M" },
  { 0x053f, "Nvidia MCP67M" },
  { 0x053a, "Nvidia Geforce 7050PV nForce 630a"},
  { 0x053b, "Nvidia Geforce 7050PV nForce 630a"},
  { 0x053e, "Nvidia Geforce 7025 nForce 630a"},
  { 0x05e0, "Nvidia GeForce GT200-400" },
  { 0x05e1, "Nvidia GeForce GTX 280" },
  { 0x05e2, "Nvidia GeForce GTX 260" },
  { 0x05e3, "Nvidia GeForce GTX 285" },
  { 0x05e4, "Nvidia GT200" },
  { 0x05e5, "Nvidia GT200" },
  { 0x05e6, "Nvidia GeForce GTX 275" },
  { 0x05e7, "Nvidia Tesla C1060" },
  { 0x05e8, "Nvidia GT200" },
  { 0x05e9, "Nvidia GT200" },
  { 0x05ea, "Nvidia GeForce GTX 260" },
  { 0x05eb, "Nvidia GeForce GTX 295" },
  { 0x05ec, "Nvidia GT200" },
  { 0x05ed, "Nvidia Quadroplex 2200 D2" },
  { 0x05ee, "Nvidia GT200" },
  { 0x05ef, "Nvidia GT200" },
  { 0x05f0, "Nvidia GT200" },
  { 0x05f1, "Nvidia GT200" },
  { 0x05f2, "Nvidia GT200" },
  { 0x05f3, "Nvidia GT200" },
  { 0x05f4, "Nvidia GT200" },
  { 0x05f5, "Nvidia GT200" },
  { 0x05f6, "Nvidia GT200" },
  { 0x05f7, "Nvidia GT200" },
  { 0x05f8, "Nvidia Quadroplex 2200 S4" },
  { 0x05f9, "Nvidia Quadro CX" },
  { 0x05fa, "Nvidia GT200" },
  { 0x05fb, "Nvidia GT200" },
  { 0x05fc, "Nvidia GT200" },
  { 0x05fd, "Nvidia QuadroFX 5800" },
  { 0x05fe, "Nvidia QuadroFX 5800" },
  { 0x05ff, "Nvidia Quadro FX 3800" },
  { 0x0600, "Nvidia Geforce 8800GTS 512" },
  { 0x0601, "Nvidia GeForce 9800 GT" },
  { 0x0602, "Nvidia Geforce 8800GT" },
  { 0x0603, "Nvidia GeForce GT 230" },
  { 0x0604, "Nvidia Geforce 9800GX2" },
  { 0x0605, "Nvidia GeForce 9800 GT" },
  { 0x0606, "Nvidia Geforce 8800GS" },
  { 0x0607, "Nvidia GeForce GTS 240" },
  { 0x0608, "Nvidia GeForce 9800M GTX" },
  { 0x0609, "Nvidia Geforce 8800M GTS" },
  { 0x060a, "Nvidia GeForce GTX 280M" },
  { 0x060b, "Nvidia GeForce 9800M GT" },
  { 0x060c, "Nvidia Geforce 8800M GTX" },
  { 0x060d, "Nvidia Geforce 8800GS" },
  { 0x060e, "Nvidia GeForce 9850 X" },
  { 0x060f, "Nvidia GeForce GTX 285M" },
  { 0x0610, "Nvidia Geforce 9600GSO" },
  { 0x0611, "Nvidia Geforce 8800GT" },
  { 0x0612, "Nvidia Geforce 9800GTX" },
  { 0x0613, "Nvidia GeForce 9800 GTX+" },
  { 0x0614, "Nvidia Geforce 9800GT" },
  { 0x0615, "Nvidia GeForce GTS 250" },
  { 0x0616, "Nvidia G92" },
  { 0x0617, "Nvidia GeForce 9800M GTX" },
  { 0x0618, "Nvidia GeForce GTX 260M" },
  { 0x0619, "Nvidia Quadro FX 4700 X2" },
  { 0x061a, "Nvidia QuadroFX 3700" },
  { 0x061b, "Nvidia Quadro VX 200" },
  { 0x061c, "Nvidia QuadroFX 3600M" },
  { 0x061d, "Nvidia Quadro FX 2800M" },
  { 0x061e, "Nvidia Quadro FX 3700M" },
  { 0x061f, "Nvidia Quadro FX 3800M" },
  { 0x0620, "Nvidia G94" },
  { 0x0621, "Nvidia GeForce GT 230" },
  { 0x0622, "Nvidia Geforce 9600GT" },
  { 0x0623, "Nvidia Geforce 9600GS" },
  { 0x0624, "Nvidia G94" },
  { 0x0625, "Nvidia GeForce 9600 GSO 512" },
  { 0x0626, "Nvidia GeForce GT 130" },
  { 0x0627, "Nvidia GeForce GT 140" },
  { 0x0628, "Nvidia GeForce 9800M GTS" },
  { 0x0629, "Nvidia G94" },
  { 0x062a, "Nvidia GeForce 9700M GTS" },
  { 0x062b, "Nvidia GeForce 9800M GS" },
  { 0x062c, "Nvidia GeForce 9800M GTS" },
  { 0x062d, "Nvidia GeForce 9600 GT" },
  { 0x062e, "Nvidia GeForce 9600 GT" },
  { 0x062f, "Nvidia GeForce 9800 S" },
  { 0x0630, "Nvidia GeForce 9700 S" },
  { 0x0631, "Nvidia GeForce GTS 160M" },
  { 0x0632, "Nvidia GeForce GTS 150M" },
  { 0x0633, "Nvidia GeForce GT 220" },
  { 0x0634, "Nvidia G94" },
  { 0x0635, "Nvidia GeForce 9600 GSO" },
  { 0x0636, "Nvidia G94" },
  { 0x0637, "Nvidia GeForce 9600 GT" },
  { 0x0638, "Nvidia Quadro FX 1800" },
  { 0x0639, "Nvidia G94" },
  { 0x063a, "Nvidia Quadro FX 2700M" },
  { 0x063b, "Nvidia G94" },
  { 0x063c, "Nvidia G94" },
  { 0x063d, "Nvidia G94" },
  { 0x063e, "Nvidia G94" },
  { 0x063f, "Nvidia G94" },
  { 0x0640, "Nvidia Geforce 9500GT" },
  { 0x0641, "Nvidia GeForce 9400 GT" },
  { 0x0642, "Nvidia GeForce 8400 GS" },
  { 0x0643, "Nvidia Geforce 9500GT" },
  { 0x0644, "Nvidia GeForce 9500 GS" },
  { 0x0645, "Nvidia GeForce 9500 GS" },
  { 0x0646, "Nvidia GeForce GT 120" },
  { 0x0647, "Nvidia Geforce 9600M GT" },
  { 0x0648, "Nvidia Geforce 9600M GS" },
  { 0x0649, "Nvidia Geforce 9600M GT" },
  { 0x064a, "Nvidia GeForce 9700M GT" },
  { 0x064b, "Nvidia Geforce 9500M G" },
  { 0x064c, "Nvidia GeForce 9650M GT" },
  { 0x064d, "Nvidia G96" },
  { 0x064e, "Nvidia G96" },
  { 0x064f, "Nvidia GeForce 9600 S" },
  { 0x0650, "Nvidia G96-825" },
  { 0x0651, "Nvidia GeForce G 110M" },
  { 0x0652, "Nvidia GeForce GT 130M" },
  { 0x0653, "Nvidia GeForce GT 120M" },
  { 0x0654, "Nvidia GeForce GT 220M" },
  { 0x0655, "Nvidia GeForce 9500 GS / GeForce 9600 S" },
  { 0x0656, "Nvidia GeForce 9500 GT / GeForce 9650 S" },
  { 0x0657, "Nvidia G96" },
  { 0x0658, "Nvidia Quadro FX 380" },
  { 0x0659, "Nvidia Quadro FX 580" },
  { 0x065a, "Nvidia Quadro FX 1700M" },
  { 0x065b, "Nvidia GeForce 9400 GT" },
  { 0x065c, "Nvidia Quadro FX 770M" },
  { 0x065d, "Nvidia G96" },
  { 0x065e, "Nvidia G96" },
  { 0x065f, "Nvidia GeForce G210" },
  { 0x06e0, "Nvidia Geforce 9300GE" },
  { 0x06e1, "Nvidia Geforce 9300GS" },
  { 0x06e2, "Nvidia Geforce 8400" },
  { 0x06e3, "Nvidia Geforce 8400SE" },
  { 0x06e4, "Nvidia Geforce 8400GS" },
  { 0x06e5, "Nvidia Geforce 9300M GS" },
  { 0x06e6, "Nvidia Geforce G100" },
  { 0x06e7, "Nvidia G98" },
  { 0x06e8, "Nvidia Geforce 9200M GS" },
  { 0x06e9, "Nvidia Geforce 9300M GS" },
  { 0x06ea, "Nvidia Quadro NVS 150M" },
  { 0x06eb, "Nvidia Quadro NVS 160M" },
  { 0x06ec, "Nvidia Geforce G105M" },
  { 0x06ed, "Nvidia G98" },
  { 0x06ee, "Nvidia G98" },
  { 0x06ef, "Nvidia G98" },
  { 0x06f0, "Nvidia G98" },
  { 0x06f1, "Nvidia G98" },
  { 0x06f2, "Nvidia G98" },
  { 0x06f3, "Nvidia G98" },
  { 0x06f4, "Nvidia G98" },
  { 0x06f5, "Nvidia G98" },
  { 0x06f6, "Nvidia G98" },
  { 0x06f7, "Nvidia G98" },
  { 0x06f8, "Nvidia Quadro NVS 420" },
  { 0x06f9, "Nvidia QuadroFX 370 LP" },
  { 0x06fa, "Nvidia Quadro NVS 450" },
  { 0x06fb, "Nvidia QuadroFX 370M" },
  { 0x06fc, "Nvidia G98" },
  { 0x06fd, "Nvidia Quadro NVS 295" },
  { 0x06fe, "Nvidia G98" },
  { 0x06ff, "Nvidia G98-GL" },
  { 0x07e0, "Nvidia GeForce 7150 / NVIDIA nForce 630i" },
  { 0x07e1, "Nvidia GeForce 7100 / NVIDIA nForce 630i" },
  { 0x07e2, "Nvidia GeForce 7050 / NVIDIA nForce 630i" },
  { 0x07e3, "Nvidia GeForce 7050 / NVIDIA nForce 610i" },
  { 0x07e4, "Nvidia MCP73" },
  { 0x07e5, "Nvidia GeForce 7050 / NVIDIA nForce 620i" },
  { 0x07e6, "Nvidia MCP73" },
  { 0x07e7, "Nvidia MCP73" },
  { 0x07e8, "Nvidia MCP73" },
  { 0x07e9, "Nvidia MCP73" },
  { 0x07ea, "Nvidia MCP73" },
  { 0x07eb, "Nvidia MCP73" },
  { 0x07ec, "Nvidia MCP73" },
  { 0x07ed, "Nvidia MCP73" },
  { 0x07ee, "Nvidia MCP73" },
  { 0x07ef, "Nvidia MCP73" },
  { 0x0840, "Nvidia GeForce 8200M" },
  { 0x0842, "Nvidia MCP77/78" },
  { 0x0844, "Nvidia GeForce 9100M G" },
  { 0x0845, "Nvidia GeForce 8200M G / GeForce 8200M" },
  { 0x0846, "Nvidia GeForce 9200" },
  { 0x0847, "Nvidia GeForce 9100" },
  { 0x0848, "Nvidia GeForce 8300" },
  { 0x0849, "Nvidia GeForce 8200" },
  { 0x084a, "Nvidia nForce 730a" },
  { 0x084b, "Nvidia GeForce 8200 / GeForce 9200" },
  { 0x084c, "Nvidia nForce 980a/780a SLI" },
  { 0x084d, "Nvidia nForce 750a SLI" },
  { 0x084f, "Nvidia GeForce 8100 / nForce 720a" },
  { 0x0850, "Nvidia MCP77/78" },
  { 0x0851, "Nvidia MCP77/78" },
  { 0x0852, "Nvidia MCP77/78" },
  { 0x0853, "Nvidia MCP77/78" },
  { 0x0854, "Nvidia MCP77/78" },
  { 0x0855, "Nvidia MCP77/78" },
  { 0x0856, "Nvidia MCP77/78" },
  { 0x0857, "Nvidia MCP77/78" },
  { 0x0858, "Nvidia MCP77/78" },
  { 0x0859, "Nvidia MCP77/78" },
  { 0x085a, "Nvidia MCP77/78" },
  { 0x085b, "Nvidia MCP77/78" },
  { 0x085c, "Nvidia MCP77/78" },
  { 0x085d, "Nvidia MCP77/78" },
  { 0x085e, "Nvidia MCP77/78" },
  { 0x085f, "Nvidia MCP77/78" },
  { 0x0860, "Nvidia Geforce 9300" },
  { 0x0861, "Nvidia Geforce 9400" },
  { 0x0862, "Nvidia GeForce 9400M G" },
  { 0x0863, "Nvidia Geforce 9400M" },
  { 0x0864, "Nvidia Geforce 9300" },
  { 0x0865, "Nvidia Geforce 9300" },
  { 0x0866, "Nvidia GeForce 9400M G" },
  { 0x0867, "Nvidia GeForce 9400" },
  { 0x0868, "Nvidia nForce 760i SLI" },
  { 0x0869, "Nvidia GeForce 9400" },
  { 0x086a, "Nvidia GeForce 9400" },
  { 0x086b, "Nvidia MCP7A-U" },
  { 0x086c, "Nvidia GeForce 9300 / nForce 730i" },
  { 0x086d, "Nvidia GeForce 9200" },
  { 0x086e, "Nvidia GeForce 9100M G" },
  { 0x086f, "Nvidia GeForce 8200M G" },
  { 0x0870, "Nvidia GeForce 9400M" },
  { 0x0871, "Nvidia GeForce 9200" },
  { 0x0872, "Nvidia GeForce G102M" },
  { 0x0873, "Nvidia GeForce G102M" },
  { 0x0874, "Nvidia ION" },
  { 0x0876, "Nvidia ION" },
  { 0x087a, "Nvidia Quadro FX 470 / GeForce 9400" },
  { 0x087b, "Nvidia MCP7A-GL" },
  { 0x087d, "Nvidia ION" },
  { 0x087e, "Nvidia ION LE" },
  { 0x087f, "Nvidia ION LE" },

  { 0x0a00, "Nvidia GT212" },
  { 0x0a10, "Nvidia GT212" },
  { 0x0a20, "Nvidia GeForce GT 220" },
  { 0x0a21, "Nvidia D10M2-20" },
  { 0x0a22, "Nvidia GeForce 315" },
  { 0x0a23, "Nvidia GeForce 210" },
  { 0x0a28, "Nvidia GeForce GT 230M" },
  { 0x0a29, "Nvidia GeForce GT 330M" },
  { 0x0a2a, "Nvidia GeForce GT 230M" },
  { 0x0a2b, "Nvidia GeForce GT 330M" },
  { 0x0a2c, "Nvidia NVS 5100M" },
  { 0x0a2d, "Nvidia GeForce GT 320M" },
  { 0x0a30, "Nvidia GT216" },
  { 0x0a34, "Nvidia GeForce GT 240M" },
  { 0x0a35, "Nvidia GeForce GT 325M" },
  { 0x0a3c, "Nvidia Quadro FX 880M" },
  { 0x0a3d, "Nvidia N10P-ES" },
  { 0x0a3f, "Nvidia GT216-INT" },
  { 0x0a60, "Nvidia GeForce G210" },
  { 0x0a61, "Nvidia NVS 2100" },
  { 0x0a62, "Nvidia GeForce 205" },
  { 0x0a63, "Nvidia GeForce 310 / NVIDIA NVS 3100" },
  { 0x0a63, "Nvidia GeForce 310" },
  { 0x0a64, "Nvidia ION" },
  { 0x0a65, "Nvidia GeForce 210" },
  { 0x0a66, "Nvidia GeForce 310" },
  { 0x0a67, "Nvidia GeForce 315" },
  { 0x0a68, "Nvidia GeForce G105M" },
  { 0x0a69, "Nvidia GeForce G105M" },
  { 0x0a6a, "Nvidia NVS 2100M" },
  { 0x0a6c, "Nvidia NVS 3100M" },
  { 0x0a6e, "Nvidia GeForce 305M" },
  { 0x0a6f, "Nvidia ION" },
  { 0x0a70, "Nvidia GeForce 310M" },
  { 0x0a72, "Nvidia GeForce 310M" },
  { 0x0a73, "Nvidia GeForce 305M" },
  { 0x0a74, "Nvidia GeForce G210M" },
  { 0x0a75, "Nvidia GeForce 310M" },
  { 0x0a76, "Nvidia ION" },
  { 0x0a78, "Nvidia Quadro FX 380 LP" },
  { 0x0a7c, "Nvidia Quadro FX 380M" },
  { 0x0a7d, "Nvidia GT218-ES" },
  { 0x0a7e, "Nvidia GT218-INT-S" },
  { 0x0a7f, "Nvidia GT218-INT-B" },

  { 0x06c0, "Nvidia GeForce GTX 480" },
  { 0x06c4, "Nvidia GeForce GTX 465" },
  { 0x06cd, "Nvidia GeForce GTX 470" },
  { 0, NULL }
};

void get_card_name(int device_id, char *str)
{
  struct pci_ids *nv_ids = (struct pci_ids*)ids;

  while(nv_ids->id != 0)
  {
    if(nv_ids->id == device_id)
    {
      strcpy(str, nv_ids->name);
      return;
    }

    nv_ids++;
  }

  /* if !found */
  strcpy(str, "Unknown Nvidia card");
}

/* Internal gpu architecture function which sets
/  a device to a specific architecture. This architecture
/  doesn't have to be the real architecture. It is mainly
/  used to choose codepaths inside nvclock.
*/
int get_gpu_arch(short device_id)
{
  int arch;
  switch(device_id & 0xff0)
  {
    case 0x0020:
      arch = NV5;
      break;
    case 0x0100:
    case 0x0110:
    case 0x0150:
    case 0x01a0:
      arch = NV10;
      break;
    case 0x0170:
    case 0x0180:
    case 0x01f0:
      arch = NV17;
      break;
    case 0x0200:
      arch = NV20;
      break;
    case 0x0250:
    case 0x0280:
      arch = NV25;
      break;
    case 0x0300:
    case 0x0320:  // MOVED: The FX5200/FX5500 and FX5100 FX cards are really NV34
      arch = NV30;
      break;
    case 0x0330:
      arch = NV35; /* Similar to NV30 but fanspeed stuff works differently */
      break;
    /* Give a seperate arch to FX5600/FX5700 cards as they need different code than other FX cards */
    case 0x0310:
    case 0x0340:
      arch = NV31;
      break;
    case 0x0040:
    case 0x0120:
    case 0x0130:
    case 0x0210:
    case 0x0230:
      arch = NV40;
      break;
    case 0x00c0:
      arch = NV41;
      break;
    case 0x0140:
      arch = NV43; /* Similar to NV40 but with different fanspeed code */
      break;
    case 0x0160:
    case 0x0220:
      arch = NV44;
      break;
    case 0x01d0:
      arch = NV46;
      break;
    case 0x0090:
      arch = NV47;
      break;
    case 0x0290:
      arch = NV49; /* 7900 */
      break;
    case 0x0380: // Added: 7650 GS G73
    case 0x0390:
      arch = NV4B; /* 7600 */
      break;
    case 0x0190:
      arch = NV50; /* 8800 'NV50 / G80' */
      break;
    case 0x0400: /* 8600 'G84' */
      arch = G84;
      break;
    case 0x05e0: /* GT2x0 */
    case 0x05f0: /* GT2x0 */
    case 0x06a0: // Added
    case 0x06b0: // Added
    case 0x0a00: // ? Added
    case 0x0a10: // ? Added
    case 0x0a20: // ? Added
    case 0x0a30: // ? Added
    case 0x0a60: // ? Added
    case 0x0a70: // ? Added
      arch = GT200;
      break;
    case 0x06e0: /* G98 */
    case 0x06f0: /* G98 */
    case 0x0840: // Added C77
    case 0x0850: // ? Added: MCP77/78 C77
    case 0x0860: /* C79 */
    case 0x0870: // Added C79
      arch = G86;
      break;
    case 0x0600: /* G92 */
    case 0x0610: /* G92 */
    case 0x0410: // Added: GT 330 rebranded two years later haha
      arch = G92;
      break;
    case 0x0620: /* 9600GT 'G94' */
    case 0x0630: // Added
      arch = G94;
      break;
    case 0x0640: /* 9500GT */
    case 0x0650: // Added
      arch = G96;
      break;
    case 0x0240:
    case 0x03d0: // SAME: both C51 and C61
    case 0x0530: /* not sure if the 70xx is C51 too */ // 7150M, 7190M, and MCP67M are C67
    case 0x07e0: // ? Added: C73
      arch = C51;
      break;
    case 0x04c0:
      arch = G78; //die shrink 7300
      break;
    case 0x0420:
      switch(device_id & 0xf)
      {
        case 0xc: // 9400 GT
          arch = G96;
          break;
        default: /* 8500 'G86' */
          arch = G86;
          break;
      }
      break;
    case 0x06c0:
      arch = GF100;
      break;
    case 0x02e0:
    case 0x00f0:
      /* The code above doesn't work for pci-express cards as multiple architectures share one id-range */
      switch(device_id)
      {
        case 0x00f0: /* 6800 */
        case 0x00f9: /* 6800Ultra */
          arch = NV40;
          break;
        case 0x00f6: /* 6800GS/XT */
          arch = NV41;
          break;
        case 0x00f1: /* 6600/6600GT */
        case 0x00f2: /* 6600GT */
        case 0x00f3: /* 6200 */
        case 0x00f4: /* 6600LE */
          arch = NV43;
          break;
        case 0x00f5: /* 7800GS */
          arch = NV47;
          break;
        case 0x00fa: /* PCX5700 */
          arch = NV31;
          break;
        case 0x00f8: /* QuadroFX 3400 */
        case 0x00fb: /* PCX5900 */
          arch = NV35;
          break;
        case 0x00fc: /* PCX5300 */
        case 0x00fd: /* Quadro NVS280/FX330, FX5200 based? */
        case 0x00ff: /* PCX4300 */
          arch = NV25;
          break;
        case 0x00fe: /* Quadro 1300, has the same id as a FX3000 */
          arch = NV35;
          break;
        case 0x02e0: /* Geforce 7600GT AGP (at least Leadtek uses this id) */
        case 0x02e1: /* Geforce 7600GS AGP (at least BFG uses this id) */
        case 0x02e2: /* Geforce 7300GT AGP (at least a Galaxy 7300GT uses this id) */
          arch = NV4B;
          break;
        case 0x02e3: // ADDED: Geforce 7900 GS
        case 0x02e4: /* Geforce 7950 GT AGP */
          arch = NV49;
          break;
      }
      break;
    default:
      arch = UNKNOWN;
  }
  return arch;
}

/* Receive the real gpu architecture */
short get_gpu_architecture()
{
  return (nv_card->PMC[NV_PMC_BOOT_0/4] >> 20) & 0xff;
}

/* Receive the gpu revision */
short get_gpu_revision()
{
  return nv_card->PMC[NV_PMC_BOOT_0/4] & NV_PMC_BOOT_0_REVISION_MASK;
}

void get_subvendor_name(short vendor_id, char *str)
{
  switch(vendor_id)
  {
    case 0x147B:
      strcpy(str, "Abit");
      break;
    case 0x1025:
      strcpy(str, "Acer");
      break;
    case 0x14C0:
      strcpy(str, "Ahtec");
      break;
    case 0x161F:
      strcpy(str, "Alienware");
      break;
    case 0x106B:
      strcpy(str, "Apple");
      break;
    case 0x1043:
      strcpy(str, "Asus");
      break;
    case 0x19F1:
      strcpy(str, "Bfg");
      break;
    case 0x270F:
      strcpy(str, "Chaintech");
      break;
    case 0x196D:
      strcpy(str, "Club3D");
      break;
    case 0x7377:
      strcpy(str, "Colorful");
      break;
    case 0x1102:
      strcpy(str, "Creative");
      break;
    case 0x1028:
      strcpy(str, "Dell");
      break;
    case 0x1019:
      strcpy(str, "Ecs");
      break;
    case 0x1048:
      strcpy(str, "Elsa");
      break;
    case 0x3842:
      strcpy(str, "Evga");
      break;
    case 0x105B:
      strcpy(str, "Foxconn");
      break;
    case 0x1509:
      strcpy(str, "Fujitsu");
      break;
    case 0x10B0:
      strcpy(str, "Gainward");
      break;
    case 0x1631:
      strcpy(str, "Gateway");
      break;
    case 0x1458:
      strcpy(str, "Gigabyte");
      break;
    case 0x103C:
      strcpy(str, "Hp");
      break;
    case 0x107D:
      strcpy(str, "Leadtek");
      break;
    case 0x1462:
      strcpy(str, "Msi");
      break;
    case 0x10DE:
    case 0x1558: // ???
      strcpy(str, "Nvidia");
      break;
    case 0x196E:
      strcpy(str, "Pny");
      break;
    case 0x1ACC:
      strcpy(str, "Point of View");
      break;
    case 0x1554:
      strcpy(str, "Prolink");
      break;
    case 0x144d:
      strcpy(str, "Sanyo/Samsung");
      break;
    case 0x104d:
      strcpy(str, "Sony");
      break;
    case 0x1179:
      strcpy(str, "Toshiba");
      break;
    case 0x1682:
      strcpy(str, "Xfx");
      break;
    case 0x1a46:
      strcpy(str, "Zepto");
      break;
    case 0x174B:
    case 0x174D:
    case 0x19DA:
      strcpy(str, "Zotac");
      break;
    case 0x0000:
      strcpy(str, "None");
      break;
    default:
      strcpy(str, "Unknown");
  }
}
