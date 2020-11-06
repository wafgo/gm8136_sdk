
/* test/libburner.c , API illustration of burning data or audio tracks to CD */
/* Copyright (C) 2005 - 2011 Thomas Schmitt <scdbackup@gmx.net> */
/* Provided under GPL, see also "License and copyright aspects" at file end */


/**                               Overview 
  
  libburner is a minimal demo application for the library libburn as provided
  on  http://libburnia-project.org . It can list the available devices, can
  blank a CD-RW or DVD-RW, can format DVD-RW and BD, can burn to CD-R,
  CD-RW, DVD-R, DVD+R, DVD+R/DL, DVD+RW, DVD-RW, DVD-RAM, BD-R, BD-RE. 
  Not supported yet: DVD-R/DL.

  It's main purpose, nevertheless, is to show you how to use libburn and also
  to serve the libburnia team as reference application. libburner.c does indeed
  define the standard way how above three gestures can be implemented and
  stay upward compatible for a good while.
  
  Before you can do anything, you have to initialize libburn by
     burn_initialize()
  and provide some signal and abort handling, e.g. by the builtin handler, by
     burn_set_signal_handling("libburner : ", NULL, 0x0) 
  as it is done in main() at the end of this file.
  Then you aquire a drive in an appropriate way conforming to the API. The twoi
  main approaches are shown here in application functions:
     libburner_aquire_by_adr()     demonstrates usage as of cdrecord traditions
     libburner_aquire_by_driveno()      demonstrates a scan-and-choose approach

  With that aquired drive you can blank a CD-RW or DVD-RW as shown in
     libburner_blank_disc()
  or you can format a DVD-RW to profile "Restricted Overwrite" (needed once)
  or an unused BD to default size with spare blocks
     libburner_format()
  With the aquired drive you can burn to CD, DVD, BD. See
     libburner_payload()

  These three functions switch temporarily to a non-fatal signal handler
  while they are waiting for the drive to become idle again:
     burn_set_signal_handling("libburner : ", NULL, 0x30)
  After the waiting loop ended, they check for eventual abort events by
     burn_is_aborting(0)
  The 0x30 handler will eventually execute
     burn_abort()
  but not wait for the drive to become idle and not call exit().
  This is needed because the worker threads might block as long as the signal
  handler has not returned. The 0x0 handler would wait for them to finish.
  Take this into respect when implementing own signal handlers.

  When everything is done, main() releases the drive and shuts down libburn:
     burn_drive_release();
     burn_finish()

  Applications must use 64 bit off_t. E.g. by defining
    #define _LARGEFILE_SOURCE
    #define _FILE_OFFSET_BITS 64
  or take special precautions to interface with the library by 64 bit integers
  where libburn/libburn.h prescribes off_t.
  This program gets fed with appropriate settings externally by libburn's
  autotools generated build system.
*/


/** See this for the decisive API specs . libburn.h is The Original */
/*  For using the installed header file :  #include <libburn/libburn.h> */
/*  This program insists in the own headerfile. */
#include "../libburn/libburn.h"

/* libburn works on Linux systems with kernel 2.4 or 2.6, FreeBSD, Solaris */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>


/** For simplicity i use global variables to represent the drives.
    Drives are systemwide global, so we do not give away much of good style.
*/

/** This list will hold the drives known to libburn. This might be all CD
    drives of the system and thus might impose severe impact on the system.
*/
static struct burn_drive_info *drive_list;

/** If you start a long lasting operation with drive_count > 1 then you are
    not friendly to the users of other drives on those systems. Beware. */
static unsigned int drive_count;

/** This variable indicates wether the drive is grabbed and must be
    finally released */
static int drive_is_grabbed = 0;

/** A number and a text describing the type of media in aquired drive */
static int current_profile= -1;
static char current_profile_name[80]= {""};


/* Some in-advance definitions to allow a more comprehensive ordering
   of the functions and their explanations in here */
int libburner_aquire_by_adr(char *drive_adr);
int libburner_aquire_by_driveno(int *drive_no);


