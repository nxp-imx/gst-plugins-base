/*
 * Copyright (c) 2013-2016, Freescale Semiconductor, Inc. All rights reserved.
 * Copyright 2017, 2018 NXP
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __IMX_COMMON_H__
#define __IMX_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <gst/gst.h>
#include <string.h>

#define IMX_GST_PLUGIN_AUTHOR "Multimedia Team <shmmmw@freescale.com>"
#define IMX_GST_PLUGIN_PACKAGE_NAME "Freescle Gstreamer Multimedia Plugins"
#define IMX_GST_PLUGIN_PACKAGE_ORIG "http://www.freescale.com"
#define IMX_GST_PLUGIN_LICENSE "LGPL"

#define IMX_GST_PLUGIN_RANK (GST_RANK_PRIMARY+1)

#define IMX_GST_PLUGIN_DEFINE(name, description, initfunc)\
  GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,\
      GST_VERSION_MINOR,\
      name.imx,\
      description,\
      initfunc,\
      VERSION,\
      IMX_GST_PLUGIN_LICENSE,\
      IMX_GST_PLUGIN_PACKAGE_NAME, IMX_GST_PLUGIN_PACKAGE_ORIG)

#define CHIPCODE(a,b,c,d)( (((unsigned int)((a)))<<24) | (((unsigned int)((b)))<<16)|(((unsigned int)((c)))<<8)|(((unsigned int)((d)))))
typedef enum
{
  CC_MX23 = CHIPCODE ('M', 'X', '2', '3'),
  CC_MX25 = CHIPCODE ('M', 'X', '2', '5'),
  CC_MX27 = CHIPCODE ('M', 'X', '2', '7'),
  CC_MX28 = CHIPCODE ('M', 'X', '2', '8'),
  CC_MX31 = CHIPCODE ('M', 'X', '3', '1'),
  CC_MX35 = CHIPCODE ('M', 'X', '3', '5'),
  CC_MX37 = CHIPCODE ('M', 'X', '3', '7'),
  CC_MX50 = CHIPCODE ('M', 'X', '5', '0'),
  CC_MX51 = CHIPCODE ('M', 'X', '5', '1'),
  CC_MX53 = CHIPCODE ('M', 'X', '5', '3'),
  CC_MX6Q = CHIPCODE ('M', 'X', '6', 'Q'),
  CC_MX60 = CHIPCODE ('M', 'X', '6', '0'),
  CC_MX6SL = CHIPCODE ('M', 'X', '6', '1'),
  CC_MX6SX = CHIPCODE ('M', 'X', '6', '2'),
  CC_MX6UL = CHIPCODE ('M', 'X', '6', '3'),
  CC_MX6SLL = CHIPCODE ('M', 'X', '6', '4'),
  CC_MX7D = CHIPCODE ('M', 'X', '7', 'D'),
  CC_MX7ULP = CHIPCODE ('M', 'X', '7', 'U'),
  CC_MX8 = CHIPCODE ('M', 'X', '8', '0'),
  CC_MX8QM = CHIPCODE ('M', 'X', '8', '1'),
  CC_MX8QXP = CHIPCODE ('M', 'X', '8', '3'),
  CC_MX8M = CHIPCODE ('M', 'X', '8', '2'),
  CC_MX8MM = CHIPCODE ('M', 'X', '8', '4'),
  CC_MX8MN = CHIPCODE ('M', 'X', '8', '5'),
  CC_MX8MP = CHIPCODE ('M', 'X', '8', '6'),
  CC_MX8ULP = CHIPCODE ('M', 'X', '8', 'U'),
  CC_MX93 = CHIPCODE ('M', 'X', '9', '3'),
  CC_MX95 = CHIPCODE ('M', 'X', '9', '5'),
  CC_UNKN = CHIPCODE ('U', 'N', 'K', 'N')

} CHIP_CODE;

typedef struct {
  CHIP_CODE code;
  int chip_num;
} CPU_INFO;

typedef struct {
  CHIP_CODE code;
  char *name;
} SOC_INFO;

typedef struct {
  CHIP_CODE chip_name;
  gboolean g3d;
  gboolean g2d;
  gboolean ipu;
  gboolean pxp;
  gboolean vpu;
  gboolean dpu;
  gboolean dcss;
} IMXV4l2FeatureMap;

typedef enum {
  G3D = 1,
  G2D,
  IPU,
  PXP,
  VPU,
  DPU,
  DCSS
} CHIP_FEATURE;


#define HAS_G3D() check_feature(imx_chip_code(), G3D)
#define HAS_G2D() check_feature(imx_chip_code(), G2D)
#define HAS_IPU() check_feature(imx_chip_code(), IPU)
#define HAS_PXP() check_feature(imx_chip_code(), PXP)
#define HAS_VPU() check_feature(imx_chip_code(), VPU)
#define HAS_DPU() check_feature(imx_chip_code(), DPU)
#define HAS_DCSS() check_feature(imx_chip_code(), DCSS)

#define IS_HANTRO() ((CC_MX8M == imx_chip_code()) || (CC_MX8MM == imx_chip_code()) || (CC_MX8MP == imx_chip_code()) )
#define IS_AMPHION() ((CC_MX8QM == imx_chip_code()) || (CC_MX8QXP == imx_chip_code()))
#define IS_IMX8MM() (CC_MX8MM == imx_chip_code())
#define IS_IMX8MN() (CC_MX8MN == imx_chip_code())
#define IS_IMX8MQ() (CC_MX8M == imx_chip_code())
#define IS_IMX8MP() (CC_MX8MP == imx_chip_code())
#define IS_IMX8ULP() (CC_MX8ULP == imx_chip_code())
#define IS_IMX93() (CC_MX93 == imx_chip_code())
#define IS_IMX95() (CC_MX95 == imx_chip_code())
#define IS_IMX8Q() ((CC_MX8QM == imx_chip_code()) || (CC_MX8QXP == imx_chip_code()))
#define IS_IMX6Q() (CC_MX6Q == imx_chip_code())


/* define rotate and flip glib enum for overlaysink and imxv4l2sink */
typedef enum
{
  GST_IMX_ROTATION_0 = 0,
  GST_IMX_ROTATION_90,
  GST_IMX_ROTATION_180,
  GST_IMX_ROTATION_270,
  GST_IMX_ROTATION_HFLIP,
  GST_IMX_ROTATION_VFLIP
}GstImxRotateMethod;

