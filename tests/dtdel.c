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
#include "dbm.h"
#include "progname.h"

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  char *dbname;
  datum key;
  int rc = 0;
  
  argc--;
  argv++;
  if (argc < 2)
    {
      fprintf (stderr, "%s: wrong arguments\n", progname);
      exit (1);
    }
  dbname = *argv;
  
  if (dbminit (dbname))
    {
      fprintf (stderr, "dbminit failed\n");
      exit (1);
    }

  while (--argc)
    {
      char *arg = *++argv;

      key.dptr = arg;
      key.dsize = strlen (arg);

      if (delete(key))
	{
	  fprintf (stderr, "%s: cannot delete %s: %s\n",
		   progname, arg, gdbm_strerror (gdbm_errno));
	  rc = 2;
	}
    }
  dbmclose ();
  exit (rc);
}
