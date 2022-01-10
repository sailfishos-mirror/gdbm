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

#include "autoconf.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <gdbm.h>
#include <gdbmapp.h>
#include <gdbmdefs.h>
#include <gdbmtest.h>

static struct gdbm_symmap gdbm_opt_map[] = {
  { "GDBM_SETCACHESIZE",     GDBM_SETCACHESIZE },
  { "GDBM_GETCACHESIZE",     GDBM_GETCACHESIZE },
  { "GDBM_FASTMODE",         GDBM_FASTMODE },
  { "GDBM_SETSYNCMODE",      GDBM_SETSYNCMODE },
  { "GDBM_GETSYNCMODE",      GDBM_GETSYNCMODE },
  { "GDBM_SETCENTFREE",      GDBM_SETCENTFREE },
  { "GDBM_GETCENTFREE",      GDBM_GETCENTFREE },
  { "GDBM_SETCOALESCEBLKS",  GDBM_SETCOALESCEBLKS },
  { "GDBM_GETCOALESCEBLKS",  GDBM_GETCOALESCEBLKS },
#if HAVE_MMAP
  { "GDBM_SETMMAP",          GDBM_SETMMAP },
  { "GDBM_GETMMAP",          GDBM_GETMMAP },
  { "GDBM_SETMAXMAPSIZE",    GDBM_SETMAXMAPSIZE },
  { "GDBM_GETMAXMAPSIZE",    GDBM_GETMAXMAPSIZE },
#endif
  { "GDBM_GETFLAGS",         GDBM_GETFLAGS },
  { "GDBM_GETDBNAME",        GDBM_GETDBNAME },
  { "GDBM_GETBLOCKSIZE",     GDBM_GETBLOCKSIZE },
  { "GDBM_GETDBFORMAT",      GDBM_GETDBFORMAT },
  { "GDBM_GETDIRDEPTH",      GDBM_GETDIRDEPTH },
  { "GDBM_GETBUCKETSIZE",    GDBM_GETBUCKETSIZE },
  { "GDBM_GETCACHEAUTO",     GDBM_GETCACHEAUTO },
  { "GDBM_SETCACHEAUTO",     GDBM_SETCACHEAUTO },
  { NULL }
};

static struct gdbm_symmap gdbm_flag_map[] = {
  { "GDBM_SYNC",      GDBM_SYNC },
  { "GDBM_NOLOCK",    GDBM_NOLOCK },
  { "GDBM_NOMMAP",    GDBM_NOMMAP },
  { "GDBM_CLOEXEC",   GDBM_CLOEXEC },
  { "GDBM_BSEXACT",   GDBM_BSEXACT },
  { "GDBM_CLOERROR",  GDBM_CLOERROR },
  { "GDBM_XVERIFY",   GDBM_XVERIFY },
  { "GDBM_PREREAD",   GDBM_PREREAD },
  { "GDBM_NUMSYNC",   GDBM_NUMSYNC },
  { NULL }
};

/* Setopt data types */
enum
  {
    GDBM_OPTTYPE_BOOL,
    GDBM_OPTTYPE_STRING,
    GDBM_OPTTYPE_SIZE,
    GDBM_OPTTYPE_INT,
  };

static size_t optarg_size[] = {
  sizeof (int),
  sizeof (char *),
  sizeof (size_t),
  sizeof (int)
};

void
gdbm_test_usage_exit (struct gdbm_test_config *gtc, char const *fmt, ...)
{
  if (fmt)
    {
      va_list ap;
      va_start (ap, fmt);
      verror (fmt, ap);
      va_end (ap);
    }
  exit (gtc->exit_usage);
}

void
gdbm_test_error_exit (struct gdbm_test_config *gtc, char const *fmt, ...)
{
  if (fmt)
    {
      va_list ap;
      va_start (ap, fmt);
      verror (fmt, ap);
      va_end (ap);
    }
  exit (gtc->exit_error);
}

size_t
gdbm_test_strtosize (char const *str, struct gdbm_test_config *gtc)
{
  size_t sz;
  if (strtosize (str, &sz))
    gdbm_test_usage_exit (gtc, "not a valid size: %s", str);
  return sz;
}

