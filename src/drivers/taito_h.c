/***************************************************************************

Taito H system
----------------------------
driver by Yochizo

This driver is heavily dependent on the Raine source.
Very thanks to Richard Bush and the Raine team. Also,
I have been given a lot of helpful informations by
Yasuhiro Ogawa. Thank you, Yasu.


Supported games :
==================
 Syvalion          (C) 1988 Taito
 Record Breaker    (C) 1988 Taito
 Dynamite League   (C) 1990 Taito


System specs :
===============
 CPU   : MC68000 (12 MHz) x 1, Z80 (4 MHz?, sound CPU) x 1
 Sound : YM2610, YM3016?
 OSC   : 20.000 MHz, 8.000 MHz, 24.000 MHz
 Chips : TC0070RGB (Palette?)
         TC0220IOC (Input)
         TC0140SYT (Sound communication)
         TC0130LNB (???)
         TC0160ROM (???)
         TC0080VCO (Video?)

 name             irq    resolution   tx-layer   tc0220ioc
 --------------------------------------------------------------
 Syvalion          2       512x400      used     port-based
 Record Breaker    2       320x240     unused    port-based
 Dynamite League   1       320x240     unused   address-based


Known issues :
===============
 - Y coordinate of sprite zooming is non-linear, so currently implemented
   hand-tuned value and this is used for only Record Breaker.
 - Sprite and BG1 priority bit is not understood. It is defined by sprite
   priority in Record Breaker and by zoom value and some scroll value in
   Dynamite League. So, some priority problems still remain.
 - A scroll value of text layer seems to be nonexistence.
 - Background zoom effect is not working in flip screen mode.
 - Sprite zoom is a bit wrong.
 - Title screen of dynamite league is wrong a bit, which is mentioned by
   Yasuhiro Ogawa.

TODO :
========
 - Make dipswitches clear.
 - Implemented BG1 : sprite priority. Currently it is not brought out priority
   bit.
 - Fix sprite coordinates.
 - Improve zoom y coordinate.
 - Text layer scroll if exists.
 - Speed up and clean up the code.

****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/taitosnd.h"
#include "vidhrdw/taitoic.h"		/* For input ports (TC0220IOC) */



/***************************************************************************

  Variable

***************************************************************************/

static data16_t *taitoh_68000_mainram;

extern data16_t	*taitoh_tx_videoram_0;
extern data16_t	*taitoh_tx_videoram_1;
extern data16_t	*taitoh_tx_charram;
extern data16_t *taitoh_tx_videoram_0;
extern data16_t *taitoh_tx_videoram_1;
extern data16_t	*taitoh_bg0_videoram_0;
extern data16_t	*taitoh_bg1_videoram_0;
extern data16_t	*taitoh_bg0_videoram_1;
extern data16_t	*taitoh_bg1_videoram_1;
extern data16_t	*taitoh_scrollram;
extern data16_t	*taitoh_chainram_0;
extern data16_t	*taitoh_chainram_1;
extern data16_t	*taitoh_spriteram;

READ16_HANDLER( taitoh_tx_charram_0_r );
READ16_HANDLER( taitoh_tx_charram_1_r );
READ16_HANDLER( taitoh_tx_videoram_0_r );
READ16_HANDLER( taitoh_tx_videoram_1_r );
READ16_HANDLER( taitoh_bg0_videoram_0_r );
READ16_HANDLER( taitoh_bg1_videoram_0_r );
READ16_HANDLER( taitoh_bg0_videoram_1_r );
READ16_HANDLER( taitoh_bg1_videoram_1_r );
READ16_HANDLER( taitoh_scrollram_r );

WRITE16_HANDLER( taitoh_tx_charram_0_w );
WRITE16_HANDLER( taitoh_tx_charram_1_w );
WRITE16_HANDLER( taitoh_tx_videoram_0_w );
WRITE16_HANDLER( taitoh_tx_videoram_1_w );
WRITE16_HANDLER( taitoh_bg0_videoram_0_w );
WRITE16_HANDLER( taitoh_bg1_videoram_0_w );
WRITE16_HANDLER( taitoh_bg0_videoram_1_w );
WRITE16_HANDLER( taitoh_bg1_videoram_1_w );
WRITE16_HANDLER( taitoh_scrollram_w );


