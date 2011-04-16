/* -*- mode: c; c-file-style: "gnu" -*-
 * compat.c -- glibc-specific functions reimplemented
 * Copyright (C) 2002, 2003 Gergely Nagy <algernon@bonehunter.rulez.org>
 *
 * This file is part of The BoneHunter compat suite.
 *
 * The BoneHunter compat suite is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * The BoneHunter compat suite is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/** @file compat.c
 * GLibC-specific functions reimplemented.
 *
 * Since most BoneHunter software are tightly bound to the quirks of
 * GNU libc and there are no plans to change this, this file contains
 * emulations, wrappers and reimplementations of certain glibc
 * functions.
 */

#include "system.h"
#include "compat.h"

/* AIX hackery */
#if malloc == rpl_malloc
#undef malloc
#undef realloc
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <limits.h>
#ifdef HAVE_PATHS_H
#include <paths.h>
#endif
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef __DOXYGEN__
#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif
#endif

int
bhc_atoi (const char *str, long *result)
{
  char *endptr;
  long int res;

  res = strtol (str, &endptr, 10);
  if (res == LONG_MIN || res == LONG_MAX)
    return -1;
  if (*str != '\0' && *endptr == '\0')
    {
      *result = res;
      return 0;
    }
  else
    return -1;
}

/** Wrapper around malloc().
 * This is a simple wrapper around malloc(), which exits if the
 * allocation failed.
 */
void *
bhc_malloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0)
    bhc_exit (2);
  return value;
}

/** Wrapper around realloc().
 * This is a simple wrapper around realloc(), which exits if the
 * allocation failed.
 */
void *
bhc_realloc (void *ptr, size_t size)
{
  register void *value = realloc (ptr, size);
  if (value == 0)
    bhc_exit (2);
  return value;
}

/** Wrapper around calloc().
 * This is a simple wrapper around calloc(), which exits if the
 * allocation failed.
 */
void *
bhc_calloc (size_t nmemb, size_t size)
{
#ifndef __glibc__
  register void *value = calloc (nmemb + 1, size);
#else
  register void *value = calloc (nmemb, size);
#endif
  if (value == 0)
    bhc_exit (2);
  return value;
}

/** Wrapper around strdup().
 * This is a simple wrapper around strdup(), which exits if the
 * allocation failed, and handles a NULL source well.
 */
char *
bhc_strdup (const char *src)
{
  register char *value;
  if (!src)
    return NULL;

  value = strdup (src);
  if (value == 0)
    bhc_exit (2);
  return value;
}

/** Wrapper around strndup().
 * This is a simple wrapper around strndup(), which exits if the
 * allocation failed, and handles a NULL source well.
 */
char *
bhc_strndup (const char *src, size_t n)
{
  register char *value;
  if (!src)
    return NULL;

  value = strndup (src, n);
  if (value == 0)
    bhc_exit (2);
  return value;
}

/** Terminate the server process
 */
void
bhc_exit (int code)
{
  raise (SIGQUIT);
  exit (code);
}

#ifdef __GLIBC__
inline char *
bhc_getcwd (void)
{
  return getcwd (NULL, 0);
}
#else
char *
bhc_getcwd (void)
{
  char *cwd = (char *)bhc_malloc (PATH_MAX + 1);
  getcwd (cwd, PATH_MAX);
  return cwd;
}
#endif

#ifndef __DOXYGEN__

#ifndef HAVE_BASENAME
char *
basename (const char *filename)
{
  char *p = strrchr (filename, '/');
  return p ? p + 1 : (char *) filename;
}
#endif

#ifndef HAVE_DIRNAME
char *
dirname (char *path)
{
  static const char dot[] = ".";
  char *last_slash;

  last_slash = path != NULL ? strrchr (path, '/') : NULL;

  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0')
    {
      char *runp;
      size_t i;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != '/')
	  break;

      if (runp != path)
	{
	  i = runp - path;
	  do {i--;} while (path[i] != '/');
	  last_slash = &path[i];
	}
    }

  if (last_slash != NULL)
    {
      char *runp;

      for (runp = last_slash; runp != path; --runp)
	if (runp[-1] != '/')
	  break;

      if (runp == path)
	{
	  if (last_slash == path + 1)
	    ++last_slash;
	  else
	    last_slash = path + 1;
	}
      else
	last_slash = runp;

      last_slash[0] = '\0';
    }
  else
    path = (char *) dot;

  return path;
}
#endif

