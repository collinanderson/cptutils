/*
  pg.c
  Copyright (c) J.J. Green 2016
*/

#include <stddef.h>

#include "pg.h"

struct pg_t
{
  int froob;
};

extern pg_t* pg_new(void)
{
  return NULL;
}


extern void pg_destroy(pg_t *pg)
{

}

extern int pg_write(pg_t *pg, const char *path)
{
  return 0;
}
