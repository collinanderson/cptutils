/*
  qscm.c
  Copyright (c) J.J. Green 2015
*/

#include <stdlib.h>
#include <string.h>

#include "qscm.h"

extern qscm_t* qscm_new(int type, size_t n)
{
  qscm_t *qscm;

  if ((qscm = malloc(sizeof(qscm_t))) != NULL)
    {
      qscm->type = type;

      if ((qscm->entries = calloc(n, sizeof(qscm_entry_t))) != NULL)
	{
	  qscm->n = n;
	  return qscm;
	}

      free(qscm);
    }

  return NULL;
}

extern void qscm_destroy(qscm_t* qscm)
{
  free(qscm->entries);
  free(qscm);
}

extern int qscm_set_entry(qscm_t* qscm, size_t i, qscm_entry_t *entry)
{
  if (i >= qscm->n) return 1;

  qscm_entry_t *dest = qscm->entries + i;

  memcpy(dest, entry, sizeof(qscm_entry_t));

  return 0;
}