#ifndef HAVE_VSNPRINTF
int
vsnprintf (char *str, size_t size, const char *format, va_list ap)
{
  return (vsprintf (str, format, ap));
}
#endif

#ifndef HAVE_VASPRINTF
int
vasprintf (char **ptr, const char *fmt, va_list a)
{
  size_t size = 1024;
  int n;
  va_list ap;

  if ((*ptr = bhc_malloc (size)) == NULL)
    return -1;

  while (1)
    {
      va_copy (ap, a);
      n = vsnprintf (*ptr, size, fmt, ap);
      va_end (ap);

      if (n > -1 && n < (long) size)
	return n;

      if (n > -1)    /* glibc 2.1 */
	size = n+1;
      else	     /* glibc 2.0 */
	size *= 2;

      if ((*ptr = bhc_realloc (*ptr, size)) == NULL)
	return -1;
    }
}
#endif

#ifndef HAVE_ASPRINTF
int
asprintf(char **ptr, const char *fmt, ...)
{
  va_list ap;
  int i;

  va_start (ap, fmt);
  i = vasprintf (ptr, fmt, ap);
  va_end (ap);

  return i;
}
#endif

#ifndef HAVE_DAEMON
int
daemon(int nochdir, int noclose)
{
  int fd;

  switch (fork ())
    {
    case -1:
      return -1;
    case 0:
      break;
    default:
      _exit (0);
    }

  if (setsid () == -1)
    return -1;

  if (!nochdir)
    chdir ("/");

  if (!noclose && (fd = open (_PATH_DEVNULL, O_RDWR, 0)) != -1)
    {
      dup2 (fd, STDIN_FILENO);
      dup2 (fd, STDOUT_FILENO);
      dup2 (fd, STDERR_FILENO);
      if (fd > 2)
	close (fd);
    }
  else
    return -1;

  return 0;
}
#endif

#ifndef HAVE_CONFSTR
#define _CS_PATH_STR "/bin:/usr/bin"
size_t
confstr (int name, char *buf, size_t len)
{
  if (name != _CS_PATH)
    {
      errno = EINVAL;
      return 0;
    }

  if (len != 0)
    memcpy (buf, _CS_PATH_STR,
	    (len < sizeof (_CS_PATH_STR)) ? len : sizeof (_CS_PATH_STR));
  return sizeof (_CS_PATH_STR);
}
#endif

#ifndef HAVE_MEMPCPY
void *
mempcpy (void *TO, const void *FROM, size_t SIZE)
{
  memcpy (TO, FROM, SIZE);
  return ((void *) ((char *) TO + SIZE));
}
#endif

#ifndef HAVE_CANONICALIZE_FILE_NAME
/* The below implementation is (C) 1993 Rick Sladkey <jrs@world.std.com>
 * Released under the GPLv2. */
