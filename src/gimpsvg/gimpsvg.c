/*
  gimpcpt.c

  (c) J.J.Green 2011, 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ggr.h>
#include <svg.h>
#include <svgwrite.h>
#include <btrace.h>

#include "gimpsvg.h"

#define ERR_SEGMENT_RGBA 1
#define ERR_NULL         2
#define ERR_INSERT       3

static int gimpsvg_convert(gradient_t*, svg_t*, gimpsvg_opt_t*);

static int gimpsvg3(gradient_t *gradient,
		    svg_t *svg,
		    const char *outfile,
		    gimpsvg_opt_t *opt)
{
  int err = gimpsvg_convert(gradient, svg, opt);

  if (err != 0)
    {
      switch (err)
	{
	case ERR_SEGMENT_RGBA:
	  btrace("error gretting colour from segment");
	  break;
	case ERR_NULL:
	  btrace("null structure");
	  break;
	case ERR_INSERT:
	  btrace("failed structure insert");
	  break;
	default:
	  btrace("unknown error");
	}
      return 1;
    }

  if (opt->verbose)
    printf("converted to %i stops\n", svg_num_stops(svg));

  if (svg_write(outfile, 1, (const svg_t**)(&svg), &(opt->preview)) != 0)
    {
      btrace("failed to write gradient to %s",
	     (outfile ? outfile : "<stdout>"));
      return 1;
    }

  return 0;
}

static int gimpsvg2(gradient_t *gradient,
		    const char *outfile,
		    gimpsvg_opt_t *opt)
{
  svg_t *svg = svg_new();

  if (svg == NULL)
    {
      btrace("failed to get new svg strcture");
      return 1;
    }

  int err = gimpsvg3(gradient, svg, outfile, opt);

  svg_destroy(svg);

  return err;
}

extern int gimpsvg(const char *infile,
		   const char *outfile,
		   gimpsvg_opt_t *opt)
{
  gradient_t *gradient = grad_load_gradient(infile);

  if (gradient == NULL)
    {
      btrace("failed to load gradient from %s",
	     (infile ? infile : "<stdin>"));
      return 1;
    }

  int err = gimpsvg2(gradient, outfile, opt);

  grad_free_gradient(gradient);

  return err;
}

#define EPSRGB   (0.5 / 256.0)
#define EPSALPHA 1e-4

static int grad_segment_jump(grad_segment_t *lseg, grad_segment_t *rseg)
{
  return ( (fabs(lseg->r1 - rseg->r0) > EPSRGB) ||
	   (fabs(lseg->g1 - rseg->g0) > EPSRGB) ||
	   (fabs(lseg->b1 - rseg->b0) > EPSRGB) ||
	   (fabs(lseg->a1 - rseg->a0) > EPSALPHA) );
}

static int gimpsvg_convert(gradient_t *grad,
			   svg_t *svg,
			   gimpsvg_opt_t *opt)
{
  if (!grad) return 1;

  strncpy((char *)svg->name, grad->name, SVG_NAME_LEN-1);

  svg_stop_t stop;
  double rgbD[3], alpha;
  grad_segment_t *gseg;

  for (gseg=grad->segments ; gseg ; gseg=gseg->next)
    {
      /* always insert the left colour */

      if (grad_segment_rgba(gseg->left, gseg, rgbD, &alpha) != 0)
	return ERR_SEGMENT_RGBA;

      rgbD_to_rgb(rgbD, &stop.colour);

      stop.value   = 100.0 * gseg->left;
      stop.opacity = alpha;

      if (svg_append(stop,svg) != 0) return ERR_INSERT;

      /* insert interior segments */

      if (gseg->type == GRAD_LINEAR && gseg->color == GRAD_RGB)
	{
	  if (grad_segment_rgba(gseg->middle, gseg, rgbD, &alpha) != 0)
	    return ERR_SEGMENT_RGBA;

	  rgbD_to_rgb(rgbD, &stop.colour);

	  stop.value = 100.0 * gseg->middle;
	  stop.opacity = alpha;

	  if (svg_append(stop, svg) != 0) return ERR_INSERT;
	}
      else
	{
	  /*
	    when the segment is non-linear and/or is not RGB, we
	    divide the segment up into small subsegments and write
	    the linear approximations.
	  */

	  int m,i;
	  double width;

	  width = gseg->right - gseg->left;
	  m = (int)(opt->samples*width) + 1;

	  for (i=1 ; i<m ; i++)
	    {
	      double z = gseg->left + i*width/m;

	      if (grad_segment_rgba(z, gseg, rgbD, &alpha) != 0)
		return ERR_SEGMENT_RGBA;

	      rgbD_to_rgb(rgbD, &stop.colour);

	      stop.value   = 100.0 * z;
	      stop.opacity = alpha;

	      if (svg_append(stop, svg) != 0) return ERR_INSERT;
	    }
	}

      /*
	insert right stop if it is not the same as the
	left colour of the next segment
      */

      if ( (! gseg->next) || grad_segment_jump(gseg,gseg->next) )
	{
	  if (grad_segment_rgba(gseg->right, gseg, rgbD, &alpha) != 0)
	    return ERR_SEGMENT_RGBA;

	  rgbD_to_rgb(rgbD, &stop.colour);

	  stop.value   = 100.0 * gseg->right;
	  stop.opacity = alpha;

	  if (svg_append(stop,svg) != 0) return ERR_INSERT;
	}
    }

  return 0;
}
