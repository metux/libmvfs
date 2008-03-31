/* Utilities for VFS modules.

   Copyright (C) 1988, 1992 Free Software Foundation
   Copyright (C) 1995, 1996 Miguel de Icaza
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <sys/stat.h>

/* Parsing code is used by ftpfs, fish and extfs */
#define MAXCOLS		30

static char *columns[MAXCOLS];	/* Points to the string in column n */
static int column_ptr[MAXCOLS];	/* Index from 0 to the starting positions of the columns */

int
mvfs_split_text (char *p)
{
    char *original = p;
    int numcols;

    memset (columns, 0, sizeof (columns));

    for (numcols = 0; *p && numcols < MAXCOLS; numcols++) {
	while (*p == ' ' || *p == '\r' || *p == '\n') {
	    *p = 0;
	    p++;
	}
	columns[numcols] = p;
	column_ptr[numcols] = p - original;
	while (*p && *p != ' ' && *p != '\r' && *p != '\n')
	    p++;
    }
    return numcols;
}

static inline int
is_num (int idx)
{
    char *column = columns[idx];

    if (!column || column[0] < '0' || column[0] > '9')
	return 0;

    return 1;
}

static inline int
is_week (const char *str, struct tm *tim)
{
    static const char *week = "SunMonTueWedThuFriSat";
    const char *pos;

    if (!str)
	return 0;

    if ((pos = strstr (week, str)) != NULL) {
	if (tim != NULL)
	    tim->tm_wday = (pos - week) / 3;
	return 1;
    }
    return 0;
}

static inline int
is_month (const char *str, struct tm *tim)
{
    static const char *month = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *pos;

    if (!str)
	return 0;

    if ((pos = strstr (month, str)) != NULL) {
	if (tim != NULL)
	    tim->tm_mon = (pos - month) / 3;
	return 1;
    }
    return 0;
}

/* Return 1 for MM-DD-YY and MM-DD-YYYY */
static int
is_dos_date (const char *str)
{
    int len;

    if (!str)
	return 0;

    len = strlen (str);
    if (len != 8 && len != 10)
	return 0;

    if (str[2] != str[5])
	return 0;

    if (!strchr ("\\-/", (int) str[2]))
	return 0;

    return 1;
}

/*
 * Check for possible locale's abbreviated month name (Jan..Dec).
 * Any 3 bytes long string without digit, control and punctuation characters.
 * isalpha() is locale specific, so it cannot be used if current
 * locale is "C" and ftp server use Cyrillic.
 * NB: It is assumed there are no whitespaces in month.
 */
static int
is_localized_month (const unsigned char *month)
{
    int i = 0;

    if (!month)
	return 0;

    while ((i < 3) && *month && !isdigit (*month) && !iscntrl (*month)
	   && !ispunct (*month)) {
	i++;
	month++;
    }
    return ((i == 3) && (*month == 0));
}

static int
is_time (const char *str, struct tm *tim)
{
    const char *p, *p2;

    if (!str)
	return 0;

    if ((p = strchr (str, ':')) && (p2 = strrchr (str, ':'))) {
	if (p != p2) {
	    if (sscanf
		(str, "%2d:%2d:%2d", &tim->tm_hour, &tim->tm_min,
		 &tim->tm_sec) != 3)
		return 0;
	} else {
	    if (sscanf (str, "%2d:%2d", &tim->tm_hour, &tim->tm_min) != 2)
		return 0;
	}
    } else
	return 0;

    return 1;
}

static int
is_year (char *str, struct tm *tim)
{
    long year;

    if (!str)
	return 0;

    if (strchr (str, ':'))
	return 0;

    if (strlen (str) != 4)
	return 0;

    if (sscanf (str, "%ld", &year) != 1)
	return 0;

    if (year < 1900 || year > 3000)
	return 0;

    tim->tm_year = (int) (year - 1900);

    return 1;
}

