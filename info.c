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

/* This list isn't used much for the speed ranges anymore */
/* Mainly mobile gpu speeds are missing */
static const struct pci_ids ids[] =
{
  { 0x0020, "nVidia Riva TnT" },
  { 0x0028, "nVidia Riva TnT 2 Pro" },
  { 0x002a, "nVidia Riva TnT 2" },
  { 0x002b, "nVidia Riva TnT 2" },
  { 0x00a0, "nVidia Riva TnT 2 Aladdin (Integrated)" },
  { 0x002c, "nVidia Riva TnT 2 VANTA" },
  { 0x002d, "nVidia Riva TnT 2 M64" },
  { 0x002e, "nVidia Riva TnT 2 VANTA" },
  { 0x002f, "nVidia Riva TnT 2 VANTA" },
  { 0x0029, "nVidia Riva TnT 2 Ultra" },
  { 0x0100, "nVidia Geforce 256 SDR" },
  { 0x0101, "nVidia Geforce 256 DDR" },
  { 0x0103, "nVidia Quadro" },
  { 0x0110, "nVidia Geforce 2 MX/MX400" },
  { 0x0111, "nVidia Geforce 2 MX 100/200" },
  { 0x0112, "nVidia Geforce 2 GO" },
  { 0x0113, "nVidia Quadro 2 MXR/EX/GO" },
  { 0x01a0, "nVidia Geforce 2 MX integrated" },
  { 0x0150, "nVidia Geforce 2 GTS/PRO" },
  { 0x0151, "nVidia Geforce 2 Ti" },
  { 0x0152, "nVidia Geforce 2 Ultra" },
  { 0x0153, "nVidia Quadro 2 Pro" },
  { 0x0170, "nVidia Geforce 4 MX460" },
  { 0x0171, "nVidia Geforce 4 MX440" },
  { 0x0172, "nVidia Geforce 4 MX420" },
  { 0x0173, "nVidia Geforce 4 MX440 SE" },
  { 0x0174, "nVidia Geforce 4 440 Go" },
  { 0x0175, "nVidia Geforce 4 420 Go" },
  { 0x0176, "nVidia Geforce 4 420 Go 32M" },
  { 0x0177, "nVidia Geforce 4 460 Go" },
  { 0x0178, "nVidia Quadro 4 550 XGL" },
  { 0x0179, "nVidia Geforce 4 440 Go 64M" },
  { 0x017a, "nVidia Quadro 4 200/400 NVS" },
  { 0x017b, "nVidia Quadro 4 550 XGL" },
  { 0x017c, "nVidia Quadro 4 500 GoGL" },
  { 0x017d, "nVidia Geforce 410 Go" },
  { 0x0180, "nVidia Geforce 4 MX440 8X" },
  { 0x0181, "nVidia Geforce 4 MX440 8X" },
  { 0x0182, "nVidia Geforce 4 MX440SE 8X" },
  { 0x0185, "nVidia Geforce 4 MX4000" },
  { 0x0183, "nVidia Geforce 4 MX420 8X" },
  { 0x0186, "nVidia Geforce 4 Go" },
  { 0x0187, "nVidia Geforce 4 488 Go" },
  { 0x0188, "nVidia Quadro 4 580 XGL" },
  { 0x0189, "nVidia GeForce4 MX with AGP8X (MAC)"},
  { 0x018a, "nVidia Quadro 4 280 NVS" },
  { 0x018b, "nVidia Quadro 4 380 XGL" },
  { 0x018c, "nVidia Quadro NVS 50 PCI" },
  { 0x018d, "nVidia Geforce 4 448 Go" },
  { 0x01f0, "nVidia Geforce 4 MX integrated" },
  { 0x0200, "nVidia Geforce 3" },
  { 0x0201, "nVidia Geforce 3 Titanium 200" },
  { 0x0202, "nVidia Geforce 3 Titanium 500" },
  { 0x0203, "nVidia Quadro DCC" },
  { 0x0250, "nVidia Geforce 4 Ti 4600" },
  { 0x0251, "nVidia Geforce 4 Ti 4400" },
  { 0x0252, "nVidia GeForce4 Ti" },
  { 0x0253, "nVidia Geforce 4 Ti 4200" },
  { 0x0258, "nVidia Quadro 4 900 XGL" },
  { 0x0259, "nVidia Quadro 4 750 XGL" },
  { 0x025a, "nVidia Quadro 4 600 XGL" },
  { 0x025b, "nVidia Quadro 4 700 XGL" },
  { 0x0280, "nVidia Geforce 4 Ti 4800" },
  { 0x0281, "nVidia Geforce 4 Ti 4200 8X" },
  { 0x0282, "nVidia Geforce 4 Ti 4800SE" },
  { 0x0286, "nVidia Geforce 4 4000 GO" },
  { 0x0288, "nVidia Quadro 4 980 XGL" },
  { 0x0289, "nVidia Quadro 4 780 XGL" },
  { 0x028c, "nVidia Quadro 4 700 GoGL" },
  { 0x0300, "nVidia GeforceFX 5800" },
  { 0x0301, "nVidia GeforceFX 5800 Ultra" },
  { 0x0302, "nVidia GeforceFX 5800" },
  { 0x0308, "nVidia QuadroFX 2000" },
  { 0x0309, "nVidia QuadroFX 1000" },
  { 0x030a, " NVIDIA ICE FX 2000" },
  { 0x0311, "nVidia GeforceFX 5600 Ultra" },
  { 0x0312, "nVidia GeforceFX 5600" },
  { 0x0313, " NVIDIA NV31" },
  { 0x0314, "nVidia GeforceFX 5600XT" },
  { 0x0316, "nVidia NV31" },
  { 0x0317, "nVidia NV31" },
  { 0x0318, "nVidia NV31GL" },
  { 0x0319, "nVidia NV31GL" },
  { 0x031a, "nVidia GeforceFX Go 5600" },
  { 0x031b, "nVidia GeforceFX Go 5650" },
  { 0x031c, "nVidia QuadroFX Go700" },
  { 0x031d, "NV31" },
  { 0x031e, "NV31GL" },
  { 0x031f, "NV31GL" },
  { 0x0320, " NVIDIA GeForce FX 5200" },
  { 0x0321, "nVidia GeforceFX 5200 Ultra" },
  { 0x0322, "nVidia GeforceFX 5200" },
  { 0x0323, "nVidia GeforceFX 5200LE" },
  { 0x0324, "nVidia GeforceFX Go 5200" },
  { 0x0325, "nVidia GeforceFX Go 5250" },
  { 0x0326, "nVidia GeforceFX 5500" },
  { 0x0327, " NVIDIA GeForce FX 5100" },
  { 0x0328, "nVidia GeForceFX Go5200 32M/64M" },
  { 0x0329, "nVidia GeForce FX 5200 (Mac)" },
  { 0x032a, "nVidia Quadro NVS 280 PCI" },
  { 0x032b, "nVidia QuadroFX 500" },
  { 0x032c, "nVidia GeforceFX Go5300" },
  { 0x032d, "nVidia GeforceFX Go5100" },
  { 0x032f, "nVidia NV34GL" },
  { 0x0330, "nVidia GeforceFX 5900 Ultra" },
  { 0x0331, "nVidia GeforceFX 5900" },
  { 0x0332, "nVidia GeforceFX 5900XT" },
  { 0x0333, "nVidia GeforceFX 5950 Ultra" },
  { 0x0334, "nVidia GeforceFX 5900ZT" },
  { 0x0338, "nVidia QuadroFX 3000" },
  { 0x033f, "nVidia GeforceFX 700" },
  { 0x0341, "nVidia GeforceFX 5700 Ultra" },
  { 0x0342, "nVidia GeforceFX 5700" },
  { 0x0343, "nVidia GeforceFX 5700LE" },
  { 0x0344, "nVidia GeforceFX 5700VE" },
  { 0x0345, "NV36" },
  { 0x0347, "nVidia GeforceFX Go5700" },
  { 0x0348, "nVidia GeforceFX Go5700" },
  { 0x0349, "NV36" },
  { 0x034b, "NV36" },
  { 0x034c, "nVidia Quadro FX Go1000" },
  { 0x034e, "nVidia QuadroFX 1100" },
  { 0x034f, "NV36GL" },
  { 0x02a0, "nVidia Xbox GPU" },
  { 0x0040, "nVidia Geforce 6800 Ultra" },
  { 0x0041, "nVidia Geforce 6800" },
  { 0x0042, "nVidia Geforce 6800LE" },
  { 0x0043, "nVidia Geforce 6800XE" },
  { 0x0044, "nVidia Geforce 6800XT" },
  { 0x0045, "nVidia Geforce 6800GT" },
  { 0x0046, "nVidia Geforce 6800GT" },
  { 0x0047, "nVidia Geforce 6800GS" },
  { 0x0048, "nVidia Geforce 6800XT" },
  { 0x0049, "NV40GL" },
  { 0x004d, "nVidia QuadroFX 4400" },
  { 0x004e, "nVidia QuadroFX 4000" },
  { 0x00c0, "nVidia Geforce 6800GS" },
  { 0x00c1, "nVidia Geforce 6800" },
  { 0x00c2, "nVidia Geforce 6800LE" },
  { 0x00c3, "nVidia Geforce 6800XT" },
  { 0x00c8, "nVidia Geforce Go 6800" },
  { 0x00c9, "nVidia Geforce Go 6800Ultra" },
  { 0x00cc, "nVidia QuadroFX Go 1400" },
  { 0x00cd, "nVidia QuadroFX 3350/4000SDI" },
  { 0x00ce, "nVidia QuadroFX 1400" },
  { 0x00f0, "nVidia Geforce 6800/Ultra" },
  { 0x00f1, "nVidia Geforce 6600/GT" },
  { 0x00f2, "nVidia Geforce 6600" },
  { 0x00f3, "nVidia Geforce 6200" },
  { 0x00f4, "nVidia Geforce 6600LE" },
  { 0x00f5, "nVidia Geforce 7800GS" },
  { 0x00f6, "nVidia Geforce 6800GS/XT" },
  { 0x00f8, "nVidia QuadroFX 3400" },
  { 0x00f9, "nVidia Geforce 6800 Ultra" },
  { 0x00fa, "nVidia GeforcePCX 5750" },
  { 0x00fb, "nVidia GeforcePCX 5900" },
  { 0x00fc, "nVidia GeforcePCX 5300 / Quadro FX330" },
  { 0x00fd, "nVidia Quadro NVS280/FX330" },
  { 0x00fe, "nVidia QuadroFX 1300" },
  { 0x00ff, "nVidia GeforcePCX 4300" },
  { 0x0140, "nVidia Geforce 6600GT" },
  { 0x0141, "nVidia Geforce 6600" },
  { 0x0142, "nVidia Geforce 6600LE" },
  { 0x0143, "NV43" },
  { 0x0144, "nVidia Geforce Go 6600" },
  { 0x0145, "nVidia Geforce 6610XL" },
  { 0x0146, "nVidia GeForce Go 6600TE/6200TE" },
  { 0x0147, "nVidia Geforce 6700XL" },
  { 0x0148, "nVidia Geforce Go 6600" },
  { 0x0149, "nVidia Geforce Go 6600GT" },
  { 0x014a, "NV43" },
  { 0x014b, "NV43" },
  { 0x014c, "nVidia QuadroFX 550" },
  { 0x014d, "nVidia QuadroFX 550" },
  { 0x014e, "nVidia QuadroFX 540" },
  { 0x014f, "nVidia Geforce 6200" },
  { 0x0160, "nVidia NV44" },
  { 0x0161, "nVidia Geforce 6200 TurboCache" },
  { 0x0162, "nVidia Geforce 6200SE TurboCache" },
  { 0x0163, "NV44" },
  { 0x0164, "nVidia Geforce Go 6200" },
  { 0x0165, "nVidia Quadro NVS 285" },
  { 0x0166, "nVidia Geforce Go 6400" },
  { 0x0167, "nVidia Geforce Go 6200" },
  { 0x0168, "nVidia Geforce Go 6400" },
  { 0x0169, "nVidia Geforce 6250" },
  { 0x016a, "nVidia Geforce 7100GS" },
  { 0x016b, "NV44GLM" },
  { 0x016c, "NV44GLM" },
  { 0x016d, "NV44GLM" },
  { 0x016e, "NV44GLM" },
  { 0x0210, "NV48" },
  { 0x0211, "nVidia Geforce 6800" },
  { 0x0212, "nVidia Geforce 6800LE" },
  { 0x0215, "nVidia Geforce 6800GT" },
  { 0x0218, "nVidia Geforce 6800XT" },
  { 0x021d, "nVidia NV48" },
  { 0x021e, "nVidia NV48" },
  { 0x0220, "Unknown NV44" },
  { 0x0221, "nVidia Geforce 6200" },
  { 0x0222, "nVidia Geforce 6200 A-LE" },
  { 0x0228, "NV44M" },
  { 0x0240, "nVidia Geforce 6150" },
  { 0x0241, "nVidia Geforce 6150LE" },
  { 0x0242, "nVidia Geforce 6100" },
  { 0x0243, "nVidia C51" },
  { 0x0244, "nVidia Geforce Go 6150" },
  { 0x0245, "nVidia Quadro NVS 210S / Geforce 6150LE" },
  { 0x0246, "nVidia C51" },
  { 0x0247, "nVidia Geforce Go 6100" },
  { 0x0248, "nVidia C51" },
  { 0x0249, "nVidia C51" },
  { 0x024a, "nVidia C51" },
  { 0x024b, "nVidia C51" },
  { 0x024c, "nVidia C51" },
  { 0x024d, "nVidia C51" },
  { 0x024e, "nVidia C51" },
  { 0x024f, "nVidia C51" },
  { 0x02dd, "nVidia Unknown NV4x" },
  { 0x02de, "nVidia Unknown NV4x" },
  { 0x0090, "nVidia G70" },
  { 0x0091, "nVidia Geforce 7800GTX" },
  { 0x0092, "nVidia Geforce 7800GT" },
  { 0x0093, "nVidia Geforce 6800GS" },
  { 0x0094, "nVidia G70" },
  { 0x0095, "nVidia GeForce 7800 SLI" },
  { 0x0098, "nVidia G70" },
  { 0x0099, "nVidia Geforce Go 7800GTX" },
  { 0x009c, "nVidia G70" },
  { 0x009d, "nVidia QuadroFX 4500" },
  { 0x009e, "nVidia G70GL" },
  { 0x0290, "nVidia Geforce 7900GTX" },
  { 0x0291, "nVidia Geforce 7900GT" },
  { 0x0292, "nVidia Geforce 7900GS" },
  { 0x0293, "nVidia Geforce 7950GX2" },
  { 0x0294, "nVidia Geforce 7950GX2" },
  { 0x0295, "nVidia Geforce 7950GT" },
  { 0x0296, "nVidia G71" },
  { 0x0297, "nVidia Geforce Go 7950GTX" },
  { 0x0298, "nVidia Geforce Go 7900GS" },
  { 0x0299, "nVidia Geforce Go 7900GTX" },
  { 0x029a, "nVidia QuadroFX 2500M" },
  { 0x029b, "nVidia QuadroFX 1500M" },
  { 0x029c, "nVidia QuadroFX 5500" },
  { 0x029d, "nVidia QuadroFX 3500" },
  { 0x029e, "nVidia QuadroFX 1500" },
  { 0x029f, "nVidia QuadroFX 4500 X2" },
  { 0x038b, " NVIDIA GeForce 7650 GS" },
  { 0x0390, "nVidia Geforce 7650GS" },
  { 0x0391, "nVidia Geforce 7600GT" },
  { 0x0392, "nVidia Geforce 7600GS" },
  { 0x0393, "nVidia Geforce 7300GT" },
  { 0x0394, "nVidia Geforce 7600LE" },
  { 0x0395, "nVidia Geforce 7300GT" },
  { 0x0397, "nVidia Geforce Go 7700" },
  { 0x0398, "nVidia Geforce Go 7600" },
  { 0x0399, "nVidia Geforce Go 7600GT" },
  { 0x039a, "nVidia Quadro NVS 300M" },
  { 0x039b, "nVidia Geforce Go 7900SE" },
  { 0x039c, "nVidia QuadroFX 550M" },
  { 0x039e, "nVidia QuadroFX 560" },
  { 0x02e0, "nVidia Geforce 7600GT" },
  { 0x02e1, "nVidia Geforce 7600GS" },
  { 0x02e2, "nVidia Geforce 7300GT" },
  { 0x02e3, "nVidia GeForce 7900 GS" },
  { 0x02e4, "nVidia Geforce 7950GT" },
  { 0x01d0, "nVidia GeForce 7350 LE" },
  { 0x01d1, "nVidia Geforce 7300LE" },
  { 0x01d2, "nVidia GeForce 7550 LE" },
  { 0x01d3, "nVidia Geforce 7300SE" },
  { 0x01d4, "nVidia GeForce Go 7350" },
  { 0x01d5, "nVidia Entry Graphics" },
  { 0x01d6, "nVidia GeForce Go 7200" },
  { 0x01d7, "nVidia Geforce Go 7300" },
  { 0x01d8, "nVidia Geforce Go 7400" },
  { 0x01d9, "nVidia Geforce Go 7400GS" },
  { 0x01da, "nVidia Quadro NVS 110M" },
  { 0x01db, "nVidia Quadro NVS 120M" },
  { 0x01dc, "nVidia QuadroFX 350M" },
  { 0x01dd, "nVidia Geforce 7500LE" },
  { 0x01de, "nVidia QuadroFX 350" },
  { 0x01df, "nVidia Geforce 7300GS" },
  { 0x04c0, "nVidia G78" },
  { 0x04c1, "nVidia G78" },
  { 0x04c2, "nVidia G78" },
  { 0x04c3, "nVidia G78" },
  { 0x04c4, "nVidia G78" },
  { 0x04c5, "nVidia G78" },
  { 0x04c6, "nVidia G78" },
  { 0x04c7, "nVidia G78" },
  { 0x04c8, "nVidia G78" },
  { 0x04c9, "nVidia G78" },
  { 0x04ca, "nVidia G78" },
  { 0x04cb, "nVidia G78" },
  { 0x04cc, "nVidia G78" },
  { 0x04cd, "nVidia G78" },
  { 0x04ce, "nVidia G78" },
  { 0x04cf, "nVidia G78" },
  { 0x0190, "nVidia Geforce 8800" },
  { 0x0191, "nVidia Geforce 8800GTX" },
  { 0x0192, "nVidia Geforce 8800" },
  { 0x0193, "nVidia Geforce 8800GTS" },
  { 0x0194, "nVidia Geforce 8800Ultra" },
  { 0x0197, "nVidia Geforce 8800" },
  { 0x019a, "nVidia G80-875" },
  { 0x019d, "nVidia QuadroFX 5600" },
  { 0x019e, "nVidia QuadroFX 4600" },
  { 0x0400, "nVidia Geforce 8600GTS" },
  { 0x0401, "nVidia Geforce 8600GT" },
  { 0x0402, "nVidia Geforce 8600GT" },
  { 0x0403, "nVidia Geforce 8600GS" },
  { 0x0404, "nVidia Geforce 8400GS" },
  { 0x0405, "nVidia Geforce 9500M GS" },
  { 0x0406, "nVidia Geforce NB9P-GE" },
  { 0x0407, "nVidia Geforce 8600M GT" },
  { 0x0408, "nVidia Geforce 8600M GTS" },
  { 0x0409, "nVidia Geforce 8700M GT" },
  { 0x040a, "nVidia Quadro NVS 370M" },
  { 0x040b, "nVidia Quadro NVS 320M" },
  { 0x040c, "nVidia QuadroFX 570M" },
  { 0x040d, "nVidia QuadroFX 1600M" },
  { 0x040e, "nVidia QuadroFX 570" },
  { 0x040f, "nVidia QuadroFX 1700" },
  { 0x0410, " NVIDIA GeForce GT 330" },
  { 0x0420, "nVidia Geforce 8400SE" },
  { 0x0421, "nVidia Geforce 8500GT" },
  { 0x0422, "nVidia Geforce 8400GS" },
  { 0x0423, "nVidia Geforce 8300GS" },
  { 0x0424, "nVidia Geforce 8400GS" },
  { 0x0425, "nVidia Geforce 8600M GS" },
  { 0x0426, "nVidia Geforce 8400M GT" },
  { 0x0427, "nVidia Geforce 8400M GS" },
  { 0x0428, "nVidia Geforce 8400M G" },
  { 0x0429, "nVidia Quadro NVS 140M" },
  { 0x042a, "nVidia Quadro NVS 130M" },
  { 0x042b, "nVidia Quadro NVS 135M" },
  { 0x042c, " NVIDIA GeForce 9400 GT" },
  { 0x042d, "nVidia Quadro FX 360M" },
  { 0x042e, "nVidia Geforce 9300M G" },
  { 0x042f, "nVidia Quadro NVS 290" },
  { 0x03d0, "nVidia Geforce 6100 nForce 430"},
  { 0x03d1, "nVidia Geforce 6100 nForce 405"},
  { 0x03d2, "nVidia Geforce 6100 nForce 400"},
  { 0x03d3, "nVidia MCP61" },
  { 0x03d4, "nVidia MCP61" },
  { 0x03d5, "nVidia Geforce 6100 nForce 420"},
  { 0x03d6, "nVidia GeForce 7025 /nVidia nForce 630a" },
  { 0x03d7, "nVidia MCP61" },
  { 0x03d8, "nVidia MCP61" },
  { 0x03d9, "nVidia MCP61" },
  { 0x03da, "nVidia MCP61" },
  { 0x03db, "nVidia MCP61" },
  { 0x03dc, "nVidia MCP61" },
  { 0x03dd, "nVidia MCP61" },
  { 0x03de, "nVidia MCP61" },
  { 0x03df, "nVidia MCP61" },
  { 0x0530, " NVIDIA GeForce 7190M / nForce 650M" },
  { 0x0531, " NVIDIA GeForce 7150M / nForce 630M" },
  { 0x0532, " NVIDIA MCP67M" },
  { 0x0533, " NVIDIA GeForce 7000M / nForce 610M" },
  { 0x053f, " NVIDIA MCP67M" },
  { 0x053a, "nVidia Geforce 7050PV nForce 630a"},
  { 0x053b, "nVidia Geforce 7050PV nForce 630a"},
  { 0x053e, "nVidia Geforce 7025 nForce 630a"},
  { 0x05e0, "nvidia GeForce GT200-400" },
  { 0x05e1, "nvidia GeForce GTX 280" },
  { 0x05e2, "nvidia GeForce GTX 260" },
  { 0x05e3, "nVidia GeForce GTX 285" },
  { 0x05e4, "nVidia GT200" },
  { 0x05e5, "nVidia GT200" },
  { 0x05e6, "nVidia GeForce GTX 275" },
  { 0x05e7, "nvidia Tesla C1060" },
  { 0x05e8, "nVidia GT200" },
  { 0x05e9, "nVidia GT200" },
  { 0x05ea, "nVidia GeForce GTX 260" },
  { 0x05eb, "nVidia GeForce GTX 295" },
  { 0x05ec, "nVidia GT200" },
  { 0x05ed, "nvidia Quadroplex 2200 D2" },
  { 0x05ee, "nVidia GT200" },
  { 0x05ef, "nVidia GT200" },
  { 0x05f0, "nVidia GT200" },
  { 0x05f1, "nVidia GT200" },
  { 0x05f2, "nVidia GT200" },
  { 0x05f3, "nVidia GT200" },
  { 0x05f4, "nVidia GT200" },
  { 0x05f5, "nVidia GT200" },
  { 0x05f6, "nVidia GT200" },
  { 0x05f7, "nVidia GT200" },
  { 0x05f8, "nvidia Quadroplex 2200 S4" },
  { 0x05f9, "nvidia Quadro CX" },
  { 0x05fa, "nVidia GT200" },
  { 0x05fb, "nVidia GT200" },
  { 0x05fc, "nVidia GT200" },
  { 0x05fd, "nvidia QuadroFX 5800" },
  { 0x05fe, "nvidia QuadroFX 5800" },
  { 0x05ff, "nVidia Quadro FX 3800" },
  { 0x0600, "nVidia Geforce 8800GTS 512" },
  { 0x0601, " NVIDIA GeForce 9800 GT" },
  { 0x0602, "nVidia Geforce 8800GT" },
  { 0x0603, " NVIDIA GeForce GT 230" },
  { 0x0604, "nVidia Geforce 9800GX2" },
  { 0x0605, " NVIDIA GeForce 9800 GT" },
  { 0x0606, "nVidia Geforce 8800GS" },
  { 0x0607, " NVIDIA GeForce GTS 240" },
  { 0x0608, " NVIDIA GeForce 9800M GTX" },
  { 0x0609, "nVidia Geforce 8800M GTS" },
  { 0x060a, " NVIDIA GeForce GTX 280M" },
  { 0x060b, " NVIDIA GeForce 9800M GT" },
  { 0x060c, "nVidia Geforce 8800M GTX" },
  { 0x060d, "nVidia Geforce 8800GS" },
  { 0x060e, " NVIDIA GeForce 9850 X" },
  { 0x060f, " NVIDIA GeForce GTX 285M" },
  { 0x0610, "nVidia Geforce 9600GSO" },
  { 0x0611, "nVidia Geforce 8800GT" },
  { 0x0612, "nVidia Geforce 9800GTX" },
  { 0x0613, " NVIDIA GeForce 9800 GTX+" },
  { 0x0614, "nVidia Geforce 9800GT" },
  { 0x0615, " NVIDIA GeForce GTS 250" },
  { 0x0616, " NVIDIA G92" },
  { 0x0617, " NVIDIA GeForce 9800M GTX" },
  { 0x0618, " NVIDIA GeForce GTX 260M" },
  { 0x0619, " NVIDIA Quadro FX 4700 X2" },
  { 0x061a, "nVidia QuadroFX 3700" },
  { 0x061b, " NVIDIA Quadro VX 200" },
  { 0x061c, "nVidia QuadroFX 3600M" },
  { 0x061d, " NVIDIA Quadro FX 2800M" },
  { 0x061e, " NVIDIA Quadro FX 3700M" },
  { 0x061f, " NVIDIA Quadro FX 3800M" },
  { 0x0620, " NVIDIA G94" },
  { 0x0621, " NVIDIA GeForce GT 230" },
  { 0x0622, "nVidia Geforce 9600GT" },
  { 0x0623, "nVidia Geforce 9600GS" },
  { 0x0624, " NVIDIA G94" },
  { 0x0625, " NVIDIA GeForce 9600 GSO 512" },
  { 0x0626, " NVIDIA GeForce GT 130" },
  { 0x0627, " NVIDIA GeForce GT 140" },
  { 0x0628, " NVIDIA GeForce 9800M GTS" },
  { 0x0629, " NVIDIA G94" },
  { 0x062a, " NVIDIA GeForce 9700M GTS" },
  { 0x062b, " NVIDIA GeForce 9800M GS" },
  { 0x062c, " NVIDIA GeForce 9800M GTS" },
  { 0x062d, " NVIDIA GeForce 9600 GT" },
  { 0x062e, " NVIDIA GeForce 9600 GT" },
  { 0x062f, " NVIDIA GeForce 9800 S" },
  { 0x0630, " NVIDIA GeForce 9700 S" },
  { 0x0631, " NVIDIA GeForce GTS 160M" },
  { 0x0632, " NVIDIA GeForce GTS 150M" },
  { 0x0633, " NVIDIA GeForce GT 220" },
  { 0x0634, " NVIDIA G94" },
  { 0x0635, " NVIDIA GeForce 9600 GSO" },
  { 0x0636, " NVIDIA G94" },
  { 0x0637, " NVIDIA GeForce 9600 GT" },
  { 0x0638, " NVIDIA Quadro FX 1800" },
  { 0x0639, " NVIDIA G94" },
  { 0x063a, " NVIDIA Quadro FX 2700M" },
  { 0x063b, " NVIDIA G94" },
  { 0x063c, " NVIDIA G94" },
  { 0x063d, " NVIDIA G94" },
  { 0x063e, " NVIDIA G94" },
  { 0x063f, " NVIDIA G94" },
  { 0x0640, "nVidia Geforce 9500GT" },
  { 0x0641, " NVIDIA GeForce 9400 GT" },
  { 0x0642, " NVIDIA GeForce 8400 GS" },
  { 0x0643, "nVidia Geforce 9500GT" },
  { 0x0644, " NVIDIA GeForce 9500 GS" },
  { 0x0645, " NVIDIA GeForce 9500 GS" },
  { 0x0646, " NVIDIA GeForce GT 120" },
  { 0x0647, "nVidia Geforce 9600M GT" },
  { 0x0648, "nVidia Geforce 9600M GS" },
  { 0x0649, "nVidia Geforce 9600M GT" },
  { 0x064a, " NVIDIA GeForce 9700M GT" },
  { 0x064b, "nVidia Geforce 9500M G" },
  { 0x064c, " NVIDIA GeForce 9650M GT" },
  { 0x064d, " NVIDIA G96" },
  { 0x064e, " NVIDIA G96" },
  { 0x064f, " NVIDIA GeForce 9600 S" },
  { 0x0650, " NVIDIA G96-825" },
  { 0x0651, " NVIDIA GeForce G 110M" },
  { 0x0652, " NVIDIA GeForce GT 130M" },
  { 0x0653, " NVIDIA GeForce GT 120M" },
  { 0x0654, " NVIDIA GeForce GT 220M" },
  { 0x0655, " NVIDIA GeForce 9500 GS / GeForce 9600 S" },
  { 0x0656, " NVIDIA GeForce 9500 GT / GeForce 9650 S" },
  { 0x0657, " NVIDIA G96" },
  { 0x0658, " NVIDIA Quadro FX 380" },
  { 0x0659, " NVIDIA Quadro FX 580" },
  { 0x065a, " NVIDIA Quadro FX 1700M" },
  { 0x065b, " NVIDIA GeForce 9400 GT" },
  { 0x065c, " NVIDIA Quadro FX 770M" },
  { 0x065d, " NVIDIA G96" },
  { 0x065e, " NVIDIA G96" },
  { 0x065f, " NVIDIA GeForce G210" },
  { 0x06e0, "nVidia Geforce 9300GE" },
  { 0x06e1, "nVidia Geforce 9300GS" },
  { 0x06e2, "nVidia Geforce 8400" },
  { 0x06e3, "nVidia Geforce 8400SE" },
  { 0x06e4, "nVidia Geforce 8400GS" },
  { 0x06e5, "nVidia Geforce 9300M GS" },
  { 0x06e6, "nVidia Geforce G100" },
  { 0x06e7, "nVidia G98" },
  { 0x06e8, "nVidia Geforce 9200M GS" },
  { 0x06e9, "nVidia Geforce 9300M GS" },
  { 0x06ea, "nVidia Quadro NVS 150M" },
  { 0x06eb, "nVidia Quadro NVS 160M" },
  { 0x06ec, "nVidia Geforce G105M" },
  { 0x06ed, "nVidia G98" },
  { 0x06ee, "nVidia G98" },
  { 0x06ef, "nVidia G98" },
  { 0x06f0, "nVidia G98" },
  { 0x06f1, "nVidia G98" },
  { 0x06f2, "nVidia G98" },
  { 0x06f3, "nVidia G98" },
  { 0x06f4, "nVidia G98" },
  { 0x06f5, "nVidia G98" },
  { 0x06f6, "nVidia G98" },
  { 0x06f7, "nVidia G98" },
  { 0x06f8, "nVidia Quadro NVS 420" },
  { 0x06f9, "nVidia QuadroFX 370 LP" },
  { 0x06fa, "nVidia Quadro NVS 450" },
  { 0x06fb, "nVidia QuadroFX 370M" },
  { 0x06fc, "nVidia G98" },
  { 0x06fd, "nVidia Quadro NVS 295" },
  { 0x06fe, "nVidia G98" },
  { 0x06ff, "nVidia G98-GL" },
  { 0x07e0, " NVIDIA GeForce 7150 / NVIDIA nForce 630i" },
  { 0x07e1, " NVIDIA GeForce 7100 / NVIDIA nForce 630i" },
  { 0x07e2, " NVIDIA GeForce 7050 / NVIDIA nForce 630i" },
  { 0x07e3, " NVIDIA GeForce 7050 / NVIDIA nForce 610i" },
  { 0x07e4, " NVIDIA MCP73" },
  { 0x07e5, " NVIDIA GeForce 7050 / NVIDIA nForce 620i" },
  { 0x07e6, " NVIDIA MCP73" },
  { 0x07e7, " NVIDIA MCP73" },
  { 0x07e8, " NVIDIA MCP73" },
  { 0x07e9, " NVIDIA MCP73" },
  { 0x07ea, " NVIDIA MCP73" },
  { 0x07eb, " NVIDIA MCP73" },
  { 0x07ec, " NVIDIA MCP73" },
  { 0x07ed, " NVIDIA MCP73" },
  { 0x07ee, " NVIDIA MCP73" },
  { 0x07ef, " NVIDIA MCP73" },
  { 0x0840, " NVIDIA GeForce 8200M" },
  { 0x0842, " NVIDIA MCP77/78" },
  { 0x0844, " NVIDIA GeForce 9100M G" },
  { 0x0845, " NVIDIA GeForce 8200M G / GeForce 8200M" },
  { 0x0846, " NVIDIA GeForce 9200" },
  { 0x0847, " NVIDIA GeForce 9100" },
  { 0x0848, " NVIDIA GeForce 8300" },
  { 0x0849, " NVIDIA GeForce 8200" },
  { 0x084a, " NVIDIA nForce 730a" },
  { 0x084b, " NVIDIA GeForce 8200 / GeForce 9200" },
  { 0x084c, " NVIDIA nForce 980a/780a SLI" },
  { 0x084d, " NVIDIA nForce 750a SLI" },
  { 0x084f, " NVIDIA GeForce 8100 / nForce 720a" },
  { 0x0850, " NVIDIA MCP77/78" },
  { 0x0851, " NVIDIA MCP77/78" },
  { 0x0852, " NVIDIA MCP77/78" },
  { 0x0853, " NVIDIA MCP77/78" },
  { 0x0854, " NVIDIA MCP77/78" },
  { 0x0855, " NVIDIA MCP77/78" },
  { 0x0856, " NVIDIA MCP77/78" },
  { 0x0857, " NVIDIA MCP77/78" },
  { 0x0858, " NVIDIA MCP77/78" },
  { 0x0859, " NVIDIA MCP77/78" },
  { 0x085a, " NVIDIA MCP77/78" },
  { 0x085b, " NVIDIA MCP77/78" },
  { 0x085c, " NVIDIA MCP77/78" },
  { 0x085d, " NVIDIA MCP77/78" },
  { 0x085e, " NVIDIA MCP77/78" },
  { 0x085f, " NVIDIA MCP77/78" },
  { 0x0860, "nVidia Geforce 9300" },
  { 0x0861, "nVidia Geforce 9400" },
  { 0x0862, " NVIDIA GeForce 9400M G" },
  { 0x0863, "nVidia Geforce 9400M" },
  { 0x0864, "nVidia Geforce 9300" },
  { 0x0865, "nVidia Geforce 9300" },
  { 0x0866, " NVIDIA GeForce 9400M G" },
  { 0x0867, " NVIDIA GeForce 9400" },
  { 0x0868, " NVIDIA nForce 760i SLI" },
  { 0x0869, " NVIDIA GeForce 9400" },
  { 0x086a, " NVIDIA GeForce 9400" },
  { 0x086b, " NVIDIA MCP7A-U" },
  { 0x086c, " NVIDIA GeForce 9300 / nForce 730i" },
  { 0x086d, " NVIDIA GeForce 9200" },
  { 0x086e, " NVIDIA GeForce 9100M G" },
  { 0x086f, " NVIDIA GeForce 8200M G" },
  { 0x0870, " NVIDIA GeForce 9400M" },
  { 0x0871, " NVIDIA GeForce 9200" },
  { 0x0872, " NVIDIA GeForce G102M" },
  { 0x0873, " NVIDIA GeForce G102M" },
  { 0x0874, " NVIDIA ION" },
  { 0x0876, " NVIDIA ION" },
  { 0x087a, " NVIDIA Quadro FX 470 / GeForce 9400" },
  { 0x087b, " NVIDIA MCP7A-GL" },
  { 0x087d, " NVIDIA ION" },
  { 0x087e, " NVIDIA ION LE" },
  { 0x087f, " NVIDIA ION LE" },

  { 0x0a00, " NVIDIA GT212" },
  { 0x0a10, " NVIDIA GT212" },
  { 0x0a20, " NVIDIA GeForce GT 220" },
  { 0x0a21, " NVIDIA D10M2-20" },
  { 0x0a22, " NVIDIA GeForce 315" },
  { 0x0a23, " NVIDIA GeForce 210" },
  { 0x0a28, " NVIDIA GeForce GT 230M" },
  { 0x0a29, " NVIDIA GeForce GT 330M" },
  { 0x0a2a, " NVIDIA GeForce GT 230M" },
  { 0x0a2b, " NVIDIA GeForce GT 330M" },
  { 0x0a2c, " NVIDIA NVS 5100M" },
  { 0x0a2d, " NVIDIA GeForce GT 320M" },
  { 0x0a30, " NVIDIA GT216" },
  { 0x0a34, " NVIDIA GeForce GT 240M" },
  { 0x0a35, " NVIDIA GeForce GT 325M" },
  { 0x0a3c, " NVIDIA Quadro FX 880M" },
  { 0x0a3d, " NVIDIA N10P-ES" },
  { 0x0a3f, " NVIDIA GT216-INT" },
  { 0x0a60, " NVIDIA GeForce G210" },
  { 0x0a61, " NVIDIA NVS 2100" },
  { 0x0a62, " NVIDIA GeForce 205" },
  { 0x0a63, " NVIDIA GeForce 310 / NVIDIA NVS 3100" },
  { 0x0a63, " NVIDIA GeForce 310" },
  { 0x0a64, " NVIDIA ION" },
  { 0x0a65, " NVIDIA GeForce 210" },
  { 0x0a66, " NVIDIA GeForce 310" },
  { 0x0a67, " NVIDIA GeForce 315" },
  { 0x0a68, " NVIDIA GeForce G105M" },
  { 0x0a69, " NVIDIA GeForce G105M" },
  { 0x0a6a, " NVIDIA NVS 2100M" },
  { 0x0a6c, " NVIDIA NVS 3100M" },
  { 0x0a6e, " NVIDIA GeForce 305M" },
  { 0x0a6f, " NVIDIA ION" },
  { 0x0a70, " NVIDIA GeForce 310M" },
  { 0x0a72, " NVIDIA GeForce 310M" },
  { 0x0a73, " NVIDIA GeForce 305M" },
  { 0x0a74, " NVIDIA GeForce G210M" },
  { 0x0a75, " NVIDIA GeForce 310M" },
  { 0x0a76, " NVIDIA ION" },
  { 0x0a78, " NVIDIA Quadro FX 380 LP" },
  { 0x0a7c, " NVIDIA Quadro FX 380M" },
  { 0x0a7d, " NVIDIA GT218-ES" },
  { 0x0a7e, " NVIDIA GT218-INT-S" },
  { 0x0a7f, " NVIDIA GT218-INT-B" },

  { 0x06c0, " NVIDIA GeForce GTX 480" },
  { 0x06c4, " NVIDIA GeForce GTX 465" },
  { 0x06cd, " NVIDIA GeForce GTX 470" },
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

void get_vendor_name(short vendor_id, char *str)
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
    case 0x0000:
      strcpy(str, "None");
      break;
    default:
      strcpy(str, "Unknown");
  }
}
