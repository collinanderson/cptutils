/*
  cptio.c

  read/write a cpt file
  (c) J.J Green 2004
  $Id: cptio.c,v 1.2 2004/03/05 01:27:49 jjg Exp jjg $
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "cptio.h"
#include "cptparse.h"
#include "cptscan.h"
#include "version.h"

/* defined in cptparse.c but not exported */

extern int cptparse(void*);

#define LBUF 1024
#define SNM(x) ((x) ? (x) : "<stdin>")

extern int cptdebug;

extern int cpt_read(char* file,cpt_t* cpt)
{
  FILE    *stream;
  yyscan_t cptscan;

  if (file)
    {
      char *s,*e,*name;

      if ((stream = fopen(file,"r")) == NULL)
	{
	  fprintf(stderr,"error reading %s : %s\n",SNM(file),strerror(errno));
	  return 1;
	}

      /* get the name from the filename */

      name = cpt->name;

      if ((s = strrchr(file,'/')) == NULL) 
	s = file;
      else
	s++;

      strncpy(name,s,CPT_NAME_LEN);

      /* chop off a trailing .cpt */

      if ((e = strrchr(name,'.')) != NULL)
	{   
	  if (strcmp(e,".cpt") == 0) *e = '\0';
	} 
    }
  else
    {
      stream = stdin;
      strncpy(cpt->name,"<stdin>",CPT_NAME_LEN);
    }

  if (cptlex_init(&cptscan) != 0)
    {
      fprintf(stderr,"problem initailising scanner : %s\n",strerror(errno));
      return 1;
    }
  
  cptset_in(stream,cptscan);

  cptset_debug(1,cptscan);

  if (cptparse(cptscan) != 0)
    {
      fprintf(stderr,"failed parse\n");
      return 1;
    }

  cptlex_destroy(cptscan);

#ifdef OLD_SCANNER

  /* default rgb colour model */

  cpt->model = rgb;
  
  for (n=1 ; fgets(lbuf,LBUF,stream) ; n++)
    { 
      /* check for comment lines */

      if (*lbuf == '#')
	{
	  char cm[6];

	  /* special comments specify the colour model */

	  if (sscanf(lbuf,"# COLOR_MODEL = %5s",cm) == 1)
	    {
	      if (strcmp(cm,"RGB") == 0)
		cpt->model = rgb;
	      else if (strcmp(cm,"HSV") == 0)
		cpt->model = hsv;
	      else
		{
		  fprintf(stderr,"dont know model %s\n",cm);
		  return 1;
		}
	    }
	  
	  continue;
	}

      /* check for segment line */
      
      switch (cpt->model)
	{
	  struct 
	  {
	    double val;
	    rgb_t  rgb;
	    hsv_t  hsv;
	  } l,r;

	case rgb:
	  
	  if (sscanf(lbuf,
		     " %lf %u %u %u %lf %u %u %u",
		     &l.val,
		     &l.rgb.red,
		     &l.rgb.green,
		     &l.rgb.blue,
		     &r.val,
		     &r.rgb.red,
		     &r.rgb.green,
		     &r.rgb.blue
		     ) == 8)
	    {
	      cpt_seg_t *seg;
	      
	      seg = cpt_seg_new();
	      
	      seg->lsmp.val     = l.val;
	      seg->lsmp.col.rgb = l.rgb;

	      seg->rsmp.val     = r.val;
	      seg->rsmp.col.rgb = r.rgb;
	      
	      if (cpt_append(seg,cpt) != 0)
		{
		  fprintf(stderr,"failed seg append");
		  return 1;
		}

	      continue;
	    }
	  break;

	case hsv:

	  if (sscanf(lbuf,
		     " %lf %lf %lf %lf %lf %lf %lf %lf",
		     &l.val,
		     &l.hsv.hue,
		     &l.hsv.sat,
		     &l.hsv.val,
		     &r.val,
		     &r.hsv.hue,
		     &r.hsv.sat,
		     &r.hsv.val
		     ) == 8)
	    {
	      cpt_seg_t *seg;
	      
	      seg = cpt_seg_new();
	      
	      seg->lsmp.val     = l.val;
	      seg->lsmp.col.hsv = l.hsv;

	      seg->rsmp.val     = r.val;
	      seg->rsmp.col.hsv = r.hsv;
	      
	      if (cpt_append(seg,cpt) != 0)
		{
		  fprintf(stderr,"failed seg append");
		  return 1;
		}

	      continue;
	    }
	  break;

	default :

	  fprintf(stderr,"bad colour model\n");
	}
	  
      /* get background etc */

      switch (cpt->model)
	{
	  colour_t c;
	  char type;

	case rgb:

	  if (sscanf(lbuf,
		     "%c %u %u %u",
		     &type,
		     &c.rgb.red,
		     &c.rgb.green,
		     &c.rgb.blue) == 4)
	    {
	      switch (type)
		{
		case 'F': case 'f':
		  cpt->fg = c;
		  break;
		  
		case 'B': case 'b':
		  cpt->bg = c;
		  break;
		  
		case 'N': case 'n':
		  cpt->nan = c;
		  break;
		  
		default:
		  fprintf(stderr,"strange FBN at line %i\n",n);
		  return 1;
		}
	      continue;
	    }
	  break;

	case hsv:

	  if (sscanf(lbuf,
		     "%c %lf %lf %lf",
		     &type,
		     &c.hsv.hue,
		     &c.hsv.sat,
		     &c.hsv.val) == 4)
	    {
	      switch (type)
		{
		case 'F': case 'f':
		  cpt->fg = c;
		  break;
		  
		case 'B': case 'b':
		  cpt->bg = c;
		  break;
		  
		case 'N': case 'n':
		  cpt->nan = c;
		  break;
		  
		default:
		  fprintf(stderr,"strange FBN at line %i\n",n);
		  return 1;
		}
	      continue;
	    }
	  break;

	default:

	  return 1;
	}

      /* everthing else we ignore */

      fprintf(stderr,"failed to scan line %i\n",n);
    }

#else

  printf("newscanner\n");

#endif

  if (!feof(stream))
    {
      if (ferror(stream))
	{
	  fprintf(stderr,"error reading %s\n",SNM(file));
	  return 1;
	}
      else
	{
	  fprintf(stderr,"wierd error reading %s\n",SNM(file));
	}
    }

  if (stream != stdin) 
    fclose(stream);

  return 0;
}

