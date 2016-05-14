/*
  png structure for svgx
  J.J.Green
*/

#include <png.h>
#include <btrace.h>
#include "pngwrite.h"

static int png_write3(png_structp pngH,
		      png_infop infoH,
		      FILE *stream,
		      png_t *png,
		      const char* name)
{
  /*
    initialize rows of PNG, assigning each to the same
    simple row
  */

  png_byte **rows = malloc(png->height * sizeof(png_byte*));

  if (rows == NULL)
    {
      btrace("failed to allocate memory");
      return 1;
    }

  for (size_t j = 0 ; j < png->height ; j++)
    rows[j] = png->row;

  png_init_io(pngH, stream);
  png_set_rows(pngH, infoH, rows);
  png_write_png(pngH, infoH, PNG_TRANSFORM_IDENTITY, NULL);

  /* tidy up */

  free(rows);

  return 0;
}

static int png_write2(png_structp pngH,
		      png_infop infoH,
		      const char *path,
		      png_t *png,
		      const char* name)
{
  if (setjmp(png_jmpbuf(pngH)))
    {
      btrace("failed to set lonjump");
      return 1;
    }

  /* Set image attributes. */

  png_set_IHDR(pngH,
	       infoH,
	       png->width,
	       png->height,
	       8,
	       PNG_COLOR_TYPE_RGBA,
	       PNG_INTERLACE_NONE,
	       PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);

  /*
    since all rows are identical, using PNG_FILTER_UP
    gives us better compression
  */

  png_set_filter(pngH, PNG_FILTER_TYPE_BASE, PNG_FILTER_UP);

  /* Write the image data */

  FILE *st;

  if ((st = fopen(path, "wb")) == NULL)
    return 1;

  int err = png_write3(pngH, infoH, st, png, name);

  fclose(st);

  return err;
}

extern int png_write(const char *path, png_t *png, const char* name)
{
  png_structp pngH = NULL;
  png_infop infoH = NULL;
  int err = 0;

  pngH = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (pngH == NULL)
    {
      btrace("failed to create png write struct");
      err++;
    }
  else
    {
      if ((infoH = png_create_info_struct(pngH)) == NULL)
	{
	  btrace("failed to create png info struct");
	  err++;
	}
      else
	{
	  err = png_write2(pngH, infoH, path, png, name);
	}

      png_destroy_write_struct(&pngH, &infoH);
    }

  return err;
}
