/*
  pspsvg.c

  convert paintshop pro gradients to the svg format

  (c) J.J. Green 2005,2006
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <grd3read.h>
#include <svgwrite.h>
#include <grdxsvg.h>
#include <gstack.h>
#include <btrace.h>

#include "pspsvg.h"

static int pspsvg_convert(grd3_t*, svg_t*, pspsvg_opt_t);

extern int pspsvg(pspsvg_opt_t opt)
{
  svg_t* svg;

  if ((svg = svg_new()) == NULL)
    {
      btrace("failed to get new svg strcture");
      return 1;
    }

  grd3_t* grd3;
  int err = 0;

  if ((grd3 = grd3_new()) == NULL)
    {
      btrace("failed to get new grd3 struture");
      err++;
    }
  else
    {
      if (grd3_read(opt.file.input, grd3) != 0)
	{
	  btrace("failed to read data from %s",
		 (opt.file.input ?  opt.file.input : "<stdin>"));
	  err++;
	}
      else
	{
	  if (pspsvg_convert(grd3, svg, opt) != 0)
	    {
	      btrace("failed to convert data");
	      err++;
	    }
	  else
	    {
	      if (svg_write(opt.file.output, 1, (const svg_t**)(&svg), &(opt.preview)) != 0)
		{
		  btrace("failed to write palette to %s",
			 (opt.file.output ? opt.file.output : "<stdout>"));
		  err++;
		}
	    }
	}
      grd3_destroy(grd3);
    }
  svg_destroy(svg);

  return err;
}

/* convert grd3 to intermediate types */

static double grd3_rgb_it(unsigned short x)
{
  return (double)x/65535.0;
}

static double grd3_op_it(unsigned short x)
{
  return (double)x/255.0;
}

static unsigned int grd3_z_it(unsigned short z)
{
  return (unsigned int)z*100;
}

static unsigned int grd3_zmid_it(unsigned short z0,
				unsigned short z1,
				unsigned short M)
{
  return (unsigned int)z0*100 + ((unsigned int)z1 - (unsigned int)z0)*M;
}


/* trim off excessive stops */

static int trim_rgb(gstack_t* stack)
{
  size_t n = gstack_size(stack);
  rgb_stop_t stop;
  gstack_t *stack0;

  if ((stack0 = gstack_new(sizeof(rgb_stop_t), n, n)) == NULL)
    return 1;

  while (! gstack_empty(stack))
    {
      gstack_pop(stack, &stop);
      gstack_push(stack0, &stop);

      if (stop.z >= 409600)
	{
	  while (! gstack_empty(stack))
	    gstack_pop(stack, &stop);
	}
    }

  while (! gstack_empty(stack0))
    {
      gstack_pop(stack0, &stop);
      gstack_push(stack, &stop);
    }

  return 0;
}

static int trim_op(gstack_t* stack)
{
  size_t n = gstack_size(stack);
  op_stop_t stop;
  gstack_t *stack0;

  if ((stack0 = gstack_new(sizeof(op_stop_t), n, n)) == NULL)
    return 1;

  while (! gstack_empty(stack))
    {
      gstack_pop(stack, &stop);
      gstack_push(stack0, &stop);

      if (stop.z >= 409600)
	{
	  while (! gstack_empty(stack))
	    gstack_pop(stack,&stop);
	}
    }

  while (! gstack_empty(stack0))
    {
      gstack_pop(stack0, &stop);
      gstack_push(stack, &stop);
    }

  return 0;
}

/*
   convert the grd3 stops to the intermediate types,
   and rectify -- replace the midpoints by explicit
   mid-point stops
*/

static int rectify_rgb2(grd3_t *grd3, gstack_t *stack)
{
  int n = grd3->rgb.n;
  grd3_rgbseg_t *pseg = grd3->rgb.seg;
  rgb_stop_t stop;

  for (int i = 0 ; i < n-1 ; i++)
    {
      stop.z = grd3_z_it(pseg[i].z);
      stop.r = grd3_rgb_it(pseg[i].r);
      stop.g = grd3_rgb_it(pseg[i].g);
      stop.b = grd3_rgb_it(pseg[i].b);

      if (gstack_push(stack, &stop) != 0)
	return 1;

      if (pseg[i].midpoint != 50)
	{
	  stop.z = grd3_zmid_it(pseg[i].z, pseg[i+1].z, pseg[i].midpoint);
	  stop.r = 0.5*(grd3_rgb_it(pseg[i].r) + grd3_rgb_it(pseg[i+1].r));
	  stop.g = 0.5*(grd3_rgb_it(pseg[i].g) + grd3_rgb_it(pseg[i+1].g));
	  stop.b = 0.5*(grd3_rgb_it(pseg[i].b) + grd3_rgb_it(pseg[i+1].b));

	  if (gstack_push(stack, &stop) != 0)
	    return 1;
	}
    }

  stop.z = grd3_z_it(pseg[n-1].z);
  stop.r = grd3_rgb_it(pseg[n-1].r);
  stop.g = grd3_rgb_it(pseg[n-1].g);
  stop.b = grd3_rgb_it(pseg[n-1].b);

  if (gstack_push(stack, &stop) != 0)
    return 1;

  /* add implicit final stop */

  if (stop.z < 409600)
    {
      stop.z = 409600;

      if (gstack_push(stack, &stop) != 0)
	return 1;
    }

  if (gstack_reverse(stack) != 0)
    return 1;

  if (trim_rgb(stack) != 0)
    return 1;

  return 0;
}

