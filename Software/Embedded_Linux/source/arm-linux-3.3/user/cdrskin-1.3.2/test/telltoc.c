
/*  test/telltoc.c , API illustration of obtaining media status info */
/*  Copyright (C) 2006 - 2011 Thomas Schmitt <scdbackup@gmx.net> 
    Provided under GPL */

/**                               Overview 
  
  telltoc is a minimal demo application for the library libburn as provided
  on  http://libburnia-project.org . It can list the available devices, can
  display some drive properties, the type of media, eventual table of content,
  multisession info for mkisofs option -C, and can read audio or data tracks. 

  It's main purpose, nevertheless, is to show you how to use libburn and also
  to serve the libburn team as reference application. telltoc.c does indeed
  define the standard way how above gestures can be implemented and stay upward
  compatible for a good while.
  
  Before you can do anything, you have to initialize libburn by
     burn_initialize()
  as it is done in main() at the end of this file. Then you aquire a
  drive in an appropriate way conforming to the API. The two main
  approaches are shown here in application functions:
     telltoc_aquire_by_adr()       demonstrates usage as of cdrecord traditions
     telltoc_aquire_by_driveno()   demonstrates a scan-and-choose approach
  With that aquired drive you can call
     telltoc_media()   prints some information about the media in a drive
     telltoc_toc()     prints a table of content (if there is content)
     telltoc_msinfo()  prints parameters for mkisofs option -C
     telltoc_read_and_print()  reads from audio or data CD or from DVD or BD
                       and prints 7-bit to stdout (encodings 0,2) or 8-bit to
                       file (encoding 1)
  When everything is done, main() releases the drive and shuts down libburn:
     burn_drive_release();
     burn_finish()
  
*/

/** See this for the decisive API specs . libburn.h is The Original */
/*  For using the installed header file :  #include <libburn/libburn.h> */
/*  This program insists in the own headerfile. */
#include "../libburn/libburn.h"

/* libburn is intended for Linux systems with kernel 2.4 or 2.6 for now */
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


/* Some in-advance definitions to allow a more comprehensive ordering
   of the functions and their explanations in here */
int telltoc_aquire_by_adr(char *drive_adr);
int telltoc_aquire_by_driveno(int *drive_no, int silent);


/* Messages from --toc to --read_and_print (CD tracksize is a bit tricky) */
static int last_track_start = 0, last_track_size = -1;
static int medium_is_cd_profile = 0;  /* 0 = undecided , -1 = no , 1 = yes */
static int cd_is_audio = 0;           /* 0 = undecided , -1 = no , 1 = yes */


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
int telltoc_aquire_drive(char *drive_adr, int *driveno, int silent_drive)
{
	int ret;

	if(drive_adr != NULL && drive_adr[0] != 0)
		ret = telltoc_aquire_by_adr(drive_adr);
	else
		ret = telltoc_aquire_by_driveno(driveno, silent_drive);
	return ret;
}


