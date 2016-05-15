/*
  xycpt.c

  convert column data to cpt format

  (c) J.J.Green 2001, 2004, 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <cptwrite.h>
#include <cptname.h>
#include <btrace.h>

#include "xycpt.h"

typedef struct fill_stack_t
{
  fill_t fill;
  double val;
  struct fill_stack_t* next;
} fill_stack_t;

static int xycpt_convert(fill_stack_t*, cpt_t*, xycpt_opt_t);
static fill_stack_t* xyread(char*, xycpt_opt_t);

extern int xycpt(xycpt_opt_t opt)
{
  cpt_t* cpt;
  fill_stack_t* xy;

  xy = xyread(opt.file.input, opt);

  if (!xy)
    {
      btrace("failed to read data from %s",
	      (opt.file.input ?  opt.file.input : "<stdin>"));
      return 1;
    }

  if ((cpt = cpt_new()) == NULL)
    {
      btrace("failed to get new cpt strcture");
      return 1;
    }

  cpt->model = model_rgb;
  cpt->fg.type = cpt->bg.type = cpt->nan.type = fill_colour;
  cpt->bg.u.colour.rgb = opt.bg;
  cpt->fg.u.colour.rgb = opt.fg;
  cpt->nan.u.colour.rgb = opt.nan;

  if (opt.file.input)
    cpt->name = cptname(opt.file.input, "*");

  int err = 0;

  if (xycpt_convert(xy, cpt, opt) != 0)
    {
      btrace("failed to convert data");
      err++;
    }
  else
    {
      if (opt.verbose)
	{
	  int n = cpt_nseg(cpt);
	  printf("converted to %i segment rgb-spline\n", n);
	}

      if (cpt_write(opt.file.output, cpt) != 0)
	{
	  btrace("failed to write palette to %s",
		 (opt.file.output ? opt.file.output : "<stdout>"));
	  err++;
	}
    }

  cpt_destroy(cpt);

  return err;
}

static cpt_seg_t* cpt_seg_new_err(void);
static int cpt_append_err(cpt_seg_t*, cpt_t*);

static int xycpt_convert(fill_stack_t* fstack, cpt_t *cpt, xycpt_opt_t opt)
{
  fill_stack_t *f, *F;
  cpt_seg_t *seg;
  int n, i;

  /* dump the linked list into an array */

  for (f=fstack, n=0 ; f ; f=f->next, n++);

  if (n<2)
    {
      btrace("there is not enough data to make a palette!");
      return 1;
    }

  if ((F = malloc(n*sizeof(fill_stack_t))) == NULL)
    return 1;

  for (f=fstack, i=0 ; f ; f=f->next, i++) F[i] = *f;

  /* convert array to cpt structure */

  if (opt.discrete)
    {
      switch (opt.reg)
	{
	case reg_lower:

	  for (i=0 ; i<n-1 ; i++)
	    {
	      if ((seg = cpt_seg_new_err()) == NULL) return 1;

	      seg->lsmp.val  = F[i].val;
	      seg->lsmp.fill = F[i+1].fill;

	      seg->rsmp.val  = F[i+1].val;
	      seg->rsmp.fill = F[i+1].fill;

	      if (cpt_append_err(seg, cpt) != 0) return 1;
	    }
	  break;

	case reg_middle:

	  if ((seg = cpt_seg_new_err()) == NULL) return 1;

	  seg->lsmp.val  = F[0].val;
	  seg->lsmp.fill = F[0].fill;

	  seg->rsmp.val  = (F[0].val + F[1].val)/2.0;
	  seg->rsmp.fill = F[0].fill;

	  if (cpt_append_err(seg, cpt) != 0) return 1;

	  for (i=1 ; i<n-1 ; i++)
	    {
	      if ((seg = cpt_seg_new_err()) == NULL) return 1;

	      seg->lsmp.val  = (F[i-1].val+F[i].val)/2.0;
	      seg->lsmp.fill = F[i].fill;

	      seg->rsmp.val  = (F[i].val+F[i+1].val)/2.0;
	      seg->rsmp.fill = F[i].fill;

	      if (cpt_append_err(seg, cpt) != 0) return 1;
	    }

	  if ((seg = cpt_seg_new_err()) == NULL) return 1;

	  seg->lsmp.val  = (F[n-2].val+F[n-1].val)/2.0;
	  seg->lsmp.fill = F[n-1].fill;

	  seg->rsmp.val  = F[n-1].val;
	  seg->rsmp.fill = F[n-1].fill;

	  if (cpt_append_err(seg, cpt) != 0) return 1;

	  break;

	case reg_upper:

	  for (i=0 ; i<n-1 ; i++)
	    {
	      if ((seg = cpt_seg_new_err()) == NULL) return 1;

	      seg->lsmp.val  = F[i].val;
	      seg->lsmp.fill = F[i].fill;

	      seg->rsmp.val  = F[i+1].val;
	      seg->rsmp.fill = F[i].fill;

	      if (cpt_append_err(seg, cpt) != 0) return 1;
	    }
	  break;

	}
    }
  else
    {
      for (i=0 ; i<n-1 ; i++)
	{
	  if ((seg = cpt_seg_new_err()) == NULL) return 1;

	  seg->lsmp.val  = F[i].val;
	  seg->lsmp.fill = F[i].fill;

	  seg->rsmp.val  = F[i+1].val;
	  seg->rsmp.fill = F[i+1].fill;

	  if (cpt_append_err(seg, cpt) != 0) return 1;
	}
    }

  free(F);

  return 0;
}

