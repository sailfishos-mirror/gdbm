/* gdbmdefs.h - The include file for dbm.  Defines structure and constants. */

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
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.   */

#include "systems.h"
#include "gdbmconst.h"
#include "gdbm.h"

/* Determine our native magic number and bail if we can't. */
#if SIZEOF_OFF_T == 4
# define GDBM_MAGIC	GDBM_MAGIC32
# define GDBM_NUMSYNC_MAGIC GDBM_NUMSYNC_MAGIC32
#elif SIZEOF_OFF_T == 8
# define GDBM_MAGIC	GDBM_MAGIC64
# define GDBM_NUMSYNC_MAGIC GDBM_NUMSYNC_MAGIC64
#else
# error "Unsupported off_t size, contact GDBM maintainer.  What crazy system is this?!?"
#endif

#define DEFAULT_TEXT_DOMAIN PACKAGE
#include "gettext.h"

#define _(s) gettext (s)
#define N_(s) s

/* The width in bits of the integer type or expression T. */
#define TYPE_WIDTH(t) (sizeof (t) * CHAR_BIT)

#define SIGNED_TYPE_MAXIMUM(t) \
  ((t) ((((t) 1 << (TYPE_WIDTH (t) - 2)) - 1) * 2 + 1))

/* Maximum value for off_t */
#define OFF_T_MAX SIGNED_TYPE_MAXIMUM (off_t)

/* Return true if both A and B are non-negative offsets and A can be added
   to B without integer overflow */
static inline int
off_t_sum_ok (off_t a, off_t b)
{
  return a >= 0 && b >= 0 && OFF_T_MAX - a >= b;
}


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* The type definitions are next.  */

/* The available file space is stored in an "avail" table.  The one with
   most activity is contained in the file header. (See below.)  When that
   one fills up, it is split in half and half is pushed on an "avail
   stack."  When the active avail table is empty and the "avail stack" is
   not empty, the top of the stack is popped into the active avail table. */

/* The following structure is the element of the available table.  */
typedef struct
{
  int   av_size;                /* The size of the available block. */
  off_t  av_adr;                /* The file address of the available block. */
} avail_elem;

static inline void
avail_elem_init (avail_elem *elem, int size, off_t adr)
{
  /* Make sure any padding in elem is filled with 0. */
  memset (elem, 0, sizeof (*elem));
  elem->av_size = size;
  elem->av_adr = adr;
}

/* This is the actual table. The in-memory images of the avail blocks are
   allocated by malloc using a calculated size.  */
typedef struct
{
  int   size;             /* The number of avail elements in the table.*/
  int   count;            /* The number of entries in the table. */
  off_t next_block;       /* The file address of the next avail block. */
  avail_elem av_table[1]; /* The table.  Make it look like an array.  */
} avail_block;

/* The dbm file header keeps track of the current location of the hash
   directory and the free space in the file.  */

typedef struct
{
  int   header_magic;  /* Version of file. */
  int   block_size;    /* The optimal i/o blocksize from stat. */
  off_t dir;           /* File address of hash directory table. */
  int   dir_size;      /* Size in bytes of the table.  */
  int   dir_bits;      /* The number of address bits used in the table.*/
  int   bucket_size;   /* Size in bytes of a hash bucket struct. */
  int   bucket_elems;  /* Number of elements in a hash bucket. */
  off_t next_block;    /* The next unallocated block address. */
} gdbm_file_header;

/* The extension header keeps additional information. */
typedef struct
{
  int version;         /* Version number (currently 0). */
  unsigned numsync;    /* Number of synchronizations. */
  int pad[6];          /* Reserve space for further use. */
} gdbm_ext_header;

/* Standard GDBM file header. */
typedef struct
{
  gdbm_file_header hdr;
  avail_block avail;
} gdbm_file_standard_header;

/* Extended GDBM file header. */
typedef struct
{
  gdbm_file_header hdr;
  gdbm_ext_header ext;
  avail_block avail;
} gdbm_file_extended_header;

/* The dbm hash bucket element contains the full 31 bit hash value, the
   "pointer" to the key and data (stored together) with their sizes.  It also
   has a small part of the actual key value.  It is used to verify the first
   part of the key has the correct value without having to read the actual
   key. */

typedef struct
{
  int   hash_value;       /* The complete 31 bit value. */
  char  key_start[SMALL]; /* Up to the first SMALL bytes of the key.  */
  off_t data_pointer;     /* The file address of the key record. The
			     data record directly follows the key.  */
  int   key_size;         /* Size of key data in the file. */
  int   data_size;        /* Size of associated data in the file. */
} bucket_element;

