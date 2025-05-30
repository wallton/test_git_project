#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h> /* setitimer */
#include <signal.h>
#include <unistd.h> /* sleep */
#include "wtp_timerlib.h"


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
void my_alarm_handler(int a)
{
    customPrintf (__func__, __LINE__, "handler!\r\n");
}
void simple_test(void)
{
    struct itimerval t;

    t.it_interval.tv_usec = 0;
    t.it_interval.tv_sec = 7;
    t.it_value.tv_usec = 0;
    t.it_value.tv_sec = 4;

    customPrintf (__func__, __LINE__, "Start timer ~~~\n");

    if( setitimer( ITIMER_REAL, &t, NULL) < 0 )
    {
        printf ("setitimer failed!\r\n");
        return ;
    }

    signal( SIGALRM, my_alarm_handler );

    while(1)
    {
        sleep(2);
    }
}
/*** --------------------------------------------------***/

void wtpThreadSetSignals(int how, int num, ...)
{
	sigset_t mask;
	va_list args;

	sigemptyset(&mask);

	va_start(args, num);

	for(; num > 0; num--) {
		sigaddset(&mask, va_arg(args, int));
	}

	pthread_sigmask(how, &mask, NULL);

	va_end(args);
}

void wtp_ATimerExpiredHandler(void *arg)
{
	(void) arg;
    int timerId = 0;

    customPrintf (__func__, __LINE__, "A-handler!\r\n");

    timerId = timer_add( 15, 0, wtp_ATimerExpiredHandler, NULL);
    if (timerId == -1) {
        printf("timer_add failed.\n");
        exit(1);
    }
    customPrintf (__func__, __LINE__, "Re-Start A-timer (%d) ~~~\n", timerId);
}

void wtp_BTimerExpiredHandler(void *arg)
{
	(void) arg;

    customPrintf (__func__, __LINE__, "B-handler!\r\n");
}

void test_timerlib(void)
{
    int timerId_A = 0;
    int timerId_B = 0;
    int countdown = 60;

	wtpThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	if (timer_init() == 0) {
		printf("timer_init failed.\n");
		exit(1);
	}

    // int timer_add(long sec, long usec, void(*hndlr)(void *), void *hndlr_arg);

    timerId_A = timer_add( 15,
					       0,
					       wtp_ATimerExpiredHandler,
					       NULL);
    if (timerId_A == -1) {
        printf("timer_add failed.\n");
        exit(1);
    }
    customPrintf (__func__, __LINE__, "Start A-timer (%d) ~~~\n", timerId_A);

    timerId_B = timer_add( 20,
					       0,
					       wtp_BTimerExpiredHandler,
					       NULL);
	if (timerId_B == -1) {
        printf("timer_add failed.\n");
        exit(1);
    }
    customPrintf (__func__, __LINE__, "Start B-timer (%d) ~~~\n", timerId_B);


    /* WAIT 2*(countdown) seconds */
    while(countdown > 0)
    {
        sleep(2); countdown --;
    }

    // void timer_rem(int /*id*/, void(* /*free_arg*/)(void *));

	timer_rem(timerId_A, NULL);
	timer_rem(timerId_B, NULL);
	timer_destroy();

}

/*** --------------------------------------------------***/

int main(int argc, char *argv[]) 
{
    test_timerlib();
	return 0;
}
