#define __INLINE__ static __inline__    /* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include "TwkUser.c"
#include <math.h>
#include "vgafreq.h"
#include "vidhrdw/vector.h"
#include "dirty.h"
/*extra functions for 15.75KHz modes */
#include "gen15khz.h"

/*15.75KHz SVGA driver (req. for 15.75KHz Arcade Monitor Modes)*/
SVGA15KHZDRIVER *SVGA15KHzdriver;

BEGIN_GFX_DRIVER_LIST
	GFX_DRIVER_VGA
	GFX_DRIVER_VESA3
	GFX_DRIVER_VESA2L
	GFX_DRIVER_VESA2B
	GFX_DRIVER_VESA1
END_GFX_DRIVER_LIST

BEGIN_COLOR_DEPTH_LIST
	COLOR_DEPTH_8
	COLOR_DEPTH_15
	COLOR_DEPTH_16
END_COLOR_DEPTH_LIST

#define BACKGROUND 0


dirtygrid grid1;
dirtygrid grid2;
char *dirty_old=grid1;
char *dirty_new=grid2;

void scale_vectorgames(int gfx_width,int gfx_height,int *width,int *height);



/* in msdos/sound.c */
int msdos_update_audio(void);


extern int use_profiler;
void osd_profiler_display(void);


/* specialized update_screen functions defined in blit.c */

/* dirty mode 1 (VIDEO_SUPPORTS_DIRTY) */
void blitscreen_dirty1_vga(void);
void blitscreen_dirty1_unchained_vga(void);
void blitscreen_dirty1_vesa_1x_1x_8bpp(void);
void blitscreen_dirty1_vesa_1x_2x_8bpp(void);
void blitscreen_dirty1_vesa_1x_2xs_8bpp(void);
void blitscreen_dirty1_vesa_2x_1x_8bpp(void);
void blitscreen_dirty1_vesa_2x_2x_8bpp(void);
void blitscreen_dirty1_vesa_2x_2xs_8bpp(void);
void blitscreen_dirty1_vesa_2x_3x_8bpp(void);
void blitscreen_dirty1_vesa_2x_3xs_8bpp(void);
void blitscreen_dirty1_vesa_3x_2x_8bpp(void);
void blitscreen_dirty1_vesa_3x_2xs_8bpp(void);
void blitscreen_dirty1_vesa_3x_3x_8bpp(void);
void blitscreen_dirty1_vesa_3x_3xs_8bpp(void);
void blitscreen_dirty1_vesa_4x_2x_8bpp(void);
void blitscreen_dirty1_vesa_4x_2xs_8bpp(void);
void blitscreen_dirty1_vesa_4x_3x_8bpp(void);
void blitscreen_dirty1_vesa_4x_3xs_8bpp(void);

void blitscreen_dirty1_vesa_1x_1x_16bpp(void);
void blitscreen_dirty1_vesa_2x_2x_16bpp(void);
void blitscreen_dirty1_vesa_2x_2xs_16bpp(void);
void blitscreen_dirty1_vesa_1x_2x_16bpp(void);
void blitscreen_dirty1_vesa_1x_2xs_16bpp(void);

/* dirty mode 0 (no osd_mark_dirty calls) */
void blitscreen_dirty0_vga(void);
void blitscreen_dirty0_unchained_vga(void);
void blitscreen_dirty0_vesa_1x_1x_8bpp(void);
void blitscreen_dirty0_vesa_1x_2x_8bpp(void);
void blitscreen_dirty0_vesa_1x_2xs_8bpp(void);
void blitscreen_dirty0_vesa_2x_1x_8bpp(void);
void blitscreen_dirty0_vesa_2x_2x_8bpp(void);
void blitscreen_dirty0_vesa_2x_2xs_8bpp(void);
void blitscreen_dirty0_vesa_2x_3x_8bpp(void);
void blitscreen_dirty0_vesa_2x_3xs_8bpp(void);
void blitscreen_dirty0_vesa_3x_2x_8bpp(void);
void blitscreen_dirty0_vesa_3x_2xs_8bpp(void);
void blitscreen_dirty0_vesa_3x_3x_8bpp(void);
void blitscreen_dirty0_vesa_3x_3xs_8bpp(void);
void blitscreen_dirty0_vesa_4x_2x_8bpp(void);
void blitscreen_dirty0_vesa_4x_2xs_8bpp(void);
void blitscreen_dirty0_vesa_4x_3x_8bpp(void);
void blitscreen_dirty0_vesa_4x_3xs_8bpp(void);

void blitscreen_dirty0_vesa_1x_1x_16bpp(void);
void blitscreen_dirty0_vesa_1x_2x_16bpp(void);
void blitscreen_dirty0_vesa_1x_2xs_16bpp(void);
void blitscreen_dirty0_vesa_2x_2x_16bpp(void);
void blitscreen_dirty0_vesa_2x_2xs_16bpp(void);

static void update_screen_dummy(void);
void (*update_screen)(void) = update_screen_dummy;

#define MAX_X_MULTIPLY 4
#define MAX_Y_MULTIPLY 3
#define MAX_X_MULTIPLY16 2
#define MAX_Y_MULTIPLY16 2

static void (*updaters8[MAX_X_MULTIPLY][MAX_Y_MULTIPLY][2][2])(void) =
{			/* 1 x 1 */
	{	{	{ blitscreen_dirty0_vesa_1x_1x_8bpp, blitscreen_dirty1_vesa_1x_1x_8bpp },
			{ blitscreen_dirty0_vesa_1x_1x_8bpp, blitscreen_dirty1_vesa_1x_1x_8bpp }
		},	/* 1 x 2 */
		{	{ blitscreen_dirty0_vesa_1x_2x_8bpp,  blitscreen_dirty1_vesa_1x_2x_8bpp },
			{ blitscreen_dirty0_vesa_1x_2xs_8bpp, blitscreen_dirty1_vesa_1x_2xs_8bpp }
		},	/* 1 x 3 */
		{	{ update_screen_dummy, update_screen_dummy },
			{ update_screen_dummy, update_screen_dummy },
		}
	},		/* 2 x 1 */
	{	{	{ blitscreen_dirty0_vesa_2x_1x_8bpp, blitscreen_dirty1_vesa_2x_1x_8bpp },
			{ blitscreen_dirty0_vesa_2x_1x_8bpp, blitscreen_dirty1_vesa_2x_1x_8bpp }
		},	/* 2 x 2 */
		{	{ blitscreen_dirty0_vesa_2x_2x_8bpp,  blitscreen_dirty1_vesa_2x_2x_8bpp },
			{ blitscreen_dirty0_vesa_2x_2xs_8bpp, blitscreen_dirty1_vesa_2x_2xs_8bpp }
		},	/* 2 x 3 */
		{	{ blitscreen_dirty0_vesa_2x_3x_8bpp,  blitscreen_dirty1_vesa_2x_3x_8bpp },
			{ blitscreen_dirty0_vesa_2x_3xs_8bpp, blitscreen_dirty1_vesa_2x_3xs_8bpp }
		}
	},		/* 3 x 1 */
	{	{	{ update_screen_dummy, update_screen_dummy },
			{ update_screen_dummy, update_screen_dummy }
		},	/* 3 x 2 */
		{	{ blitscreen_dirty0_vesa_3x_2x_8bpp,  blitscreen_dirty1_vesa_3x_2x_8bpp },
			{ blitscreen_dirty0_vesa_3x_2xs_8bpp, blitscreen_dirty1_vesa_3x_2xs_8bpp }
		},	/* 3 x 3 */
		{	{ blitscreen_dirty0_vesa_3x_3x_8bpp,  blitscreen_dirty1_vesa_3x_3x_8bpp },
			{ blitscreen_dirty0_vesa_3x_3xs_8bpp, blitscreen_dirty1_vesa_3x_3xs_8bpp }
		}
	},		/* 4 x 1 */
	{	{	{ update_screen_dummy, update_screen_dummy },
			{ update_screen_dummy, update_screen_dummy }
		},	/* 4 x 2 */
		{	{ blitscreen_dirty0_vesa_4x_2x_8bpp,  blitscreen_dirty1_vesa_4x_2x_8bpp },
			{ blitscreen_dirty0_vesa_4x_2xs_8bpp, blitscreen_dirty1_vesa_4x_2xs_8bpp }
		},	/* 4 x 3 */
		{	{ blitscreen_dirty0_vesa_4x_3x_8bpp,  blitscreen_dirty1_vesa_4x_3x_8bpp },
			{ blitscreen_dirty0_vesa_4x_3xs_8bpp, blitscreen_dirty1_vesa_4x_3xs_8bpp }
		}
	}
};

static void (*updaters16[MAX_X_MULTIPLY16][MAX_Y_MULTIPLY16][2][2])(void) =
{			/* 1 x 1 */
	{	{	{ blitscreen_dirty0_vesa_1x_1x_16bpp, blitscreen_dirty1_vesa_1x_1x_16bpp },
			{ blitscreen_dirty0_vesa_1x_1x_16bpp, blitscreen_dirty1_vesa_1x_1x_16bpp }
		},	/* 1 x 2 */
		{	{ blitscreen_dirty0_vesa_1x_2x_16bpp,  blitscreen_dirty1_vesa_1x_2x_16bpp },
			{ blitscreen_dirty0_vesa_1x_2xs_16bpp, blitscreen_dirty1_vesa_1x_2xs_16bpp }
		}
	},		/* 2 x 1 */
	{	{	{ update_screen_dummy, update_screen_dummy },
			{ update_screen_dummy, update_screen_dummy }
		},	/* 2 x 2 */
		{	{ blitscreen_dirty0_vesa_2x_2x_16bpp,  blitscreen_dirty1_vesa_2x_2x_16bpp },
			{ blitscreen_dirty0_vesa_2x_2xs_16bpp, blitscreen_dirty1_vesa_2x_2xs_16bpp }
		}
	}
};



