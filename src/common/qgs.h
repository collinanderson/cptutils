/*
  qgcm.h

  QGIS style colour-map

  Copyright (c) J.J. Green 2015
*/

#ifndef QGS_H
#define QGS_H

#include <stdlib.h>

#include "colour.h"

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
} qgs_t;

extern qgs_t* qgs_new(int type, size_t n);
extern void qgs_destroy(qgs_t* qgs);
extern int qgs_set_entry(qgs_t* qgs, size_t i, qgs_entry_t *entry);

#endif