int 		syvalion_vh_start(void);
int			recordbr_vh_start(void);
void 		syvalion_vh_stop (void);
void 		syvalion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void		recordbr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void		dleague_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************

  Interrupt(s)

***************************************************************************/

/* Handler called by the YM2610 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface syvalion_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};

static struct YM2610interface dleague_ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 4 MHz */
	{ 30 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) }
};


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( syvalion )
	PORT_START  /* DSW1 (0) */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW2 (1) */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* TRACKBALL1 X (4) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_CENTER, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL1 Y (5) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1 | IPF_CENTER | IPF_REVERSE, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL2 X (6) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_CENTER, 30, 30, 0, 0 )

	PORT_START  /* TRACKBALL2 Y (7) */
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 30, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( recordbr )
	PORT_START  /* DSW1 (0) */
	PORT_DIPNAME( 0x00, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW2 (1) */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 (4) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( dleague )
	PORT_START  /* DSW1 (0) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START  /* DSW2 (1) */
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x0c, 0x0c, "Innings per credit" )
	PORT_DIPSETTING(    0x0c, "3-3-3" )
	PORT_DIPSETTING(    0x08, "6-3" )
	PORT_DIPSETTING(    0x04, "3-2-2-2" )
	PORT_DIPSETTING(    0x00, "5-2-2" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN0 (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN1 (3) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 (4) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static READ16_HANDLER( taitoh_mirrorram_r )
{
	/* This is a read handler of main RAM mirror. */
	return taitoh_68000_mainram[offset];
}

static READ16_HANDLER( TC0220IOC_halfword_port_r )
{
	return TC0220IOC_port_r( offset );
}

static WRITE16_HANDLER( TC0220IOC_halfword_port_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_port_w( offset, data & 0xff );
}

static READ16_HANDLER( TC0220IOC_halfword_portreg_r )
{
	return TC0220IOC_portreg_r( offset );
}

static WRITE16_HANDLER( TC0220IOC_halfword_portreg_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_portreg_w( offset, data & 0xff );
}

static READ16_HANDLER( TC0220IOC_halfword_r )
{
	return TC0220IOC_r( offset );
}

static WRITE16_HANDLER( TC0220IOC_halfword_w )
{
	if (ACCESSING_LSB)
		TC0220IOC_w( offset, data & 0xff );
}

static READ16_HANDLER( syvalion_input_bypass_r )
{
	/* Bypass TC0220IOC controller for analog input */

	data8_t	port = TC0220IOC_halfword_port_r(0);	/* read port number */

	switch( port )
	{
		case 0x08:				/* trackball y coords bottom 8 bits for 2nd player */
			return input_port_7_r(0);

		case 0x09:				/* trackball y coords top 8 bits for 2nd player */
			if (input_port_7_r(0) & 0x80)	/* y- direction (negative value) */
				return 0xff;
			else							/* y+ direction (positive value) */
				return 0x00;

		case 0x0a:				/* trackball x coords bottom 8 bits for 2nd player */
			return input_port_6_r(0);

		case 0x0b:				/* trackball x coords top 8 bits for 2nd player */
			if (input_port_6_r(0) & 0x80)	/* x- direction (negative value) */
				return 0xff;
			else							/* x+ direction (positive value) */
				return 0x00;

		case 0x0c:				/* trackball y coords bottom 8 bits for 2nd player */
			return input_port_5_r(0);

		case 0x0d:				/* trackball y coords top 8 bits for 1st player */
			if (input_port_5_r(0) & 0x80)	/* y- direction (negative value) */
				return 0xff;
			else							/* y+ direction (positive value) */
				return 0x00;

		case 0x0e:				/* trackball x coords bottom 8 bits for 1st player */
			return input_port_4_r(0);

		case 0x0f:				/* trackball x coords top 8 bits for 1st player */
			if (input_port_4_r(0) & 0x80)	/* x- direction (negative value) */
				return 0xff;
			else							/* x+ direction (positive value) */
				return 0x00;

		default:
			return TC0220IOC_portreg_r( offset );
	}
}

static WRITE_HANDLER( taitoh_sound_bankswitch_w )
{
	data8_t *RAM = (data8_t *)memory_region(REGION_CPU2);

	cpu_setbank( 1, &RAM[ 0x10000 + ( ((data - 1) & 3) * 0x4000 ) ] );
}


/***************************************************************************

  Memory Map(s)

***************************************************************************/

static MEMORY_READ16_START( syvalion_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x200001, syvalion_input_bypass_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x400fff, taitoh_tx_charram_0_r },
	{ 0x401000, 0x401fff, taitoh_tx_videoram_0_r },
	{ 0x402000, 0x40bfff, MRA16_RAM },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_r },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_r },
	{ 0x410000, 0x410fff, taitoh_tx_charram_1_r },
	{ 0x411000, 0x411fff, taitoh_tx_videoram_1_r },
	{ 0x412000, 0x41bfff, MRA16_RAM },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_r },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_r },
	{ 0x420400, 0x4207ff, MRA16_RAM },
	{ 0x420800, 0x42080f, taitoh_scrollram_r },
	{ 0x500800, 0x500c1f, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( syvalion_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x400fff, taitoh_tx_charram_0_w },
	{ 0x401000, 0x401fff, taitoh_tx_videoram_0_w, &taitoh_tx_videoram_0 },
	{ 0x402000, 0x40bfff, MWA16_RAM, &taitoh_chainram_0 },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_w, &taitoh_bg0_videoram_0 },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_w, &taitoh_bg1_videoram_0 },
	{ 0x410000, 0x410fff, taitoh_tx_charram_1_w },
	{ 0x411000, 0x411fff, taitoh_tx_videoram_1_w, &taitoh_tx_videoram_1 },
	{ 0x412000, 0x41bfff, MWA16_RAM, &taitoh_chainram_1 },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_w, &taitoh_bg0_videoram_1 },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_w, &taitoh_bg1_videoram_1 },
	{ 0x420000, 0x4203ff, MWA16_NOP },
	{ 0x420400, 0x4207ff, MWA16_RAM, &taitoh_spriteram },
	{ 0x420800, 0x42080f, taitoh_scrollram_w, &taitoh_scrollram },
	{ 0x500800, 0x500c1f, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( recordbr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_r },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x40bfff, MRA16_RAM },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_r },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_r },
	{ 0x410000, 0x41bfff, MRA16_RAM },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_r },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_r },
	{ 0x420400, 0x4207ff, MRA16_RAM },
	{ 0x420800, 0x42080f, taitoh_scrollram_r },
	{ 0x500800, 0x500bff, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( recordbr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x200001, TC0220IOC_halfword_portreg_w },
	{ 0x200002, 0x200003, TC0220IOC_halfword_port_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x40bfff, MWA16_RAM, &taitoh_chainram_0 },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_w, &taitoh_bg0_videoram_0 },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_w, &taitoh_bg1_videoram_0 },
	{ 0x410000, 0x41bfff, MWA16_RAM, &taitoh_chainram_1 },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_w, &taitoh_bg0_videoram_1 },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_w, &taitoh_bg1_videoram_1 },
	{ 0x420000, 0x4203ff, MWA16_NOP },
	{ 0x420400, 0x4207ff, MWA16_RAM, &taitoh_spriteram },
	{ 0x420800, 0x42080f, taitoh_scrollram_w, &taitoh_scrollram },
	{ 0x500800, 0x500bff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( dleague_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },				/* 68000 RAM */
	{ 0x110000, 0x11ffff, taitoh_mirrorram_r },		/* 68000 RAM (Mirror) */
	{ 0x200000, 0x20000f, TC0220IOC_halfword_r },
	{ 0x300000, 0x300001, MRA16_NOP },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x40bfff, MRA16_RAM },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_r },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_r },
	{ 0x410000, 0x41bfff, MRA16_RAM },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_r },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_r },
	{ 0x420400, 0x4207ff, MRA16_RAM },
	{ 0x420800, 0x42080f, taitoh_scrollram_r },
	{ 0x500800, 0x500c1f, paletteram16_word_r },