struct osd_bitmap *scrbitmap;
static unsigned char current_palette[256][3];
static PALETTE adjusted_palette;
static unsigned char dirtycolor[256];
static int dirtypalette;
extern unsigned int doublepixel[256];
extern unsigned int quadpixel[256]; /* for quadring pixels */

int frameskip,autoframeskip;
#define FRAMESKIP_LEVELS 12

/* type of monitor output- */
/* Standard PC, NTSC, PAL or Arcade */
int monitor_type;

int vgafreq;
int always_synced;
int video_sync;
int wait_vsync;
int use_triplebuf;
int triplebuf_pos,triplebuf_page_width;
int vsync_frame_rate;
int color_depth;
int skiplines;
int skipcolumns;
int scanlines;
int stretch;
int use_tweaked;
int use_vesa;
float osd_gamma_correction = 1.0;
int brightness;
float brightness_paused_adjust;
char *resolution;
char *mode_desc;
int gfx_mode;
int gfx_width;
int gfx_height;
int tw256x224_hor;

/*new 'half' flag (req. for 15.75KHz Arcade Monitor Modes)*/
int half_yres=0;
/* indicates unchained video mode (req. for 15.75KHz Arcade Monitor Modes)*/
int unchained;
/* flags for lowscanrate modes */
int scanrate15KHz;

static int auto_resolution;
static int viswidth;
static int visheight;
static int skiplinesmax;
static int skipcolumnsmax;
static int skiplinesmin;
static int skipcolumnsmin;

static int vector_game;
static int use_dirty;

static Register *reg = 0;       /* for VGA modes */
static int reglen = 0;  /* for VGA modes */
static int videofreq;   /* for VGA modes */

int gfx_xoffset;
int gfx_yoffset;
int gfx_display_lines;
int gfx_display_columns;
static int xmultiply,ymultiply;
int throttle = 1;       /* toggled by F10 */

static int gone_to_gfx_mode;
static int frameskip_counter;
static int frames_displayed;
static uclock_t start_time,end_time;    /* to calculate fps average on exit */
#define FRAMES_TO_SKIP 20       /* skip the first few frames from the FPS calculation */
							/* to avoid counting the copyright and info screens */

unsigned char tw224x288ns_h, tw224x288ns_v, tw224x288sc_h, tw224x288sc_v;
unsigned char tw256x256ns_h, tw256x256ns_v, tw256x256sc_h, tw256x256sc_v;
unsigned char tw256x256ns_hor_h, tw256x256ns_hor_v, tw256x256sc_hor_h, tw256x256sc_hor_v;
unsigned char tw256x256ns_57_h, tw256x256ns_57_v, tw256x256sc_57_h, tw256x256sc_57_v;
unsigned char tw256x256ns_h57_h, tw256x256ns_h57_v, tw256x256sc_h57_h, tw256x256sc_h57_v;
unsigned char tw288x224ns_h, tw288x224ns_v, tw288x224sc_h, tw288x224sc_v;
unsigned char tw320x240ns_h, tw320x240ns_v, tw320x240sc_h, tw320x240sc_v;
unsigned char tw336x240ns_h, tw336x240ns_v, tw336x240sc_h, tw336x240sc_v;
unsigned char tw384x240ns_h, tw384x240ns_v, tw384x240sc_h, tw384x240sc_v;
unsigned char tw384x256ns_h, tw384x256ns_v, tw384x256sc_h, tw384x256sc_v;

struct vga_tweak { int x, y; Register *reg; int reglen; int syncvgafreq; int scanlines;
					int unchained; };
struct vga_tweak vga_tweaked[] = {
	{ 256, 256, scr256x256scanlines, sizeof(scr256x256scanlines)/sizeof(Register), 1, 1, 0 },
	{ 288, 224, scr288x224scanlines, sizeof(scr288x224scanlines)/sizeof(Register), 0, 1, 0 },
	{ 224, 288, scr224x288scanlines, sizeof(scr224x288scanlines)/sizeof(Register), 2, 1, 0 },
/* 320x204 runs at 70Hz - to get a 60Hz modes, timings would be the same as
 the 320x240 mode - just more blanking, so might as well just use that */
//	{ 320, 204, scr320x204, sizeof(scr320x204)/sizeof(Register), -1, 0, 0 },
	{ 256, 256, scr256x256, sizeof(scr256x256)/sizeof(Register),  1, 0, 0 },
	{ 288, 224, scr288x224, sizeof(scr288x224)/sizeof(Register),  0, 0, 0 },
	{ 224, 288, scr224x288, sizeof(scr224x288)/sizeof(Register),  1, 0, 0 },
	{ 200, 320, scr200x320, sizeof(scr200x320)/sizeof(Register), -1, 0, 0 },
	{ 320, 240, scr320x240scanlines, sizeof(scr320x240scanlines)/sizeof(Register),  0, 1, 1 },
	{ 320, 240, scr320x240, sizeof(scr320x240)/sizeof(Register),  0, 0, 1 },
	{ 336, 240, scr336x240scanlines, sizeof(scr336x240scanlines)/sizeof(Register),  0, 1, 1 },
	{ 336, 240, scr336x240, sizeof(scr336x240)/sizeof(Register),  0, 0, 1 },
	{ 384, 240, scr384x240scanlines, sizeof(scr384x240scanlines)/sizeof(Register),  1, 1, 1 },
	{ 384, 240, scr384x240, sizeof(scr384x240)/sizeof(Register),  1, 0, 1 },
	{ 384, 256, scr384x256scanlines, sizeof(scr384x256scanlines)/sizeof(Register),  1, 1, 1 },
	{ 384, 256, scr384x256, sizeof(scr384x256)/sizeof(Register),  1, 0, 1 },
	{ 0, 0 }
};

/* Tweak values for arcade/ntsc/pal modes */
unsigned char tw224x288arc_h, tw224x288arc_v, tw288x224arc_h, tw288x224arc_v;
unsigned char tw256x240arc_h, tw256x240arc_v, tw256x256arc_h, tw256x256arc_v;
unsigned char tw320x240arc_h, tw320x240arc_v, tw320x256arc_h, tw320x256arc_v;
unsigned char tw352x240arc_h, tw352x240arc_v, tw352x256arc_h, tw352x256arc_v;
unsigned char tw368x240arc_h, tw368x240arc_v, tw368x256arc_h, tw368x256arc_v;
unsigned char tw512x224arc_h, tw512x224arc_v, tw512x256arc_h, tw512x256arc_v;
unsigned char tw512x448arc_h, tw512x448arc_v, tw512x512arc_h, tw512x512arc_v;

/* 15.75KHz Modes */
struct vga_15KHz_tweak { int x, y; Register *reg; int reglen;
                          int syncvgafreq; int vesa; int ntsc;
                          int half_yres; int matchx; };
struct vga_15KHz_tweak arcade_tweaked[] = {
	{ 224, 288, scr224x288_15KHz, sizeof(scr224x288_15KHz)/sizeof(Register), 0, 0, 0, 0, 224 },
	{ 256, 240, scr256x240_15KHz, sizeof(scr256x240_15KHz)/sizeof(Register), 0, 0, 1, 0, 256 },
	{ 256, 256, scr256x256_15KHz, sizeof(scr256x256_15KHz)/sizeof(Register), 0, 0, 0, 0, 256 },
	{ 288, 224, scr288x224_NTSC,  sizeof(scr288x224_NTSC) /sizeof(Register), 0, 0, 1, 0, 288 },
	{ 320, 240, scr320x240_15KHz, sizeof(scr320x240_15KHz)/sizeof(Register), 1, 0, 1, 0, 320 },
	{ 320, 256, scr320x256_15KHz, sizeof(scr320x256_15KHz)/sizeof(Register), 1, 0, 0, 0, 320 },
	{ 352, 240, scr352x240_15KHz, sizeof(scr352x240_15KHz)/sizeof(Register), 1, 0, 1, 0, 352 },
	{ 352, 256, scr352x256_15KHz, sizeof(scr352x256_15KHz)/sizeof(Register), 1, 0, 0, 0, 352 },
/* force 384 games to match to 368 modes - the standard VGA clock speeds mean we can't go as wide as 384 */
	{ 368, 240, scr368x240_15KHz, sizeof(scr368x240_15KHz)/sizeof(Register), 1, 0, 1, 0, 384 },
	{ 368, 256, scr368x256_15KHz, sizeof(scr368x256_15KHz)/sizeof(Register), 1, 0, 0, 0, 384 },
/* double monitor modes */
	{ 512, 224, scr512x224_15KHz, sizeof(scr512x224_15KHz)/sizeof(Register), 0, 0, 1, 0, 512 },
	{ 512, 256, scr512x256_15KHz, sizeof(scr512x256_15KHz)/sizeof(Register), 0, 0, 0, 0, 512 },
/* SVGA Mode (VGA register array not used) */
	{ 640, 480, NULL            , 0                                        , 0, 1, 1, 0, 640 },
/* 'half y' VGA modes, used to fake hires if 'tweaked' is on */
	{ 512, 448, scr512x224_15KHz, sizeof(scr512x224_15KHz)/sizeof(Register), 0, 0, 1, 1, 512 },
	{ 512, 512, scr512x256_15KHz, sizeof(scr512x256_15KHz)/sizeof(Register), 0, 0, 0, 1, 512 },
	{ 0, 0 }
};

/* Create a bitmap. Also calls osd_clearbitmap() to appropriately initialize */
/* it to the background color. */
/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */

struct osd_bitmap *osd_new_bitmap(int width,int height,int depth)       /* ASG 980209 */
{
	struct osd_bitmap *bitmap;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	if ((bitmap = malloc(sizeof(struct osd_bitmap))) != 0)
	{
		int i,rowlen,rdwidth;
		unsigned char *bm;
		int safety;


		if (width > 64) safety = 16;
		else safety = 0;        /* don't create the safety area for GfxElement bitmaps */

