/*
  cptqml.h
  convert GMT colour table (cpt) to QGis style sheet (qml)
  Copyright (c) J.J. Green 2015
*/

#ifndef CPTQML_H
#define CPTQML_H

#include <stdbool.h>

typedef struct
{
  bool verbose;
  struct
  {
    char *input, *output;
  } file;
} cptqml_opt_t;

extern int cptqml(cptqml_opt_t);

#endif
