/*
  main.c 

  part of the gimpsvg package

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
  Free Software Foundation, Inc.,  51 Franklin Street, Fifth Floor, 
  Boston, MA 02110-1301 USA

  $Id: main.c,v 1.11 2012/01/22 20:15:47 jjg Exp jjg $
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "options.h"
#include "gimpsvg.h"
#include "colour.h"

#define SAMPLES_MIN 5

static int parse_minmax(char*,double*,double*);

int main(int argc,char** argv)
{
  struct gengetopt_args_info info;
  char *infile = NULL, *outfile = NULL;
  gimpsvg_opt_t opt;

  /* use gengetopt */

  if (options(argc,argv,&info) != 0)
    {
      fprintf(stderr,"failed to parse command line\n");
      return EXIT_FAILURE;
    }

  /* check arguments & transfer to opt structure */ 

  opt.verbose = info.verbose_given;
  opt.reverse = false;

  if ((opt.samples = info.samples_arg) < SAMPLES_MIN)
    {
      fprintf(stderr,"at least %i samples required\n",SAMPLES_MIN);
      opt.samples = SAMPLES_MIN;
    }

  /* null outfile for stdout */

  outfile = (info.output_given ? info.output_arg : NULL);

  if (!outfile && opt.verbose)
    {
      fprintf(stderr,"verbosity suppressed (<stdout> for results)\n");
      opt.verbose = 0;
    }

  /* null infile for stdin */

  switch (info.inputs_num)
    {
    case 0:
      infile = NULL;
      break;
    case 1:
      infile = info.inputs[0];
      break;
    default:
      fprintf(stderr,"Sorry, only one file at a time\n");
      return EXIT_FAILURE;
    }
  
  if (opt.verbose)
    printf("This is gimpsvg (version %s)\n",VERSION);

  int err;

  if ((err = gimpsvg(infile, outfile, opt)) != 0)
    {
      fprintf(stderr,"failed (error %i)\n",err);
      return EXIT_FAILURE;
    }

  if (opt.verbose)
    {
      printf("palette written to %s\n",(outfile ? outfile : "<stdin>"));
      printf("done.\n");
    }

  return EXIT_SUCCESS;
}
    	
    	
    	
    	
    	
    	
    	
    	
    	
    	
    	