		if (depth != 8 && depth != 16) depth = 8;

		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		rdwidth = (width + 7) & ~7;     /* round width to a quadword */
		if (depth == 16)
			rowlen = 2 * (rdwidth + 2 * safety) * sizeof(unsigned char);
		else
			rowlen =     (rdwidth + 2 * safety) * sizeof(unsigned char);

		if ((bm = malloc((height + 2 * safety) * rowlen)) == 0)
		{
			free(bitmap);
			return 0;
		}

		if ((bitmap->line = malloc(height * sizeof(unsigned char *))) == 0)
		{
			free(bm);
			free(bitmap);
			return 0;
		}

		for (i = 0;i < height;i++)
			bitmap->line[i] = &bm[(i + safety) * rowlen + safety];

		bitmap->_private = bm;

		osd_clearbitmap(bitmap);
	}

	return bitmap;
}



/* set the bitmap to black */
void osd_clearbitmap(struct osd_bitmap *bitmap)
{
	int i;


	for (i = 0;i < bitmap->height;i++)
	{
		if (bitmap->depth == 16)
			memset(bitmap->line[i],0,2*bitmap->width);
		else
			memset(bitmap->line[i],BACKGROUND,bitmap->width);
	}


	if (bitmap == scrbitmap)
	{
		extern int bitmap_dirty;        /* in mame.c */

		osd_mark_dirty (0,0,bitmap->width-1,bitmap->height-1,1);
		bitmap_dirty = 1;
	}
}



void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		free(bitmap->line);
		free(bitmap->_private);
		free(bitmap);
	}
}


void osd_mark_dirty(int _x1, int _y1, int _x2, int _y2, int ui)
{
	if (use_dirty)
	{
		int x, y;

//        if (errorlog) fprintf(errorlog, "mark_dirty %3d,%3d - %3d,%3d\n", _x1,_y1, _x2,_y2);

		_x1 -= skipcolumns;
		_x2 -= skipcolumns;
		_y1 -= skiplines;
		_y2 -= skiplines;

	if (_y1 >= gfx_display_lines || _y2 < 0 || _x1 > gfx_display_columns || _x2 < 0) return;
		if (_y1 < 0) _y1 = 0;
		if (_y2 >= gfx_display_lines) _y2 = gfx_display_lines - 1;
		if (_x1 < 0) _x1 = 0;
		if (_x2 >= gfx_display_columns) _x2 = gfx_display_columns - 1;

		for (y = _y1; y <= _y2 + 15; y += 16)
			for (x = _x1; x <= _x2 + 15; x += 16)
				MARKDIRTY(x,y);
	}
}

static void init_dirty(char dirty)
{
	memset(dirty_new, dirty, MAX_GFX_WIDTH/16 * MAX_GFX_HEIGHT/16);
}

INLINE void swap_dirty(void)
{
    char *tmp;

	tmp = dirty_old;
	dirty_old = dirty_new;
	dirty_new = tmp;
}

/*
 * This function tries to find the best display mode.
 */
static void select_display_mode(void)
{
	int width,height, i;

	auto_resolution = 0;
	/* assume unchained video mode  */
	unchained = 0;
	/* see if it's a low scanrate mode */
	switch (monitor_type)
	{
		case MONITOR_TYPE_NTSC:
		case MONITOR_TYPE_PAL:
		case MONITOR_TYPE_ARCADE:
			scanrate15KHz = 1;
			break;
		default:
			scanrate15KHz = 0;
	}

	/* initialise quadring table [useful for *all* doubling modes */
	for (i = 0; i < 256; i++)
	{
		doublepixel[i] = i | (i<<8);
		quadpixel[i] = i | (i<<8) | (i << 16) | (i << 24);
	}

	if (vector_game)
	{
		width = Machine->drv->screen_width;
		height = Machine->drv->screen_height;
	}
	else
	{
		width = Machine->drv->visible_area.max_x - Machine->drv->visible_area.min_x + 1;
		height = Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	use_vesa = -1;

	/* 16 bit color is supported only by VESA modes */
	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
	{
		if (errorlog)
			fprintf (errorlog, "Game needs 16-bit colors. Using VESA\n");
		use_tweaked = 0;
	}


  /* Check for special 15.75KHz mode (req. for 15.75KHz Arcade Modes) */
	if (scanrate15KHz == 1)
	{
		if (errorlog)
		{
			switch (monitor_type)
			{
				case MONITOR_TYPE_NTSC:
					fprintf (errorlog, "Using special NTSC video mode.\n");
					break;
				case MONITOR_TYPE_PAL:
					fprintf (errorlog, "Using special PAL video mode.\n");
					break;
				case MONITOR_TYPE_ARCADE:
					fprintf (errorlog, "Using special arcade monitor mode.\n");
					break;
			}
		}
		scanlines = 0;
  		/* if no width/height specified, pick one from our tweaked list */
		if (!gfx_width && !gfx_height)
		{
			for (i=0; arcade_tweaked[i].x != 0; i++)
			{
				/* find height/width fit */
				/* only allow VESA modes if vesa explicitly selected */
				/* only allow PAL / NTSC modes if explicitly selected */
				/* arcade modes cover 50-60Hz) */
				if ((use_tweaked == 0 ||!arcade_tweaked[i].vesa) &&
					(monitor_type == MONITOR_TYPE_ARCADE || /* handles all 15.75KHz modes */
					(arcade_tweaked[i].ntsc && monitor_type == MONITOR_TYPE_NTSC) ||  /* NTSC only */
					(!arcade_tweaked[i].ntsc && monitor_type == MONITOR_TYPE_PAL)) &&  /* PAL ONLY */
					width  <= arcade_tweaked[i].matchx &&
					height <= arcade_tweaked[i].y)

				{
					gfx_width  = arcade_tweaked[i].x;
					gfx_height = arcade_tweaked[i].y;
					break;
				}
			}
			/* if it's a vector, and there's isn't an SVGA support we want to avoid the half modes */
			/* - so force default res. */
			if (vector_game && (use_vesa == 0 || monitor_type == MONITOR_TYPE_PAL))
				gfx_width = 0;

			/* we didn't find a tweaked 15.75KHz mode to fit */
			if (gfx_width == 0)
			{
				/* pick a default resolution for the monitor type */
				/* something with the right refresh rate + an aspect ratio which can handle vectors */
				switch (monitor_type)
				{
					case MONITOR_TYPE_NTSC:
					case MONITOR_TYPE_ARCADE:
						gfx_width = 320; gfx_height = 240;
						break;
					case MONITOR_TYPE_PAL:
						gfx_width = 320; gfx_height = 256;
						break;
				}

        		use_vesa = 0;
        		color_depth = 8;
			}
			else
				use_vesa = arcade_tweaked[i].vesa;
		}

	}

	/* Select desired tweaked mode for 256x224 */
	/* still no real 256x224 mode supported */
	/* changed to not override specific user request */
	if (!gfx_width && !gfx_height && use_tweaked && width <= 256 && height <= 224)
	{
		if (tw256x224_hor)
		{
			gfx_width = 256;
			gfx_height = 256;
		}
		else
		{
			gfx_width = 288;
			gfx_height = 224;
		}
	}

	/* If using tweaked modes, check if there exists one to fit
	   the screen in, otherwise use VESA */
	if (use_tweaked && !gfx_width && !gfx_height)
	{
		for (i=0; vga_tweaked[i].x != 0; i++)
		{
			if (width <= vga_tweaked[i].x &&
				height <= vga_tweaked[i].y)
			{
				gfx_width  = vga_tweaked[i].x;
				gfx_height = vga_tweaked[i].y;
				/* if a resolution was chosen that is only available as */
				/* noscanline, we need to reset the scanline global */
				if (vga_tweaked[i].scanlines == 0)
					scanlines = 0;
				use_vesa = 0;
				/* leave the loop on match */

if (gfx_width == 320 && gfx_height == 240 && scanlines == 0)
{
	use_vesa = 1;
	gfx_width = 0;
	gfx_height = 0;
}
				break;
			}
		}
		/* If we didn't find a tweaked VGA mode, use VESA */
		if (gfx_width == 0)
		{
			if (errorlog)
				fprintf (errorlog, "Did not find a tweaked VGA mode. Using VESA.\n");
			use_vesa = 1;
		}
	}


	/* If no VESA resolution has been given, we choose a sensible one. */
	/* 640x480, 800x600 and 1024x768 are common to all VESA drivers. */
	if (!gfx_width && !gfx_height)
	{
		auto_resolution = 1;
		use_vesa = 1;

		/* vector games use 640x480 as default */
		if (vector_game)
		{
			gfx_width = 640;
			gfx_height = 480;
		}
		else
		{
			int xm,ym;

			xm = ym = 1;

			if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
					== VIDEO_PIXEL_ASPECT_RATIO_1_2)
			{
				if (Machine->orientation & ORIENTATION_SWAP_XY)
					xm++;
				else ym++;
			}

			if (scanlines && stretch)
			{
				if (ym == 1)
				{
					xm *= 2;
					ym *= 2;
				}

				/* see if pixel doubling can be applied at 640x480 */
				if (ym*height <= 480 && xm*width <= 640 &&
						(xm > 1 || (ym+1)*height > 768 || (xm+1)*width > 1024))
				{
					gfx_width = 640;
					gfx_height = 480;
				}
				/* see if pixel doubling can be applied at 800x600 */
				else if (ym*height <= 600 && xm*width <= 800 &&
						(xm > 1 || (ym+1)*height > 768 || (xm+1)*width > 1024))
				{
					gfx_width = 800;
					gfx_height = 600;
				}
				/* don't use 1024x768 right away. If 512x384 is available, it */
				/* will provide hardware scanlines. */

				if (ym > 1 && xm > 1)
				{
					xm /= 2;
					ym /= 2;
				}
			}

			if (!gfx_width && !gfx_height)
			{
				if (ym*height <= 240 && xm*width <= 320)
				{
					gfx_width = 320;
					gfx_height = 240;
				}
				else if (ym*height <= 300 && xm*width <= 400)
				{
					gfx_width = 400;
					gfx_height = 300;
				}
				else if (ym*height <= 384 && xm*width <= 512)
				{
					gfx_width = 512;
					gfx_height = 384;
				}
				else if (ym*height <= 480 && xm*width <= 640 &&
						(!stretch || (ym+1)*height > 768 || (xm+1)*width > 1024))
				{
					gfx_width = 640;
					gfx_height = 480;
				}
				else if (ym*height <= 600 && xm*width <= 800 &&
						(!stretch || (ym+1)*height > 768 || (xm+1)*width > 1024))
				{
					gfx_width = 800;
					gfx_height = 600;
				}
				else
				{
					gfx_width = 1024;
					gfx_height = 768;
				}
			}
		}
	}
}