/* This function parses from idx in the columns[] array */
int
mvfs_decode_filedate (int idx, time_t *t)
{
    char *p;
    struct tm tim;
    int d[3];
    int got_year = 0;
    int l10n = 0;		/* Locale's abbreviated month name */
    time_t current_time;
    struct tm *local_time;

    /* Let's setup default time values */
    current_time = time (NULL);
    local_time = localtime (&current_time);
    tim.tm_mday = local_time->tm_mday;
    tim.tm_mon = local_time->tm_mon;
    tim.tm_year = local_time->tm_year;

    tim.tm_hour = 0;
    tim.tm_min = 0;
    tim.tm_sec = 0;
    tim.tm_isdst = -1;		/* Let mktime() try to guess correct dst offset */

    p = columns[idx++];

    /* We eat weekday name in case of extfs */
    if (is_week (p, &tim))
	p = columns[idx++];

    /* Month name */
    if (is_month (p, &tim)) {
	/* And we expect, it followed by day number */
	if (is_num (idx))
	    tim.tm_mday = (int) atol (columns[idx++]);
	else
	    return 0;		/* No day */

    } else {
	/* We usually expect:
	   Mon DD hh:mm
	   Mon DD  YYYY
	   But in case of extfs we allow these date formats:
	   Mon DD YYYY hh:mm
	   Mon DD hh:mm YYYY
	   Wek Mon DD hh:mm:ss YYYY
	   MM-DD-YY hh:mm
	   where Mon is Jan-Dec, DD, MM, YY two digit day, month, year,
	   YYYY four digit year, hh, mm, ss two digit hour, minute or second. */

	/* Special case with MM-DD-YY or MM-DD-YYYY */
	if (is_dos_date (p)) {
	    p[2] = p[5] = '-';

	    if (sscanf (p, "%2d-%2d-%d", &d[0], &d[1], &d[2]) == 3) {
		/* Months are zero based */
		if (d[0] > 0)
		    d[0]--;

		if (d[2] > 1900) {
		    d[2] -= 1900;
		} else {
		    /* Y2K madness */
		    if (d[2] < 70)
			d[2] += 100;
		}

		tim.tm_mon = d[0];
		tim.tm_mday = d[1];
		tim.tm_year = d[2];
		got_year = 1;
	    } else
		return 0;	/* sscanf failed */
	} else {
	    /* Locale's abbreviated month name followed by day number */
	    if (is_localized_month (p) && (is_num (idx++)))
		l10n = 1;
	    else
		return 0;	/* unsupported format */
	}
    }

    /* Here we expect to find time and/or year */

    if (is_num (idx)) {
	if (is_time (columns[idx], &tim)
	    || (got_year = is_year (columns[idx], &tim))) {
	    idx++;

	    /* This is a special case for ctime() or Mon DD YYYY hh:mm */
	    if (is_num (idx) && (columns[idx + 1][0])) {
		if (got_year) {
		    if (is_time (columns[idx], &tim))
			idx++;	/* time also */
		} else {
		    if ((got_year = is_year (columns[idx], &tim)))
			idx++;	/* year also */
		}
	    }
	}			/* only time or date */
    } else
	return 0;		/* Nor time or date */

    /*
     * If the date is less than 6 months in the past, it is shown without year
     * other dates in the past or future are shown with year but without time
     * This does not check for years before 1900 ... I don't know, how
     * to represent them at all
     */
    if (!got_year && local_time->tm_mon < 6
	&& local_time->tm_mon < tim.tm_mon
	&& tim.tm_mon - local_time->tm_mon >= 6)

	tim.tm_year--;

    if (l10n || (*t = mktime (&tim)) < 0)
	*t = 0;
    return idx;
}

int
mvfs_decode_filemode (const char *p)
{				/* converts rw-rw-rw- into 0666 */
    int res = 0;
    switch (*(p++)) {
    case 'r':
	res |= 0400;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'w':
	res |= 0200;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'x':
	res |= 0100;
	break;
    case 's':
	res |= 0100 | S_ISUID;
	break;
    case 'S':
	res |= S_ISUID;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'r':
	res |= 0040;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'w':
	res |= 0020;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'x':
	res |= 0010;
	break;
    case 's':
	res |= 0010 | S_ISGID;
	break;
    case 'l':			/* Solaris produces these */
    case 'S':
	res |= S_ISGID;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'r':
	res |= 0004;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'w':
	res |= 0002;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    switch (*(p++)) {
    case 'x':
	res |= 0001;
	break;
    case 't':
	res |= 0001 | S_ISVTX;
	break;
    case 'T':
	res |= S_ISVTX;
	break;
    case '-':
	break;
    default:
	return -1;
    }
    return res;
}

/*
 * FIXME: this is broken. Consider following entry:
 * -rwx------   1 root     root            1 Aug 31 10:04 2904 1234
 * where "2904 1234" is filename. Well, this code decodes it as year :-(.
 */

int mvfs_decode_filetype (char c)
{
    switch (c) 
    {
	case 'd':
	    return S_IFDIR;
	case 'b':
	    return S_IFBLK;
	case 'c':
	    return S_IFCHR;
	case 'l':
	    return S_IFLNK;
	case 's':			/* Socket */
#ifdef S_IFSOCK
	    return S_IFSOCK;
#else
	    /* If not supported, we fall through to IFIFO */
	    return S_IFIFO;
#endif
	case 'D':			/* Solaris door */
#ifdef S_IFDOOR
	    return S_IFDOOR;
#else
	    return S_IFIFO;
#endif
	case 'p':
	    return S_IFIFO;
	case 'n':			/* Special named files */
#ifdef S_IFNAM
	    return S_IFNAM;
#endif /* S_IFNAM */
	case 'm':			/* Don't know what these are :-) */
	case '-':
	case '?':
	    return S_IFREG;
	default:
	    return -1;
    }
}
