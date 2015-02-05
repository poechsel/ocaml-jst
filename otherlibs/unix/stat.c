/***********************************************************************/
/*                                                                     */
/*                                OCaml                                */
/*                                                                     */
/*            Xavier Leroy, projet Cristal, INRIA Rocquencourt         */
/*                                                                     */
/*  Copyright 1996 Institut National de Recherche en Informatique et   */
/*  en Automatique.  All rights reserved.  This file is distributed    */
/*  under the terms of the GNU Library General Public License, with    */
/*  the special exception on linking described in file ../../LICENSE.  */
/*                                                                     */
/***********************************************************************/

#include <errno.h>
#include <mlvalues.h>
#include <memory.h>
#include <alloc.h>
#include <signals.h>
#include "unixsupport.h"
#include "cst2constr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#ifndef S_IFLNK
#define S_IFLNK 0
#endif
#ifndef S_IFIFO
#define S_IFIFO 0
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0
#endif
#ifndef S_IFBLK
#define S_IFBLK 0
#endif

#ifndef EOVERFLOW
#define EOVERFLOW ERANGE
#endif

static int file_kind_table[] = {
  S_IFREG, S_IFDIR, S_IFCHR, S_IFBLK, S_IFLNK, S_IFIFO, S_IFSOCK
};

static value stat_aux(int use_64, struct stat *buf)
{
  CAMLparam0();
  CAMLlocal5(atime, mtime, ctime, offset, v);

  atime = copy_double((double) buf->st_atime);
  mtime = copy_double((double) buf->st_mtime);
  ctime = copy_double((double) buf->st_ctime);
  offset = use_64 ? Val_file_offset(buf->st_size) : Val_int (buf->st_size);
  v = alloc_small(12, 0);
  Init_field(v, 0, Val_int (buf->st_dev));
  Init_field(v, 1, Val_int (buf->st_ino));
  Init_field(v, 2, cst_to_constr(buf->st_mode & S_IFMT, file_kind_table,
                                  sizeof(file_kind_table) / sizeof(int), 0));
  Init_field(v, 3, Val_int (buf->st_mode & 07777));
  Init_field(v, 4, Val_int (buf->st_nlink));
  Init_field(v, 5, Val_int (buf->st_uid));
  Init_field(v, 6, Val_int (buf->st_gid));
  Init_field(v, 7, Val_int (buf->st_rdev));
  Init_field(v, 8, offset);
  Init_field(v, 9, atime);
  Init_field(v, 10, mtime);
  Init_field(v, 11, ctime);
  CAMLreturn(v);
}

CAMLprim value unix_stat(value path)
{
  CAMLparam1(path);
  int ret;
  struct stat buf;
  char * p;
  p = caml_strdup(String_val(path));
  caml_enter_blocking_section();
  ret = stat(p, &buf);
  caml_leave_blocking_section();
  caml_stat_free(p);
  if (ret == -1) uerror("stat", path);
  if (buf.st_size > Max_long && (buf.st_mode & S_IFMT) == S_IFREG)
    unix_error(EOVERFLOW, "stat", path);
  CAMLreturn(stat_aux(0, &buf));
}

CAMLprim value unix_lstat(value path)
{
  CAMLparam1(path);
  int ret;
  struct stat buf;
  char * p;
  p = caml_strdup(String_val(path));
  caml_enter_blocking_section();
#ifdef HAS_SYMLINK
  ret = lstat(p, &buf);
#else
  ret = stat(p, &buf);
#endif
  caml_leave_blocking_section();
  caml_stat_free(p);
  if (ret == -1) uerror("lstat", path);
  if (buf.st_size > Max_long && (buf.st_mode & S_IFMT) == S_IFREG)
    unix_error(EOVERFLOW, "lstat", path);
  CAMLreturn(stat_aux(0, &buf));
}

CAMLprim value unix_fstat(value fd)
{
  int ret;
  struct stat buf;
  caml_enter_blocking_section();
  ret = fstat(Int_val(fd), &buf);
  caml_leave_blocking_section();
  if (ret == -1) uerror("fstat", Nothing);
  if (buf.st_size > Max_long && (buf.st_mode & S_IFMT) == S_IFREG)
    unix_error(EOVERFLOW, "fstat", Nothing);
  return stat_aux(0, &buf);
}

CAMLprim value unix_stat_64(value path)
{
  CAMLparam1(path);
  int ret;
  struct stat buf;
  char * p;
  p = caml_strdup(String_val(path));
  caml_enter_blocking_section();
  ret = stat(p, &buf);
  caml_leave_blocking_section();
  caml_stat_free(p);
  if (ret == -1) uerror("stat", path);
  CAMLreturn(stat_aux(1, &buf));
}

CAMLprim value unix_lstat_64(value path)
{
  CAMLparam1(path);
  int ret;
  struct stat buf;
  char * p;
  p = caml_strdup(String_val(path));
  caml_enter_blocking_section();
#ifdef HAS_SYMLINK
  ret = lstat(p, &buf);
#else
  ret = stat(p, &buf);
#endif
  caml_leave_blocking_section();
  caml_stat_free(p);
  if (ret == -1) uerror("lstat", path);
  CAMLreturn(stat_aux(1, &buf));
}

CAMLprim value unix_fstat_64(value fd)
{
  int ret;
  struct stat buf;
  caml_enter_blocking_section();
  ret = fstat(Int_val(fd), &buf);
  caml_leave_blocking_section();
  if (ret == -1) uerror("fstat", Nothing);
  return stat_aux(1, &buf);
}