struct setopt_def {
  int type;
  char const *descr;
  /* Data converter (for "set" options): */
  void (*conv) (char const *, union setopt_data *, struct gdbm_test_config *);
  /* Data handler (for "get" options): */
  void (*printer) (const union setopt_data *, GDBM_FILE);
};

static void
opt_bool_printer (const union setopt_data *dat, GDBM_FILE db)
{
  printf ("%s", dat->b ? "TRUE" : "FALSE");
}

static void
opt_string_printer (const union setopt_data *dat, GDBM_FILE db)
{
  printf ("%s", dat->s);
}

static void
opt_size_printer (const union setopt_data *dat, GDBM_FILE db)
{
  printf ("%zu", dat->sz);
}

static void
opt_int_printer (const union setopt_data *dat, GDBM_FILE db)
{
  printf ("%d", dat->i);
}

static void
opt_flags_printer (const union setopt_data *dat, GDBM_FILE db)
{
  int n = dat->i;
  int i;
  int delim = 0;

  for (i = 0; gdbm_flag_map[i].sym; i++)
    {
      if (gdbm_flag_map[i].tok & n)
	{
	  if (delim)
	    putchar (delim);
	  printf ("%s", gdbm_flag_map[i].sym);
	  n &= ~gdbm_flag_map[i].tok;
	  delim = '|';
	}
    }
  if (n)
    {
      if (delim)
	putchar (delim);
      printf ("%d", n);
    }
}

static void
opt_format_printer (const union setopt_data *dat, GDBM_FILE db)
{
  int n = dat->i;
  char const *format;

  switch (n)
    {
    case 0:
      format = "standard";
      break;

    case GDBM_NUMSYNC:
      format = "extended (numsync)";
      break;

    default:
      format = "unknown";
    }
  printf ("%s", format);
}

static void
opt_dirdepth_printer (const union setopt_data *dat, GDBM_FILE db)
{
  opt_int_printer (dat, db);
  printf (" (%zu bytes)", (size_t)1 << dat->i);
}

static void
opt_conv_bool (char const *str, union setopt_data *dat,
	       struct gdbm_test_config *gtc)
{
  static struct gdbm_symmap bool_map[] = {
    { "true", 1 },
    { "yes", 1 },
    { "on", 1 },
    { "1", 1 },
    { "false", 0 },
    { "no", 0 },
    { "off", 0 },
    { "0", 0 },
    { NULL }
  };
  int n = gdbm_symmap_string_to_int (str, bool_map, GDBM_SYMMAP_CI);
  if (n == -1)
    gdbm_test_usage_exit (gtc, "not a boolean: %s", str);
  dat->b = n;
}

static void
opt_conv_size (char const *str, union setopt_data *dat,
	       struct gdbm_test_config *gtc)
{
  dat->sz = gdbm_test_strtosize (str, gtc);
}

static struct setopt_def setopt_def[] = {
  [GDBM_SETCACHESIZE] = {
    .type = GDBM_OPTTYPE_SIZE,
    .descr = "cache size",
    .conv = opt_conv_size
  },
  [GDBM_GETCACHESIZE] = {
    .type = GDBM_OPTTYPE_SIZE,
    .descr = "cache size",
    .printer = opt_size_printer
  },
  [GDBM_FASTMODE] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "fast writes mode (obsolete)",
    .conv = opt_conv_bool
  },
  [GDBM_SETSYNCMODE] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "automatic database file synchronization after updates",
    .conv = opt_conv_bool
  },
  [GDBM_GETSYNCMODE] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "automatic database file synchronization after updates",
    .printer = opt_bool_printer
  },
  [GDBM_SETCENTFREE] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "central free block pool",
    .conv = opt_conv_bool
  },
  [GDBM_GETCENTFREE] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "central free block pool",
    .printer = opt_bool_printer
  },
  [GDBM_SETCOALESCEBLKS] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "coalescing free blocks",
    .conv = opt_conv_bool
  },
  [GDBM_GETCOALESCEBLKS] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "coalescing free blocks",
    .printer = opt_bool_printer
  },