/* A bucket is a small hash table.  This one consists of a number of
   bucket elements plus some bookkeeping fields.  The number of elements
   depends on the optimum blocksize for the storage device and on a
   parameter given at file creation time.  This bucket takes one block.
   When one of these tables gets full, it is split into two hash buckets.
   The contents are split between them by the use of the first few bits
   of the 31 bit hash function.  The location in a bucket is the hash
   value modulo the size of the bucket.  The in-memory images of the
   buckets are allocated by malloc using a calculated size depending of
   the file system buffer size.  To speed up write, each bucket will have
   BUCKET_AVAIL avail elements with the bucket. */

typedef struct
{
  int   av_count;            /* The number of bucket_avail entries. */
  avail_elem bucket_avail[BUCKET_AVAIL];  /* Distributed avail. */
  int   bucket_bits;         /* The number of bits used to get here. */
  int   count;               /* The number of element buckets full. */
  bucket_element h_table[1]; /* The table.  Make it look like an array.*/
} hash_bucket;

/* We want to keep from reading buckets as much as possible.  The following is
   to implement a bucket cache.  When full, buckets will be dropped in a
   least recently used order.  */

/* To speed up fetching and "sequential" access, we need to implement a
   data cache for key/data pairs read from the file.  To find a key, we
   must exactly match the key from the file.  To reduce overhead, the
   data will be read at the same time.  Both key and data will be stored
   in a data cache.  Each bucket cached will have a one element data
   cache.  */

typedef struct
{
  int     hash_val;
  int     data_size;
  int     key_size;
  char    *dptr;
  size_t  dsize;
  int     elem_loc;
} data_cache_elem;

typedef struct cache_elem cache_elem;

struct cache_elem
{
  off_t           ca_adr;
  char            ca_changed;  /* Data in the bucket changed. */
  data_cache_elem ca_data;     /* Cached datum */
  cache_elem      *ca_prev,    /* Previous element in LRU list. */
                  *ca_next,    /* Next elements in LRU list.
				  If the item is in cache_avail list, only
				  ca_next is used.  It points to the next
			          available element. */
                  *ca_coll;    /* Next element in a collision sequence */
  size_t          ca_hits;     /* Number of times this element was requested */
  hash_bucket     ca_bucket[1];/* Associated  bucket (dbf->header->bucket_size
				  bytes). */
};

/* Type of file locking in use. */
enum lock_type
  {
    LOCKING_NONE = 0,
    LOCKING_FLOCK,
    LOCKING_LOCKF,
    LOCKING_FCNTL
  };

/* This final structure contains all main memory based information for
   a gdbm file.  This allows multiple gdbm files to be opened at the same
   time by one program. */

struct gdbm_file_info
{
  /* Global variables and pointers to dynamic variables used by gdbm.  */

  /* The file name. */
  char *name;

  /* The reader/writer status. */
  unsigned read_write :2;

  /* Fast_write is set to 1 if no fsyncs are to be done. */
  unsigned fast_write :1;

  /* Central_free is set if all free blocks are kept in the header. */
  unsigned central_free :1;

  /* Coalesce_blocks is set if we should try to merge free blocks. */
  unsigned coalesce_blocks :1;

  /* Whether or not we should do file locking ourselves. */
  unsigned file_locking :1;

  /* Whether or not we're allowing mmap() use. */
  unsigned memory_mapping :1;

  /* Whether the database was open with GDBM_CLOEXEC flag */
  unsigned cloexec :1;

  /* Last error was fatal, the database needs recovery */
  unsigned need_recovery :1;

  /* Automatic bucket cache size */
  unsigned cache_auto :1;
  
  /* Last GDBM error number */
  gdbm_error last_error;
  /* Last system error number */
  int last_syserror;
  /* Last formatted error */
  char *last_errstr;

  enum lock_type lock_type;
  
  /* The fatal error handling routine. */
  void (*fatal_err) (const char *);

  /* The gdbm file descriptor which is set in gdbm_open.  */
  int desc;

  /* The file header holds information about the database. */
  gdbm_file_header *header;

  /* The table of available file space */
  avail_block *avail;
  size_t avail_size;  /* Size of avail, in bytes */

  /* Extended header (or NULL) */
  gdbm_ext_header *xheader;
  
  /* The hash table directory from extendable hashing.  See Fagin et al, 
     ACM Trans on Database Systems, Vol 4, No 3. Sept 1979, 315-344 */
  off_t *dir;

  /* The bucket cache. */
  int cache_bits;          /* Address bits used for computing bucket hash */
  size_t cache_size;       /* Cache capacity: 2^cache_bits */
  size_t cache_num;        /* Actual number of elements in cache */
  /* Cache hash table. */
  cache_elem **cache;
  
