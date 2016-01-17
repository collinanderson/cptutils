/*
  pg.c
  Copyright (c) J.J. Green 2016
*/

#include <stddef.h>

#include <gstack.h>
#include <btrace.h>

#include "pg.h"

struct pg_t
{
  bool percentage;
  gstack_t *stack;
};

typedef struct
{
  double value;
  rgb_t rgb;
  unsigned char alpha;
} pg_stop_t;

extern pg_t* pg_new(void)
{
  pg_t *pg;

  if (NULL == (pg = malloc(sizeof(pg_t))))
    btrace("failed to allocate pg_t struct");
  else
    {
      if (NULL == (pg->stack = gstack_new(sizeof(pg_stop_t), 0, 16)))
	btrace("failed to allocate pg_t struct");
      else
	return pg;
      free(pg);
    }

  return NULL;
}

extern void pg_set_percentage(pg_t *pg, bool value)
{
  pg->percentage = value;
}

extern int pg_push(pg_t *pg, double value, rgb_t rgb, unsigned char alpha)
{
  pg_stop_t stop = {
    .value  = value,
    .rgb = rgb,
    .alpha = alpha
  };

  if (gstack_push(pg->stack, &stop) != 0)
    {
      btrace("failed push of stop");
      return 1;
    }

  return 0;
}

extern void pg_destroy(pg_t *pg)
{
  gstack_destroy(pg->stack);
  free(pg);
}

extern int pg_write_stream(pg_t*, FILE*);

extern int pg_write(pg_t *pg, const char *path)
{
  int err = 1;

  if (path)
    {
      FILE *st;

      if (NULL != (st = fopen(path, "w")))
	{
	  err = pg_write_stream(pg, st);
	  fclose(st);
	}
      else
	btrace("failed opening %s", path);
    }
  else
    {
      err = pg_write_stream(pg, stdout);
    }

  return err;
}

extern int pg_write_stream(pg_t *pg, FILE* st)
{
  pg_stop_t stop;

  while (gstack_pop(pg->stack, &stop) == 0)
    {
      fprintf(st, "%-7g %3i %3i %3i %u\n",
	      stop.value,
	      stop.rgb.red,
	      stop.rgb.green,
	      stop.rgb.blue,
	      stop.alpha);
    }

  return 0;
}
