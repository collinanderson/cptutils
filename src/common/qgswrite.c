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

static int qgs_write_colorRampType(xmlTextWriter *writer, qgs_t *qgs)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorRampType") < 0 )
    {
      btrace("error from open colorRampType");
      return 1;
    }

  const char *type;

  switch (qgs->type)
    {
    case QGS_TYPE_DISCRETE:
      type = "DISCRETE";
      break;
    case QGS_TYPE_INTERPOLATED:
      type = "INTERPOLATED";
      break;
    default:
      btrace("no such QGS type %i in input", qgs->type);
      return 1;
    }

  if (xmlTextWriterWriteString(writer, BAD_CAST type) < 0 )
    {
      btrace("error writing type string");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close colorRampType");
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

static int qgs_write_colorRampEntry(xmlTextWriter *writer, qgs_entry_t *entry)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorRampEntry") < 0 )
    {
      btrace("error from open colorRampEntry");
      return 1;
    }

  if (qgs_double_attribute(writer, 10, "value", entry->value, "colorRampEntry") != 0)
    return 1;

  if (qgs_int_attribute(writer, 4, "red", (int)(entry->rgb.red), "colorRampEntry") != 0)
    return 1;

  if (qgs_int_attribute(writer, 4, "green", (int)(entry->rgb.green), "colorRampEntry") != 0)
    return 1;

  if (qgs_int_attribute(writer, 4, "blue", (int)(entry->rgb.blue), "colorRampEntry") != 0)
    return 1;

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close colorRampEntry");
      return 1;
    }

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

  if (qgs_attribute(writer, "name", "FIXME", "colorramp") != 0)
    return 1;

  /* FIXME */

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

  if (qgs_attribute(writer, "version", "0", "qgis_style") != 0)
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
