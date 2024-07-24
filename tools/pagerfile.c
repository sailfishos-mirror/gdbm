/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2024 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.   */

#include "autoconf.h"
#include <stdio.h>
#include <stdarg.h>
#include "gdbmtool.h"

void
pager_flush (struct pagerfile *pfp)
{
  if (pfp->bufsize > 0)
    {
      fwrite (pfp->bufbase, 1, pfp->bufsize, pfp->stream);
      pfp->bufsize = 0;
    }
  fflush (pfp->stream);
}

static void
pager_checklines (struct pagerfile *pfp)
{
  if (pfp->nlines > pfp->maxlines)
    {
      FILE *pagfp = popen (pfp->pager, "w");
      if (!pagfp)
	{
	  terror (_("cannot run pager `%s': %s"), pfp->pager, strerror (errno));
	  pfp->mode = mode_transparent;
	}
      else
	{
	  pfp->mode = mode_pager;
	  pfp->stream = pagfp;
	}
      pager_flush (pfp);
    }
}

static ssize_t
memccount (char const *str, int c, size_t len)
{
  ssize_t n = 0;
  for (; len > 0; len--, str++)
    if (*str == c)
      n++;
  return n;
}

ssize_t
pager_write (struct pagerfile *pfp, const char *buffer, size_t size)
{
  if (pfp->mode != mode_initial)
    {
      return fwrite (buffer, 1, size, pfp->stream);
    }
  
  while (pfp->bufsize + size > pfp->bufmax)
    pfp->bufbase = e2nrealloc (pfp->bufbase, &pfp->bufmax, 1);
  memcpy (pfp->bufbase + pfp->bufsize, buffer, size);
  pfp->bufsize += size;
  pfp->nlines += memccount (buffer, '\n', size);

  pager_checklines (pfp);
  
  return size;
}

ssize_t
pager_writez (struct pagerfile *pfp, const char *str)
{
  return pager_write (pfp, str, strlen (str));
}

void
pager_putc (struct pagerfile *pfp, int c)
{
  char buf[1];
  buf[0] = c;
  pager_write (pfp, buf, 1);
}

ssize_t
pager_writeln (struct pagerfile *pfp, const char *str)
{
  ssize_t ret = pager_writez (pfp, str);
  if (ret > 0)
    {
      if (pager_write (pfp, "\n", 1) == 1)
	ret++;
    }
  return ret;
}

ssize_t
pager_vprintf (struct pagerfile *pfp, const char *fmt, va_list ap)
{
  ssize_t ret;
  
  if (!pfp->bufbase)
    {
      if (pfp->bufmax == 0)
	pfp->bufmax = 512;
      
      pfp->bufbase = ecalloc (1, pfp->bufmax);
    }
  
  for (;;)
    {
      va_list aq;
      ssize_t n;
      char *bufptr = pfp->bufbase + pfp->bufsize;
      size_t buflen = pfp->bufmax - pfp->bufsize;
      
      va_copy (aq, ap);
      n = vsnprintf (bufptr, buflen, fmt, aq);
      va_end (aq);
      if (n < 0 || n >= buflen || !memchr (bufptr, '\0', n + 1))
	{
	  pfp->bufbase = e2nrealloc (pfp->bufbase, &pfp->bufmax, 1);
	}
      else
	{
	  if (pfp->mode == mode_initial)
	    pfp->nlines += memccount (bufptr, '\n', n);
	    
	  ret = n;
	  pfp->bufsize += n;
	  break;
	}
    }

  if (pfp->mode == mode_initial)
    pager_checklines (pfp);
  else
    pager_flush (pfp);

  return ret;
}

ssize_t
pager_printf (struct pagerfile *pfp, const char *fmt, ...)
{
  va_list ap;
  ssize_t ret;
  
  va_start (ap, fmt);
  ret = pager_vprintf (pfp, fmt, ap);
  va_end (ap);
  return ret;
}

void
pager_close (struct pagerfile *pfp)
{
  if (pfp->bufsize)
    pager_flush (pfp);
  if (pfp->mode == mode_pager)
    pclose (pfp->stream);
  free (pfp->bufbase);
  free (pfp->pager);
  free (pfp);
}

PAGERFILE *
pager_open (FILE *stream, size_t maxlines, char const *pager)
{
  struct pagerfile *pfp;

  pfp = ecalloc (1, sizeof (*pfp));

  pfp->stream = stream;
  if (maxlines > 0 && pager)
    {
      pfp->pager = estrdup (pager);
      pfp->mode = mode_initial;
      pfp->maxlines = maxlines;
    }
  else
    pfp->mode = mode_transparent;
  return pfp;
}
