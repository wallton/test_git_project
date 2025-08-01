#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*** --------------------------------------------------***/

/*** --------------------------------------------------***/


volatile sig_atomic_t    quitflag;    /* set nonzero by signal handler */

static void sig_int(int signo)    /* one signal handler for SIGINT and SIGQUIT */
{
    // signal(SIGINT, sig_int);
    // signal(SIGQUIT, sig_int);
    if (signo == SIGINT)
        printf("\ninterrupt\n");
    else if (signo == SIGQUIT) {
        printf("\nquit\n");
        quitflag = 1;    /* set flag for main loop */
    }
}

int main(int argc, char *argv[])
{
    sigset_t    newmask, oldmask, zeromask;

    if(signal(SIGINT, sig_int) == SIG_ERR)
        perror("signal(SIGINT) error");
    if(signal(SIGQUIT, sig_int) == SIG_ERR)
        perror("signal(SIGQUIT) error");

    sigemptyset(&zeromask);
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGQUIT);

    /*
    * Block SIGQUIT and save current signal mask.
    */
    if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
        perror("SIG_BLOCK error");

    while(!quitflag)
    {
        sigsuspend(&zeromask);
    }

    /*
    * SIGQUIT has been caught and is now blocked; do whatever.
    */
    quitflag = 0;

    /*
    * Reset signal mask which unblocks SIGQUIT.
    */
    if(sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
        perror("SIG_SETMASK error");

    exit(0);
}

