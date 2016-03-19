/*
  qgswrite.c
  Copyright (c) J.J. Green 2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <stdio.h>
#include <string.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "qgswrite.h"
#include "btrace.h"

#define ENCODING "utf-8"
#define BUFSZ 128

static int qgs_attribute(xmlTextWriter *writer,
			  const char *name,
			  const char *value,
			  const char *element)
{
  if (xmlTextWriterWriteAttribute(writer,
                                  BAD_CAST name,
                                  BAD_CAST value) < 0)
    {
      btrace("error setting %s %s attribute", element, name);
      return 1;
    }

  return 0;
}

/* 4*3 + 3 + 1 */

#define END_STOP_LEN 16

static char* end_stop(qgs_entry_t *entry)
{
  char *str = malloc(END_STOP_LEN);
  if (snprintf(str, END_STOP_LEN, "%i,%i,%i,%i",
	       entry->rgb.red,
	       entry->rgb.green,
	       entry->rgb.blue,
	       entry->opacity) >= END_STOP_LEN)
    return NULL;

  return str;
}

/* 6 + 4*3 + 4 + 1 */

#define MID_STOP_LEN 23

static char* mid_stop(qgs_entry_t *entry)
{
  char *str = malloc(MID_STOP_LEN);
  if (snprintf(str, MID_STOP_LEN, "%.4f;%i,%i,%i,%i",
	       entry->value,
	       entry->rgb.red,
	       entry->rgb.green,
	       entry->rgb.blue,
	       entry->opacity) >= MID_STOP_LEN)
    return NULL;

  return str;
}

static int qgs_write_endstop(xmlTextWriter *writer,
			     const char *name,
			     qgs_entry_t *entry)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "prop") < 0 )
    {
      btrace("error from open prop");
      return 1;
    }

  if (qgs_attribute(writer, "k", name, "prop") != 0)
    return 1;

  char *stop;

  if ((stop = end_stop(entry)) == NULL)
    {
      btrace("error creating endstop");
      return 1;
    }

  if (qgs_attribute(writer, "v", stop, "prop") != 0)
    return 1;

  free(stop);

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close prop");
      return 1;
    }

  return 0;
}

static char* mid_stops_string(qgs_t *qgs)
{
  int n = qgs->n;
  char *stops[n-2];

  for (int i = 0 ; i < n-2 ; i++)
    {
      if ((stops[i] = mid_stop(qgs->entries + i + 1)) == NULL)
	return NULL;
    }

  size_t sz = (n-2) * MID_STOP_LEN + n;
  char *buffer;

  if ((buffer = malloc(sz)) == NULL)
    return NULL;

  char *p = buffer;
  size_t k;

  if ((k = snprintf(buffer, sz, "%s", stops[0])) >= sz)
    {
      btrace("stop truncation");
      return NULL;
    }

  for (int i = 1 ; i < n-2 ; i++)
    {
      p += k;
      sz -= k;
      if ((k = snprintf(p, sz, ":%s", stops[i])) >= sz)
	{
	  btrace("stop truncation");
	  return NULL;
	}
    }

  for (int i = 0 ; i < n-2 ; i++)
    free(stops[i]);

  return buffer;
}

static int qgs_write_midstops(xmlTextWriter *writer, qgs_t *qgs)
{
  int n = qgs->n;

  if (n < 3) return 0;

  if (xmlTextWriterStartElement(writer, BAD_CAST "prop") < 0 )
    {
      btrace("error from open prop");
      return 1;
    }

  if (qgs_attribute(writer, "k", "stops", "prop") != 0)
    return 1;

  char *str = mid_stops_string(qgs);

  if (qgs_attribute(writer, "v", str, "prop") != 0)
    return 1;

  free(str);

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close prop");
      return 1;
    }

  return 0;
}

