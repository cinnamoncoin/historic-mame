/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *mooncrst_attributesram;
extern unsigned char *mooncrst_bulletsram;
extern void mooncrst_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void mooncrst_attributes_w(int offset,int data);
extern void mooncrst_stars_w(int offset,int data);
extern void mooncrst_gfxextend_w(int offset,int data);
extern int mooncrst_vh_start(void);
extern void mooncrst_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void mooncrst_sound_freq_w(int offset,int data);
extern int mooncrst_sh_start(void);
extern void mooncrst_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_RAM },	/* video RAM, screen attributes, sprites, bullets */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW (coins per play) */
	{ 0xb800, 0xb800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram },
	{ 0x9800, 0x983f, mooncrst_attributes_w, &mooncrst_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram },
	{ 0x9860, 0x9880, MWA_RAM, &mooncrst_bulletsram },
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb800, 0xb800, mooncrst_sound_freq_w },
	{ 0xa000, 0xa002, mooncrst_gfxextend_w },
	{ 0xb004, 0xb004, mooncrst_stars_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_3, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ 0, 0, OSD_JOY_LEFT, OSD_JOY_RIGHT,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x80,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 1, 0x80, "LANGUAGE", { "JAPANESE", "ENGLISH" } },
	{ 1, 0x40, "SW1", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the color table */
static struct GfxLayout starslayout =
{
	1,1,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 0, 0,      &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char mooncrst_color_prom[] =
{
	/* palette */
	0x00,0x7a,0x36,0x07,0x00,0xf0,0x38,0x1f,0x00,0xc7,0xf0,0x3f,0x00,0xdb,0xc6,0x38,
	0x00,0x36,0x07,0xf0,0x00,0x33,0x3f,0xdb,0x00,0x3f,0x57,0xc6,0x00,0xc6,0x3f,0xff
};

static unsigned char fantazia_color_prom[] =
{
	/* palette */
	0x08,0x3B,0xCB,0xFE,0x08,0x1F,0xC8,0x3F,0x08,0xD8,0x0F,0x3F,0x08,0xC8,0xCC,0x0F,
	0x08,0xC8,0xB8,0x1F,0x08,0x1E,0x79,0x0F,0x08,0xFE,0x0F,0xF8,0x08,0x7E,0x0F,0xCE
};



/* waveforms for the audio hardware */
static unsigned char samples[32] =	/* a simple sine (sort of) wave */
{
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
	0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	mooncrst_vh_convert_color_prom,

	0,
	mooncrst_vh_start,
	generic_vh_stop,
	mooncrst_vh_screenrefresh,

	/* sound hardware */
	samples,
	0,
	mooncrst_sh_start,
	0,
	mooncrst_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mooncrst_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mc1", 0x0000, 0x0800 )
	ROM_LOAD( "mc2", 0x0800, 0x0800 )
	ROM_LOAD( "mc3", 0x1000, 0x0800 )
	ROM_LOAD( "mc4", 0x1800, 0x0800 )
	ROM_LOAD( "mc5", 0x2000, 0x0800 )
	ROM_LOAD( "mc6", 0x2800, 0x0800 )
	ROM_LOAD( "mc7", 0x3000, 0x0800 )
	ROM_LOAD( "mc8", 0x3800, 0x0800 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mcb", 0x0000, 0x0800 )
	ROM_LOAD( "mcd", 0x0800, 0x0800 )
	ROM_LOAD( "mca", 0x1000, 0x0800 )
	ROM_LOAD( "mcc", 0x1800, 0x0800 )
ROM_END

ROM_START( mooncrsb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "EPR194", 0x0000, 0x0800 )
	ROM_LOAD( "EPR195", 0x0800, 0x0800 )
	ROM_LOAD( "EPR196", 0x1000, 0x0800 )
	ROM_LOAD( "EPR197", 0x1800, 0x0800 )
	ROM_LOAD( "EPR198", 0x2000, 0x0800 )
	ROM_LOAD( "EPR199", 0x2800, 0x0800 )
	ROM_LOAD( "EPR200", 0x3000, 0x0800 )
	ROM_LOAD( "EPR201", 0x3800, 0x0800 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR203", 0x0000, 0x0800 )
	ROM_LOAD( "EPR172", 0x0800, 0x0800 )
	ROM_LOAD( "EPR202", 0x1000, 0x0800 )
	ROM_LOAD( "EPR171", 0x1800, 0x0800 )
ROM_END

ROM_START( fantazia_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "F01.bin", 0x0000, 0x0800 )
	ROM_LOAD( "F02.bin", 0x0800, 0x0800 )
	ROM_LOAD( "F03.bin", 0x1000, 0x0800 )
	ROM_LOAD( "F04.bin", 0x1800, 0x0800 )
	ROM_LOAD( "F09.bin", 0x2000, 0x0800 )
	ROM_LOAD( "F10.bin", 0x2800, 0x0800 )
	ROM_LOAD( "F11.bin", 0x3000, 0x0800 )
	ROM_LOAD( "F12.bin", 0x3800, 0x0800 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1h_1_10.bin", 0x0000, 0x0800 )
	ROM_LOAD( "1k_2_12.bin", 0x0800, 0x0800 )
	ROM_LOAD( "1k_1_11.bin", 0x1000, 0x0800 )
	ROM_LOAD( "1h_2_09.bin", 0x1800, 0x0800 )
ROM_END



static unsigned moonqsr_decode(int A)
{
	static unsigned char evetab[] =
	{
		0x00,0x01,0x06,0x07,0x40,0x41,0x46,0x47,0x08,0x09,0x0e,0x0f,0x48,0x49,0x4e,0x4f,
		0x10,0x11,0x16,0x17,0x50,0x51,0x56,0x57,0x18,0x19,0x1e,0x1f,0x58,0x59,0x5e,0x5f,
		0x60,0x61,0x66,0x67,0x20,0x21,0x26,0x27,0x68,0x69,0x6e,0x6f,0x28,0x29,0x2e,0x2f,
		0x70,0x71,0x76,0x77,0x30,0x31,0x36,0x37,0x78,0x79,0x7e,0x7f,0x38,0x39,0x3e,0x3f,
		0x04,0x05,0x02,0x03,0x44,0x45,0x42,0x43,0x0c,0x0d,0x0a,0x0b,0x4c,0x4d,0x4a,0x4b,
		0x14,0x15,0x12,0x13,0x54,0x55,0x52,0x53,0x1c,0x1d,0x1a,0x1b,0x5c,0x5d,0x5a,0x5b,
		0x64,0x65,0x62,0x63,0x24,0x25,0x22,0x23,0x6c,0x6d,0x6a,0x6b,0x2c,0x2d,0x2a,0x2b,
		0x74,0x75,0x72,0x73,0x34,0x35,0x32,0x33,0x7c,0x7d,0x7a,0x7b,0x3c,0x3d,0x3a,0x3b,
		0x80,0x81,0x86,0x87,0xc0,0xc1,0xc6,0xc7,0x88,0x89,0x8e,0x8f,0xc8,0xc9,0xce,0xcf,
		0x90,0x91,0x96,0x97,0xd0,0xd1,0xd6,0xd7,0x98,0x99,0x9e,0x9f,0xd8,0xd9,0xde,0xdf,
		0xe0,0xe1,0xe6,0xe7,0xa0,0xa1,0xa6,0xa7,0xe8,0xe9,0xee,0xef,0xa8,0xa9,0xae,0xaf,
		0xf0,0xf1,0xf6,0xf7,0xb0,0xb1,0xb6,0xb7,0xf8,0xf9,0xfe,0xff,0xb8,0xb9,0xbe,0xbf,
		0x84,0x85,0x82,0x83,0xc4,0xc5,0xc2,0xc3,0x8c,0x8d,0x8a,0x8b,0xcc,0xcd,0xca,0xcb,
		0x94,0x95,0x92,0x93,0xd4,0xd5,0xd2,0xd3,0x9c,0x9d,0x9a,0x9b,0xdc,0xdd,0xda,0xdb,
		0xe4,0xe5,0xe2,0xe3,0xa4,0xa5,0xa2,0xa3,0xec,0xed,0xea,0xeb,0xac,0xad,0xaa,0xab,
		0xf4,0xf5,0xf2,0xf3,0xb4,0xb5,0xb2,0xb3,0xfc,0xfd,0xfa,0xfb,0xbc,0xbd,0xba,0xbb
	};
	static unsigned char oddtab[] =
	{
		0x00,0x01,0x42,0x43,0x04,0x05,0x46,0x47,0x08,0x09,0x4a,0x4b,0x0c,0x0d,0x4e,0x4f,
		0x10,0x11,0x52,0x53,0x14,0x15,0x56,0x57,0x18,0x19,0x5a,0x5b,0x1c,0x1d,0x5e,0x5f,
		0x24,0x25,0x66,0x67,0x20,0x21,0x62,0x63,0x2c,0x2d,0x6e,0x6f,0x28,0x29,0x6a,0x6b,
		0x34,0x35,0x76,0x77,0x30,0x31,0x72,0x73,0x3c,0x3d,0x7e,0x7f,0x38,0x39,0x7a,0x7b,
		0x40,0x41,0x02,0x03,0x44,0x45,0x06,0x07,0x48,0x49,0x0a,0x0b,0x4c,0x4d,0x0e,0x0f,
		0x50,0x51,0x12,0x13,0x54,0x55,0x16,0x17,0x58,0x59,0x1a,0x1b,0x5c,0x5d,0x1e,0x1f,
		0x64,0x65,0x26,0x27,0x60,0x61,0x22,0x23,0x6c,0x6d,0x2e,0x2f,0x68,0x69,0x2a,0x2b,
		0x74,0x75,0x36,0x37,0x70,0x71,0x32,0x33,0x7c,0x7d,0x3e,0x3f,0x78,0x79,0x3a,0x3b,
		0x80,0x81,0xc2,0xc3,0x84,0x85,0xc6,0xc7,0x88,0x89,0xca,0xcb,0x8c,0x8d,0xce,0xcf,
		0x90,0x91,0xd2,0xd3,0x94,0x95,0xd6,0xd7,0x98,0x99,0xda,0xdb,0x9c,0x9d,0xde,0xdf,
		0xa4,0xa5,0xe6,0xe7,0xa0,0xa1,0xe2,0xe3,0xac,0xad,0xee,0xef,0xa8,0xa9,0xea,0xeb,
		0xb4,0xb5,0xf6,0xf7,0xb0,0xb1,0xf2,0xf3,0xbc,0xbd,0xfe,0xff,0xb8,0xb9,0xfa,0xfb,
		0xc0,0xc1,0x82,0x83,0xc4,0xc5,0x86,0x87,0xc8,0xc9,0x8a,0x8b,0xcc,0xcd,0x8e,0x8f,
		0xd0,0xd1,0x92,0x93,0xd4,0xd5,0x96,0x97,0xd8,0xd9,0x9a,0x9b,0xdc,0xdd,0x9e,0x9f,
		0xe4,0xe5,0xa6,0xa7,0xe0,0xe1,0xa2,0xa3,0xec,0xed,0xae,0xaf,0xe8,0xe9,0xaa,0xab,
		0xf4,0xf5,0xb6,0xb7,0xf0,0xf1,0xb2,0xb3,0xfc,0xfd,0xbe,0xbf,0xf8,0xf9,0xba,0xbb
	};


	if (A & 1) return oddtab[RAM[A]];
	else return evetab[RAM[A]];
}



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8042],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0x804e],"\x00\x50\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x8042],1,17*5,f);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x8042],1,17*5,f);
		fclose(f);
	}
}



struct GameDriver mooncrst_driver =
{
	"mooncrst",
	&machine_driver,

	mooncrst_rom,
	moonqsr_decode, 0,

	input_ports, dsw,

	mooncrst_color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x07, 0x02,
	8*13, 8*16, 0x00,

	hiload, hisave
};

struct GameDriver mooncrsb_driver =
{
	"mooncrsb",
	&machine_driver,

	mooncrsb_rom,
	0, 0,

	input_ports, dsw,

	mooncrst_color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x07, 0x02,
	8*13, 8*16, 0x00,

	hiload, hisave
};

struct GameDriver fantazia_driver =
{
	"fantazia",
	&machine_driver,

	fantazia_rom,
	0, 0,

	input_ports, dsw,

	fantazia_color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	0x00, 0x01,
	8*13, 8*16, 0x03,

	hiload, hisave
};