/** If the persistent drive address is known, then this approach is much
    more un-obtrusive to the systemwide livestock of drives. Only the
    given drive device will be opened during this procedure.
    Special drive addresses stdio:<path> direct output to a hard disk file
    which will behave much like a DVD-RAM.
*/
int telltoc_aquire_by_adr(char *drive_adr)
{
	int ret;
	char libburn_drive_adr[BURN_DRIVE_ADR_LEN];

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
    @param silent_drive 1=do not print "Drive found  :" line with *driveno >= 0
    @return 1 success , <= 0 failure
*/
int telltoc_aquire_by_driveno(int *driveno, int silent_drive)
{
	char adr[BURN_DRIVE_ADR_LEN];
	int ret, i;

	fprintf(stderr, "Beginning to scan for devices ...\n");
	while (!burn_drive_scan(&drive_list, &drive_count))
		usleep(100002);
	if (drive_count <= 0 && *driveno >= 0) {
		fprintf(stderr, "FAILED (no drives found)\n");
		return 0;
	}
	fprintf(stderr, "Done\n");

	for (i = 0; i < (int) drive_count; i++) {
		if (*driveno >= 0 && (silent_drive || *driveno != i))
	continue;
		if (burn_drive_get_adr(&(drive_list[i]), adr) <=0)
			strcpy(adr, "-get_adr_failed-");
		printf("Drive found  : %d  --drive '%s'  : ", i,adr);
		printf("%-8s  %-16s  (%4s)\n",
			drive_list[i].vendor,drive_list[i].product,
			drive_list[i].revision);
	}
	if (*driveno < 0) {
		fprintf(stderr, 
			"Pseudo-drive \"-\" given : bus scanning done.\n");
		return 2; /* the program will end after this */
	}

	/* We already made our choice via command line. (default is 0)
	   So we just have to keep our desired drive and drop all others.
	 */
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
			fprintf(stderr, "Dropped unwanted drive %d\n",i);
	}
	/* Make the one we want ready for inquiry */
	ret= burn_drive_grab(drive_list[*driveno].drive, 1);
	if (ret != 1)
		return 0;
	drive_is_grabbed = 1;
	return 1;
}


/** This gesture is necessary to get my NEC DVD_RW ND-4570A out of a state
    of noisy overexcitement after its tray was loaded and it then was inquired
    for Next Writeable Address.
    The noise then still lasts 20 seconds. Same with cdrecord -toc, btw.
    This opens a small gap for losing the drive to another libburn instance.
    Not a problem in telltoc. This is done as very last drive operation.
    Eventually the other libburn instance will have the same sanitizing effect.
*/
int telltoc_regrab(struct burn_drive *drive) {
	int ret;

	if (drive_is_grabbed)
		burn_drive_release(drive, 0);
	drive_is_grabbed = 0;
	ret = burn_drive_grab(drive, 0);
	if (ret != 0) {
		drive_is_grabbed = 1;
	}
	return !!ret;
}


