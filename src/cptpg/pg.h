/*
  pg.h
  Copyright (c) J.J. Green 2016
*/

#ifndef PG_H
#define PG_H

typedef struct pg_t pg_t;

extern pg_t* pg_new(void);
extern void pg_destroy(pg_t*);

extern int pg_write(pg_t*, const char*);

#endif
