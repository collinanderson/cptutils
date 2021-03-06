/*
  colour.h
  
  simple RGB colours for cptutils

  (c) J.J.Green 2001,2004
*/

#ifndef COLOUR_H
#define COLOUR_H

/*
  these are the GMT colour types : 

  rgb_t :  rgb triple of integers in the range 0-255
  hsv_t :  hsv triple of doubles, hue in the range 0-360.
           saturation & value in the range of 0-1. The hue
	   rotates clockwise (ie, corresponds to a gimp
	   GRAD_HSV_CW type gradient)
  hsvp_t : as hsv_t, but interpolating paths are in HSV
           space rather than RGB

  these correspond to colours as interpreted in cpt files.
*/

typedef enum {model_rgb, model_hsv, model_hsvp} model_t;

extern const char* model_name(model_t);

typedef struct rgb_t
{
  int red, green, blue;
} rgb_t;
 
typedef struct hsv_t
{
  double hue, sat, val;
} hsv_t;

typedef union colour_t
{
  rgb_t  rgb;
  hsv_t  hsv;
} colour_t;

/*
  gimp colour types rgbD and hsvD are triples of doubles
  int the range 0-1
*/

extern int hsvD_to_rgbD(const double*, double*);
extern int rgbD_to_hsvD(const double*, double*);

extern int rgbD_to_rgb(const double*, rgb_t*);
extern int rgb_to_rgbD(rgb_t, double*);

extern int rgbD_to_hsv(const double*, hsv_t*);
extern int hsv_to_rgbD(hsv_t, double*);

extern int grey_to_rgbD(int, double*);

extern int hsv_to_rgb(hsv_t, rgb_t*);

extern int parse_rgb(const char*, rgb_t*);
extern double colour_rgb_dist(colour_t, colour_t, model_t);

extern int rgb_interpolate(double, rgb_t, rgb_t, rgb_t*);
extern int hsv_interpolate(double, hsv_t, hsv_t, hsv_t*);
extern int colour_interpolate(double, colour_t, colour_t, model_t, colour_t*);

#endif

