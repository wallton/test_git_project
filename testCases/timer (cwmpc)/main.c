#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <unistd.h> /* sleep */
#include "event.h"


/*** --------------------------------------------------***/
/*** Sample

customPrintf (__func__, __LINE__, "%d %f %s\n", i, f, str);
customPrintf (__func__, __LINE__, "\n");
*/
int customPrintf (const char *pFun, int i4Line, char *format, ...)
{
    va_list aptr;
    int     ret = 0;
    static int max_dbg = 50;
    char    bufferA[128] = {0};
    char    bufferB[256] = {0};
    time_t  curtime;
    char   *t = NULL;


    if(max_dbg)
    {
        va_start(aptr, format);
        ret = vsnprintf(bufferA, sizeof(bufferA), format, aptr);
        va_end(aptr);

        time(&curtime);
        t = ctime(&curtime);
        /* remove the newline before printing */
        if ('\n' == t[strlen(t)-1]) t[strlen(t)-1] = '\0';

        if (pFun && t)
        {
            if (ret < 0)
                snprintf (bufferB, sizeof(bufferB), "[%s][%s# %d] "  , t, pFun, i4Line);
            else
                snprintf (bufferB, sizeof(bufferB), "[%s][%s# %d] %s", t, pFun, i4Line, bufferA);

            /* newline should be given by caller */

            printf("%s", bufferB);
            /* fprintf (stderr, "%s", bufferB);
            */
        }

        max_dbg --;
    }

   return ret;
}

/*** --------------------------------------------------***/
void timerFired2( void *handle)
{
	int 	tnum = *(int *)handle;
    
	fprintf(stdout, "Timer2 %d expired\n", tnum);
}

void timerFired( void *handle )
{
	int 	tnum = *(int *)handle;
	int		ticks;
    
	fprintf(stdout, "Timer %d expired\n", tnum);
	if ( (ticks = rand()%50)< 30)
    {
        /* restart some of them */
		fprintf(stdout, "ReSet Timer %d @ %d\n", tnum, ticks);

		if (! setTimer( timerFired2, (void *)&tnum, ticks*1000 ))
			fprintf(stdout, "settimer %d failed\n", tnum);

	}
}

/*** --------------------------------------------------***/

void timertest(void)
{
	int		i;
	int		tt[10];
	int		ticks;
	int		any;

	srand(1);
	initGSLib();

	for(i=1; i<=15; ++i)
    {
		ticks = rand()%50;
		fprintf(stdout, "Set Timer %d @ %d\n", i, ticks);
		if (! setTimer( timerFired, (void *)&i, ticks*1000	))
			fprintf(stdout, "settimer %d failed\n", i);
	}

	eventLoop();
	any = 0;

	for (i=1; i<10; ++i)
    {
		any |= checkTimer(timerFired, (void *)&i);
	}

	if (any)
		fprintf(stdout, "Timers still running\n");
	else
		fprintf(stdout, "Timers expired\n");

}

/*** --------------------------------------------------***/

int main(int argc, char *argv[]) 
{
    timertest();
	return 0;
}
