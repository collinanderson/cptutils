/*
  svgpreview.h

  specify a preview image in SVG output

  Copyright (c) J.J. Green 2012
*/

#ifndef SVGPREVIEW_H
#define SVGPREVIEW_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
  bool use;
  size_t width, height;
} svg_preview_t;

extern int svg_preview_geometry(const char*, svg_preview_t*);

#endif