#if HAVE_MMAP
  [GDBM_SETMMAP] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "memory mapping",
    .conv = opt_conv_bool
  },
  [GDBM_GETMMAP] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "memory mapping",
    .printer = opt_bool_printer
  },
  [GDBM_SETMAXMAPSIZE] = {
    .type = GDBM_OPTTYPE_SIZE,
    .descr = "maximum size of a memory mapped region",
    .conv = opt_conv_size
  },
  [GDBM_GETMAXMAPSIZE] = {
    .type = GDBM_OPTTYPE_SIZE,
    .descr = "maximum size of a memory mapped region",
    .printer = opt_size_printer
  },
#endif
  [GDBM_GETFLAGS] = {
    .type = GDBM_OPTTYPE_INT,
    .descr = "gdbm_open flags",
    .printer = opt_flags_printer
  },
  [GDBM_GETDBNAME] = {
    .type = GDBM_OPTTYPE_STRING,
    .descr = "database file name",
    .printer = opt_string_printer
  },
  [GDBM_GETBLOCKSIZE] = {
    .type = GDBM_OPTTYPE_INT,
    .descr = "database block size",
    .printer = opt_int_printer
  },
  [GDBM_GETDBFORMAT] = {
    .type = GDBM_OPTTYPE_INT,
    .descr = "database format",
    .printer = opt_format_printer
  },
  [GDBM_GETDIRDEPTH] = {
    .type = GDBM_OPTTYPE_INT,
    .descr = "database directory depth",
    .printer = opt_dirdepth_printer
  },
  [GDBM_GETBUCKETSIZE] = {
    .type = GDBM_OPTTYPE_SIZE,
    .descr = "bucket size",
    .printer = opt_size_printer
  },
  [GDBM_GETCACHEAUTO] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "automatic cache resize",
    .printer = opt_bool_printer
  },
  [GDBM_SETCACHEAUTO] = {
    .type = GDBM_OPTTYPE_BOOL,
    .descr = "automatic cache resize",
    .conv = opt_conv_bool
  }
};

static void
setopt_run (struct setopt *op, GDBM_FILE db,
	    struct gdbm_test_config *gtc)
{
  char const *name = gdbm_symmap_int_to_string (op->code, gdbm_opt_map);
  if (gdbm_setopt (db, op->code, &op->data, optarg_size[setopt_def[op->code].type]))
    {
      if (name)
	gdbm_perror ("gdbm_setopt(%s)", name);
      else
	gdbm_perror ("gdbm_setopt(%d)", op->code);
      //FIXME: If not implemented?
      exit (gtc->exit_error);
    }
  else if (setopt_def[op->code].printer)
    {
      printf ("%s: ", name);
      setopt_def[op->code].printer (&op->data, db);
      if (setopt_def[op->code].descr)
	printf (" # %s", setopt_def[op->code].descr);
      putchar ('\n');
    }
}

static void
setopt_add (char const *arg, struct gdbm_test_config *gtc)
{
  size_t len = strcspn (arg, "=");
  char *optname;
  int optcode;
  union setopt_data dat;
  struct setopt *opt;

  optname = emalloc (len + 1);
  memcpy (optname, arg, len);
  optname[len] = 0;

  optcode = gdbm_symmap_string_to_int (optname, gdbm_opt_map, GDBM_SYMMAP_DFL);
  if (optcode == -1)
    gdbm_test_usage_exit (gtc, "unknown or unsupported option name: %s",
			  optname);
  if (setopt_def[optcode].printer)
    {
      if (arg[len])
	gdbm_test_usage_exit (gtc,
			      "GDBM option %s can't be used with arguments",
			      optname);
    }
  else if (!arg[len])
    gdbm_test_usage_exit (gtc, "GDBM option %s requires an argument", optname);
  else
    setopt_def[optcode].conv(arg + len + 1, &dat, gtc);

  free (optname);
  opt = emalloc (sizeof (*opt));
  opt->code = optcode;
  opt->data = dat;
  opt->next = NULL;
  if (gtc->tail)
    gtc->tail->next = opt;
  else
    gtc->head = opt;
  gtc->tail = opt;
}

enum
  {
    OPT_BLOCK_SIZE    = 'B',
    OPT_CLEAR         = 'C',
    OPT_DATABASE_NAME = 'D',
    OPT_DEBUG         = 'X',
    OPT_GDBM_OPTION   = 'O',
    OPT_LOG_FILE      = 'L',
    OPT_OPEN_FLAGS    = 'F',
  };

