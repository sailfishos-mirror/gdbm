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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "dbm.h"
#include "progname.h"

#define PAGSUF ".pag"
#define DELIM '\t'

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  char *basename;
  char *dbname;
  int line = 0;
  char buf[1024];
  datum key;
  datum data;
  
  if (argc != 2)
    {
      fprintf (stderr, "%s: wrong arguments\n", progname);
      exit (1);
    }
  basename = argv[1];
  
  /* Check if .pag file exists. Create it if it doesn't, as DBM
     cannot do it itself. */
  
  dbname = malloc (strlen (basename) + sizeof (PAGSUF));
  if (!dbname)
    abort ();

  strcat (strcpy (dbname, basename), PAGSUF);

  if (access (dbname, F_OK))
    {
      int fd = creat (dbname, 0644);
      if (fd < 0)
	{
	  fprintf (stderr, "%s: ", progname);
	  perror (dbname);
	  exit (1);
	}
      close (fd);
    }
  free (dbname);

  if (dbminit (basename))
    {
      fprintf (stderr, "dbminit failed\n");
      exit (1);
    }

  while (fgets (buf, sizeof buf, stdin))
    {
      size_t i, j;
      size_t len = strlen (buf);

      if (buf[len - 1] != '\n')
	{
	  fprintf (stderr, "%s: %d: line too long\n",
		   progname, line);
	  continue;
	}

      buf[--len] = 0;
      
      line++;

      for (i = j = 0; i < len; i++)
	{
	  if (buf[i] == '\\')
	    i++;
	  else if (buf[i] == DELIM)
	    break;
	  else
	    buf[j++] = buf[i];
	}

      if (buf[i] != DELIM)
	{
	  fprintf (stderr, "%s: %d: malformed line\n",
		   progname, line);
	  continue;
	}
      buf[j] = 0;
      
      key.dptr = buf;
      key.dsize = j;
      data.dptr = buf + i + 1;
      data.dsize = strlen (data.dptr);
      if (store (key, data) != 0)
	{
	  fprintf (stderr, "%s: %d: item not inserted\n",
		   progname, line);
	  exit (1);
	}
    }
  dbmclose ();
  exit (0);
}
