/*
  pg.h
  Copyright (c) J.J. Green 2016
*/

#ifndef PG_H
#define PG_H

#include <stdbool.h>
#include <colour.h>

typedef struct pg_t pg_t;

extern pg_t* pg_new(void);
extern void pg_destroy(pg_t*);
extern void pg_set_percentage(pg_t*, bool);
extern int pg_push(pg_t*, double, rgb_t, unsigned char);
extern int pg_write(pg_t*, const char*);

#endif
