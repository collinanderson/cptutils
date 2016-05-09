/*
  grd3read.c

  read PaintShop Pro gradients.
  2005, 2016 (c) J.J. Green
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#if defined HAVE_ENDIAN_H
#include <endian.h>
#elif defined HAVE_SYS_ENDIAN_H
#include <sys/endian.h>
#endif

#include "grd3read.h"
#include "btrace.h"

static int grd3_read_stream(FILE*, grd3_t*);

extern int grd3_read(const char* file, grd3_t* grd3)
{
  int err;

  if (file)
    {
      FILE* s;

      if ((s = fopen(file, "r")) == NULL)
	{
	  btrace("failed to open %s", file);
	  return 1;
	}

      err = grd3_read_stream(s, grd3);

      fclose(s);
    }
  else
    err = grd3_read_stream(stdin, grd3);

  return err;
}

static int read_colour_stop(FILE*, grd3_rgbseg_t*);
static int read_opacity_stop(FILE*, grd3_opseg_t*);
static int read_reserved(FILE*);

static int grd3_read_stream(FILE *s, grd3_t *grad)
{
  unsigned char b[6];
  uint16_t u[2];
  size_t n;

  /* first 4 are the grd3 magic number */

  if (fread(b, 1, 4, s) != 4)
    {
      btrace("failed read of magic number");
      return 1;
    }

  if (strncmp((const char*)b, "8BGR", 4) != 0)
    {
      char mbuf[5] = {0};

      for (size_t i = 0 ; i < 4 ; i++)
	mbuf[i] = (isalnum(b[i]) ? b[i] : '.');

      btrace("expected magic number '8BGR', but found '%s'", mbuf);

      return 1;
    }

  /* next 2 shorts, a format version (usually 3 1) */

  if (fread(u, 2, 2, s) != 2)
    {
      btrace("failed to read version");
      return 1;
    }

  grad->ver[0] = be16toh(u[0]);
  grad->ver[1] = be16toh(u[1]);

  if (grad->ver[0] != 3)
    {
      btrace("bad version of GRD format: expected 3, got %i",
	     grad->ver[0]);
      return 1;
    }

  /*
    then a single unsigned char, the title length (fgetc
    casts this to int, it may be EOF < 0)
  */

  int len = fgetc(s);

  /* make a copy of the title, if feasible */

  if (len < 0)
    {
      btrace("premature truncation");
      return 1;
    }
  else if (len == 0)
    {
      btrace("title length is zero");
      return 1;
    }
  else
    {
      unsigned char *name;

      if ((name = malloc(len+1)) == NULL)
	{
	  btrace("failed malloc");
	  return 1;
	}

      for (size_t j = 0 ; j < len ; j++)
	name[j] = fgetc(s);

      name[len] = '\0';

      grad->name = name;
    }

  /* then an short, the number of rgb samples */

  if (fread(u, 2, 1, s) != 1)
    {
      btrace("failed to read number of RGB samples");
      return 1;
    }

  n = be16toh(u[0]);

  if (n < 2)
    {
      btrace("too few samples (%i)", n);
      return 1;
    }
  else
    {
      grd3_rgbseg_t* seg;

      if ((seg = malloc(n*sizeof(grd3_rgbseg_t))) == NULL)
	{
	  btrace("failed malloc");
	  return 1;
	}

      for (size_t j = 0 ; j < n ; j++)
	{
	  if (read_colour_stop(s, seg + j) != 0)
	    {
	      btrace("error reading colour stop %i", j+1);
	      return 1;
	    }
	}

      grad->rgb.n   = n;
      grad->rgb.seg = seg;
    }

  /* then a short, the number of opacity samples */

  if (fread(u, 1, 2, s) != 2)
    {
      btrace("failed to read number of opacity samples");
      return 1;
    }

  n = be16toh(u[0]);

  if (n < 2)
    {
      btrace("there are %i opacity stop%s (not enough)",
	     n, (n==1 ? "" : "s" ));
      return 1;
    }
  else
    {
      grd3_opseg_t* seg;

      if ((seg = malloc(n*sizeof(grd3_opseg_t))) == NULL)
	{
	  btrace("failed malloc");
	  return 1;
	}

      for (size_t j = 0 ; j < n ; j++)
	{
	  if (read_opacity_stop(s, seg + j) != 0)
	    {
	      btrace("error reading opacity stop %i", j+1);
	      return 1;
	    }
	}

      grad->op.n = n;
      grad->op.seg = seg;
    }

  if (read_reserved(s) != 0)
    {
      btrace("error reading endblock");
      return 1;
    }

  return 0;
}

static int read_colour_stop_offset(FILE *s, grd3_rgbseg_t* seg)
{
  uint32_t u;

  if (fread(&u, 4, 1, s) != 1)
    {
      btrace("failed to read offset");
      return 1;
    }

  u = be32toh(u);

  if (u > 4096)
    {
      btrace("unexpected offset : %u", u);
      return 1;
    }

  seg->z = u;

  return 0;
}

