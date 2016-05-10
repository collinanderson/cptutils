/*
  ident.c

  identitifiers

  (c) J. J. Green 2002
*/

#include <stdlib.h>
#include <string.h>

#include "ident.h"

extern ident_t* ident_new(char *name, int id)
{
  ident_t *ident;

  if ((ident = malloc(sizeof(ident_t))) != NULL)
    {
      char *dup;

      if ((dup = strdup(name)) != NULL)
	{
	  ident->name = dup;
	  ident->id   = id;

	  return ident;
	}
    }

  return NULL;
}

extern void ident_destroy(ident_t *ident)
{
  free(ident->name);
  free(ident);
}
