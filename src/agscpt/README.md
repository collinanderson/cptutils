Notes
-----

This is an experimental program to convert ArcGIS style files to
GMT's cpt format.  Seems a bit of a long shot since it is a blob
of binary (described as an 'OLE object') sitting in an Access DB.
First install the `mdbtools-dev` package

	sudo apt-get install mdbtools-dev

to get through the Access layer.

That implemented, the resulting binary does not seem to be 'OLE',
at least it does not have the expected signature:

	d0 cf 11 e0 a1 b1 1a e1

instead we find

	99 70 b8 be b4 c0 d0 11  83 79 08 00 09 b9 96 cc  |.p.......y......|
	01 00 15 00 00 00 4d 00  75 00 6c 00 74 00 69 00  |......M.u.l.t.i.|
	2d 00 70 00 61 00 72 00  74 00 20 00 43 00 6f 00  |-.p.a.r.t. .C.o.|
	6c 00 6f 00 72 00 20 00  52 00 61 00 6d 00 70 00  |l.o.r. .R.a.m.p.|
	00 00 04 00 00 00 9b 70  b8 be b4 c0 d0 11 83 79  |.......p.......y|
	08 00 09 b9 96 cc 01 00  16 00 00 00 41 00 6c 00  |............A.l.|
	67 00 6f 00 72 00 69 00  74 00 68 00 6d 00 69 00  |g.o.r.i.t.h.m.i.|
	63 00 20 00 43 00 6f 00  6c 00 6f 00 72 00 20 00  |c. .C.o.l.o.r. .|
	52 00 61 00 6d 00 70 00  00 00 00 00 00 00 92 c4  |R.a.m.p.........|

From searching around, we find conceptually that and Esri ramp is a
_multi-part color ramp_, which can contain multiple ramps, in particular
the _algorithmic color ramp_, with a pair of RGBA colours (as chars) and
an algorithm: one of

- `CIE_LAB_ALGORITHM`
- `HSV_ALGORITHM`
- `LAB_LCH_ALGORITHM`

There are also _random_ and _preset_ colour-ramps, which are pretty much
what it says on the tin, and would not be amenable to conversion (for
utility and legal reasons, as for Photoshop Gradients).

See [here](http://desktop.arcgis.com/en/arcmap/10.3/map/styles-and-symbols/working-with-color-ramps.htm)
for a description.

As I see it, 2 main problems; first to find out how the colour values are
encoded, probably not insane (we can dump muliple examples and compare to
narrow it down), second to work out what the algorithms do.