/*=============================================================================
FUNCTION:               get_chipname

DESCRIPTION:            To get chipname from /proc/cpuinfo

ARGUMENTS PASSED: STR of chipname

RETURN VALUE:            chip code
=============================================================================*/
//*

static CPU_INFO cpu_info[] = {
  {CC_MX23, 0x23},
  {CC_MX25, 0x25},
  {CC_MX27, 0x27},
  {CC_MX28, 0x28},
  {CC_MX31, 0x31},
  {CC_MX35, 0x35},
  {CC_MX37, 0x37},
  {CC_MX50, 0x50},
  {CC_MX51, 0x51},
  {CC_MX53, 0x53},
  {CC_MX6Q, 0x61},
  {CC_MX6Q, 0x63},
  {CC_MX60, 0x60}
};

static CHIP_CODE getChipCodeFromCpuinfo (void)
{
  FILE *fp = NULL;
  char buf[100], *p, *rev;
  char chip_name[3];
  int len = 0, i;
  int chip_num = -1;
  CHIP_CODE cc = CC_UNKN;
  fp = fopen ("/proc/cpuinfo", "r");
  if (fp == NULL) {
    return cc;
  }
  while (!feof (fp)) {
    p = fgets (buf, 100, fp);
    p = strstr (buf, "Revision");
    if (p != NULL) {
      rev = index (p, ':');
      if (rev != NULL) {
        rev++;
        chip_num = strtoul (rev, NULL, 16);
        chip_num >>= 12;
        break;
      }
    }
  }

  fclose (fp);

  if (chip_num < 0) {
    return cc;
  }

  int num = sizeof(cpu_info) / sizeof(CPU_INFO);
  for(i=0; i<num; i++) {
    if(chip_num == cpu_info[i].chip_num) {
      cc = cpu_info[i].code;
      break;
    }
  }

  return cc;
}

static SOC_INFO soc_info[] = {
  {CC_MX23, "i.MX23"},
  {CC_MX25, "i.MX25"},
  {CC_MX27, "i.MX27"},
  {CC_MX28, "i.MX28"},
  {CC_MX31, "i.MX31"},
  {CC_MX35, "i.MX35"},
  {CC_MX37, "i.MX37"},
  {CC_MX50, "i.MX50"},
  {CC_MX51, "i.MX51"},
  {CC_MX53, "i.MX53"},
  {CC_MX6Q, "i.MX6DL"},
  {CC_MX6Q, "i.MX6Q"},
  {CC_MX6Q, "i.MX6QP"},
  {CC_MX6SL, "i.MX6SL"},
  {CC_MX6SLL, "i.MX6SLL"},
  {CC_MX6SX, "i.MX6SX"},
  {CC_MX6UL, "i.MX6UL"},
  {CC_MX6UL, "i.MX6ULL"},
  {CC_MX7D, "i.MX7D"},
  {CC_MX7ULP, "i.MX7ULP"},
  {CC_MX8, "i.MX8DV"},
  {CC_MX8QM, "i.MX8QM"},
  {CC_MX8QXP, "i.MX8QXP"},
  {CC_MX8M, "i.MX8MQ"},
  {CC_MX8MM, "i.MX8MM"},
  {CC_MX8MN, "i.MX8MN"},
  {CC_MX8MP, "i.MX8MP"},
  {CC_MX8ULP, "i.MX8ULP"},
  {CC_MX93, "i.MX93"},
  {CC_MX95, "i.MX95"},
};