#define MAX_READLINKS 32
static char *
_bhc_realpath (const char *path, char *resolved_path)
{
  char copy_path[PATH_MAX];
  char link_path[PATH_MAX];
  char *new_path = resolved_path;
  char *max_path;
  int readlinks = 0;
  int n;

  /* Make a copy of the source path since we may need to modify it. */
  strcpy (copy_path, path);
  path = copy_path;
  max_path = copy_path + PATH_MAX - 2;
  /* If it's a relative pathname use getwd for starters. */
  if (*path != '/')
    {
      new_path = bhc_getcwd ();
      new_path += strlen (new_path);
      if (new_path[-1] != '/')
	*new_path++ = '/';
    }
  else
    {
      *new_path++ = '/';
      path++;
    }
  /* Expand each slash-separated pathname component. */
  while (*path != '\0')
    {
      /* Ignore stray "/". */
      if (*path == '/')
	{
	  path++;
	  continue;
	}
      if (*path == '.')
	{
	  /* Ignore ".". */
	  if (path[1] == '\0' || path[1] == '/')
	    {
	      path++;
	      continue;
	    }
	  if (path[1] == '.')
	    {
	      if (path[2] == '\0' || path[2] == '/')
		{
		  path += 2;
		  /* Ignore ".." at root. */
		  if (new_path == resolved_path + 1)
		    continue;
		  /* Handle ".." by backing up. */
		  while ((--new_path)[-1] != '/')
		    ;
		  continue;
		}
	    }
	}
      /* Safely copy the next pathname component. */
      while (*path != '\0' && *path != '/')
	{
	  if (path > max_path)
	    {
	      errno = ENAMETOOLONG;
	      return NULL;
	    }
	  *new_path++ = *path++;
	}
      /* Protect against infinite loops. */
      if (readlinks++ > MAX_READLINKS)
	{
	  errno = ELOOP;
	  return NULL;
	}
      /* See if latest pathname component is a symlink. */
      *new_path = '\0';
      n = readlink (resolved_path, link_path, PATH_MAX - 1);
      if (n < 0)
	{
	  /* EINVAL means the file exists but isn't a symlink. */
	  if (errno != EINVAL)
	    return NULL;
	}
      else
	{
	  /* Note: readlink doesn't add the null byte. */
	  link_path[n] = '\0';
	  if (*link_path == '/')
	    /* Start over for an absolute symlink. */
	    new_path = resolved_path;
	  else
	    /* Otherwise back up over this component. */
	    while (*(--new_path) != '/')
	      ;
	  /* Safe sex check. */
	  if (strlen (path) + n >= PATH_MAX)
	    {
	      errno = ENAMETOOLONG;
	      return NULL;
	    }
	  /* Insert symlink contents into path. */
	  strcat (link_path, path);
	  strcpy (copy_path, link_path);
	  path = copy_path;
	}
      *new_path++ = '/';
    }
  /* Delete trailing slash but don't whomp a lone slash. */
  if (new_path != resolved_path + 1 && new_path[-1] == '/')
    new_path--;
  /* Make sure it's null terminated. */
  *new_path = '\0';
  return resolved_path;
}

char *
canonicalize_file_name (const char *fn)
{
  char *cfn, *tmp;

  if (!fn)
    return NULL;

  cfn = (char *)bhc_malloc (PATH_MAX);
  tmp = _bhc_realpath (fn, cfn);

  if (!tmp)
    {
      free (cfn);
      return NULL;
    }

  return cfn;
}
#endif

#ifndef HAVE_STRNDUP
char *
strndup (const char *s, size_t size)
{
  char *ns = (char *)bhc_malloc (size + 1);
  memcpy (ns, s, size);
  ns[size] = '\0';
  return ns;
}
#endif

#ifndef HAVE_STRPTIME
/* FIXME: This one needs to be implemented */
char *
strptime (const char *s, const char *format, struct tm *tm)
{
  memset (tm, 0, sizeof (struct tm));
  return NULL;
}
#endif

/* getline() and getdelim() were taken from GNU Mailutils'
   mailbox/getline.c */
/* First implementation by Alain Magloire */
#ifndef HAVE_GETLINE
ssize_t
getline (char **lineptr, size_t *n, FILE *stream)
{
  return getdelim (lineptr, n, '\n', stream);
}
#endif

#ifndef HAVE_GETDELIM
/* Default value for line length.  */
static const int line_size = 128;

ssize_t
getdelim (char **lineptr, size_t *n, int delim, FILE *stream)
{
  size_t indx = 0;
  int c;

  /* Sanity checks.  */
  if (lineptr == NULL || n == NULL || stream == NULL)
    return -1;

  /* Allocate the line the first time.  */
  if (*lineptr == NULL)
    {
      *lineptr = bhc_malloc (line_size);
      if (*lineptr == NULL)
	return -1;
      *n = line_size;
    }

  while ((c = getc (stream)) != EOF)
    {
      /* Check if more memory is needed.  */
      if (indx >= *n)
	{
	  *lineptr = bhc_realloc (*lineptr, *n + line_size);
	  if (*lineptr == NULL)
	    return -1;
	  *n += line_size;
	}

      /* Push the result in the line.  */
      (*lineptr)[indx++] = c;

      /* Bail out.  */
      if (c == delim)
	break;
    }

  /* Make room for the null character.  */
  if (indx >= *n)
    {
      *lineptr = bhc_realloc (*lineptr, *n + line_size);
      if (*lineptr == NULL)
       return -1;
      *n += line_size;
    }

  /* Null terminate the buffer.  */
  (*lineptr)[indx++] = 0;

  /* The last line may not have the delimiter, we have to
   * return what we got and the error will be seen on the
   * next iteration.  */
  return (c == EOF && (indx - 1) == 0) ? -1 : (ssize_t)(indx - 1);
}
#endif

