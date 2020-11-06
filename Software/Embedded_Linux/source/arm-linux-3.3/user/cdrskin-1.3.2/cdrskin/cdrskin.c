
/*
 cdrskin.c , Copyright 2006-2013 Thomas Schmitt <scdbackup@gmx.net>
Provided under GPL version 2 or later.

A cdrecord compatible command line interface for libburn.

This project is neither directed against original cdrecord nor does it exploit
any source code of said program. It rather tries to be an alternative method
to burn CD, DVD, or BD, which is not based on the same code as cdrecord.
See also :  http://scdbackup.sourceforge.net/cdrskin_eng.html

Interested users of cdrecord are encouraged to contribute further option
implementations as they need them. Contributions will get published under GPL
but it is essential that the authors allow a future release under LGPL.

There is a script test/cdrecord_spy.sh which may be installed between
the cdrecord command and real cdrecord in order to learn about the options
used by your favorite cdrecord frontend. Edit said script and install it
according to the instructions given inside.

The implementation of an option would probably consist of
- necessary structure members for structs CdrpreskiN and/or CdrskiN
- code in Cdrpreskin_setup() and  Cdrskin_setup() which converts
  argv[i] into CdrpreskiN/CdrskiN members (or into direct actions)
- removal of option from ignore list "ignored_partial_options" resp.
  "ignored_full_options" in Cdrskin_setup()
- functions which implement the option's run time functionality
- eventually calls of those functions in Cdrskin_run()
- changes to be made within Cdrskin_burn() or Cdrskin_blank() or other
  existing methods
See option blank= for an example.

------------------------------------------------------------------------------

For a more comprehensive example of the advised way to write an application
of libburn see  test/libburner.c .
 
------------------------------------------------------------------------------
This program is currently copyright Thomas Schmitt only.
The copyrights of several components of libburnia-project.org are willfully
tangled at toplevel to form an irrevocable commitment to true open source
spirit.
We have chosen the GPL for legal compatibility and clearly express that it
shall not hamper the use of our software by non-GPL applications which show
otherwise the due respect to the open source community.
See toplevel README and cdrskin/README for that commitment.

For a short time, this place showed a promise to release a BSD license on
mere request. I have to retract that promise now, and replace it by the
promise to make above commitment reality in a way that any BSD conformant
usage in due open source spirit will be made possible somehow and in the
particular special case. I will not raise public protest if you fork yourself
a BSD license from an (outdated) cdrskin.c which still bears that old promise.
Note that this extended commitment is valid only for cdrskin.[ch],
cdrfifo.[ch] and cleanup.[ch], but not for libburnia-project.org as a whole.

cdrskin is originally inspired by libburn-0.2/test/burniso.c :
(c) Derek Foreman <derek@signalmarketing.com> and Ben Jansens <xor@orodu.net>

------------------------------------------------------------------------------

Compilation within cdrskin-* :

  cd cdrskin
  cc -g -I.. -DCdrskin_build_timestamP='...' \
     -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
     -o cdrskin cdrskin.c cdrfifo.c cleanup.c \
     -L../libburn/.libs -lburn -lpthread

or

  cd ..
  cc -g -I. -DCdrskin_build_timestamP='...' \
     -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 \
     -o cdrskin/cdrskin cdrskin/cdrskin.c cdrskin/cdrfifo.c cdrskin/cleanup.c \
     libburn/async.o libburn/crc.o libburn/debug.o libburn/drive.o \
     libburn/file.o libburn/init.o libburn/lec.o \
     libburn/mmc.o libburn/options.o libburn/sbc.o libburn/sector.o \
     libburn/sg.o libburn/spc.o libburn/source.o libburn/structure.o \
     libburn/toc.o libburn/util.o libburn/write.o libburn/read.o \
     libburn/libdax_audioxtr.o libburn/libdax_msgs.o \
     -lpthread

*/


/** The official program version */
#ifndef Cdrskin_prog_versioN
#define Cdrskin_prog_versioN "1.3.2"
#endif

/** The official libburn interface revision to use.
    (May get changed further below)
*/
#ifndef Cdrskin_libburn_majoR
#define Cdrskin_libburn_majoR 1
#endif
#ifndef Cdrskin_libburn_minoR
#define Cdrskin_libburn_minoR 3
#endif
#ifndef Cdrskin_libburn_micrO
#define Cdrskin_libburn_micrO 2
#endif


/** The source code release timestamp */
#include "cdrskin_timestamp.h"
#ifndef Cdrskin_timestamP
#define Cdrskin_timestamP "-none-given-"
#endif

/** The binary build timestamp is to be set externally by the compiler */
#ifndef Cdrskin_build_timestamP
#define Cdrskin_build_timestamP "-none-given-"
#endif


#ifdef Cdrskin_libburn_versioN 
#undef Cdrskin_libburn_versioN 
#endif

#ifdef Cdrskin_libburn_1_3_2
#define Cdrskin_libburn_versioN "1.3.2"
#endif

#ifdef Cdrskin_libburn_1_3_3
#define Cdrskin_libburn_versioN "1.3.3"
#endif

#ifndef Cdrskin_libburn_versioN
#define Cdrskin_libburn_1_3_2
#define Cdrskin_libburn_versioN "1.3.2"
#endif

#ifdef Cdrskin_libburn_1_3_2
#undef Cdrskin_libburn_majoR
#undef Cdrskin_libburn_minoR
#undef Cdrskin_libburn_micrO
#define Cdrskin_libburn_majoR 1
#define Cdrskin_libburn_minoR 3
#define Cdrskin_libburn_micrO 2
#endif
#ifdef Cdrskin_libburn_1_3_3
#undef Cdrskin_libburn_majoR
#undef Cdrskin_libburn_minoR
#undef Cdrskin_libburn_micrO
#define Cdrskin_libburn_majoR 1
#define Cdrskin_libburn_minoR 3
#define Cdrskin_libburn_micrO 3
#endif


/* History of development macros.
   As of version 1.1.8 they are now unconditional, thus removing the option
   to compile a heavily restricted cdrskin for the old libburn at icculus.org.
*/

/* 0.2.2 */
/* Cdrskin_libburn_does_ejecT */
/* Cdrskin_libburn_has_drive_get_adR */
/* Cdrskin_progress_track_does_worK */
/* Cdrskin_is_erasable_on_load_does_worK */
/* Cdrskin_grab_abort_does_worK */

/* 0.2.4 */
/* Cdrskin_allow_libburn_taO */
/* Cdrskin_libburn_has_is_enumerablE */
/* Cdrskin_libburn_has_convert_fs_adR */
/* Cdrskin_libburn_has_convert_scsi_adR */
/* Cdrskin_libburn_has_burn_msgS */
/* Cdrskin_libburn_has_burn_aborT */
/* Cdrskin_libburn_has_cleanup_handleR */
/* Cdrskin_libburn_has_audioxtR */
/* Cdrskin_libburn_has_get_start_end_lbA */
/* Cdrskin_libburn_has_burn_disc_unsuitablE */
/* Cdrskin_libburn_has_read_atiP */
/* Cdrskin_libburn_has_buffer_progresS */

/* 0.2.6 */
/* Cdrskin_libburn_has_pretend_fulL */
/* Cdrskin_libburn_has_multI */
/* Cdrskin_libburn_has_buffer_min_filL */

/* 0.3.0 */
/* Cdrskin_atip_speed_is_oK */
/* Cdrskin_libburn_has_get_profilE */
/* Cdrskin_libburn_has_set_start_bytE */
/* Cdrskin_libburn_has_wrote_welL */
/* Cdrskin_libburn_has_bd_formattinG */
/* Cdrskin_libburn_has_burn_disc_formaT */

/* 0.3.2 */
/* Cdrskin_libburn_has_get_msc1 */
/* Cdrskin_libburn_has_toc_entry_extensionS */
/* Cdrskin_libburn_has_get_multi_capS */

/* 0.3.4 */
/* Cdrskin_libburn_has_set_filluP */
/* Cdrskin_libburn_has_get_spacE */
/* Cdrskin_libburn_write_mode_ruleS */
/* Cdrskin_libburn_has_allow_untested_profileS */
/* Cdrskin_libburn_has_set_forcE */

/* 0.3.6 */
/* Cdrskin_libburn_preset_device_familY */
/* Cdrskin_libburn_has_track_set_sizE */

/* 0.3.8 */
/* Cdrskin_libburn_has_set_waitinG */
/* Cdrskin_libburn_has_get_best_speeD */

/* 0.4.0 */
/* Cdrskin_libburn_has_random_access_rW */
/* Cdrskin_libburn_has_get_drive_rolE */
/* Cdrskin_libburn_has_drive_equals_adR */

/* 0.4.2 */
/* no novel libburn features but rather organizational changes */

/* 0.4.4 */
/* novel libburn features are transparent to cdrskin */

/* 0.4.6 */
/* Cdrskin_libburn_has_stream_recordinG */

/* 0.4.8 */
/* Bug fix release for  write_start_address=... on DVD-RAM and BD-RE */

/* 0.5.0 , 0.5.2 , 0.5.4 , 0.5.6 , 0.5.8 , 0.6.0 , 0.6.2 */
/* novel libburn features are transparent to cdrskin */

/* 0.6.4 */
/* Ended to mark novelties by macros.
   libburnia libburn and cdrskin are fixely in sync now.
   icculus libburn did not move for 30 months.
*/

/* 1.1.8 */
/* The code which got enabled by novelty macros was made unconditional.
*/


#ifdef Cdrskin_new_api_tesT

/* put macros under test caveat here */


#endif /* Cdrskin_new_api_tesT */


/** ts A90901
    The raw write modes of libburn depend in part on code borrowed from cdrdao.
    Since this code is not understood by the current developers and since CDs
    written with cdrskin -raw96r seem unreadable anyway, -raw96r is given up
    for now.
*/
#define Cdrskin_disable_raw96R 1 


/** A macro which is able to eat up a function call like printf() */
#ifdef Cdrskin_extra_leaN
#define ClN(x) 
#define Cdrskin_no_cdrfifO 1
#else
#define ClN(x) x
#ifdef Cdrskin_use_libburn_fifO
/* 
 # define Cdrskin_no_cdrfifO 1
*/
#endif
#endif

/** Verbosity level for pacifying progress messages */
#define Cdrskin_verbose_progresS 1

/** Verbosity level for command recognition and execution logging */
#define Cdrskin_verbose_cmD 2

/** Verbosity level for reporting of debugging messages */
#define Cdrskin_verbose_debuG 3

/** Verbosity level for fifo debugging */
#define Cdrskin_verbose_debug_fifO 4


#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "../libburn/libburn.h"


#define Cleanup_set_handlers burn_set_signal_handling
#define Cleanup_app_handler_T burn_abort_handler_t

/*
 # define Cdrskin_use_libburn_cleanuP 1
*/
/* May not use libburn cleanup with cdrskin fifo */
#ifndef Cdrskin_use_libburn_fifO
#ifdef Cdrskin_use_libburn_cleanuP
#undef Cdrskin_use_libburn_cleanuP
#endif
#endif

#ifdef Cdrskin_use_libburn_cleanuP
#define Cleanup_handler_funC   NULL
#define Cleanup_handler_handlE "cdrskin: "
#define Cleanup_handler_flaG 48
#else
#define Cleanup_handler_funC   (Cleanup_app_handler_T) Cdrskin_abort_handler
#define Cleanup_handler_handlE skin
#define Cleanup_handler_flaG 4
#endif /* ! Cdrskin_use_libburn_cleanuP */

/* 0= no abort going on, -1= Cdrskin_abort_handler was called
*/
static int Cdrskin_abort_leveL= 0;


/** The size of a string buffer for pathnames and similar texts */
#define Cdrskin_strleN 4096

/** The maximum length +1 of a drive address */
#define Cdrskin_adrleN BURN_DRIVE_ADR_LEN


/** If tsize= sets a value smaller than media capacity divided by this
    number then there will be a warning and gracetime set at least to 15 */
#define Cdrskin_minimum_tsize_quotienT 2048.0


/* --------------------------------------------------------------------- */

/* Imported from scdbackup-0.8.5/src/cd_backup_planer.c */

/** Macro for creation of arrays of objects (or single objects) */
#define TSOB_FELD(typ,anz) (typ *) calloc(anz, sizeof(typ));


/** Convert a text so that eventual characters special to the shell are
    made literal. Note: this does not make a text terminal-safe !
    @param in_text The text to be converted
    @param out_text The buffer for the result. 
                    It should have size >= strlen(in_text)*5+2
    @param flag Unused yet
    @return For convenience out_text is returned
*/
char *Text_shellsafe(char *in_text, char *out_text, int flag)
{
 int l,i,w=0;

 /* enclose everything by hard quotes */
 l= strlen(in_text);
 out_text[w++]= '\'';
 for(i=0;i<l;i++){
   if(in_text[i]=='\''){
     /* escape hard quote within the text */
     out_text[w++]= '\'';
     out_text[w++]= '"';
     out_text[w++]= '\'';
     out_text[w++]= '"';
     out_text[w++]= '\'';
   } else {
     out_text[w++]= in_text[i];
   }
 }
 out_text[w++]= '\'';
 out_text[w++]= 0;
 return(out_text);
}


/** Convert a text into a number of type double and multiply it by unit code
    [kmgtpe] (2^10 to 2^60) or [s] (2048). (Also accepts capital letters.)
    @param text Input like "42", "2k", "3.14m" or "-1g"
    @param flag Bitfield for control purposes:
                bit0= return -1 rathern than 0 on failure
    @return The derived double value
*/
double Scanf_io_size(char *text, int flag)
/*
 bit0= default value -1 rather than 0
*/
{
 int c;
 double ret= 0.0;

 if(flag&1)
   ret= -1.0;
 if(text[0]==0)
   return(ret);
 sscanf(text,"%lf",&ret);
 c= text[strlen(text)-1];
 if(c=='k' || c=='K') ret*= 1024.0;
 if(c=='m' || c=='M') ret*= 1024.0*1024.0;
 if(c=='g' || c=='G') ret*= 1024.0*1024.0*1024.0;
 if(c=='t' || c=='T') ret*= 1024.0*1024.0*1024.0*1024.0;
 if(c=='p' || c=='P') ret*= 1024.0*1024.0*1024.0*1024.0*1024.0;
 if(c=='e' || c=='E') ret*= 1024.0*1024.0*1024.0*1024.0*1024.0*1024.0;
 if(c=='s' || c=='S') ret*= 2048.0;
 return(ret);
}


/** Return a double representing seconds and microseconds since 1 Jan 1970 */
double Sfile_microtime(int flag)
{
 struct timeval tv;
 struct timezone tz;
 gettimeofday(&tv,&tz);
 return((double) (tv.tv_sec+1.0e-6*tv.tv_usec));
}


/** Read a line from fp and strip LF or CRLF */
char *Sfile_fgets(char *line, int maxl, FILE *fp)
{
int l;
char *ret;
 ret= fgets(line,maxl,fp);
 if(ret==NULL) return(NULL);
 l= strlen(line);
 if(l>0) if(line[l-1]=='\r') line[--l]= 0;
 if(l>0) if(line[l-1]=='\n') line[--l]= 0;
 if(l>0) if(line[l-1]=='\r') line[--l]= 0;
 return(ret);
}

#ifndef Cdrskin_extra_leaN


/** Destroy a synthetic argument array */
int Sfile_destroy_argv(int *argc, char ***argv, int flag)
{
 int i;

 if(*argc>0 && *argv!=NULL){
   for(i=0;i<*argc;i++){
     if((*argv)[i]!=NULL) 
       free((*argv)[i]);
   }
   free((char *) *argv);
 }
 *argc= 0;
 *argv= NULL;
 return(1);
}


/** Read a synthetic argument array from a list of files.
    @param progname The content for argv[0]
    @param filenames The paths of the filex from where to read
    @param filenamecount The number of paths in filenames
    @param argc Returns the number of read arguments (+1 for progname)
    @param argv Returns the array of synthetic arguments
    @param argidx Returns source file indice of argv[] items
    @param arglno Returns source file line numbers of argv[] items
    @param flag Bitfield for control purposes:
                bit0= read progname as first argument from line
                bit1= just release argument array argv and return
                bit2= tolerate failure to open file
    @return 1=ok , 0=cannot open file , -1=cannot create memory objects
*/
int Sfile_multi_read_argv(char *progname, char **filenames, int filename_count,
                          int *argc, char ***argv, int **argidx, int **arglno,
                          int flag)
{
 int ret,i,pass,maxl=0,l,argcount=0,line_no;
 char buf[Cdrskin_strleN];
 FILE *fp= NULL;

 Sfile_destroy_argv(argc,argv,0);
 if(flag&2)
   return(1);
 if((*argidx)!=NULL)
   free((char *) *argidx);
 if((*arglno)!=NULL)
   free((char *) *arglno);
 *argidx= *arglno= NULL;

 for(pass=0;pass<2;pass++) {
   if(!(flag&1)){
     argcount= 1;
     if(pass==0)
       maxl= strlen(progname)+1;
     else {
      (*argv)[0]= (char *) calloc(1, strlen(progname)+1);
      if((*argv)[0]==NULL)
         {ret= -1; goto ex;}
       strcpy((*argv)[0],progname);
     }
   } else {
     argcount= 0;
     if(pass==0)
       maxl= 1;
   }
   for(i=0; i<filename_count;i++) {
     if(strlen(filenames[i])==0)
   continue;
     fp= fopen(filenames[i],"rb");
     if(fp==NULL) {
       if(flag&4)
   continue;
       {ret= 0; goto ex;}
     }
     line_no= 0;
     while(Sfile_fgets(buf,sizeof(buf)-1,fp)!=NULL) {
       line_no++;
       l= strlen(buf);
       if(l==0 || buf[0]=='#')
     continue;
       if(pass==0){
         if(l>maxl) 
           maxl= l;
       } else {
         if(argcount >= *argc)
     break;
         (*argv)[argcount]= (char *) calloc(1, l+1);
         if((*argv)[argcount]==NULL)
           {ret= -1; goto ex;}
         strcpy((*argv)[argcount],buf);
         (*argidx)[argcount]= i;
         (*arglno)[argcount]= line_no;
       }
       argcount++;
     }
     fclose(fp); fp= NULL;
   }
   if(pass==0){
     *argc= argcount;
     if(argcount>0) {
       *argv= (char **) calloc(argcount, sizeof(char *));
       *argidx= (int *) calloc(argcount, sizeof(int));
       *arglno= (int *) calloc(argcount, sizeof(int));
       if(*argv==NULL || *argidx==NULL || *arglno==NULL)
         {ret= -1; goto ex;}
     }
     for(i=0;i<*argc;i++) {
       (*argv)[i]= NULL;
       (*argidx)[i]= -1;
       (*arglno)[i]= -1;
     }
   }     
 }

 ret= 1;
ex:;
 if(fp!=NULL)
   fclose(fp);
 return(ret);
}


/** Combine environment variable HOME with given filename
    @param filename Address relative to $HOME
    @param fileadr Resulting combined address
    @param fa_size Size of array fileadr
    @param flag Unused yet
    @return 1=ok , 0=no HOME variable , -1=result address too long
*/
int Sfile_home_adr_s(char *filename, char *fileadr, int fa_size, int flag)
{
 char *home;

 strcpy(fileadr,filename);
 home= getenv("HOME");
 if(home==NULL)
   return(0);
 if((int) (strlen(home) + strlen(filename) + 1) >= fa_size)
   return(-1);
 strcpy(fileadr,home);
 if(filename[0]!=0){
   strcat(fileadr,"/");
   strcat(fileadr,filename);
 }
 return(1);
}


#endif /* ! Cdrskin_extra_leaN */


/* -------------------------- other misc functions ----------------------- */


/* Learned from reading growisofs.c , 
   watching mkisofs, and viewing its results via od -c */
/* @return 0=no size found , 1=*size_in_bytes is valid */
int Scan_for_iso_size(unsigned char data[2048], double *size_in_bytes,
                      int flag)
{
 double sectors= 0.0;

 if(data[0]!=1)
   return(0);
 if(strncmp((char *) (data+1),"CD001",5)!=0)
   return(0);
 sectors=  data[80] | (data[81]<<8) | (data[82]<<16) | (data[83]<<24); 
 *size_in_bytes= sectors*2048.0;
 return(1);
}


int Set_descr_iso_size(unsigned char data[2048], double size_in_bytes,
                      int flag)
{
 unsigned int sectors, i;

 sectors= size_in_bytes/2048.0;
 if(size_in_bytes>((double) sectors) * 2048.0)
   sectors++;
 for(i=0;i<4;i++)
   data[87-i]= data[80+i]= (sectors >> (8*i)) & 0xff;
 return(1);
}


int Wait_for_input(int fd, int microsec, int flag)
{
 struct timeval wt;
 fd_set rds,wts,exs;
 int ready;

 FD_ZERO(&rds);
 FD_ZERO(&wts);
 FD_ZERO(&exs);
 FD_SET(fd,&rds); 
 FD_SET(fd,&exs); 
 wt.tv_sec=  microsec/1000000;
 wt.tv_usec= microsec%1000000;
 ready= select(fd+1,&rds,&wts,&exs,&wt);
 if(ready<=0)
   return(0);
 if(FD_ISSET(fd,&exs))
   return(-1);
 if(FD_ISSET(fd,&rds))
   return(1);
 return(0);
}


/* --------------------------------------------------------------------- */

/** Address translation table for users/applications which do not look
   for the output of -scanbus but guess a Bus,Target,Lun on their own.
*/

/** The maximum number of entries in the address translation table */
#define Cdradrtrn_leN 256

/** The address prefix which will prevent translation */
#define Cdrskin_no_transl_prefiX "LITERAL_ADR:"


struct CdradrtrN {
 char *from_address[Cdradrtrn_leN];
 char *to_address[Cdradrtrn_leN];
 int fill_counter;
};


#ifndef Cdrskin_extra_leaN

/** Create a device address translator object */
int Cdradrtrn_new(struct CdradrtrN **trn, int flag)
{
 struct CdradrtrN *o;
 int i;

 (*trn)= o= TSOB_FELD(struct CdradrtrN,1);
 if(o==NULL)
   return(-1);
 for(i= 0;i<Cdradrtrn_leN;i++) {
   o->from_address[i]= NULL;
   o->to_address[i]= NULL;
 }
 o->fill_counter= 0;
 return(1);
}


/** Release from memory a device address translator object */
int Cdradrtrn_destroy(struct CdradrtrN **o, int flag)
{
 int i;
 struct CdradrtrN *trn;
 
 trn= *o;
 if(trn==NULL)
   return(0);
 for(i= 0;i<trn->fill_counter;i++) {
   if(trn->from_address[i]!=NULL)
     free(trn->from_address[i]);
   if(trn->to_address[i]!=NULL)
     free(trn->to_address[i]);
 }
 free((char *) trn);
 *o= NULL;
 return(1);
}


/** Add a translation pair to the table 
    @param trn The translator which shall learn
    @param from The user side address
    @param to The cdrskin side address
    @param flag Bitfield for control purposes:
                bit0= "from" contains from+to address, to[0] contains delimiter
*/
int Cdradrtrn_add(struct CdradrtrN *trn, char *from, char *to, int flag)
{
 char buf[2*Cdrskin_adrleN+1],*from_pt,*to_pt;
 int cnt;

 cnt= trn->fill_counter;
 if(cnt>=Cdradrtrn_leN)
   return(-1);
 if(flag&1) {
   if(strlen(from)>=sizeof(buf))
     return(0);
   strcpy(buf,from);
   to_pt= strchr(buf,to[0]);
   if(to_pt==NULL)
     return(0);
   *(to_pt)= 0;
   from_pt= buf;
   to_pt++;
 } else {
   from_pt= from;
   to_pt= to;
 }
 if(strlen(from)>=Cdrskin_adrleN || strlen(to)>=Cdrskin_adrleN)
   return(0);
 trn->from_address[cnt]= calloc(1, strlen(from_pt)+1);
 trn->to_address[cnt]= calloc(1, strlen(to_pt)+1);
 if(trn->from_address[cnt]==NULL ||
    trn->to_address[cnt]==NULL)
   return(-2);
 strcpy(trn->from_address[cnt],from_pt);
 strcpy(trn->to_address[cnt],to_pt);
 trn->fill_counter++;
 return(1);
}


/** Apply eventual device address translation
    @param trn The translator 
    @param from The address from which to translate
    @param driveno With backward translation only: The libburn drive number
    @param to The result of the translation
    @param flag Bitfield for control purposes:
                bit0= translate backward
    @return <=0 error, 1=no translation found, 2=translation found,
            3=collision avoided
*/
int Cdradrtrn_translate(struct CdradrtrN *trn, char *from, int driveno,
                        char to[Cdrskin_adrleN], int flag)
{
 int i,ret= 1;
 char *adr;

 to[0]= 0;
 adr= from;
 if(flag&1)
   goto backward;

 if(strncmp(adr,Cdrskin_no_transl_prefiX,
            strlen(Cdrskin_no_transl_prefiX))==0) {
   adr= adr+strlen(Cdrskin_no_transl_prefiX);
   ret= 2;
 } else {
   for(i=0;i<trn->fill_counter;i++)
     if(strcmp(adr,trn->from_address[i])==0)
   break;
   if(i<trn->fill_counter) {
     adr= trn->to_address[i];
     ret= 2;
   }
 }
 if(strlen(adr)>=Cdrskin_adrleN)
   return(-1);
 strcpy(to,adr);
 return(ret);

backward:;
 if(strlen(from)>=Cdrskin_adrleN)
   sprintf(to,"%s%d",Cdrskin_no_transl_prefiX,driveno);
 else
   strcpy(to,from);
 for(i=0;i<trn->fill_counter;i++)
   if(strcmp(from,trn->to_address[i])==0 &&
      strlen(trn->from_address[i])<Cdrskin_adrleN)
 break;
 if(i<trn->fill_counter) {
   ret= 2;
   strcpy(to,trn->from_address[i]);
 } else {
   for(i=0;i<trn->fill_counter;i++)
     if(strcmp(from,trn->from_address[i])==0)
   break;
   if(i<trn->fill_counter)
     if(strlen(from)+strlen(Cdrskin_no_transl_prefiX)<Cdrskin_adrleN) {
       ret= 3;
       sprintf(to,"%s%s",Cdrskin_no_transl_prefiX,from);
     }
 }
 return(ret);
}

#endif /* Cdrskin_extra_leaN */


/* --------------------------------------------------------------------- */


#ifndef Cdrskin_no_cdrfifO

/* Program is to be linked with cdrfifo.c */
#include "cdrfifo.h"

#else /* ! Cdrskin_no_cdrfifO */

/* Dummy */

struct CdrfifO {
 int dummy;
};

#endif /* Cdrskin_no_cdrfifO */


/* --------------------------------------------------------------------- */

/** cdrecord pads up to 600 kB in any case. 
    libburn yields blank result on tracks <~ 600 kB
    cdrecord demands 300 sectors = 705600 bytes for -audio */
static double Cdrtrack_minimum_sizE= 300;


/** This structure represents a track resp. a data source */
struct CdrtracK {

 struct CdrskiN *boss;
 int trackno;
  
 char source_path[Cdrskin_strleN];
 char original_source_path[Cdrskin_strleN];
 int source_fd;
 int is_from_stdin;
 double fixed_size;
 double tao_to_sao_tsize;
 double padding;
 int set_by_padsize;
 int sector_pad_up; /* enforce single sector padding */
 int track_type;
 int mode_modifiers;
 double sector_size;
 int track_type_by_default;
 int swap_audio_bytes;
 int cdxa_conversion; /* bit0-30: for burn_track_set_cdxa_conv()
                         bit31  : ignore bits 0 to 30
                       */
 char isrc[13];

 char *index_string;

 int sao_pregap;
 int sao_postgap;

 /** Eventually detected data image size */
 double data_image_size;
 char *iso_fs_descr;  /* eventually block 16 to 31 of input during detection */
 /** Whether to demand a detected data image size and use it (or else abort) */
 int use_data_image_size; /* 0=no, 1=size not defined yet, 2=size defined */

 /* Whether the data source is a container of defined size with possible tail*/
 int extracting_container;

 /** Optional fifo between input fd and libburn. It uses a pipe(2) to transfer
     data to libburn. 
 */
 int fifo_enabled;

 /** The eventual own fifo object managed by this track object. */
 struct CdrfifO *fifo;

 /** fd[0] of the fifo pipe. This is from where libburn reads its data. */
 int fifo_outlet_fd;
 int fifo_size;

 /** The possibly external fifo object which knows the real input fd and
     the fd[1] of the pipe. */
 struct CdrfifO *ff_fifo;
 /** The index number if fifo follow up fd item, -1= own fifo */
 int ff_idx;

 struct burn_track *libburn_track;
 int libburn_track_is_own;

#ifdef Cdrskin_use_libburn_fifO
 struct burn_source *libburn_fifo;
#endif /* Cdrskin_use_libburn_fifO */

};

int Cdrtrack_destroy(struct CdrtracK **o, int flag);
int Cdrtrack_set_track_type(struct CdrtracK *o, int track_type, int flag);


/** Create a track resp. data source object.
    @param track Returns the address of the new object.
    @param boss The cdrskin control object (corresponds to session)
    @param trackno The index in the cdrskin tracklist array (is not constant)
    @param flag Bitfield for control purposes:
                bit0= do not set parameters by Cdrskin_get_source()
                bit1= track is originally stdin
*/
int Cdrtrack_new(struct CdrtracK **track, struct CdrskiN *boss,
                 int trackno, int flag)
{
 struct CdrtracK *o;
 int ret,skin_track_type,fifo_start_at;
 int Cdrskin_get_source(struct CdrskiN *skin, char *source_path,
                        double *fixed_size, double *tao_to_sao_tsize,
                        int *use_data_image_size,
                        double *padding, int *set_by_padsize,
                        int *track_type, int *track_type_by_default,
                        int *mode_mods, int *swap_audio_bytes,
                        int *cdxa_conversion, int flag);
 int Cdrskin_get_fifo_par(struct CdrskiN *skin, int *fifo_enabled,
                          int *fifo_size, int *fifo_start_at, int flag);

 (*track)= o= TSOB_FELD(struct CdrtracK,1);
 if(o==NULL)
   return(-1);
 o->boss= boss;
 o->trackno= trackno;
 o->source_path[0]= 0;
 o->original_source_path[0]= 0;
 o->source_fd= -1;
 o->is_from_stdin= !!(flag&2);
 o->fixed_size= 0.0;
 o->tao_to_sao_tsize= 0.0;
 o->padding= 0.0;
 o->set_by_padsize= 0;
 o->sector_pad_up= 1;
 o->track_type= BURN_MODE1;
 o->mode_modifiers= 0;
 o->sector_size= 2048.0;
 o->track_type_by_default= 1;
 o->swap_audio_bytes= 0;
 o->cdxa_conversion= 0;
 o->isrc[0]= 0;
 o->index_string= NULL;
 o->sao_pregap= -1;
 o->sao_postgap= -1;
 o->data_image_size= -1.0;
 o->iso_fs_descr= NULL;
 o->use_data_image_size= 0;
 o->extracting_container= 0;
 o->fifo_enabled= 0;
 o->fifo= NULL;
 o->fifo_outlet_fd= -1;
 o->fifo_size= 0;
 o->ff_fifo= NULL;
 o->ff_idx= -1;
 o->libburn_track= NULL;
 o->libburn_track_is_own= 0;
#ifdef Cdrskin_use_libburn_fifO
 o->libburn_fifo= NULL;
#endif /* Cdrskin_use_libburn_fifO */

 if(flag & 1)
   return(1);

 ret= Cdrskin_get_source(boss,o->source_path,&(o->fixed_size),
                         &(o->tao_to_sao_tsize),&(o->use_data_image_size),
                         &(o->padding),&(o->set_by_padsize),&(skin_track_type),
                         &(o->track_type_by_default), &(o->mode_modifiers),
                         &(o->swap_audio_bytes), &(o->cdxa_conversion), 0);
 if(ret<=0)
   goto failed;
 strcpy(o->original_source_path,o->source_path);
 if(o->fixed_size>0.0)
   o->extracting_container= 1;
 Cdrtrack_set_track_type(o,skin_track_type,0);

#ifndef Cdrskin_extra_leaN
 ret= Cdrskin_get_fifo_par(boss, &(o->fifo_enabled),&(o->fifo_size),
                           &fifo_start_at,0);
 if(ret<=0)
   goto failed;
#endif /* ! Cdrskin_extra_leaN */

 return(1);
failed:;
 Cdrtrack_destroy(track,0);
 return(-1);
}


/** Release from memory a track object previously created by Cdrtrack_new() */
int Cdrtrack_destroy(struct CdrtracK **o, int flag)
{
 struct CdrtracK *track;

 track= *o;
 if(track==NULL)
   return(0);

#ifndef Cdrskin_no_cdrfifO
 Cdrfifo_destroy(&(track->fifo),0);
#endif

 if(track->libburn_track != NULL && track->libburn_track_is_own)
   burn_track_free(track->libburn_track);
 if(track->iso_fs_descr!=NULL)
   free((char *) track->iso_fs_descr);
 if(track->index_string != NULL)
   free(track->index_string);
 free((char *) track);
 *o= NULL;
 return(1);
}


int Cdrtrack_set_track_type(struct CdrtracK *o, int track_type, int flag)
{
 if(track_type==BURN_AUDIO) {
   o->track_type= BURN_AUDIO;
   o->sector_size= 2352.0;
 } else {
   o->track_type= BURN_MODE1;
   o->sector_size= 2048.0;
 }
 return(1);
}


int Cdrtrack_get_track_type(struct CdrtracK *o, int *track_type, 
                            int *sector_size, int flag)
{
 *track_type= o->track_type;
 *sector_size= o->sector_size;
 return(1);
}


/** 
    @param flag Bitfield for control purposes:
                bit0= size returns number of actually processed source bytes
                      rather than the predicted fixed_size (if available).
                      padding returns the difference from number of written
                      bytes.
                bit1= size returns fixed_size, padding returns tao_to_sao_tsize
*/
int Cdrtrack_get_size(struct CdrtracK *track, double *size, double *padding,
                      double *sector_size, int *use_data_image_size, int flag)
{
 off_t readcounter= 0,writecounter= 0;

 *size= track->fixed_size;
 *padding= track->padding;
 *use_data_image_size= track->use_data_image_size;
 if((flag&1) && track->libburn_track!=NULL) {
   burn_track_get_counters(track->libburn_track,&readcounter,&writecounter);
   *size= readcounter;
   *padding= writecounter-readcounter;
 } else if(flag&2)
   *padding= track->tao_to_sao_tsize;
 *sector_size= track->sector_size;
 return(1);
}


int Cdrtrack_get_iso_fs_descr(struct CdrtracK *track,
                              char **descr, double *size, int flag)
{
 *descr= track->iso_fs_descr;
 *size= track->data_image_size;
 return(*descr != NULL && *size > 0.0); 
}


int Cdrtrack_get_source_path(struct CdrtracK *track,
              char **source_path, int *source_fd, int *is_from_stdin, int flag)
{
 *source_path= track->original_source_path;
 *source_fd= track->source_fd;
 *is_from_stdin= track->is_from_stdin;
 return(1);
}


#ifdef Cdrskin_use_libburn_fifO

int Cdrtrack_get_libburn_fifo(struct CdrtracK *track,
                              struct burn_source **fifo, int flag)
{
 *fifo= track->libburn_fifo;
 return(1);
}


int Cdrtrack_report_fifo(struct CdrtracK *track, int flag)
{
 int size, free_bytes, ret;
 int total_min_fill, interval_min_fill, put_counter, get_counter;
 int empty_counter, full_counter;
 double fifo_percent;
 char *status_text;

 if(track->libburn_fifo == NULL)
   return(0);

 /* Check for open input or leftover bytes in liburn fifo */
 ret = burn_fifo_inquire_status(track->libburn_fifo, &size, &free_bytes,
                                &status_text);
 if(ret >= 0 && size - free_bytes > 1) {
                                 /* not clear why free_bytes is reduced by 1 */
   fprintf(stderr,
     "cdrskin: FATAL : Fifo still contains data after burning has ended.\n");
   fprintf(stderr,
     "cdrskin: FATAL : %d bytes left.\n", size - free_bytes - 1);
   fprintf(stderr,
     "cdrskin: FATAL : This indicates an overflow of the last track.\n");
   fprintf(stderr,
     "cdrskin: NOTE : The media might appear ok but is probably truncated.\n");
   return(-1);
 }

 burn_fifo_get_statistics(track->libburn_fifo, &total_min_fill,
                          &interval_min_fill, &put_counter, &get_counter,
                          &empty_counter, &full_counter);
 fifo_percent= 100.0*((double) total_min_fill)/(double) size;
 if(fifo_percent==0 && total_min_fill>0)
   fifo_percent= 1;
 fflush(stdout);
 fprintf(stderr,"Cdrskin: fifo had %d puts and %d gets.\n",
         put_counter,get_counter);
 fprintf(stderr,
   "Cdrskin: fifo was %d times empty and %d times full, min fill was %.f%%.\n",
             empty_counter, full_counter, fifo_percent);
 return(1);
}

#endif /* Cdrskin_use_libburn_fifO */


int Cdrtrack_get_fifo(struct CdrtracK *track, struct CdrfifO **fifo, int flag)
{
 *fifo= track->fifo;
 return(1);
}


/** Try whether automatic audio extraction is appropriate and eventually open
    a file descriptor to the raw data.
    @return -3 identified as .wav but with cdrecord-inappropriate parameters
            -2 could not open track source, no use in retrying
            -1 severe error
             0 not appropriate to extract, burn plain file content
             1 to be extracted, *fd is a filedescriptor delivering raw data
*/
int Cdrtrack_extract_audio(struct CdrtracK *track, int *fd, off_t *xtr_size,
                           int flag)
{
 int l, ok= 0;
 struct libdax_audioxtr *xtr= NULL;
 char *fmt,*fmt_info;
 int num_channels,sample_rate,bits_per_sample,msb_first,ret;

 *fd= -1;

 if(track->track_type!=BURN_AUDIO && !track->track_type_by_default)
   return(0);
 l= strlen(track->source_path);
 if(l>=4)
   if(strcmp(track->source_path+l-4,".wav")==0) 
     ok= 1;
 if(l>=3)
   if(strcmp(track->source_path+l-3,".au")==0)
     ok= 1;
 if(!ok)
   return(0);

 if(track->track_type_by_default) {
   Cdrtrack_set_track_type(track,BURN_AUDIO,0);
   track->track_type_by_default= 2;
   fprintf(stderr,"cdrskin: NOTE : Activated -audio for '%s'\n",
           track->source_path);
 }

 ret= libdax_audioxtr_new(&xtr,track->source_path,0);
 if(ret<=0)
   return(ret);
 libdax_audioxtr_get_id(xtr,&fmt,&fmt_info,
                     &num_channels,&sample_rate,&bits_per_sample,&msb_first,0);
 if((strcmp(fmt,".wav")!=0 && strcmp(fmt,".au")!=0) || 
    num_channels!=2 || sample_rate!=44100 || bits_per_sample!=16) {
   fprintf(stderr,"cdrskin: ( %s )\n",fmt_info);
   fprintf(stderr,"cdrskin: FATAL : Inappropriate audio coding in '%s'.\n",
                  track->source_path);
   {ret= -3; goto ex;}
 }
 libdax_audioxtr_get_size(xtr,xtr_size,0);
 ret= libdax_audioxtr_detach_fd(xtr,fd,0);
 if(ret<=0)
   {ret= -1*!!ret; goto ex;}
 track->swap_audio_bytes= !!msb_first;
 track->extracting_container= 1;
 fprintf(stderr,"cdrskin: NOTE : %.f %saudio bytes in '%s'\n",
                (double) *xtr_size, (msb_first ? "" : "(-swab) "),
                track->source_path);
 ret= 1;
ex:
 libdax_audioxtr_destroy(&xtr,0);
 return(ret);
}


/* @param flag bit0=set *size_used as the detected data image size
*/
int Cdrtrack_activate_image_size(struct CdrtracK *track, double *size_used,
                                 int flag)
{
 if(flag&1)
   track->data_image_size= *size_used;
 else
   *size_used= track->data_image_size;
 if(track->use_data_image_size!=1)
   return(2);
 if(*size_used<=0)
   return(0);
 track->fixed_size= *size_used;
 track->use_data_image_size= 2;
 if(track->libburn_track!=NULL)
   burn_track_set_size(track->libburn_track, (off_t) *size_used);
 /* man cdrecord prescribes automatic -pad with -isosize. 
    cdrskin obeys only if the current padding is less than that. */
 if(track->padding<15*2048) {
   track->padding= 15*2048;
   track->set_by_padsize= 0;
 }
 track->extracting_container= 1;

#ifndef Cdrskin_no_cdrfifO
 if(track->ff_fifo!=NULL)
   Cdrfifo_set_fd_in_limit(track->ff_fifo,track->fixed_size,track->ff_idx,0);
#endif

 return(1);
}


int Cdrtrack_seek_isosize(struct CdrtracK *track, int fd, int flag)
{
 struct stat stbuf;
 char secbuf[2048];
 int ret,got,i;
 double size;

 if(fstat(fd,&stbuf)==-1)
   return(0);
 if((stbuf.st_mode&S_IFMT)!=S_IFREG && (stbuf.st_mode&S_IFMT)!=S_IFBLK)
   return(2);

 if(track->iso_fs_descr!=NULL)
   free((char *) track->iso_fs_descr);
 track->iso_fs_descr= TSOB_FELD(char,16*2048);
 if(track->iso_fs_descr==NULL)
   return(-1);
 for(i=0;i<32 && track->data_image_size<=0;i++) {
   for(got= 0; got<2048;got+= ret) {
     ret= read(fd, secbuf+got, 2048-got);
     if(ret<=0)
       return(0);
   }
   if(i<16)
 continue;
   memcpy(track->iso_fs_descr+(i-16)*2048,secbuf,2048);
   if(i>16)
 continue;
   ret= Scan_for_iso_size((unsigned char *) secbuf, &size, 0);
   if(ret<=0)
 break;
   track->data_image_size= size;
   if(track->use_data_image_size) {
     Cdrtrack_activate_image_size(track,&size,1);
     track->fixed_size= size;
     track->use_data_image_size= 2;
   }
 }
 ret= lseek(fd, (off_t) 0, SEEK_SET);
 if(ret!=0) {
   fprintf(stderr,
        "cdrskin: FATAL : Cannot lseek() to 0 after -isosize determination\n");
   if(errno!=0)
     fprintf(stderr, "cdrskin: errno=%d : %s\n", errno, strerror(errno));
   return(-1);
 }
 return(track->data_image_size>0);
}


/** Deliver an open file descriptor corresponding to the source path of track.
    @param flag Bitfield for control purposes:
                bit0=debugging verbosity
                bit1=open as source for direct write: 
                     no audio extract, no minimum track size
                bit2=permission to use burn_os_open_track_src() (evtl O_DIRECT)
    @return <=0 error, 1 success
*/
int Cdrtrack_open_source_path(struct CdrtracK *track, int *fd, int flag)
{
 int is_wav= 0, size_from_file= 0, ret;
 off_t xtr_size= 0;
 struct stat stbuf;
 char *device_adr,*raw_adr;
 int no_convert_fs_adr;
 int Cdrskin_get_device_adr(struct CdrskiN *skin,
           char **device_adr, char **raw_adr, int *no_convert_fs_adr,int flag);
 int Cdrskin_get_drive(struct CdrskiN *skin, struct burn_drive **drive,
           int flag);
 struct burn_drive *drive;

 if(track->source_path[0]=='-' && track->source_path[1]==0)
   *fd= 0;
 else if(track->source_path[0]=='#' && 
         (track->source_path[1]>='0' && track->source_path[1]<='9'))
   *fd= atoi(track->source_path+1);
 else {
   *fd= -1;

   ret= Cdrskin_get_device_adr(track->boss,&device_adr,&raw_adr,
                               &no_convert_fs_adr,0);
   if(ret <= 0) {
     fprintf(stderr,
             "cdrskin: FATAL : No drive found. Cannot prepare track.\n");
     return(0);
   }
/*    
   fprintf(stderr,
           "cdrskin: DEBUG : device_adr='%s' , raw_adr='%s' , ncfs=%d\n",
           device_adr, raw_adr, no_convert_fs_adr);
*/
   if(!no_convert_fs_adr) {
     if(flag&1)
       ClN(fprintf(stderr,
          "cdrskin_debug: checking track source for identity with drive\n"));

     ret= Cdrskin_get_drive(track->boss,&drive,0);
     if(ret<=0) {
       fprintf(stderr,
          "cdrskin: FATAL : Program error. Cannot determine libburn drive.\n");
       return(0);
     }
     if(burn_drive_equals_adr(drive,track->source_path,2)>0) {
       fprintf(stderr,
              "cdrskin: FATAL : track source address leads to burner drive\n");
       fprintf(stderr,
              "cdrskin:       : dev='%s' -> '%s' <- track source '%s'\n",
              raw_adr, device_adr, track->source_path);
       return(0);
     }
   }
/*
   fprintf(stderr,"cdrskin: EXPERIMENTAL : Deliberate abort\n");
   return(0);
*/

   if(!(flag&2))
     is_wav= Cdrtrack_extract_audio(track,fd,&xtr_size,0);
   if(is_wav==-1)
     return(-1);
   if(is_wav==-3)
     return(0);
   if(is_wav==0) {
     if(track->track_type != BURN_MODE1 ||
        (track->cdxa_conversion & 0x7fffffff))
       flag&= ~4;                  /* Better avoid O_DIRECT with odd sectors */
     if(flag & 4)
       *fd= burn_os_open_track_src(track->source_path, O_RDONLY, 0);
     else
       *fd= open(track->source_path, O_RDONLY);
   }
   if(*fd==-1) {
     fprintf(stderr,"cdrskin: failed to open source address '%s'\n",
             track->source_path);
     fprintf(stderr,"cdrskin: errno=%d , \"%s\"\n",errno,
                    errno==0?"-no error code available-":strerror(errno));
     return(0);
   }
   if(track->use_data_image_size==1 && xtr_size<=0) {
     ret= Cdrtrack_seek_isosize(track,*fd,0);
     if(ret==-1)
       return(-1);
   } else if(track->fixed_size<=0) {

     /* >>> ??? is it intentional that tsize overrides .wav header ? */
     if(xtr_size>0) {

       track->fixed_size= xtr_size;
       if(track->use_data_image_size==1)
         track->use_data_image_size= 2; /* count this as image size found */
       size_from_file= 1;
     } else {
       if(fstat(*fd,&stbuf)!=-1) {
         if((stbuf.st_mode&S_IFMT)==S_IFREG) {
           track->fixed_size= stbuf.st_size;
           size_from_file= 1;
         } /* all other types are assumed of open ended size */
       }
     }
   }
 }

 if(track->fixed_size < Cdrtrack_minimum_sizE * track->sector_size
    && (track->fixed_size>0 || size_from_file) && !(flag&2)) {
   if(track->track_type == BURN_AUDIO) {
     /* >>> cdrecord: We differ in automatic padding with audio:
       Audio tracks must be at least 705600 bytes and a multiple of 2352.
     */
     fprintf(stderr,
             "cdrskin: FATAL : Audio tracks must be at least %.f bytes\n",
             Cdrtrack_minimum_sizE*track->sector_size);
     return(0);
   } else {
     fprintf(stderr,
             "cdrskin: NOTE : Enforcing minimum track size of %.f bytes\n",
             Cdrtrack_minimum_sizE*track->sector_size);
     track->fixed_size= Cdrtrack_minimum_sizE*track->sector_size;
   }
 }
 track->source_fd= *fd;
 return(*fd>=0);
}


#ifndef Cdrskin_no_cdrfifO

/** Install a fifo object between data source and libburn.
    Its parameters are known to track.
    @param outlet_fd Returns the filedescriptor of the fifo outlet.
    @param previous_fifo Object address for chaining or follow-up attachment.
    @param flag Bitfield for control purposes:
                bit0= Debugging verbosity
                bit1= Do not create and attach a new fifo 
                      but attach new follow-up fd pair to previous_fifo
                bit2= Do not enforce fixed_size if not container extraction
    @return <=0 error, 1 success
*/
int Cdrtrack_attach_fifo(struct CdrtracK *track, int *outlet_fd, 
                         struct CdrfifO *previous_fifo, int flag)
{
 struct CdrfifO *ff;
 int source_fd,pipe_fds[2],ret;

 *outlet_fd= -1;
 if(track->fifo_size<=0)
   return(2);
 ret= Cdrtrack_open_source_path(track,&source_fd,
                            (flag&1) | (4 * (track->fifo_size >= 256 * 1024)));
 if(ret<=0)
   return(ret);
 if(pipe(pipe_fds)==-1)
   return(0);

 Cdrfifo_destroy(&(track->fifo),0);
 if(flag&2) {
   ret= Cdrfifo_attach_follow_up_fds(previous_fifo,source_fd,pipe_fds[1],0);
   if(ret<=0)
     return(ret);
   track->ff_fifo= previous_fifo;
   track->ff_idx= ret-1;
 } else {

   /* >>> ??? obtain track sector size and use instead of 2048 ? */

   ret= Cdrfifo_new(&ff,source_fd,pipe_fds[1],2048,track->fifo_size, flag & 1);
   if(ret<=0)
     return(ret);
   if(previous_fifo!=NULL)
     Cdrfifo_attach_peer(previous_fifo,ff,0);
   track->fifo= track->ff_fifo= ff;
   track->ff_idx= -1;
 }
 track->fifo_outlet_fd= pipe_fds[0];

 if((track->extracting_container || !(flag&4)) && track->fixed_size>0)
   Cdrfifo_set_fd_in_limit(track->ff_fifo,track->fixed_size,track->ff_idx,0);

 if(flag&1)
   printf(
        "cdrskin_debug: track %d fifo replaced source_address '%s' by '#%d'\n",
        track->trackno+1,track->source_path,track->fifo_outlet_fd);
 sprintf(track->source_path,"#%d",track->fifo_outlet_fd);
 track->source_fd= track->fifo_outlet_fd;
 *outlet_fd= track->fifo_outlet_fd;
 return(1);
}

#endif /* ! Cdrskin_no_cdrfifO */


#ifndef Cdrskin_extra_leaN

#ifdef Cdrskin_use_libburn_fifO

/** Read data into the eventual libburn fifo until either fifo_start_at bytes
    are read (-1 = no limit), it is full or or the data source is exhausted.
    @return <=0 error, 1 success
*/
int Cdrtrack_fill_libburn_fifo(struct CdrtracK *track, int fifo_start_at,
                               int flag)
{
 int ret, bs= 32 * 1024;
 int buffer_size, buffer_free;
 double data_image_size;
 char buf[64 * 1024], *buffer_text;

 if(fifo_start_at == 0)
   return(2);
 if(track->libburn_fifo == NULL)
   return(2);

 if(fifo_start_at>0 && fifo_start_at<track->fifo_size)
   printf(
      "cdrskin: NOTE : Input buffer will be initially filled up to %d bytes\n",
      fifo_start_at);
 printf("Waiting for reader process to fill input buffer ... ");
 fflush(stdout);
 ret= burn_fifo_fill(track->libburn_fifo, fifo_start_at,
                     (fifo_start_at == -1));
 if(ret < 0)
   return(0);

/** Ticket 55: check fifos for input, throw error on 0-bytes from stdin
    @return <=0 abort run, 1 go on with burning
*/
 ret= burn_fifo_inquire_status(track->libburn_fifo, &buffer_size,
                               &buffer_free, &buffer_text);
 if(track->is_from_stdin) {
   if(ret<0 || buffer_size <= buffer_free) {
     fprintf(stderr,"\ncdrskin: FATAL : (First track) fifo did not read a single byte from stdin\n");
     return(0);
   }
 }

 /* Try to obtain ISO 9660 Volume Descriptors and size from fifo.
    Not an error if there is no ISO 9660. */
 if(track->iso_fs_descr != NULL)
   free(track->iso_fs_descr);
 track->iso_fs_descr = NULL;
 if(buffer_size - buffer_free >= 64 * 1024) {
   ret= burn_fifo_peek_data(track->libburn_fifo, buf, 64 * 1024, 0);
   if(ret == 1) {
     track->iso_fs_descr = calloc(1, bs);
     if(track->iso_fs_descr == NULL)
       return(-1);
     memcpy(track->iso_fs_descr, buf + bs, bs);
     ret= Scan_for_iso_size((unsigned char *) buf + bs, &data_image_size, 0);
     if(ret > 0)
       track->data_image_size= data_image_size;
   }
 }
 return(1);
}

#endif /* Cdrskin_use_libburn_fifO */


#ifdef Cdrskin_no_cdrfifO

int Cdrtrack_fill_fifo(struct CdrtracK *track, int fifo_start_at, int flag)
{
	return(Cdrtrack_fill_libburn_fifo(track, fifo_start_at, 0));
}

#else /* Cdrskin_no_cdrfifO */

int Cdrtrack_fill_fifo(struct CdrtracK *track, int fifo_start_at, int flag)
{
 int ret,buffer_fill,buffer_space;
 double data_image_size;

 if(fifo_start_at==0)
   return(2);
 if(track->fifo==NULL) {
#ifdef Cdrskin_use_libburn_fifO
   ret= Cdrtrack_fill_libburn_fifo(track, fifo_start_at, 0);
   return(ret);
#else
   return(2);
#endif
 }
 if(fifo_start_at>0 && fifo_start_at<track->fifo_size)
   printf(
      "cdrskin: NOTE : Input buffer will be initially filled up to %d bytes\n",
      fifo_start_at);
 printf("Waiting for reader process to fill input buffer ... ");
 fflush(stdout);
 ret= Cdrfifo_fill(track->fifo,fifo_start_at,0);
 if(ret<=0)
   return(ret);

/** Ticket 55: check fifos for input, throw error on 0-bytes from stdin
    @return <=0 abort run, 1 go on with burning
*/
 if(track->is_from_stdin) {
   ret= Cdrfifo_get_buffer_state(track->fifo,&buffer_fill,&buffer_space,0);
   if(ret<0 || buffer_fill<=0) {
     fprintf(stderr,"\ncdrskin: FATAL : (First track) fifo did not read a single byte from stdin\n");
     return(0);
   }
 }
 ret= Cdrfifo_get_iso_fs_size(track->fifo,&data_image_size,0);
 if(ret>0)
   track->data_image_size= data_image_size;
 if(track->iso_fs_descr!=NULL)
   free((char *) track->iso_fs_descr);
 Cdrfifo_adopt_iso_fs_descr(track->fifo,&(track->iso_fs_descr),0);
 return(1);
}

#endif /* ! Cdrskin_no_cdrfifO */
#endif /* ! Cdrskin_extra_leaN */


int Cdrtrack_set_indice(struct CdrtracK *track, int flag)
{
 int idx= 1, adr, prev_adr= -1, ret;
 char *cpt, *ept;

 if(track->sao_pregap >= 0) {
   ret= burn_track_set_pregap_size(track->libburn_track, track->sao_pregap, 0);
   if(ret <= 0)
     return(ret);
 }
 if(track->sao_postgap >= 0) {
   ret= burn_track_set_postgap_size(track->libburn_track, track->sao_postgap,
                                    0);
   if(ret <= 0)
     return(ret);
 }

 if(track->index_string == NULL)
   return(2);

 for(ept= cpt= track->index_string; ept != NULL; cpt= ept + 1) {
     ept= strchr(cpt, ',');
     if(ept != NULL)
       *ept= 0;
     adr= -1;
     sscanf(cpt, "%d", &adr);
     if(adr < 0) {
       fprintf(stderr, "cdrskin: SORRY : Bad address number with index=\n");
       return(0);
     }
     if(idx == 1 && adr != 0) {
       fprintf(stderr,
               "cdrskin: SORRY : First address number of index= is not 0\n");
       return(0);
     }
     if(idx > 1 && adr < prev_adr) {
       fprintf(stderr,
               "cdrskin: SORRY : Backward address number with index=\n");
       return(0);
     }
     ret= burn_track_set_index(track->libburn_track, idx, adr, 0);
     if(ret <= 0)
       return(ret);
     prev_adr= adr;
     if(ept != NULL)
       *ept= ',';
     idx++;
 }

 return(1);
}     

/** Create a corresponding libburn track object and add it to the libburn
    session. This may change the trackno index set by Cdrtrack_new().
*/
int Cdrtrack_add_to_session(struct CdrtracK *track, int trackno,
                            struct burn_session *session, int flag)
/*
 bit0= debugging verbosity
 bit1= apply padding hack (<<< should be unused for now)
 bit2= permission to use O_DIRECT (if enabled at compile time)
*/
{
 struct burn_track *tr;
 struct burn_source *src= NULL, *fd_src= NULL;
 double padding,lib_padding;
 int ret,sector_pad_up;
 double fixed_size;
 int source_fd;

 track->trackno= trackno;
 tr= burn_track_create();
 if(tr == NULL)
   {ret= -1; goto ex;}
 track->libburn_track= tr;
 track->libburn_track_is_own= 1;

 /* Note: track->track_type may get set in here */
 if(track->source_fd==-1) {
   ret= Cdrtrack_open_source_path(track, &source_fd, flag & (4 | 1));
   if(ret<=0)
     goto ex;
 }

 padding= 0.0;
 sector_pad_up= track->sector_pad_up;
 if(track->padding>0) {
   if(track->set_by_padsize || track->track_type!=BURN_AUDIO)
     padding= track->padding;
   else
     sector_pad_up= 1;
 }
 if(flag&2)
   lib_padding= 0.0;
 else
   lib_padding= padding;
 if(flag&1) {
   if(sector_pad_up) {
     ClN(fprintf(stderr,"cdrskin_debug: track %d telling burn_track_define_data() to pad up last sector\n",trackno+1));
   }
   if(lib_padding>0 || !sector_pad_up) {
     ClN(fprintf(stderr,
 "cdrskin_debug: track %d telling burn_track_define_data() to pad %.f bytes\n",
           trackno+1,lib_padding));
   }
 }
 burn_track_define_data(tr,0,(int) lib_padding,sector_pad_up,
                        track->track_type | track->mode_modifiers);
 burn_track_set_default_size(tr, (off_t) track->tao_to_sao_tsize);
 burn_track_set_byte_swap(tr,
                   (track->track_type==BURN_AUDIO && track->swap_audio_bytes));
 if(!(track->cdxa_conversion & (1 << 31)))
   burn_track_set_cdxa_conv(tr, track->cdxa_conversion & 0x7fffffff);

 fixed_size= track->fixed_size;
 if((flag&2) && track->padding>0) {
   if(flag&1)
     ClN(fprintf(stderr,"cdrskin_debug: padding hack : %.f + %.f = %.f\n",
                 track->fixed_size,track->padding,
                 track->fixed_size+track->padding));
   fixed_size+= track->padding;
 }
 src= burn_fd_source_new(track->source_fd,-1,(off_t) fixed_size);

#ifdef Cdrskin_use_libburn_fifO

 if(src != NULL && track->fifo == NULL) {
   int fifo_enabled, fifo_size, fifo_start_at, chunksize, chunks;
   int Cdrskin_get_fifo_par(struct CdrskiN *skin, int *fifo_enabled,
                            int *fifo_size, int *fifo_start_at, int flag);

   Cdrskin_get_fifo_par(track->boss, &fifo_enabled, &fifo_size, &fifo_start_at,
                        0);

   if(track->track_type == BURN_AUDIO)
     chunksize= 2352;
   else if (track->cdxa_conversion == 1)
     chunksize= 2056;
   else
     chunksize= 2048;
   chunks= fifo_size / chunksize;
   if(chunks > 1 && fifo_enabled) {
     fd_src= src;
     src= burn_fifo_source_new(fd_src, chunksize, chunks,
                               (chunksize * chunks >= 128 * 1024));
     if((flag & 1) || src == NULL)
       fprintf(stderr, "cdrskin_DEBUG: %s libburn fifo of %d bytes\n",
               src != NULL ? "installed" : "failed to install",
               chunksize * chunks);
     track->libburn_fifo= src;
     if(src == NULL) {
       src= fd_src;
       fd_src= NULL;
     }
   }
 }

#endif /* Cdrskin_use_libburn_fifO */

 if(src==NULL) {
   fprintf(stderr,
           "cdrskin: FATAL : Could not create libburn data source object\n");
   {ret= 0; goto ex;}
 }
 if(burn_track_set_source(tr,src)!=BURN_SOURCE_OK) {
   fprintf(stderr,"cdrskin: FATAL : libburn rejects data source object\n");
   {ret= 0; goto ex;}
 }
 ret= Cdrtrack_set_indice(track, 0);
 if(ret <= 0)
   goto ex;

 burn_session_add_track(session,tr,BURN_POS_END);
 ret= 1;
ex:
 if(fd_src!=NULL)
   burn_source_free(fd_src);
 if(src!=NULL)
   burn_source_free(src);
 return(ret);
}


/** Release libburn track information after a session is done */
int Cdrtrack_cleanup(struct CdrtracK *track, int flag)
{
 if(track->libburn_track==NULL)
   return(0);
 if(track->libburn_track_is_own)
   burn_track_free(track->libburn_track);
 track->libburn_track= NULL;
 return(1);
}


int Cdrtrack_ensure_padding(struct CdrtracK *track, int flag)
/*
flag:
 bit0= debugging verbosity
*/
{
 if(track->track_type!=BURN_AUDIO)
   return(2);
 if(flag&1)
   fprintf(stderr,"cdrskin_debug: enforcing -pad on last -audio track\n");
 track->sector_pad_up= 1;
 return(1);
}


int Cdrtrack_get_sectors(struct CdrtracK *track, int flag)
{
 return(burn_track_get_sectors(track->libburn_track));
}


#ifndef Cdrskin_no_cdrfifO

/** Try to read bytes from the track's fifo outlet and eventually discard
    them. Not to be called unless the track is completely written.
*/
int Cdrtrack_has_input_left(struct CdrtracK *track, int flag)
{
 int ready,ret;
 char buf[2];

 if(track->fifo_outlet_fd<=0)
   return(0);
 ready= Wait_for_input(track->fifo_outlet_fd, 0, 0);
 if(ready<=0)
   return(0);
 ret= read(track->fifo_outlet_fd,buf,1);
 if(ret>0)
   return(1);
 return(0);
}

#endif /* ! Cdrskin_no_cdrfifO */
 

/* --------------------------------------------------------------------- */

/** The list of startup file names */
#define Cdrpreskin_rc_nuM 4

static char Cdrpreskin_sys_rc_nameS[Cdrpreskin_rc_nuM][80]= {
 "/etc/default/cdrskin",
 "/etc/opt/cdrskin/rc",
 "/etc/cdrskin/cdrskin.conf",
 "placeholder for $HOME/.cdrskinrc"
};


/** A structure which bundles several parameters for creation of the CdrskiN
    object. It finally becomes a managed subordinate of the CdrskiN object. 
*/
struct CdrpreskiN {

 /* to be transfered into skin */
 int verbosity;
 char queue_severity[81];
 char print_severity[81];

 /** Whether to wait for available standard input data before touching drives*/
 int do_waiti;

 /** Stores eventually given absolute device address before translation */
 char raw_device_adr[Cdrskin_adrleN];

 /** Stores an eventually given translated absolute device address between
    Cdrpreskin_setup() and Cdrskin_create() .
 */
 char device_adr[Cdrskin_adrleN];

 /** The eventual address translation table */
 struct CdradrtrN *adr_trn;

 /** Memorizes the abort handling mode from presetup to creation of 
     control object. Defined handling modes are:
     0= no abort handling
     1= try to cancel, release, exit (leave signal mode as set by caller)
     2= try to ignore all signals
     3= mode 1 in normal operation, mode 2 during abort handling
     4= mode 1 in normal operation, mode 0 during abort handling
    -1= install abort handling 1 only in Cdrskin_burn() after burning started
 */
 int abort_handler;

 /** Whether to allow getuid()!=geteuid() */
 int allow_setuid;

 /** Whether to allow user provided addresses like #4 */
 int allow_fd_source;

 /** Whether to support media types which are implemented but yet untested */
 int allow_untested_media;

 /** Whether to allow libburn pseudo-drives "stdio:<path>" .
     0=forbidden, 1=seems ok,
     2=potentially forbidden (depends on uid, euid, file type)
 */
 int allow_emulated_drives;

 /** Whether an option is given which needs a full bus scan */
 int no_whitelist;

 /** Whether the translated device address shall not follow softlinks, device
     clones and SCSI addresses */
 int no_convert_fs_adr;

 /** Whether Bus,Target,Lun addresses shall be converted literally as old
     Pseudo SCSI-Adresses. New default is to use (possibly system emulated)
     real SCSI addresses via burn_drive_convert_scsi_adr() and literally
     emulated and cdrecord-incompatible ATA: addresses. */
 int old_pseudo_scsi_adr;

 /** Whether to omit bus scanning */
 int do_not_scan;

 /** Whether bus scans shall exit!=0 if no drive was found */
 int scan_demands_drive;

 /** Whether to abort when a busy drive is encountered during bus scan */
 int abort_on_busy_drive;

 /** Linux specific : Whether to try to avoid collisions when opening drives */
 int drive_exclusive;

 /** Linux specific : Whether to obtain an exclusive drive lock via fcntl() */
 int drive_fcntl_f_setlk;

 /** Linux specific : Device file address family to use :
     0=default , 1=sr , 2=scd , 4=sg */
 int drive_scsi_dev_family;
 

 /** Whether to try to wait for unwilling drives to become willing to open */
 int drive_blocking;

 /** Explicit write mode option is determined before skin processes
     any track arguments */
 char write_mode_name[80];

#ifndef Cdrskin_extra_leaN

 /** List of startupfiles */
 char rc_filenames[Cdrpreskin_rc_nuM][Cdrskin_strleN];
 int rc_filename_count;

 /** Non-argument options from startupfiles */
 int pre_argc;
 char **pre_argv;
 int *pre_argidx;
 int *pre_arglno;

#endif /* ! Cdrskin_extra_leaN */

 /* The eventual name of a program to be executed if demands_cdrecord_caps
    is >0 and demands_cdrskin_caps is <=0
 */
 char fallback_program[Cdrskin_strleN];
 int demands_cdrecord_caps;
 int demands_cdrskin_caps;

 int result_fd;

};


/** Create a preliminary cdrskin program run control object. It will become
    part of the final control object.
    @param preskin Returns pointer to resulting
    @param flag Bitfield for control purposes: unused yet
    @return <=0 error, 1 success
*/
int Cdrpreskin_new(struct CdrpreskiN **preskin, int flag)
{
 struct CdrpreskiN *o;
 int i;

 (*preskin)= o= TSOB_FELD(struct CdrpreskiN,1);
 if(o==NULL)
   return(-1);

 o->verbosity= 0;
 strcpy(o->queue_severity,"NEVER");
 strcpy(o->print_severity,"SORRY");
 o->do_waiti= 0;
 o->raw_device_adr[0]= 0;
 o->device_adr[0]= 0;
 o->adr_trn= NULL;
 o->abort_handler= 3;
 o->allow_setuid= 0;
 o->allow_fd_source= 0;
 o->allow_untested_media= 0;
 o->allow_emulated_drives= 0;
 o->no_whitelist= 0;
 o->no_convert_fs_adr= 0;
 o->old_pseudo_scsi_adr= 0;
 o->do_not_scan= 0;
 o->scan_demands_drive= 0;
 o->abort_on_busy_drive= 0;
 o->drive_exclusive= 1;
 o->drive_fcntl_f_setlk= 1;
 o->drive_scsi_dev_family= 0;
 o->drive_blocking= 0;
 strcpy(o->write_mode_name,"DEFAULT");

#ifndef Cdrskin_extra_leaN
 o->rc_filename_count= Cdrpreskin_rc_nuM;
 for(i=0;i<o->rc_filename_count-1;i++)
   strcpy(o->rc_filenames[i],Cdrpreskin_sys_rc_nameS[i]);
 o->rc_filenames[o->rc_filename_count-1][0]= 0;
 o->pre_argc= 0;
 o->pre_argv= NULL;
 o->pre_argidx= NULL;
 o->pre_arglno= NULL;
#endif /* ! Cdrskin_extra_leaN */

 o->fallback_program[0]= 0;
 o->demands_cdrecord_caps= 0;
 o->demands_cdrskin_caps= 0;
 o->result_fd = -1;
 return(1);
}


int Cdrpreskin_destroy(struct CdrpreskiN **preskin, int flag)
{
 struct CdrpreskiN *o;

 o= *preskin;
 if(o==NULL)
   return(0);

#ifndef Cdrskin_extra_leaN
 if((o->pre_arglno)!=NULL)
   free((char *) o->pre_arglno);
 if((o->pre_argidx)!=NULL)
   free((char *) o->pre_argidx);
 if(o->pre_argc>0 && o->pre_argv!=NULL)
   Sfile_destroy_argv(&(o->pre_argc),&(o->pre_argv),0);
 Cdradrtrn_destroy(&(o->adr_trn),0);
#endif /* ! Cdrskin_extra_leaN */

 free((char *) o);
 *preskin= NULL;
 return(1);
}


int Cdrpreskin_set_severities(struct CdrpreskiN *preskin, char *queue_severity,
                              char *print_severity, int flag)
{
 if(queue_severity!=NULL)
   strcpy(preskin->queue_severity,queue_severity);
 if(print_severity!=NULL)
   strcpy(preskin->print_severity,print_severity);
 burn_msgs_set_severities(preskin->queue_severity, preskin->print_severity,
                          "cdrskin: ");
 return(1);
}
 

int Cdrpreskin_initialize_lib(struct CdrpreskiN *preskin, int flag)
{
 int ret, major, minor, micro;

 /* Needed are at least 44 bits in signed type off_t .
    This is a popular mistake in configuration or compilation.
 */
 if(sizeof(off_t) < 6) {
   fprintf(stderr,
"\ncdrskin: FATAL : Compile time misconfiguration. sizeof(off_t) too small.\n"
          );
   return(0);
 }

/* This is the minimum requirement of cdrskin towards the libburn header
   at compile time.
   It gets compared against the version macros in libburn/libburn.h :
     burn_header_version_major
     burn_header_version_minor
     burn_header_version_micro
   If the header is too old then the following code shall cause failure of
   cdrskin compilation rather than to allow production of a program with
   unpredictable bugs or memory corruption.
   The compiler message supposed to appear in this case is:
      error: 'LIBBURN_MISCONFIGURATION' undeclared (first use in this function)
      error: 'INTENTIONAL_ABORT_OF_COMPILATION__HEADERFILE_libburn_dot_h_TOO_OLD__SEE_cdrskin_dot_c' undeclared (first use in this function)
      error: 'LIBBURN_MISCONFIGURATION_' undeclared (first use in this function)
*/

/* The indendation is an advise of man gcc to help old compilers ignoring */
 #if Cdrskin_libburn_majoR > burn_header_version_major
 #define Cdrskin_libburn_dot_h_too_olD 1
 #endif
 #if Cdrskin_libburn_majoR == burn_header_version_major && Cdrskin_libburn_minoR > burn_header_version_minor
 #define Cdrskin_libburn_dot_h_too_olD 1
 #endif
 #if Cdrskin_libburn_minoR == burn_header_version_minor && Cdrskin_libburn_micrO > burn_header_version_micro
 #define Cdrskin_libburn_dot_h_too_olD 1
 #endif

#ifdef Cdrskin_libburn_dot_h_too_olD
LIBBURN_MISCONFIGURATION = 0;
INTENTIONAL_ABORT_OF_COMPILATION__HEADERFILE_libburn_dot_h_TOO_OLD__SEE_cdrskin_dot_c = 0;
LIBBURN_MISCONFIGURATION_ = 0;
#endif

 ret= burn_initialize();
 if(ret==0) {
   fprintf(stderr,"cdrskin: FATAL : Initialization of libburn failed\n");
   return(0);
 }

 /* This is the runtime check towards eventual dynamically linked libburn.
    cdrskin deliberately does not to allow the library to be older than
    the header file which was seen at compile time. More liberal would be
    to use here Cdrskin_libburn_* instead of burn_header_version_* .
 */
 burn_version(&major, &minor, &micro);
 if(major<burn_header_version_major || 
    (major==burn_header_version_major && (minor<burn_header_version_minor ||
     (minor==burn_header_version_minor && micro<burn_header_version_micro)))) {
   fprintf(stderr,"cdrskin: FATAL : libburn version too old: %d.%d.%d . Need at least: %d.%d.%d .\n",
          major, minor, micro,
          Cdrskin_libburn_majoR, Cdrskin_libburn_minoR, Cdrskin_libburn_micrO);
   return(-1);
 }
 Cdrpreskin_set_severities(preskin,NULL,NULL,0);
 burn_allow_drive_role_4(1);
 return(1);
}


/** Enable queuing of libburn messages or disable and print queue content.
    @param flag Bitfield for control purposes:
                bit0= enable queueing, else disable and print
*/
int Cdrpreskin_queue_msgs(struct CdrpreskiN *o, int flag)
{
/* In cdrskin there is not much sense in queueing library messages.
#ifndef Cdrskin_extra_leaN
#define Cdrskin_debug_libdax_msgS 1
#endif
       It would be done here only for debugging
*/
#ifdef Cdrskin_debug_libdax_msgS

 int ret;
 static char queue_severity[81]= {"NEVER"}, print_severity[81]= {"SORRY"};
 static int queueing= 0;
 char msg[BURN_MSGS_MESSAGE_LEN],msg_severity[81],filler[81];
 int error_code,os_errno,first,i;

 if(flag&1) {
   if(!queueing) {
     strcpy(queue_severity,o->queue_severity);
     strcpy(print_severity,o->print_severity);
   }
   if(o->verbosity>=Cdrskin_verbose_debuG)
     Cdrpreskin_set_severities(o,"DEBUG","NEVER",0);
   else
     Cdrpreskin_set_severities(o,"SORRY","NEVER",0);
   queueing= 1;
   return(1);
 }

 if(queueing)
   Cdrpreskin_set_severities(o,queue_severity,print_severity,0);
 queueing= 0;

 for(first= 1; ; first= 0) {
   ret= burn_msgs_obtain("ALL",&error_code,msg,&os_errno,msg_severity);
   if(ret==0)
 break;
   if(ret<0) {
     fprintf(stderr,
           "cdrskin: NOTE : Please inform libburn-hackers@pykix.org about:\n");
     fprintf(stderr,
           "cdrskin:        burn_msgs_obtain() returns %d\n",ret);
 break;
   }
   if(first)
     fprintf(stderr,
"cdrskin: -------------------- Messages from Libburn ---------------------\n");
   for(i=0;msg_severity[i]!=0;i++)
     filler[i]= ' ';
   filler[i]= 0;
   fprintf(stderr,"cdrskin: %s : %s\n",msg_severity,msg);
   if(strcmp(msg_severity,"DEBUG")!=0 && os_errno!=0)
     fprintf(stderr,"cdrskin: %s   ( errno=%d  '%s')\n",
                    filler,os_errno,strerror(os_errno));
 }
 if(first==0)
   fprintf(stderr,
"cdrskin: ----------------------------------------------------------------\n");

#endif /* Cdrskin_debug_libdax_msgS */

 return(1);
}


/** Evaluate whether the user would be allowed in any case to use device_adr
    as pseudo-drive */
int Cdrpreskin__allows_emulated_drives(char *device_adr, char reason[4096],
                                      int flag)
{
 struct stat stbuf;

 reason[0]= 0;
 if(device_adr[0]) {
   if(strcmp(device_adr,"/dev/null")==0)
     return(1);
   strcat(reason,"File object is not /dev/null. ");
 }

 if(getuid()!=geteuid()) {
   strcat(reason,"UID and EUID differ");
   return(0);
 }
 if(getuid()!=0)
   return(1);

 strcat(reason,"UID is 0. ");
 /* Directory must be owned by root and write protected against any others*/
 if(lstat("/root/cdrskin_permissions",&stbuf)==-1 || !S_ISDIR(stbuf.st_mode)) {
   strcat(reason, "No directory /root/cdrskin_permissions exists");
   return(0);
 }
 if(stbuf.st_uid!=0) {
   strcat(reason, "Directory /root/cdrskin_permissions not owned by UID 0");
   return(0);
 }
 if(stbuf.st_mode & (S_IWGRP | S_IWOTH)) {
   strcat(reason,
  "Directory /root/cdrskin_permissions has w-permission for group or others");
   return(0);
 }
 if(stat("/root/cdrskin_permissions/allow_emulated_drives",&stbuf)==-1) {
   strcat(reason,
          "No file /root/cdrskin_permissions/allow_emulated_drives exists");
   return(0);
 }
 reason[0]= 0;
 return(1);
}


int Cdrpreskin_consider_normal_user(int flag)
{
 fprintf(stderr,
    "cdrskin: HINT : Consider to allow rw-access to the writer devices and\n");
 fprintf(stderr,
    "cdrskin: HINT : to run cdrskin under your normal user identity.\n");
 return(1);
}


/* Start the fallback program as replacement of the cdrskin run.
   @param flag bit0=do not report start command
*/
int Cdrpreskin_fallback(struct CdrpreskiN *preskin, int argc, char **argv,
                        int flag)
{
 char **hargv= NULL;
 int i, wp= 1;
 char *ept, *upt;

 if(preskin->fallback_program[0] == 0)
   return(1);
 if(getuid()!=geteuid() && !preskin->allow_setuid) {
   fprintf(stderr,
     "cdrskin: SORRY : uid and euid differ. Will not start external fallback program.\n");
   Cdrpreskin_consider_normal_user(0);
   fprintf(stderr,
    "cdrskin: HINT : Option  --allow_setuid  disables this safety check.\n");
   goto failure;
 }
 if(!(flag&1)) {
   fprintf(stderr,"cdrskin: --------------------------------------------------------------------\n");
   fprintf(stderr,"cdrskin: Starting fallback program:\n");
 }
 hargv= TSOB_FELD(char *,argc+1);
 if(hargv==NULL)
   goto failure;
 hargv[0]= strdup(preskin->fallback_program);
 if(argv[0]==NULL)
   goto failure; 
 if(!(flag&1))
   fprintf(stderr,"         %s", hargv[0]);
 for(i= 1; i<argc; i++) {
   /* filter away all cdrskin specific options :  --?*  and  *_*=*   */
   if(argv[i][0]=='-' && argv[i][1]=='-' && argv[i][2])
 continue;
   ept= strchr(argv[i],'=');
   if(ept!=NULL) {
     upt= strchr(argv[i],'_');
     if(upt!=NULL && upt<ept)
 continue;
   }
   hargv[wp]= strdup(argv[i]);
   if(hargv[wp]==NULL)
     goto failure;
   if(!(flag&1))
     fprintf(stderr,"  %s", hargv[wp]);
   wp++;
 }
 hargv[wp]= NULL;
 if(!(flag&1)) {
   fprintf(stderr,"\n");
   fprintf(stderr,"cdrskin: --------------------------------------------------------------------\n");
 }
 execvp(hargv[0], hargv);
failure:;
 fprintf(stderr,"cdrskin: FATAL : Cannot start fallback program '%s'\n",
         preskin->fallback_program);
 fprintf(stderr,"cdrskin: errno=%d  \"%s\"\n",
         errno, (errno > 0 ? strerror(errno) : "unidentified error"));
 exit(15);
}


/** Convert a cdrecord-style device address into a libburn device address or
    into a libburn drive number. It depends on the "scsibus" number of the
    cdrecord-style address which kind of libburn address emerges:
      bus=0 : drive number , bus=1 : /dev/sgN , bus=2 : /dev/hdX
    (This call intentionally has no CdrpreskiN argument)
    @param flag Bitfield for control purposes: 
                bit0= old_pseudo_scsi_adr
    @return 1 success, 0=no recognizable format, -1=severe error,
            -2 could not find scsi device, -3 address format error
*/
int Cdrpreskin__cdrecord_to_dev(char *adr, char device_adr[Cdrskin_adrleN],
                                int *driveno, int flag)
{
 int comma_seen= 0,digit_seen= 0,busno= 0,k,lun_no= -1;

 *driveno= -1;
 device_adr[0]= 0;
 if(strlen(adr)==0)
   return(0);
 if(strncmp(adr,"stdio:",6)==0)
   return(0);

 /* read the trailing numeric string as device address code */
 /* accepts "1" , "0,1,0" , "ATA:0,1,0" , ... */
 for(k= strlen(adr)-1;k>=0;k--) {
   if(adr[k]==',' && !comma_seen) {
     sscanf(adr+k+1,"%d",&lun_no);
     comma_seen= 1;
     digit_seen= 0;
 continue;
   }
   if(adr[k]<'0' || adr[k]>'9')
 break;
   digit_seen= 1;
 }
 if(!digit_seen) {
   k= strlen(adr)-1;
   if(adr[k]==':' || (adr[k]>='A' && adr[k]<='Z')) {/* empty prefix ? */
     *driveno= 0;
     return(1);
   }
   return(0);
 }
 sscanf(adr+k+1,"%d",driveno);

 digit_seen= 0;
 if(k>0) if(adr[k]==',') {
   for(k--;k>=0;k--) {
     if(adr[k]<'0' || adr[k]>'9')
   break;
     digit_seen= 1;
   }
   if(digit_seen) {
     sscanf(adr+k+1,"%d",&busno);
     if(flag&1) {
       /* look for symbolic bus :  1=/dev/sgN  2=/dev/hdX */
       if(busno==1) {
         sprintf(device_adr,"/dev/sg%d",*driveno);
       } else if(busno==2) {
         sprintf(device_adr,"/dev/hd%c",'a'+(*driveno));
       } else if(busno!=0) {
         fprintf(stderr,
  "cdrskin: FATAL : dev=[Prefix:]Bus,Target,Lun expects Bus out of {0,1,2}\n");
         return(-3);
       }
     } else {
       if(busno<0) {
         fprintf(stderr,
     "cdrskin: FATAL : dev=[Prefix:]Bus,Target,Lun expects Bus number >= 0\n");
         return(-3);
       }
       if(busno>=1000) {
         busno-= 1000;
         goto ata_bus;
       } else if((strncmp(adr,"ATA",3)==0 && (adr[3]==0 || adr[3]==':')) ||
                 (strncmp(adr,"ATAPI",5)==0 && (adr[5]==0 || adr[5]==':'))) {
ata_bus:;
         if(busno>12 || (*driveno)<0 || (*driveno)>1) {
           fprintf(stderr,
"cdrskin: FATAL : dev=ATA:Bus,Target,Lun expects Bus {0..12}, Target {0,1}\n");
           return(-3);
         }
         sprintf(device_adr,"/dev/hd%c",'a'+(2*busno)+(*driveno));

       } else {
         int ret;

         ret= burn_drive_convert_scsi_adr(busno,-1,-1,*driveno,lun_no,
                                          device_adr);
         if(ret==0) {
           fprintf(stderr,
   "cdrskin: FATAL : Cannot find /dev/* with Bus,Target,Lun = %d,%d,%d\n",
                   busno,*driveno,lun_no);
           fprintf(stderr,
   "cdrskin: HINT : This drive may be in use by another program currently\n");
           return(-2);
         } else if(ret<0)
           return(-1);
         return(1);
       }
     }
   } 
 }
 return(1);
}


/** Set the eventual output fd for the result of Cdrskin_msinfo()
*/
int Cdrpreskin_set_result_fd(struct CdrpreskiN *o, int result_fd, int flag)
{
 o->result_fd= result_fd;
 return(1);
}


#ifndef Cdrskin_extra_leaN

/** Load content startup files into preskin cache */
int Cdrpreskin_read_rc(struct CdrpreskiN *o, char *progname, int flag)
{
 int ret,i;
 char **filenames_v;

 filenames_v= TSOB_FELD(char *, o->rc_filename_count+1);
 if(filenames_v==NULL)
   return(-1);
 for(i=0;i<o->rc_filename_count;i++)
   filenames_v[i]= o->rc_filenames[i];
 Sfile_home_adr_s(".cdrskinrc",o->rc_filenames[o->rc_filename_count-1],
                  Cdrskin_strleN,0);
 ret= Sfile_multi_read_argv(progname,filenames_v,o->rc_filename_count,
                            &(o->pre_argc),&(o->pre_argv),
                            &(o->pre_argidx),&(o->pre_arglno),4);
 free((char *) filenames_v);
 return(ret);
}

#endif /* ! Cdrskin_extra_leaN */


/** Interpret those arguments which do not need libburn or which influence the
    startup of libburn and/or the creation of the CdrskiN object. This is run
    before libburn gets initialized and before Cdrskin_new() is called.
    Options which need libburn or a CdrskiN object are processed in a different
    function named Cdrskin_setup().
    @param flag Bitfield for control purposes: 
                bit0= do not finalize setup
                bit1= do not read and interpret rc files
    @return <=0 error, 1 success , 2 end program run with exit value 0
*/
int Cdrpreskin_setup(struct CdrpreskiN *o, int argc, char **argv, int flag)
/*
return:
 <=0 error
   1 ok
   2 end program run (--help)
*/
{
 int i,ret;
 char *value_pt, reason[4096], *argpt;

#ifndef Cdrskin_extra_leaN
 if(argc>1) {
   if(strcmp(argv[1],"--no_rc")==0 || strcmp(argv[1],"-version")==0 ||
      strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-help")==0 ||
      strncmp(argv[1], "textfile_to_v07t=", 17) == 0 ||
      strncmp(argv[1], "-textfile_to_v07t=", 18) == 0)
     flag|= 2;
 }
 if(!(flag&2)) {
   ret= Cdrpreskin_read_rc(o,argv[0],0);
   if(ret<0)
     return(-1);
   if(o->pre_argc>1) {
     ret= Cdrpreskin_setup(o,o->pre_argc,o->pre_argv,flag|1|2);
     if(ret<=0)
       return(ret);
     /* ??? abort on ret==2 ? */
   }
 }
#endif

 if(argc==1) {
   fprintf(stderr,"cdrskin: SORRY : No options given. Try option  --help\n");
   return(0);
 }

 /* The two predefined fallback personalities are triggered by the progname */
 value_pt= strrchr(argv[0],'/');
 if(value_pt==NULL)
   value_pt= argv[0];
 else
   value_pt++;
 if(strcmp(value_pt,"unicord")==0)
   strcpy(o->fallback_program,"cdrecord");
 else if(strcmp(value_pt,"codim")==0)
   strcpy(o->fallback_program,"wodim");

 for (i= 1;i<argc;i++) {

   argpt= argv[i];
   if (strncmp(argpt, "--", 2) == 0 && strlen(argpt) > 3)
     argpt++;

   if(strcmp(argv[i],"--abort_handler")==0) {
     o->abort_handler= 3;

   } else if(strcmp(argv[i],"--allow_emulated_drives")==0) {
     if(Cdrpreskin__allows_emulated_drives("",reason,0)<=0) {
       fprintf(stderr,"cdrskin: WARNING : %s.\n",reason);
       fprintf(stderr,
     "cdrskin: WARNING : Only /dev/null will be available with \"stdio:\".\n");
       Cdrpreskin_consider_normal_user(0);
       o->allow_emulated_drives= 2;
     } else
       o->allow_emulated_drives= 1;

   } else if(strcmp(argv[i],"--allow_setuid")==0) {
     o->allow_setuid= 1;

   } else if(strcmp(argv[i],"--allow_untested_media")==0) {
     o->allow_untested_media= 1;

   } else if(strcmp(argpt, "blank=help") == 0 ||
             strcmp(argpt, "-blank=help") == 0) {

#ifndef Cdrskin_extra_leaN

     fprintf(stderr,"Blanking options:\n");
     fprintf(stderr,"\tall\t\tblank the entire disk\n");
     fprintf(stderr,"\tdisc\t\tblank the entire disk\n");
     fprintf(stderr,"\tdisk\t\tblank the entire disk\n");
     fprintf(stderr,"\tfast\t\tminimally blank the entire disk\n");
     fprintf(stderr,"\tminimal\t\tminimally blank the entire disk\n");
     fprintf(stderr,
         "\tas_needed\tblank or format media to make it ready for (re-)use\n");
     fprintf(stderr,
          "\tdeformat_sequential\t\tfully blank, even formatted DVD-RW\n");
     fprintf(stderr,
          "\tdeformat_sequential_quickest\tminimally blank, even DVD-RW\n");
     fprintf(stderr,
          "\tformat_if_needed\t\tmake overwriteable if needed and possible\n");
     fprintf(stderr,
        "\tformat_overwrite\t\tformat a DVD-RW to \"Restricted Overwrite\"\n");
     fprintf(stderr,
    "\tformat_overwrite_quickest\tto \"Restricted Overwrite intermediate\"\n");
     fprintf(stderr,
          "\tformat_overwrite_full\t\tfull-size format a DVD-RW or DVD+RW\n");
     fprintf(stderr,
          "\tformat_defectmgt[_max|_min|_none]\tformat DVD-RAM or BD-R[E]\n");
     fprintf(stderr,
         "\tformat_defectmgt[_cert_on|_cert_off]\tcertification slow|quick\n");
     fprintf(stderr,
          "\tformat_defectmgt_payload_<size>\tformat DVD-RAM or BD-R[E]\n");
     fprintf(stderr,
        "\tformat_by_index_<number>\t\tformat by index from --list_formats\n");

#else /* ! Cdrskin_extra_leaN */

     goto see_cdrskin_eng_html;

#endif /* ! Cdrskin_extra_leaN */

     if(argc==2)
       {ret= 2; goto final_checks;}

   } else if(strcmp(argv[i],"--bragg_with_audio")==0) {
     /* OBSOLETE 0.2.3 */;

   } else if(strcmp(argv[i],"--demand_a_drive")==0) {
     o->scan_demands_drive= 1;
     o->demands_cdrskin_caps= 1;

   } else if(strcmp(argv[i],"--devices") == 0 ||
             strcmp(argv[i],"--device_links") == 0) {
     o->no_whitelist= 1;
     o->demands_cdrskin_caps= 1;

   } else if(strncmp(argv[i],"dev_translation=",16)==0) {
     o->demands_cdrskin_caps= 1;

#ifndef Cdrskin_extra_leaN

     if(o->adr_trn==NULL) {
       ret= Cdradrtrn_new(&(o->adr_trn),0);
       if(ret<=0)
         goto no_adr_trn_mem;
     }
     if(argv[i][16]==0) {
       fprintf(stderr,
         "cdrskin: FATAL : dev_translation= : missing separator character\n");
       return(0);
     }
     ret= Cdradrtrn_add(o->adr_trn,argv[i]+17,argv[i]+16,1);
     if(ret==-2) {
no_adr_trn_mem:;
       fprintf(stderr,
           "cdrskin: FATAL : address_translation= : cannot allocate memory\n");
     } else if(ret==-1)
       fprintf(stderr,
             "cdrskin: FATAL : address_translation= : table full (%d items)\n",
               Cdradrtrn_leN);
     else if(ret==0)
       fprintf(stderr,
  "cdrskin: FATAL : address_translation= : no address separator '%c' found\n",
               argv[i][17]);
     if(ret<=0)
       return(0);

#else /* ! Cdrskin_extra_leaN */

     fprintf(stderr,
       "cdrskin: FATAL : dev_translation= is not available in lean version\n");
     return(0);

#endif /* Cdrskin_extra_leaN */


   } else if(strncmp(argpt, "-dev=", 5) == 0) {
     value_pt= argpt + 5;
     goto set_dev;
   } else if(strncmp(argpt, "dev=", 4) == 0) {
     value_pt= argpt + 4;
set_dev:;
     if(strcmp(value_pt,"help")==0) {

#ifndef Cdrskin_extra_leaN

       printf("\nSupported SCSI transports for this platform:\n");
       fflush(stdout);
       if(o->old_pseudo_scsi_adr) {
         fprintf(stderr,"\nTransport name:\t\tlibburn OLD_PSEUDO\n");
         fprintf(stderr,
         "Transport descr.:\tBus0=DriveNum , Bus1=/dev/sgN , Bus2=/dev/hdX\n");
       } else {
         fprintf(stderr,"\nTransport name:\t\tlibburn SCSI\n");
         fprintf(stderr,
            "Transport descr.:\tSCSI Bus,Id,Lun as of operating system\n");
       }
       fprintf(stderr,"Transp. layer ind.:\t\n");
       fprintf(stderr,"Target specifier:\tbus,target,lun\n");
       fprintf(stderr,"Target example:\t\t1,2,0\n");
       fprintf(stderr,"SCSI Bus scanning:\tsupported\n");
       fprintf(stderr,"Open via UNIX device:\tsupported\n");
       if(!o->old_pseudo_scsi_adr) {
         fprintf(stderr,"\nTransport name:\t\tlibburn HD\n");
         fprintf(stderr,
           "Transport descr.:\tLinux specific alias for /dev/hdX\n");
         fprintf(stderr,"Transp. layer ind.:\tATA:\n");
         fprintf(stderr,"Target specifier:\tbus,target,lun\n");
         fprintf(stderr,"Target example:\t\tATA:1,0,0\n");
         fprintf(stderr,"SCSI Bus scanning:\tsupported\n");
         fprintf(stderr,"Open via UNIX device:\tsupported\n");
       }
       if(o->allow_emulated_drives) {
         fprintf(stderr,"\nTransport name:\t\tlibburn on standard i/o\n");
         if(o->allow_emulated_drives==2)
           fprintf(stderr, "Transport descr.:\troot or setuid may only write into /dev/null\n");
         else
           fprintf(stderr, "Transport descr.:\twrite into file objects\n");
         fprintf(stderr,"Transp. layer ind.:\tstdio:\n");
         fprintf(stderr,"Target specifier:\tpath\n");
         fprintf(stderr,"Target example:\t\tstdio:/tmp/pseudo_drive\n");
         fprintf(stderr,"SCSI Bus scanning:\tnot supported\n");
         fprintf(stderr,"Open via UNIX device:\tnot supported\n");
       } else {
         if(Cdrpreskin__allows_emulated_drives("",reason,0)>0)
           printf("\ncdrskin: NOTE : Option --allow_emulated_drives would allow dev=stdio:<path>\n");
       }

#else /* ! Cdrskin_extra_leaN */

       goto see_cdrskin_eng_html;

#endif /* Cdrskin_extra_leaN */

       {ret= 2; goto final_checks;}
     }
     if(strlen(value_pt)>=sizeof(o->raw_device_adr))
       goto dev_too_long;
     strcpy(o->raw_device_adr,value_pt);

   } else if(strcmp(argv[i],"--drive_abort_on_busy")==0) {
     o->abort_on_busy_drive= 1;

   } else if(strcmp(argv[i],"--drive_blocking")==0) {
     o->drive_blocking= 1;

   } else if(strcmp(argv[i],"--drive_f_setlk")==0) {
     o->drive_fcntl_f_setlk= 1;

   } else if(strcmp(argv[i],"--drive_not_exclusive")==0) {
     o->drive_exclusive= 0;
     o->drive_fcntl_f_setlk= 0;

   } else if(strcmp(argv[i],"--drive_not_f_setlk")==0) {
     o->drive_fcntl_f_setlk= 0;

   } else if(strcmp(argv[i],"--drive_not_o_excl")==0) {
     o->drive_exclusive= 0;

   } else if(strncmp(argv[i],"drive_scsi_dev_family=",22)==0) {
     value_pt= argv[i]+22;
     if(strcmp(value_pt,"sr")==0)
       o->drive_scsi_dev_family= 1;
     else if(strcmp(value_pt,"scd")==0)
       o->drive_scsi_dev_family= 2;
     else if(strcmp(value_pt,"sg")==0)
       o->drive_scsi_dev_family= 4;
     else
       o->drive_scsi_dev_family= 0;
   } else if(strcmp(argv[i],"--drive_scsi_exclusive")==0) {
     o->drive_exclusive= 2;
     o->demands_cdrskin_caps= 1;

   } else if(strcmp(argpt,"driveropts=help")==0 ||
             strcmp(argpt,"-driveropts=help")==0) {

#ifndef Cdrskin_extra_leaN

     fprintf(stderr,"Driver options:\n");
     fprintf(stderr,"burnfree\tPrepare writer to use BURN-Free technology\n");
     fprintf(stderr,"noburnfree\tDisable using BURN-Free technology\n");

#else /* ! Cdrskin_extra_leaN */

     goto see_cdrskin_eng_html;

#endif /* Cdrskin_extra_leaN */

     if(argc==2 || (i==2 && argc==3 && strncmp(argv[1],"dev=",4)==0))
       {ret= 2; goto final_checks;}

   } else if(strcmp(argv[i],"--help")==0) {

#ifndef Cdrskin_extra_leaN

     printf("\n");
     printf("Usage: %s [options|source_addresses]\n", argv[0]);
     printf("Burns preformatted data to CD or DVD via libburn.\n");
     printf("For the cdrecord compatible options which control the work of\n");
     printf(
      "blanking and burning see output of option -help rather than --help.\n");
     printf("Non-cdrecord options:\n");
     printf(" --abort_handler    do not leave the drive in busy state\n");
     printf(
     " --adjust_speed_to_drive  set only speeds offered by drive and media\n");
     printf(" --allow_emulated_drives  dev=stdio:<path> on file objects\n");
     printf(
        " --allow_setuid     disable setuid warning (setuid is insecure !)\n");
     printf(
        " --allow_untested_media   enable implemented untested media types\n");
     printf(
         " --any_track        allow source_addresses to match '^-.' or '='\n");
     printf(
         " assert_write_lba=<lba>  abort if not next write address == lba\n");
     printf(
      " cd_start_tno=<number>  set number of first track in CD SAO session\n");
     printf(
        " --cdtext_dummy     show CD-TEXT pack array instead of writing CD\n");
     printf(
   " cdtext_to_textfile=<path>  extract CD-TEXT from CD to disk file\n");
     printf(
   " cdtext_to_v07t=<path|\"-\">  report CD-TEXT from CD to file or stdout\n");
     printf(" --cdtext_verbose   show CD-TEXT pack array before writing CD\n");
     printf(
    " direct_write_amount=<size>  write random access to media like DVD+RW\n");
     printf(" --demand_a_drive   exit !=0 on bus scans with empty result\n");
     printf(" --devices          list accessible devices (tells /dev/...)\n");
     printf(" --device_links     list accessible devices by (udev) links\n");
     printf(
          " dev_translation=<sep><from><sep><to>   set input address alias\n");
     printf("                    e.g.: dev_translation=+ATA:1,0,0+/dev/sg1\n");
     printf(" --drive_abort_on_busy  abort process if busy drive is found\n");
     printf("                    (might be triggered by a busy hard disk)\n");
     printf(" --drive_blocking   try to wait for busy drive to become free\n");
     printf("                    (might be stalled by a busy hard disk)\n");
     printf(" --drive_f_setlk    obtain exclusive lock via fcntl.\n");
     printf(" --drive_not_exclusive  combined not_o_excl and not_f_setlk.\n");
     printf(" --drive_not_f_setlk  do not obtain exclusive lock via fcntl.\n");
     printf(" --drive_not_o_excl   do not ask kernel to prevent opening\n");
     printf("                    busy drives. Effect is kernel dependend.\n");
     printf(
         " drive_scsi_dev_family=<sr|scd|sg|default> select Linux device\n");
     printf("                    file family to be used for (pseudo-)SCSI.\n");
     printf(
         " --drive_scsi_exclusive  try to exclusively reserve device files\n");
     printf("                    /dev/srN, /dev/scdM, /dev/stK of drive.\n");
     printf(" dvd_obs=\"default\"|number\n");
     printf(
     "                    set number of bytes per DVD/BD write: 32k or 64k\n");
     printf(" extract_audio_to=<dir>\n");
     printf(
          "                    Copy all tracks of an audio CD as separate\n");
     printf(
          "                    .WAV files <dir>/track<NN>.wav to hard disk\n");
     printf(" extract_basename=<name>\n");
     printf(
          "                    Compose track files as <dir>/<name><NN>.wav\n");
     printf(
   " --extract_dap      Enable flaw obscuring mechanisms for audio reading\n");
     printf(" extract_tracks=<number[,number[,...]]>\n");
     printf(
        "                    Restrict extract_audio_to= to list of tracks.\n");
     printf(
        " fallback_program=<cmd>  use external program for exotic CD jobs.\n");
     printf(" --fifo_disable     disable fifo despite any fs=...\n");
     printf(" --fifo_per_track   use a separate fifo for each track\n");
     printf(
      " fifo_start_at=<number> do not wait for full fifo but start burning\n");
     printf(
         "                    as soon as the given number of bytes is read\n");
     printf(" --fill_up_media    cause the last track to have maximum size\n");
     printf(" --four_channel     indicate that audio tracks have 4 channels\n");
     printf(
          " grab_drive_and_wait=<num>  grab drive, wait given number of\n");
     printf(
          "                    seconds, release drive, and do normal work\n");
     printf(
    " --grow_overwriteable_iso  emulate multi-session on media like DVD+RW\n");
     printf(
       " --ignore_signals   try to ignore any signals rather than to abort\n");
     printf(" input_sheet_v07t=<path>  read a Sony CD-TEXT definition file\n");
     printf(" --list_formats     list format descriptors for loaded media.\n");
     printf(" --list_ignored_options list all ignored cdrecord options.\n");
     printf(" --list_speeds      list speed descriptors for loaded media.\n");
     printf(" --long_toc         print overview of media content\n");
     printf(" modesty_on_drive=<options> no writing into full drive buffer\n");
     printf(" --no_abort_handler  exit even if the drive is in busy state\n");
     printf(" --no_blank_appendable  refuse to blank appendable CD-RW\n");
     printf(" --no_convert_fs_adr  only literal translations of dev=\n");
     printf(" --no_load          do not try to load the drive tray\n");
     printf(
         " --no_rc            as first argument: do not read startup files\n");
     printf(" --obs_pad          pad DVD DAO to full 16 or 32 blocks\n");
     printf(" --old_pseudo_scsi_adr  use and report literal Bus,Target,Lun\n");
     printf("                    rather than real SCSI and pseudo ATA.\n");
     printf(
    " --pacifier_with_newline  do not overwrite pacifier line by next one.\n");
     printf(" --prodvd_cli_compatible  react on some DVD types more like\n");
     printf("                    cdrecord-ProDVD with blank= and -multi\n");
     printf(" sao_postgap=\"off\"|number\n");
     printf("                    defines whether the next track will get a\n");
     printf(
          "                    post-gap and how many sectors it shall have\n");
     printf(" sao_pregap=\"off\"|number\n");
     printf("                    defines whether the next track will get a\n");
     printf(
           "                    pre-gap and how many sectors it shall have\n");
     printf(
          " --single_track     accept only last argument as source_address\n");
     printf(" stream_recording=\"on\"|\"off\"|number\n");
     printf(
         "                    \"on\" requests to prefer speed over write\n");
     printf(
         "                    error management. A number prevents this with\n");
     printf(
         "                    byte addresses below that number.\n");
     printf(" stdio_sync=\"default\"|\"off\"|number\n");
     printf(
     "                    set number of bytes after which to force output\n");
     printf(
     "                    to drives with prefix \"stdio:\".\n");
     printf(
          " tao_to_sao_tsize=<num>  use num as fixed track size if in a\n");
     printf(
          "                    non-TAO mode track data are read from \"-\"\n");
     printf(
          "                    and no tsize= was specified.\n");
     printf(
 " --tell_media_space  print maximum number of writeable data blocks\n");
     printf(" textfile_to_v07t=file_path\n");
     printf(
          "                    print CD-TEXT file in Sony format and exit.\n");
     printf(" --two_channel      indicate that audio tracks have 2 channels\n");
     printf(
         " write_start_address=<num>  write to given byte address (DVD+RW)\n");
     printf(
         " --xa1-ignore       with -xa1 do not strip 8 byte headers\n");
     printf(
 " -xamix             DO NOT USE (Dummy announcement to make K3B use -xa)\n");
     printf(
        "Preconfigured arguments are read from the following startup files\n");
     printf(
          "if they exist and are readable. The sequence is as listed here:\n");
     printf("  /etc/default/cdrskin         /etc/opt/cdrskin/rc\n");
     printf("  /etc/cdrskin/cdrskin.conf    $HOME/.cdrskinrc\n");
     printf("Each file line is a single argument. No whitespace.\n");
     printf(
         "By default any argument that does not match grep '^-.' or '=' is\n");
     printf(
         "used as track source. If it is \"-\" then stdin is used.\n");
     printf("cdrskin  : http://scdbackup.sourceforge.net/cdrskin_eng.html\n");
     printf("           mailto:scdbackup@gmx.net  (Thomas Schmitt)\n");
     printf("libburn  : http://libburnia-project.org\n");
     printf("cdrecord : ftp://ftp.berlios.de/pub/cdrecord/\n");
     printf("My respect to the authors of cdrecord and libburn.\n");
     printf("scdbackup: http://scdbackup.sourceforge.net/main_eng.html\n");
     printf("\n");

#else /* ! Cdrskin_extra_leaN */

see_cdrskin_eng_html:;
     printf("This is a capability reduced lean version without help texts.\n");
     printf("See  http://scdbackup.sourceforge.net/cdrskin_eng.html\n");

#endif /* Cdrskin_extra_leaN */


     {ret= 2; goto final_checks;}
   } else if(strcmp(argv[i],"-help")==0) {

#ifndef Cdrskin_extra_leaN

     fprintf(stderr,"Usage: %s [options|source_addresses]\n",argv[0]);
     fprintf(stderr,"Note: This is not cdrecord. See cdrskin start message on stdout. See --help.\n");
     fprintf(stderr,"Options:\n");
     fprintf(stderr,"\t-version\tprint version information and exit\n");
     fprintf(stderr,
             "\tdev=target\tpseudo-SCSI target to use as CD-Recorder\n");
     fprintf(stderr,
         "\tgracetime=#\tset the grace time before starting to write to #.\n");
     fprintf(stderr,"\t-v\t\tincrement general verbose level by one\n");
     fprintf(stderr,
            "\t-V\t\tincrement SCSI command transport verbose level by one\n");
     fprintf(stderr,
             "\tdriveropts=opt\topt= one of {burnfree,noburnfree,help}\n");
     fprintf(stderr,
             "\t-checkdrive\tcheck if a driver for the drive is present\n");
     fprintf(stderr,"\t-inq\t\tdo an inquiry for the drive and exit\n");
     fprintf(stderr,"\t-scanbus\tscan the SCSI bus and exit\n");
     fprintf(stderr,"\tspeed=#\t\tset speed of drive\n");
     fprintf(stderr,"\tblank=type\tblank a CD-RW disc (see blank=help)\n");
     fprintf(stderr,"\t-format\t\tformat a CD-RW/DVD-RW/DVD+RW disc\n");
     fprintf(stderr,
             "\tfs=#\t\tSet fifo size to # (0 to disable, default is 4 MB)\n");
     fprintf(stderr,
          "\t-load\t\tload the disk and exit (works only with tray loader)\n");
     fprintf(stderr,
 "\t-lock\t\tload and lock the disk and exit (works only with tray loader)\n");
     fprintf(stderr,
       "\t-eject\t\teject the disk after doing the work\n");
     fprintf(stderr,"\t-dummy\t\tdo everything with laser turned off\n");
     fprintf(stderr,"\t-minfo\t\tretrieve and print media information/status\n");
     fprintf(stderr,"\t-media-info\tretrieve and print media information/status\n");
     fprintf(stderr,
             "\t-msinfo\t\tretrieve multi-session info for mkisofs >= 1.10\n");
     fprintf(stderr,"\tmsifile=path\trun -msinfo and copy output to file\n");
     fprintf(stderr,"\t-toc\t\tretrieve and print TOC/PMA data\n");
     fprintf(stderr,
             "\t-atip\t\tretrieve media state, print \"Is *erasable\"\n");
     fprintf(stderr,
             "\tminbuf=percent\tset lower limit for drive buffer modesty\n");
     fprintf(stderr,
             "\t-multi\t\tgenerate a TOC that allows multi session\n");
     fprintf(stderr,
           "\t-waiti\t\twait until input is available before opening SCSI\n");
     fprintf(stderr,
           "\t-immed\t\tTry to use the SCSI IMMED flag with certain long lasting commands\n");
     fprintf(stderr,
           "\t-force\t\tforce to continue on some errors to allow blanking\n");
     fprintf(stderr,"\t-tao\t\tWrite disk in TAO mode.\n");
     fprintf(stderr,"\t-dao\t\tWrite disk in SAO mode.\n");
     fprintf(stderr,"\t-sao\t\tWrite disk in SAO mode.\n");

#ifndef Cdrskin_disable_raw96R
     fprintf(stderr,"\t-raw96r\t\tWrite disk in RAW/RAW96R mode\n");
#endif

     fprintf(stderr,"\ttsize=#\t\tannounces exact size of source data\n");
     fprintf(stderr,"\tpadsize=#\tAmount of padding\n");
     fprintf(stderr,
         "\tmcn=text\tSet the media catalog number for this CD to 'text'\n");
     fprintf(stderr,
          "\tisrc=text\tSet the ISRC number for the next track to 'text'\n");
     fprintf(stderr,
          "\tindex=list\tSet the index list for the next track to 'list'\n");
     fprintf(stderr,
             "\t-text\t\tWrite CD-Text from information from *.cue files\n");
     fprintf(stderr,
             "\ttextfile=name\tSet the file with CD-Text data to 'name'\n");
     fprintf(stderr,
             "\tcuefile=name\tSet the file with CDRWIN CUE data to 'name'\n");
     fprintf(stderr,"\t-audio\t\tSubsequent tracks are CD-DA audio tracks\n");
     fprintf(stderr,
            "\t-data\t\tSubsequent tracks are CD-ROM data mode 1 (default)\n");
     fprintf(stderr,
     "\t-xa1\t\tSubsequent tracks are CD-ROM XA mode 2 form 1 - 2056 bytes\n");
     fprintf(stderr,
           "\t-isosize\tUse iso9660 file system size for next data track\n");
     fprintf(stderr,"\t-preemp\t\tAudio tracks are mastered with 50/15 microseconds preemphasis\n");
     fprintf(stderr,"\t-nopreemp\tAudio tracks are mastered with no preemphasis (default)\n");
     fprintf(stderr,"\t-copy\t\tAudio tracks have unlimited copy permission\n");
     fprintf(stderr,"\t-nocopy\t\tAudio tracks may only be copied once for personal use (default)\n");
     fprintf(stderr,"\t-scms\t\tAudio tracks will not have any copy permission at all\n");
     fprintf(stderr,"\t-pad\t\tpadsize=30k\n");
     fprintf(stderr,
        "\t-nopad\t\tDo not pad (default, but applies only to data tracks)\n");
     fprintf(stderr,
       "\t-swab\t\tAudio data source is byte-swapped (little-endian/Intel)\n");
     fprintf(stderr,"\t-help\t\tprint this text to stderr and exit\n");
     fprintf(stderr,
             "Without option -data, .wav and .au files are extracted and burned as -audio.\n");
     fprintf(stderr,
    "By default any argument that does not match grep '^-.' or '=' is used\n");
     fprintf(stderr,
         "as track source address. Address \"-\" means stdin.\n");
     fprintf(stderr,
        "cdrskin will ensure that an announced tsize= is written even if\n");
     fprintf(stderr,
        "the source delivers fewer bytes. But 0 bytes from stdin with fifo\n");
     fprintf(stderr,
        "enabled will lead to abort and no burn attempt at all.\n");

#else /* ! Cdrskin_extra_leaN */

     fprintf(stderr,"Note: This is not cdrecord. See cdrskin start message on stdout.\n");
     fprintf(stderr,
          "(writer profile: -atip retrieve, blank=type, -eject after work)\n");
     goto see_cdrskin_eng_html;

#endif /* Cdrskin_extra_leaN */

     {ret= 2; goto final_checks;}

   } else if(strcmp(argv[i],"--ignore_signals")==0) {
     o->abort_handler= 2;

   } else if(strncmp(argv[i],"fallback_program=",17)==0) {
     strcpy(o->fallback_program,argv[i]+17);

   } else if(strcmp(argv[i],"--no_abort_handler")==0) {
     o->abort_handler= 0;

   } else if(strcmp(argv[i],"--no_convert_fs_adr")==0) {
     o->no_convert_fs_adr= 1;

   } else if(strcmp(argv[i],"--old_pseudo_scsi_adr")==0) {
     o->old_pseudo_scsi_adr= 1;
     o->demands_cdrskin_caps= 1;

   } else if(strcmp(argv[i],"--no_rc")==0) {
     if(i!=1)
       fprintf(stderr,
        "cdrskin: NOTE : option --no_rc would only work as first argument.\n");

#ifndef Cdrskin_disable_raw96R
   } else if(strcmp(argpt,"-raw96r")==0) {
     strcpy(o->write_mode_name,"RAW/RAW96R");
#endif

   } else if(strcmp(argpt,"-sao")==0 || strcmp(argpt,"-dao")==0) {
     strcpy(o->write_mode_name,"SAO");

   } else if(strcmp(argpt,"-scanbus")==0) {
     o->no_whitelist= 1;

   } else if(strcmp(argpt,"-tao")==0) {
     strcpy(o->write_mode_name,"TAO");

   } else if(strncmp(argpt, "-textfile_to_v07t=", 18) == 0) {
     o->do_not_scan= 1;
   } else if(strncmp(argpt ,"textfile_to_v07t=", 17) == 0) {
     o->do_not_scan= 1;

   } else if(strcmp(argv[i],"-V")==0 || strcmp(argpt, "-Verbose") == 0) {
     burn_set_scsi_logging(2 | 4); /* log SCSI to stderr */

   } else if(strcmp(argv[i],"-v")==0 || strcmp(argpt, "-verbose") == 0) {
     (o->verbosity)++;
     ClN(printf("cdrskin: verbosity level : %d\n",o->verbosity));
set_severities:;
     if(o->verbosity>=Cdrskin_verbose_debuG)
       Cdrpreskin_set_severities(o,"NEVER","DEBUG",0);
     else if(o->verbosity >= Cdrskin_verbose_progresS)
       Cdrpreskin_set_severities(o, "NEVER", "UPDATE", 0);

   } else if(strcmp(argv[i],"-vv")==0 || strcmp(argv[i],"-vvv")==0 ||
             strcmp(argv[i],"-vvvv")==0) {
     (o->verbosity)+= strlen(argv[i])-1;
     goto set_severities;

   } else if(strcmp(argpt,"-version")==0) {
     int major, minor, micro;

     printf(
"Cdrecord 2.01-Emulation Copyright (C) 2006-2013, see libburnia-project.org\n");
     if(o->fallback_program[0]) {
       char *hargv[2];

       printf("Fallback program  :  %s\n",o->fallback_program);
       printf("Fallback version  :\n");
       hargv[0]= argv[0];
       hargv[1]= "-version";
       Cdrpreskin_fallback(o,2,hargv,1); /* dirty never come back */
     }
     printf("System adapter    :  %s\n", burn_scsi_transport_id(0));
     printf("libburn interface :  %d.%d.%d\n",
            burn_header_version_major, burn_header_version_minor,
            burn_header_version_micro);
     burn_version(&major, &minor, &micro);
     printf("libburn in use    :  %d.%d.%d\n", major, minor, micro);

#ifndef Cdrskin_extra_leaN
     printf("cdrskin version   :  %s\n",Cdrskin_prog_versioN);
#else
     printf("cdrskin version   :  %s.lean (capability reduced lean version)\n",
            Cdrskin_prog_versioN);
#endif

     printf("Version timestamp :  %s\n",Cdrskin_timestamP);
     printf("Build timestamp   :  %s\n",Cdrskin_build_timestamP);
     {ret= 2; goto ex;}

   } else if(strcmp(argpt,"-waiti")==0) {
     o->do_waiti= 1;

   } else if(strcmp(argpt,"-xamix")==0) {
     fprintf(stderr,
       "cdrskin: FATAL : Option -xamix not implemented and data not yet convertible to other modes.\n");
     return(0);
   }

 }
 ret= 1;
final_checks:;
 if(flag&1)
   goto ex;
 if(o->verbosity >= Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,
       "cdrskin: DEBUG : Using %s\n", burn_scsi_transport_id(0)));
 if(o->do_waiti) {
   fprintf(stderr,
       "cdrskin: Option -waiti pauses program until input appears at stdin\n");
   printf("Waiting for data on stdin...\n");
   for(ret= 0; ret==0; )
     ret= Wait_for_input(0,1000000,0);
   if(ret<0 || feof(stdin))
     fprintf(stderr,
             "cdrskin: NOTE : stdin produces exception rather than data\n");
   fprintf(stderr,"cdrskin: Option -waiti pausing is done.\n");
 }
 burn_preset_device_open(o->drive_exclusive
                         | (o->drive_scsi_dev_family<<2)
                         | ((!!o->drive_fcntl_f_setlk)<<5),
                         o->drive_blocking,
                         o->abort_on_busy_drive);

 if(strlen(o->raw_device_adr)>0 && !o->no_whitelist) {
   int driveno,hret;
   char *adr,buf[Cdrskin_adrleN];

   if(strcmp(o->raw_device_adr,"stdio:-")==0) {
     fprintf(stderr,
             "cdrskin: SORRY : Cannot accept drive address \"stdio:-\".\n");
     fprintf(stderr,
             "cdrskin: HINT  : Use \"stdio:/dev/fd/1\" if you really want to write to stdout.\n");
     {ret= 0; goto ex;}
   }
   adr= o->raw_device_adr;

#ifndef Cdrskin_extra_leaN
   if(o->adr_trn!=NULL) {
     hret= Cdradrtrn_translate(o->adr_trn,adr,-1,buf,0);
     if(hret<=0) {
       fprintf(stderr,
        "cdrskin: FATAL : address translation failed (address too long ?) \n");
       {ret= 0; goto ex;}
     }
     adr= buf;
   }
#endif /* ! Cdrskin_extra_leaN */

   if(adr[0]=='/') {
     if(strlen(adr)>=sizeof(o->device_adr)) {
dev_too_long:;
       fprintf(stderr,
               "cdrskin: FATAL : dev=... too long (max. %d characters)\n",
               (int) sizeof(o->device_adr)-1);
       {ret= 0; goto ex;}
     }
     strcpy(o->device_adr,adr);
   } else {
     ret= Cdrpreskin__cdrecord_to_dev(adr,o->device_adr,&driveno,
                                      !!o->old_pseudo_scsi_adr);
     if(ret==-2 || ret==-3)
       {ret= 0; goto ex;}
     if(ret<0)
       goto ex;
     if(ret==0) {
       strcpy(o->device_adr,adr);
       ret= 1;
     }
   }

   if(strlen(o->device_adr)>0 && !o->no_convert_fs_adr) {
     int lret;
     char link_adr[Cdrskin_strleN+1];

     strcpy(link_adr,o->device_adr);
     lret = burn_drive_convert_fs_adr(link_adr,o->device_adr);
     if(lret<0) {
       fprintf(stderr,
           "cdrskin: NOTE : Please inform libburn-hackers@pykix.org about:\n");
       fprintf(stderr,
	   "cdrskin:        burn_drive_convert_fs_adr() returned %d\n",lret);
     }
   }

 }

 burn_allow_untested_profiles(!!o->allow_untested_media);

 /* A60927 : note to myself : no "ret= 1;" here. It breaks --help , -version */

ex:;
 /* Eventually replace current stdout by dup(1) from start of program */
 if(strcmp(o->device_adr,"stdio:/dev/fd/1")==0 && o->result_fd >= 0)
   sprintf(o->device_adr,"stdio:/dev/fd/%d",o->result_fd);

#ifndef Cdrskin_extra_leaN
 if(ret<=0 || !(flag&1))
   Cdradrtrn_destroy(&(o->adr_trn),0);
#endif

 return(ret);
}


/* --------------------------------------------------------------------- */



/** The maximum number of tracks */
#define Cdrskin_track_maX 99


/** Work around the fact that libburn leaves the track input fds open
    after the track is done. This can hide a few overflow bytes buffered 
    by the fifo-to-libburn pipe which would cause a broken-pipe error
    if libburn would close that outlet.
    This macro enables a coarse workaround which tries to read bytes from
    the track inlets after burning has ended. Probably not a good idea if
    libburn would close the inlet fds.
*/
#define Cdrskin_libburn_leaves_inlet_opeN 1


/** List of furter wishes towards libburn:
    - a possibilty to implement cdrskin -reset
*/


#define Cdrskin_tracksize_maX 1024.0*1024.0*1024.0*1024.0


/* Some constants obtained by hearsay and experiments */

/** The CD payload speed factor for reporting progress: 1x = 150 kB/s */
static double Cdrskin_cd_speed_factoR= 150.0*1024.0;
/** The DVD payload speed factor for reporting progress: 1x */
static double Cdrskin_dvd_speed_factoR= 1385000;
/** The BD payload speed factor for reporting progress: 1x */
static double Cdrskin_bd_speed_factoR= 4495625;

/** The effective payload speed factor for reporting progress */
static double Cdrskin_speed_factoR= 150.0*1024.0;

/** The speed conversion factors consumer x-speed to libburn speed as used with
    burn_drive_set_speed() burn_drive_get_write_speed()
*/
static double Cdrskin_libburn_cd_speed_factoR= 176.4;
static double Cdrskin_libburn_dvd_speed_factoR= 1385.0;
static double Cdrskin_libburn_bd_speed_factoR= 4495.625;

/* The effective speed conversion factor for burn_drive_set_speed() */
static double Cdrskin_libburn_speed_factoR= 176.4;

/** Add-on for burn_drive_set_speed() to accomodate to the slightley oversized
    speed ideas of my LG DVDRAM GSA-4082B. LITE-ON LTR-48125S tolerates it.
*/
static double Cdrskin_libburn_cd_speed_addoN= 40.0;
static double Cdrskin_libburn_dvd_speed_addoN= 1.0; /*poor accuracy with 2.4x*/
static double Cdrskin_libburn_bd_speed_addoN= 1.0;
static double Cdrskin_libburn_speed_addoN = 40.0;


/** The program run control object. Defaults: see Cdrskin_new(). */
struct CdrskiN {

 /** Settings already interpreted by Cdrpreskin_setup */
 struct CdrpreskiN *preskin;

 /** Job: what to do, plus some parameters. */
 int verbosity;
 int pacifier_with_newline;
 double x_speed;
 int adjust_speed_to_drive;
 int gracetime;
 int dummy_mode;
 int force_is_set;
 int stream_recording_is_set; /* see burn_write_opts_set_stream_recording() */
 int dvd_obs;                 /* DVD write chunk size: 0, 32k or 64k */
 int obs_pad;                 /* Whether to force obs end padding */
 int stdio_sync;              /* stdio fsync interval: -1, 0, >=32 */
 int single_track;
 int prodvd_cli_compatible;

 int do_devices;              /* 1= --devices , 2= --device_links */

 int do_scanbus;

 int do_load;                 /* 1= -load , 2= -lock , -1= --no_load */

 int do_checkdrive;

 int do_msinfo;
 char msifile[Cdrskin_strleN];

 int do_atip;
 int do_list_speeds;
 int do_list_formats;
 int do_cdtext_to_textfile;
 char cdtext_to_textfile_path[Cdrskin_strleN];
 int do_cdtext_to_vt07;
 char cdtext_to_vt07_path[Cdrskin_strleN];
 int do_extract_audio;
 char extract_audio_dir[Cdrskin_strleN];
 char extract_basename[249];
 char extract_audio_tracks[100];     /* if [i] > 0 : extract track i */
 int extract_flags;                  /* bit3 = for flag bit3 of
                                               burn_drive_extract_audio()
                                     */

#ifdef Libburn_develop_quality_scaN
 int do_qcheck;              /* 0= no , 1=nec_optiarc_rep_err_rate */
#endif /* Libburn_develop_quality_scaN */

 int do_blank;
 int blank_fast;
 int no_blank_appendable;
 int blank_format_type; /* bit0-7: job type
                           0=blank
                           1=format_overwrite for DVD+RW, DVD-RW
                             bit8-15: bit0-7 of burn_disc_format(flag)
                               bit8 = write zeros after formatting
                               bit9 = insist in size 0
                               bit10= format to maximum available size
                               bit11= - reserved -
                               bit12= - reserved -
                               bit13= try to disable eventual defect management
                               bit14= - reserved -
                               bit15= format by index
                           2=deformat_sequential (blank_fast might matter)
                           3=format (= format_overwrite restricted to DVD+RW)
                           4=format_defectmgt for DVD-RAM, BD-R[E]
                             bit8-15: bit0-7 of burn_disc_format(flag)
                               bit8 = write zeros after formatting
                               bit9+10: size mode
                               0 = use parameter size as far as it makes sense
                               1 = (identical to size mode 0)
                               2 = without bit7: format to maximum size
                                   with bit7   : take size from indexed format
                                                 descriptor
                               3 = without bit7: format to default size
                                   with bit7   : take size from indexed format
                                                 descriptor
                               bit11= - reserved -
                               bit12= - reserved -
                               bit13= try to disable defect management
                               bit14= - reserved -
                               bit15= format by index
                           5=format_by_index
                             gets mapped to 4 with DVD-RAM and BD-RE else to 1,
                             bit15 should be set and bit16-23 should contain
                             a usable index number
                           6=format_if_needed
                             gets mapped to default variants of specialized
                             formats if the media state requires formatting
                             before writing
                           7=if_needed
                             gets mapped to 6 for DVD-RAM and BD-RE,
                             to 0 with all other non-blanks

                           bit8-15: bit0-7 of burn_disc_format(flag)
                                    depending on job type
                        */
 int blank_format_index;   /* bit8-15 of burn_disc_format(flag) */
 double blank_format_size; /* to be used with burn_disc_format() */
 int blank_format_no_certify;

 int do_direct_write;
 int do_burn;
 int tell_media_space; /* actually do not burn but tell the available space */
 int burnfree;
 /** The write mode (like SAO or TAO). See libburn. 
     Controled by preskin->write_mode_name */
 enum burn_write_types write_type;
 int block_type;
 int multi;
 int cdxa_conversion; /* bit0-30: for burn_track_set_cdxa_conv()
                         bit31  : ignore bits 0 to 30
                       */
 int modesty_on_drive;
 int min_buffer_percent;
 int max_buffer_percent;

 double write_start_address;
 double direct_write_amount;
 int assert_write_lba;

 int do_eject;
 char eject_device[Cdrskin_strleN];


 /** The current data source and its eventual parameters. 
     source_path may be either "-" for stdin, "#N" for open filedescriptor N
     or the address of a readable file.
 */
 char source_path[Cdrskin_strleN];
 double fixed_size;
 double smallest_tsize;
 int has_open_ended_track;
 double padding;
 int set_by_padsize;
 int fill_up_media;

 /** track_type may be set to BURN_MODE1, BURN_AUDIO, etc. */
 int track_type;
 int track_type_by_default; /* 0= explicit, 1=not set, 2=by file extension */
 int swap_audio_bytes;
 int track_modemods;

 /** CDRWIN cue sheet file */
 char cuefile[Cdrskin_adrleN]; 
 int use_cdtext;

 /** CD-TEXT */
 unsigned char *text_packs;
 int num_text_packs;

 int sheet_v07t_blocks;
 char sheet_v07t_paths[8][Cdrskin_adrleN];

 int cdtext_test;

 /* Media Catalog Number and ISRC */
 char mcn[14];
 char next_isrc[13];

 char *index_string;

 int sao_pregap;
 int sao_postgap;

 /** The list of tracks with their data sources and parameters */
 struct CdrtracK *tracklist[Cdrskin_track_maX];
 int track_counter;
 /** a guess about what track might be processing right now */
 int supposed_track_idx;

 /** The number of the first track in a CD SAO session */
 int cd_start_tno;

 int fifo_enabled;
 /** Optional fifo between input fd and libburn. It uses a pipe(2) to transfer
     data to libburn. This fifo may be actually the start of a chain of fifos
     which are to be processed simultaneously.
     The fifo object knows the real input fd and the fd[1] of the pipe.
     This is just a reference pointer. The fifos are managed by the tracks
     which either line up their fifos or share the fifo of the first track.
 */
 struct CdrfifO *fifo;
 /** fd[0] of the fifo pipe. This is from where libburn reads its data. */
 int fifo_outlet_fd;
 int fifo_size;
 int fifo_start_at;
 int fifo_per_track;
 struct burn_source *cuefile_fifo;

 /** User defined address translation */
 struct CdradrtrN *adr_trn;


 /** The drives known to libburn after scan */
 struct burn_drive_info *drives;
 unsigned int n_drives;
 /** The drive selected for operation by CdrskiN */
 int driveno;
 /** The persistent drive address of that drive */
 char device_adr[Cdrskin_adrleN];


 /** Progress state info: whether libburn is actually processing payload data*/
 int is_writing;
 /** Previously detected drive state */
 enum burn_drive_status previous_drive_status;

 /** abort parameters */
 int abort_max_wait;

 /** Engagement info for eventual abort */
 int lib_is_initialized;
 pid_t control_pid; /* pid of the thread that calls libburn */
 int drive_is_grabbed;
 struct burn_drive *grabbed_drive;
 /* Whether drive was told to do something cancel-worthy 
    0= no: directly call burn_abort
    1= yes: A worker thread is busy (e.g. writing, formatting)
    2= yes: A synchronous operation is busy (e.g. grabbing).
 */
 int drive_is_busy;

#ifndef Cdrskin_extra_leaN
 /** Abort test facility */
 double abort_after_bytecount; 
#endif /* ! Cdrskin_extra_leaN */


 /** Some intermediate option info which is stored until setup finalization */
 double tao_to_sao_tsize;
 int stdin_source_used;

 /* Info about media capabilities */
 int media_does_multi;
 int media_is_overwriteable;

 /* For option -isosize and --grow_overwriteable_iso */
 int use_data_image_size;

 /* For growisofs stunt :
    0=disabled,
    1=do stunt, fabricate toc, allow multi,
    2=overwriteable_iso_head is valid
    3=initial session (mostly to appease -multi on overwriteables)
 */
 int grow_overwriteable_iso;
 /* New image head buffer for --grow_overwriteable_iso */
 char overwriteable_iso_head[32*2048]; /* block  0 to 31 of target */

};

int Cdrskin_destroy(struct CdrskiN **o, int flag);
int Cdrskin_grab_drive(struct CdrskiN *skin, int flag);
int Cdrskin_release_drive(struct CdrskiN *skin, int flag);
int Cdrskin_report_disc_status(struct CdrskiN *skin, enum burn_disc_status s,
                               int flag);

/** Create a cdrskin program run control object.
    @param skin Returns pointer to resulting
    @param flag Bitfield for control purposes: 
                bit0= library is already initialized
    @return <=0 error, 1 success
*/
int Cdrskin_new(struct CdrskiN **skin, struct CdrpreskiN *preskin, int flag)
{
 struct CdrskiN *o;
 int ret,i;

 (*skin)= o= TSOB_FELD(struct CdrskiN,1);
 if(o==NULL)
   return(-1);
 o->preskin= preskin;
 o->verbosity= preskin->verbosity;
 o->pacifier_with_newline= 0;
 o->x_speed= -1.0;
 o->adjust_speed_to_drive= 0;
 o->gracetime= 0;
 o->dummy_mode= 0;
 o->force_is_set= 0;
 o->stream_recording_is_set= 0;
 o->dvd_obs= 0;
 o->obs_pad= 0;
 o->stdio_sync= 0;
 o->single_track= 0;
 o->prodvd_cli_compatible= 0;
 o->do_devices= 0;
 o->do_scanbus= 0;
 o->do_load= 0;
 o->do_checkdrive= 0;
 o->do_msinfo= 0;
 o->msifile[0]= 0;
 o->do_atip= 0;
 o->do_list_speeds= 0;
 o->do_list_formats= 0;
 o->do_cdtext_to_textfile= 0;
 o->cdtext_to_textfile_path[0]= 0;
 o->do_cdtext_to_vt07= 0;
 o->cdtext_to_vt07_path[0]= 0;
 o->do_extract_audio= 0;
 o->extract_audio_dir[0]= 0;
 o->extract_basename[0]= 0;
 for(i= 0; i < 100; i++)
   o->extract_audio_tracks[i]= 0;
 o->extract_flags= 0;

#ifdef Libburn_develop_quality_scaN
 o->do_qcheck= 0;
#endif /* Libburn_develop_quality_scaN */

 o->do_blank= 0;
 o->blank_fast= 0;
 o->no_blank_appendable= 0;
 o->blank_format_type= 0;
 o->blank_format_index= -1;
 o->blank_format_size= 0.0;
 o->blank_format_no_certify= 0;
 o->do_direct_write= 0;
 o->do_burn= 0;
 o->tell_media_space= 0;
 o->write_type= BURN_WRITE_SAO;
 o->block_type= BURN_BLOCK_SAO;
 o->multi= 0;
 o->cdxa_conversion= 0;
 o->modesty_on_drive= 0;
 o->min_buffer_percent= 65;
 o->max_buffer_percent= 95;
 o->write_start_address= -1.0;
 o->direct_write_amount= -1.0;
 o->assert_write_lba= -1;
 o->burnfree= 1;
 o->do_eject= 0;
 o->eject_device[0]= 0;
 o->source_path[0]= 0;
 o->fixed_size= 0.0;
 o->smallest_tsize= -1.0;
 o->has_open_ended_track= 0;
 o->padding= 0.0;
 o->set_by_padsize= 0;
 o->fill_up_media= 0;
 o->track_type= BURN_MODE1;
 o->swap_audio_bytes= 1;   /* cdrecord default is big-endian (msb_first) */
 o->cuefile[0] = 0; 
 o->use_cdtext = 0;
 o->text_packs= NULL;
 o->num_text_packs= 0;
 o->sheet_v07t_blocks= 0;
 for(i= 0; i < 8; i++)
   memset(o->sheet_v07t_paths, 0, Cdrskin_adrleN);
 o->cdtext_test= 0;
 o->mcn[0]= 0;
 o->next_isrc[0]= 0;
 o->index_string= NULL;
 o->sao_pregap= -1;
 o->sao_postgap= -1;
 o->track_modemods= 0;
 o->track_type_by_default= 1;
 for(i=0;i<Cdrskin_track_maX;i++)
   o->tracklist[i]= NULL;
 o->track_counter= 0;
 o->supposed_track_idx= -1;
 o->cd_start_tno= 0;
 o->fifo_enabled= 1;
 o->fifo= NULL;
 o->fifo_outlet_fd= -1;
 o->fifo_size= 4*1024*1024;
 o->fifo_start_at= -1;
 o->fifo_per_track= 0;
 o->cuefile_fifo= NULL;
 o->adr_trn= NULL;
 o->drives= NULL;
 o->n_drives= 0;
 o->driveno= 0;
 o->device_adr[0]= 0;
 o->is_writing= 0;
 o->previous_drive_status = BURN_DRIVE_IDLE;
 o->abort_max_wait= 74*60;
 o->lib_is_initialized= (flag&1);
 o->control_pid= getpid();
 o->drive_is_grabbed= 0;
 o->drive_is_busy= 0;
 o->grabbed_drive= NULL;
#ifndef Cdrskin_extra_leaN
 o->abort_after_bytecount= -1.0;
#endif /* ! Cdrskin_extra_leaN */
 o->tao_to_sao_tsize= 0.0;
 o->stdin_source_used= 0;
 o->use_data_image_size= 0;
 o->media_does_multi= 0;
 o->media_is_overwriteable= 0;
 o->grow_overwriteable_iso= 0;
 memset(o->overwriteable_iso_head,0,sizeof(o->overwriteable_iso_head));

#ifndef Cdrskin_extra_leaN
 ret= Cdradrtrn_new(&(o->adr_trn),0);
 if(ret<=0)
   goto failed;
#endif /* ! Cdrskin_extra_leaN */

 return(1);
failed:;
 Cdrskin_destroy(skin,0);
 return(-1);
}


/** Release from memory a cdrskin object */
int Cdrskin_destroy(struct CdrskiN **o, int flag)
{
 struct CdrskiN *skin;
 int i;

 skin= *o;
 if(skin==NULL)
   return(0);
 if(skin->drive_is_grabbed)
   Cdrskin_release_drive(skin,0);
 for(i=0;i<skin->track_counter;i++)
   Cdrtrack_destroy(&(skin->tracklist[i]),0);
 if(skin->index_string != NULL)
   free(skin->index_string);

#ifndef Cdrskin_extra_leaN
 Cdradrtrn_destroy(&(skin->adr_trn),0);
#endif /* ! Cdrskin_extra_leaN */

 if(skin->cuefile_fifo != NULL)
   burn_source_free(skin->cuefile_fifo);
 if(skin->text_packs != NULL)
   free(skin->text_packs);
 Cdrpreskin_destroy(&(skin->preskin),0);
 if(skin->drives!=NULL)
   burn_drive_info_free(skin->drives);
 free((char *) skin);
 *o= NULL;
 return(1);
}


int Cdrskin_assert_driveno(struct CdrskiN *skin, int flag)
{
 if(skin->driveno < 0 || (unsigned int) skin->driveno >= skin->n_drives) {
   fprintf(stderr,
       "cdrskin: FATAL : No drive found. Cannot perform desired operation.\n");
   return(0);
 }
 return(1);
}


/** Return the addresses of the drive. device_adr is the libburn persistent
    address of the drive, raw_adr is the address as given by the user.
*/
int Cdrskin_get_device_adr(struct CdrskiN *skin,
           char **device_adr, char **raw_adr, int *no_convert_fs_adr, int flag)
{
 if(skin->driveno < 0 || (unsigned int) skin->driveno >= skin->n_drives)
   return(0);
 burn_drive_get_adr(&skin->drives[skin->driveno],skin->device_adr);
 *device_adr= skin->device_adr;
 *raw_adr= skin->preskin->raw_device_adr;
 *no_convert_fs_adr= skin->preskin->no_convert_fs_adr;
 return(1); 
}


int Cdrskin_get_drive(struct CdrskiN *skin, struct burn_drive **drive,int flag)
{
 if(skin->driveno<0 || (unsigned int) skin->driveno >= skin->n_drives)
   return(0);
 *drive= skin->drives[skin->driveno].drive;
 return ((*drive) != NULL); 
}


/** Return information about current track source */
int Cdrskin_get_source(struct CdrskiN *skin, char *source_path,
                       double *fixed_size, double *tao_to_sao_tsize,
                       int *use_data_image_size,
                       double *padding, int *set_by_padsize,
                       int *track_type, int *track_type_by_default,
                       int *mode_mods,
                       int *swap_audio_bytes, int *cdxa_conversion, int flag)
{
 strcpy(source_path,skin->source_path);
 *fixed_size= skin->fixed_size;
 *tao_to_sao_tsize = skin->tao_to_sao_tsize;
 *use_data_image_size= skin->use_data_image_size;
 *padding= skin->padding;
 *set_by_padsize= skin->set_by_padsize;
 *track_type= skin->track_type;
 *track_type_by_default= skin->track_type_by_default;
 *mode_mods= skin->track_modemods;
 *swap_audio_bytes= skin->swap_audio_bytes;
 *cdxa_conversion= skin->cdxa_conversion;
 return(1);
}


#ifndef Cdrskin_extra_leaN

/** Return information about current fifo setting */
int Cdrskin_get_fifo_par(struct CdrskiN *skin, int *fifo_enabled,
                         int *fifo_size, int *fifo_start_at, int flag)
{
 *fifo_enabled= skin->fifo_enabled;
 *fifo_size= skin->fifo_size;
 *fifo_start_at= skin->fifo_start_at;
 return(1);
}


#ifndef Cdrskin_no_cdrfifO

/** Create and install fifo objects between track data sources and libburn.
    The sources and parameters are known to skin.
    @return <=0 error, 1 success
*/
int Cdrskin_attach_fifo(struct CdrskiN *skin, int flag)
{
 struct CdrfifO *ff= NULL;
 int ret,i,hflag;

#ifdef Cdrskin_use_libburn_fifO

 int profile_number;
 char profile_name[80];

 ret= Cdrskin_assert_driveno(skin, 0);
 if(ret <= 0)
   return(ret);

 /* Refuse here and thus use libburn fifo only with single track, non-CD */
 ret= burn_disc_get_profile(skin->drives[skin->driveno].drive,
                            &profile_number, profile_name);
 if(profile_number != 0x09 && profile_number != 0x0a &&
    skin->track_counter == 1)
   return(1);

#endif /* Cdrskin_use_libburn_fifO */

 skin->fifo= NULL;
 for(i=0;i<skin->track_counter;i++) {
   hflag= (skin->verbosity>=Cdrskin_verbose_debuG);
   if(i==skin->track_counter-1)
     hflag|= 4;
   if(skin->verbosity>=Cdrskin_verbose_cmD) {
     if(skin->fifo_per_track)
       printf("cdrskin: track %d establishing fifo of %d bytes\n",
              i+1,skin->fifo_size);
     else if(i==0)
       printf("cdrskin: establishing fifo of %d bytes\n",skin->fifo_size);
     else {
       if(skin->verbosity>=Cdrskin_verbose_debuG)
        ClN(fprintf(stderr,"cdrskin_debug: attaching track %d to fifo\n",i+1));
       hflag|= 2;
     }
   }
   ret= Cdrtrack_attach_fifo(skin->tracklist[i],&(skin->fifo_outlet_fd),ff,
                             hflag);
   if(ret<=0) {
     fprintf(stderr,"cdrskin: FATAL : failed to attach fifo.\n");
     return(0);
   }
   if(i==0 || skin->fifo_per_track)
     Cdrtrack_get_fifo(skin->tracklist[i],&ff,0);
   if(i==0)
     skin->fifo= ff;
 }
 return(1);
}

#endif /* ! Cdrskin_no_cdrfifO */


/** Read data into the track fifos until either #1 is full or its data source
    is exhausted.
    @return <=0 error, 1 success
*/
int Cdrskin_fill_fifo(struct CdrskiN *skin, int flag)
{
 int ret;

 ret= Cdrtrack_fill_fifo(skin->tracklist[0],skin->fifo_start_at,0);
 if(ret<=0)
   return(ret);
 printf("input buffer ready.\n");
 fflush(stdout);
 return(1);
}

#endif /* ! Cdrskin_extra_leaN */


/** Inform libburn about the consumer x-speed factor of skin */
int Cdrskin_adjust_speed(struct CdrskiN *skin, int flag)
{
 int k_speed, modesty= 0;

 if(skin->x_speed<0)
   k_speed= 0; /* libburn.h promises 0 to be max speed. */
 else if(skin->x_speed==0) { /* cdrecord specifies 0 as minimum speed. */
   k_speed= -1;
 } else
   k_speed= skin->x_speed*Cdrskin_libburn_speed_factoR +
            Cdrskin_libburn_speed_addoN;
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,"cdrskin_debug: k_speed= %d\n",k_speed));

 if(skin->adjust_speed_to_drive && !skin->force_is_set) {
   struct burn_speed_descriptor *best_descr;
   burn_drive_get_best_speed(skin->drives[skin->driveno].drive,k_speed,
                             &best_descr,0);
   if(best_descr!=NULL) {
     k_speed= best_descr->write_speed;
     skin->x_speed = ((double) k_speed) / Cdrskin_libburn_speed_factoR;
   }  
 }
 burn_drive_set_speed(skin->drives[skin->driveno].drive,k_speed,k_speed);
 modesty= skin->modesty_on_drive;
 burn_drive_set_buffer_waiting(skin->drives[skin->driveno].drive,
                               modesty, -1, -1, -1,
                               skin->min_buffer_percent,
                               skin->max_buffer_percent);
 return(1);
}


int Cdrskin_determine_media_caps(struct CdrskiN *skin, int flag)
{
 int ret;
 struct burn_multi_caps *caps = NULL;
 
 skin->media_is_overwriteable= skin->media_does_multi= 0;
 ret= burn_disc_get_multi_caps(skin->grabbed_drive,BURN_WRITE_NONE,&caps,0);
 if(ret<=0)
   goto ex;
 skin->media_is_overwriteable= !!caps->start_adr;
 skin->media_does_multi= !!caps->multi_session;
 ret= 1;
ex:;
 if(caps != NULL)
   burn_disc_free_multi_caps(&caps);
 return(ret);
}


int Cdrskin__is_aborting(int flag)
{
 if(Cdrskin_abort_leveL)
   return(-1);
 return(burn_is_aborting(0));
}


int Cdrskin_abort(struct CdrskiN *skin, int flag)
{
 int ret;

 Cdrskin_abort_leveL= 1;
 ret= burn_abort(skin->abort_max_wait, burn_abort_pacifier, "cdrskin: ");
 if(ret<=0) {
   fprintf(stderr,
       "\ncdrskin: ABORT : Cannot cancel burn session and release drive.\n");
 } else {
   fprintf(stderr,
  "cdrskin: ABORT : Drive is released and library is shut down now.\n");
 }
 fprintf(stderr,
  "cdrskin: ABORT : Program done. Even if you do not see a shell prompt.\n");
 fprintf(stderr,"\n");
 exit(1);
}


/** Obtain access to a libburn drive for writing or information retrieval.
    If libburn is not restricted to a single persistent address then the
    unused drives are dropped. This might be done by shutting down and
    restartiing libburn with the wanted drive only. Thus, after this call,
    libburn is supposed to have open only the reserved drive.
    All other drives should be free for other use.
    Warning: Do not store struct burn_drive pointer over this call.
             Any such pointer might be invalid afterwards.
    @param flag Bitfield for control purposes:
                bit0= bus is unscanned, device is known, 
                      use burn_drive_scan_and_grab()
                bit1= do not load drive tray
                bit2= do not issue error message on failure
                bit3= demand and evtl. report media, return 0 if none to see
                bit4= grab drive with unsuitable media even if fallback program
                bit5= do not re-assess the drive if it is already grabbed
                      but rather perform a release-and-grab cycle
    @return <=0 error, 1 success
*/
int Cdrskin_grab_drive(struct CdrskiN *skin, int flag)
{
 int ret,i,profile_number, mem;
 struct burn_drive *drive;
 char profile_name[80];
 enum burn_disc_status s;

 if(skin->drive_is_grabbed && (flag & 32))
   Cdrskin_release_drive(skin,0);

 if(!skin->drive_is_grabbed) {
   if(flag&1) {
     skin->driveno= 0;
     drive= NULL;
     skin->grabbed_drive= drive;
   } else {
     ret= Cdrskin_assert_driveno(skin, 0);
     if(ret <= 0)
       return(ret);
     drive= skin->drives[skin->driveno].drive;
     skin->grabbed_drive= drive;
   }
 }
 if(skin->drive_is_grabbed) {
   drive= skin->grabbed_drive;
   mem= skin->drive_is_busy;
   skin->drive_is_busy= 2;
   ret= burn_drive_re_assess(skin->grabbed_drive, 0);
   skin->drive_is_busy= mem;
   if(Cdrskin__is_aborting(0)) {
     fprintf(stderr,"cdrskin: ABORT : Drive re-assessment aborted\n");
     Cdrskin_abort(skin, 0); /* Never comes back */
   }
   if(ret <= 0) {
     if(!(flag&4))
       fprintf(stderr,"cdrskin: FATAL : unable to re-assess drive '%s'\n",
               skin->preskin->device_adr);
     goto ex;
   }
 } else if(flag&1) {
   mem= skin->drive_is_busy;
   skin->drive_is_busy= 2;
   ret= burn_drive_scan_and_grab(&(skin->drives),skin->preskin->device_adr,
                                 (skin->do_load != -1) && !(flag&2));
   skin->drive_is_busy= mem;
   if(Cdrskin__is_aborting(0)) {
aborted:;
     fprintf(stderr,"cdrskin: ABORT : Drive aquiration aborted\n");
     Cdrskin_abort(skin, 0); /* Never comes back */
   }
   if(ret<=0) {
     if(!(flag&4))
       fprintf(stderr,"cdrskin: FATAL : unable to open drive '%s'\n",
               skin->preskin->device_adr);
     goto ex;
   }
   skin->driveno= 0;
   drive= skin->drives[skin->driveno].drive;
   skin->grabbed_drive= drive;
 } else {
   if(strlen(skin->preskin->device_adr)<=0) {
     if(skin->verbosity>=Cdrskin_verbose_debuG)   
       ClN(fprintf(stderr,
         "cdrskin_debug: Cdrskin_grab_drive() dropping unwanted drives (%d)\n",
          skin->n_drives-1));
     for(i= 0; i < (int) skin->n_drives; i++) {
       if(i==skin->driveno)
     continue;
       if(skin->verbosity>=Cdrskin_verbose_debuG)   
         ClN(fprintf(stderr,
           "cdrskin_debug: Cdrskin_grab_drive() dropped drive number %d\n",i));
       ret= burn_drive_info_forget(&(skin->drives[i]), 0);
       if(ret==1 || ret==2)
     continue;
       fprintf(stderr,
           "cdrskin: NOTE : Please inform libburn-hackers@pykix.org about:\n");
       fprintf(stderr,
           "cdrskin:        burn_drive_info_forget() returns %d\n",ret);
     }
   }

   mem= skin->drive_is_busy;
   skin->drive_is_busy= 2;
   ret= burn_drive_grab(drive,(skin->do_load != -1) && !(flag&2));
   skin->drive_is_busy= mem;
   if(Cdrskin__is_aborting(0))
     goto aborted;
   if(ret==0) {
     if(!(flag&4))
       fprintf(stderr,"cdrskin: FATAL : unable to open drive %d\n",
               skin->driveno);
     goto ex;
   }
 }
 skin->drive_is_grabbed= 1;

 s= burn_disc_get_status(drive);
 if((flag&8)) {
   if(skin->verbosity>=Cdrskin_verbose_progresS)
     Cdrskin_report_disc_status(skin,s,1);
   if(s==BURN_DISC_EMPTY) {
     Cdrskin_release_drive(skin,0);
     fprintf(stderr,"cdrskin: SORRY : No media found in drive\n");
     ret= 0; goto ex;
   }
 }

 Cdrskin_speed_factoR= Cdrskin_cd_speed_factoR;
 Cdrskin_libburn_speed_factoR= Cdrskin_libburn_cd_speed_factoR;
 Cdrskin_libburn_speed_addoN= Cdrskin_libburn_cd_speed_addoN;
 ret= burn_disc_get_profile(drive,&profile_number,profile_name);
 if(ret>0) {
   if(strstr(profile_name,"DVD")==profile_name ||
      strstr(profile_name,"stdio")==profile_name ) {
     Cdrskin_speed_factoR= Cdrskin_dvd_speed_factoR;
     Cdrskin_libburn_speed_factoR= Cdrskin_libburn_dvd_speed_factoR;
     Cdrskin_libburn_speed_addoN= Cdrskin_libburn_dvd_speed_addoN;
   } else if(strstr(profile_name,"BD")==profile_name) {
     Cdrskin_speed_factoR= Cdrskin_bd_speed_factoR;
     Cdrskin_libburn_speed_factoR= Cdrskin_libburn_bd_speed_factoR;
     Cdrskin_libburn_speed_addoN= Cdrskin_libburn_bd_speed_addoN;
   }
 }
 if(skin->preskin->fallback_program[0] && s==BURN_DISC_UNSUITABLE &&
    skin->preskin->demands_cdrskin_caps<=0 && !(flag&16)) {
   skin->preskin->demands_cdrecord_caps= 1;
   fprintf(stderr,
           "cdrskin: NOTE : Will delegate job to fallback program '%s'.\n",
           skin->preskin->fallback_program);
   Cdrskin_release_drive(skin,0);
   ret= 0; goto ex;
 }

 Cdrskin_determine_media_caps(skin,0);

 ret= 1;
ex:;
 if(ret<=0) {
   skin->drive_is_grabbed= 0;
   skin->grabbed_drive= NULL;
 }
 return(ret);
}


/** Release grabbed libburn drive 
    @param flag Bitfield for control purposes:
                bit0= eject
                bit1= leave tray locked (eventually overrides bit0)
*/
int Cdrskin_release_drive(struct CdrskiN *skin, int flag)
{
 if((!skin->drive_is_grabbed) || skin->grabbed_drive==NULL) {
   fprintf(stderr,"cdrskin: CAUGHT : release of non-grabbed drive.\n");
   return(0);
 }
 if(flag&2)
   burn_drive_leave_locked(skin->grabbed_drive,0);
 else
   burn_drive_release(skin->grabbed_drive,(flag&1));
 skin->drive_is_grabbed= 0;
 skin->grabbed_drive= NULL;
 return(1);
}


/** Clean up resources in abort situations. To be called by Cleanup subsystem
    but hardly ever by the application. The program must exit afterwards.
*/
int Cdrskin_abort_handler(struct CdrskiN *skin, int signum, int flag)
{
 struct burn_progress p;
 enum burn_drive_status drive_status= BURN_DRIVE_SPAWNING;

/*
 fprintf(stderr,
"cdrskin_DEBUG: Cdrskin_abort_handler: signum=%d, flag=%d, drive_is_busy=%d\n",
         signum, flag, skin->drive_is_busy);
*/

 if(skin->drive_is_busy == 0)
   Cdrskin_abort(skin, 0); /* Never comes back */

 if(getpid()!=skin->control_pid) {
   if(skin->verbosity>=Cdrskin_verbose_debuG)   
     ClN(fprintf(stderr,
        "\ncdrskin_debug: ABORT : [%lu] Thread rejected: pid=%lu, signum=%lu\n",
        (unsigned long int) skin->control_pid, (unsigned long int) getpid(),
        (unsigned long int) signum));

#ifdef Not_yeT
   /* >>> find more abstract and system independent way to determine 
          signals which make no sense with a return */
   if(signum==11) {
     Cleanup_set_handlers(NULL,NULL,1); /* allow abort */
     return(0); /* let exit */
   }
#endif
   usleep(1000000);

   return(-2); /* do only process the control thread */
 }
 if(skin->preskin->abort_handler==3)
   Cleanup_set_handlers(NULL,NULL,2); /* ignore all signals */
 else if(skin->preskin->abort_handler==4)
   Cleanup_set_handlers(NULL,NULL,1); /* allow abort */
 fprintf(stderr,
     "\ncdrskin: ABORT : Handling started. Please do not press CTRL+C now.\n");
 if(skin->preskin->abort_handler==3)
   fprintf(stderr,"cdrskin: ABORT : Trying to ignore any further signals\n");

#ifndef Cdrskin_no_cdrfifO
 if(skin->fifo!=NULL)
   Cdrfifo_close_all(skin->fifo,0);
#endif

 /* Only for user info */
 if(skin->grabbed_drive!=NULL)
   drive_status= burn_drive_get_status(skin->grabbed_drive,&p);
 if(drive_status!=BURN_DRIVE_IDLE) {
   fprintf(stderr,"cdrskin: ABORT : Abort processing depends on speed and buffer size\n");
   fprintf(stderr,"cdrskin: ABORT : Usually it is done with 4x speed after about a MINUTE\n");
   fprintf(stderr,"cdrskin: URGE  : But wait at least the normal burning time before any kill -9\n");
 }

 Cdrskin_abort_leveL= -1;
 if (!(flag & 1)) {
   if(skin->verbosity>=Cdrskin_verbose_debuG)
     ClN(fprintf(stderr,"cdrskin: ABORT : Calling burn_abort(-1)\n"));
   burn_abort(-1, burn_abort_pacifier, "cdrskin: ");
 }
 fprintf(stderr,
        "cdrskin: ABORT : Urged drive worker threads to do emergency halt.\n");
 return(-2);
}


/** Convert a libburn device address into a libburn drive number
    @return <=0 error, 1 success
*/
int Cdrskin_driveno_of_location(struct CdrskiN *skin, char *devicename,
                                int *driveno, int flag)
{
 int i,ret;
 char adr[Cdrskin_adrleN];

 for(i= 0; i < (int) skin->n_drives; i++) {
   ret= burn_drive_get_adr(&(skin->drives[i]), adr);
   if(ret<=0)
 continue;
   if(strcmp(adr,devicename)==0) {
     *driveno= i;
     return(1);
   }
 }
 return(0);
}


/** Convert a cdrskin address into a libburn drive number
    @return <=0 error, 1 success
*/
int Cdrskin_dev_to_driveno(struct CdrskiN *skin, char *in_adr, int *driveno,
                           int flag)
{
 int ret;
 char *adr,translated_adr[Cdrskin_adrleN],synthetic_adr[Cdrskin_adrleN];

 adr= in_adr;

#ifndef Cdrskin_extra_leaN
 /* user defined address translation */
 ret= Cdradrtrn_translate(skin->adr_trn,adr,-1,translated_adr,0);
 if(ret<=0) {
   fprintf(stderr,
        "cdrskin: FATAL : address translation failed (address too long ?) \n");
   return(0);
 }
 if(skin->verbosity>=Cdrskin_verbose_cmD && strcmp(adr,translated_adr)!=0)
   printf("cdrskin: dev_translation=... :  dev='%s' to dev='%s'\n",
          adr,translated_adr);
 adr= translated_adr;
#endif /* ! Cdrskin_extra_leaN */

 if(strncmp(adr, "stdio:", 6)==0) {
   if(skin->n_drives<=0)
     goto wrong_devno;
   *driveno= 0;
   return(1);
 } else if(adr[0]=='/') {
   ret= Cdrskin_driveno_of_location(skin,adr,driveno,0);
   if(ret<=0) {
location_not_found:;
     fprintf(stderr,
         "cdrskin: FATAL : cannot find '%s' among accessible drive devices.\n",
            adr);
     fprintf(stderr,
      "cdrskin: HINT : use option  --devices  for a list of drive devices.\n");
     return(0);
   }
   return(1);
 }
 ret= Cdrpreskin__cdrecord_to_dev(adr,synthetic_adr,driveno,
                                  !!skin->preskin->old_pseudo_scsi_adr);
 if(ret<=0) {
wrong_devno:;
   if(skin->n_drives<=0) {
     fprintf(stderr,"cdrskin: FATAL : No accessible drives.\n");
   } else {
     fprintf(stderr,
         "cdrskin: FATAL : Address does not lead to an accessible drive: %s\n",
             in_adr);
     fprintf(stderr,
    "cdrskin: HINT : dev= expects /dev/xyz, Bus,Target,0 or a number [0,%d]\n",
           skin->n_drives-1);
   }
   return(0);
 }
 if(strlen(synthetic_adr)>0) {
   if(skin->verbosity>=Cdrskin_verbose_cmD)
    ClN(printf("cdrskin: converted address '%s' to '%s'\n",adr,synthetic_adr));
   ret= Cdrskin_driveno_of_location(skin,synthetic_adr,driveno,0);
   if(ret<=0) {
     fprintf(stderr,
             "cdrskin: failure while using address converted from '%s'\n",adr);
     adr= synthetic_adr;
     goto location_not_found;
   }
 }
 if((unsigned int) (*driveno) >= skin->n_drives || (*driveno) < 0) {
   ClN(fprintf(stderr,"cdrskin: obtained drive number  %d  from '%s'\n",
               *driveno,adr));
   goto wrong_devno;
 }
 return(1);
}


/** Convert a libburn drive number into a cdrecord-style address which 
    represents a device address if possible and the drive number else.
    @param flag Bitfield for control purposes: 
                bit0= do not apply user defined address translation
    @return <0 error,
                pseudo transport groups:
                 0 volatile drive number, 
                 1 /dev/sgN, 2 /dev/hdX, 3 stdio,
                 1000000+busno = non-pseudo SCSI bus
                 2000000+busno = pseudo-ATA|ATAPI SCSI bus (currently busno==2)
*/
int Cdrskin_driveno_to_btldev(struct CdrskiN *skin, int driveno,
                              char btldev[Cdrskin_adrleN], int flag)
{
 int k,ret,still_untranslated= 1,hret,k_start;
 char *loc= NULL,buf[Cdrskin_adrleN],adr[Cdrskin_adrleN];

 if(driveno < 0 || driveno > (int) skin->n_drives)
   goto fallback;

 ret= burn_drive_get_adr(&(skin->drives[driveno]), adr);
 if(ret<=0)
   goto fallback;
 loc= adr;
 ret= burn_drive_get_drive_role(skin->drives[driveno].drive);
 if(ret!=1) {
   sprintf(btldev,"stdio:%s",adr);
   {ret= 2; goto adr_translation;}
 }

 if(!skin->preskin->old_pseudo_scsi_adr) {
   int host_no= -1,channel_no= -1,target_no= -1,lun_no= -1, bus_no= -1;

   ret= burn_drive_obtain_scsi_adr(loc,&bus_no,&host_no,&channel_no,
                                   &target_no,&lun_no); 
   if(ret<=0) {
     if(strncmp(loc,"/dev/hd",7)==0)
       if(loc[7]>='a' && loc[7]<='z')
         if(loc[8]==0) {
           bus_no= (loc[7]-'a')/2;
           sprintf(btldev,"%d,%d,0",bus_no,(loc[7]-'a')%2);
           {ret= 2000000 + bus_no; goto adr_translation;}
         }
     goto fallback;
   } else {
     sprintf(btldev,"%d,%d,%d",bus_no,target_no,lun_no);
     ret= 1000000+bus_no;
     goto adr_translation;
   }
 }

 k_start= 0;
 if(strncmp(loc,"/dev/sg",7)==0 || strncmp(loc,"/dev/sr",7)==0)
   k_start= 7;
 if(strncmp(loc,"/dev/scd",8)==0)
   k_start= 8;
 if(k_start>0) {
   for(k= k_start;loc[k]!=0;k++)
     if(loc[k]<'0' || loc[k]>'9')
   break;
   if(loc[k]==0 && k>k_start) {
     sprintf(btldev,"1,%s,0",loc+k_start);
     {ret= 1; goto adr_translation;}
   }
 }
 if(strncmp(loc,"/dev/hd",7)==0)
   if(loc[7]>='a' && loc[7]<='z')
     if(loc[8]==0) {
       sprintf(btldev,"2,%d,0",loc[7]-'a');
       {ret= 2; goto adr_translation;}
     }
fallback:;
 if(skin->preskin->old_pseudo_scsi_adr) {
   sprintf(btldev,"0,%d,0",driveno);
 } else {
   if(loc!=NULL)
     strcpy(btldev,loc);
   else
     sprintf(btldev,"%d",driveno);
 }
 ret= 0;

adr_translation:;
#ifndef Cdrskin_extra_leaN
 /* user defined address translation */
 if(!(flag&1)) {
   if(ret>0) {
     /* try whether a translation points to loc */
     hret= Cdradrtrn_translate(skin->adr_trn,loc,driveno,buf,1);
     if(hret==2) {
       still_untranslated= 0;
       strcpy(btldev,buf);
     }
   }
   if(still_untranslated) {
     Cdradrtrn_translate(skin->adr_trn,btldev,driveno,buf,1);
     strcpy(btldev,buf);
   }
 }
#endif /* ! Cdrskin_extra_leaN */

 return(ret);
}


/** Read and buffer the start of an existing ISO-9660 image from
    overwriteable target media.
*/
int Cdrskin_overwriteable_iso_size(struct CdrskiN *skin, int *size, int flag)
{
 int ret;
 off_t data_count= 0;
 double size_in_bytes;
 char *buf;

 buf= skin->overwriteable_iso_head;
 if(!skin->media_is_overwriteable)
   {ret= 0; goto ex;}
 /* Read first 64 kB */
 ret= burn_read_data(skin->grabbed_drive,(off_t) 0,buf,32*2048,&data_count,0);
 if(ret<=0)
   {ret= 0; goto ex;}
 ret= Scan_for_iso_size((unsigned char *) (buf+16*2048), &size_in_bytes,0);
 if(ret<=0) {
   if(skin->verbosity>=Cdrskin_verbose_debuG)
  ClN(fprintf(stderr,"cdrskin_debug: No detectable ISO-9660 size on media\n"));
   {ret= 0; goto ex;}
 }
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,"cdrskin_debug: detected ISO-9660 size : %.f  (%fs)\n",
               size_in_bytes, size_in_bytes/2048.0));
 if(size_in_bytes/2048.0>2147483647-1-16) {
   fprintf(stderr,
           "cdrskin: FATAL : ISO-9660 filesystem in terabyte size detected\n");
   {ret= 0; goto ex;}
 }
 *size= size_in_bytes/2048.0;
 if(size_in_bytes-((double) *size)*2048.0>0.0)
   (*size)++;
 if((*size)%16)
   *size+= 16-((*size)%16);
 if(skin->grow_overwriteable_iso==1)
   skin->grow_overwriteable_iso= 2;
 ret= 1;
ex:;
 return(ret);
}     


int Cdrskin_invalidate_iso_head(struct CdrskiN *skin, int flag)
{
 int ret;
 int size;

 fprintf(stderr,
   "cdrskin: blank=... : invalidating ISO-9660 head on overwriteable media\n");
 ret= Cdrskin_overwriteable_iso_size(skin,&size,0);
 if(ret<=0) {
   fprintf(stderr,
           "cdrskin: NOTE : Not an ISO-9660 file system. Left unaltered.\n");
   return(2);
 }
 skin->overwriteable_iso_head[16*2048]=
 skin->overwriteable_iso_head[16*2048+3]= 
 skin->overwriteable_iso_head[16*2048+4]= 'x';
 ret= burn_random_access_write(skin->grabbed_drive,(off_t) 16*2048,
                               skin->overwriteable_iso_head+16*2048,
                               (off_t) 16*2048,1);
 return(ret);
}


/** Report media status s to the user
    @param flag Bitfield for control purposes: 
                bit0= permission to check for overwriteable ISO image
                bit1= do not report media profile
                bit2= do not report but only check for pseudo appendable
    @return 1=ok, 2=ok, is pseudo appendable, <=0 error
*/
int Cdrskin_report_disc_status(struct CdrskiN *skin, enum burn_disc_status s,
                               int flag)
{
 int ret, iso_size, pseudo_appendable= 0;

 if(flag&1) {
   if(skin->media_is_overwriteable && skin->grow_overwriteable_iso>0) {
     if(skin->grow_overwriteable_iso==2)
       ret= 1;
     else
       ret= Cdrskin_overwriteable_iso_size(skin,&iso_size,0);
     if(ret>0) {
       s= BURN_DISC_APPENDABLE;
       pseudo_appendable= 1;
     }
   }
 } 
 if(flag&4)
   return(1+pseudo_appendable);

 printf("cdrskin: status %d ",s);
 if(s==BURN_DISC_FULL) {
   printf("burn_disc_full \"There is a disc with data on it in the drive\"\n"); 
 } else if(s==BURN_DISC_BLANK) {
   printf("burn_disc_blank \"The drive holds a blank disc\"\n"); 
 } else if(s==BURN_DISC_APPENDABLE) {
   printf(
        "BURN_DISC_APPENDABLE \"There is an incomplete disc in the drive\"\n");
 } else if(s==BURN_DISC_EMPTY) {
   printf("BURN_DISC_EMPTY \"There is no disc at all in the drive\"\n");
 } else if(s==BURN_DISC_UNREADY) {
   printf("BURN_DISC_UNREADY \"The current status is not yet known\"\n");
 } else if(s==BURN_DISC_UNGRABBED) {
   printf("BURN_DISC_UNGRABBED \"API usage error: drive not grabbed\"\n");
 } else if(s==BURN_DISC_UNSUITABLE) {
   printf("BURN_DISC_UNSUITABLE \"Media is not suitable\"\n");
 } else 
   printf("-unknown status code-\n");

 if(flag&2)
   return(1+pseudo_appendable);

 if((s==BURN_DISC_FULL || s==BURN_DISC_APPENDABLE || s==BURN_DISC_BLANK ||
     s==BURN_DISC_UNSUITABLE) && skin->driveno>=0) {
   char profile_name[80];
   int profile_number;

   printf("Current: ");
   ret= burn_disc_get_profile(skin->drives[skin->driveno].drive,
                              &profile_number,profile_name);
   if(ret>0 && profile_name[0]!=0)
     printf("%s\n", profile_name);
   else if(ret>0)
     printf("UNSUITABLE MEDIA (Profile %4.4Xh)\n",profile_number);
   else
     printf("-unidentified-\n");
 } else if(s==BURN_DISC_EMPTY) {
   printf("Current: none\n");
 }

 return(1+pseudo_appendable);
}


/** Perform operations -scanbus or --devices 
    @param flag Bitfield for control purposes: 
                bit0= perform --devices rather than -scanbus
                bit1= with bit0: perform --device_links
    @return <=0 error, 1 success
*/
int Cdrskin_scanbus(struct CdrskiN *skin, int flag)
{
 int ret,i,busno,first_on_bus,pseudo_transport_group= 0,skipped_devices= 0;
 int busmax= 16, busidx, max_dev_len, pad, j;
 char shellsafe[5*Cdrskin_strleN+2],perms[40],btldev[Cdrskin_adrleN];
 char adr[Cdrskin_adrleN],*raw_dev,*drives_shown= NULL;
 char link_adr[BURN_DRIVE_ADR_LEN];
 int *drives_busses= NULL;
 struct stat stbuf;

 drives_shown= calloc(1, skin->n_drives+1);
 drives_busses= calloc((skin->n_drives+1), sizeof(int));
 if(drives_shown == NULL || drives_busses == NULL)
   {ret= -1; goto ex;}

 if(flag&1) {
   max_dev_len= 0;
   for(i= 0; i < (int) skin->n_drives; i++) {
     drives_shown[i]= 0;
     ret= burn_drive_get_adr(&(skin->drives[i]), adr);
     if(ret<=0)
   continue;
     if(flag & 2) {
       ret= burn_lookup_device_link(adr, link_adr, "/dev", NULL, 0, 0);
       if(ret == 1)
         strcpy(adr, link_adr);
     }
     if(strlen(adr)>=Cdrskin_strleN)
       Text_shellsafe("failure:oversized string", shellsafe, 0);
     else
       Text_shellsafe(adr, shellsafe,0);
     if((int) strlen(shellsafe) > max_dev_len)
       max_dev_len= strlen(shellsafe);
   }
   printf("cdrskin: Overview of accessible drives (%d found) :\n",
          skin->n_drives);
   printf("-----------------------------------------------------------------------------\n");
   for(i= 0; i < (int) skin->n_drives; i++) {
     ret= burn_drive_get_adr(&(skin->drives[i]), adr);
     if(ret<=0) {
       /* >>> one should massively complain */;
   continue;
     }
     if(stat(adr,&stbuf)==-1) {
       sprintf(perms,"errno=%d",errno);
     } else {
       strcpy(perms,"------");
       if(stbuf.st_mode&S_IRUSR) perms[0]= 'r';
       if(stbuf.st_mode&S_IWUSR) perms[1]= 'w';
       if(stbuf.st_mode&S_IRGRP) perms[2]= 'r';
       if(stbuf.st_mode&S_IWGRP) perms[3]= 'w';
       if(stbuf.st_mode&S_IROTH) perms[4]= 'r';
       if(stbuf.st_mode&S_IWOTH) perms[5]= 'w';
     }
     if(flag & 2) {
       ret= burn_lookup_device_link(adr, link_adr, "/dev", NULL, 0, 0);
       if(ret == 1)
         strcpy(adr, link_adr);
     }
     if(strlen(adr)>=Cdrskin_strleN)
       Text_shellsafe("failure:oversized string",shellsafe,0);
     else
       Text_shellsafe(adr,shellsafe,0);
     printf("%d  dev=%s", i, shellsafe);
     pad= max_dev_len - strlen(shellsafe);
     if(pad > 0)
       for(j= 0; j < pad; j++)
         printf(" ");
     printf("  %s :  '%-8.8s'  '%s'\n",
            perms, skin->drives[i].vendor, skin->drives[i].product);
   }
   printf("-----------------------------------------------------------------------------\n");
 } else {
   if(!skin->preskin->old_pseudo_scsi_adr) {
     pseudo_transport_group= 1000000;
     raw_dev= skin->preskin->raw_device_adr;
     if(strncmp(raw_dev,"ATA",3)==0 && (raw_dev[3]==0 || raw_dev[3]==':'))
       pseudo_transport_group= 2000000;
     if(strncmp(raw_dev,"ATAPI",5)==0 && (raw_dev[5]==0 || raw_dev[5]==':'))
       pseudo_transport_group= 2000000;
     if(pseudo_transport_group==2000000) {
       fprintf(stderr,"scsidev: 'ATA'\ndevname: 'ATA'\n");
       fprintf(stderr,"scsibus: -2 target: -2 lun: -2\n");
     }
   }
   /* >>> fprintf(stderr,"Linux sg driver version: 3.1.25\n"); */
   printf("Using libburn version '%s'.\n", Cdrskin_libburn_versioN);
   if(pseudo_transport_group!=1000000)
   if(skin->preskin->old_pseudo_scsi_adr)
     printf("cdrskin: NOTE : The printed addresses are not cdrecord compatible !\n");

   for(i= 0; i < (int) skin->n_drives; i++) {
     drives_busses[i]= -1;
     ret= Cdrskin_driveno_to_btldev(skin,i,btldev,1);
     if(ret >= pseudo_transport_group &&
        ret < pseudo_transport_group + 1000000) {
       drives_busses[i]= ret - pseudo_transport_group;
       if(ret > pseudo_transport_group + busmax)
         busmax= 1 + ret - pseudo_transport_group;
     }
   }
   for(busidx= 0; busidx < (int) skin->n_drives + 1; busidx++) {
     if(busidx < (int) skin->n_drives)
       busno= drives_busses[busidx];
     else
       busno= busmax;
     if(busno < 0)
   continue;
     for(i= 0; i < busidx; i++)
       if(drives_busses[i] == busno)
     break;
     if(i < busidx)
   continue;
     first_on_bus= 1;
     for(i= 0; i < (int) skin->n_drives; i++) {
       ret= Cdrskin_driveno_to_btldev(skin,i,btldev,1);
       if(busno==busmax && drives_shown[i]==0) {
         if(ret/1000000 != pseudo_transport_group) {
           skipped_devices++;
           if(skin->verbosity>=Cdrskin_verbose_debuG)
             ClN(fprintf(stderr,"cdrskin_debug: skipping drive '%s%s'\n",
                         ((ret/1000000)==2?"ATA:":""), btldev));
     continue;
         }
       } else if(ret != pseudo_transport_group + busno)
     continue;
       if(first_on_bus)
         printf("scsibus%d:\n",busno);
       first_on_bus= 0;
       printf("\t%s\t  %d) '%-8s' '%-16s' '%-4s' Removable CD-ROM\n",
              btldev,i,skin->drives[i].vendor,skin->drives[i].product,
              skin->drives[i].revision);
       drives_shown[i]= 1;
     }
   }
 }
 if(skipped_devices>0) {
   if(skipped_devices>1)
     printf("cdrskin: NOTE : There were %d drives not shown.\n",
            skipped_devices);
   else
     printf("cdrskin: NOTE : There was 1 drive not shown.\n");
   printf("cdrskin: HINT : To surely see all drives try option: --devices\n");
   if(pseudo_transport_group!=2000000)
     printf("cdrskin: HINT : or try options:               dev=ATA -scanbus\n");
 }
 ret= 1;
ex:;
 if(drives_shown!=NULL)
   free((char *) drives_shown);
 if(drives_busses!=NULL)
   free((char *) drives_busses);
 return(ret);
}


/** Perform -checkdrive .
    @param flag Bitfield for control purposes:
                bit0= do not print message about pseudo-checkdrive
    @return <=0 error, 1 success
*/
int Cdrskin_checkdrive(struct CdrskiN *skin, char *profile_name, int flag)
{
 struct burn_drive_info *drive_info;
 int ret;
 char btldev[Cdrskin_adrleN];

 if(!(flag&1)) {
   if(flag&2)
     ClN(printf("cdrskin: pseudo-inquiry on drive %d\n",skin->driveno));
   else
     ClN(printf("cdrskin: pseudo-checkdrive on drive %d\n",skin->driveno));
 }
 if(skin->driveno >= (int) skin->n_drives || skin->driveno < 0) {
   fprintf(stderr,"cdrskin: FATAL : there is no drive #%d\n",skin->driveno);
   {ret= 0; goto ex;}  
 } 
 drive_info= &(skin->drives[skin->driveno]);
 ret= Cdrskin_driveno_to_btldev(skin,skin->driveno,btldev,0);
 if(ret>=0)
   fprintf(stderr,"scsidev: '%s'\n",btldev);
 printf("Device type    : ");
 ret= burn_drive_get_drive_role(drive_info->drive);
 if(ret==0)
   printf("%s\n","Emulated (null-drive)");
 else if(ret==2)
   printf("%s\n","Emulated (stdio-drive, 2k random read-write)");
 else if(ret==3)
   printf("%s\n","Emulated (stdio-drive, sequential write-only)");
 else if(ret==4)
   printf("%s\n","Emulated (stdio-drive, 2k random read-only)");
 else if(ret == 5)
   printf("%s\n","Emulated (stdio-drive, 2k random write-only)");
 else if(ret!=1)
   printf("%s\n","Emulated (stdio-drive)");
 else
   printf("%s\n","Removable CD-ROM");
 printf("Vendor_info    : '%s'\n",drive_info->vendor);
 printf("Identifikation : '%s'\n",drive_info->product);
 printf("Revision       : '%s'\n",drive_info->revision);

 if(flag&2)
   {ret= 1; goto ex;}

 printf("Driver flags   : %s\n","BURNFREE");
 printf("Supported modes:");
 if((drive_info->tao_block_types & (BURN_BLOCK_MODE1))
    == (BURN_BLOCK_MODE1))
   printf(" TAO");
 if(drive_info->sao_block_types & BURN_BLOCK_SAO)
   printf(" SAO");

#ifndef Cdrskin_disable_raw96R
 if((drive_info->raw_block_types & BURN_BLOCK_RAW96R) &&
    strstr(profile_name,"DVD")!=profile_name &&
    strstr(profile_name,"BD")!=profile_name)
   printf(" RAW/RAW96R");
#endif /* ! Cdrskin_disable_raw96R */

 printf("\n");
 ret= 1;
ex:;
 return(ret);
}


/** Predict address block number where the next write will go to
    @param flag Bitfield for control purposes:
                bit0= do not return nwa from eventual write_start_address
    @return <=0 error, 1 nwa from drive , 2 nwa from write_start_address
*/
int Cdrskin_obtain_nwa(struct CdrskiN *skin, int *nwa, int flag)
{
 int ret,lba;
 struct burn_drive *drive;
 struct burn_write_opts *o= NULL;

 if(skin->write_start_address>=0 && !(flag&1)) {
   /* (There is no sense in combining random addressing with audio) */
   *nwa= skin->write_start_address/2048;
   return(2);
 }

 /* Set write opts in order to provoke MODE SELECT. LG GSA-4082B needs it. */
 drive= skin->drives[skin->driveno].drive;
 o= burn_write_opts_new(drive);
 if(o!=NULL) {
   burn_write_opts_set_perform_opc(o, 0);
   burn_write_opts_set_write_type(o,skin->write_type,skin->block_type);
   burn_write_opts_set_underrun_proof(o,skin->burnfree);
 }
 ret= burn_disc_track_lba_nwa(drive,o,0,&lba,nwa);
 if(o!=NULL)
   burn_write_opts_free(o);
 return(ret);
}


/* @param flag bit0-3= source of text packs:
                        0= CD Lead-in
                        1= session and tracks
                        2= textfile=
               bit4-7= output format
                        0= -vv -toc
                        1= Sony CD-TEXT Input Sheet Version 0.7T
*/
int Cdrskin_print_text_packs(struct CdrskiN *skin, unsigned char *text_packs,
                             int num_packs, int flag)
{
 int i, j, from, fmt, ret, char_code= 0;
 char *from_text= "", *result= NULL;
 unsigned char *pack;

 from= flag & 15;
 fmt= (flag >> 4) & 15;
 if(from == 0)
   from_text= " from CD Lead-in";
 else if(from == 1)
   from_text= " from session and tracks";
 else if(from == 2)
   from_text= " from textfile= or cuefile= CDTEXTFILE";
 if (fmt == 1) {
   ret = burn_make_input_sheet_v07t(text_packs, num_packs, 0, 0, &result,
                                    &char_code, 0);
   if(ret <= 0)
     goto ex;
   if(char_code == 0x80)
     fprintf(stderr,
             "cdrskin : WARNING : Double byte characters encountered.\n");
   for(i = 0; i < ret; i++)
     printf("%c", result[i]);
 } else {
   printf("CD-TEXT data%s:\n", from_text);
   for(i= 0; i < num_packs; i++) {
     pack= text_packs + 18 * i;
     printf("%4d :", i);
     for(j= 0; j < 18; j++) {
       if(j >= 4 && j <= 15 && pack[j] >= 32 && pack[j] <= 126 &&
          pack[0] != 0x88 && pack[0] != 0x89 && pack[0] != 0x8f)
         printf("  %c",  pack[j]);
       else
         printf(" %2.2x", pack[j]);
     }
     printf("\n");
   }
 }
 ret= 1;
ex:;
 if(result != NULL)
   free(result);
 return(ret);
}


/* @param flag bit4= no not overwrite existing target file
*/
int Cdrskin_store_text_packs(struct CdrskiN *skin, unsigned char *text_packs,
                             int num_packs, char *path, int flag)
{
 int data_length, ret;
 struct stat stbuf;
 FILE *fp;
 unsigned char fake_head[4];

 if(stat(path, &stbuf) != -1 && (flag & 16)) {
   fprintf(stderr, "cdrskin: SORRY : Will not overwrite file '%s'\n", path);
   return(0);
 }
 fp= fopen(path, "w");
 if(fp == NULL) {
   fprintf(stderr, "cdrskin: SORRY : Cannot open file '%s' for storing extracted CD-TEXT\n", path);
   fprintf(stderr, "cdrskin: %s (errno=%d)\n", strerror(errno), errno);
   return(0);
 }
 data_length= num_packs * 18 + 2;
 fake_head[0]= (data_length >> 8) & 0xff;
 fake_head[1]= data_length & 0xff;
 fake_head[2]= fake_head[3]= 0;
 ret= fwrite(fake_head, 4, 1, fp);
 if(ret != 1) {
write_failure:;
   fprintf(stderr,
           "cdrskin: SORRY : Cannot write all data to file '%s'\n", path);
   fprintf(stderr, "cdrskin: %s (errno=%d)\n", strerror(errno), errno);
   fclose(fp);
   return(0);
 }
 ret= fwrite(text_packs, data_length - 2, 1, fp);
 if(ret != 1)
   goto write_failure;
 fprintf(stderr,
    "cdrskin: NOTE : Wrote header and %d CD-TEXT bytes to file '%s'\n",
         data_length - 2, path);
 fclose(fp);
 return(1);
}


/* @param flag bit0-3= output format
                       1= Sony CD-TEXT Input Sheet Version 0.7T
                      15= Cdrskin_store_text_packs
               bit4= do not overwrite existing the target file
*/
int Cdrskin_cdtext_to_file(struct CdrskiN *skin, char *path, int flag)
{
 int ret, fmt, char_code= 0, to_write;
 struct burn_drive *drive;
 unsigned char *text_packs= NULL;
 int num_packs= 0;
 char *result= 0;
 FILE *fp= NULL;
 struct stat stbuf;

 ret= Cdrskin_grab_drive(skin, 0);
 if(ret<=0)
   goto ex;

 fmt= flag & 15;
 drive= skin->drives[skin->driveno].drive;
 ret= burn_disc_get_leadin_text(drive, &text_packs, &num_packs, 0);
 if(ret <= 0 || num_packs <= 0) {
   fprintf(stderr, "cdrskin: No CD-Text or CD-Text unaware drive.\n");
   ret= 2; goto ex;
 }  
 if(fmt == 1) {
   ret = burn_make_input_sheet_v07t(text_packs, num_packs, 0, 0, &result,
                                    &char_code, 0);
   if(ret <= 0)
     goto ex;
   to_write= ret;
   if(stat(path, &stbuf) != -1 && (flag & 16)) {
     fprintf(stderr, "cdrskin: SORRY : Will not overwrite file '%s'\n", path);
     return(0);
   }
   if(strcmp(path, "-") == 0)
     fp= stdout;
   else
     fp = fopen(path, "w");
   if(fp == NULL) {
     if(errno > 0)
       fprintf(stderr, "cdrskin: %s (errno=%d)\n", strerror(errno), errno);
     fprintf(stderr,
             "cdrskin: SORRY : Cannot write CD-TEXT list to file '%s'\n",
             path);
     {ret= 0; goto ex;}
   }
   ret = fwrite(result, to_write, 1, fp);
   if(ret != 1) {
     if(errno > 0)
       fprintf(stderr, "cdrskin: %s (errno=%d)\n", strerror(errno), errno);
     fprintf(stderr,
            "cdrskin: SORRY : Cannot write all CD-TEXT to file '%s'\n", path);
     ret= 0;
   }
   if(ret <= 0)
     goto ex;

 } else if(fmt == 15) {
   if(skin->verbosity >= Cdrskin_verbose_debuG)
     Cdrskin_print_text_packs(skin, text_packs, num_packs, 0);
   ret= Cdrskin_store_text_packs(skin, text_packs, num_packs, path, flag & 16);
   free(text_packs);
   if(ret <= 0)
     goto ex;
   printf("CD-Text len: %d\n", num_packs * 18 + 4);

 } else {
   fprintf(stderr, "cdrskin: FATAL : Program error : Unknow format %d with Cdrskin_cdtext_to_file.\n", fmt);
   {ret= -1; goto ex;}
 }
 ret= 1;
ex:;
 if(result != NULL)
   free(result);
 if(text_packs != NULL)
   free(text_packs);
 if(fp != NULL && fp != stdout)
   fclose(fp);
 return(1);
}


/** Perform -toc under control of Cdrskin_atip().
    @param flag Bitfield for control purposes:
                bit0= do not list sessions separately (do it cdrecord style)
    @return <=0 error, 1 success
*/
int Cdrskin_toc(struct CdrskiN *skin, int flag)
{
 int num_sessions= 0,num_tracks= 0,lba= 0,track_count= 0,total_tracks= 0;
 int session_no, track_no, pmin, psec, pframe, ret, final_ret= 1;
 int track_offset = 1, open_sessions= 0, have_real_open_session= 0;
 struct burn_drive *drive;
 struct burn_disc *disc= NULL;
 struct burn_session **sessions;
 struct burn_track **tracks;
 struct burn_toc_entry toc_entry;
 enum burn_disc_status s;
 char profile_name[80];
 int profile_number;

 drive= skin->drives[skin->driveno].drive;

 s= burn_disc_get_status(drive);
 if(s == BURN_DISC_EMPTY || s == BURN_DISC_BLANK)
   goto summary;
 disc= burn_drive_get_disc(drive);
 if(disc==NULL) {
   if(skin->grow_overwriteable_iso>0) {
     ret= Cdrskin_overwriteable_iso_size(skin,&lba,0);
     if(ret>0) {
       printf(
"first: 1 last: 1  (fabricated from ISO-9660 image on overwriteable media)\n");
       printf(
"track:   1 lba:         0 (        0) 00:02:00 adr: 1 control: 4 mode: 1\n");
       burn_lba_to_msf(lba, &pmin, &psec, &pframe);
       printf("track:lout lba: %9d (%9d) %2.2d:%2.2d:%2.2d",
          lba,4*lba,pmin,psec,pframe);
       printf(" adr: 1 control: 4 mode: -1\n");
       goto summary;
     }
   }
   goto cannot_read;
 }
 sessions= burn_disc_get_sessions(disc,&num_sessions);
 open_sessions= burn_disc_get_incomplete_sessions(disc);
 if(num_sessions > 0)
   track_offset = burn_session_get_start_tno(sessions[0], 0);
 if(track_offset <= 0)
   track_offset= 1;
 if(flag&1) {
   for(session_no= 0; session_no < num_sessions + open_sessions;
       session_no++) {
     tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
     total_tracks+= num_tracks;
     if(session_no == num_sessions + open_sessions - 1 && open_sessions > 0) {
       total_tracks--; /* Do not count invisible track */
       if(num_tracks > 1)
         have_real_open_session= 1;
     }
   }
   printf("first: %d last %d\n",
          track_offset, total_tracks + track_offset - 1);
 }
 for(session_no= 0; session_no < num_sessions + open_sessions; session_no++) {
   tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
   if(tracks==NULL)
 continue;
   if(session_no == num_sessions + open_sessions - 1 && open_sessions > 0)
     num_tracks--;
   if(num_tracks <= 0)
 continue;
   if(!(flag&1))
     printf("first: %d last: %d\n",
            track_count + track_offset,
            track_count + num_tracks + track_offset - 1);
   for(track_no= 0; track_no<num_tracks; track_no++) {
     track_count++;
     burn_track_get_entry(tracks[track_no], &toc_entry);
     if(toc_entry.extensions_valid&1) { /* DVD extension valid */
       lba= toc_entry.start_lba;
       burn_lba_to_msf(lba, &pmin, &psec, &pframe);
     } else {
       pmin= toc_entry.pmin;
       psec= toc_entry.psec;
       pframe= toc_entry.pframe;
       lba= burn_msf_to_lba(pmin,psec,pframe);
     }
     if(track_no==0 && burn_session_get_hidefirst(sessions[session_no]))
       printf("cdrskin: NOTE : first track is marked as \"hidden\".\n");
     printf("track:  %2d lba: %9d (%9d) %2.2d:%2.2d:%2.2d",
            track_count + track_offset - 1, lba, 4 * lba, pmin, psec, pframe);
     printf(" adr: %d control: %d",toc_entry.adr,toc_entry.control);

     /* >>> From where does cdrecord take "mode" ? */

     /* This is not the "mode" as printed by cdrecord :
       printf(" mode: %d\n",burn_track_get_mode(tracks[track_no]));
     */
     /* own guess: cdrecord says "1" on data and "0" on audio : */
     printf(" mode: %d\n",((toc_entry.control&7)<4?0:1));

   }
   if((flag&1) &&
    session_no < num_sessions + open_sessions - 1 + have_real_open_session - 1)
 continue;
   if(have_real_open_session) {
     /* Use start of invisible track */
     burn_track_get_entry(tracks[num_tracks], &toc_entry);
   } else {
     burn_session_get_leadout_entry(sessions[session_no],&toc_entry);
   }
   if(toc_entry.extensions_valid&1) { /* DVD extension valid */
     lba= toc_entry.start_lba;
     burn_lba_to_msf(lba, &pmin, &psec, &pframe);
   } else {
     pmin= toc_entry.pmin;
     psec= toc_entry.psec;
     pframe= toc_entry.pframe;
     lba= burn_msf_to_lba(pmin,psec,pframe);
   }
   printf("track:lout lba: %9d (%9d) %2.2d:%2.2d:%2.2d",
          lba,4*lba,pmin,psec,pframe);
   printf(" adr: %d control: %d",toc_entry.adr,toc_entry.control);
   printf(" mode: -1\n");
 }

 if(skin->verbosity >= Cdrskin_verbose_cmD) {
   ret= Cdrskin_cdtext_to_file(skin, "cdtext.dat", 15 | 16);
   if(ret <= 0 && ret < final_ret)
     final_ret= ret;
 }

summary:
 ret= burn_disc_get_profile(drive, &profile_number, profile_name);
 if(ret <= 0)
   strcpy(profile_name, "media");

 printf("Media summary: %d sessions, %d tracks, %s %s\n",
        num_sessions + open_sessions - 1 + have_real_open_session, track_count, 
        s==BURN_DISC_BLANK ? "blank" :
        s==BURN_DISC_APPENDABLE ? "appendable" :
        s==BURN_DISC_FULL ? "closed" :
        s==BURN_DISC_EMPTY ? "no " : "unknown ",
        profile_name);
        
 if(have_real_open_session)
   printf("Warning      : Incomplete session encountered !\n");
 
 if(disc!=NULL)
   burn_disc_free(disc);
 if(s == BURN_DISC_EMPTY)
   return(0);
 return(final_ret);
cannot_read:;
 fprintf(stderr,"cdrecord_emulation: Cannot read TOC header\n");
 fprintf(stderr,"cdrecord_emulation: Cannot read TOC/PMA\n");
 return(0);
}


/** Perform -minfo under control of Cdrskin_atip().
    @param flag Bitfield for control purposes:
    @return <=0 error, 1 success
*/
int Cdrskin_minfo(struct CdrskiN *skin, int flag)
{
 int num_sessions= 0,num_tracks= 0,lba= 0,track_count= 0,total_tracks= 0;
 int session_no, track_no, pmin, psec, pframe, ret, size= 0, nwa= 0;
 int last_leadout= 0, ovwrt_full= 0, track_offset= 1, open_sessions= 0;
 struct burn_drive *drive;
 struct burn_disc *disc= NULL;
 struct burn_session **sessions= NULL;
 struct burn_track **tracks;
 struct burn_toc_entry toc_entry;
 enum burn_disc_status s, show_status;
 char profile_name[80];
 int pno;
 char media_class[80];
 int nominal_sessions= 1, ftils= 1, ltils= 1, first_track= 1, read_capacity= 0;
 int app_code, cd_info_valid, lra, alloc_blocks, free_blocks;
 int have_real_open_session= 0;
 off_t avail, buf_count;
 char disc_type[80], bar_code[9], buf[2 * 2048], *type_text;
 unsigned int disc_id;

 drive= skin->drives[skin->driveno].drive;

 s= burn_disc_get_status(drive);
 if(s == BURN_DISC_EMPTY) {
   fprintf(stderr, "cdrecord-Emulation: No disk / Wrong disk!\n");
   return(1);
 }
 ret= burn_disc_get_profile(drive, &pno, profile_name);
 if(ret <= 0) {
   fprintf(stderr, "cdrskin: SORRY : Cannot inquire current media profile\n");
   return(1);
 }
 if(pno >= 0x08 && pno <= 0x0a)
   strcpy(media_class, "CD");
 else if(pno >= 0x10 && pno <= 0x2f)
   strcpy(media_class, "DVD");
 else if(pno >= 0x40 && pno <= 0x43)
   strcpy(media_class, "BD");
 else
   sprintf(media_class, "Unknown class (profile 0x%4.4X)", pno);

 printf("\n");
 printf("Mounted media class:      %s\n", media_class);
 printf("Mounted media type:       %s\n", profile_name);
 ret= burn_disc_erasable(drive);
 printf("Disk Is %serasable\n",
        (ret || skin->media_is_overwriteable) ? "" : "not ");
 show_status= s;
 ret = burn_get_read_capacity(drive, &read_capacity, 0);
 if(ret <= 0)
   read_capacity= 0;
 if(skin->media_is_overwriteable && read_capacity > 0)
   ovwrt_full= 1;
 if(ovwrt_full)
   show_status= BURN_DISC_FULL;
 printf("disk status:              %s\n",
        show_status == BURN_DISC_BLANK ?      "empty" :
        show_status == BURN_DISC_APPENDABLE ? "incomplete/appendable" :
        show_status == BURN_DISC_FULL ?       "complete" :
                                              "unusable");
 printf("session status:           %s\n",
        show_status == BURN_DISC_BLANK ?      "empty" :
        show_status == BURN_DISC_APPENDABLE ? "empty" :
        show_status == BURN_DISC_FULL ?       "complete" :
                                              "unknown");

 disc= burn_drive_get_disc(drive);
 if(disc==NULL || s == BURN_DISC_BLANK) {
   first_track= 1;
   num_sessions= 0;
   nominal_sessions= 1;
   ftils= ltils= 1;
 } else {

   sessions= burn_disc_get_sessions(disc, &num_sessions);
   open_sessions= burn_disc_get_incomplete_sessions(disc);
   if(num_sessions > 0)
     track_offset= burn_session_get_start_tno(sessions[0], 0);
   if(track_offset <= 0)
     track_offset= 1;
   first_track= track_offset;
   nominal_sessions= num_sessions + open_sessions;
   if(s == BURN_DISC_APPENDABLE && open_sessions == 0)
     nominal_sessions++;
   for(session_no= 0; session_no < num_sessions + open_sessions;
       session_no++) {
     ftils= total_tracks + 1;
     tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
     if(tracks==NULL)
   continue;
     total_tracks+= num_tracks;
     ltils= total_tracks;
     if(session_no==0 && burn_session_get_hidefirst(sessions[session_no])
        && total_tracks >= 2)
       first_track= 2;
   }
   if(s == BURN_DISC_APPENDABLE && open_sessions == 0)
     ftils= ltils= total_tracks + 1;
 }
 printf("first track:              %d\n", first_track);
 printf("number of sessions:       %d\n", nominal_sessions);
 printf("first track in last sess: %d\n", ftils + track_offset - 1);
 printf("last track in last sess:  %d\n", ltils + track_offset - 1);

 burn_disc_get_cd_info(drive, disc_type, &disc_id, bar_code, &app_code,
                       &cd_info_valid);
 printf("Disk Is %sunrestricted\n", (cd_info_valid & 16) ? "" : "not ");
 if((cd_info_valid & 8) && !(cd_info_valid & 16))
   printf("Disc application code: %d\n", app_code);
 if(strcmp(media_class, "DVD") == 0 || strcmp(media_class, "BD") == 0)
   printf("Disk type: DVD, HD-DVD or BD\n");
 else if(strcmp(media_class, "CD") == 0 && (cd_info_valid & 1))
   printf("Disk type: %s\n", disc_type);
 else
   printf("Disk type: unrecognizable\n");
 if(cd_info_valid & 2)
   printf("Disk id: 0x%-X\n", disc_id);
 ret= burn_disc_get_bd_spare_info(drive, &alloc_blocks, &free_blocks, 0);
 if(ret == 1) {
   printf("BD Spare Area consumed:   %d\n", alloc_blocks - free_blocks);
   printf("BD Spare Area available:  %d\n", free_blocks);
 }

 printf("\n");
 printf("Track  Sess Type   Start Addr End Addr   Size\n");
 printf("==============================================\n");
 for(session_no= 0; session_no < num_sessions + open_sessions; session_no++) {
   tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
   if(tracks==NULL)
 continue;
   for(track_no= 0; track_no<num_tracks; track_no++) {
     track_count++;
     burn_track_get_entry(tracks[track_no], &toc_entry);
     if(toc_entry.extensions_valid&1) { /* DVD extension valid */
       lba= toc_entry.start_lba;
       size= toc_entry.track_blocks;
     } else {
       pmin= toc_entry.min;
       psec= toc_entry.sec;
       pframe= toc_entry.frame;
       size= burn_msf_to_lba(pmin,psec,pframe);
       pmin= toc_entry.pmin;
       psec= toc_entry.psec;
       pframe= toc_entry.pframe;
       lba= burn_msf_to_lba(pmin,psec,pframe);
     }

     lra= lba + size - 1;
     if(pno >= 0x08 && pno <= 0x0a && ((toc_entry.control & 7) >= 4) &&
        lra >= 2 && size >= 2) {
       /* If last two blocks not readable then assume TAO and subtract 2
          from lra and size.
       */;
       ret= burn_read_data(drive, (off_t) (lra - 1) * (off_t) 2048, buf,
                           2 * 2048, &buf_count, 2 | 4);
       if(ret <= 0) {
         lra-= 2;
         size-= 2;
       }
     }

#ifdef Cdrskin_with_last_recorded_addresS

     /* Interesting, but obviously not what cdrecord prints as "End Addr" */
     if(toc_entry.extensions_valid & 2) { /* LRA extension valid */
       if(pno == 0x11 || pno == 0x13 || pno == 0x14 || pno == 0x15 ||
	  pno == 0x41 || pno == 0x42 || pno == 0x51)
         lra= toc_entry.last_recorded_address;
     }

#endif /* Cdrskin_with_last_recorded_addresS */

     if(session_no < num_sessions) {
       type_text= ((toc_entry.control&7)<4) ? "Audio" : "Data";
     } else {
       if(track_no < num_tracks - 1) {
         type_text= "Rsrvd";
         have_real_open_session = 1;
       } else {
         type_text= "Blank";
       }
       if(toc_entry.extensions_valid & 4) {
         if(toc_entry.track_status_bits & (1 << 14))
           type_text= "Blank";
         else if(toc_entry.track_status_bits & (1 << 16)) {
           type_text= "Apdbl";
           have_real_open_session = 1;
         } else if(toc_entry.track_status_bits & (1 << 15)) {
           type_text= "Rsrvd";
           have_real_open_session = 1;
         } else
           type_text= "Invsb";
       }
     }
     printf("%5d %5d %-6s %-10d %-10d %-10d\n",
            track_count + track_offset - 1, session_no + 1,
            type_text, lba, lra, size);
     if(session_no < num_sessions)
       last_leadout= lba + size;
   }
 }
 if(last_leadout > 0)
   if(read_capacity > last_leadout)
     read_capacity= last_leadout;
 if(nominal_sessions > num_sessions) {
   ret= burn_disc_track_lba_nwa(drive, NULL, 0, &lba, &nwa);
   if(ret > 0) {
     avail= burn_disc_available_space(drive, NULL);
     size= avail / 2048;
     if(read_capacity == 0 && skin->media_is_overwriteable)
       size= 0; /* unformatted overwriteable media */
     if(nominal_sessions > num_sessions + open_sessions) {
       printf("%5d %5d %-6s %-10d %-10d %-10d\n",
              track_count + track_offset, nominal_sessions,
              ovwrt_full ? "Data" : "Blank",
              nwa, nwa + size - 1, size);
     }
   }
 }
 printf("\n");

 if(num_sessions > 0) {
   ret= burn_disc_get_msc1(drive, &lba);
   if(ret > 0)
     printf("Last session start address:         %-10d\n", lba);
   if(last_leadout > 0)
     printf("Last session leadout start address: %-10d\n", last_leadout);
   if(s == BURN_DISC_FULL) {
     if(read_capacity > 0 && (last_leadout != read_capacity ||
                    pno == 0x08 || pno == 0x10 || pno == 0x40 || pno == 0x50))
       printf("Read capacity:                      %-10d\n", read_capacity);
   }
 } else if(ovwrt_full) {
   printf("Last session start address:         %-10d\n", nwa);
   printf("Last session leadout start address: %-10d\n", size);
 }
 if(nominal_sessions > num_sessions && !skin->media_is_overwriteable) {
   printf("Next writable address:              %-10d\n", nwa);
   printf("Remaining writable size:            %-10d\n", size);
 }

 if(ovwrt_full) {
   printf("\n");
   printf("cdrskin: Media is overwriteable. No blanking needed. No reliable track size.\n");
   printf("cdrskin: Above contrary statements follow cdrecord traditions.\n");
 }

 if(have_real_open_session)
   printf("\nWarning: Incomplete session encountered !\n");

 if(disc!=NULL)
   burn_disc_free(disc);
 if(s == BURN_DISC_EMPTY)
   return(0);
 return(1);
}


int Cdrskin_print_all_profiles(struct CdrskiN *skin, struct burn_drive *drive,
                               int flag)
{
 int num_profiles, profiles[64], i, ret;
 char is_current[64], profile_name[80];

 burn_drive_get_all_profiles(drive, &num_profiles, profiles, is_current);
 for(i= 0; i < num_profiles; i++) {
   ret= burn_obtain_profile_name(profiles[i], profile_name); 
   if(ret <= 0)
     strcpy(profile_name, "unknown");
   printf("Profile: 0x%4.4X (%s)%s\n", (unsigned int) profiles[i],
          profile_name, is_current[i] ? " (current)" : "");
 }
 return(1);
}


/** Perform -atip .
    @param flag Bitfield for control purposes:
                bit0= perform -toc
                bit1= perform -toc with session markers
                bit2= perform -minfo
    @return <=0 error, 1 success
*/
int Cdrskin_atip(struct CdrskiN *skin, int flag)
{
 int ret,is_not_really_erasable= 0, current_is_cd= 1;
 double x_speed_max= -1.0, x_speed_min= -1.0;
 enum burn_disc_status s;
 struct burn_drive *drive;
 int profile_number= 0;
 char profile_name[80], *manuf= NULL, *media_code1= NULL, *media_code2= NULL;
 char *book_type= NULL, *product_id= NULL;

 ClN(printf("cdrskin: pseudo-atip on drive %d\n",skin->driveno));
 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 Cdrskin_report_disc_status(skin,s,1|2);
 if(s==BURN_DISC_APPENDABLE && skin->no_blank_appendable)
   is_not_really_erasable= 1;

 profile_name[0]= 0;
 ret= burn_disc_get_profile(drive,&profile_number,profile_name);
 if(ret<=0) {
   profile_number= 0;
   strcpy(profile_name, "-unidentified-");
 }
 if(profile_number != 0x08 && profile_number != 0x09 && profile_number != 0x0a)
   current_is_cd= 0;

 ret= Cdrskin_checkdrive(skin,profile_name,1);
 if(ret<=0)
   return(ret);

 if(burn_disc_get_status(drive) != BURN_DISC_UNSUITABLE) {
   ret= burn_disc_read_atip(drive);
   if(ret>0) {
     ret= burn_drive_get_min_write_speed(drive);
     x_speed_min= ((double) ret)/Cdrskin_libburn_speed_factoR;
   }
 }
 if(burn_disc_get_status(drive) == BURN_DISC_UNSUITABLE) {
   if(skin->verbosity>=Cdrskin_verbose_progresS) {
     if(profile_name[0])
       printf("Current: %s\n",profile_name);
     else
       printf("Current: UNSUITABLE MEDIA (Profile %4.4Xh)\n",profile_number);
   }
   {ret= 0; goto ex;}
 }
 if(burn_disc_get_status(drive) != BURN_DISC_EMPTY) {
   ret= burn_drive_get_write_speed(drive);
   x_speed_max= ((double) ret)/Cdrskin_libburn_speed_factoR;
   if(x_speed_min<0)
     x_speed_min= x_speed_max;
 }
 printf("cdrskin: burn_drive_get_write_speed = %d  (%.1fx)\n",ret,x_speed_max);
 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   if(burn_disc_get_status(drive) == BURN_DISC_EMPTY)
     printf("Current: none\n");
   else if(profile_name[0])
     printf("Current: %s\n",profile_name);
   else if(burn_disc_erasable(drive))
     printf("Current: CD-RW\n");
   else
     printf("Current: CD-R\n");
   Cdrskin_print_all_profiles(skin, drive, 0);
 }

 if(burn_disc_get_status(drive) == BURN_DISC_EMPTY)
   {ret= 0; goto ex;}

 if(strstr(profile_name,"DVD")==profile_name) {
   /* These are dummy messages for project scdbackup, so its media recognition
      gets a hint that the media is suitable and not in need of blanking.
      scdbackup will learn to interpret cdrskin's DVD messages but the
      current stable version needs to believe it is talking to its own
      growisofs_wrapper. So this is an emulation of an emulator.
      The real book type is available meanwhile. But that one is not intended.
   */
   printf("book type:     %s (emulated booktype)\n", profile_name);
   if(profile_number==0x13) /* DVD-RW */
     printf("cdrskin: message for sdvdbackup: \"(growisofs mode Restricted Overwrite)\"\n");
 } else if(strstr(profile_name,"BD")==profile_name) {
   printf("Mounted Media: %2.2Xh, %s\n", profile_number, profile_name);
 } else {
   printf("ATIP info from disk:\n");
   if(burn_disc_erasable(drive)) {
     if(is_not_really_erasable)
       printf("  Is erasable (but not while in this incomplete state)\n");
     else
       printf("  Is erasable\n");
   } else {
     printf("  Is not erasable\n");
   }
   { int start_lba,end_lba,min,sec,fr;
     int m_lo, s_lo, f_lo;

     ret= burn_drive_get_start_end_lba(drive,&start_lba,&end_lba,0);
     if(ret>0) {
       burn_lba_to_msf(start_lba,&min,&sec,&fr);
       printf("  ATIP start of lead in:  %d (%-2.2d:%-2.2d/%-2.2d)\n",
              start_lba,min,sec,fr);
       burn_lba_to_msf(end_lba,&m_lo,&s_lo,&f_lo);
       printf("  ATIP start of lead out: %d (%-2.2d:%-2.2d/%-2.2d)\n",
              end_lba, m_lo, s_lo, f_lo);
       if(current_is_cd)
         manuf= burn_guess_cd_manufacturer(min, sec, fr, m_lo, s_lo, f_lo, 0);
     }
   }
   printf("  1T speed low:  %.f 1T speed high: %.f\n",x_speed_min,x_speed_max);
 }

 ret= burn_disc_get_media_id(drive, &product_id, &media_code1, &media_code2,
                                &book_type, 0);
 if(ret > 0 && (!current_is_cd) &&
    manuf == NULL && media_code1 != NULL && media_code2 != NULL) {

   manuf= burn_guess_manufacturer(profile_number, media_code1, media_code2, 0);
 }
 if(product_id != NULL)
   printf("Product Id:    %s\n", product_id);
 if(manuf != NULL)
   printf("Producer:      %s\n", manuf);
 if(skin->verbosity >= Cdrskin_verbose_progresS) {
   if (current_is_cd) {
     if(manuf != NULL)
       printf("Manufacturer: %s\n", manuf);
   } else if(product_id != NULL && media_code1 != NULL && media_code2 != NULL){
     free(product_id);
     free(media_code1);
     free(media_code2);
     if(book_type != NULL)
       free(book_type);
     product_id= media_code1= media_code2= book_type= NULL;
     ret= burn_disc_get_media_id(drive, &product_id, &media_code1,
                                     &media_code2, &book_type, 1);
     if(ret > 0) {
       if(profile_number == 0x11 || profile_number == 0x13 ||
          profile_number == 0x14 || profile_number == 0x15)
         printf("Manufacturer:   '%s'\n", media_code1);
       else if(profile_number >= 0x40 && profile_number <= 0x43) {
         printf("Manufacturer:       '%s'\n", media_code1);
         if(media_code2[0])
           printf("Media type:         '%s'\n", media_code2);
       } else {
         printf("Manufacturer:    '%s'\n", media_code1);
         if(media_code2[0])
           printf("Media type:      '%s'\n", media_code2);
       }
     }
   }
 }

 ret= 1;
 if(flag&1)
   ret= Cdrskin_toc(skin, !(flag & 2));
                       /*cdrecord seems to ignore -toc errors if -atip is ok */
 if(ret > 0 && (flag & 4))
   ret= Cdrskin_minfo(skin, 0);

ex:;
 if(manuf != NULL)
   free(manuf);
 if(media_code1 != NULL)
   free(media_code1);
 if(media_code2 != NULL)
   free(media_code2);
 if(book_type != NULL)
   free(book_type);
 if(product_id != NULL)
   free(product_id);

 return(ret);
}


/** Perform --list_formats
    @param flag Bitfield for control purposes:
    @return <=0 error, 1 success
*/
int Cdrskin_list_formats(struct CdrskiN *skin, int flag)
{
 struct burn_drive *drive;
 int ret, i, status, num_formats, profile_no, type, alloc_blocks, free_blocks;
 off_t size;
 unsigned dummy;
 char status_text[80], profile_name[90];

 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;

 ret = burn_disc_get_formats(drive, &status, &size, &dummy,
                             &num_formats);
 if(ret <= 0) {
   fprintf(stderr, "cdrskin: SORRY: Cannot obtain format list info\n");
   ret= 0; goto ex;
 }
 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 printf("Media current: ");
 if(profile_no > 0 && ret > 0) {
   if(profile_name[0])
     printf("%s\n", profile_name);
   else
     printf("%4.4Xh\n", profile_no);
 } else
   printf("is not recognizable\n");

 if(status == BURN_FORMAT_IS_UNFORMATTED)
   sprintf(status_text, "unformatted, up to %.1f MiB",
                        ((double) size) / 1024.0 / 1024.0);
 else if(status == BURN_FORMAT_IS_FORMATTED) {
   if(profile_no==0x12 || profile_no==0x13 || profile_no==0x1a ||
      profile_no==0x43)
     sprintf(status_text, "formatted, with %.1f MiB",
                         ((double) size) / 1024.0 / 1024.0);
   else
     sprintf(status_text, "written, with %.1f MiB",
                         ((double) size) / 1024.0 / 1024.0);
 } else if(status == BURN_FORMAT_IS_UNKNOWN) {
   if (profile_no > 0)
     sprintf(status_text, "intermediate or unknown");
   else
     sprintf(status_text, "no media or unknown media");
 } else
   sprintf(status_text, "illegal status according to MMC-5");
 printf("Format status: %s\n", status_text);
 ret= burn_disc_get_bd_spare_info(drive, &alloc_blocks, &free_blocks, 0);
 if(ret == 1)
   printf("BD Spare Area: %d blocks consumed, %d blocks available\n",
          alloc_blocks - free_blocks, free_blocks);

 for (i = 0; i < num_formats; i++) {
   ret= burn_disc_get_format_descr(drive, i, &type, &size, &dummy);
   if (ret <= 0)
 continue;
   printf("Format idx %-2d: %2.2Xh , %.fs , %.1f MiB\n",
          i, type, ((double) size) / 2048.0, ((double) size) / 1024.0/1024.0);
 }
 ret= 1;
ex:;
 return(ret);
}


/** Perform --list_speeds
    @param flag Bitfield for control purposes:
    @return <=0 error, 1 success
*/
int Cdrskin_list_speeds(struct CdrskiN *skin, int flag)
{
 struct burn_drive *drive;
 int ret, i, profile_no, high= -1, low= 0x7fffffff, is_cd= 0;
 char profile_name[90], *speed_unit= "D";
 double speed_factor= 1385000.0, cd_factor= 75.0 * 2352;
 struct burn_speed_descriptor *speed_list= NULL, *item, *other;

 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;
 ret= burn_drive_get_speedlist(drive, &speed_list);
 if(ret <= 0) {
   fprintf(stderr, "cdrskin: SORRY: Cannot obtain speed list info\n");
   ret= 0; goto ex;
 }
 ret= burn_disc_get_profile(drive, &profile_no, profile_name);
 printf("Media current: ");
 if(profile_no > 0 && ret > 0) {
   if(profile_name[0])
     printf("%s\n", profile_name);
   else
     printf("%4.4Xh\n", profile_no);
 } else
   printf("is not recognizable\n");
 if(profile_no >= 0x08 && profile_no <= 0x0a)
   is_cd= profile_no;
 speed_factor= Cdrskin_libburn_speed_factoR * 1000.0;
 if(Cdrskin_libburn_speed_factoR == Cdrskin_libburn_cd_speed_factoR)
   speed_unit= "C";
 else if(Cdrskin_libburn_speed_factoR ==  Cdrskin_libburn_bd_speed_factoR)
   speed_unit= "B";

 for (item= speed_list; item != NULL; item= item->next) {
   if(item->source == 1) {
     /* CD mode page 2Ah : report only if not same speed by GET PERFORMANCE */
     for(other= speed_list; other != NULL; other= other->next)
       if(other->source == 2 && item->write_speed == other->write_speed)
     break;
     if(other != NULL)
 continue;
   }
   printf("Write speed  :  %5dk , %4.1fx%s\n",
          item->write_speed,
          ((double) item->write_speed) * 1000.0 / speed_factor, speed_unit);
   if(item->write_speed > high)
     high= item->write_speed;
   if(item->write_speed < low)
     low= item->write_speed;
 }
   
 /* Maybe there is ATIP info */
 if(is_cd) {
   ret= burn_disc_read_atip(drive);
   if(ret < 0)
     goto ex;
   if(ret > 0) {
     for(i= 0; i < 2; i++) {
       if(i == 0)
         ret= burn_drive_get_min_write_speed(drive);
       else
         ret= burn_drive_get_write_speed(drive);
       if(ret > 0) {
         if(ret < low || (i == 0 && ret != low)) { 
           printf("Write speed l:  %5dk , %4.1fx%s\n",
                  ret, ((double) ret) * 1000.0 / cd_factor, "C");
           low= ret;
         }
         if(ret > high || (i == 1 && ret != high)) {
           printf("Write speed h:  %5dk , %4.1fx%s\n",
                  ret, ((double) ret) * 1000.0 / cd_factor, "C");
           high= ret;
         }
       }
     }
   }
 }
 if(high > -1) {
   printf("Write speed L:  %5dk , %4.1fx%s\n",
          low, ((double) low) * 1000.0 / speed_factor, speed_unit);
   printf("Write speed H:  %5dk , %4.1fx%s\n",
           high, ((double) high) * 1000.0 / speed_factor, speed_unit);
   ret= burn_drive_get_best_speed(drive, -1, &item, 2);
   if(ret > 0 && item != NULL)
     if(item->write_speed != low)
       printf("Write speed 0:  %5dk , %4.1fx%s\n",
              item->write_speed,
             ((double) item->write_speed) * 1000.0 / speed_factor, speed_unit);
   ret= burn_drive_get_best_speed(drive, 0, &item, 2);
   if(ret > 0 && item != NULL)
     if(item->write_speed != high)
       printf("Write speed-1:  %5dk , %4.1fx%s\n",
              item->write_speed,
             ((double) item->write_speed) * 1000.0 / speed_factor, speed_unit);
 } else {
   fprintf(stderr,
      "cdrskin: SORRY : Could not get any write speed information from drive");
 }
 ret= 1;
ex:;
 if(speed_list != NULL)
   burn_drive_free_speedlist(&speed_list);
 return(ret);
}


int Cdrskin_read_textfile(struct CdrskiN *skin, char *path, int flag)
{
 int ret, num_packs = 0;
 unsigned char *text_packs = NULL;

 ret= burn_cdtext_from_packfile(path, &text_packs, &num_packs, 0);
 if(ret <= 0)
   goto ex;

 if(skin->text_packs != NULL)
   free(skin->text_packs);
 skin->text_packs= text_packs;
 skin->num_text_packs= num_packs;
 ret= 1;
ex:;
 if(ret <= 0 && text_packs != NULL)
   free(text_packs);
 return(ret);
}


/*
    @param flag         Bitfield for control purposes:
                        bit3= Enable DAP : "flaw obscuring mechanisms like
                                            audio data mute and interpolate"
*/
int Cdrskin_extract_audio_to_dir(struct CdrskiN *skin,
                                 char *dir, char *basename,
                                 char print_tracks[100], int flag)
{
 int num_sessions= 0, num_tracks= 0, profile_number, session_no, track_no;
 int track_count= 0, existing= 0, ret, pass, not_audio= 0, pick_tracks= 0, i;
 int tracks_extracted= 0, min_tno= 100, max_tno= 0;
 struct burn_drive *drive;
 struct burn_disc *disc= NULL;
 struct burn_session **sessions;
 struct burn_track **tracks;
 enum burn_disc_status s;
 struct burn_toc_entry toc_entry;
 struct stat stbuf;
 char profile_name[80], path[4096 + 256];

 ret= Cdrskin_grab_drive(skin, 0);
 if(ret<=0)
   goto ex;
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 disc= burn_drive_get_disc(drive);
 if(s == BURN_DISC_EMPTY || s == BURN_DISC_BLANK || disc == NULL) {
not_readable:;
   fprintf(stderr, "cdrskin: SORRY : No audio CD in drive.\n");
   ret= 0; goto ex;
 }
 ret= burn_disc_get_profile(drive, &profile_number, profile_name);
 if(ret <= 0 || profile_number < 0x08 || profile_number > 0x0a) {
not_audio_cd:;
   fprintf(stderr, "cdrskin: SORRY : Medium in drive is not an audio CD.\n");
   ret= 0; goto ex;
 }
 for(i= 0; i < 99; i++)
   if(print_tracks[i])
     pick_tracks= 1;
 sessions= burn_disc_get_sessions(disc, &num_sessions);
 if(num_sessions <= 0)
   goto not_readable;
 for(pass= 0; pass < 2; pass++) {
   if(pass > 0) {
     if(existing > 0)
       {ret= 0; goto ex;}
     if(not_audio == track_count)
       goto not_audio_cd;
   }
   track_count= 0;
   for(session_no= 0; session_no < num_sessions; session_no++) {
     tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
     if(tracks==NULL)
   continue;
     for(track_no= 0; track_no < num_tracks; track_no++) {
       track_count++;
       burn_track_get_entry(tracks[track_no], &toc_entry);
       if(toc_entry.point < min_tno)
         min_tno= toc_entry.point;
       if(toc_entry.point > max_tno)
         max_tno= toc_entry.point;
       if(pick_tracks) {
         if(toc_entry.point <= 0 || toc_entry.point > 99)
     continue;                                          /* should not happen */
         if(print_tracks[toc_entry.point] == 0)
     continue;
       }
       if((toc_entry.control & 7) >= 4) {
         if(pass == 0)
           fprintf(stderr,
                   "cdrskin: WARNING : Track %2.2d is not an audio track.\n",
                   toc_entry.point);
         not_audio++;
     continue;
       }
       sprintf(path, "%s/%s%2.2u.wav", dir, basename, toc_entry.point);
       if(pass == 0) {
         if(stat(path, &stbuf) != -1) {
           fprintf(stderr,
                   "cdrskin: SORRY : May not overwrite existing file: '%s'\n",
                   path);
           existing++;
         }
     continue;
       }
       if(skin->verbosity >= Cdrskin_verbose_progresS)
         fprintf(stderr, "cdrskin: Writing audio track file: %s\n", path);
       ret= burn_drive_extract_audio_track(drive, tracks[track_no], path,
                   (flag & 8) | (skin->verbosity >= Cdrskin_verbose_progresS));
       if(ret <= 0)
         goto ex;
       tracks_extracted++;
     }
   }
 }
 if(tracks_extracted == 0 && pick_tracks) {
   fprintf(stderr,
  "cdrskin: SORRY : Not a single track matched the list of extract_tracks=\n");
   if(min_tno < 100 && max_tno > 0)
     fprintf(stderr, "cdrskin: HINT : Track range is %2.2d to %2.2d\n",
             min_tno, max_tno);
   ret= 0; goto ex;
 }
 ret= 1;
ex:;
 return(ret);
}


#ifndef Cdrskin_extra_leaN

/* A70324: proposal by Eduard Bloch */
int Cdrskin_warn_of_mini_tsize(struct CdrskiN *skin, int flag)
{
 off_t media_space= 0;
 enum burn_disc_status s;
 struct burn_drive *drive;
 int ret;

 ret= burn_drive_get_drive_role(skin->drives[skin->driveno].drive);
 if(ret != 1)
   return(1);

 if(skin->multi || skin->has_open_ended_track || skin->smallest_tsize<0)
   return(1);
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 if(s!=BURN_DISC_BLANK)
   return(1);
 media_space= burn_disc_available_space(drive, NULL);
 if(media_space<=0 || 
    skin->smallest_tsize >= media_space / Cdrskin_minimum_tsize_quotienT)
   return(1);
 fprintf(stderr,"\n");
 fprintf(stderr,"\n");
 fprintf(stderr,
         "cdrskin: WARNING: Very small track size set by option tsize=\n");
 fprintf(stderr,
         "cdrskin:          Track size %.1f MB <-> media capacity %.1f MB\n",
         skin->smallest_tsize/1024.0/1024.0,
         ((double) media_space)/1024.0/1024.0);
 fprintf(stderr,"\n");
 fprintf(stderr,
         "cdrskin: Will wait at least 15 seconds until real burning starts\n");
 fprintf(stderr,"\n");
 if(skin->gracetime<15)
   skin->gracetime= 15;
 return(1);
}


/** Emulate the gracetime= behavior of cdrecord 
    @param flag Bitfield for control purposes:
                bit0= do not print message about pseudo-checkdrive
*/
int Cdrskin_wait_before_action(struct CdrskiN *skin, int flag)
/* flag: bit0= BLANK rather than write mode
         bit1= FORMAT rather than write mode
*/
{
 int i;

 Cdrskin_warn_of_mini_tsize(skin,0);

 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   char speed_text[80];
   if(skin->x_speed<0)
     strcpy(speed_text,"MAX");
   else if(skin->x_speed==0)
     strcpy(speed_text,"MIN");
   else
     sprintf(speed_text,"%.f",skin->x_speed);
   printf(
  "Starting to write CD/DVD at speed %s in %s %s mode for %s session.\n",
          speed_text,(skin->dummy_mode?"dummy":"real"),
          (flag&2?"FORMAT":(flag&1?"BLANK":skin->preskin->write_mode_name)),
          (skin->multi?"multi":"single"));
   printf("Last chance to quit, starting real write in %3d seconds.",
          skin->gracetime);
   fflush(stdout);
 }
 for(i= skin->gracetime-1;i>=0;i--) {
   usleep(1000000);
   if(Cdrskin__is_aborting(0))
     return(0);
   if(skin->verbosity>=Cdrskin_verbose_progresS) {
     printf("\b\b\b\b\b\b\b\b\b\b\b\b\b %3d seconds.",i);
     fflush(stdout);
   }
 }
 if(skin->verbosity>=Cdrskin_verbose_progresS)
   {printf(" Operation starts.\n");fflush(stdout);} 
 return(1);
}

#endif /* Cdrskin_extra_leaN */


/** Perform blank=[all|fast]
    @return <=0 error, 1 success
*/
int Cdrskin_blank(struct CdrskiN *skin, int flag)
{
 enum burn_disc_status s;
 struct burn_progress p;
 struct burn_drive *drive;
 int ret,loop_counter= 0,hint_force= 0,do_format= 0, profile_number= -1;
 int wrote_well= 1, format_flag= 0, status, num_formats;
 off_t size;
 unsigned dummy;
 double start_time;
 char *verb= "blank", *presperf="blanking", *fmt_text= "...";
 char profile_name[80];
 static char fmtp[][40]= {
                  "...", "format_overwrite", "deformat_sequential",
                  "(-format)", "format_defectmgt", "format_by_index",
                  "format_if_needed", "as_needed"};
 static int fmtp_max= 7;

 start_time= Sfile_microtime(0); /* will be refreshed later */
 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 profile_name[0]= 0;
 if(skin->grabbed_drive)
   burn_disc_get_profile(skin->grabbed_drive,&profile_number,profile_name);

 ret= Cdrskin_report_disc_status(skin,s,
                           1|(4*!(skin->verbosity>=Cdrskin_verbose_progresS)));
 if(ret==2)
   s= BURN_DISC_APPENDABLE;
 do_format= skin->blank_format_type & 0xff;
 if(s==BURN_DISC_UNSUITABLE) {
   if(skin->force_is_set) {
     ClN(fprintf(stderr,"cdrskin: NOTE : -force blank=... : Treating unsuitable media as burn_disc_full\n"));
     ret= burn_disc_pretend_full(drive);
     s= burn_disc_get_status(drive);
   } else
     hint_force= 1;
 }
 if(do_format)
   if(do_format>=0 && do_format<=fmtp_max)
     fmt_text= fmtp[do_format];
 if(do_format==5) { /* format_by_index */
   if(profile_number == 0x12 || profile_number == 0x43)
     do_format= 4;
   else
     do_format= 1;
 } else if(do_format==6 || do_format==7) { /* format_if_needed , as_needed */
   /* Find out whether format is needed at all.
      Eventuelly set up a suitable formatting run
   */
   if(profile_number == 0x14 && do_format==6) { /* sequential DVD-RW */
     do_format= 1;
     skin->blank_format_type= 1|(1<<8);
     skin->blank_format_size= 128*1024*1024;
   } else if(profile_number == 0x12 ||
             profile_number == 0x43 ||
            (profile_number == 0x41 && do_format==6)) {
                                               /* DVD-RAM , BD-RE , BD-R SRM */
     ret= burn_disc_get_formats(drive, &status, &size, &dummy, &num_formats);
     if((ret>0 && status!=BURN_FORMAT_IS_FORMATTED)) {
       do_format= 4;
       skin->blank_format_type= 4|(3<<9);  /* default payload size */
       skin->blank_format_size= 0;
     }
   } else if(do_format==7) { /* try to blank what is not blank yet */
     if(s!=BURN_DISC_BLANK) {
       do_format= 0;
       skin->blank_fast= 1;
     }
   }
   if(do_format==6 || do_format==7) {
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(fprintf(stderr,
        "cdrskin: NOTE : blank=%s : no need for action detected\n", fmt_text));
     {ret= 2; goto ex;}
   }
 }

 if(do_format == 1 || do_format == 3 || do_format == 4) {
   verb= "format";
   presperf= "formatting";
 }
 if(do_format==2) {
   /* Forceful blanking to Sequential Recording for DVD-R[W] and CD-RW */

   if(!(profile_number == 0x14 || profile_number == 0x13 ||
        profile_number == 0x0a)) {
     if(skin->grow_overwriteable_iso>0 && skin->media_is_overwriteable)
       goto pseudo_blank_ov;
     else
       goto unsupported_format_type;
   }

 } else if(do_format==1 || do_format==3) {
   /* Formatting to become overwriteable for DVD-RW and DVD+RW */

   if(do_format==3 && profile_number != 0x1a) {
     fprintf(stderr, "cdrskin: SORRY : -format does DVD+RW only\n");
     if(profile_number==0x14)
       fprintf(stderr,
          "cdrskin: HINT  : blank=format_overwrite would format this media\n");
     {ret= 0; goto ex;}
   }

   if(profile_number == 0x14) { /* DVD-RW sequential */
       /* ok */;
   } else if(profile_number == 0x13) { /* DVD-RW restricted overwrite */
     if(!(skin->force_is_set || ((skin->blank_format_type>>8)&4))) {
       fprintf(stderr,
       "cdrskin: NOTE : blank=format_... : media is already formatted\n");
       fprintf(stderr,
       "cdrskin: HINT : If you really want to re-format, add option -force\n");
       {ret= 2; goto ex;}
     }
   } else if(profile_number == 0x1a) { /* DVD+RW */
     if(!((skin->blank_format_type>>8)&4)) {
       fprintf(stderr,
       "cdrskin: NOTE : blank=format_... : DVD+RW do not need this\n");
       fprintf(stderr,
      "cdrskin: HINT : For de-icing use option blank=format_overwrite_full\n");
       {ret= 2; goto ex;}
     }
   } else {
     fprintf(stderr,
            "cdrskin: SORRY : blank=%s for now does DVD-RW and DVD+RW only\n",
            fmt_text);
     {ret= 0; goto ex;}
   }
   if(s==BURN_DISC_UNSUITABLE)
     fprintf(stderr,
         "cdrskin: NOTE : blank=%s accepted not yet suitable media\n",
         fmt_text);

 } else if(do_format==4) {
   /* Formatting and influencing defect management of DVD-RAM , BD-RE */
   if(!(profile_number == 0x12 || profile_number == 0x41 ||
        profile_number == 0x43)) {
     fprintf(stderr,
            "cdrskin: SORRY : blank=%s for now does DVD-RAM and BD only\n",
            fmt_text);
     {ret= 0; goto ex;}
   }
   if(s==BURN_DISC_UNSUITABLE)
     fprintf(stderr,
         "cdrskin: NOTE : blank=%s accepted not yet suitable media\n",
         fmt_text);

 } else if(do_format==0) {
   /* Classical blanking of erasable media */

   if(skin->grow_overwriteable_iso>0 && skin->media_is_overwriteable) {
pseudo_blank_ov:;
     if(skin->dummy_mode) {
       fprintf(stderr,
     "cdrskin: would have begun to pseudo-blank disc if not in -dummy mode\n");
       goto blanking_done;
     }
     skin->grow_overwriteable_iso= 3;
     ret= Cdrskin_invalidate_iso_head(skin, 0);
     if(ret<=0)
       goto ex;
     goto blanking_done;
   } else if(s!=BURN_DISC_FULL && 
      (s!=BURN_DISC_APPENDABLE || skin->no_blank_appendable) &&
      (profile_number!=0x13 || !skin->prodvd_cli_compatible) &&
      (s!=BURN_DISC_BLANK || !skin->force_is_set)) {
     if(s==BURN_DISC_BLANK) {
       fprintf(stderr,
       "cdrskin: NOTE : blank=... : media was already blank (and still is)\n");
       {ret= 2; goto ex;}
     } else if(s==BURN_DISC_APPENDABLE) {
       fprintf(stderr,
               "cdrskin: FATAL : blank=... : media is still appendable\n");
     } else {
       fprintf(stderr,
               "cdrskin: FATAL : blank=... : no blankworthy disc found\n");
       if(hint_force)
         fprintf(stderr,
    "cdrskin: HINT : If you are certain to have a CD-RW, try option -force\n");
     }
     {ret= 0; goto ex;}
   }
   if(!burn_disc_erasable(drive)) {
     fprintf(stderr,"cdrskin: FATAL : blank=... : media is not erasable\n");
     {ret= 0; goto ex;}
   }
   if((profile_number == 0x14 || profile_number == 0x13) &&
      !skin->prodvd_cli_compatible)
     skin->blank_fast= 0; /* only with deformat_sequential_quickest */

 } else {
unsupported_format_type:;
   fprintf(stderr,
          "cdrskin: SORRY : blank=%s is unsupported with media type %s\n",
          fmt_text, profile_name);
   {ret= 0; goto ex;}
 }

 if(skin->dummy_mode) {
   fprintf(stderr,
           "cdrskin: would have begun to %s disc if not in -dummy mode\n",
           verb);
   goto blanking_done;
 }
 fprintf(stderr,"cdrskin: beginning to %s disc\n",verb);
 Cdrskin_adjust_speed(skin,0);

#ifndef Cdrskin_extra_leaN
 Cdrskin_wait_before_action(skin,
                            1+(do_format==1 || do_format==3 || do_format==4));
#endif /* ! Cdrskin_extra_leaN */

 if(Cdrskin__is_aborting(0))
   {ret= 0; goto ex;}
 skin->drive_is_busy= 1;
 if(do_format==0 || do_format==2) {
   burn_disc_erase(drive,skin->blank_fast);

 } else if(do_format==1 || do_format==3 || do_format==4) {
   format_flag= (skin->blank_format_type>>8)&(1|2|4|32|128);
   if(skin->force_is_set)
     format_flag|= 16;
   if(format_flag&128)
     format_flag|= (skin->blank_format_index&255)<<8;
   if(skin->blank_format_no_certify)
     format_flag|= 64;
   burn_disc_format(drive,(off_t) skin->blank_format_size,format_flag);

 } else {
   fprintf(stderr,"cdrskin: SORRY : Format type %d not implemented yet.\n",
           do_format);
   ret= 0; goto ex;
 }

 loop_counter= 0;
 start_time= Sfile_microtime(0);
 while(burn_drive_get_status(drive, &p) != BURN_DRIVE_IDLE) {
   if(loop_counter>0)
     if(skin->verbosity>=Cdrskin_verbose_progresS) {
       double percent= 50.0;

       if(p.sectors>0) /* i want a display of 1 to 99 percent */
         percent= 1.0+((double) p.sector+1.0)/((double) p.sectors)*98.0;
       fprintf(stderr,
          "%scdrskin: %s ( done %.1f%% , %lu seconds elapsed )          %s",
          skin->pacifier_with_newline ? "" : "\r",
          presperf,percent,(unsigned long) (Sfile_microtime(0)-start_time),
          skin->pacifier_with_newline ? "\n" : "");
     }
   sleep(1);
   loop_counter++;
 }
blanking_done:;
 wrote_well = burn_drive_wrote_well(drive);
 if(wrote_well && skin->verbosity>=Cdrskin_verbose_progresS) {
   fprintf(stderr,
           "%scdrskin: %s done                                        \n",
           skin->pacifier_with_newline ? "" : "\r", presperf);
   printf("%s time:   %.3fs\n",
          (do_format==1 || do_format==3 || do_format==4 ?
           "Formatting":"Blanking"),
          Sfile_microtime(0)-start_time);
 }
 fflush(stdout);
 if(!wrote_well)
   fprintf(stderr,
           "%scdrskin: %s failed                                      \n",
           skin->pacifier_with_newline ? "" : "\r", presperf);
 ret= !!(wrote_well);
ex:;
 skin->drive_is_busy= 0;
 if(Cdrskin__is_aborting(0))
   Cdrskin_abort(skin, 0); /* Never comes back */
 return(ret);
}


int Cdrskin__libburn_fifo_status(struct burn_source *current_fifo,
                                 char fifo_text[80], int flag)
{
 int ret, size, free_space, fill, fifo_percent;
 char *status_text= "";

 ret= burn_fifo_inquire_status(current_fifo, &size, &free_space,
                               &status_text);
 if(ret <= 0 || ret >= 4) {
   strcpy(fifo_text, "(fifo   0%) ");
 } else if(ret == 1) {
   burn_fifo_next_interval(current_fifo, &fill);
   fifo_percent= 100.0 * ((double) fill) / (double) size;
   if(fifo_percent<100 && fill>0)
     fifo_percent++;
   sprintf(fifo_text, "(fifo %3d%%) ", fifo_percent);
 }
 return(1);
}


/** Report burn progress. This is done partially in cdrecord style.
    Actual reporting happens only if write progress hit the next MB or if in
    non-write-progress states a second has elapsed since the last report. 
    After an actual report a new statistics interval begins.
    @param drive_status As obtained from burn_drive_get_status()
    @param p Progress information from burn_drive_get_status()
    @param start_time Timestamp of burn start in seconds
    @param last_time Timestamp of report interval start in seconds
    @param total_count Returns the total number of bytes written so far
    @param total_count Returns the number of bytes written during interval
    @param flag Bitfield for control purposes:
                bit0= report in growisofs style rather than cdrecord style
    @return <=0 error, 1 seems to be writing payload, 2 doing something else 
*/
int Cdrskin_burn_pacifier(struct CdrskiN *skin, int start_tno,
                          enum burn_drive_status drive_status,
                          struct burn_progress *p,
                          double start_time, double *last_time,
                          double *total_count, double *last_count,
                          int *min_buffer_fill, int flag)
/*
 bit0= growisofs style
*/
{
 double bytes_to_write= 0.0;
 double written_bytes= 0.0,written_total_bytes= 0.0;
 double fixed_size,padding,sector_size,speed_factor;
 double measured_total_speed,measured_speed;
 double elapsed_time,elapsed_total_time,current_time;
 double estim_time,estim_minutes,estim_seconds,percent;
 int ret,fifo_percent,fill,advance_interval=0,new_mb,old_mb,time_to_tell;
 int old_track_idx,buffer_fill,formatting= 0,use_data_image_size;
 char fifo_text[80],mb_text[40], pending[40];
 char *debug_mark= ""; /* use this to prepend a marker text for experiments */

#ifndef Cdrskin_no_cdrfifO
 double buffer_size;
 int fs, bs, space;
#endif

#ifdef Cdrskin_use_libburn_fifO
 struct burn_source *current_fifo= NULL;

#ifdef NIX
 int size, free_space;
 char *status_text= "";
#endif

#endif /* Cdrskin_use_libburn_fifO */

 /* for debugging */
 static double last_fifo_in= 0.0,last_fifo_out= 0.0,curr_fifo_in,curr_fifo_out;

 if(Cdrskin__is_aborting(0))
   Cdrskin_abort(skin, 0); /* Never comes back */

 current_time= Sfile_microtime(0);
 elapsed_total_time= current_time-start_time;
 elapsed_time= current_time-*last_time;
 time_to_tell= (elapsed_time>=1.0)&&(elapsed_total_time>=1.0);
 written_total_bytes= *last_count; /* to be overwritten by p.sector */

 if(drive_status==BURN_DRIVE_FORMATTING) {
   formatting= 1;
 } else if(drive_status==BURN_DRIVE_WRITING) {
   ;
 } else if(drive_status==BURN_DRIVE_WRITING_LEADIN
           || drive_status==BURN_DRIVE_WRITING_PREGAP
           || formatting) {
   if(time_to_tell || skin->is_writing) {
     if(skin->verbosity>=Cdrskin_verbose_progresS) {
       if(skin->is_writing)
         fprintf(stderr,"\n");
       fprintf(stderr,
           "%scdrskin: %s (burning since %.f seconds)%s",
           skin->pacifier_with_newline ? "" : "\r",
           (formatting?"formatting":"working pre-track"), elapsed_total_time,
           skin->pacifier_with_newline ? "\n" : "         ");
     }
     skin->is_writing= 0;
     advance_interval= 1;
   }
   {ret= 2; goto ex;}
 } else if(drive_status==BURN_DRIVE_WRITING_LEADOUT
           || drive_status==BURN_DRIVE_CLOSING_TRACK
           || drive_status==BURN_DRIVE_CLOSING_SESSION) {

   if(drive_status==BURN_DRIVE_CLOSING_SESSION &&
      skin->previous_drive_status!=drive_status)
     {printf("\nFixating...\n"); fflush(stdout);}
   if(time_to_tell || skin->is_writing) {
     if(skin->verbosity>=Cdrskin_verbose_progresS) {
       if(skin->is_writing && !skin->pacifier_with_newline)
         fprintf(stderr,"\n");
       fprintf(stderr,
               "%scdrskin: working post-track (burning since %.f seconds)%s",
               skin->pacifier_with_newline ? "" : "\r", elapsed_total_time,
               skin->pacifier_with_newline ? "\n" : "        ");
     }
     skin->is_writing= 0;
     advance_interval= 1;
   }
   {ret= 2; goto ex;}
 } else
   goto thank_you_for_patience;

 old_track_idx= skin->supposed_track_idx;
 skin->supposed_track_idx= p->track;

 if(old_track_idx>=0 && old_track_idx<skin->supposed_track_idx &&
    skin->supposed_track_idx < skin->track_counter) {
   Cdrtrack_get_size(skin->tracklist[old_track_idx],&fixed_size,&padding,
                     &sector_size,&use_data_image_size,1);
   if(skin->verbosity>=Cdrskin_verbose_progresS)
     printf("\n");
   printf("%sTrack %-2.2d: Total bytes read/written: %.f/%.f (%.f sectors).\n",
          debug_mark, old_track_idx + start_tno, fixed_size,fixed_size+padding,
          (fixed_size+padding)/sector_size);
   *last_count= 0;
 }

 sector_size= 2048.0;
 if(skin->supposed_track_idx>=0 && 
    skin->supposed_track_idx<skin->track_counter)
   Cdrtrack_get_size(skin->tracklist[skin->supposed_track_idx],&fixed_size,
                     &padding,&sector_size,&use_data_image_size,0);

 bytes_to_write= ((double) p->sectors)*sector_size;
 written_total_bytes= ((double) p->sector)*sector_size;
 written_bytes= written_total_bytes-*last_count;
 if(written_total_bytes<1024*1024) {
thank_you_for_patience:;
   if(time_to_tell || (skin->is_writing && elapsed_total_time>=1.0)) {
     if(skin->verbosity>=Cdrskin_verbose_progresS) {
       if(skin->is_writing) {
         fprintf(stderr,"\n");
       }
       pending[0]= 0;
/*
       if(bytes_to_write > 0 && skin->verbosity >= Cdrskin_verbose_debuG)
         sprintf(pending, " pnd %.f", bytes_to_write - written_total_bytes);
*/
       fprintf(stderr,
             "%scdrskin: thank you for being patient for %.f seconds%21.21s%s",
             skin->pacifier_with_newline ? "" : "\r",
             elapsed_total_time, pending,
             skin->pacifier_with_newline ? "\n" : "");
     }
     advance_interval= 1;
   }
   skin->is_writing= 0;
   {ret= 2; goto ex;}
 }
 new_mb= written_total_bytes/(1024*1024);
 old_mb= (*last_count)/(1024*1024);
 if(new_mb==old_mb && !(written_total_bytes>=skin->fixed_size &&
                        skin->fixed_size>0 && time_to_tell))
   {ret= 1; goto ex;}


#ifndef Cdrskin_extra_leaN

 percent= 0.0;
 if(bytes_to_write>0)
   percent= written_total_bytes/bytes_to_write*100.0;
 measured_total_speed= 0.0;
 measured_speed= 0.0;
 estim_time= -1.0;
 estim_minutes= -1.0;
 estim_seconds= -1.0;
 if(elapsed_total_time>0.0) {
   measured_total_speed= written_total_bytes/elapsed_total_time;
   estim_time= (bytes_to_write-written_bytes)/measured_total_speed;
   if(estim_time>0.0 && estim_time<86400.0) {
     estim_minutes= ((int) estim_time)/60;
     estim_seconds= estim_time-estim_minutes*60.0;
     if(estim_seconds<0.0)
       estim_seconds= 0.0;
   }
 }
 if(written_bytes==written_total_bytes && elapsed_total_time>0) {
   measured_speed= measured_total_speed;
 } else if(elapsed_time>0.0) {
   measured_speed= written_bytes/elapsed_time;
 } else if(written_bytes>0.0) {
   measured_speed= 99.91*Cdrskin_speed_factoR;
 }

 if(measured_speed<=0.0 && written_total_bytes>=skin->fixed_size && 
    skin->fixed_size>0) {
   if(!skin->is_writing)
     goto thank_you_for_patience;
   skin->is_writing= 0;
   measured_speed= measured_total_speed;
 } else 
   skin->is_writing= 1;
 if(skin->supposed_track_idx<0)
   skin->supposed_track_idx= 0;
 if(*last_count <= 0.0 && !skin->pacifier_with_newline)
   printf("\r%-78.78s\r", ""); /* wipe output line */
 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   if(flag&1) {
     printf("%.f/%.f (%2.1f%%) @%1.1f, remaining %.f:%2.2d\n",
            written_total_bytes,bytes_to_write,percent,
            measured_speed/Cdrskin_speed_factoR,
            estim_minutes,(int) estim_seconds);
   } else {
     fill= 0;
     fifo_percent= 50;
     fifo_text[0]= 0;
     curr_fifo_in= last_fifo_in;
     curr_fifo_out= last_fifo_out;

     if(skin->cuefile_fifo != NULL)
       Cdrskin__libburn_fifo_status(skin->cuefile_fifo, fifo_text, 0);

#ifdef Cdrskin_use_libburn_fifO

     /* Inquire fifo fill and format fifo pacifier text */
     if(skin->fifo == NULL && skin->supposed_track_idx >= 0 && 
        skin->supposed_track_idx < skin->track_counter &&
        skin->fifo_size > 0 && fifo_text[0] == 0) {
       Cdrtrack_get_libburn_fifo(skin->tracklist[skin->supposed_track_idx],
                                 &current_fifo, 0);
       if(current_fifo != NULL) {
         Cdrskin__libburn_fifo_status(current_fifo, fifo_text, 0);
       } else if(skin->fifo_size > 0) {
         strcpy(fifo_text, "(fifo 100%) ");
       }
     }

#endif /* Cdrskin_use_libburn_fifO */

#ifndef Cdrskin_no_cdrfifO

     if(skin->fifo!=NULL) {
       ret= Cdrfifo_get_buffer_state(skin->fifo,&fill,&space,0);
       buffer_size= fill+space;
       if(ret==2 || ret==0) {
         fifo_percent= 100;
       } else if(ret>0 && buffer_size>0.0) {
         /* obtain minimum fill of pacifier interval */
         Cdrfifo_next_interval(skin->fifo,&fill,0);
         fifo_percent= 100.0*((double) fill)/buffer_size;
         if(fifo_percent<100 && fill>0)
           fifo_percent++;
       }
       if(skin->verbosity>=Cdrskin_verbose_debuG) {
         Cdrfifo_get_counters(skin->fifo,&curr_fifo_in,&curr_fifo_out,0);
         Cdrfifo_get_sizes(skin->fifo,&bs,&fs,0);
       }
       if(skin->fifo_size>0) {
         sprintf(fifo_text,"(fifo %3d%%) ",fifo_percent);
         if(skin->verbosity>=Cdrskin_verbose_debug_fifO) {
           fprintf(stderr,
                   "\ncdrskin_debug: fifo >= %9d / %d :  %8.f in, %8.f out\n",
                   fill,(int) buffer_size,
                   curr_fifo_in-last_fifo_in,curr_fifo_out-last_fifo_out);
           last_fifo_in= curr_fifo_in;
           last_fifo_out= curr_fifo_out;
         }
       }
     }

#endif /* ! Cdrskin_no_cdrfifO */

     if(skin->supposed_track_idx >= 0 && 
        skin->supposed_track_idx < skin->track_counter) {
       /* fixed_size,padding are fetched above via Cdrtrack_get_size() */;
     } else if(skin->fixed_size!=0) {
       fixed_size= skin->fixed_size;
       padding= skin->padding;
     }
     if(fixed_size || (skin->fill_up_media &&
                       skin->supposed_track_idx==skin->track_counter-1)) {
       sprintf(mb_text,"%4d of %4d",(int) (written_total_bytes/1024.0/1024.0),
               (int) ((double) Cdrtrack_get_sectors(
                          skin->tracklist[skin->supposed_track_idx],0)*
                      sector_size/1024.0/1024.0));
     } else
       sprintf(mb_text,"%4d",(int) (written_total_bytes/1024.0/1024.0));
     speed_factor= Cdrskin_speed_factoR*sector_size/2048;

     buffer_fill= 50;
     if(p->buffer_capacity>0)
       buffer_fill= (double) (p->buffer_capacity - p->buffer_available)*100.0
                    / (double) p->buffer_capacity;
     
     if(buffer_fill<*min_buffer_fill)
       *min_buffer_fill= buffer_fill;

     printf("%s%sTrack %-2.2d: %s MB written %s[buf %3d%%]  %4.1fx.%s",
            skin->pacifier_with_newline ? "" : "\r",
            debug_mark, skin->supposed_track_idx + start_tno, mb_text,
            fifo_text, buffer_fill, measured_speed / speed_factor,
            skin->pacifier_with_newline ? "\n" : "");
     fflush(stdout);
   }
   if(skin->is_writing==0) {
     printf("\n");
     goto thank_you_for_patience;
   }
 }

#else /* ! Cdrskin_extra_leaN */

 if(skin->supposed_track_idx<0)
   skin->supposed_track_idx= 0;
 if(written_bytes<=0.0 && written_total_bytes>=skin->fixed_size && 
    skin->fixed_size>0) {
   if(!skin->is_writing)
     goto thank_you_for_patience;
   skin->is_writing= 0;
 } else {
   if((!skin->is_writing) && !skin->pacifier_with_newline)
     printf("\n");
   skin->is_writing= 1;
 }
 printf("%sTrack %-2.2d: %3d MB written %s",
        skin->pacifier_with_newline ? "" : "\r",
        skin->supposed_track_idx + start_tno,
        (int) (written_total_bytes / 1024.0 / 1024.0),
        skin->pacifier_with_newline ? "\n" : "");
 if(skin->is_writing == 0 && !skin->pacifier_with_newline)
   printf("\n");
 fflush(stdout);

#endif /* Cdrskin_extra_leaN */


 advance_interval= 1;
 ret= 1;
ex:; 
 if(advance_interval) {
   if(written_total_bytes>0)
     *last_count= written_total_bytes;
   else
     *last_count= 0.0;
   if(*last_count>*total_count)
     *total_count= *last_count;
   *last_time= current_time;
 }
 skin->previous_drive_status= drive_status;
 return(ret);
}


/** After everything else about burn_write_opts and burn_disc is set up, this
    call determines the effective write mode and checks whether the drive
    promises to support it.
*/
int Cdrskin_activate_write_mode(struct CdrskiN *skin,
                                struct burn_write_opts *opts,
                                struct burn_disc *disc,
                                int flag)
{
 int profile_number= -1, ret, was_still_default= 0;
 char profile_name[80], reasons[BURN_REASONS_LEN];
 enum burn_disc_status s= BURN_DISC_UNGRABBED;
 enum burn_write_types wt;

 profile_name[0]= 0;
 if(skin->grabbed_drive) {
   burn_disc_get_profile(skin->grabbed_drive,&profile_number,profile_name);
   s= burn_disc_get_status(skin->grabbed_drive);
 }
 if(strcmp(skin->preskin->write_mode_name,"DEFAULT")==0) {
   was_still_default= 1;
   wt= burn_write_opts_auto_write_type(opts, disc, reasons, 0);
   if(wt==BURN_WRITE_NONE) {
     if(strncmp(reasons,"MEDIA: ",7)==0)
       ret= -1;
     else
       ret= 0;
     goto report_failure;
   }
   skin->write_type= wt;

#ifndef Cdrskin_disable_raw96R
   if(wt==BURN_WRITE_RAW)
     strcpy(skin->preskin->write_mode_name,"RAW/RAW96R");
   else
#endif /* ! Cdrskin_disable_raw96R */
   if(wt==BURN_WRITE_TAO)
     strcpy(skin->preskin->write_mode_name,"TAO");
   else if(wt==BURN_WRITE_SAO)
     strcpy(skin->preskin->write_mode_name,"SAO");
   else
     sprintf(skin->preskin->write_mode_name,"LIBBURN/%d", (int) wt);
 }

#ifndef Cdrskin_disable_raw96R
 if(strcmp(skin->preskin->write_mode_name,"RAW/RAW96R")==0) {
   skin->write_type= BURN_WRITE_RAW;
   skin->block_type= BURN_BLOCK_RAW96R;
 } else
#endif /* ! Cdrskin_disable_raw96R */

 if(strcmp(skin->preskin->write_mode_name,"TAO")==0) {
   skin->write_type= BURN_WRITE_TAO;
   skin->block_type= BURN_BLOCK_MODE1;
 } else if(strncmp(skin->preskin->write_mode_name,"LIBBURN/",8)==0) {
   skin->block_type= BURN_BLOCK_MODE1;
 } else {
   strcpy(skin->preskin->write_mode_name,"SAO");
   skin->write_type= BURN_WRITE_SAO;
   skin->block_type= BURN_BLOCK_SAO;
   /* cdrecord tolerates the combination of -sao and -multi. -multi wins. */
   burn_write_opts_set_write_type(opts,skin->write_type,skin->block_type);
   ret = burn_precheck_write(opts,disc,reasons,0);
   if(ret <= 0) {
     if(strstr(reasons, "multi session capability lacking") != NULL) {
       fprintf(stderr,"cdrskin: WARNING : Cannot do SAO/DAO. Reason: %s\n",
               reasons);
       fprintf(stderr,"cdrskin: Defaulting to TAO/Incremental.\n");
       skin->write_type= BURN_WRITE_TAO;
       skin->block_type= BURN_BLOCK_MODE1;
     }
   }
 }
 if(!was_still_default)
   burn_write_opts_set_write_type(opts,skin->write_type,skin->block_type);
 ret = burn_precheck_write(opts,disc,reasons,0);
 if(ret<=0) {
report_failure:;
   if(ret!=-1)
     fprintf(stderr,"cdrskin: Reason: %s\n",reasons);
   fprintf(stderr,"cdrskin: Media : %s%s\n",
             s==BURN_DISC_BLANK?"blank ":
             s==BURN_DISC_APPENDABLE?"appendable ":
             s==BURN_DISC_FULL?"** closed ** ":"",
             profile_name[0]?profile_name:
             s==BURN_DISC_EMPTY?"no media":"unknown media");
   return(0);
 }
 if(skin->verbosity>=Cdrskin_verbose_cmD)
   printf("cdrskin: Write type : %s\n", skin->preskin->write_mode_name);
 return(1);
}


#ifndef Cdrskin_extra_leaN

int Cdrskin_announce_tracks(struct CdrskiN *skin, int start_tno, int flag)
{
 int i,mb,use_data_image_size;
 double size,padding,sector_size= 2048.0;
 double sectors;

 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   if(start_tno < 1)
     start_tno= 1;
   for(i=0;i<skin->track_counter;i++) {
     Cdrtrack_get_size(skin->tracklist[i],&size,&padding,&sector_size,
                       &use_data_image_size,0);
     if(size<=0) {
       printf("Track %-2.2d: %s unknown length",
              i + start_tno, (sector_size==2048?"data ":"audio"));
     } else {
       mb= size/1024.0/1024.0;
       printf("Track %-2.2d: %s %4d MB        ",
              i + start_tno, (sector_size==2048?"data ":"audio"), mb);
     }
     if(padding>0)
       printf(" padsize:  %.f KB\n",padding/1024.0);
     else
       printf("\n");
   }
   if(skin->fixed_size<=0) {
     printf("Total size:       0 MB (00:00.00) = 0 sectors\n");
     printf("Lout start:       0 MB (00:02/00) = 0 sectors\n");
   } else {
     /* >>> This is quite a fake. Need to learn about 12:35.25 and "Lout" 
            ??? Is there a way to obtain the toc in advance (print_cue()) ? */
     double seconds;
     int min,sec,frac;

     mb= skin->fixed_size/1024.0/1024.0;
     seconds= skin->fixed_size/150.0/1024.0+2.0;
     min= seconds/60.0;
     sec= seconds-min*60;
     frac= (seconds-min*60-sec)*100;
     if(frac>99)
       frac= 99;
     sectors= (int) (skin->fixed_size/sector_size);
     if(sectors*sector_size != skin->fixed_size)
       sectors++;
     printf("Total size:    %5d MB (%-2.2d:%-2.2d.%-2.2d) = %d sectors\n",
             mb,min,sec,frac,(int) sectors);
     seconds+= 2;
     min= seconds/60.0;
     sec= seconds-min*60;
     frac= (seconds-min*60-sec)*100;
     if(frac>99)
       frac= 99;
     sectors+= 150;
     printf("Lout start:    %5d MB (%-2.2d:%-2.2d/%-2.2d) = %d sectors\n",
            mb,min,sec,frac,(int) sectors);
   }
 }
 return(1);
}

#endif /* ! Cdrskin_extra_leaN */


int Cdrskin_direct_write(struct CdrskiN *skin, int flag)
{
 off_t byte_address, data_count, chunksize, i, alignment, fill;
 int ret, max_chunksize= 64*1024, source_fd= -1, is_from_stdin, eof_sensed= 0;
 char *buf= NULL, *source_path, amount_text[81];
 struct burn_multi_caps *caps= NULL;

 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   goto ex;

 ret= burn_disc_get_multi_caps(skin->grabbed_drive,BURN_WRITE_NONE,&caps,0);
 if(ret<=0)
   goto ex;
 if(caps->start_adr==0) {
   fprintf(stderr,
      "cdrskin: SORRY : Direct writing is not supported by drive and media\n");
   {ret= 0; goto ex;}
 }
 alignment= caps->start_alignment;
 if(alignment>0 && (((off_t) skin->direct_write_amount) % alignment)!=0) {
   fprintf(stderr,
     "cdrskin: SORRY : direct_write_amount=%.f not aligned to blocks of %dk\n",
      skin->direct_write_amount,(int) alignment/1024);
   {ret= 0; goto ex;}
 }

 if(skin->track_counter<=0) {
   fprintf(stderr,
           "cdrskin: SORRY : No track source given for direct writing\n");
   {ret= 0; goto ex;}
 }
 Cdrtrack_get_source_path(skin->tracklist[0],
                          &source_path,&source_fd,&is_from_stdin,0);
 if(source_fd==-1) {
   ret= Cdrtrack_open_source_path(skin->tracklist[0],&source_fd,
                               2 | (skin->verbosity >= Cdrskin_verbose_debuG) |
                               (4 * (skin->fifo_size >= 256 * 1024)));
   if(ret<=0)
     goto ex;
 }
 buf= calloc(1, max_chunksize);
 if(buf==NULL) {
   fprintf(stderr,
           "cdrskin: FATAL : Cannot allocate %d bytes of read buffer.\n",
           max_chunksize);
   {ret= -1; goto ex;}
 }
 byte_address= skin->write_start_address;
 if(byte_address<0)
   byte_address= 0;
 data_count= skin->direct_write_amount;
 if(data_count>0)
   sprintf(amount_text,"%.fk",(double) (data_count/1024));
 else
   strcpy(amount_text,"0=open_ended");
 fprintf(stderr,"Beginning direct write (start=%.fk,amount=%s) ...\n",
         (double) (byte_address/1024),amount_text);
 for(i= 0; i<data_count || data_count==0; i+= chunksize) {
   if(data_count==0)
     chunksize= (alignment > 0 ? alignment : 2048);
   else
     chunksize= data_count-i;
   if(chunksize>max_chunksize)
     chunksize= max_chunksize;

   /* read buffer from first track */
   for(fill= 0; fill<chunksize;fill+= ret) {
     ret= read(source_fd,buf+fill,chunksize-fill); 
     if(ret==-1) {
       fprintf(stderr,"cdrskin: FATAL : Error while reading from '%s'\n",
               source_path);
       if(errno>0)
         fprintf(stderr,"cdrskin: %s (errno=%d)\n", strerror(errno), errno);
       ret= 0; goto ex;
     } else if(ret==0) {
       eof_sensed= 1;
       if(data_count==0) {
         memset(buf+fill,0,(size_t) (chunksize-fill));
   break;
       } else {
         fprintf(stderr,
                 "cdrskin: FATAL : Premature EOF while reading from '%s'\n",
                 source_path);
         ret= 0; goto ex;
       }
     }
   }
   ret= burn_random_access_write(skin->grabbed_drive,byte_address,
                                 buf,chunksize,0);
   if(ret<=0)
     goto ex;
   if(eof_sensed)
 break;
   byte_address+= chunksize;
   fprintf(stderr, "%s%9.fk written     %s",
           skin->pacifier_with_newline ? "" : "\r",
           ((double) (i+chunksize)) / 1024.0,
           skin->pacifier_with_newline ? "\n" : ""); 
 }
 fprintf(stderr, "%s%9.fk written     \n",
         skin->pacifier_with_newline ? "" : "\r", ((double) i) / 1024.0); 
 /* flush drive buffer */
 fprintf(stderr,"syncing cache ...\n"); 
 ret = burn_random_access_write(skin->grabbed_drive,byte_address,buf,0,1);
 if(ret<=0)
   goto ex;
 ret= 1;
ex:;
 if(caps!=NULL)
   burn_disc_free_multi_caps(&caps);
 if(buf!=NULL)
   free(buf);
 if(ret>0)
   fprintf(stderr,"writing done\n"); 
 else
   fprintf(stderr,"writing failed\n"); 
 return(ret);
}


int Cdrskin_grow_overwriteable_iso(struct CdrskiN *skin, int flag)
{
 int ret, i, went_well= 1;
 char *track_descr, *md;
 unsigned char *td; /* Because the representation of 255 in char is ambigous */
 double track_size, media_size;

 ret= Cdrtrack_get_iso_fs_descr(skin->tracklist[0],&track_descr,&track_size,0);
 if(ret<=0) {
   fprintf(stderr,"cdrskin: SORRY : Saw no ISO-9660 filesystem in track 0\n");
   return(ret);
 }
 if(skin->grow_overwriteable_iso==3) /* initial session */
   return(1);
 if(skin->grow_overwriteable_iso!=2) {
   fprintf(stderr,
          "cdrskin: SORRY : Could not read ISO-9660 descriptors from media\n");
   return(0);
 }
 ret= Scan_for_iso_size((unsigned char *) skin->overwriteable_iso_head+16*2048,
                        &media_size, 0);
 if(ret<=0) {
   fprintf(stderr,"cdrskin: SORRY : No recognizable ISO-9660 on media\n");
   return(0);
 }
 if(skin->write_start_address>=0.0)
   media_size= skin->write_start_address;

 /* Write new sum into media descr 0 */
 md= skin->overwriteable_iso_head+16*2048;
 memcpy(md,track_descr,2048);
 Set_descr_iso_size((unsigned char *) md,track_size+media_size,0);
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,"cdrskin_debug: new ISO-9660 size : %.f  (%fs)\n",
               track_size+media_size, (track_size+media_size)/2048));

 /* Copy type 255 CD001 descriptors from track to media descriptor buffer
    and adjust their size entries */
 for(i=1; i<16; i++) {
   td= (unsigned char *) (track_descr + i * 2048);
   md= skin->overwriteable_iso_head+(16+i)*2048;
   if(td[0] != 255)
 break;
   /* demand media descrN[0] == track descrN[0] */
   if(((char *) td)[0] != md[0]) {
     fprintf(stderr,
   "cdrskin: SORRY : Type mismatch of ISO volume descriptor #%d (%u <-> %u)\n",
             i, ((unsigned int) td[0]) & 0xff, ((unsigned int) md[0])&0xff);
     went_well= 0;
   }
   memcpy(md,td,2048);
   Set_descr_iso_size((unsigned char *) md,track_size+media_size,0);
 }
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,"cdrskin_debug: copied %d secondary ISO descriptors\n",
               i-1));

 /* write block 16 to 31 to media */
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(fprintf(stderr,"cdrskin_debug: writing to media: blocks 16 to 31\n"));
 ret= burn_random_access_write(skin->grabbed_drive, (off_t) (16*2048),
                               skin->overwriteable_iso_head+16*2048,
                               (off_t) (16*2048), 1);
 if(ret<=0)
   return(ret);

 return(went_well);
}


int Cdrskin_cdtext_test(struct CdrskiN *skin, struct burn_write_opts *o,
                        struct burn_session *session, int flag)
{
 int ret, num_packs;
 unsigned char *text_packs = NULL;

 if(skin->num_text_packs > 0) {
   Cdrskin_print_text_packs(skin, skin->text_packs, skin->num_text_packs, 2);
   ret= 1; goto ex;
 }
 ret= burn_cdtext_from_session(session, &text_packs, &num_packs, 0);
 if(ret < 0)
   goto ex;
 if(ret > 0) {
   Cdrskin_print_text_packs(skin, text_packs, num_packs, 1);
   ret= 1; goto ex;
 }
 ret= 1;
ex:
 if (text_packs != NULL)
   free(text_packs);
 return(ret);
}


int Cdrskin_read_input_sheet_v07t(struct CdrskiN *skin, char *path, int block,
                        	struct burn_session *session, int flag)
{
 int ret= 0;

 ret= burn_session_input_sheet_v07t(session, path, block, 1);
 return(ret);
}


int Cdrskin_cdrtracks_from_session(struct CdrskiN *skin,
                                   struct burn_session *session, int flag)
{
 int ret, num_tracks, i;
 struct burn_track **tracks;
 struct CdrtracK *t;
 double sectors;

 for(i= 0; i < skin->track_counter; i++)
   Cdrtrack_destroy(&(skin->tracklist[i]), 0);
 skin->track_counter= 0;
 
 tracks= burn_session_get_tracks(session, &num_tracks);
 if(num_tracks <= 0) {
   fprintf(stderr, "cdrskin: SORRY : No tracks defined for burn run.\n");
   ret= 0; goto ex;
 }
 for(i= 0; i < num_tracks; i++) {
   /* Create Cdrtrack without reading parameters from skin */
   ret= Cdrtrack_new(&(skin->tracklist[skin->track_counter]), skin,
                     skin->track_counter, 1);
   if(ret <= 0) {
     fprintf(stderr,
             "cdrskin: FATAL : Creation of track control object failed.\n");
     goto ex;
   }

   /* Set essential properties */
   t= skin->tracklist[skin->track_counter];
   t->track_type= burn_track_get_mode(tracks[i]);
   if(t->track_type & BURN_AUDIO) {
     t->sector_size= 2352;
     t->track_type= BURN_AUDIO;
   } else {
     t->sector_size= 2048;
     t->track_type= BURN_MODE1;
   }
   sectors= burn_track_get_sectors(tracks[i]);
   t->fixed_size= sectors * t->sector_size;
   t->libburn_track= tracks[i];
   t->libburn_track_is_own= 0;

   skin->track_counter++;
 }
 ret= 1;
ex:
 if(ret <= 0) {
   for(i= 0; i < skin->track_counter; i++)
     Cdrtrack_destroy(&(skin->tracklist[i]), 0);
   skin->track_counter= 0;
 }
 return(ret);
}


/** Burn data via libburn according to the parameters set in skin.
    @return <=0 error, 1 success
*/
int Cdrskin_burn(struct CdrskiN *skin, int flag)
{
 struct burn_disc *disc;
 struct burn_session *session;
 struct burn_write_opts *o = NULL;
 struct burn_source *cuefile_fifo= NULL;
 enum burn_disc_status s;
 enum burn_drive_status drive_status;
 struct burn_progress p;
 struct burn_drive *drive;
 int ret,loop_counter= 0,max_track= -1,i,hflag,nwa,num, wrote_well= 2;
 int fifo_disabled= 0, min_buffer_fill= 101, length, start_tno= 1;
 int use_data_image_size, needs_early_fifo_fill= 0,iso_size= -1, non_audio= 0;
 double start_time,last_time;
 double total_count= 0.0,last_count= 0.0,size,padding,sector_size= 2048.0;
 char *doing;
 char *source_path;
 unsigned char *payload;
 int source_fd, is_from_stdin;
 int text_flag= 4; /* Check CRCs and silently repair CRCs if all are 0 */
 unsigned char *text_packs= NULL;
 int num_packs= 0, start_block, block_no;

#ifndef Cdrskin_no_cdrfifO
 double put_counter, get_counter, empty_counter, full_counter;
 int total_min_fill, fifo_percent;
#endif
 off_t free_space;
 char msg[80];

 if(skin->tell_media_space)
   doing= "estimating";
 else
   doing= "burning";
 printf("cdrskin: beginning to %s disc\n",
        skin->tell_media_space?"estimate":"burn");
 if(skin->fill_up_media && skin->multi) {
   ClN(fprintf(stderr,
           "cdrskin: NOTE : Option --fill_up_media disabled option -multi\n"));
   skin->multi= 0;
 }
 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   goto burn_failed;
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 if(skin->verbosity>=Cdrskin_verbose_progresS)
   Cdrskin_report_disc_status(skin,s,1);

 disc= burn_disc_create();
 session= burn_session_create();
 ret= burn_disc_add_session(disc,session,BURN_POS_END);
 if(ret==0) {
   fprintf(stderr,"cdrskin: FATAL : Cannot add session to disc object.\n");
burn_failed:;
   if(cuefile_fifo != NULL)
     burn_source_free(cuefile_fifo);
   if(skin->verbosity>=Cdrskin_verbose_progresS) 
     printf("cdrskin: %s failed\n", doing);
   fprintf(stderr,"cdrskin: FATAL : %s failed.\n", doing);
   return(0);
 }
 skin->fixed_size= 0.0;
 skin->has_open_ended_track= 0;
 if(skin->cuefile[0]) {
   if(skin->track_counter > 0) {
     fprintf(stderr,
   "cdrskin: SORRY : Option cuefile= cannot be combined with track sources\n");
     goto burn_failed;
   }
   if(strcmp(skin->preskin->write_mode_name, "SAO") != 0 &&
      strcmp(skin->preskin->write_mode_name, "DEFAULT") != 0) {
     fprintf(stderr,
            "cdrskin: SORRY : Option cuefile= works only with write mode SAO");
     goto burn_failed;
   }
   strcpy(skin->preskin->write_mode_name, "SAO");

   ret= burn_session_by_cue_file(session, skin->cuefile, skin->fifo_size,
                                 &cuefile_fifo,
                                 &text_packs, &num_packs, !skin->use_cdtext);
   if(ret <= 0)
     goto burn_failed;
   if(num_packs > 0) {
     if(skin->num_text_packs > 0) {
       fprintf(stderr, "cdrskin: WARNING : Option textfile= overrides CDTEXTFILE from option cuesheet=\n");
       if(text_packs != NULL)
         free(text_packs);
     } else {
       skin->text_packs= text_packs;
       skin->num_text_packs= num_packs;
     }
   }
   ret= Cdrskin_cdrtracks_from_session(skin, session, 0);
   if(ret <= 0)
     goto burn_failed;
   skin->cuefile_fifo= cuefile_fifo;
   cuefile_fifo= NULL;

 } else {
   for(i=0;i<skin->track_counter;i++) {
     hflag= (skin->verbosity>=Cdrskin_verbose_debuG);
     if(i==skin->track_counter-1)
       Cdrtrack_ensure_padding(skin->tracklist[i],hflag&1);

/* if(skin->fifo_size >= 256 * 1024) */

       hflag|= 4;

     ret= Cdrtrack_add_to_session(skin->tracklist[i],i,session,hflag);
     if(ret<=0) {
       fprintf(stderr,"cdrskin: FATAL : Cannot add track %d to session.\n",i+1);
       goto burn_failed;
     }
     Cdrtrack_get_size(skin->tracklist[i],&size,&padding,&sector_size,
                       &use_data_image_size,0);
     if(use_data_image_size==1) {
       /* still unfulfilled -isosize demand pending */
       needs_early_fifo_fill= 1;
     }
     non_audio|= (skin->tracklist[i]->track_type != BURN_AUDIO);
   }
 }

 if(skin->sheet_v07t_blocks > 0) {
   if(skin->num_text_packs > 0) {
     fprintf(stderr, "cdrskin: WARNING : Option textfile= or cuefile= overrides option input_sheet_v07t=\n");
   } else if(non_audio) {
     fprintf(stderr, "cdrskin: SORRY : Option input_sheet_v07t= works only if all tracks are -audio\n");
     goto burn_failed;
   } else {
     /* If cuefile and session has block 0, then start at block 1 */
     start_block= 0;
     if(skin->cuefile[0]) {
       for(i= 0x80; i < 0x8f; i++) {
         ret= burn_session_get_cdtext(session, 0, i, "", &payload, &length, 0);
         if(ret > 0 && length > 0)
       break;
       }
       if(i < 0x8f)
         start_block= 1;
     }
     block_no = start_block;
     for(i= 0; i < skin->sheet_v07t_blocks && block_no < 8; i++) {
       ret= Cdrskin_read_input_sheet_v07t(skin, skin->sheet_v07t_paths[i],
                                          block_no, session, 0);
       if(ret <= 0)
         goto burn_failed;
       block_no += ret;
     }
     if(i < skin->sheet_v07t_blocks) {
       fprintf(stderr, "cdrskin: WARNING : Too many CD-TEXT blocks. input_sheet_v07t= files ignored: %d\n",
               skin->sheet_v07t_blocks - i);
     }
   }
 }

#ifndef Cdrskin_extra_leaN
 /* Final decision on track size has to be made after eventual -isosize
    determination via fifo content.
 */
 if(needs_early_fifo_fill && !skin->tell_media_space) {
   int start_memorized;

   start_memorized= skin->fifo_start_at;
   /* try  ISO-9660 size recognition via fifo */
   if(32*2048<=skin->fifo_size)
     skin->fifo_start_at= 32*2048;
   else
     skin->fifo_start_at= skin->fifo_size;
   ret= Cdrskin_fill_fifo(skin,0);
   if(ret<=0)
     goto fifo_filling_failed;
   if((start_memorized>skin->fifo_start_at || start_memorized<=0) &&
      skin->fifo_start_at<skin->fifo_size)
     needs_early_fifo_fill= 2; /* continue filling fifo at normal stage */
   skin->fifo_start_at= start_memorized;
 }
#endif /* Cdrskin_extra_leaN */

 for(i=0;i<skin->track_counter;i++) {
   Cdrtrack_get_size(skin->tracklist[i],&size,&padding,&sector_size,
                     &use_data_image_size,0);
   if(use_data_image_size==1 && size<=0 && skin->tell_media_space)
     size= 1024*1024; /* a dummy size */
   ret= Cdrtrack_activate_image_size(skin->tracklist[i],&size,
                                     !!skin->tell_media_space);
   if(ret<=0) {
     Cdrtrack_get_source_path(skin->tracklist[i],
                              &source_path,&source_fd,&is_from_stdin,0);
     fprintf(stderr,
           "cdrskin: FATAL : Cannot determine -isosize of track source\n");
     fprintf(stderr,
           "cdrskin:         '%s'\n", source_path);
     {ret= 0; goto ex;}
   }
   Cdrtrack_get_size(skin->tracklist[i],&size,&padding,&sector_size,
                     &use_data_image_size,0);
   if(use_data_image_size==2 && skin->verbosity>=Cdrskin_verbose_debuG)
     ClN(fprintf(stderr,
            "cdrskin: DEBUG: track %2.2d : activated -isosize %.fs (= %.fb)\n",
            i+1, size/2048.0,size));
   if(size>0)
     skin->fixed_size+= size+padding;
   else
     skin->has_open_ended_track= 1;
   if(skin->tracklist[i]->isrc[0] &&
      skin->tracklist[i]->libburn_track != NULL) {
      ret= burn_track_set_isrc_string(skin->tracklist[i]->libburn_track,
                                      skin->tracklist[i]->isrc, 0);
      if(ret <= 0)
        goto ex;
   }
 }
 
 o= burn_write_opts_new(drive);
 burn_write_opts_set_perform_opc(o, 0);

/* growisofs stunt: assessment of media and start for next session */
 if((skin->grow_overwriteable_iso==1 || skin->grow_overwriteable_iso==2) &&
     skin->media_is_overwriteable) {
   /* Obtain ISO size from media, keep 64 kB head in memory */
   ret= Cdrskin_overwriteable_iso_size(skin,&iso_size,0);
   if(ret<0)
     goto ex;
   if(ret>0 && skin->write_start_address<0) {
     skin->write_start_address= ((double) iso_size)*2048.0;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf(
            "cdrskin: write start address by --grow_overwriteable_iso : %ds\n",
            iso_size));
   } else if(ret==0)
     skin->grow_overwriteable_iso= 3; /* do not patch ISO header later on */
 } 

 burn_write_opts_set_start_byte(o, skin->write_start_address);

 if(skin->media_is_overwriteable && skin->multi) {
   if(skin->grow_overwriteable_iso<=0) {
     fprintf(stderr, "cdrskin: FATAL : -multi cannot leave a recognizable end mark on this media.\n");
     fprintf(stderr, "cdrskin: HINT  : For ISO-9660 images try --grow_overwriteable_iso -multi\n");
     {ret= 0; goto ex;}
   }
   skin->multi= 0;
 }
 if(skin->multi && !skin->media_does_multi) {
   if(skin->prodvd_cli_compatible) {
     skin->multi= 0;
     if(skin->verbosity>=Cdrskin_verbose_progresS)
       fprintf(stderr, "cdrskin: NOTE : Ignored option -multi.\n");
   }
 }
 burn_write_opts_set_multi(o,skin->multi);
 burn_write_opts_set_fillup(o, skin->fill_up_media);

 burn_write_opts_set_force(o, !!skin->force_is_set);
 burn_write_opts_set_stream_recording(o, skin->stream_recording_is_set);

#ifdef Cdrskin_dvd_obs_default_64K
 if(skin->dvd_obs == 0)
   burn_write_opts_set_dvd_obs(o, 64 * 1024);
 else
#endif
   burn_write_opts_set_dvd_obs(o, skin->dvd_obs);
 burn_write_opts_set_obs_pad(o, skin->obs_pad);
 burn_write_opts_set_stdio_fsync(o, skin->stdio_sync);

 if(skin->dummy_mode) {
   fprintf(stderr,
           "cdrskin: NOTE : -dummy mode will prevent actual writing\n");
   burn_write_opts_set_simulate(o, 1);
 }
 burn_write_opts_set_underrun_proof(o,skin->burnfree);
 if(skin->num_text_packs > 0) {
   if(non_audio) {
     fprintf(stderr, "cdrskin: SORRY : Option textfile= works only if all tracks are -audio\n");
     goto burn_failed;
   }
   if(!!skin->force_is_set)
	text_flag= 1; /* No CRC verification or repairing */
   ret= burn_write_opts_set_leadin_text(o, skin->text_packs,
                                        skin->num_text_packs, text_flag);
   if(ret <= 0)
     goto burn_failed;
 }
 if(skin->mcn[0]) {
   burn_write_opts_set_mediacatalog(o, (unsigned char *) skin->mcn);
   burn_write_opts_set_has_mediacatalog(o, 1);
 }
 if(skin->cd_start_tno >= 1 && skin->cd_start_tno <= 99) {
   ret= burn_session_set_start_tno(session, skin->cd_start_tno, 0);
   if(ret <= 0)
     goto burn_failed;
 }
 ret= Cdrskin_activate_write_mode(skin,o,disc,0);
 if(ret<=0)
   goto burn_failed;

 if(skin->cdtext_test) {
   ret= Cdrskin_cdtext_test(skin, o, session, (skin->cdtext_test == 1));
   if(ret <= 0)
     goto ex;
   if(skin->cdtext_test >= 2) {
     fprintf(stderr,
       "cdrskin: Option --cdtext_dummy prevents actual burn run\n");
     ret= 1; goto ex;
   }
 }

 ret= Cdrskin_obtain_nwa(skin, &nwa,0);
 if(ret<=0)
   nwa= -1;
 if(skin->assert_write_lba>=0 && nwa!=skin->assert_write_lba) {
   fprintf(stderr,
       "cdrskin: FATAL : Option assert_write_lba= demands block number %10d\n",
       skin->assert_write_lba);
   fprintf(stderr,
       "cdrskin: FATAL : but predicted actual write start address is   %10d\n",
       nwa);
   {ret= 0; goto ex;}
 }

#ifndef Cdrskin_extra_leaN
 Cdrskin_announce_tracks(skin, burn_session_get_start_tno(session, 0), 0);
#endif

 if(skin->tell_media_space || skin->track_counter <= 0) {
   /* write capacity estimation and return without actual burning */

   free_space= burn_disc_available_space(drive,o);
   sprintf(msg,"%d\n",(int) (free_space/(off_t) 2048));
   if(skin->preskin->result_fd>=0) {
     write(skin->preskin->result_fd,msg,strlen(msg));
   } else
     printf("%s",msg);
   if(skin->track_counter>0)
     fprintf(stderr,
       "cdrskin: NOTE : %s burn run suppressed by option --tell_media_space\n",
       skin->preskin->write_mode_name);
   {ret= 1; goto ex;}
 }

 if(skin->fixed_size > 0 && !skin->force_is_set) {
   free_space= burn_disc_available_space(drive,o);
   if(skin->fixed_size > free_space && free_space > 0) {
     fprintf(stderr,
 "cdrskin: FATAL : predicted session size %lus does not fit on media (%lus)\n",
             (unsigned long) ((skin->fixed_size + 2047.0) / 2048.0),
             (unsigned long) ((free_space + 2047) / 2048));
     ClN(fprintf(stderr,
              "cdrskin: HINT : This test may be disabled by option -force\n");)
     {ret= 0; goto ex;}
   }
 }

 Cdrskin_adjust_speed(skin,0);

#ifndef Cdrskin_extra_leaN
 Cdrskin_wait_before_action(skin,0);
 if(burn_is_aborting(0))
   {ret= 0; goto ex;}

 if(needs_early_fifo_fill==1)
   ret= 1;
 else if(skin->cuefile[0] != 0)
   ret= 1;
 else
   ret= Cdrskin_fill_fifo(skin,0);
 if(ret<=0) {
fifo_filling_failed:;
   fprintf(stderr,"cdrskin: FATAL : Filling of fifo failed\n");
   goto ex;
 }

#endif /* ! Cdrskin_extra_leaN */

 start_tno = burn_session_get_start_tno(session, 0);
 if(skin->verbosity>=Cdrskin_verbose_progresS && nwa>=0) {
   printf("Starting new track at sector: %d\n",nwa);
   fflush(stdout);
 }
 if(burn_is_aborting(0))
   {ret= 0; goto ex;}
 skin->drive_is_busy= 1;
 burn_disc_write(o, disc);
 if(skin->preskin->abort_handler==-1)
   Cleanup_set_handlers(Cleanup_handler_handlE, Cleanup_handler_funC,
                        Cleanup_handler_flaG);
 last_time= start_time= Sfile_microtime(0);

 burn_write_opts_free(o);
 o= NULL;

 while (burn_drive_get_status(drive, NULL) == BURN_DRIVE_SPAWNING) {

   /* >>> how do i learn about success or failure ? */

   ;
 }
 loop_counter= 0;
 while (1) {
    drive_status= burn_drive_get_status(drive, &p);
    if(drive_status==BURN_DRIVE_IDLE)
 break;

   /* >>> how do i learn about success or failure ? */

   if(loop_counter>0 || Cdrskin__is_aborting(0))
     Cdrskin_burn_pacifier(skin, start_tno,
                           drive_status,&p,start_time,&last_time,
                           &total_count,&last_count,&min_buffer_fill,0);

   if(max_track<skin->supposed_track_idx)
      max_track= skin->supposed_track_idx;


#ifndef Cdrskin_extra_leaN

   /* <<< debugging : artificial abort without a previous signal */;
   if(skin->abort_after_bytecount>=0.0 && 
      total_count>=skin->abort_after_bytecount) {
      /* whatever signal handling is installed: this thread is the boss now */
      fprintf(stderr,
       "cdrskin: DEVELOPMENT : synthetic abort by abort_after_bytecount=%.f\n",
              skin->abort_after_bytecount);
      skin->control_pid= getpid();
      ret= Cdrskin_abort_handler(skin,0,0);
      fprintf(stderr,"cdrskin: done (aborted)\n");
      exit(1);
   }

   if(Cdrskin__is_aborting(0))
     fifo_disabled= 1;
   if(skin->fifo==NULL || fifo_disabled) {
     usleep(20000);
   } else {

#ifdef Cdrskin_no_cdrfifO

     /* Should never happen as skin->fifo should be NULL */
     usleep(20000);

#else /* Cdrskin_no_cdrfifO */

     ret= Cdrfifo_try_to_work(skin->fifo,20000,NULL,NULL,0);
     if(ret<0) {
       int abh;

       abh= skin->preskin->abort_handler;
       if(abh!=2) 
         fprintf(stderr,
              "\ncdrskin: FATAL : Fifo encountered error during burn loop.\n");
       if(abh==0) {
         ret= -1; goto ex;
       } else if(abh==1 || abh==3 || abh==4 || abh==-1) {
         Cdrskin_abort_handler(skin,0,0);
         fprintf(stderr,"cdrskin: done (aborted)\n");
         exit(10);
       } else {
          if(skin->verbosity>=Cdrskin_verbose_debuG)
            fprintf(stderr,
                    "\ncdrskin_debug: Cdrfifo_try_to_work() returns %d\n",ret);
       }
     }
     if(ret==2) { /* <0 = error , 2 = work is done */
       if(skin->verbosity>=Cdrskin_verbose_debuG)
         fprintf(stderr,"\ncdrskin_debug: fifo ended work with ret=%d\n",ret);
       fifo_disabled= 1;
     }

#endif /* ! Cdrskin_no_cdrfifO */

   }

#else /* ! Cdrskin_extra_leaN */
   usleep(20000);
#endif /* Cdrskin_extra_leaN */

   loop_counter++;
 }
 skin->drive_is_busy= 0;
 if(skin->verbosity>=Cdrskin_verbose_progresS)
   printf("\n");

 if(Cdrskin__is_aborting(0))
   Cdrskin_abort(skin, 0); /* Never comes back */

 wrote_well = burn_drive_wrote_well(drive);
 if(skin->media_is_overwriteable && skin->grow_overwriteable_iso>0 &&
    wrote_well) {
   /* growisofs final stunt : update volume descriptors at start of media */
   ret= Cdrskin_grow_overwriteable_iso(skin,0);
   if(ret<=0)
     wrote_well= 0;
 }
 if(max_track<0) {
   printf("Track %-2.2d: Total bytes read/written: %.f/%.f (%.f sectors).\n",
          start_tno, total_count, total_count, total_count / sector_size);
 } else {
   Cdrtrack_get_size(skin->tracklist[max_track],&size,&padding,&sector_size,
                     &use_data_image_size,1);
   if (start_tno <= 0)
     start_tno = 1;
   printf("Track %-2.2d: Total bytes read/written: %.f/%.f (%.f sectors).\n",
         max_track + start_tno, size,size+padding,(size+padding)/sector_size);
 }
 if(skin->verbosity>=Cdrskin_verbose_progresS)
   printf("Writing  time:  %.3fs\n",Sfile_microtime(0)-start_time);


#ifndef Cdrskin_extra_leaN

#ifdef Cdrskin_use_libburn_fifO

 if(skin->fifo == NULL && skin->verbosity>=Cdrskin_verbose_progresS) {
   /* >>> this should rather be done for each track
          (for now this libburn_fifo should only be used with single track)
   */
   Cdrtrack_report_fifo(skin->tracklist[skin->track_counter - 1], 0);
 }

#endif /* Cdrskin_use_libburn_fifO */

#ifndef Cdrskin_no_cdrfifO

 if(skin->fifo!=NULL && skin->fifo_size>0 && wrote_well) {
   int dummy,final_fill;
   Cdrfifo_get_buffer_state(skin->fifo,&final_fill,&dummy,0);
   if(final_fill>0) {
fifo_full_at_end:;
     fprintf(stderr,
       "cdrskin: FATAL : Fifo still contains data after burning has ended.\n");
     fprintf(stderr,
       "cdrskin: FATAL : %.d bytes left.\n",final_fill);
     fprintf(stderr,
       "cdrskin: FATAL : This indicates an overflow of the last track.\n");
     fprintf(stderr,
     "cdrskin: NOTE : The media might appear ok but is probably truncated.\n");
     ret= -1; goto ex;
   }

#ifdef Cdrskin_libburn_leaves_inlet_opeN
   for(i= 0;i<skin->track_counter;i++) {
     ret= Cdrtrack_has_input_left(skin->tracklist[i],0);
     if(ret>0) {
       fprintf(stderr,
  "cdrskin: FATAL : Fifo outlet of track #%d is still buffering some bytes.\n",
               i+1);
       goto fifo_full_at_end;
     }
   }
#endif /* Cdrskin_libburn_leaves_inlet_opeN */

 }

 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   if(skin->fifo!=NULL && skin->fifo_size>0) {
     int dummy;

     Cdrfifo_get_min_fill(skin->fifo,&total_min_fill,&dummy,0);
     fifo_percent= 100.0*((double) total_min_fill)/(double) skin->fifo_size;
     if(fifo_percent==0 && total_min_fill>0)
       fifo_percent= 1;
     Cdrfifo_get_cdr_counters(skin->fifo,&put_counter,&get_counter,
                              &empty_counter,&full_counter,0);
     fflush(stdout);
     fprintf(stderr,"Cdrskin: fifo had %.f puts and %.f gets.\n",
             put_counter,get_counter);
     fprintf(stderr,
  "Cdrskin: fifo was %.f times empty and %.f times full, min fill was %d%%.\n",
             empty_counter,full_counter,fifo_percent);
   }
 }

#endif /* ! Cdrskin_no_cdrfifO */

 if(skin->verbosity>=Cdrskin_verbose_progresS) {
   drive_status= burn_drive_get_status(drive, &p);
   if(p.buffer_min_fill<=p.buffer_capacity && p.buffer_capacity>0) {
     num= 100.0 * ((double) p.buffer_min_fill)/(double) p.buffer_capacity;
     if(num<min_buffer_fill)
       min_buffer_fill= num; 
   }
   if(min_buffer_fill>100)
     min_buffer_fill= 50;
   printf("Min drive buffer fill was %d%%\n", min_buffer_fill);
 }

#endif /* ! Cdrskin_extra_leaN */

 ret= 1;
 if(wrote_well) {
   if(skin->verbosity>=Cdrskin_verbose_progresS) 
     printf("cdrskin: burning done\n");
 } else
   ret= 0;
ex:;
 if(ret<=0) {
   if(skin->verbosity>=Cdrskin_verbose_progresS) 
     printf("cdrskin: %s failed\n",doing);
   fprintf(stderr,"cdrskin: FATAL : %s failed.\n",doing);
 }
 skin->drive_is_busy= 0;
 if(skin->verbosity>=Cdrskin_verbose_debuG)
   ClN(printf("cdrskin_debug: do_eject= %d\n",skin->do_eject));
 for(i= 0;i<skin->track_counter;i++)
   Cdrtrack_cleanup(skin->tracklist[i],0);
 if(o != NULL)
   burn_write_opts_free(o);
 if(cuefile_fifo != NULL)
   burn_source_free(cuefile_fifo);
 burn_session_free(session);
 burn_disc_free(disc);
 return(ret);
}


#ifdef Libburn_develop_quality_scaN

int Cdrskin_qcheck(struct CdrskiN *skin, int flag)
{
 struct burn_drive *drive;
 int ret, rate_period, profile_number;
 char profile_name[80]; 

 printf("cdrskin: beginning to perform quality check on disc\n");
 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;

 ret= burn_disc_get_profile(drive, &profile_number, profile_name);
 if(ret <= 0)
   profile_number= 0;
 if(profile_number != 0x08 && profile_number != 0x09 && profile_number != 0x0a)
   rate_period= 8;
 else
   rate_period= 75;
 if(skin->do_qcheck == 1) {
   ret= burn_nec_optiarc_rep_err_rate(drive, 0, rate_period, 0);
   if(ret<=0)
     return(ret);
 }
 return(1);
}

#endif /* Libburn_develop_quality_scaN */


/** Print lba of first track of last session and Next Writeable Address of
    the next unwritten session.
*/
int Cdrskin_msinfo(struct CdrskiN *skin, int flag)
{
 int num_sessions, session_no, ret, num_tracks, open_sessions= 0;
 int nwa= -123456789, lba= -123456789, aux_lba;
 char msg[80];
 enum burn_disc_status s;
 struct burn_drive *drive;
 struct burn_disc *disc= NULL;
 struct burn_session **sessions= NULL;
 struct burn_track **tracks;
 struct burn_toc_entry toc_entry;

 ret= Cdrskin_grab_drive(skin,0);
 if(ret<=0)
   return(ret);
 drive= skin->drives[skin->driveno].drive;
 s= burn_disc_get_status(drive);
 if(s!=BURN_DISC_APPENDABLE) {
   if(skin->grow_overwriteable_iso==1 || skin->grow_overwriteable_iso==2) {
     lba= 0;
     ret= Cdrskin_overwriteable_iso_size(skin,&nwa,0);
     if(ret>0) {
       s= BURN_DISC_APPENDABLE;
       goto put_out;
     }
   }
   Cdrskin_report_disc_status(skin,s,0);
   fprintf(stderr,"cdrskin: FATAL : -msinfo can only operate on appendable (i.e. -multi) discs\n");
   if(skin->grow_overwriteable_iso>0)
     fprintf(stderr,"cdrskin:         or on overwriteables with existing ISO-9660 file system.\n");
   {ret= 0; goto ex;}
 }
 disc= burn_drive_get_disc(drive);
 if(disc==NULL) {
   /* No TOC available. Try to inquire directly. */
   ret= burn_disc_get_msc1(drive,&lba);
   if(ret>0)
     goto obtain_nwa;
   fprintf(stderr,"cdrskin: FATAL : Cannot obtain info about CD content\n");
   {ret= 0; goto ex;}
 }
 sessions= burn_disc_get_sessions(disc,&num_sessions);
 open_sessions= burn_disc_get_incomplete_sessions(disc);
 for(session_no= 0; session_no < num_sessions + open_sessions; session_no++) {
   tracks= burn_session_get_tracks(sessions[session_no],&num_tracks);
   if(tracks==NULL || num_tracks<=0)
 continue;
   burn_track_get_entry(tracks[0],&toc_entry);
   if(toc_entry.extensions_valid&1) { /* DVD extension valid */
     if(session_no >= num_sessions) {
       if(!(toc_entry.extensions_valid & 4))
 continue; /* open session with no track status bits from libburn */
       if((toc_entry.track_status_bits & (1 << 14)) ||
          !((toc_entry.track_status_bits & (1 << 16)) ||
            ((toc_entry.track_status_bits & (1 << 17)) &&
             toc_entry.last_recorded_address > toc_entry.start_lba)))
 continue; /* Blank or not appendable and not recorded */
     }   
     lba= toc_entry.start_lba;
   } else {
     lba= burn_msf_to_lba(toc_entry.pmin,toc_entry.psec,toc_entry.pframe);
   }
 }
 if(lba==-123456789) {
   fprintf(stderr,"cdrskin: FATAL : Cannot find any track on CD\n");
   {ret= 0; goto ex;}
 }

obtain_nwa:;
 ret= Cdrskin_obtain_nwa(skin,&nwa,flag);
 if(ret<=0) {
   if (sessions == NULL) {
     fprintf(stderr,
           "cdrskin: SORRY : Cannot obtain next writeable address\n");
     {ret= 0; goto ex;}
   }
   ClN(fprintf(stderr,
       "cdrskin: NOTE : Guessing next writeable address from leadout\n"));
   burn_session_get_leadout_entry(sessions[num_sessions-1],&toc_entry);
   if(toc_entry.extensions_valid&1) { /* DVD extension valid */
     aux_lba= toc_entry.start_lba;
   } else {
     aux_lba= burn_msf_to_lba(toc_entry.pmin,toc_entry.psec,toc_entry.pframe);
   }
   if(num_sessions>0)
     nwa= aux_lba+6900;
   else
     nwa= aux_lba+11400;
 }

put_out:;
 if(skin->preskin->result_fd>=0) {
   sprintf(msg,"%d,%d\n",lba,nwa);
   write(skin->preskin->result_fd,msg,strlen(msg));
 } else
   printf("%d,%d\n",lba,nwa);

 if(strlen(skin->msifile)) {
   FILE *fp;

   fp = fopen(skin->msifile, "w");
   if(fp==NULL) {
     if(errno>0)
       fprintf(stderr,"cdrskin: %s (errno=%d)\n", strerror(errno), errno);
     fprintf(stderr,"cdrskin: FATAL : Cannot write msinfo to file '%s'\n",
             skin->msifile);
     {ret= 0; goto ex;}
   }
   fprintf(fp,"%d,%d",lba,nwa);
   fclose(fp);
 }
 ret= 1;
ex:;
 return(ret);
}


/** Work around the failure of libburn to eject the tray.
    This employs a system(2) call and is therefore an absolute no-no for any
    pseudo user identities.
    @return <=0 error, 1 success
*/
int Cdrskin_eject(struct CdrskiN *skin, int flag)
{
 int i,ret,max_try= 5;

 if(!skin->do_eject)
   return(1);

 if((int) skin->n_drives <= skin->driveno || skin->driveno < 0)
   return(2);

 /* ??? A61012 : retry loop might now be obsolete 
                (a matching bug in burn_disc_write_sync() was removed ) */
 /* A61227 : A kindof race condition with -atip -eject on SuSE 9.3. Loop saved
             me. Waiting seems to help. I suspect the media demon. */

 for(i= 0;i<max_try;i++) {
   ret= Cdrskin_grab_drive(skin,2|((i<max_try-1)<<2)|16);
   if(ret>0 || i>=max_try-1)
 break;
   if(skin->verbosity>=Cdrskin_verbose_progresS)
     ClN(fprintf(stderr,
          "cdrskin: NOTE : Attempt #%d of %d failed to grab drive for eject\n",
          i+1,max_try));
   usleep(1000000);
 }
 if(ret>0) {
    ret= Cdrskin_release_drive(skin,1);
    if(ret<=0)
      goto sorry_failed_to_eject;
 } else {
sorry_failed_to_eject:;
   fprintf(stderr,"cdrskin: SORRY : Failed to finally eject tray.\n");
   return(0);
 }
 return(1);
}


/** Interpret all arguments of the program after libburn has been initialized
    and drives have been scanned. This call reports to stderr any valid 
    cdrecord options which are not implemented yet.
    @param flag Bitfield for control purposes: 
                bit0= do not finalize setup
                bit1= do not interpret (again) skin->preskin->pre_argv
    @return <=0 error, 1 success
*/
int Cdrskin_setup(struct CdrskiN *skin, int argc, char **argv, int flag)
{
 int i,k,l,ret, idx= -1, cd_start_tno;
 double value,grab_and_wait_value= -1.0, num;
 char *cpt,*value_pt,adr[Cdrskin_adrleN],*blank_mode= "", *argpt;
 struct stat stbuf;

 /* cdrecord 2.01 options which are not scheduled for implementation, yet */
 static char ignored_partial_options[][41]= {
   "timeout=", "debug=", "kdebug=", "kd=", "driver=", "ts=",
   "pregap=", "defpregap=",
   "pktsize=",
   ""
 };
 static char ignored_full_options[][41]= {
   "-d", "-silent", "-s", "-setdropts", "-prcap", 
   "-reset", "-abort", "-overburn", "-ignsize", "-useinfo",
   "-fix", "-nofix",
   "-raw", "-raw96p", "-raw16", "-raw96r",
   "-clone",
   "-cdi", 
   "-shorttrack", "-noshorttrack", "-packet", "-noclose",
   ""
 };

 /* are we pretending to be cdrecord ? */
 cpt= strrchr(argv[0],'/');
 if(cpt==NULL)
   cpt= argv[0];
 else
   cpt++;
 if(strcmp(cpt,"cdrecord")==0 && !(flag&1)) {
   fprintf(stderr,"\n");
   fprintf(stderr,
     "Note: This is not cdrecord by Joerg Schilling. Do not bother him.\n");
   fprintf(stderr,
     "      See cdrskin start message on stdout. See --help. See -version.\n");
   fprintf(stderr,"\n");
   /* allow automatic -tao to -sao redirection */
   skin->tao_to_sao_tsize=650*1024*1024;
 }

#ifndef Cdrskin_extra_leaN
 if(!(flag&2)) {
   if(skin->preskin->pre_argc>1) {
     ret= Cdrskin_setup(skin,skin->preskin->pre_argc,skin->preskin->pre_argv,
                        flag|1|2);
     if(ret<=0)
       return(ret);
   }
 }
#endif

 for (i= 1;i<argc;i++) {

   argpt= argv[i];
   if (strncmp(argpt, "--", 2) == 0 && strlen(argpt) > 3)
     argpt++;

   /* is this a known option which is not planned to be implemented ? */
   /* such an option will not be accepted as data source */
   for(k=0;ignored_partial_options[k][0]!=0;k++) {
     if(argpt[0]=='-')
       if(strncmp(argpt+1,ignored_partial_options[k],
                          strlen(ignored_partial_options[k]))==0)
         goto no_volunteer;
     if(strncmp(argpt,ignored_partial_options[k],
                      strlen(ignored_partial_options[k]))==0)
       goto no_volunteer;
   }
   for(k=0;ignored_full_options[k][0]!=0;k++) 
     if(strcmp(argpt,ignored_full_options[k])==0) 
       goto no_volunteer;
   if(0) {
no_volunteer:;
     skin->preskin->demands_cdrecord_caps= 1;
     if(skin->preskin->fallback_program[0])
       fprintf(stderr,"cdrskin: NOTE : Unimplemented option: '%s'\n",argpt);
     else  
       fprintf(stderr,"cdrskin: NOTE : ignoring unimplemented option : '%s'\n",
               argpt);
     fprintf(stderr,
       "cdrskin: NOTE : option is waiting for a volunteer to implement it.\n");
 continue;
   }

#ifndef Cdrskin_extra_leaN
   if(strncmp(argv[i],"abort_after_bytecount=",22)==0) {
     skin->abort_after_bytecount= Scanf_io_size(argv[i]+22,0);
     fprintf(stderr,
             "cdrskin: NOTE : will perform synthetic abort after %.f bytes\n",
             skin->abort_after_bytecount);

   } else if(strcmp(argv[i],"--abort_handler")==0) {
#else /* ! Cdrskin_extra_leaN */
   if(strcmp(argv[i],"--abort_handler")==0) {
#endif
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i],"-abort_max_wait=",16)==0) {
     value_pt= argv[i]+16;
     goto set_abort_max_wait;

   } else if(strncmp(argv[i],"abort_max_wait=",15)==0) {
     value_pt= argv[i]+15;
set_abort_max_wait:;
     value= Scanf_io_size(value_pt,0);
     if(value<0 || value>86400) {
       fprintf(stderr,
             "cdrskin: NOTE : ignored out-of-range value: abort_max_wait=%s\n",
             value_pt);
     } else {
       skin->abort_max_wait= value;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         printf(
            "cdrskin: maximum waiting time with abort handling : %d seconds\n",
            skin->abort_max_wait);
     }

   } else if(strcmp(argv[i],"--adjust_speed_to_drive")==0) {
     skin->adjust_speed_to_drive= 1;

   } else if(strcmp(argv[i],"--allow_emulated_drives")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--allow_setuid")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--allow_untested_media")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--any_track")==0) {
     skin->single_track= -1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf(
   "cdrskin: --any_track : will accept any unknown option as track source\n"));

   } else if(strncmp(argv[i],"assert_write_lba=",17)==0) {
     value_pt= argv[i]+17;
     value= Scanf_io_size(value_pt,0);
     l= strlen(value_pt);
     if(l>1) if(isalpha(value_pt[l-1]))
       value/= 2048.0;
     skin->assert_write_lba= value; 

   } else if(strcmp(argpt,"-atip")==0) {
     if(skin->do_atip<1)
       skin->do_atip= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: will put out some -atip style lines\n"));

   } else if(strcmp(argpt,"-audio")==0) {
     skin->track_type= BURN_AUDIO;
     skin->track_type_by_default= 0;

   } else if(strncmp(argpt,"-blank=",7)==0) {
     cpt= argpt + 7;
     goto set_blank;
   } else if(strncmp(argpt,"blank=",6)==0) {
     cpt= argpt + 6;
set_blank:;
     skin->blank_format_type= 0;
     blank_mode= cpt;
     if(strcmp(cpt,"all")==0 || strcmp(cpt,"disc")==0 
        || strcmp(cpt,"disk")==0) {
       skin->do_blank= 1;
       skin->blank_fast= 0;
       blank_mode= "all";
     } else if(strcmp(cpt,"fast")==0 || strcmp(cpt,"minimal")==0) { 
       skin->do_blank= 1;
       skin->blank_fast= 1;
       blank_mode= "fast";
     } else if(strcmp(cpt,"format_if_needed")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 6;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strcmp(cpt,"format_overwrite")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 1|(1<<8);
       skin->blank_format_size= 128*1024*1024;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strcmp(cpt,"format_overwrite_full")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 1|(1<<10);
       skin->blank_format_size= 0;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strcmp(cpt,"format_overwrite_quickest")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 1;
       skin->blank_format_size= 0;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strncmp(cpt,"format_defectmgt",16)==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 4|(3<<9);  /* default payload size */
       skin->blank_format_size= 0;
       skin->preskin->demands_cdrskin_caps= 1;
       if(cpt[16]=='_') {
         cpt+= 17;
         if(strcmp(cpt,"none")==0)
           skin->blank_format_type= 4|(1<<13);
         else if(strcmp(cpt,"max")==0)
           skin->blank_format_type= 4;  /* smallest payload size above 0 */
         else if(strcmp(cpt,"min")==0)
           skin->blank_format_type= 4|(2<<9); /*largest payload size with mgt*/
         else if(strncmp(cpt,"payload_",8)==0) {
           skin->blank_format_size= Scanf_io_size(cpt+8,0);
           skin->blank_format_type= 4;
         } else if(strcmp(cpt,"cert_off")==0)
           skin->blank_format_no_certify= 1;
         else if(strcmp(cpt,"cert_on")==0)
           skin->blank_format_no_certify= 0;
         else
           goto unsupported_blank_option;
       }
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strncmp(cpt,"format_by_index_",16)==0) {
       sscanf(cpt+16, "%d", &idx);
       if(idx<0 || idx>255) { 
         fprintf(stderr,"cdrskin: SORRY : blank=%s provides unusable index\n",
                 cpt);
         return(0);
       }
       skin->do_blank= 1;
       skin->blank_format_type= 5|(2<<9)|(1<<15);
       skin->blank_format_index= idx;
       skin->blank_format_size= 0;
       skin->preskin->demands_cdrskin_caps= 1;

     } else if(strcmp(cpt,"deformat_sequential")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 2;
       skin->blank_fast= 0;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strcmp(cpt,"deformat_sequential_quickest")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 2;
       skin->blank_fast= 1;
       skin->preskin->demands_cdrskin_caps= 1;
     } else if(strcmp(cpt,"as_needed")==0) {
       skin->do_blank= 1;
       skin->blank_format_type= 7;
     } else if(strcmp(cpt,"help")==0) {
       /* is handled in Cdrpreskin_setup() */;
 continue;
     } else { 
unsupported_blank_option:;
       fprintf(stderr,"cdrskin: FATAL : Blank option '%s' not supported yet\n",
                      cpt);
       return(0);
     }
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: blank mode : blank=%s\n",blank_mode));

   } else if(strcmp(argv[i],"--bragg_with_audio")==0) {
     /* OBSOLETE 0.2.3 : was handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i], "-cd_start_tno=", 14) == 0) {
     value_pt= argv[i] + 14;
     goto set_cd_start_tno;
   } else if(strncmp(argv[i], "cd_start_tno=", 13) == 0) {
     value_pt= argv[i] + 13;
set_cd_start_tno:;
     cd_start_tno= -1;
     sscanf(value_pt, "%d", &cd_start_tno);
     if(cd_start_tno < 1 || cd_start_tno > 99) {
       fprintf(stderr,
         "cdrskin: FATAL : cd_start_tno= gives a number outside [1 ... 99]\n");
       return(0);
     }
     skin->cd_start_tno= cd_start_tno;

   } else if(strcmp(argv[i],"--cdtext_dummy")==0) {
     skin->cdtext_test= 2;

   } else if(strncmp(argv[i], "-cdtext_to_textfile=", 20) == 0) {
     value_pt= argv[i] + 20;
     goto set_cdtext_to_textfile;
   } else if(strncmp(argv[i], "cdtext_to_textfile=", 19) == 0) {
     value_pt= argv[i] + 19;
set_cdtext_to_textfile:;
     if(strlen(value_pt) >= sizeof(skin->cdtext_to_textfile_path)) {
       fprintf(stderr,
    "cdrskin: FATAL : cdtext_to_textfile=... too long. (max: %d, given: %d)\n",
    (int) sizeof(skin->cdtext_to_textfile_path)-1,(int) strlen(value_pt));
       return(0);
     }
     skin->do_cdtext_to_textfile= 1;
     strcpy(skin->cdtext_to_textfile_path, value_pt);

   } else if(strncmp(argv[i], "-cdtext_to_v07t=", 16) == 0) {
     value_pt= argv[i] + 16;
     goto set_cdtext_to_v07t;
   } else if(strncmp(argv[i], "cdtext_to_v07t=", 15) == 0) {
     value_pt= argv[i] + 15;
set_cdtext_to_v07t:;
     if(strlen(value_pt) >= sizeof(skin->cdtext_to_vt07_path)) {
       fprintf(stderr,
        "cdrskin: FATAL : cdtext_to_vt07=... too long. (max: %d, given: %d)\n",
        (int) sizeof(skin->cdtext_to_vt07_path)-1,(int) strlen(value_pt));
       return(0);
     }
     skin->do_cdtext_to_vt07= 1;
     strcpy(skin->cdtext_to_vt07_path, value_pt);

   } else if(strcmp(argv[i],"--cdtext_verbose")==0) {
     skin->cdtext_test= 1;

   } else if(strcmp(argpt,"-checkdrive")==0) {
     skin->do_checkdrive= 1;

   } else if(strcmp(argpt,"-copy")==0) {
     skin->track_modemods|= BURN_COPY;
     skin->track_modemods&= ~BURN_SCMS;

   } else if(strncmp(argpt, "-cuefile=", 9)==0) {
     value_pt= argpt + 9;
     goto set_cuefile;
   } else if(strncmp(argpt, "cuefile=", 8)==0) {
     value_pt= argpt + 8;
set_cuefile:;
     if(strlen(value_pt) >= sizeof(skin->cuefile)) {
       fprintf(stderr,
          "cdrskin: FATAL : cuefile=... too long. (max: %d, given: %d)\n",
          (int) sizeof(skin->cuefile) - 1, (int) strlen(value_pt));
       return(0);
     }
     strcpy(skin->cuefile, value_pt);
     skin->do_burn= 1;

   } else if(strcmp(argpt,"-data")==0) {
option_data:;
     /* All Subsequent Tracks Option */
     skin->cdxa_conversion= (skin->cdxa_conversion & ~0x7fffffff) | 0;
     skin->track_type= BURN_MODE1;
     skin->track_type_by_default= 0;

   } else if(strcmp(argv[i],"--demand_a_drive")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--devices")==0) {
     skin->do_devices= 1;

   } else if(strcmp(argv[i],"--device_links")==0) {
     skin->do_devices= 2;

#ifndef Cdrskin_extra_leaN

   } else if(strncmp(argv[i],"dev_translation=",16)==0) {
     if(argv[i][16]==0) {
       fprintf(stderr,
         "cdrskin: FATAL : dev_translation= : missing separator character\n");
       return(0);
     }
     ret= Cdradrtrn_add(skin->adr_trn,argv[i]+17,argv[i]+16,1);
     if(ret==-2)
       fprintf(stderr,
           "cdrskin: FATAL : address_translation= : cannot allocate memory\n");
     else if(ret==-1)
       fprintf(stderr,
             "cdrskin: FATAL : address_translation= : table full (%d items)\n",
               Cdradrtrn_leN);
     else if(ret==0)
       fprintf(stderr,
   "cdrskin: FATAL : address_translation= : no address separator '%c' found\n",
               argv[i][17]);
     if(ret<=0)
       return(0);

#endif /* ! Cdrskin_extra_leaN */


   } else if(strncmp(argpt,"-dev=",5)==0) {
     /* is handled in Cdrpreskin_setup() */;
   } else if(strncmp(argpt,"dev=",4)==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i],"direct_write_amount=",20)==0) {
     skin->direct_write_amount= Scanf_io_size(argv[i]+20,0);
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: amount for direct writing : %.f\n",
                  skin->direct_write_amount));
     if(skin->direct_write_amount>=0.0) {
       skin->do_direct_write= 1;
       printf("cdrskin: NOTE : Direct writing will only use first track source and no fifo.\n");
       skin->preskin->demands_cdrskin_caps= 1;
     } else
       skin->do_direct_write= 0;

   } else if(strcmp(argv[i],"--drive_abort_on_busy")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--drive_blocking")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--drive_not_exclusive")==0 ||
             strcmp(argv[i],"--drive_not_f_setlk")==0 ||
             strcmp(argv[i],"--drive_not_o_excl")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i],"drive_scsi_dev_family=",22)==0) {
     /* is handled in Cdrpreskin_setup() */;
     if(skin->verbosity>=Cdrskin_verbose_debuG &&
        skin->preskin->drive_scsi_dev_family!=0)
       ClN(fprintf(stderr,"cdrskin_debug: drive_scsi_dev_family= %d\n",
                        skin->preskin->drive_scsi_dev_family));

   } else if(strcmp(argv[i],"--drive_scsi_exclusive")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argpt, "-driveropts=", 12)==0) {
     value_pt= argpt + 12;
     goto set_driveropts;
   } else if(strncmp(argpt, "driveropts=", 11)==0) {
     value_pt= argpt + 11;
set_driveropts:;
     if(strcmp(value_pt,"burnfree")==0 || strcmp(value_pt,"burnproof")==0) {
       skin->burnfree= 1;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         ClN(printf("cdrskin: burnfree : on\n"));
     } else if(strcmp(argpt+11,"noburnfree")==0 ||
               strcmp(argpt+11,"noburnproof")==0 ) {
       skin->burnfree= 0;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         ClN(printf("cdrskin: burnfree : off\n"));
     } else if(strcmp(argpt+11,"help")==0) {
       /* handled in Cdrpreskin_setup() */;
     } else 
       goto ignore_unknown;

   } else if(strcmp(argpt,"-dummy")==0) {
     skin->dummy_mode= 1;

   } else if(strncmp(argv[i], "-dvd_obs=", 9)==0) {
     value_pt= argv[i] + 9;
     goto dvd_obs;
   } else if(strncmp(argv[i], "dvd_obs=", 8)==0) {
     value_pt= argv[i] + 8;
dvd_obs:;
     if(strcmp(value_pt, "default") == 0)
       num= 0;
     else
       num = Scanf_io_size(value_pt,0);
     if(num != 0 && num != 32768 && num != 65536) {
       fprintf(stderr,
       "cdrskin: SORRY : Option dvd_obs= accepts only sizes 0, 32k, 64k\n");
     } else
       skin->dvd_obs= num;

   } else if(strcmp(argpt,"-eject")==0) {
     skin->do_eject= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: eject after work : on\n"));

   } else if(strncmp(argv[i],"eject_device=",13)==0) {
     if(strlen(argv[i]+13)>=sizeof(skin->eject_device)) {
       fprintf(stderr,
          "cdrskin: FATAL : eject_device=... too long. (max: %d, given: %d)\n",
          (int) sizeof(skin->eject_device)-1,(int) strlen(argv[i]+13));
       return(0);
     }
     strcpy(skin->eject_device,argv[i]+13);
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: ignoring obsolete  eject_device=%s\n",
              skin->eject_device));

   } else if(strncmp(argv[i],"-extract_audio_to=", 18)==0) {
     value_pt= argpt + 18;
     goto extract_audio_to;
   } else if(strncmp(argpt, "extract_audio_to=", 17) == 0) {
     value_pt= argpt + 17;
extract_audio_to:;
     if(strlen(value_pt) >= sizeof(skin->extract_audio_dir)) {
       fprintf(stderr,
               "cdrskin: FAILURE : extract_audio_to=... is much too long.\n");
       return(0);
     }
     skin->do_extract_audio= 1;
     strcpy(skin->extract_audio_dir, value_pt);

   } else if(strncmp(argv[i],"-extract_tracks=", 16)==0) {
     value_pt= argpt + 16;
     goto extract_tracks;
   } else if(strncmp(argpt, "extract_tracks=", 15) == 0) {
     value_pt= argpt + 15;
extract_tracks:;
     value= 0.0;
     for(cpt= value_pt; ; cpt++) {
       if(*cpt >= '0' && *cpt <= '9') {
         value= value * 10 + *cpt - '0';
       } else {
         if(value >= 1.0 && value <= 99.0) {
           skin->extract_audio_tracks[(int) value]= 1;
           if(skin->verbosity >= Cdrskin_verbose_cmD)
             fprintf(stderr, "cdrskin: Will extract track number %.f\n",
                             value);
         } else {
           fprintf(stderr,
           "cdrskin: WARNING : extract_tracks= with unsuitable number: %.f\n",
                   value);
         }
         if(*cpt == 0)
     break;
         value= 0;
       }
     }

   } else if(strncmp(argv[i],"-extract_basename=", 18)==0) {
     value_pt= argpt + 18;
     goto extract_basename;
   } else if(strncmp(argpt, "extract_basename=", 17) == 0) {
     value_pt= argpt + 17;
extract_basename:;
     if(strchr(value_pt, '/') != NULL) {
       fprintf(stderr,
         "cdrskin: FAILURE : extract_basename=... may not contain '/'\n");
       return(0);
     }
     if(strlen(value_pt) > 248) {
       fprintf(stderr,
         "cdrskin: FAILURE : Oversized extract_basename=... (Max. 248)\n");
       return(0);
     }
     strcpy(skin->extract_basename, value_pt);

   } else if(strcmp(argv[i],"--extract_dap") == 0) {
     skin->extract_flags|= 8;

#ifndef Cdrskin_extra_leaN

   } else if(strcmp(argv[i],"--fifo_disable")==0) {
     skin->fifo_enabled= 0;
     skin->fifo_size= 0;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: option fs=... disabled\n"));

   } else if(strcmp(argv[i],"--fifo_start_empty")==0) { /* obsoleted */
     skin->fifo_start_at= 0;

   } else if(strncmp(argv[i],"fifo_start_at=",14)==0) {
     value= Scanf_io_size(argv[i]+14,0);
     if(value>1024.0*1024.0*1024.0)
       value= 1024.0*1024.0*1024.0;
     else if(value<0)
       value= 0;
     skin->fifo_start_at= value;

   } else if(strcmp(argv[i],"--fifo_per_track")==0) {
     skin->fifo_per_track= 1;

#endif /* ! Cdrskin_extra_leaN */

   } else if(strcmp(argv[i],"--fill_up_media")==0) {
     skin->fill_up_media= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf(
               "cdrskin: will fill up last track to full free media space\n"));

   } else if(strcmp(argpt,"-force")==0) {
     skin->force_is_set= 1;

   } else if(strcmp(argpt,"-format")==0) {
     skin->do_blank= 1;
     skin->blank_format_type= 3|(1<<10);
     skin->blank_format_size= 0;
     skin->force_is_set= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf(
       "cdrskin: will format DVD+RW by blank=format_overwrite_full -force\n"));

   } else if(strcmp(argv[i],"--four_channel")==0) {
     skin->track_modemods|= BURN_4CH;

#ifndef Cdrskin_extra_leaN

   } else if(strncmp(argpt, "-fs=", 4) == 0) {
     value_pt= argpt + 4;
     goto fs_equals;
   } else if(strncmp(argpt, "fs=", 3) == 0) {
     value_pt= argpt + 3;
fs_equals:;
     if(skin->fifo_enabled) {
       value= Scanf_io_size(value_pt,0);
       if(value<0.0 || value>1024.0*1024.0*1024.0) {
         fprintf(stderr,
                 "cdrskin: FATAL : fs=N expects a size between 0 and 1g\n");
         return(0);
       }
       skin->fifo_size= value;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         printf("cdrskin: fifo size : %d\n",skin->fifo_size);
     }

   } else if(strncmp(argv[i],"grab_drive_and_wait=",20)==0) {
     value_pt= argv[i]+20;
     grab_and_wait_value= Scanf_io_size(value_pt,0);
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strncmp(argpt, "-gracetime=", 11) == 0) {
     value_pt= argpt + 11;
     goto gracetime_equals;
   } else if(strncmp(argpt, "gracetime=", 10) == 0) {
     value_pt= argpt + 10;
gracetime_equals:;
     sscanf(value_pt,"%d",&(skin->gracetime));

   } else if(strncmp(argv[i],"--grow_overwriteable_iso",24)==0) {
     skin->grow_overwriteable_iso= 1;
     skin->use_data_image_size= 1;
     skin->preskin->demands_cdrskin_caps= 1;

#else /* ! Cdrskin_extra_leaN */

   } else if(
      strcmp(argv[i],"--fifo_disable")==0 ||
      strcmp(argv[i],"--fifo_start_empty")==0 ||
      strncmp(argv[i],"fifo_start_at=",14)==0 ||
      strcmp(argv[i],"--fifo_per_track")==0 ||
      strncmp(argv[i],"-fs=",4)==0 ||
      strncmp(argv[i],"fs=",3)==0 ||
      strncmp(argv[i],"-gracetime=",11)==0 ||
      strncmp(argv[i],"gracetime=",10)==0) {
     fprintf(stderr,
             "cdrskin: NOTE : lean version ignores option: '%s'\n",
             argv[i]);

#endif /* Cdrskin_extra_leaN */

   } else if(strcmp(argv[i],"--help")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"-help")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--ignore_signals")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argpt,"-immed")==0) {
     skin->modesty_on_drive= 1;
     skin->min_buffer_percent= 75;
     skin->max_buffer_percent= 95;

   } else if(strncmp(argpt, "-index=", 7) == 0) {
     value_pt= argpt + 7;
     goto set_index;
   } else if(strncmp(argpt, "index=", 6) == 0) {
     value_pt= argpt + 6;
set_index:;
     if(skin->index_string != NULL)
       free(skin->index_string);
     skin->index_string= strdup(value_pt);
     if(skin->index_string == NULL) {
       fprintf(stderr, "cdrskin: FATAL : Out of memory\n");
       return(-1);
     }

   } else if(strncmp(argv[i], "input_sheet_v07t=", 17)==0) {
     if(skin->sheet_v07t_blocks >= 8) {
       fprintf(stderr,
            "cdrskin: SORRY : Too many input_sheet_v07t= options. (Max. 8)\n");
       return(0);
     }
     if(argv[i][17] == 0) {
       fprintf(stderr,
        "cdrskin: SORRY : Missing file path after option input_sheet_v07t=\n");
       return(0);
     }
     if(strlen(argv[i] + 17) > Cdrskin_adrleN) {
       fprintf(stderr,
       "cdrskin: SORRY : File path too long after option input_sheet_v07t=\n");
       return(0);
     }
     strcpy(skin->sheet_v07t_paths[skin->sheet_v07t_blocks], argv[i] + 17);
     skin->sheet_v07t_blocks++;
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strcmp(argpt,"-inq")==0) {
     skin->do_checkdrive= 2;

   } else if(strcmp(argpt,"-isosize")==0) {
     skin->use_data_image_size= 1;

   } else if(strncmp(argpt, "-isrc=", 6) == 0) {
     value_pt= argpt + 6;
     goto set_isrc;
   } else if(strncmp(argpt, "isrc=", 5) == 0) {
     value_pt= argpt + 5;
set_isrc:;
     if(strlen(value_pt) != 12) {
       fprintf(stderr,
          "cdrskin: SORRY : isrc=... is not exactly 12 characters long.\n");
       return(0);
     }
     memcpy(skin->next_isrc, value_pt, 13);

   } else if(strcmp(argv[i],"--list_formats")==0) {
     skin->do_list_formats= 1;
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strcmp(argv[i],"--list_ignored_options")==0) {
     /* is also handled in Cdrpreskin_setup() */;

     printf("cdrskin: List of all ignored options:\n");
     for(k=0;ignored_partial_options[k][0]!=0;k++)
       printf("%s\n",ignored_partial_options[k]);
     for(k=0;ignored_full_options[k][0]!=0;k++) 
       printf("%s\n",ignored_full_options[k]);
     printf("\n");

   } else if(strcmp(argv[i],"--list_speeds")==0) {
     skin->do_list_speeds= 1;
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strncmp(argv[i],"fallback_program=",17)==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argpt,"-load")==0) {
     skin->do_load= 1;

   } else if(strcmp(argpt,"-lock")==0) {
     skin->do_load= 2;

   } else if(strcmp(argv[i],"--long_toc")==0) {
     skin->do_atip= 3;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: will put out some -atip style lines plus -toc\n"));

   } else if(strncmp(argpt,"-mcn=", 5) == 0) {
     value_pt= argpt + 5;
     goto set_mcn;
   } else if(strncmp(argpt,"mcn=", 4) == 0) {
     value_pt= argpt + 4;
set_mcn:;
     if(strlen(value_pt) != 13) {
       fprintf(stderr,
          "cdrskin: SORRY : mcn=... is not exactly 13 characters long.\n");
       return(0);
     }
     memcpy(skin->mcn, value_pt, 14);

   } else if(strncmp(argpt, "-minbuf=", 8) == 0) {
     value_pt= argpt + 8;
     goto minbuf_equals;
   } else if(strncmp(argpt, "minbuf=", 7) == 0) {
     value_pt= argpt + 7;
minbuf_equals:;
     skin->modesty_on_drive= 1;
     sscanf(value_pt,"%lf",&value);
     if (value<25 || value>95) {
       fprintf(stderr,
               "cdrskin: FATAL : minbuf= value must be between 25 and 95\n");
       return(0);
     }
     skin->min_buffer_percent= value;
     skin->max_buffer_percent= 95;
     ClN(printf("cdrskin: minbuf=%d percent desired buffer fill\n",
                skin->min_buffer_percent));

   } else if(strcmp(argpt,"-minfo") == 0 ||
             strcmp(argpt,"-media-info") == 0) {
     skin->do_atip= 4;

   } else if(strcmp(argpt, "-mode2") == 0) {
     fprintf(stderr,
             "cdrskin: NOTE : defaulting option -mode2 to option -data\n");
     goto option_data;

   } else if(strncmp(argv[i],"modesty_on_drive=",17)==0) {
     value_pt= argv[i]+17;
     if(*value_pt=='0') {
       skin->modesty_on_drive= 0;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         ClN(printf(
               "cdrskin: modesty_on_drive=0 : buffer waiting by os driver\n"));
     } else if(*value_pt=='1') {
       skin->modesty_on_drive= 1;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         ClN(printf(
                 "cdrskin: modesty_on_drive=1 : buffer waiting by libburn\n"));
     } else if(*value_pt=='-' && argv[i][18]=='1') {
       skin->modesty_on_drive= -1;
       if(skin->verbosity>=Cdrskin_verbose_cmD)
         ClN(printf(
       "cdrskin: modesty_on_drive=-1 : buffer waiting as libburn defaults\n"));
     } else {
       fprintf(stderr,
               "cdrskin: FATAL : modesty_on_drive= must be -1, 0 or 1\n");
       return(0);
     }
     while(1) {
       value_pt= strchr(value_pt,':');
       if(value_pt==NULL)
     break;
       value_pt++;
       if(strncmp(value_pt,"min_percent=",12)==0) {
         sscanf(value_pt+12,"%lf",&value);
         if (value<25 || value>100) {
           fprintf(stderr,
                   "cdrskin: FATAL : modest_on_drive= min_percent value must be between 25 and 100\n");
           return(0);
         }
         skin->min_buffer_percent= value;
         ClN(printf("cdrskin: modesty_on_drive : %d percent min buffer fill\n",
                skin->min_buffer_percent));
       } else if(strncmp(value_pt,"max_percent=",12)==0) {
         sscanf(value_pt+12,"%lf",&value);
         if (value<25 || value>100) {
           fprintf(stderr,
                   "cdrskin: FATAL : modest_on_drive= max_percent value must be between 25 and 100\n");
           return(0);
         }
         skin->max_buffer_percent= value;
         ClN(printf("cdrskin: modesty_on_drive : %d percent max buffer fill\n",
                skin->max_buffer_percent));
       } else {
         fprintf(stderr,
                "cdrskin: SORRY : modest_on_drive= unknown option code : %s\n",
                value_pt);
       }
     }
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strcmp(argpt,"-multi")==0) {
     skin->multi= 1;

   } else if(strncmp(argpt, "-msifile=", 9) == 0) {
     value_pt= argpt + 9;
     goto msifile_equals;
   } else if(strncmp(argpt, "msifile=", 8) == 0) {
     value_pt= argpt + 8;
msifile_equals:;
     if(strlen(value_pt)>=sizeof(skin->msifile)) {
       fprintf(stderr,
          "cdrskin: FATAL : msifile=... too long. (max: %d, given: %d)\n",
          (int) sizeof(skin->msifile)-1,(int) strlen(value_pt));
       return(0);
     }
     strcpy(skin->msifile, value_pt);
     skin->do_msinfo= 1;

   } else if(strcmp(argpt,"-msinfo")==0) {
     skin->do_msinfo= 1;

#ifdef Libburn_develop_quality_scaN

   } else if(strcmp(argv[i],"--nec_optiarc_qcheck")==0) {
     skin->do_qcheck= 1;

#endif /* Libburn_develop_quality_scaN */

   } else if(strcmp(argv[i],"--no_abort_handler")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--no_blank_appendable")==0) {
     skin->no_blank_appendable= 1;

   } else if(strcmp(argv[i],"--no_convert_fs_adr")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i],"--no_load")==0) {
     skin->do_load= -1;

   } else if(strcmp(argv[i],"--no_rc")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argpt,"-nocopy")==0) {
     skin->track_modemods&= ~BURN_COPY;

   } else if(strcmp(argpt,"-nopad")==0) {
     skin->padding= 0.0;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: padding : off\n"));

   } else if(strcmp(argpt,"-nopreemp")==0) {
     skin->track_modemods&= ~BURN_PREEMPHASIS;

   } else if(strcmp(argv[i],"--obs_pad")==0) {
     skin->obs_pad= 1;

   } else if(strcmp(argv[i],"--old_pseudo_scsi_adr")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argv[i], "--pacifier_with_newline") == 0) {
     skin->pacifier_with_newline= 1;

   } else if(strcmp(argpt,"-pad")==0) {
     skin->padding= 15*2048;
     skin->set_by_padsize= 0;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: padding : %.f\n",skin->padding));

   } else if(strncmp(argpt, "-padsize=", 9) == 0) {
     value_pt= argpt + 9;
     goto set_padsize;
   } else if(strncmp(argpt, "padsize=", 8) == 0) {
     value_pt= argpt + 8;
set_padsize:;
     skin->padding= Scanf_io_size(value_pt, 0);
     skin->set_by_padsize= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: padding : %.f\n",skin->padding));

   } else if(strcmp(argpt,"-preemp")==0) {
     skin->track_modemods|= BURN_PREEMPHASIS;

   } else if(strcmp(argv[i],"--prodvd_cli_compatible")==0) {
     skin->prodvd_cli_compatible= 1;

   } else if(strcmp(argpt,"-sao")==0 || strcmp(argpt,"-dao")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argpt,"-scanbus")==0) {
     skin->do_scanbus= 1;

   } else if(strcmp(argpt,"-scms")==0) {
     skin->track_modemods|= BURN_SCMS;

   } else if(strcmp(argv[i],"--single_track")==0) {
     skin->single_track= 1;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf(
"cdrskin: --single_track : will only accept last argument as track source\n"));
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strncmp(argv[i], "-sao_postgap=", 13) == 0) {
     value_pt= argv[i] + 13;
     goto set_sao_postgap;
   } else if(strncmp(argv[i], "sao_postgap=", 12) == 0) {
     value_pt= argv[i] + 12;
set_sao_postgap:;
     skin->sao_postgap= -1;
     if(strcmp(value_pt, "off") == 0)
       skin->sao_postgap = -1;
     else
       sscanf(value_pt, "%d", &(skin->sao_postgap));
     if(skin->sao_postgap < 0) {
       fprintf(stderr,
            "cdrskin: SORRY : sao_postgap must be \"off\" or a number >= 0\n");
       return(0);
     }

   } else if(strncmp(argv[i], "-sao_pregap=", 12) == 0) {
     value_pt= argv[i] + 12;
     goto set_sao_pregap;
   } else if(strncmp(argv[i], "sao_pregap=", 11) == 0) {
     value_pt= argv[i] + 11;
set_sao_pregap:;
     skin->sao_pregap= -1;
     if(strcmp(value_pt, "off") == 0)
       skin->sao_pregap = -1;
     else
       sscanf(value_pt, "%d", &(skin->sao_pregap));
     if(skin->sao_pregap < 0) {
       fprintf(stderr,
             "cdrskin: SORRY : sao_pregap must be \"off\" or a number >= 0\n");
       return(0);
     }

   } else if(strncmp(argpt, "-speed=", 7) == 0) {
     value_pt= argpt + 7;
     goto set_speed;
   } else if(strncmp(argpt, "speed=", 6) == 0) {
     value_pt= argpt + 6;
set_speed:;
     if(strcmp(value_pt,"any")==0)
       skin->x_speed= -1;
     else
       sscanf(value_pt,"%lf",&(skin->x_speed));
     if(skin->x_speed<1.0 && skin->x_speed!=0.0 && skin->x_speed!=-1) {
       fprintf(stderr,"cdrskin: FATAL : speed= must be -1, 0 or at least 1\n");
       return(0);
     }
     if(skin->x_speed<0)
       skin->preskin->demands_cdrskin_caps= 1;

     /* >>> cdrecord speed=0 -> minimum speed , libburn -> maximum speed */;

     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: speed : %f\n",skin->x_speed));

   } else if(strncmp(argv[i], "-stdio_sync=", 12)==0) {
     value_pt= argv[i] + 12;
     goto stdio_sync;
   } else if(strncmp(argv[i], "stdio_sync=", 11)==0) {
     value_pt= argv[i] + 11;
stdio_sync:;
     if(strcmp(value_pt, "default") == 0 || strcmp(value_pt, "on") == 0)
       num= 0;
     else if(strcmp(value_pt, "off") == 0)
       num= -1;
     else
       num = Scanf_io_size(value_pt,0);
     if(num > 0)
       num/= 2048;
     if(num != -1 && num != 0 && (num < 32 || num > 512 * 1024)) {
       fprintf(stderr,
 "cdrskin: SORRY : Option stdio_sync= accepts only sizes -1, 0, 32k ... 1g\n");
     } else
       skin->stdio_sync= num;

   } else if(strncmp(argv[i],"-stream_recording=",18)==0) {
     value_pt= argv[i]+18;
     goto set_stream_recording;
   } else if(strncmp(argv[i],"stream_recording=",17)==0) {
     value_pt= argv[i]+17;
set_stream_recording:;
     if(strcmp(value_pt, "on")==0)
       skin->stream_recording_is_set= 1;
     else if(value_pt[0] >= '0' && value_pt[0] <= '9') {
       num= Scanf_io_size(value_pt, 0);
       num/= 2048.0;
       if(num >= 16 && num <= 0x7FFFFFFF)
         skin->stream_recording_is_set= num;
       else
         skin->stream_recording_is_set= 0;
     } else
       skin->stream_recording_is_set= 0;
   } else if(strcmp(argpt,"-swab")==0) {
     skin->swap_audio_bytes= 0;

   } else if(strcmp(argpt,"-tao")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i],"tao_to_sao_tsize=",17)==0) {
     skin->tao_to_sao_tsize= Scanf_io_size(argv[i]+17,0);
     if(skin->tao_to_sao_tsize>Cdrskin_tracksize_maX)
       goto track_too_large;
     skin->preskin->demands_cdrskin_caps= 1;

#ifndef Cdrskin_extra_leaN
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       printf("cdrskin: size default for non-tao write modes: %.f\n",
              skin->tao_to_sao_tsize);
#endif /* ! Cdrskin_extra_leaN */

   } else if(strcmp(argv[i],"--tell_media_space")==0) {
     skin->tell_media_space= 1;
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strcmp(argpt, "-text") == 0) {
     skin->use_cdtext= 1;

   } else if(strncmp(argpt, "-textfile=", 10) == 0) {
     value_pt= argpt + 10;
     goto set_textfile;
   } else if(strncmp(argpt ,"textfile=", 9) == 0) {
     value_pt= argpt + 9;
set_textfile:;
     ret= Cdrskin_read_textfile(skin, value_pt, 0);
     if(ret <= 0)
       return(ret);

   } else if(strncmp(argpt, "-textfile_to_v07t=", 18) == 0) {
     value_pt= argpt + 18;
     goto textfile_to_v07t;
   } else if(strncmp(argpt ,"textfile_to_v07t=", 17) == 0) {
     value_pt= argpt + 17;
textfile_to_v07t:;
     ret= Cdrskin_read_textfile(skin, value_pt, 0);
     if(ret <= 0)
       return(ret);
     ret= Cdrskin_print_text_packs(skin, skin->text_packs,
                                   skin->num_text_packs, (1 << 4) | 2);
     if(ret <= 0)
       return(ret);
     if(i != 1 || argc != 2)
       fprintf(stderr, "cdrskin: WARNING : Program run ended by option textfile_to_v07t=. Other options may have been ignored.\n");
     return(2);

   } else if(strcmp(argpt,"-toc")==0) {
     skin->do_atip= 2;
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: will put out some -atip style lines plus -toc\n"));

   } else if(strncmp(argpt, "-tsize=", 7) == 0) {
     value_pt= argpt + 7;
     goto set_tsize;
   } else if(strncmp(argpt, "tsize=", 6) == 0) {
     value_pt= argpt + 6;
set_tsize:;
     skin->fixed_size= Scanf_io_size(value_pt,0);
     if(skin->fixed_size>Cdrskin_tracksize_maX) {
track_too_large:;
       fprintf(stderr,"cdrskin: FATAL : Track size too large\n");
       return(0);
     }
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: fixed track size : %.f\n",skin->fixed_size));
     if(skin->smallest_tsize<0 || skin->smallest_tsize>skin->fixed_size)
       skin->smallest_tsize= skin->fixed_size;

   } else if(strcmp(argv[i],"--two_channel")==0) {
     skin->track_modemods&= ~BURN_4CH;

   } else if(strcmp(argv[i],"-V")==0 || strcmp(argpt, "-Verbose")==0) {
     /* is handled in Cdrpreskin_setup() */;
   } else if(strcmp(argv[i],"-v")==0 || strcmp(argpt,"-verbose")==0) {
     /* is handled in Cdrpreskin_setup() */;
   } else if(strcmp(argv[i],"-vv")==0 || strcmp(argv[i],"-vvv")==0 ||
             strcmp(argv[i],"-vvvv")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strcmp(argpt,"-version")==0) {
     /* is handled in Cdrpreskin_setup() and should really not get here */;

   } else if(strcmp(argpt,"-waiti")==0) {
     /* is handled in Cdrpreskin_setup() */;

   } else if(strncmp(argv[i],"write_start_address=",20)==0) {
     skin->write_start_address= Scanf_io_size(argv[i]+20,0);
     if(skin->verbosity>=Cdrskin_verbose_cmD)
       ClN(printf("cdrskin: write start address : %.f\n",
                  skin->write_start_address));
     skin->preskin->demands_cdrskin_caps= 1;

   } else if(strcmp(argpt, "-xa") == 0) {
     fprintf(stderr,"cdrskin: NOTE : defaulting option -xa to option -data\n");
     goto option_data;

   } else if(strcmp(argpt, "-xa1") == 0) {
     /* All Subsequent Tracks Option */
     skin->cdxa_conversion= (skin->cdxa_conversion & ~0x7fffffff) | 1;
     skin->track_type= BURN_MODE1;
     skin->track_type_by_default= 0;

   } else if(strcmp(argpt, "-xa2") == 0) {
     fprintf(stderr,
             "cdrskin: NOTE : defaulting option -xa2 to option -data\n");
     goto option_data;

   } else if(strcmp(argv[i], "--xa1-ignore") == 0) {
     skin->cdxa_conversion|= (1 << 31);

   } else if( i==argc-1 ||
             (skin->single_track==0 && strchr(argv[i],'=')==NULL 
               && !(argv[i][0]=='-' && argv[i][1]!=0) ) ||
             (skin->single_track==-1)) {
     if(strlen(argv[i])>=sizeof(skin->source_path)) {
       fprintf(stderr,
            "cdrskin: FATAL : Source address too long. (max: %d, given: %d)\n",
               (int) sizeof(skin->source_path)-1,(int) strlen(argv[i]));
       return(0);
     }
     strcpy(skin->source_path,argv[i]);
     if(strcmp(skin->source_path,"-")==0) {
       if(skin->stdin_source_used) {
         fprintf(stderr,
    "cdrskin: FATAL : \"-\" (stdin) can be used as track source only once.\n");
         return(0);
       }
       skin->stdin_source_used= 1;
     } else if(argv[i][0]=='#' && (argv[i][1]>='0' && argv[i][1]<='9')) {
       if(skin->preskin->allow_fd_source==0) { 
         fprintf(stderr,
              "cdrskin: SORRY : '%s' is a reserved source path with cdrskin\n",
              argv[i]);
         fprintf(stderr,
      "cdrskin: SORRY : which would use an open file descriptor as source.\n");
         fprintf(stderr,
            "cdrskin: SORRY : Its usage is dangerous and disabled for now.\n");
         return(0);
       }
     } else {
       if(stat(skin->source_path,&stbuf)!=-1) {
         if((stbuf.st_mode&S_IFMT)==S_IFREG)
           ;
         else if((stbuf.st_mode&S_IFMT)==S_IFDIR) {
           fprintf(stderr,
                   "cdrskin: FATAL : Source address is a directory: '%s'\n",
                   skin->source_path);
           return(0);
         }
       }
     }

     if(skin->track_counter>=Cdrskin_track_maX) {
       fprintf(stderr,"cdrskin: FATAL : Too many tracks given. (max %d)\n",
               Cdrskin_track_maX);
       return(0);
     }
     ret= Cdrtrack_new(&(skin->tracklist[skin->track_counter]),skin,
                       skin->track_counter,
                       (strcmp(skin->source_path,"-")==0)<<1);
     if(ret<=0) {
       fprintf(stderr,
               "cdrskin: FATAL : Creation of track control object failed.\n");
       return(ret);
     }
     if(skin->next_isrc[0])
       memcpy(skin->tracklist[skin->track_counter]->isrc, skin->next_isrc, 13);
     skin->tracklist[skin->track_counter]->index_string= skin->index_string;
     skin->tracklist[skin->track_counter]->sao_pregap= skin->sao_pregap;
     skin->tracklist[skin->track_counter]->sao_postgap= skin->sao_postgap;
     skin->index_string= NULL;
     skin->sao_postgap= skin->sao_pregap= -1;
     skin->track_counter++;
     skin->use_data_image_size= 0;
     if(skin->verbosity>=Cdrskin_verbose_cmD) {
       if(strcmp(skin->source_path,"-")==0)
         printf("cdrskin: track %d data source : '-'  (i.e. standard input)\n",
                skin->track_counter);
       else
         printf("cdrskin: track %d data source : '%s'\n",
                skin->track_counter,skin->source_path);
     }
     /* reset track options */
     if(skin->set_by_padsize)
       skin->padding= 0; /* cdrecord-ProDVD-2.01b31 resets to 30k
                            the man page says padsize= is reset to 0
                            Joerg Schilling will change in 2.01.01 to 0 */
     skin->fixed_size= 0;
     skin->next_isrc[0]= 0;
   } else {
ignore_unknown:;
     if(skin->preskin->fallback_program[0])
       fprintf(stderr,"cdrskin: NOTE : Unknown option : '%s'\n",argv[i]);
     else
       fprintf(stderr,"cdrskin: NOTE : ignoring unknown option : '%s'\n",
               argv[i]);
     skin->preskin->demands_cdrecord_caps= 1;
   }
 }

 if(flag&1) /* no finalizing yet */
   return(1);

 if(skin->preskin->fallback_program[0] &&
    skin->preskin->demands_cdrecord_caps>0 &&
    skin->preskin->demands_cdrskin_caps<=0) {
   fprintf(stderr,"cdrskin: NOTE : Unsupported options found.\n");	
   fprintf(stderr,
           "cdrskin: NOTE : Will delegate job to fallback program '%s'.\n",
           skin->preskin->fallback_program);
   return(0);
 }

#ifndef Cdrskin_extra_leaN
 if(skin->verbosity>=Cdrskin_verbose_cmD) {
   if(skin->preskin->abort_handler==1)
     printf("cdrskin: installed abort handler.\n");
   else if(skin->preskin->abort_handler==2) 
     printf("cdrskin: will try to ignore any signals.\n");
   else if(skin->preskin->abort_handler==3)
     printf("cdrskin: installed hard abort handler.\n");
   else if(skin->preskin->abort_handler==4)
     printf("cdrskin: installed soft abort handler.\n");
   else if(skin->preskin->abort_handler==-1)
     printf("cdrskin: will install abort handler in eventual burn loop.\n");
 }
#endif /* ! Cdrskin_extra_leaN */

 if(strlen(skin->preskin->raw_device_adr)>0 ||
    strlen(skin->preskin->device_adr)>0) {
   if(strlen(skin->preskin->device_adr)>0)
     cpt= skin->preskin->device_adr;
   else
     cpt= skin->preskin->raw_device_adr;
   if(strcmp(cpt,"ATA")!=0 && strcmp(cpt,"ATAPI")!=0 && strcmp(cpt,"SCSI")!=0){
     ret= Cdrskin_dev_to_driveno(skin,cpt,&(skin->driveno),0);
     if(ret<=0)
       return(ret);
     if(skin->verbosity>=Cdrskin_verbose_cmD) {
       ret= burn_drive_get_adr(&(skin->drives[skin->driveno]), adr);
       if(ret<=0)
         adr[0]= 0;
       printf("cdrskin: active drive number : %d  '%s'\n",
              skin->driveno,adr);
     }
   }
 }
 if(grab_and_wait_value>0) {
   Cdrskin_grab_drive(skin,16);
   for(k= 0; k<grab_and_wait_value; k++) {
     fprintf(stderr, "%scdrskin: holding drive grabbed since %d seconds%s",
             skin->pacifier_with_newline ? "" : "\r", k,
             skin->pacifier_with_newline ? "\n" : "                 ");
     usleep(1000000);
   }
   fprintf(stderr,
        "%scdrskin: held drive grabbed for %d seconds                      \n",
        skin->pacifier_with_newline ? "" : "\r", k);
   Cdrskin_release_drive(skin,0);
 }
     
 if(skin->track_counter>0) {
   skin->do_burn= 1;

#ifndef Cdrskin_no_cdrfifO
   if(!skin->do_direct_write) {
     ret= Cdrskin_attach_fifo(skin,0);
     if(ret<=0)
       return(ret);
   }
#endif /* ! Cdrskin_no_cdrfifO */

 }
 return(1);
}


/** Initialize libburn, create a CdrskiN program run control object,
    set eventual device whitelist, and obtain the list of available drives.
    @param o Returns the CdrskiN object created
    @param lib_initialized Returns whether libburn was initialized here
    @param exit_value Returns after error the proposal for an exit value
    @param flag bit0= do not scan for devices
    @return <=0 error, 1 success
*/
int Cdrskin_create(struct CdrskiN **o, struct CdrpreskiN **preskin,
                   int *exit_value, int flag)
{
 int ret, stdio_drive= 0, mem;
 struct CdrskiN *skin;
 char reason[4096];

 *o= NULL;
 *exit_value= 0;

 if(strlen((*preskin)->device_adr)>0) {       /* disable scan for all others */
   ClN(printf(
       "cdrskin: NOTE : greying out all drives besides given dev='%s'\n",
       (*preskin)->device_adr));
   burn_drive_add_whitelist((*preskin)->device_adr);
   if(strncmp((*preskin)->device_adr, "stdio:", 6)==0) {
     ret= Cdrpreskin__allows_emulated_drives((*preskin)->device_adr+6,reason,0);
     if((*preskin)->allow_emulated_drives && ret>0) {
       stdio_drive= 1;
       (*preskin)->demands_cdrskin_caps= 1;
     } else if((*preskin)->allow_emulated_drives) {
       fprintf(stderr,"cdrskin: SORRY : dev=stdio:... rejected despite --allow_emulated_drives\n");
       fprintf(stderr,"cdrskin: SORRY : Reason: %s.\n", reason);
     } else {
       fprintf(stderr,"cdrskin: SORRY : dev=stdio:... works only with option --allow_emulated_drives\n");
       if(ret<=0) {
         fprintf(stderr,"cdrskin: SORRY : but: %s.\n", reason);
         fprintf(stderr,
                "cdrskin: SORRY : So this option would not help anyway.\n");
       }
     }
     if(!stdio_drive) {
       Cdrpreskin_consider_normal_user(0);
       {*exit_value= 2; goto ex;}
     }
   }
 }

 ret= Cdrskin_new(&skin,*preskin,1);
 if(ret<=0) {
   fprintf(stderr,"cdrskin: FATAL : Creation of control object failed\n");
   {*exit_value= 2; goto ex;}
 }
 *preskin= NULL; /* the preskin object now is under management of skin */
 *o= skin;
 if(skin->preskin->abort_handler==1 || skin->preskin->abort_handler==3 || 
    skin->preskin->abort_handler==4)
   Cleanup_set_handlers(Cleanup_handler_handlE, Cleanup_handler_funC,
                        Cleanup_handler_flaG);
 else if(skin->preskin->abort_handler==2)
   Cleanup_set_handlers(Cleanup_handler_handlE, Cleanup_handler_funC,
                        2 | 8);

 if(!(flag & 1))
   printf("cdrskin: scanning for devices ...\n");
 fflush(stdout);

 if(skin->preskin->verbosity<Cdrskin_verbose_debuG)
   burn_msgs_set_severities("NEVER", "NOTE", "cdrskin: ");

 /* In cdrskin there is not much sense in queueing library messages.
    It is done here only for testing it from time to time */
 Cdrpreskin_queue_msgs(skin->preskin,1);

 if(stdio_drive) {
   mem= skin->drive_is_busy;
   skin->drive_is_busy= 1;
   ret= burn_drive_scan_and_grab(&(skin->drives),skin->preskin->device_adr,0);
   skin->drive_is_busy= mem;
   if(Cdrskin__is_aborting(0)) {
     fprintf(stderr,"cdrskin: ABORT : Startup aborted\n");
     Cdrskin_abort(skin, 0); /* Never comes back */
   }
   if(ret <= 0) {
     fprintf(stderr,"cdrskin: FATAL : Failed to grab emulated stdio-drive\n");
     {*exit_value= 2; goto ex;}
   }
   skin->n_drives= 1;
   skin->driveno= 0;
   burn_drive_release(skin->drives[0].drive, 0);
 } else if(flag & 1){
   skin->n_drives= 0;
   skin->driveno= 0;
 } else {
   while (!burn_drive_scan(&(skin->drives), &(skin->n_drives))) {
     usleep(20000);
     /* >>> ??? set a timeout ? */
   }
   if(skin->n_drives <= 0)
     skin->driveno= -1;
 }

 burn_msgs_set_severities(skin->preskin->queue_severity,
                          skin->preskin->print_severity, "cdrskin: ");

 /* This prints the eventual queued messages */
 Cdrpreskin_queue_msgs(skin->preskin,0);

 if(!(flag & 1))
   printf("cdrskin: ... scanning for devices done\n");
 fflush(stdout);
ex:;
 return((*exit_value)==0);
}


/** Perform the activities which were ordered by setup
    @param skin Knows what to do
    @param exit_value Returns the proposal for an exit value
    @param flag Unused yet
    @return <=0 error, 1 success
*/
int Cdrskin_run(struct CdrskiN *skin, int *exit_value, int flag)
{
 int ret;

 *exit_value= 0;

 if(skin->preskin->allow_setuid==0 && getuid()!=geteuid()) {
   fprintf(stderr,"\n");
   fprintf(stderr,"cdrskin: WARNING : THIS PROGRAM WAS TREATED WITH chmod u+s WHICH IS INSECURE !\n");
   fprintf(stderr,
    "cdrskin: HINT : Consider to allow rw-access to the writer device and\n");
   fprintf(stderr,
    "cdrskin: HINT : to run cdrskin under your normal user identity.\n");
   fprintf(stderr,
    "cdrskin: HINT : Option  --allow_setuid  disables this safety check.\n");
   fprintf(stderr,"\n");
 }

 if(skin->do_devices) {
   if(skin->n_drives<=0 && skin->preskin->scan_demands_drive)
     {*exit_value= 4; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_scanbus(skin, 1 | (2 * (skin->do_devices == 2)));
   if(ret<=0) {
     fprintf(stderr,"cdrskin: FATAL : --devices failed.\n");
     {*exit_value= 4; goto ex;}
   }
 }

 if(skin->do_scanbus) {
   if(skin->n_drives<=0 && skin->preskin->scan_demands_drive)
     {*exit_value= 5; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_scanbus(skin,0);
   if(ret<=0)
     fprintf(stderr,"cdrskin: FATAL : -scanbus failed.\n");
   {*exit_value= 5*(ret<=0); goto ex;}
 }
 if(skin->do_load > 0) {
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_grab_drive(skin,8);
   if(ret>0) {
     if(skin->do_load==2 && !skin->do_eject) {
       printf(
   "cdrskin: NOTE : Option -lock orders program to exit with locked tray.\n");
       printf(
 "cdrskin: HINT : Run cdrskin with option -eject to unlock the drive tray.\n");
     } else if(!skin->do_eject)
       printf(
  "cdrskin: NOTE : option -load orders program to exit after loading tray.\n");
     Cdrskin_release_drive(skin,(skin->do_load==2)<<1);
   }
   {*exit_value= 14*(ret<=0); goto ex;}
 }
 if(skin->do_checkdrive) {
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_checkdrive(skin,"",(skin->do_checkdrive==2)<<1);
   {*exit_value= 6*(ret<=0); goto ex;}
 }
 if(skin->do_msinfo) {
   if(skin->n_drives<=0)
     {*exit_value= 12; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_msinfo(skin,0);
   if(ret<=0)
     {*exit_value= 12; goto ex;}
 }
 if(skin->do_atip) {
   if(skin->n_drives<=0)
     {*exit_value= 7; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_atip(skin, skin->do_atip == 4 ? 4 :
                                (skin->do_atip>1) | (2 * (skin->do_atip > 2)));
   if(ret<=0)
     {*exit_value= 7; goto ex;}
 }
 if(skin->do_cdtext_to_textfile) {
   ret= Cdrskin_cdtext_to_file(skin, skin->cdtext_to_textfile_path, 15);
   if(ret<=0)
     {*exit_value= 18; goto ex;}
 }
 if(skin->do_cdtext_to_vt07) {
   ret= Cdrskin_cdtext_to_file(skin, skin->cdtext_to_vt07_path, 1);
   if(ret<=0)
     {*exit_value= 19; goto ex;}
 }
 if(skin->do_extract_audio) {
   ret= Cdrskin_extract_audio_to_dir(skin, skin->extract_audio_dir,
                                     skin->extract_basename,
                                     skin->extract_audio_tracks,
                                     skin->extract_flags & 8);
   if(ret<=0)
     {*exit_value= 19; goto ex;}
 }
 if(skin->do_list_speeds) {
   if(skin->n_drives<=0)
     {*exit_value= 17; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_list_speeds(skin, 0);
   if(ret<=0)
     {*exit_value= 17; goto ex;}
 }
 if(skin->do_list_formats) {
   if(skin->n_drives<=0)
     {*exit_value= 16; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_list_formats(skin, 0);
   if(ret<=0)
     {*exit_value= 16; goto ex;}
 }
 if(skin->do_blank) {
   if(skin->n_drives<=0)
     {*exit_value= 8; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_blank(skin,0);
   if(ret<=0)
     {*exit_value= 8; goto ex;}
 }
 if(skin->do_direct_write) {
   skin->do_burn= 0;
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_direct_write(skin,0);
   if(ret<=0)
     {*exit_value= 13; goto ex;}
 }
 if(skin->do_burn || skin->tell_media_space) {
   if(skin->n_drives<=0)
     {*exit_value= 10; goto no_drive;}
   if(Cdrskin__is_aborting(0))
     goto ex;
   ret= Cdrskin_burn(skin,0);
   if(ret<=0)
     {*exit_value= 10; goto ex;}
 }

#ifdef Libburn_develop_quality_scaN

 if(skin->do_qcheck) {
   ret= Cdrskin_qcheck(skin, 0);
   if(ret<=0)
     {*exit_value= 15; goto ex;}
 }

#endif /* Libburn_develop_quality_scaN */

ex:;
 if(Cdrskin__is_aborting(0))
   Cdrskin_abort(skin, 0); /* Never comes back */
 return((*exit_value)==0);
no_drive:;
 fprintf(stderr,"cdrskin: FATAL : This run would need an accessible drive\n");
 goto ex;
}


int main(int argc, char **argv)
{
 int ret,exit_value= 0,lib_initialized= 0,i,result_fd= -1, do_not_scan;
 struct CdrpreskiN *preskin= NULL, *h_preskin= NULL;
 struct CdrskiN *skin= NULL;
 char *lean_id= "";
#ifdef Cdrskin_extra_leaN
 lean_id= ".lean";
#endif

 /* For -msinfo: Redirect normal stdout to stderr */
 for(i=1; i<argc; i++)
   if(strcmp(argv[i],"-msinfo")==0 ||
      strcmp(argv[i],"--tell_media_space")==0 ||
      strcmp(argv[i],"dev=stdio:/dev/fd/1")==0 ||
      strcmp(argv[i],"-dev=stdio:/dev/fd/1")==0
)
 break;
 if(i<argc) {
   result_fd= dup(1);
   close(1);
   dup2(2,1);
 }
 
 printf("cdrskin %s%s : limited cdrecord compatibility wrapper for libburn\n",
        Cdrskin_prog_versioN,lean_id);
 fflush(stdout);

 ret= Cdrpreskin_new(&preskin,0);
 if(ret<=0) {
   fprintf(stderr,"cdrskin: FATAL : Creation of control object failed\n");
   {exit_value= 2; goto ex;}
 }
 Cdrpreskin_set_result_fd(preskin,result_fd,0);

 /* <<< A60925: i would prefer to do this later, after it is clear that no
       -version or -help cause idle end. But address conversion and its debug
       messaging need libburn running */
 ret= Cdrpreskin_initialize_lib(preskin,0);
 if(ret<=0) {
   fprintf(stderr,"cdrskin: FATAL : Initialization of burn library failed\n");
   {exit_value= 2; goto ex;}
 }
 lib_initialized= 1;

 ret= Cdrpreskin_setup(preskin,argc,argv,0);
 if(ret<=0)
   {exit_value= 11; goto ex;}
 if(ret==2)
   {exit_value= 0; goto ex;}
 do_not_scan= preskin->do_not_scan; /* preskin will vanish in Cdrskin_create */
 ret= Cdrskin_create(&skin,&preskin,&exit_value, preskin->do_not_scan);
 if(ret<=0)
   {exit_value= 2; goto ex;}
 if(skin->n_drives<=0 && !do_not_scan) {
   fprintf(stderr,"cdrskin: NOTE : No usable drive detected.\n");
   if(getuid()!=0) {
     fprintf(stderr,
      "cdrskin: HINT : Run this program as superuser with option --devices\n");
     fprintf(stderr,
      "cdrskin: HINT : Allow rw-access to the dev='...' file of the burner.\n");
     fprintf(stderr,
      "cdrskin: HINT : Busy drives are invisible. (Busy = open O_EXCL)\n");
   }
 }

 ret= Cdrskin_setup(skin,argc,argv,0);
 if(ret<=0)
   {exit_value= 3; goto ex;}
 if(skin->verbosity>=Cdrskin_verbose_cmD)
   ClN(printf("cdrskin: called as :  %s\n",argv[0]));

 if(skin->verbosity>=Cdrskin_verbose_debuG) {
#ifdef Cdrskin_new_api_tesT
   ClN(fprintf(stderr,"cdrskin_debug: Compiled with option -experimental\n"));
#endif
 }

 if(!Cdrskin__is_aborting(0))
   Cdrskin_run(skin,&exit_value,0);

ex:;
 if(Cdrskin__is_aborting(0))
   Cdrskin_abort(skin, 0); /* Never comes back */

 if(preskin!=NULL)
   h_preskin= preskin;
 else if(skin!=NULL)
   h_preskin= skin->preskin;
 if(h_preskin!=NULL) {
   if(skin!=NULL)
     if(skin->verbosity>=Cdrskin_verbose_debuG)
       ClN(fprintf(stderr,
       "cdrskin_debug: demands_cdrecord_caps= %d , demands_cdrskin_caps= %d\n",
       h_preskin->demands_cdrecord_caps, h_preskin->demands_cdrskin_caps));

   if(exit_value && h_preskin->demands_cdrecord_caps>0 &&
      h_preskin->demands_cdrskin_caps<=0) {              /* prepare fallback */
     /* detach preskin from managers which would destroy it */
     preskin= NULL;
     if(skin!=NULL)
       skin->preskin= NULL;
   } else
     h_preskin= NULL;                                    /* prevent fallback */
 }
 if(skin!=NULL) {
   Cleanup_set_handlers(NULL,NULL,1);
   if(skin->preskin!=NULL)
     Cdrskin_eject(skin,0);
   Cdrskin_destroy(&skin,0);
 }
 Cdrpreskin_destroy(&preskin,0);
 if(lib_initialized)
   burn_finish();
 if(h_preskin!=NULL)
   Cdrpreskin_fallback(h_preskin,argc,argv,0); /* never come back */
 exit(exit_value);
}