  /* Cache elements are linked in a list sorted by relative access time */
  cache_elem *cache_mru;   /* Most recently used element - head of the list */
  cache_elem *cache_lru;   /* Last recently used element - tail of the list */ 
  cache_elem *cache_avail; /* Pool of available elements (linked by prev, next)
			    */
  /* Points to dbf->cache_mru.ca_bucket -- the current hash bucket */
  hash_bucket *bucket;
  
  /* The directory entry used to get the current hash bucket. */
  int bucket_dir;

  /* Cache statistics */
  size_t cache_access_count; /* Number of cache accesses */
  size_t cache_hits;         /* Number of cache hits */
  
  /* Bookkeeping of things that need to be written back at the
     end of an update. */
  unsigned header_changed :1;
  unsigned directory_changed :1;

  off_t file_size;       /* Cached value of the current disk file size.
			    If -1, fstat will be used to retrieve it. */
  
  /* Mmap info */
  size_t mapped_size_max;/* Max. allowed value for mapped_size */
  void  *mapped_region;  /* Mapped region */
  size_t mapped_size;    /* Size of the region */
  off_t  mapped_pos;     /* Current offset in the region */
  off_t  mapped_off;     /* Position in the file where the region
			    begins */
  int mmap_preread :1;   /* 1 if prefault reading is requested */

#ifdef GDBM_FAILURE_ATOMIC

  int eo;                /* even/odd state:  Chooses among two snapshot files,
                            oscillates between 0 and 1. */
  int snapfd[2];         /* Snapshot files.  Must be in same filesystem
                            as GDBM data file, and this filesystem must
                            support ioctl(FICLONE). */

#endif /* GDBM_FAILURE_ATOMIC */
};

#define GDBM_DIR_COUNT(db) ((db)->header->dir_size / sizeof (off_t))

/* Offset of the avail block in GDBM header. */
#define GDBM_HEADER_AVAIL_OFFSET(db) \
  ((char*)(db)->avail - (char*)(db)->header)

/* Execute CODE without clobbering errno */
#define SAVE_ERRNO(code)                        \
  do                                            \
    {                                           \
      int __gc = gdbm_errno;			\
      int __ec = errno;                         \
      code;                                     \
      errno = __ec;                             \
      gdbm_errno = __gc;			\
    }                                           \
  while (0)                                     \

#define _GDBM_MAX_DUMP_LINE_LEN 76

/* Return immediately if the database needs recovery */	
#define GDBM_ASSERT_CONSISTENCY(dbf, onerr)			\
  do								\
    {								\
      if (dbf->need_recovery)					\
	{							\
          GDBM_SET_ERRNO (dbf, GDBM_NEED_RECOVERY, TRUE);	\
	  return onerr;						\
	}							\
    }								\
  while (0)

/* Debugging hooks */
#ifdef GDBM_DEBUG_ENABLE
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define _gdbm_str_(s) #s
#define _gdbm_cat_(a,b) a ":" _gdbm_str_(b)
#define __gdbm_locus__ _gdbm_cat_(__FILE__,__LINE__)

# define GDBM_DEBUG(flags, fmt, ...)					 \
  do									 \
    {									 \
      if (gdbm_debug_printer && gdbm_debug_flags & (flags))	 	 \
	SAVE_ERRNO (gdbm_debug_printer (__gdbm_locus__ ":%s: " fmt "\n", \
					__func__, __VA_ARGS__));	 \
    }						                         \
  while (0)

# define GDBM_DEBUG_DATUM(flags, dat, fmt, ...)				\
  do									\
    {									\
      if (gdbm_debug_printer && gdbm_debug_flags & (flags))		\
	{								\
	  SAVE_ERRNO(							\
	    gdbm_debug_printer (__gdbm_locus__ ":%s: " fmt "\n",        \
				__func__, __VA_ARGS__);			\
	    gdbm_debug_datum (dat, __gdbm_locus__": ");			\
	  );                                                            \
        }								\
    }									\
  while (0)

# define GDBM_SET_ERRNO2(dbf, ec, fatal, m)				\
  do									\
    {									\
      GDBM_DEBUG((m) | GDBM_DEBUG_ERR, "%s: error " #ec "%s",           \
		 ((dbf) ? ((GDBM_FILE)dbf)->name : "<nodbf>"),		\
		 ((fatal) ? " (needs recovery)" : ""));		        \
      gdbm_set_errno(dbf, ec, fatal);                                   \
    }                                                                   \
  while (0)
#else
# define GDBM_DEBUG(flags, fmt, ...)
# define GDBM_DEBUG_DATUM(flags, dat, fmt, ...)
# define GDBM_SET_ERRNO2(dbf, ec, fatal, m) gdbm_set_errno (dbf, ec, fatal)
#endif

# define GDBM_SET_ERRNO(dbf, ec, fatal) GDBM_SET_ERRNO2 (dbf, ec, fatal, 0)


/* Now define all the routines in use. */
#include "proto.h"
