/*
  png structure for svgx
  J.J.Green
  $Id$
*/

#include "png.h"

extern png_t* png_new(size_t w, size_t h)
{
  png_t *png;

  if ((png = malloc(sizeof(png_t))) == NULL) return NULL;

  png->width  = w;
  png->height = h;

  png->row = malloc(4 * w);

  if (png->row == NULL) return NULL;
 
  return png;
}

extern void png_destroy(png_t* png)
{
  free(png->row);
  free(png);
}
