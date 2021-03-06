/*
  ggr.h

  gradient structures

  This file contains code derived from from gradiant.c
  gradient.h and gradient_header.h, part of the Gimp
  distribution. Thanks to the Gimp people for making
  this available under the GPL.

  The file give access to the Gimp gradient structures
  and some io functionality. I have made a couple of
  changes from the original:
  * structs are now public
  * I removed the glib function calls
  * the pixmap field is missing (I'll look at this later)

  gimp/app/gradient.c,gradient.h,gradient_header.h,
  are copyright
    (c) 1995 Spencer Kimball andPeter Mattis,
    with modifications to the gradient editor
    module (c) 1996-1997 Federico Mena Quintero.
  The rest of this program is
    (c) 2001 J.J.Green (j.j.green@shef.ac.uk)

  This program is free software; you can redistribute it
  and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your
  option) any later version.

  This program is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURIGHTE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301 USA
*/

#ifndef GGR_H
#define GGR_H

#include "colour.h"

typedef enum
  {
    GRAD_LINEAR = 0,
    GRAD_CURVED,
    GRAD_SINE,
    GRAD_SPHERE_INCREASING,
    GRAD_SPHERE_DECREASING
  } grad_type_t;

typedef enum
  {
    GRAD_RGB = 0,   /* normal RGB */
    GRAD_HSV_CCW,   /* counterclockwise hue */
    GRAD_HSV_CW,    /* clockwise hue */
    GRAD_HSV_SHORT, /* shorter of cw & ccw hue */
    GRAD_HSV_LONG   /* longer of cw & ccw hue */
  } grad_color_t;

typedef enum
  {
    GRAD_FIXED = 0,
    GRAD_FG,
    GRAD_FG_TRANSP,
    GRAD_BG,
    GRAD_BG_TRANSP
  } grad_ect_t;

typedef struct grad_segment_t
{
  double       left, middle, right; /* Left pos, midpoint, right pos */
  double       r0, g0, b0, a0;      /* Left color */
  double       r1, g1, b1, a1;      /* Right color */
  grad_type_t  type;                /* Segment's blending function */
  grad_color_t color;               /* Segment's coloring type */
  grad_ect_t   ect_left, ect_right; /* Endpoint colour type */

  struct grad_segment_t *prev, *next; /* For linked list of segments */
} grad_segment_t;

typedef struct gradient_t
{
  char           *name;
  char           *filename;
  grad_segment_t *segments;
  grad_segment_t *last_visited;
} gradient_t;

extern grad_segment_t* seg_new_segment(void);

extern gradient_t*     grad_new_gradient(void);
extern void            grad_free_gradient(gradient_t*);
extern gradient_t*     grad_load_gradient(const char*);
extern int             grad_save_gradient(const gradient_t*, const char*);

extern int gradient_colour(double, gradient_t*, double*, double*);
extern int grad_segment_colour(double, const grad_segment_t*, double*, double*);
extern int grad_segment_rgba(double, const grad_segment_t*, double*, double*);

#endif