int telltoc_media(struct burn_drive *drive)
{
	int ret, media_found = 0, profile_no = -1;
	double max_speed = 0.0, min_speed = 0.0, speed_conv;
	off_t available = 0;
	enum burn_disc_status s;
	char profile_name[80], speed_unit[40];
	struct burn_multi_caps *caps;
	struct burn_write_opts *o = NULL;

	printf("Media current: ");
	ret = burn_disc_get_profile(drive, &profile_no, profile_name);
	if (profile_no > 0 && ret > 0) {
		if (profile_name[0])
			printf("%s\n", profile_name);
		else
			printf("%4.4Xh\n", profile_no);
	} else
		printf("is not recognizable\n");

	speed_conv = 176.4;
	strcpy(speed_unit,"176.4 kB/s  (CD, data speed 150 KiB/s)");
	if (strstr(profile_name, "DVD") == profile_name) {
		speed_conv = 1385.0;
		strcpy(speed_unit,"1385.0 kB/s  (DVD)");
	}

	/* >>> libburn does not obtain full profile list yet */

	printf("Media status : ");
	s = burn_disc_get_status(drive);
	if (s == BURN_DISC_FULL) {
		printf("is written , is closed\n");
		media_found = 1;
	} else if (s == BURN_DISC_APPENDABLE) {
		printf("is written , is appendable\n");
		media_found = 1;
	} else if (s == BURN_DISC_BLANK) {
		printf("is blank\n");
		media_found = 1;
	} else if (s == BURN_DISC_EMPTY)
		printf("is not present\n");
	else
		printf("is not recognizable\n");

	printf("Media reuse  : ");
	if (media_found) {
		if (burn_disc_erasable(drive))
			printf("is erasable\n");
		else
			printf("is not erasable\n");	
	} else
		printf("is not recognizable\n");

	ret = burn_disc_get_multi_caps(drive, BURN_WRITE_NONE, &caps, 0);
	if (ret > 0) {
		/* Media appears writeable */
		printf("Write multi  : ");
		printf("%s multi-session , ",
			 caps->multi_session == 1 ? "allows" : "prohibits");
		if (caps->multi_track)
			printf("allows multiple tracks\n");
		else
			printf("enforces single track\n");
		printf("Write start  : ");
		if (caps->start_adr == 1)
			printf(
			"allows addresses [%.f , %.f]s , alignment=%.fs\n",
				(double) caps->start_range_low / 2048 ,
				(double) caps->start_range_high / 2048 ,
				(double) caps->start_alignment / 2048 );
		else
			printf("prohibits write start addressing\n");
		printf("Write modes  : ");
		if (caps->might_do_tao)
			printf("TAO%s",
				caps->advised_write_mode == BURN_WRITE_TAO ?
				" (advised)" : "");
		if (caps->might_do_sao)
			printf("%sSAO%s",
				caps->might_do_tao ? " , " : "",
				caps->advised_write_mode == BURN_WRITE_SAO ?
				" (advised)" : "");
		if (caps->might_do_raw)
			printf("%sRAW%s",
				caps->might_do_tao | caps->might_do_sao ?
				" , " : "",
				caps->advised_write_mode == BURN_WRITE_RAW ?
				" (advised)" : "");
		printf("\n");
		printf("Write dummy  : ");
		if (caps->might_simulate)
			printf("supposed to work with non-RAW modes\n");
		else
			printf("will not work\n");
		o= burn_write_opts_new(drive);
		if (o != NULL) {
			burn_write_opts_set_perform_opc(o, 0);
 			if(caps->advised_write_mode == BURN_WRITE_TAO)
   				burn_write_opts_set_write_type(o,
					BURN_WRITE_TAO, BURN_BLOCK_MODE1);
			else if (caps->advised_write_mode == BURN_WRITE_SAO)
				burn_write_opts_set_write_type(o,
					BURN_WRITE_SAO, BURN_BLOCK_SAO);
			else {
				burn_write_opts_free(o);
				o = NULL;
			}
		}
		available = burn_disc_available_space(drive, o);
		printf("Write space  : %.1f MiB  (%.fs)\n",
			((double) available) / 1024.0 / 1024.0,
			((double) available) / 2048.0);
		burn_disc_free_multi_caps(&caps);
		if (o != NULL)
			burn_write_opts_free(o);
	}

	ret = burn_drive_get_write_speed(drive);
	max_speed = ((double ) ret) / speed_conv;
	ret = burn_drive_get_min_write_speed(drive);
	min_speed = ((double ) ret) / speed_conv;
	if (!media_found)
		printf("Drive speed  : max=%.1f  , min=%.1f\n",
			 max_speed, min_speed);
	else
		printf("Avail. speed : max=%.1f  , min=%.1f\n", 
                         max_speed, min_speed);

	ret = 0;
	if (media_found)
		ret = burn_disc_read_atip(drive);
	if(ret>0) {
		ret = burn_drive_get_min_write_speed(drive);
		min_speed = ((double ) ret) / speed_conv;
		ret = burn_drive_get_write_speed(drive);
		max_speed = ((double ) ret) / speed_conv;
		printf("Media speed  : max=%.1f  , min=%.1f\n",
			max_speed, min_speed);
	}
	printf("Speed unit 1x: %s\n", speed_unit);

	return 1;
}


int telltoc_speedlist(struct burn_drive *drive)
{
	int ret, has_modern_entries = 0;
	struct burn_speed_descriptor *speed_list, *sd;

	ret = burn_drive_get_speedlist(drive, &speed_list);
	if (ret <= 0) {
		fprintf(stderr, "SORRY: Cannot obtain speed list info\n");
		return 2;
	}
	for (sd = speed_list; sd != NULL; sd = sd->next)
		if (sd->source == 2)
			has_modern_entries = 1;
	for (sd = speed_list; sd != NULL; sd = sd->next) {
		if (has_modern_entries && sd->source < 2)
	continue;
		if (sd->write_speed <= 0)
	continue;
		printf("Speed descr. : %d kB/s", sd->write_speed);
		if (sd->end_lba >= 0)
			printf(", %.1f MiB", ((double) sd->end_lba) / 512.0);
		if (sd->profile_name[0])
			printf(", %s", sd->profile_name);
		printf("\n");
	}
	burn_drive_free_speedlist(&speed_list);
	return 1;
}


