/*********************************************************************

  artwork.c

  Generic backdrop/overlay functions.

  Created by Mike Balfour - 10/01/1998

  Added some overlay and backdrop functions
  for vector games. Mathis Rosenhauer - 10/09/1998

  Please don't use the artwork functions in your own drivers yet.
  They are very likely to change in the near future.
*********************************************************************/

#include "driver.h"
#include "artwork.h"

#define MIN(x,y) (x)<(y)?(x):(y)
#define MAX(x,y) (x)>(y)?(x):(y)

/* from png.c */
extern int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap,
		     unsigned char **palette, int *num_palette,
		     unsigned char **trans, int *num_trans);

/* Local variables */
static unsigned char isblack[256];

/*********************************************************************
  backdrop_refresh

  This remaps the "original" palette indexes to the abstract OS indexes
  used by MAME.  This needs to be called every time palette_recalc
  returns a non-zero value, since the mappings will have changed.
 *********************************************************************/

void backdrop_refresh(struct artwork *a)
{
	int i,j;
	int height,width;
	struct osd_bitmap *back = NULL;
	struct osd_bitmap *orig = NULL;
	int offset;

	offset = a->start_pen;
	back = a->artwork;
	orig = a->orig_artwork;
	height = a->artwork->height;
	width = a->artwork->width;

	for ( j=0; j<height; j++)
		for (i=0; i<width; i++)
			back->line[j][i] = Machine->pens[orig->line[j][i]+offset];
}

/*********************************************************************
  backdrop_set_palette

  This sets the palette colors used by the backdrop to the new colors
  passed in as palette.  The values should be stored as one byte of red,
  one byte of blue, one byte of green.  This could hopefully be used
  for special effects, like lightening and darkening the backdrop.
 *********************************************************************/
void backdrop_set_palette(struct artwork *a, unsigned char *palette)
{
	int i;

	/* Load colors into the palette */
	if ((Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE))
	{
		for (i = 0; i < a->num_pens_used; i++)
			palette_change_color(i + a->start_pen, palette[i*3], palette[i*3+1], palette[i*3+2]);

		palette_recalc();
		backdrop_refresh(a);
	}
}

/*********************************************************************
  artwork_free

  Don't forget to clean up when you're done with the backdrop!!!
 *********************************************************************/

void artwork_free(struct artwork *a)
{
	osd_free_bitmap(a->artwork);
	osd_free_bitmap(a->orig_artwork);
	if (a->vector_bitmap)
		osd_free_bitmap(a->vector_bitmap);
	if (a->orig_palette)
		free (a->orig_palette);
	if (a->transparency)
		free (a->transparency);
	if (a->brightness)
		free (a->brightness);
	if (a->pTable)
		free (a->pTable);
	free(a);
}

/*********************************************************************
  backdrop_black_recalc

  If you use any of the experimental backdrop draw* blitters below,
  call this once per frame.  It will catch palette changes and mark
  every black as transparent.  If it returns a non-zero value, redraw
  the whole background.
 *********************************************************************/

int backdrop_black_recalc(void)
{
	unsigned char r,g,b;
	int i;
	int redraw = 0;

	/* Determine what colors can be overwritten */
	for (i=0; i<256; i++)
	{
		osd_get_pen(i,&r,&g,&b);

		if ((r==0) && (g==0) && (b==0))
		{
			if (isblack[i] != 1)
				redraw = 1;
			isblack[i] = 1;
		}
		else
		{
			if (isblack[i] != 0)
				redraw = 1;
			isblack[i] = 0;
		}
	}
	return redraw;
}

/*********************************************************************
  draw_backdrop

  This is an experimental backdrop blitter.  How to use:
  1)  Draw the dirty background video game graphic with no transparency.
  2)  Call draw_backdrop with a clipping rectangle containing the location
      of the dirty_graphic.

  draw_backdrop will fill in everything that's colored black with the
  backdrop.
 *********************************************************************/

