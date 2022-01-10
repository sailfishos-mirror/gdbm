/* This file is part of GDBM test suite.
   Copyright (C) 2011-2022 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.
*/
#include "autoconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gdbm.h>
#include <gdbmapp.h>
#include <gdbmtest.h>

char *parseopt_program_doc = "dump conents of a GDBM database";
char *parseopt_program_args = "DBNAME";

static struct gdbm_option gtdump_options[] = {
  { 'd', "delimiter", "CHAR", "CHAR delimits key and value (default: horizontal tab)" },
  { 0 }
};

struct gtdump_params
{
  int delimiter;
};

#define GTDUMP_PARAMS_STATIC_INITIALIZER \
  {					 \
    .delimiter = '\t',			 \
  }

static int
gtdump_option_parser (int key, char *arg, void *closure,
		      struct gdbm_test_config *oc)
{
  struct gtdump_params *p = closure;
  
  switch (key)
    {
    case 'd':
      p->delimiter = arg[0];
      break;

    default:
      return 1;
    }
  return 0;
}      

int
main (int argc, char **argv)
{
  GDBM_FILE dbf;
  struct gtdump_params params = GTDUMP_PARAMS_STATIC_INITIALIZER;
  datum key;
  datum data;
  
  dbf = gdbm_test_init (argc, argv,
			GDBM_TESTOPT_DATABASE, GDBM_TESTDB_ARG,
			GDBM_TESTOPT_OPTIONS, gtdump_options,
			GDBM_TESTOPT_PARSEOPT, gtdump_option_parser, &params,
			GDBM_TESTOPT_EXIT_ERROR, 1,
			GDBM_TESTOPT_END);

  key = gdbm_firstkey (dbf);
  while (key.dptr)
    {
      size_t i;
      datum nextkey;
      
      for (i = 0; i < key.dsize && key.dptr[i]; i++)
	{
	  if (key.dptr[i] == params.delimiter || key.dptr[i] == '\\')
	    fputc ('\\', stdout);
	  fputc (key.dptr[i], stdout);
	}

      fputc (params.delimiter, stdout);

      data = gdbm_fetch (dbf, key);
      i = data.dsize;
      if (data.dptr[i-1] == 0)
	i--;
      
      fwrite (data.dptr, i, 1, stdout);
      free (data.dptr);
      
      fputc ('\n', stdout);

      nextkey = gdbm_nextkey (dbf, key);
      free (key.dptr);
      key = nextkey;
    }

  if (gdbm_errno != GDBM_ITEM_NOT_FOUND)
    {
      error ("unexpected error: %s", gdbm_strerror (gdbm_errno));
      exit (1);
    }
  
  if (gdbm_close (dbf))
    {
      gdbm_perror ("gdbm_close");
      exit (3);
    }
  exit (0);
}
