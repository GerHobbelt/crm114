dnl **
dnl *	extern char *environ checking
dnl **

dnl derived from:
dnl
dnl   http://svn.osc.edu/browse/mpiexec/trunk/configure.in?view=markup&rev=187

AC_DEFUN([AX_ENVIRON], [
	AC_MSG_CHECKING([for environment access])
	AC_TRY_COMPILE([#include <unistd.h>],
	[char **cp = __environ], has=yes, has=no)
	if test $has = yes ; then
		AC_DEFINE(HAVE___ENVIRON, 1, [Define if you have the '__environ' global environment variable])
		AC_MSG_RESULT([__environ])
	else
		AC_TRY_COMPILE([extern char **environ;],
		[char **cp = environ], has=yes, has=no)
		if test $has = yes ; then
			AC_DEFINE(HAVE_ENVIRON, 1, [Define if you have the 'environ' global environment variable])
			AC_MSG_RESULT([environ])
		else
			AC_MSG_RESULT()
			AC_MSG_ERROR([No known variable "environ" found.])
		fi
	fi
	])

