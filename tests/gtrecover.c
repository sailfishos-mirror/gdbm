/* This file is part of GDBM test suite.
   Copyright (C) 2016-2022 Free Software Foundation, Inc.

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
#include <gdbm.h>
#include <gdbmapp.h>
#include <gdbmtest.h>
#include <assert.h>

char *parseopt_program_doc = "Recover a GDBM database from failure";
char *parseopt_program_args = "DBNAME";

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

static struct gdbm_option gtrecover_options[] = {
  { 'v', "verbose", NULL, "verbose mode" },
  { OPT_BACKUP, "backup", NULL, "create backup copy of the database" },
  { OPT_MAX_FAILURES, "max-failures", "N", "max. number of failures" },
  { OPT_MAX_FAILED_KEYS, "max-failed-keys", "N", "max. number of failed keys" },
  { OPT_MAX_FAILED_BUCKETS, "max-failed-buckets", "N", "max. number of failed buckets" },
  { 0}
};

struct gtrecover_params
{
  gdbm_recovery rcvr;
  int rcvr_flags;
};

#define GTRECOVER_PARAMS_STATIC_INITIALIZER {}

static int
gtrecover_option_parser (int key, char *arg, void *closure,
			 struct gdbm_test_config *gtc)
{
  struct gtrecover_params *p = closure;
  
  switch (key)
    {
    case 'v':
      p->rcvr.errfun = err_printer;
      p->rcvr_flags |= GDBM_RCVR_ERRFUN;
      break;

    case OPT_BACKUP:
      p->rcvr_flags |= GDBM_RCVR_BACKUP;
      break;

    case OPT_MAX_FAILURES:
      p->rcvr.max_failures = gdbm_test_strtosize (arg, gtc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILURES;
      break;

    case OPT_MAX_FAILED_KEYS:
      p->rcvr.max_failed_keys = gdbm_test_strtosize (arg, gtc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILED_KEYS;
      break;
      
    case OPT_MAX_FAILED_BUCKETS:
      p->rcvr.max_failures = gdbm_test_strtosize (arg, gtc);
      p->rcvr_flags |= GDBM_RCVR_MAX_FAILED_BUCKETS;
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
  struct gtrecover_params params = GTRECOVER_PARAMS_STATIC_INITIALIZER;
  int rc = 0;

  dbf = gdbm_test_init (argc, argv,
			GDBM_TESTOPT_DATABASE, GDBM_TESTDB_ARG,
			GDBM_TESTOPT_OPTIONS, gtrecover_options,
			GDBM_TESTOPT_PARSEOPT, gtrecover_option_parser, &params,
			GDBM_TESTOPT_OPEN_FLAGS, GDBM_WRITER,
			GDBM_TESTOPT_EXIT_ERROR, 1,
			GDBM_TESTOPT_END);
  
  rc = gdbm_recover (dbf, &params.rcvr, params.rcvr_flags);

  if (gdbm_close (dbf))
    {
      gdbm_perror ("gdbm_close");
      rc = 3;
    }
  exit (rc);
}
