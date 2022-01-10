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

void
print_key (FILE *fp, datum key, int delim)
{
  size_t i;
  
  for (i = 0; i < key.dsize && key.dptr[i]; i++)
    {
      if (key.dptr[i] == delim || key.dptr[i] == '\\')
	fputc ('\\', fp);
      fputc (key.dptr[i], fp);
    }
}

char *parseopt_program_doc = "fetch keys from GDBM database";
char *parseopt_program_args = "DBNAME KEY [KEY...]";

static struct gdbm_option gtfetch_options[] = {
  { 'd', "delimiter", "CHAR", "CHAR delimits key and value (default: horizontal tab)" },
  { '0', "null", NULL, "include trailing null to key length" },
  { 0 }
};

struct gtfetch_params
{
  int delimiter;
  int null_opt;
};

#define GTFETCH_PARAMS_STATIC_INITIALIZER {}

static int
gtfetch_option_parser (int key, char *arg, void *closure,
		      struct gdbm_test_config *oc)
{
  struct gtfetch_params *p = closure;
  
  switch (key)
    {
    case 'd':
      p->delimiter = arg[0];
      break;

    case '0':
      p->null_opt = 1;
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
  struct gtfetch_params params = GTFETCH_PARAMS_STATIC_INITIALIZER;
  datum key;
  datum data;
  int rc = 0;
  
  dbf = gdbm_test_init (argc, argv,
			GDBM_TESTOPT_DATABASE, GDBM_TESTDB_ARG,
			GDBM_TESTOPT_OPEN_FLAGS, GDBM_WRITER,
			GDBM_TESTOPT_OPTIONS, gtfetch_options,
			GDBM_TESTOPT_PARSEOPT, gtfetch_option_parser, &params,
			GDBM_TESTOPT_RETURN_ARGC, &argc,
			GDBM_TESTOPT_RETURN_ARGV, &argv,
			GDBM_TESTOPT_EXIT_ERROR, 1,
			GDBM_TESTOPT_EXIT_USAGE, 1,//FIXME
			GDBM_TESTOPT_END);

  if (argc < 1)
    {
      error ("required arguments missing");
      exit (1);
    }
    
  while (argc--)
    {
      char *arg = *argv++;

      key.dptr = arg;
      key.dsize = strlen (arg) + !!params.null_opt;

      data = gdbm_fetch (dbf, key);
      if (data.dptr == NULL)
	{
	  rc = 2;
	  if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	    {
	      fprintf (stderr, "%s: ", progname);
	      print_key (stderr, key, params.delimiter);
	      fprintf (stderr, ": not found\n");
	    }
	  else
	    {
	      gdbm_perror ("error fetching %s", arg);
	    }
	  continue;
	}
      if (params.delimiter)
	{
	  print_key (stdout, key, params.delimiter);
	  fputc (params.delimiter, stdout);
	}
      
      fwrite (data.dptr, data.dsize - !!params.null_opt, 1, stdout);
      free (data.dptr);
      
      fputc ('\n', stdout);
    }

  if (gdbm_close (dbf))
    {
      gdbm_perror ("gdbm_close");
      rc = 3;
    }
  exit (rc);
}
