/*
  cptsvg.c

  (c) J.J.Green 2001,2005
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <btrace.h>
#include <cptread.h>
#include <svgwrite.h>

#include "cptsvg.h"

static int cptsvg_convert(cpt_t*,svg_t*,cptsvg_opt_t);

extern int cptsvg(char* infile, char* outfile, cptsvg_opt_t opt)
{
    cpt_t *cpt;
    svg_t *svg;
    int err = 0;

    if ((cpt = cpt_new()) != NULL)
      {
 	if (cpt_read(infile, cpt) == 0)
	  {
	    if ((svg = svg_new()) != NULL)
	      {
		if (cptsvg_convert(cpt, svg, opt) == 0)
		  {
		    if (svg_write(outfile, 1, 
				  (const svg_t**)(&svg), 
				  &(opt.preview)) == 0)
		      {
			/* success */
		      }
		    else
		      {
			btrace("error writing sgv struct");
			err = 1;
		      }
		  }
		else
		  {
		    btrace("failed to convert cpt to svg");
		    err = 1;
		  }

		svg_destroy(svg);
	      }
	    else
	      {
		btrace("failed to allocate svg");
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
      btrace("failed to write svg to %s", (outfile ? outfile : "<stdout>"));

    return err;
}

static int cptsvg_convert(cpt_t* cpt, svg_t* svg, cptsvg_opt_t opt)
{
  cpt_seg_t *seg;
  svg_stop_t lstop, rstop;
  double min, max;
  int n, m = 0;

  /* check we have at least one cpt segment */

  if (cpt->segment == NULL)
    {
      btrace("cpt has no segments");
      return 1;
    }

  /* copy the name */

  if (cpt->name)
    {
      if (snprintf((char*)svg->name, 
		   SVG_NAME_LEN, 
		   "%s", cpt->name) >= SVG_NAME_LEN)
	btrace("truncated svg name!");
    }

  /* find the min/max of the cpt range */

  min = cpt->segment->lsmp.val;

  max = cpt->segment->rsmp.val;
  for (seg=cpt->segment ; seg ; seg=seg->rseg)
    max = seg->rsmp.val;
  
  /* get the colour model */

  switch (cpt->model)
    {
    case model_rgb: 
    case model_hsv:
      break;
    case model_hsvp:
      btrace("conversion of colour model %s not implemented", 
	     model_name(cpt->model));
      return 1;
    default:
      btrace("unknown colour model");
      return 1;
    }

  /* convert cpt segments to svg stops*/

  for (n=0, seg=cpt->segment ; seg ; seg=seg->rseg)
    {
      cpt_sample_t lsmp, rsmp;
      rgb_t rcol, lcol;
      filltype_t type;

      lsmp = seg->lsmp;
      rsmp = seg->rsmp;

      if (lsmp.fill.type != rsmp.fill.type)
        {
          btrace("sorry, can't convert mixed fill types");
          return 1;
        }
            
      type = lsmp.fill.type;

      switch (type)
        {
        case fill_colour:
	  
          switch (cpt->model)
            {
            case model_rgb:
              lcol = lsmp.fill.u.colour.rgb;
              rcol = rsmp.fill.u.colour.rgb;
              break;
            case model_hsv:
	      if (hsv_to_rgb(lsmp.fill.u.colour.hsv, &lcol) != 0 ||
		  hsv_to_rgb(rsmp.fill.u.colour.hsv, &rcol) != 0)
		{
		  btrace("failed conversion of HSV to RGB");
		  return 1;
		}		
	      break;
            case model_hsvp:
	      /* not reached (see above) */
	      btrace("conversion not implemented");
              return 1;
            default:
              return 1;
            }
          break;
	  
        case fill_grey:
        case fill_hatch:
        case fill_file:
        case fill_empty:
	  /* fixme */
	  btrace("fill type not implemented yet");
          return 1;

        default:
          btrace("strange fill type");
          return 1;
        }

      /* always insert the left colour */

      lstop.value   = 100*(lsmp.val-min)/(max-min);
      lstop.colour  = lcol;
      lstop.opacity = 1.0; 

      if (svg_append(lstop, svg) == 0) m++;
      else 
	{
	  btrace("error adding stop for segment %i left", n);
	  return 1;
	}

      /*
	if there is a segment to the right, and if its left
	segment is the same colour at the our right segment
	then dont insert it. Otherwise do.
      */

      if ( ! ((seg->rseg) && 
	      fill_eq(rsmp.fill, seg->rseg->lsmp.fill, cpt->model)))
	{
	  rstop.value   = 100*(rsmp.val-min)/(max-min);
	  rstop.colour  = rcol;
	  rstop.opacity = 1.0; 

	  if (svg_append(rstop, svg) == 0) m++;
	  else
	    {
	      btrace("error adding stop for segment %i right", n);
	      return 1;
	    }
	}

      n++;
    }

  if (opt.verbose)
    printf("converted %i segments to %i stops", n, m);

  return 0;
}