/* center image inside the display based on the visual area */
static void adjust_display(int xmin, int ymin, int xmax, int ymax)
{
	int temp;
	int w,h;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		temp = xmin; xmin = ymin; ymin = temp;
		temp = xmax; xmax = ymax; ymax = temp;
		w = Machine->drv->screen_height;
		h = Machine->drv->screen_width;
	}
	else
	{
		w = Machine->drv->screen_width;
		h = Machine->drv->screen_height;
	}

	if (!vector_game)
	{
		if (Machine->orientation & ORIENTATION_FLIP_X)
		{
			temp = w - xmin - 1;
			xmin = w - xmax - 1;
			xmax = temp;
		}
		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			temp = h - ymin - 1;
			ymin = h - ymax - 1;
			ymax = temp;
		}
	}

	viswidth  = xmax - xmin + 1;
	visheight = ymax - ymin + 1;


	xmultiply = 1;
	ymultiply = 1;

	if (use_vesa && !vector_game)
	{
		if (stretch)
		{
			if (!(Machine->orientation & ORIENTATION_SWAP_XY) &&
					!(Machine->drv->video_attributes & VIDEO_DUAL_MONITOR))
			{
				/* horizontal, non dual monitor games may be stretched at will */
				while ((xmultiply+1) * viswidth <= gfx_width)
					xmultiply++;
				while ((ymultiply+1) * visheight <= gfx_height)
					ymultiply++;
			}
			else
			{
				int tw,th;

				tw = gfx_width;
				th = gfx_height;

				if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
						== VIDEO_PIXEL_ASPECT_RATIO_1_2)
				{
					if (Machine->orientation & ORIENTATION_SWAP_XY)
						tw /= 2;
					else th /= 2;
				}

				/* Hack for 320x480 and 400x600 "vmame" video modes */
				if ((gfx_width == 320 && gfx_height == 480) ||
						(gfx_width == 400 && gfx_height == 600))
					th /= 2;

				/* maintain aspect ratio for other games */
				while ((xmultiply+1) * viswidth <= tw &&
						(ymultiply+1) * visheight <= th)
				{
					xmultiply++;
					ymultiply++;
				}

				if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
						== VIDEO_PIXEL_ASPECT_RATIO_1_2)
				{
					if (Machine->orientation & ORIENTATION_SWAP_XY)
						xmultiply *= 2;
					else ymultiply *= 2;
				}

				/* Hack for 320x480 and 400x600 "vmame" video modes */
				if ((gfx_width == 320 && gfx_height == 480) ||
						(gfx_width == 400 && gfx_height == 600))
					ymultiply *= 2;
			}
		}
		else
		{
			if ((Machine->drv->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK)
					== VIDEO_PIXEL_ASPECT_RATIO_1_2)
			{
				if (Machine->orientation & ORIENTATION_SWAP_XY)
					xmultiply *= 2;
				else ymultiply *= 2;
			}

			/* Hack for 320x480 and 400x600 "vmame" video modes */
			if ((gfx_width == 320 && gfx_height == 480) ||
					(gfx_width == 400 && gfx_height == 600))
				ymultiply *= 2;
		}
	}

	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
	{
		if (xmultiply > MAX_X_MULTIPLY16) xmultiply = MAX_X_MULTIPLY16;
		if (ymultiply > MAX_Y_MULTIPLY16) ymultiply = MAX_Y_MULTIPLY16;
	}
	else
	{
		if (xmultiply > MAX_X_MULTIPLY) xmultiply = MAX_X_MULTIPLY;
		if (ymultiply > MAX_Y_MULTIPLY) ymultiply = MAX_Y_MULTIPLY;
	}

	gfx_display_lines = visheight;
	gfx_display_columns = viswidth;

	gfx_xoffset = (gfx_width - viswidth * xmultiply) / 2;
	if (gfx_display_columns > gfx_width / xmultiply)
		gfx_display_columns = gfx_width / xmultiply;

	gfx_yoffset = (gfx_height - visheight * ymultiply) / 2;
		if (gfx_display_lines > gfx_height / ymultiply)
			gfx_display_lines = gfx_height / ymultiply;


	skiplinesmin = ymin;
	skiplinesmax = visheight - gfx_display_lines + ymin;
	skipcolumnsmin = xmin;
	skipcolumnsmax = viswidth - gfx_display_columns + xmin;

	/* Align on a quadword !*/
	gfx_xoffset &= ~7;

	/* the skipcolumns from mame.cfg/cmdline is relative to the visible area */
	skipcolumns = xmin + skipcolumns;
	skiplines   = ymin + skiplines;

	/* Just in case the visual area doesn't fit */
	if (gfx_xoffset < 0)
	{
		skipcolumns -= gfx_xoffset;
		gfx_xoffset = 0;
	}
	if (gfx_yoffset < 0)
	{
		skiplines   -= gfx_yoffset;
		gfx_yoffset = 0;
	}

	/* Failsafe against silly parameters */
	if (skiplines < skiplinesmin)
		skiplines = skiplinesmin;
	if (skipcolumns < skipcolumnsmin)
		skipcolumns = skipcolumnsmin;
	if (skiplines > skiplinesmax)
		skiplines = skiplinesmax;
	if (skipcolumns > skipcolumnsmax)
		skipcolumns = skipcolumnsmax;

	if (errorlog)
		fprintf(errorlog,
				"gfx_width = %d gfx_height = %d\n"
				"gfx_xoffset = %d gfx_yoffset = %d\n"
				"xmin %d ymin %d xmax %d ymax %d\n"
				"skiplines %d skipcolumns %d\n"
				"gfx_display_lines %d gfx_display_columns %d\n"
				"xmultiply %d ymultiply %d\n",
				gfx_width,gfx_height,
				gfx_xoffset,gfx_yoffset,
				xmin, ymin, xmax, ymax, skiplines, skipcolumns,gfx_display_lines,gfx_display_columns,xmultiply,ymultiply);

	set_ui_visarea (skipcolumns, skiplines, skipcolumns+gfx_display_columns-1, skiplines+gfx_display_lines-1);
}



int game_width;
int game_height;
int game_attributes;

/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int attributes)
{
	int	vga_reg_val;

	if (errorlog)
		fprintf (errorlog, "width %d, height %d\n", width,height);

	brightness = 100;
	brightness_paused_adjust = 1.0;

	if (frameskip < 0) frameskip = 0;
	if (frameskip >= FRAMESKIP_LEVELS) frameskip = FRAMESKIP_LEVELS-1;


	gone_to_gfx_mode = 0;

	/* Look if this is a vector game */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		vector_game = 1;
	else
		vector_game = 0;


	/* Is the game using a dirty system? */
	if ((Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY) || vector_game)
		use_dirty = 1;
	else
		use_dirty = 0;

	select_display_mode();

	if (vector_game)
	{
		scale_vectorgames(gfx_width,gfx_height,&width, &height);
	}

	game_width = width;
	game_height = height;
	game_attributes = attributes;

	if (color_depth == 16 && (Machine->drv->video_attributes & VIDEO_SUPPORTS_16BIT))
		scrbitmap = osd_new_bitmap(width,height,16);
	else
		scrbitmap = osd_new_bitmap(width,height,8);

	if (!scrbitmap) return 0;

	if (!osd_set_display(width, height, attributes))
		return 0;

	/* center display based on visible area */
 	if (vector_game)
		adjust_display(0, 0, width-1, height-1);
	else
	{
		struct rectangle vis = Machine->drv->visible_area;
		adjust_display(vis.min_x, vis.min_y, vis.max_x, vis.max_y);
	}

   /*Check for SVGA 15.75KHz mode (req. for 15.75KHz Arcade Monitor Modes)
     need to do this here, as the double params will be set up correctly */
	if (use_vesa == 1 && scanrate15KHz)
	{
		int dbl;
		dbl = (ymultiply >= 2);
		/* find a driver */
		if (!getSVGA15KHzdriver (&SVGA15KHzdriver))
		{
			printf ("\nUnable to find 15.75KHz SVGA driver for %dx%d\n", gfx_width, gfx_height);
			if (use_triplebuf)
				printf ("\nTriple buffering is on - turn it off to use Generic 15.75KHz SVGA driver\n");
			return 0;
		}
		if(errorlog)
			fprintf (errorlog, "Using %s 15.75KHz SVGA driver\n", SVGA15KHzdriver->name);
		/*and try to set the mode */
		if (!SVGA15KHzdriver->setSVGA15KHzmode (dbl, gfx_width, gfx_height))
		{
			printf ("\nUnable to set SVGA 15.75KHz mode %dx%d (driver: %s)\n", gfx_width, gfx_height, SVGA15KHzdriver->name);
			return 0;
		}
		/* if we're doubling, we might as well have scanlines */
		/* the 15.75KHz driver is going to drop every other line anyway -
			so we can avoid drawing them and save some time */
		if(dbl)
			scanlines=1;
	}

	if (use_vesa == 0)
	{
		if (use_dirty) /* supports dirty ? */
		{
			if (unchained)
			{
				update_screen = blitscreen_dirty1_unchained_vga;
				if (errorlog) fprintf (errorlog, "blitscreen_dirty1_unchained_vga\n");
			}
			else
			{
				update_screen = blitscreen_dirty1_vga;
				if (errorlog) fprintf (errorlog, "blitscreen_dirty1_vga\n");
			}
		}
		else
		{
			/* check for unchained modes */
			if (unchained)
			{
				update_screen = blitscreen_dirty0_unchained_vga;
				if (errorlog) fprintf (errorlog, "blitscreen_dirty0_unchained_vga\n");
			}
			else
			{
				update_screen = blitscreen_dirty0_vga;
				if (errorlog) fprintf (errorlog, "blitscreen_dirty0_vga\n");
			}
		}
	}
	else
	{
		if (scrbitmap->depth == 16)
		{
			update_screen = updaters16[xmultiply-1][ymultiply-1][scanlines?1:0][use_dirty?1:0];
		}
		else
		{
			update_screen = updaters8[xmultiply-1][ymultiply-1][scanlines?1:0][use_dirty?1:0];
		}
	}

    return scrbitmap;
}

