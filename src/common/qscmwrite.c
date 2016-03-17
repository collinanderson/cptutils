/*
  qscmwrite.c
  Copyright (c) J.J. Green 2015
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <stdio.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "qscmwrite.h"
#include "btrace.h"

#define ENCODING "utf-8"
#define BUFSZ 128

static int qscm_attribute(xmlTextWriter *writer,
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

static int qscm_write_colorRampType(xmlTextWriter *writer, qscm_t *qscm)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorRampType") < 0 )
    {
      btrace("error from open colorRampType");
      return 1;
    }

  const char *type;

  switch (qscm->type)
    {
    case QSCM_TYPE_DISCRETE:
      type = "DISCRETE";
      break;
    case QSCM_TYPE_INTERPOLATED:
      type = "INTERPOLATED";
      break;
    default:
      btrace("no such QSCM type %i in input", qscm->type);
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

static int qscm_int_attribute(xmlTextWriter *writer,
			      size_t len,
			      const char *name,
			      int value,
			      const char *element)
{
  char buffer[len];
  snprintf(buffer, len, "%i", value);
  return qscm_attribute(writer, name, buffer, element);
}

static int qscm_double_attribute(xmlTextWriter *writer,
				 size_t len,
				 const char *name,
				 double value,
				 const char *element)
{
  char buffer[len];
  snprintf(buffer, len, "%.3f", value);
  return qscm_attribute(writer, name, buffer, element);
}

static int qscm_write_colorRampEntry(xmlTextWriter *writer, qscm_entry_t *entry)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "colorRampEntry") < 0 )
    {
      btrace("error from open colorRampEntry");
      return 1;
    }

  if (qscm_double_attribute(writer, 10, "value", entry->value, "colorRampEntry") != 0)
    return 1;

  if (qscm_int_attribute(writer, 4, "red", (int)(entry->red), "colorRampEntry") != 0)
    return 1;

  if (qscm_int_attribute(writer, 4, "green", (int)(entry->green), "colorRampEntry") != 0)
    return 1;

  if (qscm_int_attribute(writer, 4, "blue", (int)(entry->blue), "colorRampEntry") != 0)
    return 1;

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close colorRampEntry");
      return 1;
    }

  return 0;
}

static int qscm_write_customColorRamp(xmlTextWriter *writer, qscm_t *qscm)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "customColorRamp") < 0 )
    {
      btrace("error from open customColorRamp");
      return 1;
    }

  if (qscm_write_colorRampType(writer, qscm) != 0)
    {
      btrace("failed to write colorRampType");
      return 1;
    }

  for (size_t i = 0 ; i < qscm->n ; i++)
    {
      if (qscm_write_colorRampEntry(writer, qscm->entries + i) != 0)
	{
	  btrace("failed to write colorRampEntry %zi", i);
	  return 1;
	}
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close customColorRamp");
      return 1;
    }

  return 0;
}

static int qscm_write_rasterproperties(xmlTextWriter *writer, qscm_t *qscm)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "rasterproperties") < 0 )
    {
      btrace("error from open rasterproperties");
      return 1;
    }

  if (qscm_write_customColorRamp(writer, qscm) != 0)
    {
      btrace("failed to write customColorRamp");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close rasterproperties");
      return 1;
    }

  return 0;
}

static int qscm_write_qgis(xmlTextWriter *writer, qscm_t *qscm)
{
  if (xmlTextWriterStartElement(writer, BAD_CAST "qgis") < 0 )
    {
      btrace("error from open qgis");
      return 1;
    }

  if (qscm_attribute(writer, "version", "1.6.0-Copiapo", "qgis") != 0)
    return 1;

  if (qscm_write_rasterproperties(writer, qscm) != 0)
    {
      btrace("failed to write rasterproperties");
      return 1;
    }

  if (xmlTextWriterEndElement(writer) < 0)
    {
      btrace("error from close qgis");
      return 1;
    }

  return 0;
}

static int qscm_write_mem(xmlTextWriter *writer, qscm_t *qscm)
{
  if (qscm->n < 2)
    {
      btrace("not enough stops for a gradient (%zi)", qscm->n);
      return 1;
    }

  if (xmlTextWriterStartDocument(writer, NULL, ENCODING, NULL) < 0)
    {
      btrace("error from start document");
      return 1;
    }

  if (qscm_write_qgis(writer, qscm) != 0)
    {
      btrace("failed to write qgis");
      return 1;
    }

  if ( xmlTextWriterEndDocument(writer) < 0 )
    {
      btrace("error from end document");
      return 1;
    }

  return 0;
}

extern int qscm_write(const char *path, qscm_t *qscm)
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

          if (qscm_write_mem(writer, qscm) == 0)
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