int telltoc_formatlist(struct burn_drive *drive)
{
	int ret, i, status, num_formats, profile_no, type;
	off_t size;
	unsigned dummy;
	char status_text[80], profile_name[90];

	ret = burn_disc_get_formats(drive, &status, &size, &dummy,
				 &num_formats);
	if (ret <= 0) {
		fprintf(stderr, "SORRY: Cannot obtain format list info\n");
		return 2;
	}
	if (status == BURN_FORMAT_IS_UNFORMATTED)
		sprintf(status_text, "unformatted, up to %.1f MiB",
                        ((double) size) / 1024.0 / 1024.0);
	else if(status == BURN_FORMAT_IS_FORMATTED)
		sprintf(status_text, "formatted, with %.1f MiB",
                        ((double) size) / 1024.0 / 1024.0);
	else if(status == BURN_FORMAT_IS_UNKNOWN) {
		burn_disc_get_profile(drive, &profile_no, profile_name);
		if (profile_no > 0)
			sprintf(status_text, "intermediate or unknown");
		else
			sprintf(status_text, "no media or unknown media");
	} else
		sprintf(status_text, "illegal status according to MMC-5");
	printf("Format status: %s\n", status_text);

	for (i = 0; i < num_formats; i++) {
		ret = burn_disc_get_format_descr(drive, i,
						 &type, &size, &dummy);
		if (ret <= 0)
	continue;
		printf("Format descr.: %2.2Xh  , %.1f MiB  (%.fs)\n",
			type, ((double) size) / 1024.0 / 1024.0,
			((double) size) / 2048.0);
	}
	return 1;
}


void telltoc_detect_cd(struct burn_drive *drive)
{
	int pno;
	char profile_name[80];

	if (burn_disc_get_profile(drive, &pno, profile_name) > 0) {
		if (pno >= 0x08 && pno <= 0x0a)
			medium_is_cd_profile = 1;
		else
			medium_is_cd_profile = -1;
	}
}


int telltoc_toc(struct burn_drive *drive)
{
	int num_sessions = 0 , num_tracks = 0 , lba = 0, pmin, psec, pframe;
	int track_count = 0, track_is_audio;
	int session_no, track_no;
	struct burn_disc *disc= NULL;
	struct burn_session **sessions;
	struct burn_track **tracks;
	struct burn_toc_entry toc_entry;

	disc = burn_drive_get_disc(drive);
	if (disc==NULL) {
		fprintf(stderr, "SORRY: Cannot obtain Table Of Content\n");
		return 2;
	}
	sessions = burn_disc_get_sessions(disc, &num_sessions);
	for (session_no = 0; session_no<num_sessions; session_no++) {
		tracks = burn_session_get_tracks(sessions[session_no],
						&num_tracks);
		if (tracks==NULL)
	continue;
		for(track_no= 0; track_no<num_tracks; track_no++) {
			track_count++;
			burn_track_get_entry(tracks[track_no], &toc_entry);
			if (toc_entry.extensions_valid & 1) {
				/* DVD extension valid */
				lba = toc_entry.start_lba;
				burn_lba_to_msf(lba, &pmin, &psec, &pframe);
			} else {
				pmin = toc_entry.pmin;
				psec = toc_entry.psec;
				pframe = toc_entry.pframe;
				lba= burn_msf_to_lba(pmin, psec, pframe);
			}

			if ((toc_entry.control & 7) < 4) {
				if (cd_is_audio == 0)
					cd_is_audio = 1;
				track_is_audio = 1;
			} else {
				track_is_audio = 0;
				cd_is_audio = -1;
			}

			printf("Media content: session %2d  ", session_no+1);
			printf("track    %2d %s  lba: %9d  %4.2d:%2.2d:%2.2d\n",
				track_count,
				(track_is_audio ? "audio" : "data "),
				lba, pmin, psec, pframe);
			last_track_start = lba;
		}
		burn_session_get_leadout_entry(sessions[session_no],
						&toc_entry);
		if (toc_entry.extensions_valid & 1) {
			lba = toc_entry.start_lba;
			burn_lba_to_msf(lba, &pmin, &psec, &pframe);
		} else {
			pmin = toc_entry.pmin;
			psec = toc_entry.psec;
			pframe = toc_entry.pframe;
			lba= burn_msf_to_lba(pmin, psec, pframe);
		}
		printf("Media content: session %2d  ", session_no+1);
		printf("leadout            lba: %9d  %4.2d:%2.2d:%2.2d\n",
			lba, pmin, psec, pframe);
		last_track_size = lba - last_track_start;
		telltoc_detect_cd(drive);
	}
	if (disc!=NULL)
		burn_disc_free(disc);
	return 1;
}