#ifndef HAVE_ARGP_PARSE
const char *argp_program_version;
const char *argp_program_bug_address;
void (*argp_program_version_hook) (FILE *stream,
				   struct argp_state *state);

static char *
argp_pre_v (const char *doc)
{
  char *pv = strchr (doc, '\v');
  char *str = strndup (doc, (pv != NULL) ? (size_t)(pv - doc) :
		       strlen (doc));
  return str;
}

static char *
argp_post_v (const char *doc)
{
  char *str = strchr (doc, '\v');
  return (str) ? (str + 1) : "";
}

error_t
argp_parse (const struct argp *argps, int argc, char **argv,
	    unsigned flags, int *arg_index, void *input)
{
  char *options, *tmp;
  int optpos = 0, optionspos = 0, optionssize = 30;
  struct argp_state *state;
  int c;
#if HAVE_GETOPT_LONG
  struct option *longopts;
  int longoptionspos = 0;

  longopts = (struct option *)bhc_calloc (optionssize,
					 sizeof (struct option));
#endif

  state = (struct argp_state *)bhc_malloc (sizeof (struct argp_state));
  state->input = input;
  state->argv0 = argv[0];

  options = (char *)bhc_calloc (optionssize, sizeof (char *));
  while (argps->options[optpos].name != NULL ||
	 argps->options[optpos].doc != NULL)
  {
    if (optionspos >= optionssize)
      {
	optionssize *= 2;
	XSREALLOC (options, char, optionssize);
#if HAVE_GETOPT_LONG
	XSREALLOC (longopts, struct option, optionssize);
#endif
      }
    if (!argps->options[optpos].name)
      {
	optpos++;
	continue;
      }

    options[optionspos++] = (char) argps->options[optpos].key;
    if (argps->options[optpos].arg)
      options[optionspos++] = ':';

#if HAVE_GETOPT_LONG
    longopts[longoptionspos].name = argps->options[optpos].name;
    if (argps->options[optpos].arg)
      {
	if (argps->options[optpos].flags & OPTION_ARG_OPTIONAL)
	  longopts[longoptionspos].has_arg = optional_argument;
	else
	  longopts[longoptionspos].has_arg = required_argument;
      }
    else
      longopts[longoptionspos].has_arg = no_argument;
    longopts[longoptionspos].flag = NULL;
    longopts[longoptionspos].val = argps->options[optpos].key;
    longoptionspos++;
#endif

    optpos++;
  }
  if (optionspos + 4 >= optionssize)
    {
      optionssize += 5;
      XSREALLOC (options, char, optionssize);
#if HAVE_GETOPT_LONG
      XSREALLOC (longopts, struct option, optionssize);
#endif
    }
  options[optionspos++] = 'V';
  options[optionspos++] = '?';
  options[optionspos] = '\0';
#if HAVE_GETOPT_LONG
  longopts[longoptionspos].name = "help";
  longopts[longoptionspos].has_arg = no_argument;
  longopts[longoptionspos].flag = NULL;
  longopts[longoptionspos].val = '?';
  longoptionspos++;
  longopts[longoptionspos].name = "version";
  longopts[longoptionspos].has_arg = no_argument;
  longopts[longoptionspos].flag = NULL;
  longopts[longoptionspos].val = 'V';
  longoptionspos++;
  longopts[longoptionspos].name = NULL;
  longopts[longoptionspos].has_arg = 0;
  longopts[longoptionspos].flag = NULL;
  longopts[longoptionspos].val = 0;
#endif

#if HAVE_GETOPT_LONG
  while ((c = getopt_long (argc, argv, options, longopts, NULL)) != -1)
#else
  while ((c = getopt (argc, argv, options)) != -1)
#endif
    {
      switch (c)
	{
	case '?':
	  tmp = argp_pre_v (argps->doc);
	  printf ("Usage: %s [OPTION...]\n%s\n\n", argv[0], tmp);
	  free (tmp);
	  optpos = 0;
	  while (argps->options[optpos].name != NULL ||
		 argps->options[optpos].doc != NULL)
	    {
	      if (!argps->options[optpos].name)
		{
		  printf (" %s\n", argps->options[optpos].doc);
		  optpos++;
		  continue;
		}
	      if (!(argps->options[optpos].flags & OPTION_HIDDEN))
#if HAVE_GETOPT_LONG
		printf ("  -%c, --%-15s %s%-12s\t%s%s\n",
			argps->options[optpos].key,
			argps->options[optpos].name,
			(argps->options[optpos].flags &
			 OPTION_ARG_OPTIONAL) ? "[" : "",
			(argps->options[optpos].arg) ?
			 argps->options[optpos].arg : "",
			 argps->options[optpos].doc,
			(argps->options[optpos].flags &
			 OPTION_ARG_OPTIONAL) ? "]" : "");
#else
		printf ("  -%c %-12s\t%s\n", argps->options[optpos].key,
			(argps->options[optpos].arg) ?
			argps->options[optpos].arg : "",
			argps->options[optpos].doc);
#endif
	      optpos++;
	    }
	  printf ("\n%s\n", argp_post_v (argps->doc));
	  printf ("Report bugs to %s.\n", argp_program_bug_address);
	  exit (0);
	  break;
	case 'V':
	  if (argp_program_version_hook == NULL)
	    printf ("%s\n", argp_program_version);
	  else
	    argp_program_version_hook (stderr, state);
	  exit (0);
	  break;
	default:
	  argps->parser (c, optarg, state);
	  break;
	}
    }
  for (; optind < argc; optind++)
    argps->parser (ARGP_KEY_ARG, argv[optind], state);
  argps->parser (ARGP_KEY_END, NULL, state);

  free (state);
  free (options);
#if HAVE_GETOPT_LONG
  free (longopts);
#endif
  return 0;
}