static struct gdbm_option test_options[] = {
  { 0, NULL, "", "Database options" },
  { OPT_BLOCK_SIZE, "block-size", "SIZE", "set block size" },
  { OPT_OPEN_FLAGS, NULL, "GDBM_OPTION", "set gdbm_open flag" },
  { OPT_GDBM_OPTION, NULL, "GDBM_OPTION[=VALUE]", "set (or get) a GDBM option" },
#ifdef GDBM_DEBUG_ENABLE
  { OPT_DEBUG, "debug", "FLAG", "set debug flag" },
#endif
  { 0 }
};

static void
set_open_flag (char const *arg, struct gdbm_test_config *gtc)
{
  int flag = gdbm_symmap_string_to_int (arg, gdbm_flag_map, GDBM_SYMMAP_DFL);
  if (flag == -1)
    gdbm_test_usage_exit (gtc, "unknown or unsupported flag: %s", arg);
  gtc->open_flags |= flag;
}

static int
test_options_parser (int key, char *arg, void *closure,
		     struct gdbm_test_config *gtc)
{
  switch (key)
    {
    case OPT_BLOCK_SIZE:
      {
	size_t n = gdbm_test_strtosize (arg, gtc);
	if (n > INT_MAX)
	  gdbm_test_usage_exit (gtc, "block size out of range: %s", arg);
	gtc->block_size = n;
      }
      break;

    case OPT_OPEN_FLAGS:
      set_open_flag (arg, gtc);
      break;

    case OPT_GDBM_OPTION:
      setopt_add (arg, gtc);
      break;

#ifdef GDBM_DEBUG_ENABLE
    case OPT_DEBUG:
      {
	char *p;

	for (p = strtok (arg, ","); p; p = strtok (NULL, ","))
	  {
	    int f = gdbm_debug_token (p);
	    if (!f)
	      error ("unknown debug flag: %s", p);
	    else
	      gdbm_debug_flags |= f;
	  }
      }
      break;
#endif

    default:
      return 1;
    }
  return 0;
}

static struct gdbm_option database_option[] = {
  { OPT_DATABASE_NAME, "database", "NAME", "set database file name" },
  { 0 }
};

static int
database_option_parser (int key, char *arg, void *closure,
			struct gdbm_test_config *gtc)
{
  switch (key)
    {
    case OPT_DATABASE_NAME:
      gtc->dbname = arg;
      break;

    default:
      return 1;
    }
  return 0;
}

static struct gdbm_option clear_option[] = {
  { OPT_CLEAR, "clear", NULL, "clear the database before use" },
  { 0 }
};

static int
clear_option_parser (int key, char *arg, void *closure,
		     struct gdbm_test_config *gtc)
{
  switch (key)
    {
    case OPT_CLEAR:
      gtc->open_flags = GDBM_NEWDB | (gtc->open_flags & ~GDBM_OPENMASK);
      break;

    default:
      return 1;
    }
  return 0;
}

static struct gdbm_option timing_option[] = {
  { OPT_LOG_FILE, "logfile", "NAME", "set log file name" },
  { 0 }
};

static int
timing_option_parser (int key, char *arg, void *closure,
		      struct gdbm_test_config *gtc)
{
  switch (key)
    {
    case OPT_LOG_FILE:
      gtc->logname = arg;
      break;

    default:
      return 1;
    }
  return 0;
}

struct parser_closure
{
  gdbm_test_option_parser_t parser;
  void *closure;
};

struct parseopt_closure
{
  struct gdbm_option *optab;
  size_t optab_size;
  size_t optab_count;

  struct parser_closure *pcl;
  size_t pcl_size;
  size_t pcl_count;
};

#define PARSEOPT_CLOSURE_INITIALIZER {}

static void
parseopt_closure_free (struct parseopt_closure *pc)
{
  free (pc->optab);
  free (pc->pcl);
}

static void
end_options (struct parseopt_closure *pc)
{
  if (pc->optab_size == pc->optab_count)
    pc->optab = e2nrealloc (pc->optab, &pc->optab_size, sizeof (pc->optab[0]));
  memset (&pc->optab[pc->optab_count++], 0, sizeof (pc->optab[0]));
}

