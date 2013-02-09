/******************************************************************************
 *  (C)opyright 2007 KWICK! Community GmbH, All Rights Reserved
 *  (C)opyright 2007 Andre Pascha <shoki@bsdler.de>, All Rights Reserved
 *
 *       Title: Linux watchdog daemon
 *      Author: apa
 *    $RCSfile$
 *   $Revision$
 *       $Date$
 *      $State$
 *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <syslog.h>


/* defines */
#define PROGNAME		"wauwau"
#define VERSION			"1.4"
#define AUTHOR			"Andre Pascha"
#define WATCHDOG_DEVICE		"/dev/watchdog"
#define WATCHDOG_TIMEOUT	12
#define KEEPALIVE_DIVISOR	4
#define PIDFILE			"/var/run/wauwau.pid"

/* prototypes */
static void writepid(char *path);
void sighandler (int signo);
int main(int argc, const char *argv[]);

/* global vars */
int fd=0;

void sighandler (int signo)
{
    if (fd) {
	/* Be careful.
	 *  * If the kernel was compiled with the "Disable watchdog
	 *  shutdown on close" 
	 *   * support, then the next write & close operations will not
	 *   disable the watchdog
	 *    */
	syslog(LOG_INFO, "Disabled watchdog");
	write(fd,"V",1);
	close(fd);
    }
    unlink(PIDFILE);
    closelog();
    exit(0);
}

static void writepid(char *path)
{
    pid_t self = 0;
    FILE *pidfile = NULL;

    self = getpid();
    pidfile = fopen(path, "w");
    if (pidfile) {
	fprintf(pidfile, "%ld\n", (long)self);
	fclose(pidfile);
    }
    else {
	perror("Warning: couldn't open pidfile");
    }
}

int main(int argc, const char *argv[])
{
    size_t timeout = WATCHDOG_TIMEOUT;	/*default timeout*/
    size_t keepalive_time;
    struct sigaction new, old;

    new.sa_handler = sighandler;
    sigemptyset (&new.sa_mask);
    new.sa_flags = 0;

    if (sigaction (SIGINT, &new, &old) < 0) {
	printf ("Error setting SIGINT handler!\n");
	exit(0);
    }

    fd=open(WATCHDOG_DEVICE, O_WRONLY);
    if(fd==-1)
    {
	perror("failed to open watchdog device");
	exit(1);
    }

    daemon(0, 0);
    writepid(PIDFILE);

    openlog(PROGNAME, LOG_PID, LOG_USER);
    syslog(LOG_INFO, "Starting %s %s", PROGNAME ,VERSION);

    if (ioctl(fd,WDIOC_SETTIMEOUT, &timeout) < 0)
	perror("failed to set timeout");
    keepalive_time = timeout / KEEPALIVE_DIVISOR;
    syslog(LOG_INFO, "Running watchdog with %ds timeout and %ds keepalive", timeout ,keepalive_time);

    while(1)
    {
	ioctl(fd, WDIOC_KEEPALIVE, 0);
	sleep(keepalive_time); 
    }
}
