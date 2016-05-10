/*
  convert a utf8 multibyte string to ascii with
  character transliteration
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <iconv.h>
#include <locale.h>

#include <btrace.h>

#include "utf8x.h"

extern int utf8_to_x(const char *type,
		     const unsigned char *in,
		     char *out,
		     size_t lenout)
{
  const char lname[] = "en_US.UTF-8";
  const char icopt[] = "TRANSLIT";
  size_t icnamelen = strlen(type) + 2 + strlen(icopt) + 1;
  char icname[icnamelen];

  if (snprintf(icname, icnamelen, "%s//%s", type, icopt) > icnamelen)
    {
      btrace("failed to create iconv string");
      return 1;
    }

  if (setlocale(LC_CTYPE, lname) == NULL)
    {
      btrace("failed to set locale to %s", lname);
      return 1;
    }

  int err = 0;
  iconv_t cv = iconv_open(icname, "UTF-8");

  if (cv == (iconv_t)(-1))
    {
      btrace("error opening iconv descriptor: %s", strerror(errno));
      err++;
    }
  else
    {
      size_t lenin = strlen((const char*)in) + 1;

      if (iconv(cv,
		(char**)&(in), &(lenin),
		(char**)&(out), &(lenout)) == (size_t)-1)
	{
	  btrace("error in iconv: %s", strerror(errno));
	  err++;
	}

      if (iconv_close(cv) == -1)
	{
	  btrace("error closing iconv descriptor: %s", strerror(errno));
	  err++;
	}
    }

  return err;
}
