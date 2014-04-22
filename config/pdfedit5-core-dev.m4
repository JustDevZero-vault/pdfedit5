# TODO doc - PDFEDIT_CORE_DEV_PATH
# SYNOPSIS
#
# 	AX_PDFEDIT_CORE_DEV
#
# DESCRIPTION
# 	Test for pdfedit-core-dev library from PDFedit package. This 
# 	macro searches for pdfedit-core-dev-config script or uses the
# 	given one (by --with-pdfedit-core-dev-config parameter) and 
# 	uses it to get proper compiler and linker flags which are exported
# 	as PDFEDIT_CORE_DEV_CPPFLAGS resp. PDFEDIT_CODE_DEV_LDFLAGS.
# 	You can use --with-pdfedit-core-dev-config=no to prevent from
# 	library detection completely.
# 	PDFEDIT_CORE_DEV_PATH environment variable is used if it is set
# 	for non-standard pdfedit library location (previously installed with
# 	non standard PREFIX or libdir)
#
# LICENSE
#	
# PDFedit5 - free program for PDF document manipulation.
# Copyright (C) 2014  PDFedit5: Daniel Ripoll
# Copyright (C) 2006-2009  PDFedit team: Michal Hocko,
#                                        Jozef Misutka,
#                                        Martin Petricek
#                   Former team members: Miroslav Jahoda
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program (in doc/LICENSE.GPL); if not, write to the 
# Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
# MA  02111-1307  USA
#
# Project is hosted on http://github.com/mephiston/pdfedit5
AC_DEFUN([AX_PDFEDIT5_CORE_DEV],
[
	AC_ARG_WITH([pdfedit5-core-dev-config],
		AS_HELP_STRING([--with-pdfedit5-core-dev-config@<:@=special-lib@:>@],
			[use the specific pdfedit5-core-dev-config script]
		),
		[
		if test "$withval" = "no"; then
			want_pdfedit5_core_dev="no"
		elif test "$withval" = "yes"; then
			want_pdfedit5_core_dev="yes"
			ax_pdfedit5_user_core_dev_config=""
		else
		{
			want_pdfedit5_core_dev="yes"
			if test -x $withval; then
				ax_pdfedit5_user_core_dev_config="$withval"
				AC_MSG_NOTICE(Using provided $ax_pdfedit5_user_core_dev_config configuration script for pdfedit5-core-dev)
			else
				AC_MSG_ERROR(--with-pdfedit5-core-dev-config expected executable file)
			fi
		}
		fi
		],
		[want_pdfedit5_core_dev="yes"]
	)

	if test "x$want_pdfedit5_core_dev" = "xyes"; 
	then
		AC_REQUIRE([AC_PROG_CC])
		export want_pdfedit5_core_dev

		CPPFLAGS_SAVED="$CPPFLAGS"
		LDFLAGS_SAVED="$LDFLAGS"
		config="pdfedit5-core-dev-config"
		ax_pdfedit5_core_dev_config="$ax_pdfedit5_user_core_dev_config"
		if test -z "$ax_pdfedit5_core_dev_config"
		then
			for i in  $PDFEDIT5_CORE_DEV_PATH/bin/$config $exec_prefix/$config /usr/bin/$config /usr/local/bin/$config
			do
				if test -x "$i"
				then
					ax_pdfedit5_core_dev_config="$i"
					break;
				fi
			done
		fi

		if test -z "$ax_pdfedit5_core_dev_config"
		then
			AC_MSG_ERROR($config not found)
		fi

		PDFEDIT5_CORE_DEV_CPPFLAGS="`$ax_pdfedit5_core_dev_config --cflags`"
		CPPFLAGS="$CPPFLAGS $PDFEDIT5_CORE_DEV_CPPFLAGS"
		export CPPFLAGS
		PDFEDIT5_CODE_DEV_LDFLAGS="`$ax_pdfedit5_core_dev_config --libs`"
		LDFLAGS="$LDFLAGS $PDFEDIT5_CODE_DEV_LDFLAGS"
		export LDFLAGS
		AC_CACHE_CHECK([whether pdfedit5-core-dev library is available],
			ax_cv_pdfedit5_core_dev,
			[AC_LANG_PUSH(C++)
				AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <kernel/pdfedit5-core-dev.h>]],
					[[pdfedit5_core_dev_init(); return 0;]]),
					ax_cv_pdfedit5_core_dev=yes, ax_cv_pdfedit5_core_dev=no)
			 AC_LANG_POP([C++])
			]
		)
		if test "$ax_cv_pdfedit5_core_dev" = "no"
		then
			AC_MSG_ERROR(Not able to link with pdfedit5-core-dev library)
		fi
		ax_pdfedit5_version="`$ax_pdfedit5_core_dev_config --version`"
		AC_MSG_NOTICE(pdfedit5-core-dev with $ax_pdfedit5_version found)
		CPPFLAGS="$CPPFLAGS_SAVED"
		LDFLAGS="$LDFLAGS_SAVED"
		AC_SUBST(PDFEDIT5_CORE_DEV_CPPFLAGS)
		AC_SUBST(PDFEDIT5_CODE_DEV_LDFLAGS)
	else
		AC_MSG_NOTICE(Not using pdfedit5-core-dev)
	fi
]
)