void draw_backdrop(struct osd_bitmap *dest,const struct osd_bitmap *src,int sx,int sy,
					const struct rectangle *clip)
{
	int ox,oy,ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	/*  int col;
		int *sd4;
		int trans4,col4;*/
	struct rectangle myclip;

	if (!src) return;
	if (!dest) return;

    /* Rotate and swap as necessary... */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - src->width - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - src->height - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + src->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + src->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

    /* VERY IMPORTANT to mark this rectangle as dirty! :) - MAB */
	osd_mark_dirty (sx,sy,ex,ey,0);

	start = sy-oy;
	dy = 1;

	for (y = sy;y <= ey;y++)
	{
		bm = dest->line[y];
		bme = bm + ex;
		sd = src->line[start] + (sx-ox);
		for( bm = bm+sx ; bm <= bme ; bm++ )
		{
			if (isblack[*bm])
				*bm = *sd;
			sd++;
		}
		start+=dy;
	}
}


/*********************************************************************
  drawgfx_backdrop

  This is an experimental backdrop blitter.  How to use:

  Every time you want to draw a background tile, instead of calling
  drawgfx, call this and pass in the backdrop bitmap.  Wherever the
  tile is black, the backdrop will be drawn.
 *********************************************************************/
void drawgfx_backdrop(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,const struct osd_bitmap *back)
{
	int ox,oy,ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	/*int col;
	  int *sd4;
	  int trans4,col4;*/
	struct rectangle myclip;

	const unsigned char *sb;

	if (!gfx) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - gfx->width - sx;
		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - gfx->height - sy;
		if (clip)
		{
			int temp;


			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	/* start = code * gfx->height; */
	if (flipy)	/* Y flop */
	{
		start = code * gfx->height + gfx->height-1 - (sy-oy);
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height + (sy-oy);
		dy = 1;
	}


	if (gfx->colortable)	/* remap colors */
	{
		const unsigned short *paldata;	/* ASG 980209 */

		paldata = &gfx->colortable[gfx->color_granularity * color];

		if (flipx)	/* X flip */
		{
			for (y = sy;y <= ey;y++)
			{
				bm  = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
                sb = back->line[y] + sx;
				for( bm += sx ; bm <= bme ; bm++ )
				{
					if (isblack[paldata[*sd]])
                        *bm = *sb;
                    else
						*bm = paldata[*sd];
					sd--;
                    sb++;
				}
				start+=dy;
			}
		}
		else		/* normal */
		{
			for (y = sy;y <= ey;y++)
			{
				bm  = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + (sx-ox);
                sb = back->line[y] + sx;
				for( bm += sx ; bm <= bme ; bm++ )
				{
					if (isblack[paldata[*sd]])
                        *bm = *sb;
                    else
						*bm = paldata[*sd];
					sd++;
                    sb++;
				}
				start+=dy;
			}
		}
	}
	else
	{
		if (flipx)	/* X flip */
		{
			for (y = sy;y <= ey;y++)
			{
				bm = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + gfx->width-1 - (sx-ox);
                sb = back->line[y] + sx;
				for( bm = bm+sx ; bm <= bme ; bm++ )
				{
					if (isblack[*sd])
                        *bm = *sb;
                    else
						*bm = *sd;
					sd--;
                    sb++;
				}
				start+=dy;
			}
		}
		else		/* normal */
		{
			for (y = sy;y <= ey;y++)
			{
				bm = dest->line[y];
				bme = bm + ex;
				sd = gfx->gfxdata->line[start] + (sx-ox);
                sb = back->line[y] + sx;
				for( bm = bm+sx ; bm <= bme ; bm++ )
				{
					if (isblack[*sb])
                        *bm = *sb;
                    else
						*bm = *sd;
					sd++;
                    sb++;
				}
				start+=dy;
			}
		}
	}
}

/*********************************************************************
  overlay_draw

  This is an experimental backdrop blitter.  How to use:
  1)  Refresh all of your bitmap.
		- This is usually done with copybitmap(bitmap,tmpbitmap,...);
  2)  Call overlay_draw with the bitmap and the overlay.

  Not so tough, is it? :)

  Note: we don't have to worry about marking dirty rectangles here,
  because we should only have color changes in redrawn sections, which
  should already be marked as dirty by the original blitter.

  TODO: support translucency and multiple intensities if we need to.

 *********************************************************************/

void overlay_draw(struct osd_bitmap *dest,const struct artwork *overlay)
{
	int i,j;
	int height,width;
	struct osd_bitmap *o = NULL;
	int black;
	unsigned char *dst, *ovr;

	o = overlay->artwork;
	height = overlay->artwork->height;
	width = overlay->artwork->width;
	black = Machine->pens[0];

	for ( j=0; j<height; j++)
	{
		dst = dest->line[j];
		ovr = o->line[j];
		for (i=0; i<width; i++)
		{
			if (*dst!=black)
				*dst = *ovr;
			dst++;
			ovr++;
		}
	}
}


/*********************************************************************
  RGBtoHSV and HSVtoRGB

  This is overkill for now but maybe they come in handy later
  (Stolen from Foley's book)
 *********************************************************************/
static void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v )
{
	float min, max, delta;

	min = MIN( r, MIN( g, b ));
	max = MAX( r, MAX( g, b ));
	*v = max;

	delta = max - min;

	if( max != 0 )
		*s = delta / max;
	else {
		*s = 0;
		*h = 0;
		return;
	}

	if( r == max )
		*h = ( g - b ) / delta;
	else if( g == max )
		*h = 2 + ( b - r ) / delta;
	else
		*h = 4 + ( r - g ) / delta;

	*h *= 60;
	if( *h < 0 )
		*h += 360;
}

