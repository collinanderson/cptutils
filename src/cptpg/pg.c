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
  unsigned char red, green, blue, alpha;
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

extern int pg_push(pg_t *pg,
		   double value,
		   unsigned char red,
		   unsigned char green,
		   unsigned char blue,
		   unsigned char alpha)
{
  pg_stop_t stop = {
    .value  = value,
    .red = red,
    .blue = blue,
    .green = green,
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

extern int pg_write(pg_t *pg, const char *path)
{
  return 0;
}
