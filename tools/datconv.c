/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990-2025 Free Software Foundation, Inc.

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
#include <wctype.h>

#define DEFFMT(name, type, fmt)			\
static int					\
 name (PAGERFILE *fp, void *ptr, int size)	\
{                                               \
  pager_printf (fp, fmt, *(type*) ptr);		\
  return size;                                  \
}

DEFFMT (f_char, char, "%c")
DEFFMT (f_short, short, "%hd")
DEFFMT (f_ushort, unsigned short, "%hu")
DEFFMT (f_int, int, "%d")
DEFFMT (f_uint, unsigned, "%u")
DEFFMT (f_long, long, "%ld")
DEFFMT (f_ulong, unsigned long, "%lu")
DEFFMT (f_llong, long long, "%lld")
DEFFMT (f_ullong, unsigned long long, "%llu")
DEFFMT (f_float, float, "%f")
DEFFMT (f_double, double, "%e")

static int
f_stringz (PAGERFILE *fp, void *ptr, int size)
{
  wchar_t wc;
  char *str = ptr;
  int i;

  mbtowc (NULL, NULL, 0);
  for (i = 0; i < size; )
    {
      int n = mbtowc (&wc, &str[i], MB_CUR_MAX);
      if (n == 0)
	break;
      if (n == -1 || !iswprint (wc))
	{
	  int c;
	  if ((c = escape (str[i])))
	    pager_printf (fp, "\\%c", c);
	  else
	    pager_printf (fp, "\\%03o", *(unsigned char*)(str+i));
	  i++;
	}
      else
	{
	  pager_write (fp, str + i, n);
	  i += n;
	}
    }
  return i + 1;
}

static int
f_string (PAGERFILE *fp, void *ptr, int size)
{
  wchar_t wc;
  char *str = ptr;
  int i;
  
  mbtowc (NULL, NULL, 0);
  for (i = 0; i < size; )
    {
      int n = mbtowc (&wc, &str[i], MB_CUR_MAX);
      if (n == 0)
	{
	  pager_printf (fp, "\\%03o", *(unsigned char*)(str+i));
	  i++;
	}
      else if (n == -1 || !iswprint (wc))
	{
	  int c;
	  if ((c = escape (str[i])))
	    pager_printf (fp, "\\%c", c);
	  else
	    pager_printf (fp, "\\%03o", *(unsigned char*)(str+i));
	  i++;
	}
      else
	{
	  pager_write (fp, str + i, n);
	  i += n;
	}
    }
  return i;
}

int
s_char (struct xdatum *xd, char *str)
{
  xd_store (xd, str, 1);
  return 0;
}

#define DEFNSCAN(name, type, temptype, strto)	\
int						\
name (struct xdatum *xd, char *str)             \
{                                               \
  temptype n;                                   \
  type t;                                       \
  char *p;                                      \
                                                \
  errno = 0;                                    \
  n = strto (str, &p, 0);                       \
  if (*p)                                       \
    return 1;                                   \
  if (errno == ERANGE || (t = n) != n)          \
    return 1;                                   \
  xd_store (xd, &t, sizeof (t));                \
  return 0;                                     \
}

DEFNSCAN(s_short, short, long, strtol);
DEFNSCAN(s_ushort, unsigned short, unsigned long, strtoul);
DEFNSCAN(s_int, int, long, strtol)
DEFNSCAN(s_uint, unsigned, unsigned long, strtol)
DEFNSCAN(s_long, long, long, strtoul)
DEFNSCAN(s_ulong, unsigned long, unsigned long, strtoul)
DEFNSCAN(s_llong, long long, long long, strtoll)
DEFNSCAN(s_ullong, unsigned long long, unsigned long long, strtoull)

int
s_double (struct xdatum *xd, char *str)
{
  double d;
  char *p;
  
  errno = 0;
  d = strtod (str, &p);
  if (errno || *p)
    return 1;
  xd_store (xd, &d, sizeof (d));
  return 0;
}

int
s_float (struct xdatum *xd, char *str)
{
  float d;
  char *p;
  
  errno = 0;
  d = strtod (str, &p);
  if (errno || *p)
    return 1;
  xd_store (xd, &d, sizeof (d));
  return 0;
}

int
s_stringz (struct xdatum *xd, char *str)
{
  xd_store (xd, str, strlen (str) + 1);
  return 0;
}

int
s_string (struct xdatum *xd, char *str)
{
  xd_store (xd, str, strlen (str));
  return 0;
}

static struct datadef datatab[] = {
  { "char",     sizeof(char),      f_char, s_char },
  { "short",    sizeof(short),     f_short, s_short },
  { "ushort",   sizeof(unsigned short), f_ushort, s_ushort },
  { "int",      sizeof(int),       f_int, s_int  },
  { "unsigned", sizeof(unsigned),  f_uint, s_uint },
  { "uint",     sizeof(unsigned),  f_uint, s_uint },
  { "long",     sizeof(long),      f_long, s_long },
  { "ulong",    sizeof(unsigned long),     f_ulong, s_ulong },
  { "llong",    sizeof(long long), f_llong, s_llong },
  { "ullong",   sizeof(unsigned long long), f_ullong, s_ullong },
  { "float",    sizeof(float),     f_float, s_float }, 
  { "double",   sizeof(double),    f_double, s_double },
  { "stringz",  0, f_stringz, s_stringz },
  { "string",   0, f_string, s_string },
  { NULL }
};

