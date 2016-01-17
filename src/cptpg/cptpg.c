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
  return 0;
}