static void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
	int i;
	float f, p, q, t;

	if( s == 0 ) {
		*r = *g = *b = v;
		return;
	}

	h /= 60;
	i = h;
	f = h - i;
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
	case 0: *r = v; *g = t; *b = p; break;
	case 1: *r = q; *g = v; *b = p; break;
	case 2: *r = p; *g = v; *b = t; break;
	case 3: *r = p; *g = q; *b = v; break;
	case 4: *r = t; *g = p; *b = v; break;
	default: *r = v; *g = p; *b = q; break;
	}

}

/*********************************************************************
  transparency_hist

  Calculates a histogram of all transparent pixels in the overlay.
  The function returns a array of ints with the number of shades
  for each transparent color based on the color histogram.
 *********************************************************************/
static unsigned int *transparency_hist (struct artwork *a, int num_shades)
{
	int i, j;
	unsigned int *hist;
	int num_pix=0, min_shades;
	unsigned char pen;

	if ((hist = (unsigned int *)malloc(a->num_pens_trans*sizeof(unsigned int)))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Not enough memory!\n");
		return NULL;
	}
	memset (hist, 0, a->num_pens_trans*sizeof(int));

	for ( j=0; j<a->orig_artwork->height; j++)
		for (i=0; i<a->orig_artwork->width; i++)
		{
			pen = a->orig_artwork->line[j][i];
			if (pen < a->num_pens_trans)
			{
				hist[pen]++;
				num_pix++;
			}
		}

	/* we try to get at least 3 shades per transparent color */
	min_shades = ((num_shades-a->num_pens_used-3*a->num_pens_trans) < 0) ? 0 : 3;

	if (errorlog && (min_shades==0))
		fprintf(errorlog,"Too many colors in overlay. Vector colors may be wrong.\n");

	num_pix /= num_shades-(a->num_pens_used-a->num_pens_trans)
		-min_shades*a->num_pens_trans;

	for (i=0; i<a->num_pens_trans; i++)
		hist[i] = hist[i]/num_pix + min_shades;

	return hist;
}

