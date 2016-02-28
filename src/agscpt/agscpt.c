/*
  agscpt.c

  (c) J.J.Green 2016
*/

#include <mdbtools.h>
#include <btrace.h>

#include "agscpt.h"

extern int agscpt(agscpt_opt_t opt)
{
  MdbHandle *mdb;

  if (!(mdb = mdb_open(opt.file.input, MDB_NOFLAGS)))
    {
      btrace("failed to open %s", opt.file.input);
      return 1;
    }

  if (!mdb_read_catalog(mdb, MDB_TABLE))
    {
      btrace("failed read catalog, not an Access DB?");
      return 1;
    }

  MdbTableDef *table;

  if (! (table = mdb_read_table_by_name(mdb, "Color Ramps", MDB_TABLE)))
    {
      btrace("Access DB has no 'Color Ramps' table");
      return 1;
    }


  mdb_read_columns(table);
  mdb_rewind_table(table);

  char **bound_values = g_malloc(table->num_cols * sizeof(char*));
  int  *bound_lens = g_malloc(table->num_cols * sizeof(int));

  for (int i = 0 ; i < table->num_cols ; i++)
    {
      bound_values[i] = (char*)g_malloc0(MDB_BIND_SIZE);
      mdb_bind_column(table, i+1, bound_values[i], &bound_lens[i]);
    }

#define NAME_COLUMN 1
#define OLE_COLUMN 3

  for (int i=0; i<table->num_cols; i++)
    {
      MdbColumn *col = g_ptr_array_index(table->columns,i);
      printf("%s\n", col->name);
    }

  while (mdb_fetch_row(table))
    {
      MdbColumn *col;

      col = g_ptr_array_index(table->columns, NAME_COLUMN);
      if (col->col_type != MDB_TEXT)
	{
	  btrace("Column %i has unexpected type (%i)",
		 NAME_COLUMN, col->col_type);
	  return 1;
	}
      char *name = bound_values[NAME_COLUMN];
      printf("%s %s\n", col->name, name);

      col = g_ptr_array_index(table->columns, OLE_COLUMN);
      if (col->col_type != MDB_OLE)
	{
	  btrace("Column %i has unexpected type (%i)",
		 OLE_COLUMN, col->col_type);
	  return 1;
	}
      size_t ole_length;
      char *ole = mdb_ole_read_full(mdb, col, &ole_length);
      printf("%s %zi\n", col->name, ole_length);

      // now use gsf_ole to parse the fucker

    }

  mdb_free_tabledef(table);
  mdb_close(mdb);

  return 0;
}
