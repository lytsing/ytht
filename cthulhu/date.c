#include <time.h>
#include "misc.h"
#ifndef DEBUG
# include "string.h"
#endif

int months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int lmonths[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const char *smonths[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

int
#ifdef DEBUG
__date (s, t)
	char    *s;
#else
__date (string, t)
	string_t *string;
#endif
	time_t   t;
{
	int year = (t / ((365*24*60*60) + 21600));
	int month, x, *xmonths;
	unsigned int hours, mins, secs, day, wday;
#ifndef DEBUG
	char *s;

	CHECK_BUF (string, 32);

	s = string->str + string->off;
	string->off += 31;
#endif
	xmonths = (((year+2)%4)?months:lmonths);

	wday = (4+(t/86400))%7;
	if (year) t %= (year * 365*24*60*60 + ((year-2)/4*(24*60*60)));

	x = t / (24*60*60);

	for (month=0;;month++) if ((x -= xmonths[month]) <= 0) break;
	day = xmonths[month] + x;
	x = t % (24*60*60);
	hours = x / (60*60);
	x %= (60*60);
	mins  = x / 60;
	secs  = x % 60;

	s[0] = days[wday][0];
	s[1] = days[wday][1];
	s[2] = days[wday][2];
	s[3] = ',';
	s[4] = ' ';
	if (!day) day = 1;
	s[5] = day/10+'0';
	s[6] = day%10+'0';
	s[7] = ' ';
	s[8] = smonths[month][0];
	s[9] = smonths[month][1];
	s[10] = smonths[month][2];
	s[11] = ' ';
	if (year >= 30) { s[12] = '2'; year-=30; s[13] = year/100+'0'; }
	else { year+=70; s[12] = '1'; s[13] = '9'; }
	year %= 100;
	s[14] = year/10+'0';
	s[15] = year%10+'0';
	s[16] = ' ';
	if (hours > 23) hours = 0;
	s[17] = hours/10+'0';
	s[18] = hours%10+'0';
	s[19] = ':';
	if (mins > 59) mins = 0;
	s[20] = mins/10+'0';
	s[21] = mins%10+'0';
	s[22] = ':';
	if (secs > 59) secs = 0;
	s[23] = secs/10+'0';
	s[24] = secs%10+'0';
	s[25] = ' ';
	s[26] = 'G';
	s[27] = 'M';
	s[28] = 'T';
	s[29] = '\r';
	s[30] = '\n';
	return (31);
}