/* set the actual display screen but don't allocate the screen bitmap */
int osd_set_display(int width,int height, int attributes)
{
	int     i;
	/* moved 'found' to here (req. for 15.75KHz Arcade Monitor Modes) */
	int     found;

	if (!gfx_height || !gfx_width)
	{
		printf("Please specify height AND width (e.g. -640x480)\n");
		return 0;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}
	/* Mark the dirty buffers as dirty */

	if (use_dirty)
	{
		if (vector_game)
			/* vector games only use one dirty buffer */
			init_dirty (0);
		else
			init_dirty(1);
		swap_dirty();
		init_dirty(1);
	}
	for (i = 0;i < 256;i++)
	{
		dirtycolor[i] = 1;
	}
	dirtypalette = 1;
	/* handle special 15.75KHz modes, these now include SVGA modes */
	found = 0;
	/*move video freq set to here, as we need to set it explicitly for the 15.75KHz modes */
	videofreq = vgafreq;

	if (scanrate15KHz == 1)
	{
		/* pick the mode from our 15.75KHz tweaked modes */
		for (i=0; ((arcade_tweaked[i].x != 0) && !found); i++)
		{
			if (gfx_width  == arcade_tweaked[i].x &&
				gfx_height == arcade_tweaked[i].y)
			{
				/* check for SVGA mode with no vesa flag */
				if (arcade_tweaked[i].vesa&& use_vesa == 0)
				{
					printf ("\n %dx%d SVGA 15.75KHz mode only available if tweaked flag is set to 0\n", gfx_width, gfx_height);
					return 0;
				}
				/* check for a NTSC or PAL mode with no arcade flag */
				if (monitor_type != MONITOR_TYPE_ARCADE)
				{
					if (arcade_tweaked[i].ntsc && monitor_type != MONITOR_TYPE_NTSC)
					{
						printf("\n %dx%d 15.75KHz mode only available if -monitor set to 'arcade' or 'ntsc' \n", gfx_width, gfx_height);
						return 0;
					}
					if (!arcade_tweaked[i].ntsc && monitor_type != MONITOR_TYPE_PAL)
					{
						printf("\n %dx%d 15.75KHz mode only available if -monitor set to 'arcade' or 'pal' \n", gfx_width, gfx_height);
						return 0;
					}

				}

				reg = arcade_tweaked[i].reg;
				reglen = arcade_tweaked[i].reglen;
				use_vesa = arcade_tweaked[i].vesa;
				half_yres = arcade_tweaked[i].half_yres;
				/* all 15.75KHz VGA modes are unchained */
				unchained = !use_vesa;

				if (errorlog)
					fprintf (errorlog, "15.75KHz mode (%dx%d) vesa:%d half:%d unchained:%d\n",
										gfx_width, gfx_height, use_vesa, half_yres, unchained);
				/* always use the freq from the structure */
				videofreq = arcade_tweaked[i].syncvgafreq;
				found = 1;
			}
		}
		/* explicitly asked for an 15.75KHz mode which doesn't exist , so inform and exit */
		if (!found)
		{
			printf ("\nNo %dx%d 15.75KHz mode available.\n", gfx_width, gfx_height);
			return 0;
		}
		/* center the mode */
		center15KHz (reg);
	}

	if (use_vesa != 1 && use_tweaked == 1)
	{
		/* setup tweaked modes */
		/* handle special noscanlines 256x256 57Hz tweaked mode */
		if (!found && gfx_width == 256 && gfx_height == 256 &&
				video_sync && Machine->drv->frames_per_second == 57)
		{
			if (!(Machine->orientation & ORIENTATION_SWAP_XY))
			{
				reg = (scanlines ? scr256x256horscanlines_57 : scr256x256hor_57);
				reglen = sizeof(scr256x256hor_57)/sizeof(Register);
				videofreq = 0;
				found = 1;
			}
			else
			{
				reg = scr256x256_57;
						/* still no -scanlines mode */
				reglen = sizeof(scr256x256_57)/sizeof(Register);
				videofreq = 0;
				found = 1;
			}
		}

		/* handle special 256x256 horizontal tweaked mode */
		if (!found && gfx_width == 256 && gfx_height == 256 &&
				!(Machine->orientation & ORIENTATION_SWAP_XY))
		{
			reg = (scanlines ? scr256x256horscanlines : scr256x256hor);
			reglen = sizeof(scr256x256hor)/sizeof(Register);
			videofreq = 0;
			found = 1;
		}

		/* find the matching tweaked mode */
		/* use noscanline modes if scanline modes not possible */
		for (i=0; ((vga_tweaked[i].x != 0) && !found); i++)
		{
			int scan;
			scan = vga_tweaked[i].scanlines;

			if (gfx_width  == vga_tweaked[i].x &&
				gfx_height == vga_tweaked[i].y &&
				(scanlines == scan || scan == 0))
			{
				reg = vga_tweaked[i].reg;
				reglen = vga_tweaked[i].reglen;
				if (videofreq == -1)
					videofreq = vga_tweaked[i].syncvgafreq;
				found = 1;
				unchained = vga_tweaked[i].unchained;
			}
		}


		/* can't find a VGA mode, use VESA */
		if (found == 0)
		{
			use_vesa = 1;
		}
		else
		{
			use_vesa = 0;
			if (videofreq < 0) videofreq = 0;
			else if (videofreq > 3) videofreq = 3;
		}
	}

	if (use_vesa != 0)
	{
		/*removed local 'found' */
		int mode, bits, err;

		mode = gfx_mode;
		found = 0;
		bits = scrbitmap->depth;

		/* Try the specified vesamode, 565 and 555 for 16 bit color modes, */
		/* doubled resolution in case of noscanlines and if not succesful  */
		/* repeat for all "lower" VESA modes. NS/BW 19980102 */

		while (!found)
		{
			set_color_depth(bits);

			/* allocate a wide enough virtual screen if possible */
			/* we round the width (in dwords) to be an even multiple 256 - that */
			/* way, during page flipping page only one byte of the video RAM */
			/* address changes, therefore preventing flickering. */
			if (bits == 8)
				triplebuf_page_width = (gfx_width + 0x3ff) & ~0x3ff;
			else
				triplebuf_page_width = (gfx_width + 0x1ff) & ~0x1ff;

			/* don't force triple buffer for 15.75Khz modes */
			/* the virtual screen width created won't fit into to the 8bit register used */
			/* by the generic driver*/
			if (scanrate15KHz && !use_triplebuf)
				err = set_gfx_mode(mode,gfx_width,gfx_height,0,0);
			else
			{
				/* don't ask for a larger screen if triplebuffer not requested - could */
				/* cause problems in some cases. */
				err = 1;
				if (use_triplebuf)
					err = set_gfx_mode(mode,gfx_width,gfx_height,3*triplebuf_page_width,0);
				if (err)
					err = set_gfx_mode(mode,gfx_width,gfx_height,0,0);
			}

			if (errorlog)
			{
				fprintf (errorlog,"Trying ");
				if      (mode == GFX_VESA1)
					fprintf (errorlog, "VESA1");
				else if (mode == GFX_VESA2B)
					fprintf (errorlog, "VESA2B");
				else if (mode == GFX_VESA2L)
				    fprintf (errorlog, "VESA2L");
				else if (mode == GFX_VESA3)
					fprintf (errorlog, "VESA3");
			    fprintf (errorlog, "  %dx%d, %d bit\n",
						gfx_width, gfx_height, bits);
			}

			if (err == 0)
			{
				found = 1;
				continue;
			}
			else if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);

			/* Now adjust parameters for the next loop */

			/* try 5-5-5 in case there is no 5-6-5 16 bit color mode */
			if (scrbitmap->depth == 16)
			{
				if (bits == 16)
				{
					bits = 15;
					continue;
				}
				else
					bits = 16; /* reset to 5-6-5 */
			}

			/* try VESA modes in VESA3-VESA2L-VESA2B-VESA1 order */

			if (mode == GFX_VESA3)
			{
				mode = GFX_VESA2L;
				continue;
			}
			else if (mode == GFX_VESA2L)
			{
				mode = GFX_VESA2B;
				continue;
			}
			else if (mode == GFX_VESA2B)
			{
				mode = GFX_VESA1;
				continue;
			}
			else if (mode == GFX_VESA1)
				mode = gfx_mode; /* restart with the mode given in mame.cfg */

			/* try higher resolutions */
			if (auto_resolution)
			{
				if (stretch && gfx_width <= 512)
				{
					/* low res VESA mode not available, try an high res one */
					gfx_width *= 2;
					gfx_height *= 2;
					continue;
				}

				/* try next higher resolution */
				if (gfx_height < 300 && gfx_width < 400)
				{
					gfx_width = 400;
					gfx_height = 300;
					continue;
				}
				else if (gfx_height < 384 && gfx_width < 512)
				{
					gfx_width = 512;
					gfx_height = 384;
					continue;
				}
				else if (gfx_height < 480 && gfx_width < 640)
				{
					gfx_width = 640;
					gfx_height = 480;
					continue;
				}
				else if (gfx_height < 600 && gfx_width < 800)
				{
					gfx_width = 800;
					gfx_height = 600;
					continue;
				}
				else if (gfx_height < 768 && gfx_width < 1024)
				{
					gfx_width = 1024;
					gfx_height = 768;
					continue;
				}
			}

			/* If there was no continue up to this point, we give up */
			break;
		}

		if (found == 0)
		{
			printf ("\nNo %d-bit %dx%d VESA mode available.\n",
					scrbitmap->depth,gfx_width,gfx_height);
			printf ("\nPossible causes:\n"
"1) Your video card does not support VESA modes at all. Almost all\n"
"   video cards support VESA modes natively these days, so you probably\n"
"   have an older card which needs some driver loaded first.\n"
"   In case you can't find such a driver in the software that came with\n"
"   your video card, Scitech Display Doctor or (for S3 cards) S3VBE\n"
"   are good alternatives.\n"
"2) Your VESA implementation does not support this resolution. For example,\n"
"   '-320x240', '-400x300' and '-512x384' are only supported by a few\n"
"   implementations.\n"
"3) Your video card doesn't support this resolution at this color depth.\n"
"   For example, 1024x768 in 16 bit colors requires 2MB video memory.\n"
"   You can either force an 8 bit video mode ('-depth 8') or use a lower\n"
"   resolution ('-640x480', '-800x600').\n");
			return 0;
		}
		else
		{
			if (errorlog)
				fprintf (errorlog, "Found matching %s mode\n", gfx_driver->desc);
			gfx_mode = mode;

			/* disable triple buffering if the screen is not large enough */
			if (errorlog)
				fprintf (errorlog, "Virtual screen size %dx%d\n",VIRTUAL_W,VIRTUAL_H);
			if (VIRTUAL_W < 3*triplebuf_page_width)
			{
				use_triplebuf = 0;
				if (errorlog)
					fprintf (errorlog, "Triple buffer disabled\n");
			}

			/* if triple buffering is enabled, turn off vsync */
			if (use_triplebuf)
			{
				wait_vsync = 0;
				video_sync = 0;
			}
		}
	}
	else
	{


		/* set the VGA clock */
		if (video_sync || always_synced || wait_vsync)
			reg[0].value = (reg[0].value & 0xf3) | (videofreq << 2);


		/* set the horizontal and vertical total */
		if(scanrate15KHz)
		{
			/*15.75KHz modes*/
			if ((gfx_width == 224) && (gfx_height == 288))
			{
				reg[1].value = tw224x288arc_h;
				reg[7].value = tw224x288arc_v;
			}
			else if ((gfx_width == 256) && (gfx_height == 256))
			{
				reg[1].value = tw256x256arc_h;
				reg[7].value = tw256x256arc_v;
			}
			else if ((gfx_width == 256) && (gfx_height == 240))
			{
				reg[1].value = tw256x240arc_h;
				reg[7].value = tw256x240arc_v;
			}
			else if ((gfx_width == 288) && (gfx_height == 224))
			{
				reg[1].value = tw288x224arc_h;
				reg[7].value = tw288x224arc_v;
			}
			else if ((gfx_width == 320) && (gfx_height == 240))
			{
				reg[1].value = tw320x240arc_h;
				reg[7].value = tw320x240arc_v;
			}
			else if ((gfx_width == 320) && (gfx_height == 256))
			{
				reg[1].value = tw320x256arc_h;
				reg[7].value = tw320x256arc_v;
			}
			else if ((gfx_width == 352) && (gfx_height == 240))
			{
				reg[1].value = tw352x240arc_h;
				reg[7].value = tw352x240arc_v;
			}
			else if ((gfx_width == 352) && (gfx_height == 256))
			{
				reg[1].value = tw352x256arc_h;
				reg[7].value = tw352x256arc_v;
			}
			else if ((gfx_width == 368) && (gfx_height == 240))
			{
				reg[1].value = tw368x240arc_h;
				reg[7].value = tw368x240arc_v;
			}
			else if ((gfx_width == 368) && (gfx_height == 256))
			{
				reg[1].value = tw368x256arc_h;
				reg[7].value = tw368x256arc_v;
			}
			else if ((gfx_width == 512) && (gfx_height == 224))
			{
				reg[1].value = tw512x224arc_h;
				reg[7].value = tw512x224arc_v;
			}
			else if ((gfx_width == 512) && (gfx_height == 256))
			{
				reg[1].value = tw512x256arc_h;
				reg[7].value = tw512x256arc_v;
			}
			else if ((gfx_width == 512) && (gfx_height == 448))
			{
				reg[1].value = tw512x448arc_h;
				reg[7].value = tw512x448arc_v;
			}
			else if ((gfx_width == 512) && (gfx_height == 512))
			{
				reg[1].value = tw512x512arc_h;
				reg[7].value = tw512x512arc_v;
			}
		}
		else
		{
			/* normal PC monitor modes */
			if ((gfx_width == 224) && (gfx_height == 288))
			{
				if (scanlines)
				{
					reg[1].value = tw224x288sc_h;
					reg[7].value = tw224x288sc_v;
				}
				else
				{
					reg[1].value = tw224x288ns_h;
					reg[7].value = tw224x288ns_v;
				}
			}
			else if ((gfx_width == 288) && (gfx_height == 224))
			{
				if (scanlines)
				{
					reg[1].value = tw288x224sc_h;
					reg[7].value = tw288x224sc_v;
				}
				else
				{
					reg[1].value = tw288x224ns_h;
					reg[7].value = tw288x224ns_v;
				}
			}
			/* 320x240 mode */
			else if ((gfx_width == 320) && (gfx_height == 240))
			{
				if (scanlines)
				{
					reg[1].value = tw320x240sc_h;
					reg[7].value = tw320x240sc_v;
				}
				else
				{
					reg[1].value = tw320x240ns_h;
					reg[7].value = tw320x240ns_v;
				}
			}
			/* 336x240 mode */
			else if ((gfx_width == 336) && (gfx_height == 240))
			{
				if (scanlines)
				{
					reg[1].value = tw336x240sc_h;
					reg[7].value = tw336x240sc_v;
				}
				else
				{
					reg[1].value = tw336x240ns_h;
					reg[7].value = tw336x240ns_v;
				}
			}
			/* 384x240 mode */
			else if ((gfx_width == 384) && (gfx_height == 240))
			{
				if (scanlines)
				{
					reg[1].value = tw384x240sc_h;
					reg[7].value = tw384x240sc_v;
				}
				else
				{
					reg[1].value = tw384x240ns_h;
					reg[7].value = tw384x240ns_v;
				}
			}
			else if ((gfx_width == 256) && (gfx_height == 256))
			{
				if (Machine->orientation & ORIENTATION_SWAP_XY)
				{
					/* vertical 256x256 */
					if (Machine->drv->frames_per_second != 57)
					{
						if (scanlines)
						{
							reg[1].value = tw256x256sc_h;
							reg[7].value = tw256x256sc_v;
						}
						else
						{
							reg[1].value = tw256x256ns_h;
							reg[7].value = tw256x256ns_v;
						}
					}
					else
					{
						if (scanlines)
						{
							reg[1].value = tw256x256sc_57_h;
							reg[7].value = tw256x256sc_57_v;
						}
						else
						{
							reg[1].value = tw256x256ns_57_h;
							reg[7].value = tw256x256ns_57_v;
						}
					}
				}
				else
				{
					/* horizontal 256x256 */
					if (Machine->drv->frames_per_second != 57)
					{
						if (scanlines)
						{
							reg[1].value = tw256x256sc_hor_h;
							reg[7].value = tw256x256sc_hor_v;
						}
						else
						{
							reg[1].value = tw256x256ns_hor_h;
							reg[7].value = tw256x256ns_hor_v;
						}
					}
					else
					{
						if (scanlines)
						{
							reg[1].value = tw256x256sc_h57_h;
							reg[7].value = tw256x256sc_h57_v;
						}
						else
						{
							reg[1].value = tw256x256ns_h57_h;
							reg[7].value = tw256x256ns_h57_v;
						}
					}
				}
			}
		}

		/* big hack: open a mode 13h screen using Allegro, then load the custom screen */
		/* definition over it. */
		if (set_gfx_mode(GFX_VGA,320,200,0,0) != 0)
			return 0;
		/* tweak the mode */
		outRegArray(reg,reglen);
		/* check for unchained mode,  if unchained clear all pages */
		if (unchained)
		{
			unsigned long address;
			/* turn off any other 'vsync's */
			video_sync = 0;
			wait_vsync = 0;
			/* clear all 4 bit planes */
			outportw (0x3c4, (0x02 | (0x0f << 0x08)));
			for (address = 0xa0000; address < 0xb0000; address += 4)
				_farpokel(screen->seg, address, 0);
		}
	}


	gone_to_gfx_mode = 1;


	vsync_frame_rate = Machine->drv->frames_per_second;

	if (video_sync)
	{
		uclock_t a,b;
		float rate;


		/* wait some time to let everything stabilize */
		for (i = 0;i < 60;i++)
		{
			vsync();
			a = uclock();
		}

		/* small delay for really really fast machines */
		for (i = 0;i < 100000;i++) ;

		vsync();
		b = uclock();
		rate = ((float)UCLOCKS_PER_SEC)/(b-a);

		if (errorlog) fprintf(errorlog,"target frame rate = %dfps, video frame rate = %3.2fHz\n",Machine->drv->frames_per_second,rate);

		/* don't allow more than 8% difference between target and actual frame rate */
		while (rate > Machine->drv->frames_per_second * 108 / 100)
			rate /= 2;

		if (rate < Machine->drv->frames_per_second * 92 / 100)
		{
			osd_close_display();
if (errorlog) fprintf(errorlog,"-vsync option cannot be used with this display mode:\n"
					"video refresh frequency = %dHz, target frame rate = %dfps\n",
					(int)(UCLOCKS_PER_SEC/(b-a)),Machine->drv->frames_per_second);
			return 0;
		}

if (errorlog) fprintf(errorlog,"adjusted video frame rate = %3.2fHz\n",rate);
		vsync_frame_rate = rate;

		if (Machine->sample_rate)
		{
			Machine->sample_rate = Machine->sample_rate * Machine->drv->frames_per_second / rate;
if (errorlog) fprintf(errorlog,"sample rate adjusted to match video freq: %d\n",Machine->sample_rate);
		}
	}

	return 1;
}



