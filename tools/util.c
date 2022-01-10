/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990-2022 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.    */

#include "gdbmtool.h"
#include <pwd.h>

char *
mkfilename (const char *dir, const char *file, const char *suf)
{
  char *tmp;
  size_t dirlen = strlen (dir);
  size_t suflen = suf ? strlen (suf) : 0;
  size_t fillen = strlen (file);
  size_t len;
  
  while (dirlen > 0 && dir[dirlen-1] == '/')
    dirlen--;

  len = dirlen + (dir[0] ? 1 : 0) + fillen + suflen;
  tmp = emalloc (len + 1);
  memcpy (tmp, dir, dirlen);
  if (dir[0])
    tmp[dirlen++] = '/';
  memcpy (tmp + dirlen, file, fillen);
  if (suf)
    memcpy (tmp + dirlen + fillen, suf, suflen);
  tmp[len] = 0;
  return tmp;
}

char *
tildexpand (char *s)
{
  if (s[0] == '~')
    {
      char *p = s + 1;
      size_t len = strcspn (p, "/");
      struct passwd *pw;

      if (len == 0)
	pw = getpwuid (getuid ());
      else
	{
	  char *user = emalloc (len + 1);
	  
	  memcpy (user, p, len);
	  user[len] = 0;
	  pw = getpwnam (user);
	  free (user);
	}
      if (pw)
	return mkfilename (pw->pw_dir, p + len + 1, NULL);
    }
  return estrdup (s);
}

int
vgetyn (const char *prompt, va_list ap)
{
  int state = 0;
  int c, resp;
  va_list aq;
  
  do
    {
      switch (state)
	{
	case 1:
	  if (c == ' ' || c == '\t')
	    continue;
	  resp = c;
	  state = 2;
	  /* fall through */
	case 2:
	  if (c == '\n')
	    {
	      switch (resp)
		{
		case 'y':
		case 'Y':
		  return 1;
		case 'n':
		case 'N':
		  return 0;
		default:
		  /* TRANSLATORS: Please, don't translate 'y' and 'n'. */
		  fprintf (stdout, "%s\n", _("Please, reply 'y' or 'n'"));
		}
	      /* fall through */
	    }
	  else
	    break;
	  
	case 0:
	  va_copy (aq, ap);
	  vfprintf (stdout, prompt, aq);
	  va_end (aq);
	  fprintf (stdout, " [y/n]?");
	  fflush (stdout);
	  state = 1;
	  break;
	}
    } while ((c = getchar ()) != EOF);
  exit (EXIT_USAGE);
}
	
int
getyn (const char *prompt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, prompt);
  rc = vgetyn (prompt, ap);
  va_end (ap);
  return rc;
}

int
strtosize (char const *arg, size_t *psize)
{
  char *p;
  unsigned long n;
  size_t size;

  errno = 0;
  n = strtoul (arg, &p, 10);
  if (errno)
    goto badsize;
  size = n;
  if (*p)
    {
#     define XB()							\
      if (SIZE_T_MAX / size < 1024)					\
	goto badsize;							\
      size <<= 10;

      switch (*p)
	{
	case 'e':
	case 'E':
	  {
	    size_t m = 1;

	    errno = 0;
	    n = strtoul (p + 1, &p, 10);
	    if (errno || *p)
	      goto badsize;
	    while (n--)
	      {
		if (SIZE_T_MAX / m < 10)
		  goto badsize;
		m *= 10;
	      }
	    if (SIZE_T_MAX / size < m)
	      goto badsize;
	    size *= m;
	  }
	  break;

	case 'g':
	case 'G':
	  XB ();
	  /* fall through */
	case 'm':
	case 'M':
	  XB ();
	  /* fall through */
	case 'k':
	case 'K':
	  XB ();
	  if (p[1] == 0)
	    break;
	  /* fall through */
	default:
	badsize:
	  return -1;
	}
    }
  *psize = size;
  return 0;
}

int
gdbm_symmap_string_to_int (char const *str, struct gdbm_symmap *map, int flags)
{
  int i;
# define PREFIX "GDBM_"
  static char prefix[] = PREFIX;
  static int prefix_len = sizeof (PREFIX) - 1;

  if (flags & GDBM_SYMMAP_GDBM)
    {
      if (((flags & GDBM_SYMMAP_CI) ? strncasecmp : strncmp)
	  (str, prefix, prefix_len) == 0)
	str += prefix_len;
    }

  for (i = 0; map[i].sym; i++)
    {
      if (((flags & GDBM_SYMMAP_CI) ? strcasecmp : strcmp)
	  (map[i].sym + ((flags & GDBM_SYMMAP_GDBM) ? prefix_len : 0), str) == 0)
	return map[i].tok;
    }

  return -1;
}

char const *
gdbm_symmap_int_to_string (int n, struct gdbm_symmap *map)
{
  int i;

  for (i = 0; map[i].sym; i++)
    if (map[i].tok == n)
      return map[i].sym;
  return NULL;
}
