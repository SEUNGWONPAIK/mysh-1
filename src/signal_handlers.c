#include "signal_handlers.h"
#include <signal.h>

void catch_sigint(int signalNo)
{
  if(signalNo==SIGINT)
	signal(SIGINT,SIG_IGN);
}

void catch_sigtstp(int signalNo)
{
  if(signalNo==SIGTSTP)
	signal(SIGTSTP,SIG_IGN);
}
