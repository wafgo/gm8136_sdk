/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include "libburn/libburn.h"
#include "libburn/toc.h"
#include "libburn/mmc.h"

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

static struct burn_drive_info *drives;
static unsigned int n_drives;
int NEXT;

static void catch_int ()
{
	NEXT = 1;
}

static void poll_drive(int d)
{
	fprintf(stderr, "polling disc in %s - %s:\n",
		drives[d].vendor, drives[d].product);

	if (!burn_drive_grab(drives[d].drive, 1)) {
		fprintf(stderr, "Unable to open the drive!\n");
		return;
	}

	while (burn_drive_get_status(drives[d].drive, NULL))
		usleep(1000);

	while (burn_disc_get_status(drives[d].drive) == BURN_DISC_UNREADY)
		usleep(1000);
	
	while (NEXT == 0) {
		sleep(2);
		mmc_get_event(drives[d].drive);
	}
	burn_drive_release(drives[d].drive, 0);
}

int main()
{
    	int i;
	struct sigaction newact;
	struct sigaction oldact;
	fprintf(stderr, "Initializing library...");
	if (burn_initialize())
		fprintf(stderr, "Success\n");
	else {
		printf("Failed\n");
		return 1;
	}

	fprintf(stderr, "Scanning for devices...");
	while (!burn_drive_scan(&drives, &n_drives)) ;
	fprintf(stderr, "Done\n");
	if (!drives) {
	    	printf("No burner found\n");
		return 1;
	} 

	newact.sa_handler = catch_int;
	sigaction(SIGINT, &newact, &oldact);
	for (i = 0; i < (int) n_drives; i++) {
	    NEXT=0;
	    poll_drive(i);
	}
	sigaction(SIGINT, &oldact, NULL);
	burn_drive_info_free(drives);
	burn_finish();
	return 0;
}