/* shut up the display */
void osd_close_display(void)
{
	if (gone_to_gfx_mode != 0)
	{
		/* tidy up if 15.75KHz SVGA mode used */
		if (scanrate15KHz && use_vesa == 1)
		{
			/* check we've got a valid driver before calling it */
			if (SVGA15KHzdriver != NULL)
				SVGA15KHzdriver->resetSVGA15KHzmode();
		}

		set_gfx_mode (GFX_TEXT,0,0,0,0);

		if (frames_displayed > FRAMES_TO_SKIP)
			printf("Average FPS: %f\n",(double)UCLOCKS_PER_SEC/(end_time-start_time)*(frames_displayed-FRAMES_TO_SKIP));
	}

	if (scrbitmap)
	{
		osd_free_bitmap(scrbitmap);
		scrbitmap = NULL;
	}
}




/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* If 'totalcolors' is 32768, 'palette' is ignored and the *pens array is filled */
/* with pen values corresponding to a 5-5-5 15-bit palette */
void osd_allocate_colors(unsigned int totalcolors,const unsigned char *palette,unsigned short *pens)
{
	if (totalcolors == 32768)
	{
		int r1,g1,b1;
		int r,g,b;


		for (r1 = 0; r1 < 32; r1++)
		{
			for (g1 = 0; g1 < 32; g1++)
			{
				for (b1 = 0; b1 < 32; b1++)
				{
					r = 255 * brightness * brightness_paused_adjust * pow(r1 / 31.0, 1 / osd_gamma_correction) / 100;
					g = 255 * brightness * brightness_paused_adjust * pow(g1 / 31.0, 1 / osd_gamma_correction) / 100;
					b = 255 * brightness * brightness_paused_adjust * pow(b1 / 31.0, 1 / osd_gamma_correction) / 100;
					*pens++ = makecol(r,g,b);
				}
			}
		}

		Machine->uifont->colortable[0] = makecol(0x00,0x00,0x00);
		Machine->uifont->colortable[1] = makecol(0xff,0xff,0xff);
		Machine->uifont->colortable[2] = makecol(0xff,0xff,0xff);
		Machine->uifont->colortable[3] = makecol(0x00,0x00,0x00);
	}
	else
	{
		int i;


		/* initialize the palette */
		for (i = 0;i < 256;i++)
			current_palette[i][0] = current_palette[i][1] = current_palette[i][2] = 0;

		if (totalcolors >= 255)
		{
			int bestblack,bestwhite;
			int bestblackscore,bestwhitescore;


			bestblack = bestwhite = 0;
			bestblackscore = 3*255*255;
			bestwhitescore = 0;
			for (i = 0;i < totalcolors;i++)
			{
				int r,g,b,score;

				r = palette[3*i];
				g = palette[3*i+1];
				b = palette[3*i+2];
				score = r*r + g*g + b*b;

				if (score < bestblackscore)
				{
					bestblack = i;
					bestblackscore = score;
				}
				if (score > bestwhitescore)
				{
					bestwhite = i;
					bestwhitescore = score;
				}
			}

			for (i = 0;i < totalcolors;i++)
				pens[i] = i;

			/* map black to pen 0, otherwise the screen border will not be black */
			pens[bestblack] = 0;
			pens[0] = bestblack;

			Machine->uifont->colortable[0] = pens[bestblack];
			Machine->uifont->colortable[1] = pens[bestwhite];
			Machine->uifont->colortable[2] = pens[bestwhite];
			Machine->uifont->colortable[3] = pens[bestblack];
		}
		else
		{
			/* reserve color 1 for the user interface text */
			current_palette[1][0] = current_palette[1][1] = current_palette[1][2] = 0xff;
			Machine->uifont->colortable[0] = 0;
			Machine->uifont->colortable[1] = 1;
			Machine->uifont->colortable[2] = 1;
			Machine->uifont->colortable[3] = 0;

			/* fill the palette starting from the end, so we mess up badly written */
			/* drivers which don't go through Machine->pens[] */
			for (i = 0;i < totalcolors;i++)
				pens[i] = 255-i;
		}

		for (i = 0;i < totalcolors;i++)
		{
			current_palette[pens[i]][0] = palette[3*i];
			current_palette[pens[i]][1] = palette[3*i+1];
			current_palette[pens[i]][2] = palette[3*i+2];
		}
	}
}


