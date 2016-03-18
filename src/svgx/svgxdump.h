/*
  svgxdump.h

  per-format conversion of svg

  Copyright (c) J.J. Green 2012
*/

#ifndef SVGXDUMP_H
#define SVGXDUMP_H

#include "svg.h"
#include "svgx.h"

extern int svgcpt_dump(const svg_t*, svgx_opt_t*);
extern int svgggr_dump(const svg_t*, svgx_opt_t*);
extern int svgpov_dump(const svg_t*, svgx_opt_t*);
extern int svggpt_dump(const svg_t*, svgx_opt_t*);
extern int svgcss3_dump(const svg_t*, svgx_opt_t*);
extern int svggrd3_dump(const svg_t*, svgx_opt_t*);
extern int svgsao_dump(const svg_t*, svgx_opt_t*);
extern int svgpng_dump(const svg_t*, svgx_opt_t*);
extern int svgsvg_dump(const svg_t*, svgx_opt_t*);
extern int svgqgs_dump(const svg_t*, svgx_opt_t*);

#endif