//FIXME: Duplicated
#define OPT_END(opt)							\
  ((opt)->opt_short == 0 && (opt)->opt_long == 0 && (opt)->opt_descr == NULL)

static void
add_options (struct parseopt_closure *pc, struct gdbm_option *op,
	     gdbm_test_option_parser_t pfn, void *data)
{
  if (op)
    {
      for (; !OPT_END (op); op++)
	{
	  if (pc->optab_size == pc->optab_count)
	    pc->optab = e2nrealloc (pc->optab, &pc->optab_size,
				    sizeof (pc->optab[0]));
	  pc->optab[pc->optab_count++] = *op;
	}
    }
  if (pfn)
    {
      if (pc->pcl_size == pc->pcl_count)
	pc->pcl = e2nrealloc (pc->pcl, &pc->pcl_size, sizeof (pc->pcl[0]));
      pc->pcl[pc->pcl_count].parser = pfn;
      pc->pcl[pc->pcl_count].closure = data;
      pc->pcl_count++;
    }
}

#ifdef GDBM_DEBUG_ENABLE
void
debug_printer (char const *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
}
#endif

struct gdbm_test_config
gdbm_test_parse_args_v (int argc, char **argv, va_list ap)
{
  int key;

  struct parseopt_closure pc = PARSEOPT_CLOSURE_INITIALIZER;

  int testdb = GDBM_TESTDB_DEFAULT;

  int *ret_argc = NULL;
  char ***ret_argv = NULL;

  struct gdbm_test_config gtc = {
    .open_flags = GDBM_READER,
    .create_mode = 0644,
    .exit_usage = EX_USAGE,
    .exit_error = EX_UNAVAILABLE
  };

  set_progname (argv[0]);

#ifdef GDBM_DEBUG_ENABLE
  gdbm_debug_printer = debug_printer;
#endif

  while ((key = va_arg (ap, int)) != GDBM_TESTOPT_END)
    {
      switch (key)
	{
	case GDBM_TESTOPT_OPTIONS:
	  add_options (&pc, va_arg (ap, struct gdbm_option *), NULL, NULL);
	  break;

	case GDBM_TESTOPT_PARSEOPT:
	  {
	    gdbm_test_option_parser_t pfn = va_arg (ap, gdbm_test_option_parser_t);
	    void *data = va_arg (ap, void *);
	    add_options (&pc, NULL, pfn, data);
	  }
	  break;

	case GDBM_TESTOPT_DATABASE:
	  testdb = va_arg (ap, int);
	  break;

	case GDBM_TESTOPT_BLOCK_SIZE:
	  gtc.block_size = va_arg (ap, int);
	  break;

	case GDBM_TESTOPT_RETURN_ARGC:
	  ret_argc = va_arg (ap, int *);
	  break;

	case GDBM_TESTOPT_RETURN_ARGV:
	  ret_argv = va_arg (ap, char ***);
	  break;

	case GDBM_TESTOPT_DATABASE_NAME:
	  gtc.dbname = va_arg (ap, char *);
	  break;

	case GDBM_TESTOPT_OPEN_FLAGS:
	  gtc.open_flags = va_arg (ap, int);
	  break;

	case GDBM_TESTOPT_CREATE_MODE:
	  gtc.create_mode = va_arg (ap, int);
	  break;

	case GDBM_TESTOPT_TIMING:
	  gtc.timing = va_arg (ap, int);
	  break;
	  
	case GDBM_TESTOPT_EXIT_USAGE:
	  gtc.exit_usage = va_arg (ap, int);
	  break;

	case GDBM_TESTOPT_EXIT_ERROR:
	  gtc.exit_error = va_arg (ap, int);
	  break;

	default:
	  abort ();
	}
    }

  if (testdb != GDBM_TESTDB_NONE)
    {
      add_options (&pc, test_options, test_options_parser, NULL);
      if (testdb == GDBM_TESTDB_DEFAULT)
	add_options (&pc, database_option, database_option_parser, NULL);
      if ((gtc.open_flags & GDBM_OPENMASK) == GDBM_WRCREAT)
	add_options (&pc, clear_option, clear_option_parser, NULL);
      if (gtc.timing)
	add_options (&pc, timing_option, timing_option_parser, NULL);
    }
  end_options (&pc);
  
  for (key = parseopt_first (argc, argv, pc.optab);
       key != EOF;
       key = parseopt_next ())
    {
      int res = -1;
      int i;

      if (key == '?')
	exit (gtc.exit_usage);

      for (i = 0; i < pc.pcl_count; i++)
	if ((res = pc.pcl[i].parser (key, optarg, pc.pcl[i].closure, &gtc)) == 0)
	  break;

      if (res)
	{
	  error ("unhandled option: %d", key);
	  exit (EX_SOFTWARE);
	}
    }

  argc -= optind;
  argv += optind;
  if (testdb == GDBM_TESTDB_ARG)
    {
      if (!argc)
	gdbm_test_usage_exit (&gtc, "required database name missing");
      gtc.dbname = argv[0];
      argv++;
      argc--;
    }
  else if (gtc.dbname)
    {
      gdbm_test_usage_exit (&gtc, "database name not set; use the -%c option",
			    OPT_DATABASE_NAME);
    }
  
  if (ret_argc)
    *ret_argc = argc;
  if (ret_argv)
    *ret_argv = argv;
  else if (argc > 0)
    gdbm_test_usage_exit (&gtc, "superfluous arguments");

  parseopt_closure_free (&pc);

  return gtc;
}

