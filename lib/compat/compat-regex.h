/* Definitions for data structures and routines for the regular
   expression library.
   Copyright (C) 1985,1989-93,1995-98,2000,2001,2002,2003
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* Cleaned from _RE_ARGS and other stuff for Thy. */

#ifndef __DOXYGEN__

#if defined(HAVE_REGEX_H) && defined(HAVE_REGCOMP) && \
    defined(HAVE_WORKING_REGEX_H)
#include <sys/types.h>
#include <regex.h>
#else

#warning You are using the compatilibty regex definitions.
#warning They might not match your system!

#ifndef _COMPAT_REGEX_H
#define _COMPAT_REGEX_H 1

#include <sys/types.h>

/* The following bits are used to determine the regexp syntax we
   recognize.  The set/not-set meanings are chosen so that Emacs syntax
   remains the value 0.  The bits are given in alphabetical order, and
   the definitions shifted by one from the previous bit; thus, when we
   add or remove a bit, only one other definition need change.  */
typedef unsigned long int reg_syntax_t;

/* POSIX `cflags' bits (i.e., information for `regcomp').  */

/* If this bit is set, then use extended regular expression syntax.
   If not set, then use basic regular expression syntax.  */
#define REG_EXTENDED 1


#define REG_NOERROR 0
#define REG_NOMATCH 1

/* This data structure represents a compiled pattern.  Before calling
   the pattern compiler, the fields `buffer', `allocated', `fastmap',
   `translate', and `no_sub' can be set.  After the pattern has been
   compiled, the `re_nsub' field is available.  All other fields are
   private to the regex routines.  */

#ifndef RE_TRANSLATE_TYPE
# define RE_TRANSLATE_TYPE char *
#endif

struct re_pattern_buffer
{
/* [[[begin pattern_buffer]]] */
	/* Space that holds the compiled pattern.  It is declared as
          `unsigned char *' because its elements are
           sometimes used as array indexes.  */
  unsigned char *buffer;

	/* Number of bytes to which `buffer' points.  */
  unsigned long int allocated;

	/* Number of bytes actually used in `buffer'.  */
  unsigned long int used;

	/* Syntax setting with which the pattern was compiled.  */
  reg_syntax_t syntax;

	/* Pointer to a fastmap, if any, otherwise zero.  re_search uses
           the fastmap, if there is one, to skip over impossible
           starting points for matches.  */
  char *fastmap;

	/* Either a translate table to apply to all characters before
           comparing them, or zero for no translation.  The translation
           is applied to a pattern when it is compiled and to a string
           when it is matched.  */
  RE_TRANSLATE_TYPE translate;

	/* Number of subexpressions found by the compiler.  */
  size_t re_nsub;

	/* Zero if this pattern cannot match the empty string, one else.
           Well, in truth it's used only in `re_search_2', to see
           whether or not we should use the fastmap, so we don't set
           this absolutely perfectly; see `re_compile_fastmap' (the
           `duplicate' case).  */
  unsigned can_be_null : 1;

	/* If REGS_UNALLOCATED, allocate space in the `regs' structure
             for `max (RE_NREGS, re_nsub + 1)' groups.
           If REGS_REALLOCATE, reallocate space if necessary.
           If REGS_FIXED, use what's there.  */
#define REGS_UNALLOCATED 0
#define REGS_REALLOCATE 1
#define REGS_FIXED 2
  unsigned regs_allocated : 2;

	/* Set to zero when `regex_compile' compiles a pattern; set to one
           by `re_compile_fastmap' if it updates the fastmap.  */
  unsigned fastmap_accurate : 1;

	/* If set, `re_match_2' does not return information about
           subexpressions.  */
  unsigned no_sub : 1;

	/* If set, a beginning-of-line anchor doesn't match at the
           beginning of the string.  */
  unsigned not_bol : 1;

	/* Similarly for an end-of-line anchor.  */
  unsigned not_eol : 1;

	/* If true, an anchor at a newline matches.  */
  unsigned newline_anchor : 1;

/* [[[end pattern_buffer]]] */
};

typedef struct re_pattern_buffer regex_t;

/* Type for byte offsets within the string.  POSIX mandates this.  */
typedef int regoff_t;

/* POSIX specification for registers.  Aside from the different names
   than `re_registers', POSIX uses an array of structures, instead of
   a structure of arrays.  */
typedef struct
{
  regoff_t rm_so;  /* Byte offset from string's start to substring's
		      start. */
  regoff_t rm_eo;  /* Byte offset from string's start to substring's
		      end. */
} regmatch_t;

/* POSIX compatibility.  */
extern int regcomp (regex_t * ,
		    const char * __pattern,
		    int __cflags);

extern int regexec (const regex_t * ,
		    const char * __string, size_t __nmatch,
		    regmatch_t __pmatch[],
		    int __eflags);

extern size_t regerror (int __errcode, const regex_t *,
			char *__errbuf, size_t __errbuf_size);

extern void regfree (regex_t *);

#endif /* compat-regex.h */

#endif /* !defined(HAVE_REGEX_H) || !defined(HAVE_REGCOMP) */

#endif /* !__DOXYGEN__ */

/*
Local variables:
make-backup-files: t
version-control: t
trim-versions-without-asking: nil
arch-tag: 37e0fa64-c2a0-4600-8ea8-eb41ca0dc002
End:
*/
