/*
  xycpt.c

  convert column data to cpt format

  (c) J.J.Green 2001,2004
  $Id: xycpt.c,v 1.3 2004/04/12 15:21:52 jjg Exp jjg $
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "xycpt.h"
#include "cpt.h"
#include "cptio.h"

typedef struct fill_stack_t
{
  fill_t fill;
  double val;
  struct fill_stack_t* next;
} fill_stack_t;

static int xycpt_convert(fill_stack_t*,cpt_t*,xycpt_opt_t);
static fill_stack_t* xyread(char*);

extern int xycpt(xycpt_opt_t opt)
{
  cpt_t* cpt;
  fill_stack_t* xy;

  xy = xyread(opt.file.input);
    
  if (!xy)
    {
      fprintf(stderr,"failed to read data from %s\n",
	      (opt.file.input ?  opt.file.input : "<stdin>"));
      return 1;
    }
    
  /* create a cpt struct */

  if ((cpt = cpt_new()) == NULL)
    {
      fprintf(stderr,"failed to get new cpt strcture\n");
      return 1;
    }

  cpt->model = rgb;
  
  cpt->fg.type = cpt->bg.type = cpt->nan.type = colour;
  
  /*
    cpt->fg.u.colour.rgb  = opt.fg;
    cpt->bg.u.colour.rgb  = opt.bg;
    cpt->nan.u.colour.rgb = opt.nan;
  */

  strncpy(cpt->name,(opt.file.input ?  opt.file.input : "<stdin>"),CPT_NAME_LEN);

  /* transfer the gradient data to the cpt_t struct */

  if (xycpt_convert(xy,cpt,opt) != 0)
    {
      fprintf(stderr,"failed to convert data\n");
      return 1;
    }
  
  if (opt.verbose) 
    {
      int n = cpt_nseg(cpt);
      printf("converted to %i segment rgb-spline\n",n);
    }
  
  /* write the cpt file */
  
  if (cpt_write(opt.file.output,cpt) != 0)
    {
      fprintf(stderr,"failed to write palette to %s\n",
	      (opt.file.output ? opt.file.output : "<stdout>"));
      return 1;
    }
  
  /* tidy */
  
  cpt_destroy(cpt);
  
  return 0;
}

static cpt_seg_t* cpt_seg_new_err(void);
static int cpt_append_err(cpt_seg_t*,cpt_t*);

static int xycpt_convert(fill_stack_t* fstack,cpt_t *cpt,xycpt_opt_t opt)
{
  fill_stack_t *f,*F;
  cpt_seg_t *seg;
  int n,i;

  /* dump the linked list into an array */

  for (f=fstack,n=0 ; f ; f=f->next,n++); 

  if (n<2)
    {
      fprintf(stderr,"there is not enough data to make a palette!\n");
      return 1;
    }

  if ((F = malloc(n*sizeof(fill_stack_t))) == NULL)
    return 1;

  for (f=fstack,i=0 ; f ; f=f->next,i++) F[i] = *f;

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

	      if (cpt_append_err(seg,cpt) != 0) return 1;
	    }
	  break;

	case reg_middle:

	  if ((seg = cpt_seg_new_err()) == NULL) return 1;

	  seg->lsmp.val  = F[0].val;
	  seg->lsmp.fill = F[0].fill;

	  seg->rsmp.val  = (F[0].val + F[1].val)/2.0;
	  seg->rsmp.fill = F[0].fill;

	  if (cpt_append_err(seg,cpt) != 0) return 1;

	  for (i=1 ; i<n-1 ; i++)
	    {
	      if ((seg = cpt_seg_new_err()) == NULL) return 1;

	      seg->lsmp.val  = (F[i-1].val+F[i].val)/2.0;
	      seg->lsmp.fill = F[i].fill;

	      seg->rsmp.val  = (F[i].val+F[i+1].val)/2.0;
	      seg->rsmp.fill = F[i].fill;

	      if (cpt_append_err(seg,cpt) != 0) return 1;
	    }

	  if ((seg = cpt_seg_new_err()) == NULL) return 1;
	  
	  seg->lsmp.val  = (F[n-2].val+F[n-1].val)/2.0;
	  seg->lsmp.fill = F[n-1].fill;
	  
	  seg->rsmp.val  = F[n-1].val;
	  seg->rsmp.fill = F[n-1].fill;

	  if (cpt_append_err(seg,cpt) != 0) return 1;

	  break;

	case reg_upper:

	  for (i=0 ; i<n-1 ; i++)
	    {
	      if ((seg = cpt_seg_new_err()) == NULL) return 1;
  	      
	      seg->lsmp.val  = F[i].val;
	      seg->lsmp.fill = F[i].fill;
	      
	      seg->rsmp.val  = F[i+1].val;
	      seg->rsmp.fill = F[i].fill;

	      if (cpt_append_err(seg,cpt) != 0) return 1;
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

	  if (cpt_append_err(seg,cpt) != 0) return 1;
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
      fprintf(stderr,"error creating segment\n");
      return NULL;
    }

  seg->annote = none;

  return seg;
}

static int cpt_append_err(cpt_seg_t* seg,cpt_t* cpt)
{
  if (cpt_append(seg,cpt) != 0)
    {
      fprintf(stderr,"error adding segment\n");
      return 1;
    }

  return 0;
}

/*
  handle stream choice
*/ 

static fill_stack_t* xyread_stream(FILE*);