static gstack_t* rectify_rgb(grd3_t* grd3)
{

  int n = grd3->rgb.n;

  if (n<2)
    {
      btrace("input (grd) has %i rgb stop(s)", n);
      return NULL;
    }

  gstack_t *stack = gstack_new(sizeof(rgb_stop_t), 2*n, n);

  if (stack == NULL)
    return NULL;

  if (rectify_rgb2(grd3, stack) == 0)
    return stack;

  gstack_destroy(stack);
  return NULL;
}

static int rectify_op2(grd3_t *grd3, gstack_t *stack)
{
  int n = grd3->op.n;
  grd3_opseg_t *pseg = grd3->op.seg;
  op_stop_t stop;

  if (pseg[0].z > 0)
    {
      stop.z  = 0;
      stop.op = grd3_op_it(pseg[0].opacity);

      if (gstack_push(stack, &stop) != 0)
	return 1;
    }

  for (int i = 0 ; i < n-1 ; i++)
    {
      stop.z  = grd3_z_it(pseg[i].z);
      stop.op = grd3_op_it(pseg[i].opacity);

      if (gstack_push(stack, &stop) != 0)
	return 1;

      if (pseg[i].midpoint != 50)
	{
	  stop.z  = grd3_zmid_it(pseg[i].z, pseg[i+1].z, pseg[i].midpoint);
	  stop.op = 0.5*(grd3_op_it(pseg[i].opacity) +
			 grd3_op_it(pseg[i+1].opacity));

	  if (gstack_push(stack, &stop) != 0)
	    return 1;
	}
    }

  stop.z  = grd3_z_it(pseg[n-1].z);
  stop.op = grd3_op_it(pseg[n-1].opacity);

  if (gstack_push(stack, &stop) != 0)
    return 1;

  if (stop.z < 409600)
    {
      stop.z = 409600;
      if (gstack_push(stack, &stop) != 0)
	return 1;
    }

  if (gstack_reverse(stack) != 0)
    return 1;

  if (trim_op(stack) != 0)
    return 1;

  return 0;
}

static gstack_t* rectify_op(grd3_t *grd3)
{
  int n = grd3->op.n;

  if (n<2)
    {
      btrace("input (grd) has %i opacity stop(s)", n);
      return NULL;
    }

  gstack_t *stack = gstack_new(sizeof(op_stop_t), 2*n, n);

  if (stack == NULL)
    return NULL;

  if (rectify_op2(grd3, stack) == 0)
    return stack;

  gstack_destroy(stack);
  return NULL;
}

/*
   In grd3 files, names are non null-terminated
   arrays of unsigned char, but we store them as
   null-terminated arrays of unsigned char.  In
   the wild one sees the upper half of the range
   being used, and it seems to be latin-1.

   SVG uses utf8, so we need to convert our latin-1
   to it: the implementation taken from
   http://stackoverflow.com/questions/4059775
*/

static int latin1_to_utf8(const unsigned char *in,
			  unsigned char *out,
			  size_t lenout)
{
  size_t lenin = 0;
  const unsigned char* p;

  for (p=in ; *p ; p++) lenin++;

  if (2*lenin >= lenout)
    return 1;

  while (*in)
    {
      if (*in < 128)
	*out++ = *in++;
      else if (*in < 192)
	return 1;
      else
	{
	  *out++ = 0xc2 + (*in > 0xbf);
	  *out++ = (*in++ & 0x3f) + 0x80;
	}
    }

  return 0;
}

static int pspsvg_convert(grd3_t *grd3, svg_t *svg, pspsvg_opt_t opt)
{
  gstack_t *rgbrec,*oprec;

  if (latin1_to_utf8(grd3->name, svg->name, SVG_NAME_LEN) != 0)
    {
      btrace("failed latin1 to unicode name conversion");
      return 1;
    }

  if (opt.verbose)
    printf("processing \"%s\"\n", svg->name);

  if ((rgbrec = rectify_rgb(grd3)) == NULL)
    return 1;

  if ((oprec = rectify_op(grd3)) == NULL)
    return 1;

  if (opt.verbose)
    printf("stops: rgb %i/%zi, opacity %i/%zi\n",
	   grd3->rgb.n,
	   gstack_size(rgbrec),
	   grd3->op.n,
	   gstack_size(oprec));

  if (grdxsvg(rgbrec, oprec, svg) != 0)
    {
      btrace("failed conversion of rectified stops to svg");
      return 1;
    }

  gstack_destroy(rgbrec);
  gstack_destroy(oprec);

  return 0;
}
