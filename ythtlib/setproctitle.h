#ifdef LINUX
#ifndef HAVE_SETPROCTITLE
void initproctitle(int argc, char **argv);
void setproctitle(const char *fmt, ...);
#endif
#endif
