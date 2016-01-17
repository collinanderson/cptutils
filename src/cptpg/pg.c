/*
  pg.c
  Copyright (c) J.J. Green 2016
*/

#include <stddef.h>
#include <math.h>

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

static int leq_last(pg_stop_t *stop, double *last)
{
  if (stop->value <= *last)
    {
      *last = stop->value;
      return 1;
    }
  else
    return -1;
}

static int geq_last(pg_stop_t *stop, double *last)
{
  if (stop->value >= *last)
    {
      *last = stop->value;
      return 1;
    }
  else
    return -1;
}

static bool pg_is_decreasing(pg_t *pg)
{
  double last = INFINITY;
  return gstack_foreach(pg->stack,
			(int (*)(void*, void*))leq_last,
			(void*)&last) == 0;
}

static bool pg_is_increasing(pg_t *pg)
{
  double last = -INFINITY;
  return gstack_foreach(pg->stack,
			(int (*)(void*, void*))geq_last,
			(void*)&last) == 0;
}

static int pg_coerce_increasing(pg_t *pg)
{
  if (pg_is_increasing(pg))
    return 0;

  if (pg_is_decreasing(pg))
    {
      gstack_reverse(pg->stack);
      return 0;
    }

  btrace("neither increasing nor decreasing, corrupt cpt?");

  return 1;
}

typedef struct
{
  double min, max;
} range_t;

static int get_range(pg_stop_t *stop, range_t *range)
{
  if (stop->value > range->max)
    range->max = stop->value;

  if (stop->value < range->min)
    range->min = stop->value;

  return 1;
}

typedef struct
{
  double m, c;
} affine_t;

static int affine_transform(pg_stop_t *stop, affine_t *affine)
{
  stop->value = affine->m * stop->value + affine->c;
  return 1;
}

static int pg_coerce_range(pg_t *pg, double min, double max)
{
  range_t range = {
    .max = -INFINITY,
    .min = INFINITY
  };

  if (gstack_foreach(pg->stack,
		     (int (*)(void*, void*))get_range,
		     (void*)&range) != 0)
    {
      btrace("failed to get range");
      return 1;
    }

  double
    m = (max - min) / (range.max - range.min),
    c = min - range.min * m;
  affine_t affine = { .m = m, .c = c };

  if (gstack_foreach(pg->stack,
		     (int (*)(void*, void*))affine_transform,
		     (void*)&affine) != 0)
    {
      btrace("failed to tranform values");
      return 1;
    }

  return 0;
}

extern int pg_write_stream(pg_t *pg, FILE* st)
{
  if (pg_coerce_increasing(pg) != 0)
    {
      btrace("failed to coerce increasing");
      return 1;
    }

  pg_stop_t stop;
  const char *format;

  if (pg->percentage)
    {
      if (pg_coerce_range(pg, 0, 100) != 0)
	{
	  btrace("failed to coerce into range [0, 100]");
	  return 1;
	}
      format = "%7.3f%% %3i %3i %3i %u\n";
    }
  else
    format = "%-7g %3i %3i %3i %u\n";

  while (gstack_pop(pg->stack, &stop) == 0)
    {
      fprintf(st, format,
	      stop.value,
	      stop.rgb.red,
	      stop.rgb.green,
	      stop.rgb.blue,
	      stop.alpha);
    }

  return 0;
}