int telltoc_msinfo(struct burn_drive *drive, 
			int msinfo_explicit, int msinfo_alone)
{
	int ret, lba, nwa = -123456789, aux_lba;
	enum burn_disc_status s;
	struct burn_write_opts *o= NULL;

	s = burn_disc_get_status(drive);
	if (s!=BURN_DISC_APPENDABLE) {
		if (!msinfo_explicit)
			return 2;
		fprintf(stderr,
		   "SORRY: --msinfo can only operate on appendable media.\n");
		return 0;
	}

	/* man mkisofs , option -C :
	   The first number is the sector number of the first sector in
	   the last session of the disk that should be appended to.
	*/
	ret = burn_disc_get_msc1(drive, &lba);
	if (ret <= 0) {
		fprintf(stderr,
			"SORRY: Cannot obtain start address of last session\n");
		{ ret = 0; goto ex; }
	}

	/* man mkisofs , option -C :
	   The second  number is the starting sector number of the new session.
	*/
	/* Set some roughly suitable write opts to be sent to drive. */
	o= burn_write_opts_new(drive);
 	if(o!=NULL) {
   		burn_write_opts_set_perform_opc(o, 0);
   		burn_write_opts_set_write_type(o,
					BURN_WRITE_TAO, BURN_BLOCK_MODE1);
	}
	/* Now try to inquire nwa from drive */
	ret= burn_disc_track_lba_nwa(drive,o,0,&aux_lba,&nwa);
	telltoc_regrab(drive); /* necessary to calm down my NEC drive */
	if(ret<=0) {
		fprintf(stderr,
			"SORRY: Cannot obtain next writeable address\n");
		{ ret = 0; goto ex; }
	}

	if (!msinfo_alone)
		printf("Media msinfo : mkisofs ... -C ");
	printf("%d,%d\n",lba,nwa);
	ret = 1;
ex:;
	if (o != NULL)
		burn_write_opts_free(o);
	return ret;
}


