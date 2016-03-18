/*
  qgs.c
  Copyright (c) J.J. Green 2015
*/

#include <stdlib.h>
#include <string.h>

#include "qgs.h"

extern qgs_t* qgs_new(int type, size_t n)
{
  qgs_t *qgs;

  if ((qgs = malloc(sizeof(qgs_t))) != NULL)
    {
      qgs->type = type;

      if ((qgs->entries = calloc(n, sizeof(qgs_entry_t))) != NULL)
	{
	  qgs->n = n;
	  return qgs;
	}

      free(qgs);
    }

  return NULL;
}

extern void qgs_destroy(qgs_t* qgs)
{
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
