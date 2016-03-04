/*
  svgpreview.h

  specify a preview image in SVG output

  Copyright (c) J.J. Green 2012
*/

#include <stdio.h>
#include <string.h>

#include "svgpreview.h"

/*
  scan a string of the form <double>[x<double>] and fill in 
  the geometry section of the svg preview struct. the 
  preview->use should be true, else the string will not 
  be inspected (it might be NULL).
*/

extern int svg_preview_geometry(const char *geom, svg_preview_t *preview)
{
  if (! preview->use) return 0;
  
  switch ( sscanf(geom, "%zux%zu", &(preview->width), &(preview->height)) )
    {
    case 1:
      preview->height = preview->width;
      break;
    case 2:
      break;
    default:
      return 1;
    };   
  
  return 0;
}