/*********************************************************************
  overlay_set_palette

  Generates a palette for vector games with an overlay.

  The 'glowing' effect is simulated by alpha blending the transparent
  colors with a black (the screen) background. Then different shades
  of each transparent color are calculated by alpha blending this
  color with different levels of brightness (values in HSV) of the
  transparent color from v=0 to v=1. This doesn't work very well with
  blue. The number of shades is proportional to the number of pixels of
  that color. A look up table is also generated to map beam
  brightness and overlay colors to pens. If you have a beam brightness
  of 128 under a transparent pixel of pen 7 then
     Table (7,128)
  returns the pen of the resulting color. The table is usually
  converted to OS colors later.
 *********************************************************************/
int overlay_set_palette (struct artwork *a, unsigned char *palette, int num_shades)
{
	unsigned int i,j, shades=0, step;
	unsigned int *hist;
	float h, s, v, r, g, b;

	/* adjust palette start */

	palette += 3*a->start_pen;

	/* allocate the alpha LUT which gives us pens for a
	   given transparent color and beam intensity */

	if ((a->pTable = (unsigned char*)malloc(256*a->num_pens_trans))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Not enough memory for Talpha.\n");
		return 0;
	}
	if ((a->brightness = (unsigned char*)malloc(256))==NULL)
	{
		if (errorlog)
			fprintf(errorlog,"Not enough memory.\n");
		free (a->pTable);
		return 0;
	}

	if((hist = transparency_hist (a, num_shades))==NULL)
		return 0;

	/* Copy all artwork colors to the palette */
	memcpy (palette, a->orig_palette, 3*a->num_pens_used);

	/* Fill the palette with shades of the transparent colors */
	for (i=0; i<a->num_pens_trans; i++)
	{
		RGBtoHSV( a->orig_palette[i*3]/255.0,
			  a->orig_palette[i*3+1]/255.0,
			  a->orig_palette[i*3+2]/255.0, &h, &s, &v );

		/* blend transparent entries with black background */
		/* we don't need the original palette entry any more */
		HSVtoRGB ( &r, &g, &b, h, s, v*a->transparency[i]/255.0);
		palette [i*3+0] = r * 255.0;
		palette [i*3+1] = g * 255.0;
		palette [i*3+2] = b * 255.0;

		if (hist[i]>1)
		{
			for (j=0; j<hist[i]-1; j++)
			{
				/* we start from 1 because the 0 level is already in the palette */
				HSVtoRGB ( &r, &g, &b, h, s, v*a->transparency[i]/255.0 +
					   ((1-(v*a->transparency[i]/255.0))*(j+1))/(hist[i]-1));
				palette [(a->num_pens_used + shades + j)*3+0] = r * 255.0;
				palette [(a->num_pens_used + shades + j)*3+1] = g * 255.0;
				palette [(a->num_pens_used + shades + j)*3+2] = b * 255.0;
			}

			/* create alpha LUT for quick alpha blending */
			for (j=0; j<256; j++)
			{
				step = hist[i]*j/256.0;
				if (step == 0)
				/* no beam, just overlay over black screen */
					a->pTable[i*256+j] = i + a->start_pen;
				else
					a->pTable[i*256+j] = a->num_pens_used +
						shades + step-1 + a->start_pen;
			}

			shades += hist[i]-1;
		}
	}

	return 1;
}

/*********************************************************************
  overlay_remap

  This remaps the "original" palette indexes to the abstract OS indexes
  used by MAME.  This needs to be called during startup after the
  OS colors have been initialized (vh_start). It can only be called
  once because we have no backup for pTable and there is no need to
  call it more than once because we are dealing with a static palette.
 *********************************************************************/
void overlay_remap(struct artwork *a)
{
	int i,j;
	unsigned char r,g,b;

	int offset = a->start_pen;
	int height = a->artwork->height;
	int width = a->artwork->width;
	struct osd_bitmap *overlay = a->artwork;
	struct osd_bitmap *orig = a->orig_artwork;

	for ( j=0; j<height; j++)
		for (i=0; i<width; i++)
			overlay->line[j][i] =
				Machine->pens[orig->line[j][i]+offset];

	/* since the palette is static we can remap
	   pens in the alpha table to OS colors */

	for (i=0; i<256*a->num_pens_trans; i++)
		a->pTable[i]=Machine->pens[a->pTable[i]];

	/* Calculate brightness of all colors */

	for (i=0; i<256; i++)
	{
		osd_get_pen (i, &r, &g, &b);
		a->brightness[i]=(222*r+707*g+71*b)/1000;
	}
}

