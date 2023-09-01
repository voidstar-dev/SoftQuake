// A basic attempt at keeping versions syncronised with patch notes
// See patchtool.sh


#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>


#include "softquake_version.h"

void sep()
{
	printf("========================\n");
}

void patch_heading(const char *s, ...)
{
	printf("*** ");
	va_list vl;
	va_start(vl, s);
	vprintf(s, vl);
	va_end(vl);
}

const char *today()
{
	static char buf[512];
	struct tm *now = 0;
	time_t t_now = time(0);
	now = localtime(&t_now);
	assert(now);
	strftime(buf, sizeof(buf), "%Y-%m-%d", now);
	return buf;
}

void shellcmd(const char *s)
{
	int rc = system(s);
	if(rc != 0)
	{
		fprintf(stderr, "Could not execute shell command: %s\n", s);
		exit(rc);
	}

}

int main()
{
	sep();
	patch_heading("%.2f\n", SOFTQUAKE_VERSION);
	patch_heading("%s\n", today());
	sep();

	fflush(stdout);

	shellcmd("cat lastpatch.txt");

	
	return 0;
}