static int print_cpt_aux(FILE*,char,colour_t,model_t);

extern int cpt_write(char* outfile,cpt_t* cpt)
{
    FILE       *stream;
    cpt_seg_t  *seg;
    const char *model;
    time_t      t;

    if (outfile)
    {
	stream = fopen(outfile,"wb");
	if (!stream) return 1;
    }
    else stream = stdout;

    t = time(NULL);

    switch (cpt->model)
      {
      case rgb : model = "RGB"; break;
      case hsv : model = "HSV"; break;
      default :
	fprintf(stderr,"cpt file corrupt : bad model specified\n");
	return 1;
      }

    fprintf(stream,"# %s\n",(outfile ? outfile : "<stdout>"));
    fprintf(stream,"# autogenerated GMT palette \"%s\"\n",(cpt->name ? cpt->name : ""));
    fprintf(stream,"# libgimpcpt version %s, %s",VERSION,ctime(&t));
    fprintf(stream,"# COLOR_MODEL = %s\n",model);

    seg = cpt->segment;

    while (seg)
    {
	cpt_sample_t l,r;

	l = seg->lsmp;
	r = seg->rsmp;
	  
	switch (cpt->model)
	  {
	  case rgb:
  
	    fprintf(stream,
		    "%#7e %3i %3i %3i %#7e %3i %3i %3i\n",
		    l.val,
		    l.col.rgb.red,
		    l.col.rgb.green,
		    l.col.rgb.blue,
		    r.val,
		    r.col.rgb.red,
		    r.col.rgb.green,
		    r.col.rgb.blue);
	    break;

	  case hsv:
	    fprintf(stream,
		    "%#7e %7e %7e %7e %#7e %7e %7e %7e\n",
		    l.val,
		    l.col.hsv.hue,
		    l.col.hsv.sat,
		    l.col.hsv.val,
		    r.val,
		    r.col.hsv.hue,
		    r.col.hsv.sat,
		    r.col.hsv.val);
	    break;

	  default:
	    return 1;
	  }

	seg = seg->rseg;
    }

    print_cpt_aux(stream,'B',cpt->bg,cpt->model);
    print_cpt_aux(stream,'F',cpt->fg,cpt->model);
    print_cpt_aux(stream,'N',cpt->nan,cpt->model);

    fclose(stream);

    return 0;
}

static int print_cpt_aux(FILE* stream,char c,colour_t col,model_t model)
{
  switch (model)
    {
    case rgb:
      fprintf(stream,"%c %3i %3i %3i\n",
	      c, col.rgb.red, col.rgb.green, col.rgb.blue);
      break;

    case hsv:
      fprintf(stream,"%c %2f %2f %3f\n",
	      c, col.hsv.hue, col.hsv.val, col.hsv.sat);
      break;

    default:
      return 1;
    }

  return 0;
}