/* ------------------------------- API gestures ---------------------------- */

/** You need to aquire a drive before burning. The API offers this as one
    compact call and alternatively as application controllable gestures of
    whitelisting, scanning for drives and finally grabbing one of them.

    If you have a persistent address of the drive, then the compact call is
    to prefer because it only touches one drive. On modern Linux kernels,
    there should be no fatal disturbance of ongoing burns of other libburn
    instances with any of our approaches. We use open(O_EXCL) by default.
    On /dev/hdX it should cooperate with growisofs and some cdrecord variants.
    On /dev/sgN versus /dev/scdM expect it not to respect other programs.
*/
int libburner_aquire_drive(char *drive_adr, int *driveno)
{
	int ret;

	if(drive_adr != NULL && drive_adr[0] != 0)
		ret = libburner_aquire_by_adr(drive_adr);
	else
		ret = libburner_aquire_by_driveno(driveno);
	if (ret <= 0 || *driveno <= 0)
		return ret;
	burn_disc_get_profile(drive_list[0].drive, &current_profile,
				 current_profile_name);
	if (current_profile_name[0])
		printf("Detected media type: %s\n", current_profile_name);
	return 1;
}


/** If the persistent drive address is known, then this approach is much
    more un-obtrusive to the systemwide livestock of drives. Only the
    given drive device will be opened during this procedure.
*/
int libburner_aquire_by_adr(char *drive_adr)
{
	int ret;
	char libburn_drive_adr[BURN_DRIVE_ADR_LEN];

	/* Some not-so-harmless drive addresses get blocked in this demo */
	if (strncmp(drive_adr, "stdio:/dev/fd/", 14) == 0 ||
		strcmp(drive_adr, "stdio:-") == 0) {
		fprintf(stderr, "Will not work with pseudo-drive '%s'\n",
			drive_adr);
		return 0;
	}

	/* This tries to resolve links or alternative device files */
	ret = burn_drive_convert_fs_adr(drive_adr, libburn_drive_adr);	
	if (ret<=0) {
		fprintf(stderr, "Address does not lead to a CD burner: '%s'\n",
				 drive_adr);
		return 0;
	}
	fprintf(stderr,"Aquiring drive '%s' ...\n", libburn_drive_adr);
	ret = burn_drive_scan_and_grab(&drive_list, libburn_drive_adr, 1);
	if (ret <= 0) {
		fprintf(stderr,"FAILURE with persistent drive address  '%s'\n",
			libburn_drive_adr);
	} else {
		fprintf(stderr,"Done\n");
		drive_is_grabbed = 1;
	}
	return ret;
}


