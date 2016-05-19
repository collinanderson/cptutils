/*
  svgxdump.h

  conversion of svg structures into other formats
  and dumping them into files.  This is where the
  work is done in svgx

  Copyright (c) J.J. Green 2012
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <btrace.h>
#include <cpt.h>
#include <cptwrite.h>
#include <ggr.h>
#include <povwrite.h>
#include <gptwrite.h>
#include <css3write.h>
#include <grd3write.h>
#include <saowrite.h>
#include <svgwrite.h>
#include <qgswrite.h>
#include <utf8.h>

// FIXME - move to common
#include "pngwrite.h"

#include "svgxdump.h"
#include "utf8x.h"

/*
  given a utf8 string, modify it in-place so that is is
  a reasonable filename on a modern Unix system.
*/

static int posixify(char *str)
{
  /* cannot have an empty string */

  if (*str == '\0')
    return 1;

  /* convert leading dot to underscore */

  if ((u8_seqlen(str) == 1) && (*str == '.'))
    *str = '_';

  /* convert forward slash to underscore */

  char *p = str;
  int cn;

  while ((p = u8_strchr(p, '/', &cn)) != NULL)
    *p = '_';

  return 0;
}

/*
  create a new opt with an autogenerated filename and call
  f(svg, opt), this is common code for the dump function in
  the case opt.job == job_all;
*/

static int call_autonamed(const svg_t *svg,
			  svgx_opt_t *opt,
			  const char *suffix,
			  int (*f)(const svg_t*, svgx_opt_t*))
{
  const char *name = (char*)svg->name;
  size_t n = strlen(name) + strlen(suffix) + 2;
  char file[n];

  if (snprintf(file, n, "%s.%s", name, suffix) >= n)
    {
      btrace("filename truncated! %s", file);
      return 1;
    }

  if (posixify(file) != 0)
    {
      btrace("failed to create POSIX filename");
      return 0;
    }

  svgx_opt_t opt2 = *opt;

  opt2.job = job_named;
  int err = 0;

  if (opt->output.file)
    {
      size_t m = strlen(opt->output.file) + 1 + n;
      char path[m];

      if (snprintf(path, m, "%s/%s", opt->output.file, file) >= m)
	{
	  btrace("filename truncated! %s", path);
	  return 1;
	}

      opt2.output.file = path;
      err = f(svg, &opt2);
      if (opt->verbose) printf("  %s\n", path);
    }
  else
    {
      opt2.output.file = file;
      err = f(svg, &opt2);
      if (opt->verbose) printf("  %s\n", file);
    }

  return err;
}

/*
  here there are blocks for each svgx_dump() function,
  which converts the svg structure to type x by a call
  to the file static svgx() function.  For some x there
  are a few helper functions too.

  Each block could (perhaps should) be moved into a
  seperate file.
*/

/* cpt */

static int svgcpt(const svg_t *svg, cpt_t *cpt)
{
  svg_node_t *node, *next;

  node = svg->nodes;
  next = node->r;

  while (next)
    {
      double z1, z2;

      z1 = node->stop.value;
      z2 = next->stop.value;

      if (z1 < z2)
	{
	  rgb_t c1, c2;
	  cpt_seg_t *seg;

	  c1 = node->stop.colour;
	  c2 = next->stop.colour;

	  if ((seg = cpt_seg_new()) == NULL)
	    {
	      btrace("failed to create cpt segment");
	      return 1;
	    }

	  seg->lsmp.val = z1;
	  seg->rsmp.val = z2;

	  seg->lsmp.fill.type         = fill_colour;
	  seg->lsmp.fill.u.colour.rgb = c1;

	  seg->rsmp.fill.type         = fill_colour;
	  seg->rsmp.fill.u.colour.rgb = c2;

	  if (cpt_append(seg, cpt) != 0)
	    {
	      btrace("failed to append segment");
	      return 1;
	    }
	}

      node = next;
      next = node->r;
    }

  return 0;
}