struct datadef *
datadef_lookup (const char *name)
{
  struct datadef *p;

  for (p = datatab; p->name; p++)
    if (strcmp (p->name, name) == 0)
      return p;
  return NULL;
}

struct dsegm *
dsegm_new (int type)
{
  struct dsegm *p = emalloc (sizeof (*p));
  p->next = NULL;
  p->type = type;
  return p;
}

struct dsegm *
dsegm_new_field (struct datadef *type, char *id, int dim)
{
  struct dsegm *p = dsegm_new (FDEF_FLD);
  p->v.field.type = type;
  p->v.field.name = id;
  p->v.field.dim = dim;
  return p;
}

void
dsegm_list_free (struct dsegm *dp)
{
  while (dp)
    {
      struct dsegm *next = dp->next;
      if (dp->type == FDEF_FLD)
	free (dp->v.field.name);
      free (dp);
      dp = next;
    }
}

struct dsegm *
dsegm_list_find (struct dsegm *dp, char const *name)
{
  for (; dp; dp = dp->next)
    if (dp->type == FDEF_FLD && dp->v.field.name &&
	strcmp (dp->v.field.name, name) == 0)
      break;
  return dp;
}

void
datum_format (PAGERFILE *fp, datum const *dat, struct dsegm *ds)
{
  int off = 0;
  char *delim[2];
  int first_field = 1;
  
  if (!ds)
    {
      pager_printf (fp, "%.*s\n", dat->dsize, dat->dptr);
      return;
    }

  if (variable_get ("delim1", VART_STRING, (void*) &delim[0]))
    abort ();
  if (variable_get ("delim2", VART_STRING, (void*) &delim[1]))
    abort ();
  
  for (; ds && off <= dat->dsize; ds = ds->next)
    {
      switch (ds->type)
	{
	case FDEF_FLD:
	  if (!first_field)
	    pager_writez (fp, delim[1]);
	  if (ds->v.field.name)
	    pager_printf (fp, "%s=", ds->v.field.name);
	  if (ds->v.field.dim > 1)
	    pager_printf (fp, "{ ");
	  if (ds->v.field.type->format)
	    {
	      int i, n;

	      for (i = 0; i < ds->v.field.dim; i++)
		{
		  if (i)
		    pager_write (fp, delim[0], strlen (delim[0]));
		  if (off + ds->v.field.type->size > dat->dsize)
		    {
		      pager_printf (fp, _("(not enough data)"));
		      off += dat->dsize;
		      break;
		    }
		  else
		    {
		      n = ds->v.field.type->format (fp,
						    (char*) dat->dptr + off,
						    ds->v.field.type->size ?
						      ds->v.field.type->size :
						      dat->dsize - off);
		      off += n;
		    }
		}
	    }
	  if (ds->v.field.dim > 1)
	    pager_printf (fp, " }");
	  first_field = 0;
	  break;
	  
	case FDEF_OFF:
	  off = ds->v.n;
	  break;
	  
	case FDEF_PAD:
	  off += ds->v.n;
	  break;
	}
    }
}

void
datum_format_file (FILE *fp, datum const *dat, struct dsegm *ds)
{
  PAGERFILE *pager = pager_open (fp, 0, NULL);
  datum_format (pager, dat, ds);
  pager_close (pager);
}

struct xdatum
{
  char *dptr;
  size_t dsize;
  size_t dmax;
  int off;
};

void
xd_expand (struct xdatum *xd, size_t size)
{
  if (xd->dmax < size || 1)
    {
      xd->dptr = erealloc (xd->dptr, size);
      memset (xd->dptr + xd->dmax, 0, size - xd->dmax);
      xd->dmax = size;
    }
}
  
void
xd_store (struct xdatum *xd, void *val, size_t size)
{
  xd_expand (xd, xd->off + size);
  memcpy (xd->dptr + xd->off, val, size);
  xd->off += size;
  if (xd->off > xd->dsize)
    xd->dsize = xd->off;
} 

