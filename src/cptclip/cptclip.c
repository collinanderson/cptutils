/*
  cptclip.c

  (c) J.J.Green 2010
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cptread.h>
#include <cptwrite.h>
#include <btrace.h>

#include "cptclip.h"

static int cptclip_convert(cpt_t*,cptclip_opt_t);

extern int cptclip(char* infile,char* outfile,cptclip_opt_t opt)
{
  cpt_t *cpt;
  int err = 0;

  if ((cpt = cpt_new()) != NULL)
    {
      if (cpt_read(infile, cpt) == 0)
	{
	  if (cptclip_convert(cpt, opt) == 0)
	    {
	      if (cpt_nseg(cpt) > 0)
		{
		  if (cpt_write(outfile, cpt) == 0)
		    {
		      /* success */
		    }
		  else
		    {
		      btrace("error writing cpt struct");
		      err = 1;
		    }
		}
	      else
		{
		  btrace("clipped result has no segments");
		  err = 1;
		}
	    }
	  else
	    {
	      btrace("failed to convert");
	      err = 1;
	    }
	}
      else
	{
	  btrace("failed to load cpt from %s",
		 (infile ? infile : "<stdin>"));
	  err = 1;
	}
      
      cpt_destroy(cpt);    
    }	
  else
    {
      btrace("failed to create cpt struct");
      err = 1;
    }
  
  if (err)
    btrace("failed to write cpt to %s",
	   (outfile ? outfile : "<stdout>"));
  
  return err;
}

static int cptclip_z_inc(cpt_t*,cptclip_opt_t);
static int cptclip_z_dec(cpt_t*,cptclip_opt_t);

static int cptclip_convert(cpt_t* cpt,cptclip_opt_t opt)
{
  if (cpt->segment == NULL)
    {
      btrace("cpt has no segments");
      return 1;
    }

  if (opt.segments)
    {
      int i,nseg = cpt_nseg(cpt);

      for (i=1 ; i<opt.u.segs.min ; i++)
	{	
	  cpt_seg_t* s = cpt_pop(cpt);
	  cpt_seg_destroy(s);
	}

      for (i=nseg ; i>opt.u.segs.max ; i--)
	{
	  cpt_seg_t* s = cpt_shift(cpt);
	  cpt_seg_destroy(s);
	}

      return 0;
    }

  return (
	  cpt_increasing(cpt) ? 
	  cptclip_z_inc(cpt,opt) :
	  cptclip_z_dec(cpt,opt)
	  );
}

/*
  handles cptclip_z_inc and _dec by considering the target
  interval in the gradient's left-right frame of reference.
*/

static int cptclip_z_id(cpt_t* cpt,
			double zleft,
			double zright,
			bool (*left_of)(double,double),
			bool (*right_of)(double,double))
{
  cpt_seg_t* seg;

  /*
    pop segments from the left until one of them
    intersects the requested interval
  */

  while ((seg = cpt_pop(cpt)) != NULL)
    {
      if (right_of(seg->rsmp.val,zleft)) 
	break;

      cpt_seg_destroy(seg);
    }

  if (seg)
    {
      /*
	if this segment is not completely inside the
	requested interval then clip it to the interval
      */

      if (left_of(seg->lsmp.val,zleft))
	{
	  fill_t fill;
	  double z = 
	    (zleft - seg->lsmp.val)/
	    (seg->rsmp.val - seg->lsmp.val); 

	  if  (fill_interpolate(z,
				seg->lsmp.fill,
				seg->rsmp.fill,
				cpt->model,
				&fill) != 0)
            {
              btrace("fill interpolation failed");
              return 1;
            }

	  seg->lsmp.fill = fill;
	  seg->lsmp.val  = zleft;
	}

      /*
	push the (possibly modified) segment back to 
	where it came
      */

      cpt_prepend(seg,cpt);
    }

  /* likewise on the right */

  while ((seg = cpt_shift(cpt)) != NULL)
    {
      if (left_of(seg->lsmp.val,zright)) 
	break;
      
      cpt_seg_destroy(seg);
    }

  if (seg)
    {
      if (right_of(seg->rsmp.val,zright))
	{
	  fill_t fill;
	  double z = 
	    (zright - seg->lsmp.val)/ 
	    (seg->rsmp.val - seg->lsmp.val);

	  if  (fill_interpolate(z,
				seg->lsmp.fill,
				seg->rsmp.fill,
				cpt->model,
				&fill) != 0)
            {
              btrace("fill interpolation failed");
              return 1;
            }

	  seg->rsmp.fill = fill;
	  seg->rsmp.val  = zright;
	}

      cpt_append(seg,cpt);
    }

  return 0;
}

static bool lt(double a,double b){ return a < b; }
static bool gt(double a,double b){ return a > b; }

static int cptclip_z_inc(cpt_t* cpt,cptclip_opt_t opt)
{
  return cptclip_z_id(cpt,
		      opt.u.z.min,
		      opt.u.z.max,
		      lt,gt);
}

static int cptclip_z_dec(cpt_t* cpt,cptclip_opt_t opt)
{
  return cptclip_z_id(cpt,
		      opt.u.z.max,
		      opt.u.z.min,
		      gt,lt);
}

