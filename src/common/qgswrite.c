/*
  qgswrite.c
  Copyright (c) J.J. Green 2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <stdio.h>

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

static int qgs_int_attribute(xmlTextWriter *writer,
			      size_t len,
			      const char *name,
			      int value,
			      const char *element)
{
  char buffer[len];
  snprintf(buffer, len, "%i", value);
  return qgs_attribute(writer, name, buffer, element);
}

static int qgs_double_attribute(xmlTextWriter *writer,
				 size_t len,
				 const char *name,
				 double value,
				 const char *element)
{
  char buffer[len];
  snprintf(buffer, len, "%.3f", value);
  return qgs_attribute(writer, name, buffer, element);
}

static char* stop_rgba(qgs_entry_t *entry)
{
  char *str = malloc(16);
  snprintf(str, 16, "%i,%i,%i,%i",
	   entry->rgb.red,
	   entry->rgb.green,
	   entry->rgb.blue,
	   entry->opacity);

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

  char *rgba = stop_rgba(entry);

  if (qgs_attribute(writer, "v", rgba, "prop") != 0)
    return 1;

  free(rgba);

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
