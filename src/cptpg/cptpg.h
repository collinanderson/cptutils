/*
  cptpg.h
  Copyright (c) J.J. Green 2016
*/

#ifndef CPTPG_H
#define CPTPG_H

typedef struct cptpg_opt_t
{
  int verbose;
} cptpg_opt_t;

extern int cptpg(const char*, const char*, cptpg_opt_t);

#endif