error_t
argp_error (const struct argp_state *state, char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "%s: ", state->argv0);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\nTry `%s -?' for more information.\n",
	   state->argv0);
  exit (1);

  return 1;
}
#endif

#if !defined(HAVE_GETSUBOPT) || defined(HAVE_BROKEN_GETSUBOPT)
int
bhc_getsubopt (char **optionp, char *const *tokens,
	       char **valuep)
{
  char *endp, *vstart;
  int cnt;

  if (**optionp == '\0')
    return -1;

  /* Find end of next token.  */
  endp = strchr (*optionp, ',');
  if (!endp)
    endp = *optionp + strlen (*optionp);

  /* Find start of value.  */
  vstart = memchr (*optionp, '=', endp - *optionp);

  if (vstart == NULL)
    vstart = endp;

  /* Try to match the characters between *OPTIONP and VSTART against
     one of the TOKENS.  */
  for (cnt = 0; tokens[cnt] != NULL; ++cnt)
    if (memcmp (*optionp, tokens[cnt], vstart - *optionp) == 0
	&& tokens[cnt][vstart - *optionp] == '\0')
      {
	/* We found the current option in TOKENS.  */
	*valuep = vstart != endp ? vstart + 1 : NULL;

	if (*endp != '\0')
	  *endp++ = '\0';
	*optionp = endp;

	return cnt;
      }

  /* The current suboption does not match any option.  */
  *valuep = *optionp;

  if (*endp != '\0')
    *endp++ = '\0';
  *optionp = endp;

  return -1;
}
#else
#if HAVE_SUBOPTARG
extern char *suboptarg;
#else
#endif
int
bhc_getsubopt (char **optionp, char *const *tokens,
	       char **valuep)
{
  int i = getsubopt (optionp, tokens, valuep);
#if HAVE_SUBOPTARG
  if (!*valuep)
    *valuep = bhc_strdup ((suboptarg) ? suboptarg : "");
#else
  if (!*valuep)
    *valuep = bhc_strdup ("");
#endif
  return i;
}
#endif

#ifndef HAVE_SCANDIR
int
scandir (const char *dir, struct dirent ***namelist,
	 int (*sd_select)(const struct dirent *),
	 int (*sd_compar)(const struct dirent **,
			  const struct dirent **))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d = opendir (dir)) == NULL)
    return -1;

  *namelist=NULL;
  while ((entry = readdir (d)) != NULL)
    {
      if (sd_select == NULL || (sd_select != NULL && (*sd_select) (entry)))
	{
	  *namelist = (struct dirent **)bhc_realloc
	    ((void *) (*namelist),
	     (size_t)((i + 1) * sizeof (struct dirent *)));
	  if (*namelist == NULL)
	    return -1;

	  entrysize = sizeof (struct dirent) -
	    sizeof (entry->d_name) + strlen (entry->d_name) + 1;
	  (*namelist)[i] = (struct dirent *)bhc_malloc (entrysize);
	  if ((*namelist)[i] == NULL)
	    return -1;
	  memcpy ((*namelist)[i], entry, entrysize);
	  i++;
	}
    }
  if (closedir (d))
    return -1;
  if (i == 0)
    return -1;
  if (sd_compar != NULL)
    qsort ((void *)(*namelist), (size_t) i, sizeof (struct dirent *),
	   sd_compar);

   return i;
}
#endif

#ifndef HAVE_ALPHASORT
int
alphasort (const struct dirent **a, const struct dirent **b)
{
  return (strcmp ((*a)->d_name, (*b)->d_name));
}
#endif

#ifndef HAVE_SETENV
int
setenv (const char *name, const char *value, int overwrite)
{
  char *tmp;
  int i;

  if (!name || !value)
    return 0;

  tmp = (char *)bhc_malloc (strlen (name) + strlen (value) + 2);
  strcpy (tmp, name);
  strcat (tmp, "=");
  strcat (tmp, value);

  i = putenv (tmp);

  free (tmp);

  return i;
}
#endif

#if !HAVE_WORKING_SETPROCTITLE
static char **bhc_setproc_argv = NULL;
static char *bhc_setproc_argv_end = NULL;
#endif

#if HAVE_WORKING_SETPROCTITLE
void
bhc_setproctitle_init (int argc, char **argv, char **envp)
{
}
#else
void
bhc_setproctitle_init (int argc, char **argv, char **envp)
{
  int i;

  bhc_setproc_argv = argv;

  for (i = 0; i < argc; i++)
    {
      if (i == 0 || bhc_setproc_argv_end + 1 == argv[i])
	bhc_setproc_argv_end = argv[i] + strlen (argv[i]);
    }
  for (i = 0; bhc_setproc_argv_end != NULL && envp != NULL &&
	 envp[i] != NULL; i++)
    {
      if (bhc_setproc_argv_end + 1 == envp[i])
	bhc_setproc_argv_end = envp[i] + strlen (envp[i]);
    }
}
#endif

#if HAVE_WORKING_SETPROCTITLE
void
bhc_setproctitle (const char *name)
{
  setproctitle (name);
}
#else
void
bhc_setproctitle (const char *name)
{
  int i;
  char *buf, *p;

  if (bhc_setproc_argv_end == NULL)
    return;

  buf = bhc_strdup (name);
  i = strlen (buf);

  if (i > bhc_setproc_argv_end - bhc_setproc_argv[0] - 2)
    {
      i = bhc_setproc_argv_end - bhc_setproc_argv[0] - 2;
      buf[i] = '\0';
    }

  strncpy (bhc_setproc_argv[0], buf, i + 1);
  p = &bhc_setproc_argv[0][i];
  while (p < bhc_setproc_argv_end)
    *p++ = 0x0;
  bhc_setproc_argv[1] = NULL;

  free (buf);
}
#endif

#ifndef HAVE_VSYSLOG
void
vsyslog (int priority, const char *format, va_list ap)
{
  char *msg;

  vasprintf (&msg, format, ap);
  syslog (priority, "%s", msg);
  free (msg);
}
#endif

#if !defined(HAVE___VA_ARGS__) && !defined(HAVE_VARIADIC_MACROS)
#if BHC_OPTION_ALL_LOGGING
#define _bhc_log_body(prio) \
{ \
    va_list ap; \
    \
    va_start (ap, fmt); \
    vsyslog (prio, fmt, ap); \
    va_end (ap); \
}

void bhc_error (char *fmt, ...) _bhc_log_body (LOG_NOTICE);
void bhc_log (char *fmt, ...) _bhc_log_body (LOG_WARNING);
#  if BHC_OPTION_DEBUG
   void bhc_debug (char *fmt, ...) _bhc_log_body (LOG_DEBUG);
#  else
   void bhc_debug (char *fmt, ...) {};
#  endif
#undef _bhc_log_body
#else
void bhc_error (char *fmt, ...) {}
void bhc_log (char *fmt, ...) {}
void bhc_debug (char *fmt, ...) {}
#endif
#endif

#endif /* !__DOXYGEN__ */

/* arch-tag: c8eb0ba7-516c-4592-96a4-d6bb25ffc0a0 */
