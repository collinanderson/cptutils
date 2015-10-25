/*
  cptqml.c
  convert GMT colour table (cpt) to QGis style sheet (qml)
  Copyright (c) J.J. Green 2015
*/

#include <cptread.h>
#include <qmlwrite.h>
#include <btrace.h>

#include "cptqml.h"

static qml_t* cptqml_convert(cpt_t* cpt, cptqml_opt_t opt)
{
  if (cpt->model != model_rgb)
    {
      btrace("cannot convert non-RGB cpt");
      return NULL;
    }

  int n = cpt_nseg(cpt);

  qml_t *qml = qml_new(QML_TYPE_INTERPOLATED, n);

  for (int i = 0 ; i < n ; i++)
    {
      cpt_seg_t *seg = cpt_segment(cpt, i);

      qml_entry_t entry = {
	.red = seg->lsmp.fill.u.colour.rgb.red,
	.green = seg->lsmp.fill.u.colour.rgb.green,
	.blue = seg->lsmp.fill.u.colour.rgb.blue,
	.value = seg->lsmp.val,
	.label = NULL
      };

      if (qml_set_entry(qml, i, &entry) != 0)
	{
	  btrace("failed to add qml entry %i", i);
	  return NULL;
	}
    }

  return qml;
}

extern int cptqml(cptqml_opt_t opt)
{
  cpt_t *cpt;
  int err = 1;

  if ((cpt = cpt_new()) != NULL)
    {
      if (cpt_read(opt.file.input, cpt) == 0)
        {
	  qml_t *qml = cptqml_convert(cpt, opt);

          if (qml != NULL)
            {
              if (qml_write(opt.file.output, qml) == 0)
                {
		  qml_destroy(qml);
		  err = 0;
                }
              else
		btrace("error writing qml");
            }
          else
	    btrace("failed to convert");
        }
      else
	btrace("failed to load cpt from %s",
	       (opt.file.input ? opt.file.input : "<stdin>"));

      cpt_destroy(cpt);
    }
  else
    btrace("failed to create cpt struct");

  if (err)
    btrace("failed to write cpt to %s",
	   (opt.file.output ? opt.file.output : "<stdout>"));

  return err;
}