/**
  @param encoding determins how to format output on stdout:
                  0 = default , 1 = raw 8 bit (dangerous for tty) , 2 = hex
*/
int telltoc_read_and_print(struct burn_drive *drive, 
	int start_sector, int sector_count, char *raw_file, int encoding)
{
	int j, i, request = 16, done, lbas = 0, final_cd_try = -1, todo;
	int ret = 0, sector_size, chunk_size, read_audio = 0;
	char buf[16 * 2048], line[81];
	off_t data_count, total_count= 0, last_reported_count= 0;
	struct stat stbuf;
	FILE *raw_fp = NULL;

	if (medium_is_cd_profile == 0)
		telltoc_detect_cd(drive);
	if (start_sector == -1)
		start_sector = last_track_start;
	if (sector_count == -1) {
		sector_count = last_track_start + last_track_size
				- start_sector;
		if (medium_is_cd_profile > 0)  /* In case it is a TAO track */
			final_cd_try = 0; /* allow it (-1 is denial) */
	}

	if (sector_count <= 0)
		sector_count = 2147483632;

	if (encoding == 1) {
                if (stat(raw_file,&stbuf) != -1) {
			if (!(S_ISCHR(stbuf.st_mode) || S_ISFIFO(stbuf.st_mode)
				 || (stbuf.st_mode & S_IFMT) == S_IFSOCK )) {
                   	 	fprintf(stderr,
				  "SORRY: target file '%s' already existing\n",
				  raw_file);
				return 1;
			}
                }
                raw_fp = fopen(raw_file,"w");
                if (raw_fp == NULL) {
                    fprintf(stderr,"SORRY: cannot open target file '%s' (%s)\n", raw_file, strerror(errno));
                    return 1;
                }
		printf(
	"Data         : start=%ds , count=%ds , read=0s , encoding=%d:'%s'\n",
		start_sector, sector_count, encoding, raw_file);
	} else 
		printf(
		"Data         : start=%ds , count=%ds , read=0 , encoding=%d\n",
		start_sector, sector_count, encoding);

	/* Whether to read audio or data */
	if (cd_is_audio > 0) {
		read_audio = 1;
	} else if (medium_is_cd_profile > 0 && cd_is_audio == 0) {
		/* Try whether the start sector is audio */
		ret = burn_read_audio(drive, start_sector,
					buf, (off_t) 2352, &data_count, 2 | 4);
		if (ret > 0)
			read_audio = 1;
	}
	if (read_audio) {
		sector_size = 2352;
		chunk_size = 12;
	} else {
		sector_size = 2048;
		chunk_size = 16;
		if (start_sector < 0)
			start_sector = 0;
	}

	todo = sector_count - 2*(final_cd_try > -1);
	for (done = 0; done < todo && final_cd_try != 1; done += request) {
		if (todo - done > chunk_size)
			request = chunk_size;
		else
			request = todo - done;

		if (read_audio) {
			ret = burn_read_audio(drive, start_sector + done,
					buf, (off_t) (request * sector_size),
					&data_count, 0);
		} else {
			ret = burn_read_data(drive,
					((off_t) start_sector + done) *
							(off_t) sector_size,
					buf, (off_t) (request * sector_size),
					&data_count, 1);
		}
print_result:;
		total_count += data_count;
		if (encoding == 1) {
			if (data_count > 0)
				fwrite(buf, data_count, 1, raw_fp);
		} else for (i = 0; i < data_count; i += 16) {
			if (encoding == 0) {
				sprintf(line, "%8ds + %4d : ",
					start_sector + done + i / sector_size,
					i % sector_size);
				lbas = strlen(line);
			}
			for (j = 0; j < 16 && i + j < data_count; j++) {
				if (buf[i + j] >= ' ' && buf[i + j] <= 126 &&
					encoding != 2)
					sprintf(line + lbas + 3 * j, " %c ",
					 	(int) buf[i + j]);
				else
					sprintf(line + lbas + 3 * j, "%2.2X ",
					 	(unsigned char) buf[i + j]);
			}
			line[lbas + 3 * (j - 1) + 2] = 0;
			printf("%s\n",line);
		}
		if (encoding == 1 &&
		    total_count - last_reported_count >= 1000 * sector_size) {
			fprintf(stderr, 
	     		 "\rReading data : start=%ds , count=%ds , read=%ds  ",
			 start_sector, sector_count,
			  (int) (total_count / (off_t) sector_size));
			last_reported_count = total_count;
		}
		if (ret <= 0) {
			fprintf(stderr, "SORRY : Reading failed.\n");
	break;
		}
	}
	if (ret > 0 && medium_is_cd_profile > 0 && final_cd_try == 0) {
		/* In a SAO track the last 2 frames should be data too */
		final_cd_try = 1;
		if (read_audio) {
			ret = burn_read_audio(drive, start_sector + todo,
					buf, (off_t) (2 * sector_size),
					&data_count, 2);
		} else {
			burn_read_data(drive,
					((off_t) start_sector + todo) *
							(off_t) sector_size,
					buf, (off_t) (2 * sector_size),
					&data_count, 2);
		}
		if (data_count < 2 * sector_size)
			fprintf(stderr, "\rNOTE : Last two frames of CD track unreadable. This is normal if TAO track.\n");
		if (data_count > 0)
			goto print_result;	
	}
	if (last_reported_count > 0)
		fprintf(stderr,
"\r                                                                       \r");
	printf("End Of Data  : start=%ds , count=%ds , read=%ds\n",
		start_sector, sector_count,
	        (int) (total_count / (off_t) sector_size));

	return ret;
}


