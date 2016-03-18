/*
  qgs.c
  Copyright (c) J.J. Green 2015
*/

#include <stdlib.h>
#include <string.h>

#include "qgs.h"

extern qgs_t* qgs_new(void)
{
  qgs_t *qgs;

  if ((qgs = malloc(sizeof(qgs_t))) == NULL)
    return NULL;

  qgs->type = QGS_TYPE_UNSET;
  qgs->name = NULL;
  qgs->n = 0;
  qgs->entries = NULL;

  return qgs;
}

extern int qgs_set_name(qgs_t* qgs, const char *name)
{
  if ((qgs->name = strdup(name)) == NULL)
    return 1;
  else
    return 0;
}

extern int qgs_set_type(qgs_t* qgs, int type)
{
  qgs->type = type;
  return 0;
}

extern int qgs_alloc_entries(qgs_t* qgs, size_t n)
{
  if ((qgs->entries = calloc(n, sizeof(qgs_entry_t))) == NULL)
    return 1;

  qgs->n = n;
  return 0;
}

extern void qgs_destroy(qgs_t* qgs)
{
  free(qgs->name);
  free(qgs->entries);
  free(qgs);
}

extern int qgs_set_entry(qgs_t* qgs, size_t i, qgs_entry_t *entry)
{
  if (i >= qgs->n) return 1;

  qgs_entry_t *dest = qgs->entries + i;

  memcpy(dest, entry, sizeof(qgs_entry_t));

  return 0;
}