/** This method demonstrates how to use libburn without knowing a persistent
    drive address in advance. It has to make sure that after assessing the list
    of available drives, all unwanted drives get closed again. As long as they
    are open, no other libburn instance can see them. This is an intended
    locking feature. The application is responsible for giving up the locks
    by either burn_drive_release() (only after burn_drive_grab() !),
    burn_drive_info_forget(), burn_drive_info_free(), or burn_finish().
    @param driveno the index number in libburn's drive list. This will get
                   set to 0 on success and will then be the drive index to
                   use in the further dourse of processing.
    @return 1 success , <= 0 failure
*/
int libburner_aquire_by_driveno(int *driveno)
{
	char adr[BURN_DRIVE_ADR_LEN];
	int ret, i;

	printf("Beginning to scan for devices ...\n");
	while (!burn_drive_scan(&drive_list, &drive_count))
		usleep(100002);
	if (drive_count <= 0 && *driveno >= 0) {
		printf("FAILED (no drives found)\n");
		return 0;
	}
	printf("Done\n");

	/*
	Interactive programs may choose the drive number at this moment.

	drive[0] to drive[drive_count-1] are struct burn_drive_info
	as defined in  libburn/libburn.h  . This structure is part of API
	and thus will strive for future compatibility on source level.
	Have a look at the info offered.
	Caution: do not take .location for drive address. Always use
		burn_drive_get_adr() or you might become incompatible
		in future.
	Note: bugs with struct burn_drive_info - if any - will not be
		easy to fix. Please report them but also strive for
		workarounds on application level.
	*/
	printf("\nOverview of accessible drives (%d found) :\n",
		drive_count);
	printf("-----------------------------------------------------------------------------\n");
	for (i = 0; i < (int) drive_count; i++) {
		if (burn_drive_get_adr(&(drive_list[i]), adr) <=0)
			strcpy(adr, "-get_adr_failed-");
		printf("%d  --drive '%s'  :  '%s'  '%s'\n",
			i,adr,drive_list[i].vendor,drive_list[i].product);
	}
	printf("-----------------------------------------------------------------------------\n\n");

	/*
	On multi-drive systems save yourself from sysadmins' revenge.

	Be aware that you hold reserved all available drives at this point.
	So either make your choice quick enough not to annoy other system
	users, or set free the drives for a while.

	The tested way of setting free all drives is to shutdown the library
	and to restart when the choice has been made. The list of selectable
	drives should also hold persistent drive addresses as obtained
	above by burn_drive_get_adr(). By such an address one may use
	burn_drive_scan_and_grab() to finally aquire exactly one drive.

	A not yet tested shortcut should be to call burn_drive_info_free()
	and to call either burn_drive_scan() or burn_drive_scan_and_grab()
	before accessing any drives again.

	In both cases you have to be aware that the desired drive might get
	aquired in the meantime by another user resp. libburn process.
	*/

	/* We already made our choice via command line. (default is 0)
	   So we just have to keep our desired drive and drop all others.
	   No other libburn instance will have a chance to steal our drive.
	 */
	if (*driveno < 0) {
		printf("Pseudo-drive \"-\" given : bus scanning done.\n");
		return 2; /* the program will end after this */
	}
	if ((int) drive_count <= *driveno) {
		fprintf(stderr,
			"Found only %d drives. Number %d not available.\n",
			drive_count, *driveno);
		return 0; /* the program will end after this */
	}

	/* Drop all drives which we do not want to use */
	for (i = 0; i < (int) drive_count; i++) {
		if (i == *driveno) /* the one drive we want to keep */
	continue;
		ret = burn_drive_info_forget(&(drive_list[i]),0);
		if (ret != 1)
			fprintf(stderr, "Cannot drop drive %d. Please report \"ret=%d\" to libburn-hackers@pykix.org\n",
				i, ret);
		else
			printf("Dropped unwanted drive %d\n",i);
	}
	/* Make the one we want ready for blanking or burning */
	ret= burn_drive_grab(drive_list[*driveno].drive, 1);
	if (ret != 1)
		return 0;
	drive_is_grabbed = 1;
	return 1;
}


