/*
  fill.h
  
  fills for gimpcpt

  (c) J.J.Green 2001,2004
  $Id: fill.c,v 1.2 2004/03/22 23:22:05 jjg Exp jjg $
*/

#include <stdio.h>
#include "fill.h"

extern int fill_eq(fill_t a,fill_t b)
{
  if (a.type != b.type) return 0;

  switch (a.type)
    {
    case empty : return 1;
    case grey :
      return (a.u.grey == b.u.grey);
    case hatch :
      return ((a.u.hatch.sign == b.u.hatch.sign) &&
	      (a.u.hatch.dpi == b.u.hatch.dpi) &&
	      (a.u.hatch.n == b.u.hatch.n));
    case file : /* fixme */
      return 0;
    case colour :
      return colour_rgb_dist(a.u.colour,b.u.colour,rgb)<1e-8;
    }

  fprintf(stderr,"no such fill type\n");

  return 1;
}

extern int fill_interpolate(double z,fill_t a,fill_t b,fill_t *f)
{
  filltype_t type;

  if (a.type != b.type) 
    return 1;

  type = a.type;

  f->type = type;

  switch (type)
    {
    case empty : 
      break;
    case grey :
      f->u.grey = (a.u.grey*(1-z) + b.u.grey);
      break;
    case hatch :
    case file :
      *f = a;
      break;
    case colour :
      /* fixme */
      *f = a;
      break;
    }

  return 0;
}