struct gdbm_test_config
gdbm_test_parse_args (int argc, char **argv, ...)
{
  struct gdbm_test_config gtc;
  va_list ap;
  
  va_start (ap, argv);
  gtc = gdbm_test_parse_args_v (argc, argv, ap);
  va_end (ap);

  return gtc;
}

GDBM_FILE
gdbm_test_open (struct gdbm_test_config *gtc)
{
  GDBM_FILE dbf = NULL;
  struct setopt *sop;

  if (gtc->dbname)
    {
      dbf = gdbm_open (gtc->dbname, gtc->block_size, gtc->open_flags,
		       gtc->create_mode, NULL);
      if (!dbf)
	{
	  gdbm_perror ("can't open %s", gtc->dbname);
	  gdbm_test_error_exit (gtc, NULL);
	}

      for (sop = gtc->head; sop != NULL;)
	{
	  struct setopt *next = sop->next;
	  setopt_run (sop, dbf, gtc);
	  free (sop);
	  sop = next;
	}

      if (gtc->timing)
	{
	  clock_gettime (CLOCK_MONOTONIC, &gtc->start);
	  if (gtc->logname)
	    {
	      gtc->logfile = fopen (gtc->logname, "w");
	      if (gtc->logfile == NULL)
		gdbm_test_error_exit (gtc, "can't open log file %s: %s",
				      gtc->logname, strerror (errno));
	    }
	  else
	    gtc->logfile = stdout;
	}
    }
  
  return dbf;
}

GDBM_FILE
gdbm_test_init (int argc, char **argv, ...)
{
  struct gdbm_test_config gtc;
  va_list ap;

  va_start (ap, argv);
  gtc = gdbm_test_parse_args_v (argc, argv, ap);
  va_end (ap);

  return gdbm_test_open (&gtc);
}

static void
ts_sub (struct timespec *a, struct timespec *b, struct timespec *res)
{
  res->tv_sec = a->tv_sec - b->tv_sec;
  res->tv_nsec = a->tv_nsec - b->tv_nsec;
  if (res->tv_nsec < 0)
    {
      --res->tv_sec;
      res->tv_nsec += 1e9;
    }
}

void
gdbm_test_time (struct gdbm_test_config *gtc, struct timespec *ts)
{
  struct timespec now;
  if (!gtc->timing)
    {
      error ("gdbm_test_time called, but timing is not enabled");
      abort ();
    }
  clock_gettime (CLOCK_MONOTONIC, &now);
  ts_sub (&now, &gtc->start, ts);
}

void
gdbm_test_log (struct gdbm_test_config *gtc, char const *fmt, ...)
{
  struct timespec d;

  gdbm_test_time (gtc, &d);
  fprintf (gtc->logfile, "%ld.%09ld", d.tv_sec, d.tv_nsec);
  if (fmt)
    {
      va_list ap;
      va_start (ap, fmt);
      fputc (' ', gtc->logfile);
      vfprintf (gtc->logfile, fmt, ap);
      va_end (ap);
    }
  fputc ('\n', gtc->logfile);
}