/** The setup parameters of telltoc */
static char drive_adr[BURN_DRIVE_ADR_LEN] = {""};
static int driveno = 0;
static int do_media = 0;
static int do_toc = 0;
static int do_msinfo = 0;
static int print_help = 0;
static int do_capacities = 0;
static int read_start = -2, read_count = -2, print_encoding = 0;
static char print_raw_file[4096] = {""};


/** Converts command line arguments into above setup parameters.
    drive_adr[] must provide at least BURN_DRIVE_ADR_LEN bytes.
    source_adr[] must provide at least 4096 bytes.
*/
int telltoc_setup(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--drive")) {
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
        } else if (strcmp(argv[i],"--media")==0) {
	    do_media = 1;

        } else if (!strcmp(argv[i], "--msinfo")) {
	    do_msinfo = 1;

        } else if (!strcmp(argv[i], "--capacities")) {
	    do_capacities = 1;

        } else if (!strcmp(argv[i], "--toc")) {
	    do_toc = 1;

        } else if (!strcmp(argv[i], "--read_and_print")) {
            i+= 3;
            if (i >= argc) {
                fprintf(stderr,"--read_and_print requires three arguments: start count encoding(try 0, not 1)\n");
                return 1;
            }
            sscanf(argv[i-2], "%d", &read_start);
            sscanf(argv[i-1], "%d", &read_count);
            print_encoding = 0;
            if(strncmp(argv[i], "raw:", 4) == 0 || strcmp(argv[i],"1:") == 0) {
                print_encoding = 1;
                strcpy(print_raw_file, strchr(argv[i], ':') + 1);
		if (strcmp(print_raw_file, "-") == 0) {
                	fprintf(stderr,
		"--read_and_print does not write to \"-\" as stdout.\n");
                	return 1;
		}
            } else if(strcmp(argv[i], "hex") == 0 || strcmp(argv[i], "2") == 0)
               print_encoding = 2;
            
        } else if (!strcmp(argv[i], "--help")) {
            print_help = 1;

        } else  {
            fprintf(stderr, "Unidentified option: %s\n", argv[i]);
            return 7;
        }
    }
    if (argc==1)
	print_help = 1;
    if (print_help) {
        printf("Usage: %s\n", argv[0]);
        printf("       [--drive <address>|<driveno>|\"-\"]\n");
        printf("       [--media]  [--capacities]  [--toc]  [--msinfo]\n");
        printf("       [--read_and_print <start> <count> \"0\"|\"hex\"|\"raw\":<path>]\n");
        printf("Examples\n");
        printf("A bus scan (needs rw-permissions to see a drive):\n");
        printf("  %s --drive -\n",argv[0]);
	printf("Obtain info about the type of loaded media:\n");
        printf("  %s --drive /dev/hdc --media\n",argv[0]);
	printf("Obtain table of content:\n");
        printf("  %s --drive /dev/hdc --toc\n",argv[0]);
	printf("Obtain parameters for option -C of program mkisofs:\n");
        printf("  msinfo=$(%s --drive /dev/hdc --msinfo 2>/dev/null)\n",
		argv[0]);
        printf("  mkisofs ... -C \"$msinfo\" ...\n");
	printf("Obtain what is available about drive 0 and its media\n");
	printf("  %s --drive 0\n",argv[0]);
	printf("View blocks 16 to 19 of audio or data CD or DVD or BD in human readable form\n");
	printf("  %s --drive /dev/sr1 --read_and_print 16 4 0 | less\n",
		argv[0]);
        printf("Copy last track from CD to file /tmp/data\n");
        printf("  %s --drive /dev/sr1 --toc --read_and_print -1 -1 raw:/tmp/data\n",
                argv[0]);
    }
    return 0;
}


