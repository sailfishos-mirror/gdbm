/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2022 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <gdbm.h>

enum
  {
    GDBM_TESTOPT_END,
    GDBM_TESTOPT_OPTIONS,
    GDBM_TESTOPT_PARSEOPT,
    GDBM_TESTOPT_RETURN_ARGC,
    GDBM_TESTOPT_RETURN_ARGV,
    GDBM_TESTOPT_DATABASE_NAME,
    GDBM_TESTOPT_DATABASE,
    GDBM_TESTOPT_OPEN_FLAGS,
    GDBM_TESTOPT_CREATE_MODE,
    GDBM_TESTOPT_BLOCK_SIZE,
    GDBM_TESTOPT_TIMING,
    GDBM_TESTOPT_EXIT_USAGE,
    GDBM_TESTOPT_EXIT_ERROR
  };

enum
  {
    /* All database options enabled.  Database name set by the -D option. */
    GDBM_TESTDB_DEFAULT,
    /* All database options except -D enabled.  Database name set by the
       first non-optional argv element. */
    GDBM_TESTDB_ARG,
    /* All database options disabled.  Database name not set or used. */
    GDBM_TESTDB_NONE
  };

union setopt_data
{
  int b;
  char *s;
  size_t sz;
  int i;
};

struct setopt
{
  struct setopt *next;
  int code;
  union setopt_data data;
};

struct gdbm_test_config 
{
  char *dbname;
  int block_size;
  int open_flags;
  int create_mode;
  struct setopt *head;
  struct setopt *tail;
  int exit_usage;
  int exit_error;
  int timing;
  struct timespec start;
  char *logname;
  FILE *logfile;
};

typedef int (*gdbm_test_option_parser_t) (int, char *, void *,
					  struct gdbm_test_config *);

void gdbm_test_usage_exit (struct gdbm_test_config *oc,
			   char const *fmt, ...);
void gdbm_test_error_exit (struct gdbm_test_config *oc,
			   char const *fmt, ...);
size_t gdbm_test_strtosize (char const *arg, struct gdbm_test_config *oc);


struct gdbm_test_config gdbm_test_parse_args_v (int argc, char **argv, va_list ap);
struct gdbm_test_config gdbm_test_parse_args (int argc, char **argv, ...);
GDBM_FILE gdbm_test_open (struct gdbm_test_config *gtc);
void gdbm_test_time (struct gdbm_test_config *gtc, struct timespec *ts);
void gdbm_test_log (struct gdbm_test_config *gtc, char const *fmt, ...);

GDBM_FILE gdbm_test_init (int argc, char **argv, ...);

