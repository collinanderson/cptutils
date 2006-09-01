/*
  pspwrite.h

  writes paintshop pro gradients.
  2006 (c) J.J. Green

  $Id: pspwrite.c,v 1.1 2006/09/01 22:32:38 jjg Exp jjg $
*/

#include <stdio.h>
#include <string.h>

#include "pspwrite.h"
#include "htons.h"

static int write_initial_rgb(FILE*,psp_rgbseg_t*);
static int write_rgb(FILE*,psp_rgbseg_t*);
static int write_op(FILE*,psp_opseg_t*);
static int write_zeros(FILE*,int);

extern int psp_write(FILE *s,psp_t *psp)
{
  int i,n;
  unsigned char len = strlen(psp->name);
  unsigned short u[4];

  /* magic */

  fwrite(pspmagic,1,4,s);
  
  /* version */

  for (i=0 ; i<2 ; i++) u[i] = htons(psp->ver[i]);
  fwrite(u,2,2,s);

  /* title */

  fwrite(&len,1,1,s);
  fwrite(psp->name,1,len,s);

  /* rgb gradient */

  n = psp->rgb.n;

  u[0] = htons(n);
  fwrite(u,2,1,s);

  write_initial_rgb(s,psp->rgb.seg);
  for (i=1 ; i<n ; i++) write_rgb(s,psp->rgb.seg+i);

  write_zeros(s,2);

  /* opacity gradient */

  n = psp->op.n;

  u[0] = htons(psp->op.n);
  fwrite(u,2,1,s);

  for (i=0 ; i<n ; i++) write_op(s,psp->op.seg+i);

  write_zeros(s,2);

  /* foot */

  write_zeros(s,1);

  return 0;
}

static int write_initial_rgb(FILE *s,psp_rgbseg_t *seg)
{
  unsigned short u[8];

  u[0] = 0;
  u[1] = 0;
  u[2] = 0;
  u[3] = htons(seg->midpoint);
  u[4] = 0;
  u[5] = htons(seg->r);
  u[6] = htons(seg->g);
  u[7] = htons(seg->b);

  fwrite(u,2,8,s);

  return 0;
}

static int write_rgb(FILE *s,psp_rgbseg_t *seg)
{
  unsigned short u[10];

  u[0] = 0;
  u[1] = 0;
  u[2] = 0;
  u[3] = htons(seg->z); 
  u[4] = 0;
  u[5] = htons(seg->midpoint); 
  u[6] = 0;
  u[7] = htons(seg->r);
  u[8] = htons(seg->g);
  u[9] = htons(seg->b);

  fwrite(u,2,10,s);

  return 0;
}

static int write_op(FILE *s,psp_opseg_t *seg)
{
  unsigned short u[5];

  u[0] = 0;
  u[1] = htons(seg->z);
  u[2] = 0;
  u[3] = htons(seg->midpoint);
  u[4] = htons(seg->opacity);

  fwrite(u,2,5,s);

  return 0;
}

static int write_zeros(FILE *s,int n)
{
  int i;
  unsigned short u[n];

  for (i=0 ; i<n ; i++) u[i] = 0;

  fwrite(u,2,n,s);

  return 0;
}
