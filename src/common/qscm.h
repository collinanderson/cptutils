/*
  qgcm.h

  QGIS style colour-map

  Copyright (c) J.J. Green 2015
*/

#ifndef QSCM_H
#define QSCM_H

#include <stdlib.h>

#define QSCM_TYPE_DISCRETE 1
#define QSCM_TYPE_INTERPOLATED 2

typedef struct {
  unsigned char red, green, blue;
  double value;
} qscm_entry_t;

typedef struct {
  size_t n;
  qscm_entry_t *entries;
  int type;
} qscm_t;

extern qscm_t* qscm_new(int type, size_t n);
extern void qscm_destroy(qscm_t* qscm);
extern int qscm_set_entry(qscm_t* qscm, size_t i, qscm_entry_t *entry);

#endif