/** Makes a previously used CD-RW or unformatted DVD-RW ready for thorough
    re-usal.

    To our knowledge it is hardly possible to abort an ongoing blank operation
    because after start it is entirely handled by the drive.
    So expect signal handling to wait the normal blanking timespan until it
    can allow the process to end. External kill -9 will not help the drive.
*/
int libburner_blank_disc(struct burn_drive *drive, int blank_fast)
{
	enum burn_disc_status disc_state;
	struct burn_progress p;
	double percent = 1.0;

	disc_state = burn_disc_get_status(drive);
	printf(
	    "Drive media status:  %d  (see  libburn/libburn.h  BURN_DISC_*)\n",
	    disc_state);
	if (current_profile == 0x13) {
		; /* formatted DVD-RW will get blanked to sequential state */
	} else if (disc_state == BURN_DISC_BLANK) {
		fprintf(stderr,
		  "IDLE: Blank media detected. Will leave it untouched\n");
		return 2;
	} else if (disc_state == BURN_DISC_FULL ||
		   disc_state == BURN_DISC_APPENDABLE) {
		; /* this is what libburner is willing to blank */
	} else if (disc_state == BURN_DISC_EMPTY) {
		fprintf(stderr,"FATAL: No media detected in drive\n");
		return 0;
	} else {
		fprintf(stderr,
			"FATAL: Unsuitable drive and media state\n");
		return 0;
	}
	if(!burn_disc_erasable(drive)) {
		fprintf(stderr,
			"FATAL : Media is not of erasable type\n");
		return 0;
	}
	/* Switch to asynchronous signal handling for the time of waiting */
	burn_set_signal_handling("libburner : ", NULL, 0x30);

	printf("Beginning to %s-blank media.\n", (blank_fast?"fast":"full"));
	burn_disc_erase(drive, blank_fast);

	sleep(1);
	while (burn_drive_get_status(drive, &p) != BURN_DRIVE_IDLE) {
		if(p.sectors>0 && p.sector>=0) /* display 1 to 99 percent */
			percent = 1.0 + ((double) p.sector+1.0)
					 / ((double) p.sectors) * 98.0;
		printf("Blanking  ( %.1f%% done )\n", percent);
		sleep(1);
	}
	if (burn_is_aborting(0) > 0)
		return -1;
	/* Back to synchronous handling */
	burn_set_signal_handling("libburner : ", NULL, 0x0);
	printf("Done\n");
	return 1;
}


/** Formats unformatted DVD-RW to profile 0013h "Restricted Overwrite"
    which needs no blanking for re-use but is not capable of multi-session.
    Expect a behavior similar to blanking with unusual noises from the drive.

    Formats unformatted BD-RE to default size. This will allocate some
    reserve space, test for bad blocks and make the media ready for writing.
    Expect a very long run time.

    Formats unformatted blank BD-R to hold a default amount of spare blocks
    for eventual mishaps during writing. If BD-R get written without being
    formatted, then they get no such reserve and will burn at full speed.
*/
int libburner_format(struct burn_drive *drive)
{
	struct burn_progress p;
	double percent = 1.0;
	int ret, status, num_formats, format_flag= 0;
	off_t size = 0;
	unsigned dummy;
	enum burn_disc_status disc_state;

	if (current_profile == 0x13) {
		fprintf(stderr, "IDLE: DVD-RW media is already formatted\n");
		return 2;
	} else if (current_profile == 0x41 || current_profile == 0x43) {
		disc_state = burn_disc_get_status(drive);
		if (disc_state != BURN_DISC_BLANK && current_profile == 0x41) {
			fprintf(stderr,
				"FATAL: BD-R is not blank. Cannot format.\n");
			return 0;
		}
		ret = burn_disc_get_formats(drive, &status, &size, &dummy,
								&num_formats);
		if (ret > 0 && status != BURN_FORMAT_IS_UNFORMATTED) {
			fprintf(stderr,
				"IDLE: BD media is already formatted\n");
			return 2;
		}
		size = 0;           /* does not really matter */
		format_flag = 3<<1; /* format to default size, no quick */
	} else if (current_profile == 0x14) { /* sequential DVD-RW */
		size = 128 * 1024 * 1024;
		format_flag = 1; /* write initial 128 MiB */
	} else {
		fprintf(stderr, "FATAL: Can only format DVD-RW or BD\n");
		return 0;
	}
	burn_set_signal_handling("libburner : ", NULL, 0x30);

	printf("Beginning to format media.\n");
	burn_disc_format(drive, size, format_flag);

	sleep(1);
	while (burn_drive_get_status(drive, &p) != BURN_DRIVE_IDLE) {
		if(p.sectors>0 && p.sector>=0) /* display 1 to 99 percent */
			percent = 1.0 + ((double) p.sector+1.0)
					 / ((double) p.sectors) * 98.0;
		printf("Formatting  ( %.1f%% done )\n", percent);
		sleep(1);
	}
	if (burn_is_aborting(0) > 0)
		return -1;
	burn_set_signal_handling("libburner : ", NULL, 0x0);
	burn_disc_get_profile(drive_list[0].drive, &current_profile,
				 current_profile_name);
	if (current_profile == 0x14 || current_profile == 0x13)
		printf("Media type now: %4.4xh  \"%s\"\n",
				 current_profile, current_profile_name);
	if (current_profile == 0x14) {
		fprintf(stderr,
		  "FATAL: Failed to change media profile to desired value\n");
		return 0;
	}
	return 1;
}