void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
	if (scrbitmap->depth != 8)
	{
		if (errorlog) fprintf(errorlog,"error: osd_modify_pen() doesn't work with %d bit video modes.\n",scrbitmap->depth);
		return;
	}


	if (current_palette[pen][0] != red ||
			current_palette[pen][1] != green ||
			current_palette[pen][2] != blue)
	{
		current_palette[pen][0] = red;
		current_palette[pen][1] = green;
		current_palette[pen][2] = blue;

		dirtycolor[pen] = 1;
		dirtypalette = 1;
	}
}



void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
	if (scrbitmap->depth == 16)
	{
		*red = getr(pen);
		*green = getg(pen);
		*blue = getb(pen);
	}
	else
	{
		*red = current_palette[pen][0];
		*green = current_palette[pen][1];
		*blue = current_palette[pen][2];
	}
}



void update_screen_dummy(void)
{
	if (errorlog)
		fprintf(errorlog, "msdos/video.c: undefined update_screen() function for %d x %d!\n",xmultiply,ymultiply);
}

INLINE void pan_display(void)
{
	/* horizontal panning */
	if (osd_key_pressed(OSD_KEY_LSHIFT))
	{
		if (osd_key_pressed(OSD_KEY_PGUP))
		{
			if (skipcolumns < skipcolumnsmax)
			{
				skipcolumns++;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
		if (osd_key_pressed(OSD_KEY_PGDN))
		{
			if (skipcolumns > skipcolumnsmin)
			{
				skipcolumns--;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
	}
	else /*  vertical panning */
	{
		if (osd_key_pressed(OSD_KEY_PGDN))
		{
			if (skiplines < skiplinesmax)
			{
				skiplines++;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
		if (osd_key_pressed(OSD_KEY_PGUP))
		{
			if (skiplines > skiplinesmin)
			{
				skiplines--;
				osd_mark_dirty (0,0,scrbitmap->width-1,scrbitmap->height-1,1);
			}
		}
	}

	if (use_dirty) init_dirty(1);

	set_ui_visarea (skipcolumns, skiplines, skipcolumns+gfx_display_columns-1, skiplines+gfx_display_lines-1);
}



int osd_skip_this_frame(void)
{
	static const int skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 0,0,0,0,0,0,0,0,0,0,0,0 },
		{ 0,0,0,0,0,0,0,0,0,0,0,1 },
		{ 0,0,0,0,0,1,0,0,0,0,0,1 },
		{ 0,0,0,1,0,0,0,1,0,0,0,1 },
		{ 0,0,1,0,0,1,0,0,1,0,0,1 },
		{ 0,1,0,0,1,0,1,0,0,1,0,1 },
		{ 0,1,0,1,0,1,0,1,0,1,0,1 },
		{ 0,1,0,1,1,0,1,0,1,1,0,1 },
		{ 0,1,1,0,1,1,0,1,1,0,1,1 },
		{ 0,1,1,1,0,1,1,1,0,1,1,1 },
		{ 0,1,1,1,1,1,0,1,1,1,1,1 },
		{ 0,1,1,1,1,1,1,1,1,1,1,1 }
	};

	return skiptable[frameskip][frameskip_counter];
}

/* Update the display. */
void osd_update_video_and_audio(void)
{
	static const int waittable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
	{
		{ 1,1,1,1,1,1,1,1,1,1,1,1 },
		{ 2,1,1,1,1,1,1,1,1,1,1,0 },
		{ 2,1,1,1,1,0,2,1,1,1,1,0 },
		{ 2,1,1,0,2,1,1,0,2,1,1,0 },
		{ 2,1,0,2,1,0,2,1,0,2,1,0 },
		{ 2,0,2,1,0,2,0,2,1,0,2,0 },
		{ 2,0,2,0,2,0,2,0,2,0,2,0 },
		{ 2,0,2,0,0,3,0,2,0,0,3,0 },
		{ 3,0,0,3,0,0,3,0,0,3,0,0 },
		{ 4,0,0,0,4,0,0,0,4,0,0,0 },
		{ 6,0,0,0,0,0,6,0,0,0,0,0 },
		{12,0,0,0,0,0,0,0,0,0,0,0 }
	};
	int i;
	static int showfps,showfpstemp,showprofile;
	uclock_t curr;
	static uclock_t prev_frames[FRAMESKIP_LEVELS],prev;
	static int speed=0;
	static int vups,vfcount;
	int need_to_clear_bitmap = 0;
	int already_synced;


	if (throttle)
	{
		static uclock_t last;

		/* if too much time has passed since last sound update, disable throttling */
		/* temporarily - we wouldn't be able to keep synch anyway. */
		curr = uclock();
		if ((curr - last) > 2*UCLOCKS_PER_SEC / Machine->drv->frames_per_second)
			throttle = 0;
		last = curr;

		already_synced = msdos_update_audio();

		throttle = 1;
	}
	else
		already_synced = msdos_update_audio();


	if (osd_skip_this_frame() == 0)
	{
		if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
		{
			showfpstemp--;
			if ((showfps == 0) && (showfpstemp == 0))
			{
				need_to_clear_bitmap = 1;
			}
		}

		if (osd_key_pressed_memory(OSD_KEY_SHOW_FPS))
		{
			if (showfpstemp)
			{
				showfpstemp = 0;
				need_to_clear_bitmap = 1;
			}
			else
			{
				showfps ^= 1;
				if (showfps == 0)
				{
					need_to_clear_bitmap = 1;
				}
			}
		}

		if (use_profiler && osd_key_pressed_memory(OSD_KEY_SHOW_PROFILE))
		{
			showprofile ^= 1;
			if (showprofile == 0)
				need_to_clear_bitmap = 1;
		}

		/* now wait until it's time to update the screen */
		if (throttle)
		{
			osd_profiler(OSD_PROFILE_IDLE);
			if (video_sync)
			{
				static uclock_t last;


				do
				{
					vsync();
					curr = uclock();
				} while (UCLOCKS_PER_SEC / (curr - last) > Machine->drv->frames_per_second * 11 /10);

				last = curr;
			}
			else
			{
				uclock_t target,target2;


				/* wait for video sync but use normal throttling */
				if (wait_vsync)
					vsync();

				curr = uclock();

				if (already_synced == 0)
				{
				/* wait only if the audio update hasn't synced us already */

					/* wait until enough time has passed since last frame... */
					target = prev +
							waittable[frameskip][frameskip_counter] * UCLOCKS_PER_SEC/Machine->drv->frames_per_second;

					/* ... OR since FRAMESKIP_LEVELS frames ago. This way, if a frame takes */
					/* longer than the allotted time, we can compensate in the following frames. */
					target2 = prev_frames[frameskip_counter] +
							FRAMESKIP_LEVELS * UCLOCKS_PER_SEC/Machine->drv->frames_per_second;

					if (target - target2 > 0) target = target2;

					if (curr - target < 0)
					{
						do
						{
							curr = uclock();
						} while (curr - target < 0);
					}
				}

			}
			osd_profiler(OSD_PROFILE_END);
		}
		else curr = uclock();


		/* for the FPS average calculation */
		if (++frames_displayed == FRAMES_TO_SKIP)
			start_time = curr;
		else
			end_time = curr;


		if (frameskip_counter == 0 && curr - prev_frames[frameskip_counter])
		{
			int divdr;


			divdr = Machine->drv->frames_per_second * (curr - prev_frames[frameskip_counter]) / (100 * FRAMESKIP_LEVELS);
			speed = (UCLOCKS_PER_SEC + divdr/2) / divdr;
		}

		prev = curr;
		for (i = 0;i < waittable[frameskip][frameskip_counter];i++)
			prev_frames[(frameskip_counter + FRAMESKIP_LEVELS - i) % FRAMESKIP_LEVELS] = curr;

		vfcount += waittable[frameskip][frameskip_counter];
		if (vfcount >= Machine->drv->frames_per_second)
		{
			extern int vector_updates; /* avgdvg_go()'s per Mame frame, should be 1 */


			vfcount = 0;
			vups = vector_updates;
			vector_updates = 0;
		}

		if (showfps || showfpstemp)
		{
			int fps;
			char buf[30];
			int divdr;


			divdr = 100 * FRAMESKIP_LEVELS;
			fps = (Machine->drv->frames_per_second * (FRAMESKIP_LEVELS - frameskip) * speed + (divdr / 2)) / divdr;
			sprintf(buf,"%s%2d%4d%%%4d/%d fps",autoframeskip?"auto":"fskp",frameskip,speed,fps,Machine->drv->frames_per_second);
			ui_text(buf,Machine->uiwidth-strlen(buf)*Machine->uifontwidth,0);
			if (vector_game)
			{
				sprintf(buf," %d vector updates",vups);
				ui_text(buf,Machine->uiwidth-strlen(buf)*Machine->uifontwidth,Machine->uifontheight);
			}
		}


		if (showprofile) osd_profiler_display();


		if (scrbitmap->depth == 8)
		{
			if (dirtypalette)
			{
				dirtypalette = 0;

				for (i = 0;i < 256;i++)
				{
					if (dirtycolor[i])
					{
						int r,g,b;


						dirtycolor[i] = 0;

						r = 255 * brightness * brightness_paused_adjust * pow(current_palette[i][0] / 255.0, 1 / osd_gamma_correction) / 100;
						g = 255 * brightness * brightness_paused_adjust * pow(current_palette[i][1] / 255.0, 1 / osd_gamma_correction) / 100;
						b = 255 * brightness * brightness_paused_adjust * pow(current_palette[i][2] / 255.0, 1 / osd_gamma_correction) / 100;

						adjusted_palette[i].r = r >> 2;
						adjusted_palette[i].g = g >> 2;
						adjusted_palette[i].b = b >> 2;

						set_color(i,&adjusted_palette[i]);
					}
				}
			}
		}


		/* copy the bitmap to screen memory */
		osd_profiler(OSD_PROFILE_BLIT);
		update_screen();
		osd_profiler(OSD_PROFILE_END);

		/* see if we need to give the card enough time to draw both odd/even fields of the interlaced display
			(req. for 15.75KHz Arcade Monitor Modes */
		interlace_sync();


		if (need_to_clear_bitmap)
			osd_clearbitmap(scrbitmap);

		if (use_dirty)
		{
			if (!vector_game)
				swap_dirty();
			init_dirty(0);
		}

		if (need_to_clear_bitmap)
			osd_clearbitmap(scrbitmap);


		if (throttle && autoframeskip && frameskip_counter == 0)
		{
			static int frameskipadjust;
			int adjspeed;

			/* adjust speed to video refresh rate if vsync is on */
			adjspeed = speed * Machine->drv->frames_per_second / vsync_frame_rate;

			if (adjspeed < 60)
			{
				frameskipadjust = 0;
				frameskip += 3;
				if (frameskip >= FRAMESKIP_LEVELS) frameskip = FRAMESKIP_LEVELS-1;
			}
			else if (adjspeed < 80)
			{
				frameskipadjust = 0;
				frameskip += 2;
				if (frameskip >= FRAMESKIP_LEVELS) frameskip = FRAMESKIP_LEVELS-1;
			}
			else if (adjspeed < 99) /* allow 99% speed */
			{
				frameskipadjust = 0;
				/* don't push frameskip too far if we are close to 100% speed */
				if (frameskip < 8) frameskip++;
			}
			else if (adjspeed >= 100)
			{
				/* increase frameskip quickly, decrease it slowly */
				frameskipadjust++;
				if (frameskipadjust >= 3)
				{
					frameskipadjust = 0;
					if (frameskip > 0) frameskip--;
				}
			}
		}
	}

	/* Check for PGUP, PGDN and pan screen */
	if (osd_key_pressed(OSD_KEY_PGDN) || osd_key_pressed(OSD_KEY_PGUP))
		pan_display();

	if (osd_key_pressed_memory(OSD_KEY_FRAMESKIP_INC))
	{
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = 0;
		}
		else
		{
			if (frameskip == FRAMESKIP_LEVELS-1)
			{
				frameskip = 0;
				autoframeskip = 1;
			}
			else
				frameskip++;
		}

		if (showfps == 0)
			showfpstemp = 2*Machine->drv->frames_per_second;

		/* reset the frame counter every time the frameskip key is pressed, so */
		/* we'll measure the average FPS on a consistent status. */
		frames_displayed = 0;
	}

	if (osd_key_pressed_memory(OSD_KEY_FRAMESKIP_DEC))
	{
		if (autoframeskip)
		{
			autoframeskip = 0;
			frameskip = FRAMESKIP_LEVELS-1;
		}
		else
		{
			if (frameskip == 0)
				autoframeskip = 1;
			else
				frameskip--;
		}

		if (showfps == 0)
			showfpstemp = 2*Machine->drv->frames_per_second;

		/* reset the frame counter every time the frameskip key is pressed, so */
		/* we'll measure the average FPS on a consistent status. */
		frames_displayed = 0;
	}

	if (osd_key_pressed_memory(OSD_KEY_THROTTLE))
	{
		throttle ^= 1;

		/* reset the frame counter every time the throttle key is pressed, so */
		/* we'll measure the average FPS on a consistent status. */
		frames_displayed = 0;
	}


	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;
}



void osd_set_gamma(float _gamma)
{
	if (scrbitmap->depth == 8)
	{
		int i;


		osd_gamma_correction = _gamma;

		for (i = 0;i < 256;i++)
		{
			if (i != Machine->uifont->colortable[1])        /* don't touch the user interface text */
				dirtycolor[i] = 1;
		}
		dirtypalette = 1;
	}
}

float osd_get_gamma(void)
{
	return osd_gamma_correction;
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
	if (scrbitmap->depth == 8)
	{
		int i;


		brightness = _brightness;

		for (i = 0;i < 256;i++)
		{
			if (i != Machine->uifont->colortable[1])        /* don't touch the user interface text */
				dirtycolor[i] = 1;
		}
		dirtypalette = 1;
	}
}

int osd_get_brightness(void)
{
	return brightness;
}


void osd_save_snapshot(void)
{
	save_screen_snapshot();
}

void osd_pause(int paused)
{
	if (scrbitmap->depth == 8)
	{
		int i;

		if (paused) brightness_paused_adjust = 0.65;
		else brightness_paused_adjust = 1.0;

		for (i = 0;i < 256;i++)
		{
			if (i != Machine->uifont->colortable[1])        /* don't touch the user interface text */
				dirtycolor[i] = 1;
		}
		dirtypalette = 1;
	}
}