MEMORY_END

static MEMORY_WRITE16_START( dleague_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &taitoh_68000_mainram },
	{ 0x200000, 0x20000f, TC0220IOC_halfword_w },
	{ 0x300000, 0x300001, taitosound_port16_lsb_w },
	{ 0x300002, 0x300003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x40bfff, MWA16_RAM, &taitoh_chainram_0 },
	{ 0x40c000, 0x40dfff, taitoh_bg0_videoram_0_w, &taitoh_bg0_videoram_0 },
	{ 0x40e000, 0x40ffff, taitoh_bg1_videoram_0_w, &taitoh_bg1_videoram_0 },
	{ 0x410000, 0x41bfff, MWA16_RAM, &taitoh_chainram_1 },
	{ 0x41c000, 0x41dfff, taitoh_bg0_videoram_1_w, &taitoh_bg0_videoram_1 },
	{ 0x41e000, 0x41ffff, taitoh_bg1_videoram_1_w, &taitoh_bg1_videoram_1 },
	{ 0x420000, 0x4203ff, MWA16_NOP },
	{ 0x420400, 0x4207ff, MWA16_RAM, &taitoh_spriteram },
	{ 0x420800, 0x42080f, taitoh_scrollram_w, &taitoh_scrollram },
	{ 0x500800, 0x500c1f, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
	{ 0xea00, 0xea00, MRA_NOP },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP },		/* pan control */
	{ 0xee00, 0xee00, MWA_NOP }, 		/* ? */
	{ 0xf000, 0xf000, MWA_NOP }, 		/* ? */
	{ 0xf200, 0xf200, taitoh_sound_bankswitch_w },