static int
dsconv (struct xdatum *xd, struct dsegm *ds, struct kvpair *kv)
{
  int i;
  int err = 0;
  struct slist *s;
  
  if (!ds->v.field.type->scan)
    abort ();
  
  if (kv->type == KV_STRING && ds->v.field.dim > 1)
    {
      /* If a char[] value was supplied as a quoted string.
	 convert it to list for further processing */
      if (ds->v.field.type->size == 1)
	{
	  struct slist *head = slist_new_l (kv->val.s, 1);
	  struct slist *tail = head;
	  char *p;
	  for (p = kv->val.s + 1; *p; p++)
	    slist_insert (&tail, slist_new_l (p, 1));
	  free (kv->val.s);
	  kv->val.l = head;
	  kv->type = KV_LIST;
	}
    }
	  
  switch (kv->type)
    {
    case KV_STRING:
      err = ds->v.field.type->scan (xd, kv->val.s);
      if (err)
	lerror (&kv->loc, _("cannot convert"));
      break;
	      
    case KV_LIST:
      for (i = 0, s = kv->val.l; i < ds->v.field.dim && s; i++, s = s->next)
	{
	  err = ds->v.field.type->scan (xd, s->str);
	  if (err)
	    {
	      lerror (&kv->loc, _("cannot convert value #%d: %s"), i, s->str);
	      break;
	    }
	}
      if (s)
	{
	  lerror (&kv->loc, "surplus initializers ignored");
	  err = 1;
	}
    }
  return err;
}
  
static int
datum_scan_notag (datum *dat, struct dsegm *ds, struct kvpair *kv)
{
  struct xdatum xd;
  int err = 0;
  
  memset (&xd, 0, sizeof (xd));
  
  for (; err == 0 && ds && kv; ds = ds->next)
    {
      if (kv->key)
	{
	  lerror (&kv->loc,
		       _("mixing tagged and untagged values is not allowed"));
	  err = 1;
	  break;
	}
      
      switch (ds->type)
	{
	case FDEF_FLD:
	  err = dsconv (&xd, ds, kv);
	  kv = kv->next;
	  break;

	case FDEF_OFF:
	  xd_expand (&xd, ds->v.n);
	  xd.off = ds->v.n;
	  break;
	  
	case FDEF_PAD:
	  xd_expand (&xd, xd.off + ds->v.n);
	  xd.off += ds->v.n;
	  break;
	}
    }

  if (err)
    {
      free (xd.dptr);
      return 1;
    }

  dat->dptr  = xd.dptr;
  dat->dsize = xd.dsize;
      
  return 0;
}

static int
datum_scan_tag (datum *dat, struct dsegm *ds, struct kvpair *kvlist)
{
  struct xdatum xd;
  int err = 0;
  struct kvpair *kv;

  /* Check keywords for consistency */
  for (kv = kvlist; kv; kv = kv->next)
    {
      if (!kv->key)
	{
	  lerror (&kv->loc,
		  _("mixing tagged and untagged values is not allowed"));
	  return 1;
	}
      if (!dsegm_list_find (ds, kv->key))
	{
	  lerror (&kv->loc, _("%s: no such field in datum"), kv->key);
	  return 1;
	}
    }

  /* Initialize datum */
  memset (&xd, 0, sizeof (xd));

  for (; err == 0 && ds; ds = ds->next)
    {
      switch (ds->type)
	{
	case FDEF_FLD:
	  kv = kvlist_find (kvlist, ds->v.field.name);
	  if (kv)
	    err = dsconv (&xd, ds, kv);
	  else
	    {
	      size_t sz = ds->v.field.type->size * ds->v.field.dim;
	      xd_expand (&xd, xd.off + sz);
	      xd.off += sz;
	    }
	  break;

	case FDEF_OFF:
	  xd_expand (&xd, ds->v.n);
	  xd.off = ds->v.n;
	  break;
	  
	case FDEF_PAD:
	  xd_expand (&xd, xd.off + ds->v.n);
	  xd.off += ds->v.n;
	  break;
	}
    }

  if (err)
    {
      free (xd.dptr);
      return 1;
    }

  dat->dptr  = xd.dptr;
  dat->dsize = xd.dsize;
      
  return 0;
}

int
datum_scan (datum *dat, struct dsegm *ds, struct kvpair *kv)
{
  return (kv->key ? datum_scan_tag : datum_scan_notag) (dat, ds, kv);
}

void
dsprint (PAGERFILE *fp, int what, struct dsegm *ds)
{
  static char *dsstr[] = { "key", "content" };
  int delim;
  
  pager_printf (fp, "define %s", dsstr[what]);
  if (ds->next)
    {
      pager_printf (fp, " {\n");
      delim = '\t';
    }
  else
    delim = ' ';
  for (; ds; ds = ds->next)
    {
      switch (ds->type)
	{
	case FDEF_FLD:
	  pager_printf (fp, "%c%s", delim, ds->v.field.type->name);
	  if (ds->v.field.name)
	    pager_printf (fp, " %s", ds->v.field.name);
	  if (ds->v.field.dim > 1)
	    pager_printf (fp, "[%d]", ds->v.field.dim);
	  break;
	  
	case FDEF_OFF:
	  pager_printf (fp, "%coffset %d", delim, ds->v.n);
	  break;

	case FDEF_PAD:
	  pager_printf (fp, "%cpad %d", delim, ds->v.n);
	  break;
	}
      if (ds->next)
	pager_putc (fp, ',');
      pager_putc (fp, '\n');
    }
  if (delim == '\t')
    pager_writeln (fp, "}");
}