static fill_stack_t* xyread(char* file)
{
  FILE* stream;
  fill_stack_t* xy;

  if (file)
    {
      if ((stream = fopen(file,"r")) == NULL)
	{
	  fprintf(stderr,"failed to open %s\n",file);
	  return NULL;
	}

      xy = xyread_stream(stream);

      fclose(stream);
    }
  else
    xy = xyread_stream(stdin);

  return xy;
}

/* 
   read the column data 

   we read the first data line and work out how many
   columns there are, then call a specific function
   depending on the value found.
*/

#define BUFSIZE 1024
#define NTOK 4

static int skipline(const char*);

static fill_stack_t* xyread1(FILE*,char*,int);
static fill_stack_t* xyread2(FILE*,char*);
static fill_stack_t* xyread3(FILE*,char*,int);
static fill_stack_t* xyread4(FILE*,char*);

static fill_stack_t* xyread_stream(FILE* stream)
{
  char buf[BUFSIZE];
  char *tok[NTOK];
  int  i;
  fill_stack_t* f;

  /* read first non-comment line */

  do
    if (fgets(buf,BUFSIZE,stream) == NULL) 
      {
	fprintf(stderr,"no first data line\n");
	return NULL;
      }
  while (skipline(buf));
  
  /* tokenise */

  if ((tok[0] = strtok(buf," \t\n")) == NULL)
    {
      fprintf(stderr,"no tokens\n");
      return NULL;
    }

  for (i=1 ; i<NTOK ; i++)
    {
      if ((tok[i] = strtok(NULL," \t")) == NULL)
	break;
    }
 
  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  switch (i)
    {
    case 1:
      f->fill.type = grey;
      f->val       = 0;

      f->fill.u.grey = atoi(tok[0]);

      f->next = xyread1(stream,buf,1);
      break;

    case 2:
      f->fill.type   = grey;
      f->val         = atof(tok[0]);

      f->fill.u.grey = atoi(tok[1]);

      f->next = xyread2(stream,buf);
      break;

    case 3:
      f->fill.type = colour;
      f->val       = 0;

      f->fill.u.colour.rgb.red   = atoi(tok[0]);
      f->fill.u.colour.rgb.green = atoi(tok[1]);
      f->fill.u.colour.rgb.blue  = atoi(tok[2]);

      f->next = xyread3(stream,buf,1);
      break;
 
    case 4:
      f->fill.type = colour;
      f->val       = atof(tok[0]);

      f->fill.u.colour.rgb.red   = atoi(tok[1]);
      f->fill.u.colour.rgb.green = atoi(tok[2]);
      f->fill.u.colour.rgb.blue  = atoi(tok[3]);

      f->next = xyread4(stream,buf);

      break;

    default:
      fprintf(stderr,"bad input : found %i tokens\n",i);
      free(f);
      return NULL;
    }

  return f;
}

static fill_stack_t* xyread1(FILE* stream,char* buf,int n)
{
  fill_stack_t* f;
  int i;

  do
    if (fgets(buf,BUFSIZE,stream) == NULL) 
      return NULL;
  while (skipline(buf));
  
  if (sscanf(buf,"%i",&i) != 1)
    {
      fprintf(stderr,"bad line\n  %s",buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = grey;
  f->fill.u.grey = i;
  f->val         = n;
  f->next        = xyread1(stream,buf,n+1);

  return f;
}

static fill_stack_t* xyread2(FILE* stream,char* buf)
{
  fill_stack_t* f;
  int i;
  double v;

  do
    if (fgets(buf,BUFSIZE,stream) == NULL) 
      return NULL;
  while (skipline(buf));
  
  if (sscanf(buf,"%lf %i",&v,&i) != 2)
    {
      fprintf(stderr,"bad line\n  %s",buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type   = grey;
  f->fill.u.grey = i;
  f->val         = v;
  f->next        = xyread2(stream,buf);

  return f;
}

static fill_stack_t* xyread3(FILE* stream,char* buf,int n)
{
  fill_stack_t* f;
  int r,g,b;

  do
    if (fgets(buf,BUFSIZE,stream) == NULL) 
      return NULL;
  while (skipline(buf));
  
  if (sscanf(buf,"%i %i %i",&r,&g,&b) != 3)
    {
      fprintf(stderr,"bad line\n  %s",buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = colour;
  f->val       = n;

  f->fill.u.colour.rgb.red   = r;
  f->fill.u.colour.rgb.green = g;
  f->fill.u.colour.rgb.blue  = b;

  f->next = xyread3(stream,buf,n+1);

  return f;
}

static fill_stack_t* xyread4(FILE* stream,char* buf)
{
  fill_stack_t* f;
  int r,g,b;
  double z;

  do
    if (fgets(buf,BUFSIZE,stream) == NULL) 
      return NULL;
  while (skipline(buf));
  
  if (sscanf(buf,"%lf %i %i %i",&z,&r,&g,&b) != 4)
    {
      fprintf(stderr,"bad line\n  %s",buf);
      return NULL;
    }

  if ((f = malloc(sizeof(fill_stack_t))) == NULL)
    return NULL;

  f->fill.type = colour;
  f->val       = z;

  f->fill.u.colour.rgb.red   = r;
  f->fill.u.colour.rgb.green = g;
  f->fill.u.colour.rgb.blue  = b;

  f->next = xyread4(stream,buf);

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
  }
  while (s++);

  return 0;
}