MEMORY_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout tilelayout =
{
	16,16,	/* 16x16 pixels */
	32768,	/* 32768 tiles */
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 0x100000*8+4, 0x100000*8, 0x100000*8+12, 0x100000*8+8,
	    0x200000*8+4, 0x200000*8, 0x200000*8+12, 0x200000*8+8, 0x300000*8+4, 0x300000*8, 0x300000*8+12, 0x300000*8+8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

/* for Syvalion */
static struct GfxLayout charlayout =
{
	8, 8,	/* 8x8 pixels */
	256,	/* 256 chars */
	4,		/* 4 bit per pixel */
	{ 0x1000*8 + 8, 0x1000*8, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8
};


static struct GfxDecodeInfo syvalion_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ 0,           0, &charlayout, 32*16, 16    },	/* this is dynamically changed */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo recordbr_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo dleague_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0,     32*16 },
	{ REGION_GFX2, 0, &charlayout, 32*16, 16    },
	{ -1 } /* end of array */
};


static const struct MachineDriver machine_driver_syvalion =
{
	{
		{
			CPU_M68000,
			24000000 / 2,		/* 12 MHz */
			syvalion_readmem, syvalion_writemem, 0, 0,
			m68_level2_irq, 1
		},
		{
			CPU_Z80,
			8000000 / 2,		/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	10,
	0,

	/* video hardware */
	64*16, 64*16, { 0*16, 32*16-1, 3*16, 28*16-1 },
	syvalion_gfxdecodeinfo,
	33*16, 33*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	syvalion_vh_start,
	syvalion_vh_stop,
	syvalion_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2610,
			&syvalion_ym2610_interface
		},
	}
};

static const struct MachineDriver machine_driver_recordbr =
{
	{
		{
			CPU_M68000,
			24000000 / 2,		/* 12 MHz */
			recordbr_readmem, recordbr_writemem, 0, 0,
			m68_level2_irq, 1
		},
		{
			CPU_Z80,
			8000000 / 2,		/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	10,
	0,

	/* video hardware */
	64*16, 64*16, { 1*16, 21*16-1, 2*16, 17*16-1 },
	recordbr_gfxdecodeinfo,
	32*16, 32*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	recordbr_vh_start,
	0,
	recordbr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2610,
			&syvalion_ym2610_interface
		},
	}
};

