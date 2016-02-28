/*
  agscpt.h

  (c) J.J.Green 2016
*/

#ifndef AGSCPT_H
#define AGSCPT_H

#include <stdbool.h>
#include <stdio.h>

#include "colour.h"

typedef enum {
  job_first,
  job_list,
  job_named,
  job_all
} agscpt_job_t;

typedef struct
{
  agscpt_job_t job;
  bool verbose;
  char *name;
  rgb_t fg, bg, nan;
  struct
  {
    char *input, *output;
  } file;
} agscpt_opt_t;

extern int agscpt(agscpt_opt_t);

#endif