/** Brings preformatted track images (ISO 9660, audio, ...) onto media.
    To make sure a data image is fully readable on any Linux machine, this
    function adds 300 kiB of padding to the (usualy single) track.
    Audio tracks get padded to complete their last sector.
    A fifo of 4 MB is installed between each track and its data source.
    Each of the 4 MB buffers gets allocated automatically as soon as a track
    begins to be processed and it gets freed as soon as the track is done.
    The fifos do not wait for buffer fill but writing starts immediately.

    In case of external signals expect abort handling of an ongoing burn to
    last up to a minute. Wait the normal burning timespan before any kill -9.

    For simplicity, this function allows memory leaks in case of failure.
    In apps which do not abort immediately, one should clean up better.
*/
int libburner_payload(struct burn_drive *drive, 
		      char source_adr[][4096], int source_adr_count,
		      int multi, int simulate_burn, int all_tracks_type)
{
	struct burn_source *data_src, *fifo_src[99];
	struct burn_disc *target_disc;
	struct burn_session *session;
	struct burn_write_opts *burn_options;
	enum burn_disc_status disc_state;
	struct burn_track *track, *tracklist[99];
	struct burn_progress progress;
	time_t start_time;
	int last_sector = 0, padding = 0, trackno, unpredicted_size = 0, fd;
	int fifo_chunksize = 2352, fifo_chunks = 1783; /* ~ 4 MB fifo */
	off_t fixed_size;
	char *adr, reasons[BURN_REASONS_LEN];
	struct stat stbuf;

	if (all_tracks_type != BURN_AUDIO) {
		all_tracks_type = BURN_MODE1;
		/* a padding of 300 kiB helps to avoid the read-ahead bug */
		padding = 300*1024;
		fifo_chunksize = 2048;
		fifo_chunks = 2048; /* 4 MB fifo */
	}

	target_disc = burn_disc_create();
	session = burn_session_create();
	burn_disc_add_session(target_disc, session, BURN_POS_END);

	for (trackno = 0 ; trackno < source_adr_count; trackno++) {
	  tracklist[trackno] = track = burn_track_create();
	  burn_track_define_data(track, 0, padding, 1, all_tracks_type);

	  /* Open file descriptor to source of track data */
	  adr = source_adr[trackno];
	  fixed_size = 0;
	  if (adr[0] == '-' && adr[1] == 0) {
		fd = 0;
	  } else {
		fd = open(adr, O_RDONLY);
		if (fd>=0)
			if (fstat(fd,&stbuf)!=-1)
				if((stbuf.st_mode&S_IFMT)==S_IFREG)
					fixed_size = stbuf.st_size;
	  }
	  if (fixed_size==0)
		unpredicted_size = 1;

	  /* Convert this filedescriptor into a burn_source object */
	  data_src = NULL;
	  if (fd>=0)
	  	data_src = burn_fd_source_new(fd, -1, fixed_size);
	  if (data_src == NULL) {
		fprintf(stderr,
		       "FATAL: Could not open data source '%s'.\n",adr);
		if(errno!=0)
			fprintf(stderr,"(Most recent system error: %s )\n",
				strerror(errno));
		return 0;
	  }
	  /* Install a fifo object on top of that data source object */
	  fifo_src[trackno] = burn_fifo_source_new(data_src,
					fifo_chunksize, fifo_chunks, 0);
	  if (fifo_src[trackno] == NULL) {
		fprintf(stderr,
			"FATAL: Could not create fifo object of 4 MB\n");
		return 0;
	  }

	  /* Use the fifo object as data source for the track */
	  if (burn_track_set_source(track, fifo_src[trackno])
							 != BURN_SOURCE_OK) {
		fprintf(stderr,
		       "FATAL: Cannot attach source object to track object\n");
		return 0;
	  }

	  burn_session_add_track(session, track, BURN_POS_END);
	  printf("Track %d : source is '%s'\n", trackno+1, adr);

	  /* Give up local reference to the data burn_source object */
	  burn_source_free(data_src);
	  
	} /* trackno loop end */

	/* Evaluate drive and media */
	disc_state = burn_disc_get_status(drive);
	if (disc_state != BURN_DISC_BLANK &&
	    disc_state != BURN_DISC_APPENDABLE) {
		if (disc_state == BURN_DISC_FULL) {
			fprintf(stderr, "FATAL: Closed media with data detected. Need blank or appendable media.\n");
			if (burn_disc_erasable(drive))
				fprintf(stderr, "HINT: Try --blank_fast\n\n");
		} else if (disc_state == BURN_DISC_EMPTY) 
			fprintf(stderr,"FATAL: No media detected in drive\n");
		else
			fprintf(stderr,
			 "FATAL: Cannot recognize state of drive and media\n");
		return 0;
	}

	burn_options = burn_write_opts_new(drive);
	burn_write_opts_set_perform_opc(burn_options, 0);
	burn_write_opts_set_multi(burn_options, !!multi);
	if(simulate_burn)
		printf("\n*** Will TRY to SIMULATE burning ***\n\n");
	burn_write_opts_set_simulate(burn_options, simulate_burn);
	burn_drive_set_speed(drive, 0, 0);
	burn_write_opts_set_underrun_proof(burn_options, 1);
	if (burn_write_opts_auto_write_type(burn_options, target_disc,
					reasons, 0) == BURN_WRITE_NONE) {
		fprintf(stderr, "FATAL: Failed to find a suitable write mode with this media.\n");
		fprintf(stderr, "Reasons given:\n%s\n", reasons);
		return 0;
	}
	burn_set_signal_handling("libburner : ", NULL, 0x30);

	printf("Burning starts. With e.g. 4x media expect up to a minute of zero progress.\n");
	start_time = time(0);
	burn_disc_write(burn_options, target_disc);

	burn_write_opts_free(burn_options);
	while (burn_drive_get_status(drive, NULL) == BURN_DRIVE_SPAWNING)
		usleep(100002);
	while (burn_drive_get_status(drive, &progress) != BURN_DRIVE_IDLE) {
		if (progress.sectors <= 0 ||
		    (progress.sector >= progress.sectors - 1 &&
	             !unpredicted_size) ||
		    (unpredicted_size && progress.sector == last_sector))
			printf(
			     "Thank you for being patient since %d seconds.",
			     (int) (time(0) - start_time));
		else if(unpredicted_size)
			printf("Track %d : sector %d", progress.track+1,
				progress.sector);
		else
			printf("Track %d : sector %d of %d",progress.track+1,
				progress.sector, progress.sectors);
		last_sector = progress.sector;
		if (progress.track >= 0 && progress.track < source_adr_count) {
			int size, free_bytes, ret;
			char *status_text;
	
			ret = burn_fifo_inquire_status(
				fifo_src[progress.track], &size, &free_bytes,
				&status_text);
			if (ret >= 0 ) 
				printf("  [fifo %s, %2d%% fill]", status_text,
					(int) (100.0 - 100.0 *
						((double) free_bytes) /
						(double) size));
		} 
		printf("\n");
		sleep(1);
	}
	printf("\n");

	for (trackno = 0 ; trackno < source_adr_count; trackno++) {
	  	burn_source_free(fifo_src[trackno]);
		burn_track_free(tracklist[trackno]);
	}
	burn_session_free(session);
	burn_disc_free(target_disc);
	if (burn_is_aborting(0) > 0)
		return -1;
	if (multi && current_profile != 0x1a && current_profile != 0x13 &&
		current_profile != 0x12 && current_profile != 0x43) 
			/* not with DVD+RW, formatted DVD-RW, DVD-RAM, BD-RE */
		printf("NOTE: Media left appendable.\n");
	if (simulate_burn)
		printf("\n*** Did TRY to SIMULATE burning ***\n\n");
	return 1;
}


