dnl Macros to check the presence of generic (non-typed) symbols.
dnl Copyright (c) 2006-2008 Diego Pettenò <flameeyes@gmail.com>
dnl Copyright (c) 2006-2008 xine project
dnl Copyright (c) 2012 Lucas De Marchi <lucas.de.marchi@gmail.com>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
dnl 02110-1301, USA.
dnl
dnl As a special exception, the copyright owners of the
dnl macro gives unlimited permission to copy, distribute and modify the
dnl configure scripts that are the output of Autoconf when processing the
dnl Macro. You need not follow the terms of the GNU General Public
dnl License when using or distributing such scripts, even though portions
dnl of the text of the Macro appear in them. The GNU General Public
dnl License (GPL) does govern all other use of the material that
dnl constitutes the Autoconf Macro.
dnl
dnl This special exception to the GPL applies to versions of the
dnl Autoconf Macro released by this project. When you make and
dnl distribute a modified version of the Autoconf Macro, you may extend
dnl this special exception to the GPL to apply to your modified version as
dnl well.

dnl Check if FLAG in ENV-VAR is supported by compiler and append it
dnl to WHERE-TO-APPEND variable
dnl CC_CHECK_FLAG_APPEND([WHERE-TO-APPEND], [ENV-VAR], [FLAG])

AC_DEFUN([CC_CHECK_FLAG_APPEND], [
  AC_CACHE_CHECK([if $CC supports flag $3 in envvar $2],
                 AS_TR_SH([cc_cv_$2_$3]),
		 [eval "AS_TR_SH([cc_save_$2])='${$2}'"
		  eval "AS_TR_SH([$2])='$3'"
		  AC_COMPILE_IFELSE([AC_LANG_SOURCE([int a = 0; int main(void) { return a; } ])],
                                    [eval "AS_TR_SH([cc_cv_$2_$3])='yes'"],
                                    [eval "AS_TR_SH([cc_cv_$2_$3])='no'"])
		  eval "AS_TR_SH([$2])='$cc_save_$2'"])

  AS_IF([eval test x$]AS_TR_SH([cc_cv_$2_$3])[ = xyes],
        [eval "$1='${$1} $3'"])
])

dnl CC_CHECK_FLAGS_APPEND([WHERE-TO-APPEND], [ENV-VAR], [FLAG1 FLAG2])
AC_DEFUN([CC_CHECK_FLAGS_APPEND], [
  for flag in $3; do
    CC_CHECK_FLAG_APPEND($1, $2, $flag)
  done
])

