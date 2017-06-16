#include "thread_util.h"

void nameThread(std::thread &thread, const std::string &name)
{
#ifdef _GNU_SOURCE
	pthread_setname_np(thread.native_handle(), name.c_str());
#endif
}

void nameThisThread(const std::string &name)
{
#ifdef _GNU_SOURCE
	pthread_setname_np(pthread_self(), name.c_str());
#endif
}