/*********************************************************************
  allocate_artwork_mem

  Allocates memory for all the bitmaps.
 *********************************************************************/
static struct artwork *allocate_artwork_mem (int width, int height)
{
	struct artwork *a;
	int temp;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		temp = height;
		height = width;
		width = temp;
	}

	a = (struct artwork *)malloc(sizeof(struct artwork));
	if (a == NULL)
	{
		if (errorlog) fprintf(errorlog,"Not enough memory for artwork!\n");
		return NULL;
	}

	a->transparency = NULL;
	a->orig_palette = NULL;
	a->pTable = NULL;
	a->brightness = NULL;
	a->vector_bitmap = NULL;

	if ((a->orig_artwork = osd_create_bitmap(width, height)) == 0)
	{
		if (errorlog) fprintf(errorlog,"Not enough memory for artwork!\n");
		free(a);
		return NULL;
	}
	fillbitmap(a->orig_artwork,0,0);

	/* Create a second bitmap for public use */
	if ((a->artwork = osd_create_bitmap(width,height)) == 0)
	{
		if (errorlog) fprintf(errorlog,"Not enough memory for artwork!\n");
		osd_free_bitmap(a->orig_artwork);
		free(a);
		return NULL;
	}

	/* Create bitmap for the vector screen */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		if ((a->vector_bitmap = osd_create_bitmap(width,height)) == 0)
		{
			if (errorlog) fprintf(errorlog,"Not enough memory for artwork!\n");
			osd_free_bitmap(a->orig_artwork);
			osd_free_bitmap(a->artwork);
			free(a);
			return NULL;
		}
		fillbitmap(a->vector_bitmap,0,0);
	}

	return a;
}

/*********************************************************************
  artwork_load

  This is what loads your backdrop in from disk.
  start_pen = the first pen available for the backdrop to use
  max_pens = the number of pens the backdrop can use
  So, for example, suppose you want to load "dotron.png", have it
  start with color 192, and only use 48 pens.  You would call
  backdrop = backdrop_load("dotron.png",192,48);
 *********************************************************************/

struct artwork *artwork_load(const char *filename, int start_pen, int max_pens)
{
	struct osd_bitmap *picture = NULL;
	struct artwork *a = NULL;

	if ((a = allocate_artwork_mem(Machine->scrbitmap->width, Machine->scrbitmap->height))==NULL)
		return NULL;

	a->start_pen = start_pen;

	/* Get original picture */
	if (png_read_artwork(filename, &picture, &a->orig_palette,
			     &a->num_pens_used, &a->transparency, &a->num_pens_trans) == 0)
	{
		artwork_free(a);
		return NULL;
	}

	/* Make sure we don't have too many colors */
	if (a->num_pens_used > max_pens)
	{
		if (errorlog) fprintf(errorlog,"Too many colors in overlay.\n");
		if (errorlog) fprintf(errorlog,"Colors found: %d  Max Allowed: %d\n",
				      a->num_pens_used,max_pens);
		artwork_free(a);
		osd_free_bitmap(picture);
		return NULL;
	}

	/* Scale the original picture to be the same size as the visible area */
	/*copybitmap(a->orig_artwork,picture,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);*/
	if (Machine->orientation & ORIENTATION_SWAP_XY)
		copybitmapzoom(a->orig_artwork,picture,0,0,0,0,
			       0, TRANSPARENCY_PEN, 0,
			       (a->orig_artwork->height<<16)/picture->height,
			       (a->orig_artwork->width<<16)/picture->width);
	else
		copybitmapzoom(a->orig_artwork,picture,0,0,0,0,
			       0, TRANSPARENCY_PEN, 0,
			       (a->orig_artwork->width<<16)/picture->width,
			       (a->orig_artwork->height<<16)/picture->height);

