/* tilemap.h */

#define TILEMAP_TRANSPARENT		0x01
#define TILEMAP_SPLIT			0x02
/*
	TILEMAP_SPLIT should be used if the pixels from a single tile
	can appear in more than one plane.
*/

#define TILEMAP_BACK			0x10
#define TILEMAP_FRONT			0x20
/*
	when rendering a split layer, pass TILEMAP_FRONT or TILEMAP_BACK or'd with the
	tile_priority value to specify the part to draw.
*/

extern struct tile_info {
	unsigned char *pen_data; /* pointer to gfx data */
	unsigned short *pal_data; /* pointer to palette */
	unsigned int pen_usage;	/* used pens mask */
	/*
		you must set tile_info.pen_data, tile_info.pal_data and tile_info.pen_usage
		in the callback (usually with the SET_TILE_INFO() macro below).
		tile_info.flags and tile_info.priority will be automatically preset to 0,
		games that don't need them don't need to explicitly set them to 0
	*/
	unsigned char flags; /* see below */
	unsigned char priority;
} tile_info;

#define SET_TILE_INFO(GFX,CODE,COLOR) { \
	const struct GfxElement *gfx = Machine->gfx[(GFX)]; \
	int code = (CODE) % gfx->total_elements; \
	tile_info.pen_data = gfx->gfxdata->line[code*gfx->height]; \
	tile_info.pal_data = &gfx->colortable[gfx->color_granularity * (COLOR)]; \
	tile_info.pen_usage = gfx->pen_usage[code]; \
}

/* tile flags, set by get_tile_info callback */
#define TILE_FLIPX				0x01
#define TILE_FLIPY				0x02

#define TILE_SPLIT(T)			((T)<<2)
/* TILE_SPLIT is for use with TILEMAP_SPLIT layers.  It selects transparency type. */

#define TILE_FLIPYX(YX)			(YX)
/*
	TILE_FLIPYX is a shortcut that can be used by approx 80% of games,
	since yflip frequently occurs one bit higher than xflip within a
	tile attributes byte.
*/

struct tilemap {
	int type;
	int attributes;
	int transparent_pen;
	unsigned int transmask[4];

	int num_rows, num_cols, num_tiles;
	int tile_width, tile_height, width, height;

	void (*mark_visible)( int, int );
	void (*draw)( int, int );

	unsigned char **pendata;
	unsigned short **paldata;
	unsigned int *pen_usage;

	char *priority, **priority_row;
	char *visible, **visible_row;
	char *dirty_vram;
	char *dirty_pixels;
	unsigned char *flags;

	/* callback to interpret video VRAM for the tilemap */
	void (*tile_get_info)( int col, int row ); /* public */

	int scrolled;
	int scroll_rows, scroll_cols;
	int *rowscroll, *colscroll;

	struct rectangle clip;
	int clip_left,clip_right,clip_top,clip_bottom;

	/* cached color data */
	struct osd_bitmap *pixmap;
	int pixmap_line_offset;

	/* foreground mask */
	struct osd_bitmap *fg_mask;
	unsigned char *fg_mask_data;
	unsigned char **fg_mask_data_row;
	int fg_mask_line_offset;

	/* background mask (for the back half of a split layer) */
	struct osd_bitmap *bg_mask;
	unsigned char *bg_mask_data;
	unsigned char **bg_mask_data_row;
	int bg_mask_line_offset;
};

int tilemap_start( void );
void tilemap_stop( void );

struct tilemap *tilemap_create(
	int type,
	int tile_width, int tile_height, /* in pixels */
	int num_cols, int num_rows, /* in tiles */
	int scroll_rows, int scroll_cols
);

void tilemap_dispose( struct tilemap *tilemap );

void tilemap_mark_tile_dirty( struct tilemap *tilemap, int col, int row );
void tilemap_mark_all_tiles_dirty( struct tilemap *tilemap );
void tilemap_mark_all_pixels_dirty( struct tilemap *tilemap );

void tilemap_set_scrollx( struct tilemap *tilemap, int row, int value );
void tilemap_set_scrolly( struct tilemap *tilemap, int col, int value );

#define TILEMAP_FLIPX 0x1
#define TILEMAP_FLIPY 0x2
void tilemap_set_attributes( struct tilemap *tilemap, int attributes );

void tilemap_set_clip( struct tilemap *tilemap, const struct rectangle *clip );

void tilemap_update( struct tilemap *tilemap );
void tilemap_mark_palette( void );
void tilemap_render( struct tilemap *tilemap );
void tilemap_draw( struct osd_bitmap *dest, struct tilemap *tilemap, int priority );