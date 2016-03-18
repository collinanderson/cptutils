/*
  qgs.h

  QGIS style colour-map

  Copyright (c) J.J. Green 2015
*/

#ifndef QGS_H
#define QGS_H

#include <stdlib.h>

#include "colour.h"

#define QGS_TYPE_UNSET 0
#define QGS_TYPE_DISCRETE 1
#define QGS_TYPE_INTERPOLATED 2

typedef struct {
  rgb_t rgb;
  unsigned char opacity;
  double value;
} qgs_entry_t;

typedef struct {
  size_t n;
  qgs_entry_t *entries;
  int type;
  char *name;
} qgs_t;

extern qgs_t* qgs_new(void);
extern void qgs_destroy(qgs_t *qgs);

extern int qgs_set_name(qgs_t *qgs, const char *name);
extern int qgs_set_type(qgs_t *qgs, int type);
extern int qgs_alloc_entries(qgs_t *qgs, size_t n);
extern int qgs_set_entry(qgs_t *qgs, size_t i, qgs_entry_t *entry);

#endif