int main(int argc, char **argv)
{
	int ret, toc_failed = 0, msinfo_alone = 0, msinfo_explicit = 0;
	int full_default = 0;

	ret = telltoc_setup(argc, argv);
	if (ret)
		exit(ret);

	/* Behavior shall be different if --msinfo is only option */
	if (do_msinfo) {
		msinfo_explicit = 1;
		if (!(do_media || do_toc))
			msinfo_alone = 1;
	}
	/* Default option is to do everything if possible */
    	if (do_media==0 && do_msinfo==0 && do_capacities==0 && do_toc==0 &&
		(read_start < 0 || read_count <= 0) && driveno!=-1) {
		if(print_help)
			exit(0);
		full_default = do_media = do_msinfo = do_capacities= do_toc= 1;
	}

	fprintf(stderr, "Initializing libburnia-project.org ...\n");
	if (burn_initialize())
		fprintf(stderr, "Done\n");
	else {
		fprintf(stderr,"\nFATAL: Failed to initialize.\n");
		exit(33);
	}

	/* Print messages of severity WARNING or more directly to stderr */
	burn_msgs_set_severities("NEVER", "WARNING", "telltoc : ");

	/* Activate the default signal handler */
	burn_set_signal_handling("telltoc : ", NULL, 0);

	/** Note: driveno might change its value in this call */
	ret = telltoc_aquire_drive(drive_adr, &driveno, !full_default);
	if (ret<=0) {
		fprintf(stderr,"\nFATAL: Failed to aquire drive.\n");
		{ ret = 34; goto finish_libburn; }
	}
	if (ret == 2)
		{ ret = 0; goto release_drive; }

	if (do_media) {
		ret = telltoc_media(drive_list[driveno].drive);
		if (ret<=0)
			{ret = 36; goto release_drive; }
	}
	if (do_capacities) {
		ret = telltoc_speedlist(drive_list[driveno].drive);
		if (ret<=0)
			{ret = 39; goto release_drive; }
		ret = telltoc_formatlist(drive_list[driveno].drive);
		if (ret<=0)
			{ret = 39; goto release_drive; }
	}
	if (do_toc) {
		ret = telltoc_toc(drive_list[driveno].drive);
		if (ret<=0)
			{ret = 37; goto release_drive; }
		if (ret==2)
			toc_failed = 1;
	}
	if (do_msinfo) {
		ret = telltoc_msinfo(drive_list[driveno].drive,
					msinfo_explicit, msinfo_alone);
		if (ret<=0)
			{ret = 38; goto release_drive; }
	}
	if (read_start != -2 && (read_count > 0 || read_count == -1)) {
		ret = telltoc_read_and_print(drive_list[driveno].drive,
				read_start, read_count, print_raw_file,
				print_encoding);
		if (ret<=0)
			{ret = 40; goto release_drive; }
	}

	ret = 0;
	if (toc_failed)
		ret = 37;
release_drive:;
	if (drive_is_grabbed)
		burn_drive_release(drive_list[driveno].drive, 0);

finish_libburn:;
	/* This app does not bother to know about exact scan state. 
	   Better to accept a memory leak here. We are done anyway. */
	/* burn_drive_info_free(drive_list); */

	burn_finish();
	exit(ret);
}

/*  License and copyright aspects:
    See libburner.c
*/