/** The setup parameters of libburner */
static char drive_adr[BURN_DRIVE_ADR_LEN] = {""};
static int driveno = 0;
static int do_blank = 0;
static char source_adr[99][4096];
static int source_adr_count = 0;
static int do_multi = 0;
static int simulate_burn = 0;
static int all_tracks_type = BURN_MODE1;


/** Converts command line arguments into above setup parameters.
*/
int libburner_setup(int argc, char **argv)
{
    int i, insuffient_parameters = 0, print_help = 0;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--audio")) {
            all_tracks_type = BURN_AUDIO;

        } else if (!strcmp(argv[i], "--blank_fast")) {
            do_blank = 1;

        } else if (!strcmp(argv[i], "--blank_full")) {
            do_blank = 2;

        } else if (!strcmp(argv[i], "--burn_for_real")) {
            simulate_burn = 0;

        } else if (!strcmp(argv[i], "--drive")) {
            ++i;
            if (i >= argc) {
                fprintf(stderr,"--drive requires an argument\n");
                return 1;
            } else if (strcmp(argv[i], "-") == 0) {
                drive_adr[0] = 0;
                driveno = -1;
            } else if (isdigit(argv[i][0])) {
                drive_adr[0] = 0;
                driveno = atoi(argv[i]);
            } else {
                if(strlen(argv[i]) >= BURN_DRIVE_ADR_LEN) {
                    fprintf(stderr,"--drive address too long (max. %d)\n",
                            BURN_DRIVE_ADR_LEN-1);
                    return 2;
                }
                strcpy(drive_adr, argv[i]);
            }
        } else if ((!strcmp(argv[i], "--format_overwrite")) ||
		   (!strcmp(argv[i], "--format"))) {
            do_blank = 101;

        } else if (!strcmp(argv[i], "--multi")) {
	    do_multi = 1;

	} else if (!strcmp(argv[i], "--stdin_size")) { /* obsoleted */
	    i++;

        } else if (!strcmp(argv[i], "--try_to_simulate")) {
            simulate_burn = 1;

        } else if (!strcmp(argv[i], "--help")) {
            print_help = 1;

        } else if (!strncmp(argv[i], "--",2)) {
            fprintf(stderr, "Unidentified option: %s\n", argv[i]);
            return 7;
        } else {
            if(strlen(argv[i]) >= 4096) {
                fprintf(stderr, "Source address too long (max. %d)\n", 4096-1);
                return 5;
            }
            if(source_adr_count >= 99) {
                fprintf(stderr, "Too many tracks (max. 99)\n");
                return 6;
            }
            strcpy(source_adr[source_adr_count], argv[i]);
            source_adr_count++;
        }
    }
    insuffient_parameters = 1;
    if (driveno < 0)
        insuffient_parameters = 0;
    if (source_adr_count > 0)
        insuffient_parameters = 0; 
    if (do_blank)
        insuffient_parameters = 0;
    if (print_help || insuffient_parameters ) {
        printf("Usage: %s\n", argv[0]);
        printf("       [--drive <address>|<driveno>|\"-\"]  [--audio]\n");
        printf("       [--blank_fast|--blank_full|--format]  [--try_to_simulate]\n");
        printf("       [--multi]  [<one or more imagefiles>|\"-\"]\n");
        printf("Examples\n");
        printf("A bus scan (needs rw-permissions to see a drive):\n");
        printf("  %s --drive -\n",argv[0]);
        printf("Burn a file to drive chosen by number, leave appendable:\n");
        printf("  %s --drive 0 --multi my_image_file\n", argv[0]);
        printf("Burn a file to drive chosen by persistent address, close:\n");
        printf("  %s --drive /dev/hdc my_image_file\n", argv[0]);
        printf("Blank a used CD-RW (is combinable with burning in one run):\n");
        printf("  %s --drive /dev/hdc --blank_fast\n",argv[0]);
        printf("Blank a used DVD-RW (is combinable with burning in one run):\n");
        printf("  %s --drive /dev/hdc --blank_full\n",argv[0]);
        printf("Format a DVD-RW, BD-RE or BD-R:\n");
        printf("  %s --drive /dev/hdc --format\n", argv[0]);
        printf("Burn two audio tracks (to CD only):\n");
        printf("  lame --decode -t /path/to/track1.mp3 track1.cd\n");
        printf("  test/dewav /path/to/track2.wav -o track2.cd\n");
        printf("  %s --drive /dev/hdc --audio track1.cd track2.cd\n", argv[0]);
        printf("Burn a compressed afio archive on-the-fly:\n");
        printf("  ( cd my_directory ; find . -print | afio -oZ - ) | \\\n");
        printf("  %s --drive /dev/hdc -\n", argv[0]);
        printf("To be read from *not mounted* media via: afio -tvZ /dev/hdc\n");
        if (insuffient_parameters)
            return 6;
    }
    return 0;
}


