/*
  gmtcol.h

  Copyright (c) J.J. Green 2013
*/

#ifndef GMTCOL_H
#define GMTCOL_H

struct gmtcol_t 
{
  char *name; 
  unsigned char r, g, b; 
};

extern const struct gmtcol_t* gmtcol(const char*);

#endif
