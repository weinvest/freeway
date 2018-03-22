#ifndef _WIN32
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include "utils/DCallStack.h"
CrashWills* CrashHandler = nullptr;
Sigfunc *Signal(int signo, Sigfunc *func)
{
     struct sigaction    act, oact;
     act.sa_handler = func;
     sigemptyset(&act.sa_mask);
     act.sa_flags = 0;
     if (signo == SIGALRM) {
 #ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
 #endif
     } else {
 #ifdef  SA_RESTART
         act.sa_flags |= SA_RESTART;
 #endif
     }
     if (sigaction(signo, &act, &oact) < 0)
         return(SIG_ERR);
     return(oact.sa_handler);
}

void OutputCallStack( void )
{
    const int ARRAY_SIZE = 20;
    void *array[ARRAY_SIZE];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, ARRAY_SIZE);
    strings = (char **)backtrace_symbols (array, size);

    for (i = 0; i < size; i++)
    {
	fprintf(stderr, "%lu %s \n",i,strings[i]);
    }

    free (strings);
}

static void SigHandler(int signum)
{
    const int ARRAY_SIZE = 20;
    void *array[ARRAY_SIZE];
    size_t size;
    char **strings;
    size_t i;

    signal(signum, SIG_DFL);

    size = backtrace (array, ARRAY_SIZE);
    strings = (char **)backtrace_symbols (array, size);

    fprintf(stderr, "widebright received SIGSEGV! Stack trace:\n");
    for (i = 0; i < size; i++)
    {
	fprintf(stderr, "%lu %s \n",i,strings[i]);
    }

    free (strings);

    if(nullptr != CrashHandler)
    {
        CrashHandler();
    }
}

void RegDebugSigHandle( void )
{
    Signal(SIGABRT,SigHandler);
    Signal(SIGBUS,SigHandler);
    Signal(SIGFPE,SigHandler);
    Signal(SIGILL,SigHandler);
    Signal(SIGIOT,SigHandler);
    Signal(SIGPIPE,SigHandler);
    //Signal(SIGPOLL,SigHandler);
    Signal(SIGPROF,SigHandler);
    Signal(SIGQUIT,SigHandler);
    Signal(SIGSEGV,SigHandler);
    //Signal(SIGSTKFLT,SigHandler);
    Signal(SIGSYS,SigHandler);
    //Signal(SIGTERM,SigHandler);
    Signal(SIGTRAP,SigHandler);
    Signal(SIGTSTP,SigHandler);
    Signal(SIGTTIN,SigHandler);
    Signal(SIGTTOU,SigHandler);
    Signal(SIGUSR1,SigHandler);
    Signal(SIGUSR2,SigHandler);
    Signal(SIGVTALRM,SigHandler);
}
#else
void RegDebugSigHandle( void )
{}
#endif