int main(int argc, char **argv)
{
	int ret;

	/* A warning to programmers who start their own projekt from here. */
	if (sizeof(off_t) != 8) {
		 fprintf(stderr,
	   "\nFATAL: Compile time misconfiguration. off_t is not 64 bit.\n\n");
		 exit(39);
	}

	ret = libburner_setup(argc, argv);
	if (ret)
		exit(ret);

	printf("Initializing libburnia-project.org ...\n");
	if (burn_initialize())
		printf("Done\n");
	else {
		printf("FAILED\n");
		fprintf(stderr,"\nFATAL: Failed to initialize.\n");
		exit(33);
	}

	/* Print messages of severity SORRY or more directly to stderr */
	burn_msgs_set_severities("NEVER", "SORRY", "libburner : ");

	/* Activate the synchronous signal handler which eventually will try to
	   properly shutdown drive and library on aborting events. */
	burn_set_signal_handling("libburner : ", NULL, 0x0);

	/** Note: driveno might change its value in this call */
	ret = libburner_aquire_drive(drive_adr, &driveno);
	if (ret<=0) {
		fprintf(stderr,"\nFATAL: Failed to aquire drive.\n");
		{ ret = 34; goto finish_libburn; }
	}
	if (ret == 2)
		{ ret = 0; goto release_drive; }
	if (do_blank) {
		if (do_blank > 100)
			ret = libburner_format(drive_list[driveno].drive);
		else
			ret = libburner_blank_disc(drive_list[driveno].drive,
							do_blank == 1);
		if (ret<=0)
			{ ret = 36; goto release_drive; }
	}
	if (source_adr_count > 0) {
		ret = libburner_payload(drive_list[driveno].drive,
				source_adr, source_adr_count,
				do_multi, simulate_burn, all_tracks_type);
		if (ret<=0)
			{ ret = 38; goto release_drive; }
	}
	ret = 0;
release_drive:;
	if (drive_is_grabbed)
		burn_drive_release(drive_list[driveno].drive, 0);

finish_libburn:;
	if (burn_is_aborting(0) > 0) {
		burn_abort(4400, burn_abort_pacifier, "libburner : ");
		fprintf(stderr,"\nlibburner run aborted\n");
		exit(1);
	} 
	/* This app does not bother to know about exact scan state. 
	   Better to accept a memory leak here. We are done anyway. */
	/* burn_drive_info_free(drive_list); */
	burn_finish();
	exit(ret);
}


/*  License and copyright aspects:

This all is provided under GPL.
Read. Try. Think. Play. Write yourself some code. Be free of my copyright.

Be also invited to study the code of cdrskin/cdrskin.c et al.

History:
libburner is a compilation of my own contributions to test/burniso.c and
fresh code which replaced the remaining parts under copyright of
Derek Foreman.
My respect and my thanks to Derek for providing me a start back in 2005.

*/