static CHIP_CODE getChipCodeFromSocid (void)
{
  FILE *fp = NULL;
  char soc_name[100];
  CHIP_CODE code = CC_UNKN;

  fp = fopen("/sys/devices/soc0/soc_id", "r");
  if (fp == NULL) {
    g_print("open /sys/devices/soc0/soc_id failed.\n");
    return  CC_UNKN;
  }

  if (fscanf(fp, "%100s", soc_name) != 1) {
    g_print("fscanf soc_id failed.\n");
    fclose(fp);
    return CC_UNKN;
  }
  fclose(fp);

  //GST_INFO("SOC is %s\n", soc_name);

  int num = sizeof(soc_info) / sizeof(SOC_INFO);
  int i;
  for(i=0; i<num; i++) {
    if(!strcmp(soc_name, soc_info[i].name)) {
      code = soc_info[i].code;
      break;
    }
  }

  return code;
}

#define KERN_VER(a, b, c) (((a) << 16) + ((b) << 8) + (c))

static CHIP_CODE gimx_chip_code = CC_UNKN;

static CHIP_CODE imx_chip_code (void)
{
  struct utsname sys_name;
  int kv, kv_major, kv_minor, kv_rel;
  char soc_name[255];
  int rev_major, rev_minor;
  int idx, num;

  if (gimx_chip_code != CC_UNKN)
    return gimx_chip_code;

  if (uname(&sys_name) < 0) {
    g_print("get kernel version via uname failed.\n");
    return CC_UNKN;
  }

  if (sscanf(sys_name.release, "%d.%d.%d", &kv_major, &kv_minor, &kv_rel) != 3) {
    g_print("sscanf kernel version failed.\n");
    return CC_UNKN;
  }

  kv = ((kv_major << 16) + (kv_minor << 8) + kv_rel);
  //GST_INFO("kernel:%s, %d.%d.%d\n", sys_name.release, kv_major, kv_minor, kv_rel);

  if (kv < KERN_VER(3, 10, 0))
    gimx_chip_code = getChipCodeFromCpuinfo();
  else
    gimx_chip_code = getChipCodeFromSocid();

  return gimx_chip_code;
}

static IMXV4l2FeatureMap g_imxv4l2feature_maps[] = {
  /* chip_name, g3d, g2d, ipu, pxp, vpu, dpu, dcss*/
  {CC_MX6Q, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE},
  {CC_MX6SL, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX6SLL, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX6SX, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX6UL, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX7D, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX7ULP, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},
  {CC_MX8, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE},
  {CC_MX8QM, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE},
  {CC_MX8QXP, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE},
  {CC_MX8M, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE},
  {CC_MX8MM, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE},
  {CC_MX8MP, TRUE, TRUE, FALSE, FALSE, TRUE, FALSE, FALSE},
  {CC_MX8MN, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
  {CC_MX8ULP, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX93, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE},
  {CC_MX95, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, FALSE},
};


static gboolean check_feature(CHIP_CODE chip_name, CHIP_FEATURE feature)
{
  int i;
  gboolean ret = FALSE;
  for (i=0; i<sizeof(g_imxv4l2feature_maps)/sizeof(IMXV4l2FeatureMap); i++) {	
    if ( chip_name== g_imxv4l2feature_maps[i].chip_name) {
      switch (feature) {
        case G3D:
          ret = g_imxv4l2feature_maps[i].g3d;
          break;
        case G2D:
          ret = g_imxv4l2feature_maps[i].g2d;
          break;
        case IPU:
          ret = g_imxv4l2feature_maps[i].ipu;
          break;
        case PXP:
          ret = g_imxv4l2feature_maps[i].pxp;
          break;
        case VPU:
          ret = g_imxv4l2feature_maps[i].vpu;
          break;
        case DPU:
          ret = g_imxv4l2feature_maps[i].dpu;
          break;
        case DCSS:
          ret = g_imxv4l2feature_maps[i].dcss;
          break;
        default:
          break;
      }
      break;
    }
  }
  return ret;
}

#ifdef __cplusplus
}
#endif

#endif /* __IMX_COMMON_H__ */ 