extern int svgcpt_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "cpt", svgcpt_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  cpt_t *cpt;

  if ((cpt = cpt_new()) == NULL)
    {
      btrace("failed to create cpt structure");
      return 1;
    }

  cpt->model = model_rgb;

  cpt->fg.type = cpt->bg.type = cpt->nan.type = fill_colour;

  cpt->bg.u.colour.rgb  = opt->format.cpt.bg;
  cpt->fg.u.colour.rgb  = opt->format.cpt.fg;
  cpt->nan.u.colour.rgb = opt->format.cpt.nan;

  cpt->name = strdup(name);

  int err = 0;

  if (svgcpt(svg, cpt) != 0)
    {
      btrace("failed to convert %s to cpt", name);
      err++;
    }
  else
    {
      if (cpt_write(file, cpt) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  cpt_destroy(cpt);

  return err;
}

/* ggr */

static int svgggr(const svg_t *svg, gradient_t *ggr)
{
  svg_node_t *node, *next;
  grad_segment_t *gseg, *prev=NULL;
  int n=0;

  ggr->name = strdup((char*)svg->name);

  node = svg->nodes;
  next = node->r;

  while (next)
    {
      double z1, z2;

      z1 = node->stop.value;
      z2 = next->stop.value;

      if (z1 < z2)
	{
	  rgb_t c1, c2;
	  double o1, o2;
	  double lcol[3], rcol[3];

	  c1 = node->stop.colour;
	  c2 = next->stop.colour;

	  o1 = node->stop.opacity;
	  o2 = next->stop.opacity;

	  if ((gseg = seg_new_segment()) == NULL) return 1;

	  gseg->prev = prev;

	  if (prev)
	    prev->next = gseg;
	  else
	    ggr->segments = gseg;

	  gseg->left   = z1/100.0;
	  gseg->middle = (z1+z2)/200.0;
	  gseg->right  = z2/100.0;

	  lcol[0] = lcol[1] = lcol[2] = 0.0;
	  rcol[0] = rcol[1] = rcol[2] = 0.0;

	  gseg->a0 = o1;
	  gseg->a1 = o2;

	  rgb_to_rgbD(c1, lcol);
	  rgb_to_rgbD(c2, rcol);

	  gseg->r0 = lcol[0];
	  gseg->g0 = lcol[1];
	  gseg->b0 = lcol[2];

	  gseg->r1 = rcol[0];
	  gseg->g1 = rcol[1];
	  gseg->b1 = rcol[2];

	  gseg->type  = GRAD_LINEAR;
	  gseg->color = GRAD_RGB;

	  gseg->ect_left = GRAD_FIXED;
	  gseg->ect_right = GRAD_FIXED;

	  prev = gseg;

	  n++;
	}

      node = next;
      next = node->r;
    }

  return 0;
}

extern int svgggr_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "ggr", svgggr_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  gradient_t *ggr;

  if ((ggr = grad_new_gradient()) == NULL)
    {
      btrace("failed to create ggr structure");
      return 1;
    }

  int err = 0;

  if (svgggr(svg, ggr) != 0)
    {
      btrace("failed to convert %s to cpt", name);
      err++;
    }
  else
    {
      if (grad_save_gradient(ggr, file) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  grad_free_gradient(ggr);

  return err;
}

/* povray */

static int svgpov_valid(const svg_t *svg, int permissive, int verbose)
{
  int m = svg_num_stops(svg);

  if (m > POV_STOPS_MAX)
    {
      if (permissive)
	{
	  if (verbose)
	    printf("warning : format limit broken %i stops (max is %i)\n",
		   m, POV_STOPS_MAX);
	}
      else
	{
	  btrace("format limit: POV-ray allows no more than %i stops, "
		 "but this gradient has %i", POV_STOPS_MAX, m);

	  return 0;
	}
    }

  if (m < 2)
    {
      btrace("found %i stops, but at least 2 required", m);
      return 0;
    }

  return 1;
}

static bool has_name(const svg_t *svg)
{
  return svg->name[0] != '\0';
}

static int svgpov(const svg_t *svg, pov_t *pov)
{
  int n, m, nmod;
  svg_node_t *node;

  /* count & allocate */

  m = svg_num_stops(svg);

  if (m < 2)
    {
      btrace("bad number of stops : %i", m);
      return 1;
    }

  if (pov_stops_alloc(pov, m) != 0)
    {
      btrace("failed alloc for %i stops", m);
      return 1;
    }

  /* convert */

  for (n=0, node = svg->nodes ; node ; n++, node = node->r)
    {
      pov_stop_t stop;
      rgb_t rgb;
      double c[3], t, z;

      rgb = node->stop.colour;

      if (rgb_to_rgbD(rgb, c) != 0)
	{
	  btrace("failed conversion to rgbD");
	  return 1;
	}

      t = 1.0 - node->stop.opacity;

      if ((t < 0.0) || (t > 1.0))
	{
	  btrace("bad value for transparency : %f", t);
	  return 1;
	}

      z = node->stop.value/100.0;

      if ((z < 0.0) || (z > 1.0))
	{
	  btrace("bad z value : %f", t);
	  return 1;
	}

      stop.z = z;

      stop.rgbt[0] = c[0];
      stop.rgbt[1] = c[1];
      stop.rgbt[2] = c[2];
      stop.rgbt[3] = t;

      pov->stop[n] = stop;
    }

  if (n != m)
    {
      btrace("missmatch between stops expected (%i) and found (%i)", m, n);
      return 1;
    }

  pov->n = n;

  /* povray names need to be alphanumeric ascii */

  if (has_name(svg))
    {
      char aname[SVG_NAME_LEN];

      if (utf8_to_x("ASCII", svg->name, aname, SVG_NAME_LEN) != 0)
	{
	  btrace("failed to convert name %s to ascii", aname);
	  return 1;
	}

      if (pov_set_name(pov, aname, &nmod) != 0)
	{
	  btrace("failed to assign povray name (%s)", aname);
	  return 1;
	}

      /* warn if name was modified */

      if (nmod > 0)
	btrace("name modified : %s to %s", svg->name, pov->name);
    }
  else
    {
      const char aname[] = "unassigned";

      if (pov_set_name(pov, aname, &nmod) != 0)
	{
	  btrace("failed to assign povray name (%s)", aname);
	  return 1;
	}
    }

  return 0;
}

extern int svgpov_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "inc", svgpov_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  pov_t *pov;

  if (! svgpov_valid(svg, opt->permissive, opt->verbose))
    {
      btrace("cannot create valid povray file");
      return 1;
    }

  if ((pov = pov_new()) == NULL)
    {
      btrace("failed to create pov structure");
      return 1;
    }

  int err = 0;

  if (svgpov(svg, pov) != 0)
    {
      btrace("failed to convert %s to pov", name);
      err++;
    }
  else
    {
      if (pov_write(file, pov) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  pov_destroy(pov);

  return err;
}

/* qgs */

static unsigned char unit_uchar(double x)
{
  return nearbyint(x * 255);
}

static void svg_stop_to_qgs_entry(svg_stop_t stop, qgs_entry_t *entry)
{
  entry->rgb = stop.colour;
  entry->opacity = unit_uchar(stop.opacity);
  entry->value = stop.value/100.0;
}

static qgs_t* svgqgs(const svg_t *svg)
{
  /* count & create */

  int m = svg_num_stops(svg);

  if (m < 2)
    {
      btrace("bad number of stops : %i", m);
      return NULL;
    }

  qgs_t *qgs;

  if ((qgs = qgs_new()) == NULL)
    {
      btrace("failed to allocate qgs");
      return NULL;
    }

  if (qgs_set_name(qgs, (const char*)svg->name) != 0)
    {
      btrace("failed to set name for qgs");
      return NULL;
    }

  if (qgs_set_type(qgs, QGS_TYPE_INTERPOLATED) != 0)
    {
      btrace("failed to set type for qgs");
      return NULL;
    }

  if (qgs_alloc_entries(qgs, m) != 0)
    {
      btrace("failed qgs allocate for %i stops", m);
      return NULL;
    }

  /* convert */

  svg_node_t *node;
  int n;

  for (n = 0, node = svg->nodes ; node ; n++, node = node->r)
    {
      qgs_entry_t entry;

      svg_stop_to_qgs_entry(node->stop, &entry);

      if (qgs_set_entry(qgs, n, &entry) != 0)
	{
	  btrace("failed to set qgs entry %zi", n);
	  return NULL;
	}
    }

  if (n != m)
    {
      btrace("missmatch between stops expected (%i) and found (%i)", m, n);
      return NULL;
    }

  return qgs;
}

extern int svgqgs_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "gpt", svggpt_dump);

  qgs_t *qgs;

  if ((qgs = svgqgs(svg)) == NULL)
    {
      btrace("failed to convert %s to qgs", (char*)svg->name);
      return 1;
    }

  const char *file = opt->output.file;
  int err = 0;

  if (qgs_write(file, qgs) != 0)
    {
      btrace("failed to write to %s", file);
      err++;
    }

  qgs_destroy(qgs);

  return err;
}

/* sao */

static int svgsao(const svg_t *svg, sao_t *sao)
{
  svg_node_t *node;
  int n=0;

  for (node = svg->nodes ; node ; node = node->r, n++)
    {
      double rgbD[3], val = node->stop.value / 100.0;

      if (rgb_to_rgbD(node->stop.colour, rgbD) != 0)
	{
	  btrace("error converting colour of stop %i", n+1);
	  return 1;
	}

      int err = 0;

      err += sao_red_push(sao, val, rgbD[0]);
      err += sao_green_push(sao, val, rgbD[1]);
      err += sao_blue_push(sao, val, rgbD[2]);

      if (err)
	{
	  btrace("error adding sao stop %i", n+1);
	  return 1;
	}
    }

  return 0;
}

extern int svgsao_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "sao", svgsao_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  sao_t *sao;

  if ((sao = sao_new()) == NULL)
    {
      btrace("failed to create sao structure");
      return 1;
    }

  int err = 0;

  if (svgsao(svg, sao) != 0)
    {
      btrace("failed to convert %s to sao", opt->name);
      err++;
    }
  else
    {
      if (sao_write(file, sao, name) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  sao_destroy(sao);

  return err;
}

/* gpt */

static int svggpt(const svg_t *svg, gpt_t *gpt)
{
  int n, m;
  svg_node_t *node;

  /* count & allocate */

  m = svg_num_stops(svg);

  if (m < 2)
    {
      btrace("bad number of stops : %i", m);
      return 1;
    }

  if (gpt_stops_alloc(gpt, m) != 0)
    {
      btrace("failed alloc for %i stops", m);
      return 1;
    }

  /* convert */

  for (n=0, node = svg->nodes ; node ; n++, node = node->r)
    {
      gpt_stop_t stop;
      rgb_t rgb;
      double c[3];

      rgb = node->stop.colour;

      if (rgb_to_rgbD(rgb, c) != 0)
	{
	  btrace("failed conversion to rgbD");
	  return 1;
	}

      stop.z = node->stop.value/100.0;

      stop.rgb[0] = c[0];
      stop.rgb[1] = c[1];
      stop.rgb[2] = c[2];

      gpt->stop[n] = stop;
    }

  if (n != m)
    {
      btrace("missmatch between stops expected (%i) and found (%i)", m, n);
      return 1;
    }

  gpt->n = n;

  return 0;
}

extern int svggpt_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "gpt", svggpt_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  gpt_t *gpt;

  if ((gpt = gpt_new()) == NULL)
    {
      btrace("failed to create gpt structure");
      return 1;
    }

  int err = 0;

  if (svggpt(svg, gpt) != 0)
    {
      btrace("failed to convert %s to gpt", name);
      err++;
    }
  else
    {
      if (gpt_write(file, gpt) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  gpt_destroy(gpt);

  return err;
}

/* css3 */

static int svgcss3(const svg_t *svg, css3_t *css3)
{
  int n, m;
  svg_node_t *node;

  /* count & allocate */

  m = svg_num_stops(svg);

  if (m < 2)
    {
      btrace("bad number of stops : %i", m);
      return 1;
    }

  if (css3_stops_alloc(css3, m) != 0)
    {
      btrace("failed alloc for %i stops", m);
      return 1;
    }

  /* convert */

  for (n=0, node = svg->nodes ; node ; n++, node = node->r)
    {
      css3_stop_t stop;

      stop.rgb   = node->stop.colour;
      stop.z     = node->stop.value;
      stop.alpha = node->stop.opacity;

      css3->stop[n] = stop;
    }

  if (n != m)
    {
      btrace("missmatch between stops expected (%i) and found (%i)", m, n);
      return 1;
    }

  css3->n = n;

  return 0;
}

extern int svgcss3_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "c3g", svgcss3_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  css3_t *css3;

  if ((css3 = css3_new()) == NULL)
    {
      btrace("failed to create css3 structure");
      return 1;
    }

  int err = 0;

  if (svgcss3(svg, css3) != 0)
    {
      btrace("failed to convert %s to css3", name);
      err++;
    }
  else
    {
      if (css3_write(file, css3) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  css3_destroy(css3);

  return err;
}

/* grd3 */

static double clampd(double z, double min, double max)
{
  if (z < min) z = min;
  if (z > max) z = max;

  return z;
}

static int clampi(int z, int min, int max)
{
  if (z < min) z = min;
  if (z > max) z = max;

  return z;
}

static int svggrd3(const svg_t *svg, grd3_t *grd3)
{
  int m, n;
  svg_node_t *node;
  grd3_rgbseg_t *pcseg;
  grd3_opseg_t *poseg;

  /* count & allocate */

  m = svg_num_stops(svg);

  if (m < 2)
    {
      btrace("bad number of stops : %i", m);
      return 1;
    }

  pcseg = malloc(m*sizeof(grd3_rgbseg_t));
  poseg = malloc(m*sizeof(grd3_opseg_t));

  if (! (pcseg && poseg))
    {
      btrace("failed to allocate segments");
      return 1;
    }

  /* convert segments */

  for (n=0, node = svg->nodes ; node ; n++, node = node->r)
    {
      rgb_t rgb;
      double op, z;

      rgb = node->stop.colour;
      op  = node->stop.opacity;
      z   = node->stop.value;

      pcseg[n].z        = clampd(4096*z/100.0, 0, 4096);
      pcseg[n].midpoint = 50;
      pcseg[n].r        = clampi(rgb.red*257,   0, 65535);
      pcseg[n].g        = clampi(rgb.green*257, 0, 65535);;
      pcseg[n].b        = clampi(rgb.blue*257,  0, 65535);;

      poseg[n].z        = clampd(4096*z/100.0, 0, 4096);
      poseg[n].midpoint = 50;
      poseg[n].opacity  = clampi(op*256, 0, 255);
    }

  /* copy name */

  char buffer[SVG_NAME_LEN];

  if (utf8_to_x("LATIN1", svg->name, buffer, SVG_NAME_LEN) != 0)
    {
      btrace("failed to convert utf name to latin1");
      return 1;
    }

  grd3->name = (unsigned char*)strdup(buffer);

  grd3->rgb.n   = m;
  grd3->rgb.seg = pcseg;

  grd3->op.n    = m;
  grd3->op.seg  = poseg;

  return 0;
}

extern int svggrd3_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "grd", svggrd3_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  grd3_t *grd3;

  if ((grd3 = grd3_new()) == NULL)
    {
      btrace("failed to create grd3 structure");
      return 1;
    }

  int err = 0;

  if (svggrd3(svg, grd3) != 0)
    {
      btrace("failed to convert %s to grd3", name);
      err++;
    }
  else
    {
      if (grd3_write(file, grd3) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  grd3_destroy(grd3);

  return err;
}

/* png */

static int svgpng(const svg_t *svg, png_t *png)
{

  size_t nz = png->width;
  unsigned char *row = png->row;
  int i;

  for (i=0 ; i<nz ; i++)
    {
      rgb_t rgb;
      double op, z = 100.0 * (double)i/(double)(nz-1);

      if (svg_interpolate(svg, z, &rgb, &op) != 0)
	{
	  btrace("failed svg interpolate at %.3g", z);
	  return 1;
	}

      op *= 256;

      if (op > 255) op = 255;

      row[4*i]   = rgb.red;
      row[4*i+1] = rgb.green;
      row[4*i+2] = rgb.blue;
      row[4*i+3] = (unsigned char)op;
    }

  return 0;
}

extern int svgpng_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "png", svgpng_dump);

  const char *name = (char*)svg->name;
  const char *file = opt->output.file;
  png_t *png;

  if ((png = png_new(opt->format.png.width,
		     opt->format.png.height)) == NULL)
    {
      btrace("failed to create png structure");
      return 1;
    }

  int err = 0;

  if (svgpng(svg, png) != 0)
    {
      btrace("failed to convert %s to png", name);
      err++;
    }
  else
    {
      if (png_write(file, png, name) != 0)
	{
	  btrace("failed to write to %s", file);
	  err++;
	}
    }

  png_destroy(png);

  return err;
}

/* svg */

extern int svgsvg_dump(const svg_t *svg, svgx_opt_t *opt)
{
  if (opt->job == job_all)
    return call_autonamed(svg, opt, "svg", svgsvg_dump);

  const char *file = opt->output.file;

  if (svg_write(file, 1,
		(const svg_t**)(&svg),
		&(opt->format.svg.preview)) != 0)
    {
      btrace("failed to write to %s", file);
      return 1;
    }

  return 0;
}