static int read_colour_stop_midpoint(FILE *s, grd3_rgbseg_t* seg)
{
  uint32_t u;

  if (fread(&u, 4, 1, s) != 1)
    {
      btrace("failed to read midpoint");
      return 1;
    }

  u = be32toh(u);

  if (! ((0 < u) && (u < 100)))
    {
      btrace("unexpected midpoint : %u", u);
      return 1;
    }

  seg->midpoint = u;

  return 0;
}

static int read_colour_stop_model(FILE *s, grd3_rgbseg_t* seg)
{
  uint16_t u;

  if (fread(&u, 2, 1, s) != 1)
    {
      btrace("failed to read model");
      return 1;
    }

  u = be16toh(u);

  switch (u)
    {
    case 0:
      break;
    case 1:
      btrace("unsupported colour model HSV");
      return 1;
    case 2:
      btrace("unsupported colour model CMYK");
      return 1;
    case 7:
      btrace("unsupported colour model Lab");
      return 1;
    case 8:
      btrace("unsupported colour model Grayscale");
      return 1;
    default:
      btrace("unknown colour model : %u", u);
      return 1;
    }

  return 0;
}

static int read_colour_stop_colours(FILE *s, grd3_rgbseg_t* seg)
{
  uint16_t u[4];

  if (fread(u, 2, 4, s) != 4)
    {
      btrace("failed to read colour values");
      return 1;
    }

  if (be16toh(u[3]))
    {
      btrace("unexpected rgb padding : %u", be16toh(u[0]));
      return 1;
    }

  seg->r = be16toh(u[0]);
  seg->g = be16toh(u[1]);
  seg->b = be16toh(u[2]);

  return 0;
}

static int read_colour_stop_type(FILE *s, grd3_rgbseg_t* seg)
{
  uint16_t u;

  if (fread(&u, 2, 1, s) != 1)
    {
      btrace("failed to read colour type");
      return 1;
    }

  u = be16toh(u);

  switch (u)
    {
    case 0:
      break;
    case 1:
      btrace("foreground colour type not supported");
      return 1;
    case 2:
      btrace("background colour type not supported");
      return 1;
    default:
      btrace("unexpected colour type : %i", u);
      return 1;
    }

  return 0;
}

typedef int (*colour_stop_reader_t)(FILE*, grd3_rgbseg_t*);

static int read_colour_stop(FILE *s, grd3_rgbseg_t* seg)
{
  colour_stop_reader_t *reader, readers[] = {
    read_colour_stop_offset,
    read_colour_stop_midpoint,
    read_colour_stop_model,
    read_colour_stop_colours,
    read_colour_stop_type,
    NULL
  };

  int err = 0;

  for (reader = readers ; *reader ; reader++)
    err += (*reader)(s, seg);

  return err;
}

static int read_opacity_stop_offset(FILE *s, grd3_opseg_t* seg)
{
  uint32_t u;

  if (fread(&u, 4, 1, s) != 1)
    {
      btrace("failed to read offset");
      return 1;
    }

  u = be32toh(u);

  if (u > 4096)
    {
      btrace("unexpected offset : %u", u);
      return 1;
    }

  seg->z = u;

  return 0;
}

static int read_opacity_stop_midpoint(FILE *s, grd3_opseg_t* seg)
{
  uint32_t u;

  if (fread(&u, 4, 1, s) != 1)
    {
      btrace("failed to read offset");
      return 1;
    }

  u = be32toh(u);

  if ( ! ((0 < u) && (u < 100)) )
    {
      btrace("unexpected midpoint : %u", u);
      return 1;
    }

  seg->midpoint = u;

  return 0;
}

static int read_opacity_stop_opacity(FILE *s, grd3_opseg_t* seg)
{
  uint16_t u;

  if (fread(&u, 2, 1, s) != 1)
    {
      btrace("failed to read opacity");
      return 1;
    }

  u = be16toh(u);

  if (u > 255)
    {
      btrace("unexpected midpoint : %u", u);
      return 1;
    }

  seg->opacity = u;

  return 0;
}

typedef int (*opacity_stop_reader_t)(FILE*, grd3_opseg_t*);

static int read_opacity_stop(FILE *s, grd3_opseg_t* seg)
{
  opacity_stop_reader_t *reader, readers[] = {
    read_opacity_stop_offset,
    read_opacity_stop_midpoint,
    read_opacity_stop_opacity,
    NULL
  };

  int err = 0;

  for (reader = readers ; *reader ; reader++)
    err += (*reader)(s, seg);

  return err;
}

static int read_reserved(FILE *s)
{
  if (fseek(s, 6L, SEEK_CUR) != 0)
    {
      btrace("fseek error : %s", strerror(errno));
      return 1;
    }

  return 0;
}