/* error message versions of functions used in xycpt_convert */

static cpt_seg_t* cpt_seg_new_err(void)
{
  cpt_seg_t* seg;

  if ((seg = cpt_seg_new()) == NULL)
    {
      btrace("error creating segment");
      return NULL;
    }

  seg->annote = none;

  return seg;
}

static int cpt_append_err(cpt_seg_t* seg, cpt_t* cpt)
{
  if (cpt_append(seg, cpt) != 0)
    {
      btrace("error adding segment");
      return 1;
    }

  return 0;
}

/*
  handle stream choice
*/

static fill_stack_t* xyread_stream(FILE*, xycpt_opt_t);

static fill_stack_t* xyread(char* file, xycpt_opt_t opt)
{
  FILE* stream;
  fill_stack_t* xy;

  if (file)
    {
      if ((stream = fopen(file, "r")) == NULL)
	{
	  btrace("failed to open %s", file);
	  return NULL;
	}

      xy = xyread_stream(stream, opt);

      fclose(stream);
    }
  else
    xy = xyread_stream(stdin, opt);

  return xy;
}

/*
   read the column data

   we read the first data line and work out how many
   columns there are, then call a specific function
   depending on the value found. the i variant expect
   integers between 0 and 255, the f expects floats
   between 0 and 1
*/

#define BUFSIZE 1024
#define NTOK 4

static int skipline(const char*);

static fill_stack_t* xyread1i(FILE*, char*, int);
static fill_stack_t* xyread2i(FILE*, char*);
static fill_stack_t* xyread3i(FILE*, char*, int);
static fill_stack_t* xyread4i(FILE*, char*);

static fill_stack_t* xyread1f(FILE*, char*, int);
static fill_stack_t* xyread2f(FILE*, char*);
static fill_stack_t* xyread3f(FILE*, char*, int);
static fill_stack_t* xyread4f(FILE*, char*);

static int unital(const char*);
static int dchar(const char*);

static int colour8(double);

