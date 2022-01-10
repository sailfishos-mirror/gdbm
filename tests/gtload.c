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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gdbm.h>
#include <gdbmapp.h>
#include <gdbmtest.h>

void
err_printer (void *data, char const *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, "\n");
}

enum
  {
    OPT_NULL = 256,
    OPT_RECOVER,
    OPT_BACKUP,
    OPT_MAX_FAILURES,
    OPT_MAX_FAILED_KEYS,
    OPT_MAX_FAILED_BUCKETS,
  };

static struct gdbm_option gtload_options[] = {
  { 'r', "replace", NULL, "replace existing keys" },
  { OPT_NULL, "null", NULL, "include trailing null to key length" },
  { 'v', "verbose", NULL, "verbose mode" },
  { 'd', "delimiter", "CHAR", "CHAR delimits key and value (default: horizontal tab)" },
  { OPT_RECOVER, "recover", NULL, "recovery mode" },
  
  { 0, NULL, NULL, "Recovery options" },
  { OPT_BACKUP, "backup", NULL, "create backup copy of the database" },
  { OPT_MAX_FAILURES, "max-failures", "N", "max. number of failures" },
  { OPT_MAX_FAILED_KEYS, "max-failed-keys", "N", "max. number of failed keys" },
  { OPT_MAX_FAILED_BUCKETS, "max-failed-buckets", "N", "max. number of failed buckets" },
  { 0}
};

struct gtload_params
{
  int delimiter;
  int replace;
  int null_opt;
  int verbose;
  int recover;
  gdbm_recovery rcvr;
  int rcvr_flags;
};

#define GTLOAD_PARAMS_STATIC_INITIALIZER \
  {					 \
    .delimiter = '\t',			 \
  }
  
static int
gtload_option_parser (int key, char *arg, void *closure,
			 struct gdbm_test_config *oc)
{
  struct gtload_params *p = closure;
  
  switch (key)
    {
    case 'r':
      p->replace = GDBM_REPLACE;
      break;

    case OPT_NULL:
      p->null_opt = 1;
      break;

    case 'v':
      p->verbose = 1;
      break;

    case 'd':
      p->delimiter = arg[0];
      break;

    case OPT_RECOVER:
      p->recover = 1;
      break;

    case OPT_BACKUP:
      p->rcvr_flags |= GDBM_RCVR_BACKUP;
      break;

    case OPT_MAX_FAILURES:
      p->rcvr.max_failures = gdbm_test_strtosize (arg, oc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILURES;
      break;

    case OPT_MAX_FAILED_KEYS:
      p->rcvr.max_failed_keys = gdbm_test_strtosize (arg, oc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILED_KEYS;
      break;
      
    case OPT_MAX_FAILED_BUCKETS:
      p->rcvr.max_failures = gdbm_test_strtosize (arg, oc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILED_BUCKETS;
      break;

    default:
      return 1;
    }
  return 0;
}

char *parseopt_program_doc = "load a GDBM database";
char *parseopt_program_args = "DBNAME";

int
main (int argc, char **argv)
{
  GDBM_FILE dbf;
  struct gtload_params params = GTLOAD_PARAMS_STATIC_INITIALIZER;
  int line = 0;
  char buf[1024];
  
  dbf = gdbm_test_init (argc, argv,
			GDBM_TESTOPT_DATABASE, GDBM_TESTDB_ARG,
			GDBM_TESTOPT_OPTIONS, gtload_options,
			GDBM_TESTOPT_PARSEOPT, gtload_option_parser, &params,
			GDBM_TESTOPT_OPEN_FLAGS, GDBM_WRCREAT,
			GDBM_TESTOPT_EXIT_ERROR, 1,
			GDBM_TESTOPT_END);

  if (params.verbose && params.recover)
    {
      params.rcvr.errfun = err_printer;
      params.rcvr_flags |= GDBM_RCVR_ERRFUN;
    }

  while (fgets (buf, sizeof buf, stdin))
    {
      size_t i, j;
      size_t len = strlen (buf);
      datum key;
      datum data;

      if (buf[len - 1] != '\n')
	{
	  error ("%d: line too long", line);
	  continue;
	}

      buf[--len] = 0;
      
      line++;

      for (i = j = 0; i < len; i++)
	{
	  if (buf[i] == '\\')
	    i++;
	  else if (buf[i] == params.delimiter)
	    break;
	  else
	    buf[j++] = buf[i];
	}

      if (buf[i] != params.delimiter)
	{
	  error ("%d: malformed line", line);
	  continue;
	}
      buf[j] = 0;
      
      key.dptr = buf;
      key.dsize = j + params.null_opt;
      data.dptr = buf + i + 1;
      data.dsize = strlen (data.dptr) + params.null_opt;
      if (gdbm_store (dbf, key, data, params.replace) != 0)
	{
	  error ("%d: item not inserted: %s", line, gdbm_db_strerror (dbf));
	  if (gdbm_needs_recovery (dbf) && params.recover)
	    {
	      int rc = gdbm_recover (dbf, &params.rcvr, params.rcvr_flags);
	      if (rc)
		gdbm_perror ("recovery failed");
	      params.recover = 0;
	    }
	  else
	    {
	      exit (1);
	    }
	}
    }
  if (gdbm_close (dbf))
    {
      gdbm_perror ("gdbm_close");
      exit (3);
    }
  exit (0);
}
