/***************************************************************************

Commando memory map (preliminary)

MAIN CPU
0000-bfff ROM
d000-d3ff Video RAM
d400-d7ff Color RAM
d800-dbff background video RAM
dc00-dfff background color RAM
e000-efff RAM
fe00-ff7f Sprites

read:
c000      IN0
c001      IN1
c002      IN2
c003      DSW1
c004      DSW2

write:
c808-c809 background scroll position

SOUND CPU
0000-3fff ROM
4000-47ff RAM

write:
8000      YM2203 #1 control
8001      YM2203 #1 write
8002      YM2203 #2 control
8003      YM2203 #2 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern int commando_interrupt(void);

extern unsigned char *commando_bgvideoram,*commando_bgcolorram;
extern unsigned char *commando_scroll;
extern void commando_bgvideoram_w(int offset,int data);
extern void commando_bgcolorram_w(int offset,int data);
int commando_vh_start(void);
void commando_vh_stop(void);
extern void commando_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void commando_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int c1942_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xd000, 0xd3ff, videoram_w, &videoram },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xdbff, commando_bgvideoram_w, &commando_bgvideoram },
	{ 0xdc00, 0xdfff, commando_bgcolorram_w, &commando_bgcolorram },
	{ 0xfe00, 0xff7f, MWA_RAM, &spriteram },
	{ 0xc808, 0xc809, MWA_RAM, &commando_scroll },
	{ 0xc800, 0xc800, sound_command_w },
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, sound_command_latch_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, AY8910_control_port_1_w },
	{ 0x8003, 0x8003, AY8910_write_port_1_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
	{ 1, 3, "MOVE UP" },
	{ 1, 1, "MOVE LEFT"  },
	{ 1, 0, "MOVE RIGHT" },
	{ 1, 2, "MOVE DOWN" },
	{ 1, 4, "FIRE" },
	{ 1, 5, "GRENADE" },
	{ -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x0c, "LIVES", { "5", "2", "4", "3" }, 1 },
	{ 4, 0x07, "BONUS", { "NONE", "20000 700000", "30000 800000", "10000 600000", "40000 1000000", "20000 600000", "30000 700000", "10000 500000" }, 1 },
	{ 4, 0x10, "DIFFICULTY", { "DIFFICULT", "NORMAL" }, 1 },
	{ 3, 0x03, "STARTING STAGE", { "7", "3", "5", "1" }, 1 },
	{ 4, 0x08, "DEMO SOUNDS", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 tiles */
	256,	/* 256 tiles */
	3,	/* 3 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0x4000*8+4, 0x4000*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,           0, 16 },
	{ 1, 0x04000, &tilelayout,   16*4+4*16, 16 },
	{ 1, 0x06000, &tilelayout,   16*4+4*16, 16 },
	{ 1, 0x10000, &tilelayout,   16*4+4*16, 16 },
	{ 1, 0x12000, &tilelayout,   16*4+4*16, 16 },
	{ 1, 0x1c000, &spritelayout,      16*4, 4 },
	{ 1, 0x24000, &spritelayout,      16*4, 4 },
	{ 1, 0x2c000, &spritelayout,      16*4, 4 },
	{ -1 } /* end of array */
};