static int qgs_write_colorramp_props(xmlTextWriter *writer, qgs_t *qgs)
{
  int n = qgs->n;

  if (qgs_write_endstop(writer, "color1", qgs->entries) != 0)
    return 1;

  if (qgs_write_endstop(writer, "color2", qgs->entries + (n - 1)) != 0)
    return 1;

  if (qgs_write_midstops(writer, qgs) != 0)
    return 1;

  return 0;
}

static int qgs_write_colorramp(xmlTextWriter *writer, qgs_t *qgs)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorramp") < 0 )
    {
      btrace("error from open coloramp");
      return 1;
    }

  if (qgs_attribute(writer, "type", "gradient", "colorramp") != 0)
    return 1;

  if (qgs_attribute(writer, "name", qgs->name, "colorramp") != 0)
    return 1;

  if (qgs_write_colorramp_props(writer, qgs) != 0)
    return 1;

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close colorramp");
      return 1;
    }

  return 0;
}

static int qgs_write_colorramps(xmlTextWriter *writer, qgs_t *qgs)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorramps") < 0 )
    {
      btrace("error from open colorramps");
      return 1;
    }

  if (qgs_write_colorramp(writer, qgs) != 0)
    {
      btrace("failed to write colorramp");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close colorramps");
      return 1;
    }

  return 0;
}

static int qgs_write_symbols(xmlTextWriter *writer, qgs_t *qgs)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "symbols") < 0 )
    {
      btrace("error from open symbols");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close symbols");
      return 1;
    }

  return 0;
}

static int qgs_write_qgis(xmlTextWriter *writer, qgs_t *qgs)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "qgis_style") < 0 )
    {
      btrace("error from open qgis");
      return 1;
    }

  if (qgs_attribute(writer, "version", "1", "qgis_style") != 0)
    return 1;

  if (qgs_write_symbols(writer, qgs) != 0)
    {
      btrace("failed to write symbols");
      return 1;
    }

  if (qgs_write_colorramps(writer, qgs) != 0)
    {
      btrace("failed to write colorramps");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close qgis");
      return 1;
    }

  return 0;
}

static int qgs_write_mem(xmlTextWriter *writer, qgs_t *qgs)
{
  if (qgs->n < 2)
    {
      btrace("not enough stops for a gradient (%zi)", qgs->n);
      return 1;
    }

  if (xmlTextWriterWriteDTD(writer, BAD_CAST "qgis_style", NULL, NULL, NULL) < 0)
    {
      btrace("error writing doctype");
      return 1;
    }

  if (qgs_write_qgis(writer, qgs) != 0)
    {
      btrace("failed to write qgis");
      return 1;
    }

  if (xmlTextWriterEndDocument(writer) < 0)
    {
      btrace("error from end document");
      return 1;
    }

  return 0;
}

extern int qgs_write(const char *path, qgs_t *qgs)
{
  xmlBuffer *buffer;
  int err = 0;

  if ((buffer = xmlBufferCreate()) != NULL)
    {
      xmlTextWriter *writer;

      if ((writer = xmlNewTextWriterMemory(buffer, 0)) != NULL)
        {
          /*
            we free the writer at the start of each of
            these branches since this must be done before
            using the buffer
          */

          if (qgs_write_mem(writer, qgs) == 0)
            {
              xmlFreeTextWriter(writer);

	      if (path)
                {
                  FILE* fp;

                  if ((fp = fopen(path, "w")) != NULL)
                    {
                      fprintf(fp, "%s", buffer->content);

                      if (fclose(fp) != 0)
                        {
                          btrace("error closing file %s", path);
                          err = 1;
                        }
                    }
                  else
                    {
                      btrace("error opening file %s", path);
                      err = 1;
                    }
                }
              else
                fprintf(stdout, "%s", buffer->content);
            }
          else
            {
              xmlFreeTextWriter(writer);

              btrace("failed memory write");
              err = 1;
            }
        }
      else
        {
          btrace("error creating the xml writer");
          err = 1;
        }
    }
  else
    {
      btrace("error creating xml writer buffer");
      err = 1;
    }

  xmlBufferFree(buffer);

  return err;
}