	/* If the game uses dynamic colors, we assume that it's safe
	   to init the palette and remap the colors now */
	if (Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
		backdrop_set_palette(a,a->orig_palette);

	/* We don't need the original any more */
	osd_free_bitmap(picture);

	return a;
}

/*********************************************************************
  artwork_create

  This works similar to artwork_load but generates artwork from
  an array of artwork_element. This is useful for very simple artwork
  like the overlay in the Space invaders series of games. The first
  entry determines the size of the temp bitmap which is used to create
  the artwork. All following entries should stay in these bounds. The
  temp bitmap is then scaled to the final size of the artwork (screen).
  The end of the array is marked by an entry with negative coordinates.
  If there are transparent and opaque overlay elements, the opaque ones
  have to be at the end of the list to stay compatible with the PNG
  artwork.
 *********************************************************************/
struct artwork *artwork_create(const struct artwork_element *ae, int start_pen)
{
	struct artwork *a;
	struct osd_bitmap *tmpbitmap=NULL;
	int pen;

	if ((a = allocate_artwork_mem(Machine->scrbitmap->width, Machine->scrbitmap->height))==NULL)
		return NULL;

	a->start_pen = start_pen;

	if ((a->orig_palette = (unsigned char *)malloc(256*3)) == NULL)
	{
		if (errorlog) fprintf(errorlog,"Not enough memory for overlay!\n");
		artwork_free(a);
		return NULL;
	}

	if ((a->transparency = (unsigned char *)malloc(256)) == NULL)
	{
		if (errorlog) fprintf(errorlog,"Not enough memory for overlay!\n");
		artwork_free(a);
		return NULL;
	}

	a->num_pens_used = 0;
	a->num_pens_trans = 0;

	while (ae->box.min_x >= 0)
	{
		/* look if the color is already in the palette */
		pen =0;
		while ((pen < a->num_pens_used) &&
		       ((ae->red != a->orig_palette[3*pen]) ||
				(ae->green != a->orig_palette[3*pen+1]) ||
				(ae->blue != a->orig_palette[3*pen+2])))
			pen++;

		if (pen == a->num_pens_used)
		{
			a->orig_palette[3*pen]=ae->red;
			a->orig_palette[3*pen+1]=ae->green;
			a->orig_palette[3*pen+2]=ae->blue;
			a->num_pens_used++;
			if (ae->alpha < 255)
			{
				a->transparency[pen]=ae->alpha;
				a->num_pens_trans++;
			}
		}

		if (!tmpbitmap)
		{
			/* create the bitmap with size of the first overlay element */
			if ((tmpbitmap = osd_create_bitmap(ae->box.max_x+1, ae->box.max_y+1))
			    == NULL)
			{
				if (errorlog) fprintf(errorlog,"Not enough memory for artwork!\n");
				return NULL;
			}
			fillbitmap(tmpbitmap,0,0);
		}
		else
			fillbitmap (tmpbitmap, pen, &ae->box);

		ae++;
	}

	/* Scale the original picture to be the same size as the visible area */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
		copybitmapzoom(a->orig_artwork,tmpbitmap,0,0,0,0,
					   0, TRANSPARENCY_PEN, 0,
					   (a->orig_artwork->height<<16)/tmpbitmap->height,
					   (a->orig_artwork->width<<16)/tmpbitmap->width);
	else
		copybitmapzoom(a->orig_artwork,tmpbitmap,0,0,0,0,
					   0, TRANSPARENCY_PEN, 0,
					   (a->orig_artwork->width<<16)/tmpbitmap->width,
					   (a->orig_artwork->height<<16)/tmpbitmap->height);

	osd_free_bitmap(tmpbitmap);

	/* If the game uses dynamic colors, we assume that it's safe
	   to init the palette and remap the colors now */
	if (Machine->drv->video_attributes & VIDEO_MODIFIES_PALETTE)
		backdrop_set_palette(a,a->orig_palette);

	return a;
}
