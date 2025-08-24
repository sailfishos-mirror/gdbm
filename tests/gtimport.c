/* This file is part of GDBM test suite.
   Copyright (C) 2025 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "gdbm.h"

void
db_perror (char const *msg)
{
  int en = errno;
  fprintf (stderr, "%s: %s", msg, gdbm_strerror (gdbm_errno));
  if (gdbm_check_syserr (gdbm_errno))
    fprintf (stderr, ": %s", strerror (en));
  fputc ('\n', stderr);
}

void
print_datum (datum v)
{
  char *p = (char*) v.dptr;
  int i;
  printf ("%d:", v.dsize);
  for (i = 0; i < v.dsize; i++)
    printf (" %02X", p[i]);
  putchar ('\n');
}

int
datumcmp (void const *a, void const *b)
{
  datum const *adat = a;
  datum const *bdat = b;
  return memcmp (adat->dptr, bdat->dptr,
		 adat->dsize > bdat->dsize ? adat->dsize : bdat->dsize);
}

int
main (int argc, char **argv)
{
  GDBM_FILE dbf = NULL;
  unsigned long line;
  gdbm_count_t rcount;
  datum *keys;
  int i;

  assert (argc == 2);

  if (gdbm_load (&dbf, argv[1], GDBM_INSERT,
		 GDBM_META_MASK_MODE|GDBM_META_MASK_OWNER, &line))
    {
      int en = errno;
      fprintf (stderr, "%s: %s", argv[1], gdbm_strerror (gdbm_errno));
      if (gdbm_check_syserr (gdbm_errno))
	fprintf (stderr, ": %s", strerror (en));
      fputc ('\n', stderr);
      return 1;
    }

  if (gdbm_count (dbf, &rcount))
    {
      db_perror ("gdbm_count");
      return 1;
    }
  keys = calloc (rcount, sizeof (keys[0]));
  assert (keys != NULL);

  keys[0] = gdbm_firstkey (dbf);
  for (i = 1; i < rcount; i++)
    {
      keys[i] = gdbm_nextkey (dbf, keys[i-1]);
      if (keys[i].dptr == NULL)
	{
	  db_perror ("gdbm_nextkey");
	  return 1;
	}
    }
  qsort (keys, rcount, sizeof (datum), datumcmp);
  for (i = 0; i < rcount; i++)
    {
      datum val;

      print_datum (keys[i]);
      val = gdbm_fetch (dbf, keys[i]);
      if (val.dptr == NULL)
	{
	  int en = errno;
	  fprintf (stderr, "can't get key: %s", gdbm_strerror (gdbm_errno));
	  if (gdbm_check_syserr (gdbm_errno))
	    fprintf (stderr, ": %s", strerror (en));
	  fputc ('\n', stderr);
	  return 1;
	}
      print_datum (val);
      putchar ('\n');
    }

  gdbm_close (dbf);
  return 0;
}
