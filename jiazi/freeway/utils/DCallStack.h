#ifndef _DCALLSTACK_H
#define _DCALLSTACK_H
typedef void Sigfunc(int);
typedef void CrashWills( void );
Sigfunc *Signal(int signo, Sigfunc *func);
void RegDebugSigHandle( void );
void OutputCallStack( void );
extern CrashWills* CrashHandler;
#endif//_DCALLSTACK_H