static fill_stack_t* xyread_stream(FILE* stream, xycpt_opt_t opt)
{
  char buf[BUFSIZE];
  char *tok[NTOK];
  int  i;
  fill_stack_t* f;

  /* chose the string to colour function */

  int (*atocol)(const char*) = (opt.unital ? unital : dchar);

  /* read first non-comment line */

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      {
	btrace("no first data line");
	return NULL;
      }
  while (skipline(buf));

  /* tokenise */

  if ((tok[0] = strtok(buf, " \t\n")) == NULL)
    {
      btrace("no tokens");
      return NULL;
    }

  for (i=1 ; i<NTOK ; i++)
    {
      if ((tok[i] = strtok(NULL, " \t")) == NULL)
	break;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  switch (i)
    {
    case 1:
      f->fill.type = fill_grey;
      f->val       = 0;

      f->fill.u.grey = atocol(tok[0]);

      f->next = (opt.unital ?
		 xyread1f(stream, buf, 1) :
		 xyread1i(stream, buf, 1));
      break;

    case 2:
      f->fill.type   = fill_grey;
      f->val         = atof(tok[0]);

      f->fill.u.grey = atocol(tok[1]);

      f->next = (opt.unital ?
		 xyread2f(stream, buf) :
		 xyread2i(stream, buf));
      break;

    case 3:
      f->fill.type = fill_colour;
      f->val       = 0;

      f->fill.u.colour.rgb.red   = atocol(tok[0]);
      f->fill.u.colour.rgb.green = atocol(tok[1]);
      f->fill.u.colour.rgb.blue  = atocol(tok[2]);

      f->next = (opt.unital ?
		 xyread3f(stream, buf, 1) :
		 xyread3i(stream, buf, 1));
      break;

    case 4:
      f->fill.type = fill_colour;
      f->val       = atof(tok[0]);

      f->fill.u.colour.rgb.red   = atocol(tok[1]);
      f->fill.u.colour.rgb.green = atocol(tok[2]);
      f->fill.u.colour.rgb.blue  = atocol(tok[3]);

      f->next = (opt.unital ?
		 xyread4f(stream, buf) :
		 xyread4i(stream, buf));

      break;

    default:
      btrace("bad input: found %i tokens", i);
      free(f);
      return NULL;
    }

  return f;
}

static fill_stack_t* xyread1i(FILE* stream, char* buf, int n)
{
  fill_stack_t* f;
  int i;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%d", &i) != 1)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = fill_grey;
  f->fill.u.grey = i;
  f->val         = n;
  f->next        = xyread1i(stream, buf, n+1);

  return f;
}

static fill_stack_t* xyread1f(FILE* stream, char* buf, int n)
{
  fill_stack_t* f;
  double d;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf", &d) != 1)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = fill_grey;
  f->fill.u.grey = colour8(d);
  f->val         = n;
  f->next        = xyread1f(stream, buf, n+1);

  return f;
}

static fill_stack_t* xyread2i(FILE* stream, char* buf)
{
  fill_stack_t* f;
  int i;
  double v;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf %d", &v, &i) != 2)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = fill_grey;
  f->fill.u.grey = i;
  f->val         = v;
  f->next        = xyread2i(stream, buf);

  return f;
}

static fill_stack_t* xyread2f(FILE* stream, char* buf)
{
  fill_stack_t* f;
  double v, d;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf %lf", &v, &d) != 2)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = fill_grey;
  f->fill.u.grey = colour8(d);
  f->val         = v;
  f->next        = xyread2f(stream, buf);

  return f;
}

static fill_stack_t* xyread3i(FILE* stream, char* buf, int n)
{
  fill_stack_t* f;
  int r, g, b;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%d %d %d", &r, &g, &b) != 3)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = fill_colour;
  f->val       = n;

  f->fill.u.colour.rgb.red   = r;
  f->fill.u.colour.rgb.green = g;
  f->fill.u.colour.rgb.blue  = b;

  f->next = xyread3i(stream, buf, n+1);

  return f;
}

static fill_stack_t* xyread3f(FILE* stream, char* buf, int n)
{
  fill_stack_t* f;
  double r, g, b;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf %lf %lf", &r, &g, &b) != 3)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = fill_colour;
  f->val       = n;

  f->fill.u.colour.rgb.red   = colour8(r);
  f->fill.u.colour.rgb.green = colour8(g);
  f->fill.u.colour.rgb.blue  = colour8(b);

  f->next = xyread3f(stream, buf, n+1);

  return f;
}

static fill_stack_t* xyread4i(FILE* stream, char* buf)
{
  fill_stack_t* f;
  int r, g, b;
  double z;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf %d %d %d", &z, &r, &g, &b) != 4)
    {
      btrace("bad line: %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = fill_colour;
  f->val       = z;

  f->fill.u.colour.rgb.red   = r;
  f->fill.u.colour.rgb.green = g;
  f->fill.u.colour.rgb.blue  = b;

  f->next = xyread4i(stream, buf);

  return f;
}

static fill_stack_t* xyread4f(FILE* stream, char* buf)
{
  fill_stack_t* f;
  double r, g, b;
  double z;

  do
    if (fgets(buf, BUFSIZE, stream) == NULL)
      return NULL;
  while (skipline(buf));

  if (sscanf(buf, "%lf %lf %lf %lf", &z, &r, &g, &b) != 4)
    {
      btrace("bad line:  %s", buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = fill_colour;
  f->val       = z;

  f->fill.u.colour.rgb.red   = colour8(r);
  f->fill.u.colour.rgb.green = colour8(g);
  f->fill.u.colour.rgb.blue  = colour8(b);

  f->next = xyread4f(stream, buf);

  return f;
}

static int skipline(const char* line)
{
  const char *s;

  if (line == NULL) return 1;

  s = line;

  do {
    switch (*s)
      {
      case '#':
      case '\n':
      case '\0':
	return 1;
      case ' ':
      case '\t':
	break;
      default:
	return 0;
      }
    s++;
  }
  while (true);
}

#define MAX(a, b) (((a)>(b)) ? (a) : (b))
#define MIN(a, b) (((a)>(b)) ? (b) : (a))

static int unital(const char* str)
{
  return colour8(atof(str));
}

static int colour8(double d)
{
  int i = nearbyint(255*d);

  i = MAX(i, 0);
  i = MIN(i, 255);

  return i;
}

/*
  "In general, there's no reason to use atoi in modern code"
  http://stackoverflow.com/questions/4694926/
*/

static int dchar(const char* str)
{
  return strtol(str, NULL, 10);
}
