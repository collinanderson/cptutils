/*
  cptpg.c
  Copyright (c) J.J. Green 2016
*/

#include <btrace.h>
#include <cpt.h>
#include <cptread.h>

#include "cptpg.h"
#include "pg.h"

static int cptpg_convert(cpt_t*, pg_t*, cptpg_opt_t);

extern int cptpg(const char *infile, const char *outfile, cptpg_opt_t opt)
{
    cpt_t *cpt;

    int err = 1;

    if ((cpt = cpt_new()) != NULL)
      {
        if (cpt_read(infile, cpt) == 0)
          {
	    pg_t *pg;

	    if ((pg = pg_new()) != NULL)
              {
                if (cptpg_convert(cpt, pg, opt) == 0)
                  {
                    if (pg_write(pg, outfile) == 0)
		      err = 0;
                    else
		      btrace("error writing pg");
                  }
                else
		  btrace("failed to convert cpt to svg");

                pg_destroy(pg);
              }
            else
	      btrace("failed to allocate pg");
          }
        else
	  btrace("failed to load cpt from %s",
		 (infile ? infile : "<stdin>"));

        cpt_destroy(cpt);
      }
    else
      btrace("failed to create cpt struct");

    if (err)
      btrace("failed to write svg to %s", (outfile ? outfile : "<stdout>"));

    return err;
}

static int cptpg_convert(cpt_t *cpt, pg_t *pg, cptpg_opt_t opt)
{
  /* check we have at least one cpt segment */

  if (cpt->segment == NULL)
    {
      btrace("cpt has no segments");
      return 1;
    }

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

  int m = 0, n;
  cpt_seg_t *seg;

  for (n = 0, seg = cpt->segment ; seg ; seg = seg->rseg)
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
          btrace("fill type not implemented yet");
          return 1;

        default:
          btrace("strange fill type");
          return 1;
        }

      /* always insert the left colour */

      if (pg_push(pg, lsmp.val, lcol, 255) == 0)
	m++;
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
	  if (pg_push(pg, rsmp.val, rcol, 255) == 0)
	    m++;
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
