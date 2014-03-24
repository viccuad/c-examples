#include <getopt.h>
#include <stdio.h>
#include <utmp.h>

#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <err.h>

typedef int bool;
enum {false, true};

#define NOMBRE_PROGRAMA "my_who"

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 64
#endif

#define MAX(a,b)	((a)>(b) ? (a) : (b))

#define DEV_DIR 	"/dev/"
#define DEV_DIR_LEN (sizeof (DEV_DIR) - 1)

#define UT_TYPE(U)	U->ut_type
#define UT_TYPE_EQ(U,TIPO)	(UT_TYPE(U) == TIPO)

#define UT_USER(U)	U->ut_user
#define UT_ID(U)	U->ut_id
#define UT_LINE(U)	U->ut_line
#define UT_HOST(U)	U->ut_host
#define UT_PID(U)	U->ut_pid
#define UT_TIME(U)	U->ut_time

#define UT_EXIT_E_TERMINATION(U)	U->ut_exit.e_termination
#define UT_EXIT_E_EXIT(U)		U->ut_exit.e_exit


#define IDLESTR_LEN 6

