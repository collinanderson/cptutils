/*
  css3write.c

  write a gnuplot colour map struct to
  a file or stream.

  J.J.Green 2010
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "css3.h"
#include "btrace.h"

extern int css3_write(const char* file,css3_t* css3)
{
  FILE *st;
  int n,i;

  n = css3->n;

  if ( n<2 )
    {
      btrace("CSS3 does not support %i stops\n", n);
      return 1;
    }

  if ( file )
    {
      st = fopen(file,"w");
      if (!st)
	{
	  btrace("failed to open %s : %s", strerror(errno));
	  return 1;
	}
    }
  else st = stdout;

  time_t t = time(NULL);

  fprintf(st,"/*\n");
  fprintf(st,"   CSS3 gradient\n");
  fprintf(st,"   cptutils %s\n",VERSION);
  fprintf(st,"   %s",ctime(&t));
  fprintf(st,"*/\n");
  fprintf(st,"\n");
  fprintf(st,"linear-gradient(\n");
  fprintf(st,"  %.3gdeg,\n",css3->angle);
  
  for (i=0 ; i<n ; i++)
    {
      css3_stop_t stop = css3->stop[i];

      if (stop.alpha < 1.0)
	fprintf(st,"  rgba(%3i,%3i,%3i,%5.3f) ",
		stop.rgb.red,
		stop.rgb.green,
		stop.rgb.blue,
		stop.alpha);
      else
	fprintf(st,"  rgb(%3i,%3i,%3i) ",
		stop.rgb.red,
		stop.rgb.green,
		stop.rgb.blue);

      fprintf(st,"%7.3f%%%s\n",stop.z,(n-1-i ? "," : ""));
    }

  fprintf(st,"  );\n");

  fclose(st);

  return 0;
}

