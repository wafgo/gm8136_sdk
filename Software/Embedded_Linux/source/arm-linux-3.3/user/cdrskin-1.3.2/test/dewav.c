
/* dewav
   Demo of libburn extension libdax_audioxtr
   Audio track data extraction facility of libdax and libburn.
   Copyright (C) 2006 Thomas Schmitt <scdbackup@gmx.net>, provided under GPL
*/

#include <stdio.h> 
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


/* libdax_audioxtr is quite independent of libburn. It only needs
   the messaging facility libdax_msgs. So we got two build variations:
*/
#ifdef Dewav_without_libburN

/* This build environment is standalone relying only on libdax components */
#include "../libburn/libdax_msgs.h"
struct libdax_msgs *libdax_messenger= NULL;

/* The API for .wav extraction */
#define LIBDAX_AUDIOXTR_H_PUBLIC 1
#include "../libburn/libdax_audioxtr.h"

#else /* Dewav_without_libburN */

/* This build environment uses libdax_msgs and libdax_audioxtr via libburn */
/* Thus the API header of libburn */
#include "../libburn/libburn.h"

#endif /* ! Dewav_without_libburN */



int main(int argc, char **argv)
{
 /* This program acts as filter from in_path to out_path */
 char *in_path= "", *out_path= "-";

 /* The read-and-extract object for use with in_path */
 struct libdax_audioxtr *xtr= NULL;
 /* The file descriptor eventually detached from xtr */
 int xtr_fd= -2;

 /* Default output is stdout */
 int out_fd= 1;

 /* Inquired source parameters */
 char *fmt, *fmt_info;
 int num_channels, sample_rate, bits_per_sample, msb_first;
 off_t data_size;

 /* Auxiliary variables */
 int ret, i, be_strict= 1, buf_count, detach_fd= 0, extract_all= 0;
 char buf[2048];

 if(argc < 2)
   goto help;
 for(i= 1; i<argc; i++) {
   if(strcmp(argv[i],"-o")==0) {
     if(i>=argc-1) {
       fprintf(stderr,"%s: option -o needs a file address as argument.\n",
               argv[0]);
       exit(1);
     }
     i++;
     out_path= argv[i];
   } else if(strcmp(argv[i],"--lax")==0) {
     be_strict= 0;
   } else if(strcmp(argv[i],"--strict")==0) {
     be_strict= 1;
   } else if(strcmp(argv[i],"--detach_fd")==0) {
     /* Test the dirty detach method. Always --extract_all  */
     detach_fd= 1;
   } else if(strcmp(argv[i],"--extract_all")==0) {
     /* Dirty : read all available bytes regardless of data_size  */
     extract_all= 1;
   } else if(strcmp(argv[i],"--help")==0) {
help:;
     fprintf(stderr,
     "usage: %s [-o output_path|\"-\"] [--lax|--strict] [source_path|\"-\"]\n",
            argv[0]);
     exit(0);
   } else {
     if(in_path[0]!=0) {
       fprintf(stderr,"%s: only one input file is allowed.\n", argv[0]);
       exit(2);
     }
     in_path= argv[i];
   }
 }
 if(in_path[0] == 0)
   in_path= "-";


/* Depending on wether this was built standalone or with full libburn :
*/
#ifdef Dewav_without_libburN

 /* Initialize and set up libdax messaging system */
 ret= libdax_msgs_new(&libdax_messenger,0);
 if(ret<=0) {
   fprintf(stderr,"Failed to create libdax_messenger object.\n");
   exit(3);
 }
 libdax_msgs_set_severities(libdax_messenger, LIBDAX_MSGS_SEV_NEVER,
                            LIBDAX_MSGS_SEV_NOTE, "", 0);
 fprintf(stderr, "dewav on libdax\n");

#else /* Dewav_without_libburN */

 /* Initialize libburn and set up its messaging system */
 if(burn_initialize() == 0) {
   fprintf(stderr,"Failed to initialize libburn.\n");
   exit(3);
 }
 /* Print messages of severity NOTE or more directly to stderr */
 burn_msgs_set_severities("NEVER", "NOTE", "");
 fprintf(stderr, "dewav on libburn\n");

#endif /* ! Dewav_without_libburN */


 /* Open audio source and create extractor object */
 ret= libdax_audioxtr_new(&xtr, in_path, 0);
 if(ret<=0)
   exit(4);
 if(strcmp(out_path,"-")!=0) {
   out_fd= open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
   if(out_fd == -1) {
     fprintf(stderr, "Cannot open file: %s\n", out_path);
     fprintf(stderr, "Error reported: '%s'  (%d)\n",strerror(errno), errno);
     exit(5);
   }
 }
 /* Obtain and print parameters of audio source */
 libdax_audioxtr_get_id(xtr, &fmt, &fmt_info,
                 &num_channels, &sample_rate, &bits_per_sample, &msb_first, 0);
 fprintf(stderr, "Detected format: %s\n", fmt_info);
 libdax_audioxtr_get_size(xtr, &data_size, 0);
 fprintf(stderr, "Data size      : %.f bytes\n", (double) data_size);
 if((strcmp(fmt,".wav")!=0 && strcmp(fmt,".au")!=0) ||
    num_channels!=2 || sample_rate!=44100 || bits_per_sample!=16) {
   fprintf(stderr,
         "%sAudio source parameters do not comply to cdrskin/README specs\n",
         (be_strict ? "" : "WARNING: "));
   if(be_strict)
     exit(6);
 }
 if(msb_first==0)
   fprintf(stderr,
           "NOTE: Extracted data to be written with cdrskin option -swab\n");

 if(detach_fd) {
   /* Take over fd from xtr */;
   ret= libdax_audioxtr_detach_fd(xtr, &xtr_fd, 0);
   if(ret<=0) {
     fprintf(stderr, "Cannot detach file descriptor from extractor\n");
     exit(8);
   }
   /* not needed any more */
   libdax_audioxtr_destroy(&xtr, 0);
   fprintf(stderr, "Note: detached fd and freed extractor object.\n");
 }

 /* Extract and put out raw audio data */;
 while(1) {
   if(detach_fd) {
     buf_count= read(xtr_fd, buf, sizeof(buf));
     if(buf_count==-1)
       fprintf(stderr,"Error while reading from detached fd\n(%d) '%s'\n",
                      errno, strerror(errno));
   } else {
     buf_count= libdax_audioxtr_read(xtr, buf, sizeof(buf), !!extract_all);
   }
   if(buf_count < 0)
     exit(7);
   if(buf_count == 0)
 break;

   ret= write(out_fd, buf, buf_count);
   if(ret == -1) {
     fprintf(stderr, "Failed to write buffer of %d bytes to: %s\n",
                     buf_count, out_path);
     fprintf(stderr, "Error reported: '%s'  (%d)\n", strerror(errno), errno);
     exit(5);
   }

 }

 /* Shutdown */
 if(out_fd>2)
   close(out_fd);
 /* ( It is permissible to do this with xtr==NULL ) */
 libdax_audioxtr_destroy(&xtr, 0);

#ifdef Dewav_without_libburN

 libdax_msgs_destroy(&libdax_messenger,0);

#else /* Dewav_without_libburN */

 burn_finish();

#endif /* ! Dewav_without_libburN */

 exit(0);
} 