static const struct MachineDriver machine_driver_dleague =
{
	{
		{
			CPU_M68000,
			24000000 / 2,		/* 12 MHz */
			dleague_readmem, dleague_writemem, 0, 0,
			m68_level1_irq, 1
		},
		{
			CPU_Z80,
			8000000 / 2,		/* 4 MHz ??? */
			sound_readmem, sound_writemem, 0, 0,
			ignore_interrupt, 0
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	10,
	0,

	/* video hardware */
	64*16, 64*16, { 1*16, 21*16-1, 2*16, 17*16-1 },
	dleague_gfxdecodeinfo,
	33*16, 33*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	recordbr_vh_start,
	0,
	dleague_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2610,
			&dleague_ym2610_interface
		},
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( syvalion )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* main cpu */
	ROM_LOAD16_BYTE( "b51-20.bin", 0x00000, 0x20000, 0x440b6418 )
	ROM_LOAD16_BYTE( "b51-22.bin", 0x00001, 0x20000, 0xe6c61079 )
	ROM_LOAD16_BYTE( "b51-19.bin", 0x40000, 0x20000, 0x2abd762c )
	ROM_LOAD16_BYTE( "b51-21.bin", 0x40001, 0x20000, 0xaa111f30 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )		/* sound cpu */
	ROM_LOAD( "b51-23.bin", 0x00000, 0x04000, 0x734662de )
	ROM_CONTINUE(           0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b51-16.bin", 0x000000, 0x20000, 0xc0fcf7a5 )
	ROM_LOAD16_BYTE( "b51-12.bin", 0x000001, 0x20000, 0x6b36d358 )
	ROM_LOAD16_BYTE( "b51-15.bin", 0x040000, 0x20000, 0x30b2ee02 )
	ROM_LOAD16_BYTE( "b51-11.bin", 0x040001, 0x20000, 0xae9a9ac5 )
	ROM_LOAD16_BYTE( "b51-08.bin", 0x100000, 0x20000, 0x9f6a535c )
	ROM_LOAD16_BYTE( "b51-04.bin", 0x100001, 0x20000, 0x03aea658 )
	ROM_LOAD16_BYTE( "b51-07.bin", 0x140000, 0x20000, 0x764d4dc8 )
	ROM_LOAD16_BYTE( "b51-03.bin", 0x140001, 0x20000, 0x8fd9b299 )
	ROM_LOAD16_BYTE( "b51-14.bin", 0x200000, 0x20000, 0xdea7216e )
	ROM_LOAD16_BYTE( "b51-10.bin", 0x200001, 0x20000, 0x6aa97fbc )
	ROM_LOAD16_BYTE( "b51-13.bin", 0x240000, 0x20000, 0xdab28958 )
	ROM_LOAD16_BYTE( "b51-09.bin", 0x240001, 0x20000, 0xcbb4f33d )
	ROM_LOAD16_BYTE( "b51-06.bin", 0x300000, 0x20000, 0x81bef4f0 )
	ROM_LOAD16_BYTE( "b51-02.bin", 0x300001, 0x20000, 0x906ba440 )
	ROM_LOAD16_BYTE( "b51-05.bin", 0x340000, 0x20000, 0x47976ae9 )
	ROM_LOAD16_BYTE( "b51-01.bin", 0x340001, 0x20000, 0x8dab004a )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b51-18.bin", 0x00000, 0x80000, 0x8b23ac83 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "b51-17.bin", 0x00000, 0x80000, 0xd85096aa )
ROM_END

ROM_START( recordbr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* main cpu */
	ROM_LOAD16_BYTE( "b56-17.rom", 0x00000, 0x20000, 0x3e0a9c35 )
	ROM_LOAD16_BYTE( "b56-16.rom", 0x00001, 0x20000, 0xb447f12c )
	ROM_LOAD16_BYTE( "b56-15.rom", 0x40000, 0x20000, 0xb346e282 )
	ROM_LOAD16_BYTE( "b56-21.rom", 0x40001, 0x20000, 0xe5f63790 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )		/* sound cpu */
	ROM_LOAD( "b56-19.rom", 0x00000, 0x04000, 0xc68085ee )
	ROM_CONTINUE(           0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "b56-04",     0x000000, 0x20000, 0xf7afdff0 )
	ROM_LOAD16_BYTE( "b56-08",     0x000001, 0x20000, 0xc9f0d38a )
	ROM_LOAD16_BYTE( "b56-03",     0x100000, 0x20000, 0x4045fd44 )
	ROM_LOAD16_BYTE( "b56-07",     0x100001, 0x20000, 0x0c76e4c8 )
	ROM_LOAD16_BYTE( "b56-02",     0x200000, 0x20000, 0x68c604ec )
	ROM_LOAD16_BYTE( "b56-06",     0x200001, 0x20000, 0x5fbcd302 )
	ROM_LOAD16_BYTE( "b56-01.rom", 0x300000, 0x20000, 0x766b7260 )
	ROM_LOAD16_BYTE( "b56-05.rom", 0x300001, 0x20000, 0xed390378 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "b56-09.bin", 0x00000, 0x80000, 0x7fd9ee68 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* samples */
	ROM_LOAD( "b56-10.bin", 0x00000, 0x80000, 0xde1bce59 )
ROM_END

ROM_START( dleague )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "c02-19a.33", 0x00000, 0x20000, 0x7e904e45 )
	ROM_LOAD16_BYTE( "c02-21a.36", 0x00001, 0x20000, 0x18c8a32b )
	ROM_LOAD16_BYTE( "c02-20.34",  0x40000, 0x10000, 0xcdf593f3 )
	ROM_LOAD16_BYTE( "c02-22.37",  0x40001, 0x10000, 0xf50db2d7 )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )
	ROM_LOAD( "c02-23.40", 0x00000, 0x04000, 0x5632ee49 )
	ROM_CONTINUE(          0x10000, 0x0c000 )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD       ( "c02-02.15", 0x000000, 0x80000, 0xb273f854 )
	ROM_LOAD16_BYTE( "c02-06.6",  0x080000, 0x20000, 0xb8473c7b )
	ROM_LOAD16_BYTE( "c02-10.14", 0x080001, 0x20000, 0x50c02f0f )
	ROM_LOAD       ( "c02-03.17", 0x100000, 0x80000, 0xc3fd0dcd )
	ROM_LOAD16_BYTE( "c02-07.7",  0x180000, 0x20000, 0x8c1e3296 )
	ROM_LOAD16_BYTE( "c02-11.16", 0x180001, 0x20000, 0xfbe548b8 )
	ROM_LOAD       ( "c02-24.19", 0x200000, 0x80000, 0x18ef740a )
	ROM_LOAD16_BYTE( "c02-08.8",  0x280000, 0x20000, 0x1a3c2f93 )
	ROM_LOAD16_BYTE( "c02-12.18", 0x280001, 0x20000, 0xb1c151c5 )
	ROM_LOAD       ( "c02-05.21", 0x300000, 0x80000, 0xfe3a5179 )
	ROM_LOAD16_BYTE( "c02-09.9",  0x380000, 0x20000, 0xa614d234 )
	ROM_LOAD16_BYTE( "c02-13.20", 0x380001, 0x20000, 0x8eb3194d )

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c02-18.22", 0x00000, 0x02000, 0xc88f0bbe )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "c02-01.31", 0x00000, 0x80000, 0xd5a3d1aa )
ROM_END

/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT     MONITOR  COMPANY  FULLNAME */
GAME( 1988, syvalion, 0,        syvalion, syvalion, 0,       ROT0,    "Taito Corporation", "Syvalion (Japan)" )
GAME( 1988, recordbr, 0,        recordbr, recordbr, 0,       ROT0,    "Taito Corporation Japan", "Recordbreaker (World)" )
GAME( 1990, dleague,  0,        dleague,  dleague,  0,       ROT0,    "Taito Corporation", "Dynamite League (Japan)" )