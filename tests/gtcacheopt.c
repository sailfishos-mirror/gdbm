/*
  NAME
    gtcacheopt - test GDBM_GETCACHESIZE and GDBM_SETCACHESIZE options.

  SYNOPSIS
    gtcacheopt [-v]

  DESCRIPTION
    Reducing the cache size should retain most recently used elements
    and ensure correct rehashing.

    Operation:

    1) Create new database.
    2) Generate at least 10 full buckets,
    3) Check GDBM_GETCACHESIZE.
    4) Get cache statistics.
    5) Save directory indices and bucket addresses of first 8
       most recently used cache elements.
    6) Set cache size to 8.
    7) Verify that buckets saved in step 5 are still in place and
       can be located via the cache table.

  OPTIONS
     -v   Verbosely print what's being done.

  EXIT CODE
     0    success
     1    failure
     
  LICENSE
    This file is part of GDBM test suite.
    Copyright (C) 2021 Free Software Foundation, Inc.

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
#include "gdbmdefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

char dbname[] = "a.db";
int verbose = 0;

#define NBUCKETS 10
#define CACHE_SIZE 8
#define DATASIZE (4*IGNORE_SIZE)

static void
test_getcachesize (GDBM_FILE dbf, size_t expected_size)
{
  size_t size;
  int cache_auto;

  if (gdbm_setopt (dbf, GDBM_GETCACHESIZE, &size, sizeof (size)))
    {
      fprintf (stderr, "GDBM_GETCACHESIZE: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  if (verbose)
    printf ("size = %zu\n", size);

  if (expected_size && expected_size != size)
    {
      fprintf (stderr, "expected_size != size (%zu != %zu)\n",
	       expected_size, size);
      exit (1);
    }
  
  if (gdbm_setopt (dbf, GDBM_GETCACHEAUTO, &cache_auto, sizeof (cache_auto)))
    {
      fprintf (stderr, "GDBM_GETCACHESIZE: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  if (verbose)
    printf ("cache_auto = %d\n", cache_auto);

  if (expected_size && cache_auto != 0)
    {
      fprintf (stderr, "cache_auto != 0\n");
      exit (1);
    }
}

static int
bi_dir (GDBM_FILE dbf, off_t adr)
{
  int i;

  for (i = 0; i < dbf->header->dir_size; i++)
    if (dbf->dir[i] == adr)
      return i;
  fprintf (stderr, "bucket not found in dir?\n");
  exit (1);
}

int
main (int argc, char **argv)
{
  GDBM_FILE dbf;
  datum key, content;
  
  int nkeys;

  char data[DATASIZE];

  int i;

  cache_elem *elem;
  struct bucket_info
  {
    off_t bi_dir;
    hash_bucket *bi_bucket;
  } *binfo;
  
  while ((i = getopt (argc, argv, "v")) != EOF)
    {
      switch (i)
	{
	case 'v':
	  verbose++;
	  break;

	default:
	  return 2;
	}
    }

  if (verbose)
    printf ("creating database\n");
  
  dbf = gdbm_open (dbname, GDBM_MIN_BLOCK_SIZE, GDBM_NEWDB, 0644, NULL);
  if (!dbf)
    {
      fprintf (stderr, "gdbm_open: %s\n", gdbm_strerror (gdbm_errno));
      return 1;
    }

  /* Initialize keys. */
  nkeys = NBUCKETS * dbf->header->bucket_elems;

  /* Initialize content */
  for (i = 0; i < DATASIZE; i++)
    data[i] = i+1;
  content.dsize = DATASIZE;
  content.dptr = data;

  /* Populate the database */
  if (verbose)
    printf ("populating database (%d keys)\n", nkeys);
  key.dsize = sizeof (i);
  key.dptr = (char*) &i;
  for (i = 0; i < nkeys; i++)
    {
      if (gdbm_store (dbf, key, content, 0) != 0)
	{
	  fprintf (stderr, "%d: item not inserted: %s\n",
		   i, gdbm_db_strerror (dbf));
	  gdbm_close (dbf);
	  return 1;
	}
    }

  /* Test the GDBM_GETCACHESIZE option */
  test_getcachesize (dbf, 0);

  if (verbose)
    printf ("examining cache list\n");
  binfo = calloc (CACHE_SIZE, sizeof (binfo[0]));
  assert (binfo != NULL);

  for (i = 0, elem = dbf->cache_mru; i < CACHE_SIZE; elem = elem->ca_next, i++)
    {
      if (!elem)
	{
	  fprintf (stderr, "not enough elements in cache (%d)\n", i);
	  exit (1);
	}
      binfo[i].bi_dir = bi_dir (dbf, elem->ca_adr);
      binfo[i].bi_bucket = elem->ca_bucket;
    }

  if (verbose)
    printf ("setting new cache size\n");
  i = CACHE_SIZE;
  if (gdbm_setopt (dbf, GDBM_SETCACHESIZE, &i, sizeof (i)))
    {
      fprintf (stderr, "GDBM_GETCACHESIZE: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  if (verbose)
    printf ("verifying cache list\n");

  test_getcachesize (dbf, CACHE_SIZE);
  
  for (i = 0, elem = dbf->cache_mru; i < CACHE_SIZE; elem = elem->ca_next, i++)
    {
      if (!elem)
	{
	  fprintf (stderr, "not enough elements in cache (%d)\n", i);
	  exit (1);
	}
      if (dbf->dir[binfo[i].bi_dir] != elem->ca_adr)
	{
	  fprintf (stderr, "%d: address mismatch\n", i);
	  exit (1);
	}
    }

  if (verbose)
    printf ("verifying cache table\n");

  for (i = 0; i < CACHE_SIZE; i++)
    {
      if (_gdbm_get_bucket (dbf, binfo[i].bi_dir))
	{
	  fprintf (stderr, "%d: can't get bucket: %s\n", i,
		   gdbm_db_strerror (dbf));
	  exit (1);
	}
      if (dbf->bucket != binfo[i].bi_bucket)
	{
	  fprintf (stderr, "%d: bucket pointer mismatch\n", i);
	  exit (1);
	}
    }	  
  
  gdbm_close (dbf);

  return 0;
}
  

  