/* these are NOT the original color PROMs */
static unsigned char color_prom[] =
{
	/* 1D: palette red component */
	0x00,0x88,0x00,0x99,0x66,0x00,0x77,0x33,0x44,0x88,0xaa,0x77,0x55,0x33,0x77,0x55,
	0x33,0x88,0x00,0x66,0x44,0x00,0x77,0x33,0xaa,0x88,0x99,0x66,0x44,0x33,0x66,0x44,
	0xaa,0x88,0x55,0x55,0x44,0x99,0x55,0x77,0x00,0x00,0x00,0x00,0x66,0x44,0x77,0xbb,
	0x00,0x99,0xaa,0x88,0xbb,0x77,0x44,0x66,0x33,0x88,0x77,0xaa,0x88,0x66,0x44,0x88,
	0x55,0x88,0x00,0x99,0x66,0x00,0x77,0x33,0x88,0x33,0xaa,0x99,0x77,0x77,0x55,0x44,
	0x99,0x88,0xaa,0x77,0x66,0x55,0x44,0x77,0x00,0x88,0x66,0x55,0x44,0x99,0x77,0xbb,
	0x00,0x88,0x66,0x55,0x44,0x99,0x77,0x88,0x00,0x88,0x88,0xaa,0x88,0x66,0x00,0xcc,
	0x00,0x99,0x77,0x66,0x44,0xaa,0x66,0x44,0x00,0x88,0xaa,0x77,0x66,0x55,0x44,0x77,
	0x11,0x00,0x00,0x00,0xcc,0xaa,0x88,0xcc,0x77,0xcc,0xaa,0xaa,0x88,0x66,0x44,0x00,
	0x11,0xaa,0x88,0x66,0x66,0x44,0x22,0xcc,0x77,0x66,0xbb,0x99,0x77,0x55,0x44,0x00,
	0x11,0xaa,0x77,0x55,0x99,0x99,0x55,0xbb,0x55,0x33,0x77,0x88,0x77,0x66,0x44,0x00,
	0x11,0x77,0x55,0x33,0x88,0x44,0x66,0xcc,0x55,0xff,0x99,0xaa,0x99,0x77,0x55,0x00,
	0xaa,0xaa,0x00,0x00,0xcc,0xbb,0x00,0x00,0xcc,0xbb,0x00,0x00,0xcc,0x00,0x00,0x00,
	0xcc,0x00,0x00,0x00,0xcc,0x00,0x00,0x00,0xcc,0xaa,0x00,0x00,0xaa,0x00,0x44,0x00,
	0xbb,0x33,0x66,0x00,0x55,0xbb,0xaa,0x00,0x55,0xbb,0x00,0x00,0xbb,0x55,0x88,0x00,
	0x33,0x77,0x55,0x00,0x44,0xaa,0x00,0x00,0xcc,0x00,0x33,0x00,0x88,0x44,0x66,0x00,
	/* 2D: palette green component */
	0x00,0x55,0x55,0x66,0x33,0x77,0x44,0x44,0x33,0x66,0x88,0x88,0x66,0x44,0x55,0x44,
	0x44,0x66,0x55,0x88,0x66,0x77,0x55,0x44,0x88,0x66,0xbb,0x88,0x66,0x44,0x44,0x33,
	0x88,0x66,0x55,0x66,0x44,0x77,0x44,0x55,0x00,0x99,0x66,0x44,0x66,0x44,0x55,0x99,
	0x00,0x77,0x88,0x66,0x99,0x55,0x33,0x44,0x33,0x66,0x77,0xaa,0x88,0x66,0x44,0x55,
	0x44,0x66,0x55,0x77,0x44,0x77,0x55,0x44,0x55,0x44,0x99,0x88,0x66,0x44,0x44,0x33,
	0x77,0x66,0xaa,0x77,0x66,0x55,0x44,0x55,0x00,0x55,0x33,0x22,0x00,0x66,0x44,0x99,
	0x00,0x55,0x33,0x22,0x00,0x66,0x44,0x55,0x00,0x66,0x55,0x99,0x77,0x55,0x00,0xaa,
	0x00,0x77,0x55,0x44,0x33,0x88,0x66,0x44,0x00,0x55,0xaa,0x77,0x66,0x55,0x44,0x44,
	0x11,0x99,0x66,0x44,0xaa,0x88,0x66,0xaa,0x55,0xcc,0x00,0xaa,0x88,0x66,0x44,0x00,
	0x11,0x99,0x77,0x55,0x88,0x66,0x44,0xaa,0x55,0x66,0xaa,0x99,0x77,0x55,0x44,0x00,
	0x11,0xaa,0x77,0x55,0x88,0x66,0x22,0x99,0x55,0x44,0x66,0x55,0x44,0x33,0x00,0x00,
	0x11,0x99,0x66,0x44,0x66,0x33,0x44,0xaa,0x55,0xee,0xcc,0x88,0x77,0x55,0x44,0x00,
	0x00,0xaa,0x00,0x00,0xcc,0xaa,0x00,0x00,0xcc,0x66,0x00,0x00,0xcc,0x00,0x00,0x00,
	0xcc,0x00,0x00,0x00,0xcc,0x88,0x00,0x00,0xcc,0x00,0x00,0x00,0x00,0x66,0x44,0x00,
	0x99,0x22,0x33,0x00,0x44,0xaa,0x00,0x00,0x44,0xaa,0x77,0x00,0xaa,0x44,0x66,0x00,
	0x55,0x99,0x77,0x00,0x44,0x99,0x66,0x00,0xcc,0xbb,0x22,0x00,0x88,0x44,0x66,0x00,
	/* 3D: palette blue component */
	0x00,0x33,0x77,0x44,0x00,0x88,0x00,0x55,0x00,0x44,0x66,0x55,0x22,0x00,0x33,0x33,
	0x00,0x44,0x77,0x44,0x00,0x88,0x33,0x55,0x66,0x44,0x66,0x44,0x00,0x00,0x22,0x00,
	0x66,0x44,0x33,0x33,0x00,0x55,0x33,0x33,0x00,0xaa,0x88,0x55,0x55,0x33,0x33,0x77,
	0x00,0x55,0x66,0x44,0x77,0x33,0x00,0x00,0x22,0x44,0x66,0x99,0x77,0x55,0x33,0x33,
	0x33,0x44,0x77,0x55,0x00,0x88,0x33,0x55,0x33,0x55,0x66,0x55,0x00,0x00,0x00,0x00,
	0x55,0x44,0x99,0x66,0x55,0x44,0x33,0x33,0x00,0x33,0x00,0x00,0x00,0x44,0x00,0x77,
	0x00,0x33,0x00,0x00,0x00,0x44,0x00,0x00,0x00,0x44,0x33,0x77,0x55,0x33,0x00,0x88,
	0x00,0x55,0x33,0x00,0x00,0x66,0x55,0x33,0x00,0x33,0x99,0x66,0x55,0x44,0x33,0x00,
	0x11,0xaa,0x88,0x55,0x00,0x00,0x00,0x88,0x33,0xff,0x00,0x99,0x77,0x55,0x33,0x00,
	0x11,0x77,0x55,0x33,0x88,0x55,0x33,0x88,0x33,0x55,0x00,0x88,0x66,0x44,0x33,0x00,
	0x11,0xaa,0x77,0x55,0x55,0x44,0x00,0x77,0x44,0x55,0x00,0x33,0x00,0x00,0x00,0x00,
	0x11,0x44,0x00,0x00,0x44,0x00,0x00,0x88,0x44,0xbb,0x44,0x66,0x55,0x33,0x33,0x00,
	0x00,0xaa,0x00,0x00,0xcc,0x00,0x00,0x00,0xcc,0xaa,0x00,0x00,0xcc,0xbb,0x00,0x00,
	0xcc,0xcc,0x00,0x00,0xcc,0xbb,0x00,0x00,0xcc,0x00,0x00,0x00,0x00,0xcc,0x77,0x00,
	0x77,0x00,0x00,0x00,0x00,0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00,0x00,0x00,0x00,
	0x44,0x99,0x66,0x00,0x33,0x77,0x99,0x00,0xcc,0x77,0x66,0x00,0x77,0x33,0x55,0x00,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			commando_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,16*4+4*16+16*8,
	commando_vh_convert_color_prom,

	0,
	commando_vh_start,
	commando_vh_stop,
	commando_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	c1942_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( commando_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "m09_cm04.bin", 0x0000, 0x8000 )
	ROM_LOAD( "m08_cm03.bin", 0x8000, 0x4000 )

	ROM_REGION(0x34000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d05_vt01.bin", 0x00000, 0x4000 )	/* characters */
	ROM_LOAD( "a05_vt11.bin", 0x04000, 0x4000 )	/* tiles */
	ROM_LOAD( "a07_vt13.bin", 0x08000, 0x4000 )	/* tiles */
	ROM_LOAD( "a09_vt15.bin", 0x0c000, 0x4000 )	/* tiles */
	ROM_LOAD( "a06_vt12.bin", 0x10000, 0x4000 )	/* tiles */
	ROM_LOAD( "a08_vt14.bin", 0x14000, 0x4000 )	/* tiles */
	ROM_LOAD( "a10_vt16.bin", 0x18000, 0x4000 )	/* tiles */
	ROM_LOAD( "e07_vt05.bin", 0x1c000, 0x4000 )	/* sprites */
	ROM_LOAD( "h07_vt08.bin", 0x20000, 0x4000 )	/* sprites */
	ROM_LOAD( "e08_vt06.bin", 0x24000, 0x4000 )	/* sprites */
	ROM_LOAD( "h08_vt09.bin", 0x28000, 0x4000 )	/* sprites */
	ROM_LOAD( "e09_vt07.bin", 0x2c000, 0x4000 )	/* sprites */
	ROM_LOAD( "h09_vt10.bin", 0x30000, 0x4000 )	/* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "f09_cm02.bin", 0x0000, 0x4000 )
ROM_END



static unsigned commando_decode(int A)
{
	static const unsigned char decode[] =
	{
        0x00,0x01,0x20,0x21,0x40,0x41,0x60,0x61,0x80,0x81,0xA0,0xA1,0xC0,0xC1,0xE0,0xE1,
        0x10,0x11,0x30,0x31,0x50,0x51,0x70,0x71,0x90,0x91,0xB0,0xB1,0xD0,0xD1,0xF0,0xF1,
        0x02,0x03,0x22,0x23,0x42,0x43,0x62,0x63,0x82,0x83,0xA2,0xA3,0xC2,0xC3,0xE2,0xE3,
        0x12,0x13,0x32,0x33,0x52,0x53,0x72,0x73,0x92,0x93,0xB2,0xB3,0xD2,0xD3,0xF2,0xF3,
        0x04,0x05,0x24,0x25,0x44,0x45,0x64,0x65,0x84,0x85,0xA4,0xA5,0xC4,0xC5,0xE4,0xE5,
        0x14,0x15,0x34,0x35,0x54,0x55,0x74,0x75,0x94,0x95,0xB4,0xB5,0xD4,0xD5,0xF4,0xF5,
        0x06,0x07,0x26,0x27,0x46,0x47,0x66,0x67,0x86,0x87,0xA6,0xA7,0xC6,0xC7,0xE6,0xE7,
        0x16,0x17,0x36,0x37,0x56,0x57,0x76,0x77,0x96,0x97,0xB6,0xB7,0xD6,0xD7,0xF6,0xF7,
        0x08,0x09,0x28,0x29,0x48,0x49,0x68,0x69,0x88,0x89,0xA8,0xA9,0xC8,0xC9,0xE8,0xE9,
        0x18,0x19,0x38,0x39,0x58,0x59,0x78,0x79,0x98,0x99,0xB8,0xB9,0xD8,0xD9,0xF8,0xF9,
        0x0A,0x0B,0x2A,0x2B,0x4A,0x4B,0x6A,0x6B,0x8A,0x8B,0xAA,0xAB,0xCA,0xCB,0xEA,0xEB,
        0x1A,0x1B,0x3A,0x3B,0x5A,0x5B,0x7A,0x7B,0x9A,0x9B,0xBA,0xBB,0xDA,0xDB,0xFA,0xFB,
        0x0C,0x0D,0x2C,0x2D,0x4C,0x4D,0x6C,0x6D,0x8C,0x8D,0xAC,0xAD,0xCC,0xCD,0xEC,0xED,
        0x1C,0x1D,0x3C,0x3D,0x5C,0x5D,0x7C,0x7D,0x9C,0x9D,0xBC,0xBD,0xDC,0xDD,0xFC,0xFD,
        0x0E,0x0F,0x2E,0x2F,0x4E,0x4F,0x6E,0x6F,0x8E,0x8F,0xAE,0xAF,0xCE,0xCF,0xEE,0xEF,
        0x1E,0x1F,0x3E,0x3F,0x5E,0x5F,0x7E,0x7F,0x9E,0x9F,0xBE,0xBF,0xDE,0xDF,0xFE,0xFF
	};


	if (A > 0) return decode[RAM[A]];
	else return RAM[A];
}



static int hiload(const char *name)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xee00],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0xee4e],"\x00\x08\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0xee00],1,13*7,f);
			RAM[0xee97] = RAM[0xee00];
			RAM[0xee98] = RAM[0xee01];
			RAM[0xee99] = RAM[0xee02];
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0xee00],1,13*7,f);
		fclose(f);
	}
}



struct GameDriver commando_driver =
{
	"commando",
	&machine_driver,

	commando_rom,
	0, commando_decode,
	0,

	input_ports, dsw, keys,

	color_prom, 0, 0,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0, 1,
	8*13, 8*16, 6,

	hiload, hisave
};
