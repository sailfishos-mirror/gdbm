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

char *parseopt_program_doc = "delete keys from GDBM database";
char *parseopt_program_args = "DBNAME KEY [KEY...]";

static struct gdbm_option gtdel_options[] = {
  { '0', "null", NULL, "include trailing null to key length" },
  { 0 }
};

struct gtdel_params
{
  int null_opt;
};

#define GTDEL_PARAMS_STATIC_INITIALIZER {}

static int
gtdel_option_parser (int key, char *arg, void *closure,
		     struct gdbm_test_config *oc)
{
  struct gtdel_params *p = closure;
  
  switch (key)
    {
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
  datum key;
  int rc = 0;
  struct gtdel_params params = GTDEL_PARAMS_STATIC_INITIALIZER;
  
  dbf = gdbm_test_init (argc, argv,
			GDBM_TESTOPT_DATABASE, GDBM_TESTDB_ARG,
			GDBM_TESTOPT_OPEN_FLAGS, GDBM_WRITER,
			GDBM_TESTOPT_OPTIONS, gtdel_options,
			GDBM_TESTOPT_PARSEOPT, gtdel_option_parser, &params,
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

      if (gdbm_delete(dbf, key))
	{
	  fprintf (stderr, "%s: cannot delete %s: %s\n",
		   progname, arg, gdbm_strerror (gdbm_errno));
	  rc = 2;
	}
    }
  if (gdbm_close (dbf))
    {
      fprintf (stderr, "gdbm_close: %s; %s\n", gdbm_strerror (gdbm_errno),
	       strerror (errno));
      rc = 3;
    }
  exit (rc);
}
