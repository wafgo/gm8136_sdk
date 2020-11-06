/*
 cdrfifo.c , Copyright 2006 Thomas Schmitt <scdbackup@gmx.net>

 A fd-to-fd or fd-to-memory fifo to be used within cdrskin or independently.
 By chaining of fifo objects, several fifos can be run simultaneously
 in fd-to-fd mode. Modes are controlled by parameter flag of 
 Cdrfifo_try_to_work().

 Provided under GPL license within cdrskin and under BSD license elsewise.
*/

/*
 Compile as standalone tool :
   cc -g -o cdrfifo -DCdrfifo_standalonE cdrfifo.c
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>

#ifndef Cdrfifo_standalonE
/* <<< until release of 0.7.4 : for Libburn_has_open_trac_srC */
#include "../libburn/libburn.h"
#endif

#include "cdrfifo.h"


/* Macro for creation of arrays of objects (or single objects) */
#define TSOB_FELD(typ,anz) (typ *) calloc(anz, sizeof(typ));


#define Cdrfifo_buffer_chunK 2048

/** Number of follow-up fd pairs */
#define Cdrfifo_ffd_maX 100


/* 1= enable , 0= disable status messages to stderr
   2= report each 
*/
static int Cdrfifo_debuG= 0;


struct CdrfifO {
 int chunk_size;

 int source_fd;
 double in_counter;

 double fd_in_counter;
 double fd_in_limit;

 char *buffer;
 int buffer_size;
 int buffer_is_full;
 int write_idx;
 int read_idx;
  
 int dest_fd;
 double out_counter;

 struct timeval start_time;
 double speed_limit;

 /* statistics */
 double interval_counter;
 struct timeval interval_start_time;
 double interval_start_counter;
 int total_min_fill;
 int interval_min_fill;

 double put_counter;
 double get_counter;
 double empty_counter;
 double full_counter;

 /* eventual ISO-9660 image size obtained from first 64k of input */
 double iso_fs_size; 
 char *iso_fs_descr; /* eventually block 16 to 31 of input */

 /* (sequential) fd chaining */
 /* fds: 0=source, 1=dest  */
 int follow_up_fds[Cdrfifo_ffd_maX][2];

 /* index of first byte in buffer which does not belong to predecessor fd */
 int follow_up_eop[Cdrfifo_ffd_maX];
 /* if follow_up_eop[i]==buffer_size : read_idx was 0 when this was set */
 int follow_up_was_full_buffer[Cdrfifo_ffd_maX];

 /* index of first byte in buffer which belongs to [this] fd pair */
 int follow_up_sod[Cdrfifo_ffd_maX];

 /* values for fd_in_limit */
 double follow_up_in_limits[Cdrfifo_ffd_maX];

 /* number of defined follow-ups */
 int follow_up_fd_counter;

 /* index of currently active (i.e. reading) follow-up */
 int follow_up_fd_idx;


 /* (simultaneous) peer chaining */
 struct CdrfifO *next;
 struct CdrfifO *prev;

 /* rank in peer chain */
 int chain_idx;
};


/** Create a fifo object.
    @param ff Returns the address of the new object.
    @param source_fd Filedescriptor opened to a readable data stream.
    @param dest_fd Filedescriptor opened to a writable data stream.
                   To work with libburn, it needs to be attached to a
                   struct burn_source object.
    @param chunk_size Size of buffer block for a single transaction (0=default)
    @param buffer_size Size of fifo buffer
    @param flag bit0= Debugging verbosity
    @return 1 on success, <=0 on failure
*/
int Cdrfifo_new(struct CdrfifO **ff, int source_fd, int dest_fd,
                int chunk_size, int buffer_size, int flag)
{
 struct CdrfifO *o;
 struct timezone tz;
 int i;

 (*ff)= o= TSOB_FELD(struct CdrfifO,1);
 if(o==NULL)
   return(-1);
 if(chunk_size<=0)
   chunk_size= Cdrfifo_buffer_chunK;
 o->chunk_size= chunk_size;
 if(buffer_size%chunk_size)
   buffer_size+= chunk_size-(buffer_size%chunk_size);
 o->source_fd= source_fd;
 o->in_counter= 0.0;
 o->fd_in_counter= 0;
 o->fd_in_limit= -1.0;
 o->buffer= NULL;
 o->buffer_is_full= 0;
 o->buffer_size= buffer_size;
 o->write_idx= 0;
 o->read_idx= 0;
 o->dest_fd= dest_fd;
 o->out_counter= 0.0;
 memset(&(o->start_time),0,sizeof(o->start_time));
 gettimeofday(&(o->start_time),&tz);
 o->speed_limit= 0.0;
 o->interval_counter= 0.0;
 memset(&(o->interval_start_time),0,sizeof(o->interval_start_time));
 gettimeofday(&(o->interval_start_time),&tz);
 o->interval_start_counter= 0.0;
 o->total_min_fill= buffer_size;
 o->interval_min_fill= buffer_size;
 o->put_counter= 0.0;
 o->get_counter= 0.0;
 o->empty_counter= 0.0;
 o->full_counter= 0.0;
 o->iso_fs_size= -1.0;
 o->iso_fs_descr= NULL;
 for(i= 0; i<Cdrfifo_ffd_maX; i++) {
   o->follow_up_fds[i][0]= o->follow_up_fds[i][1]= -1;
   o->follow_up_eop[i]= o->follow_up_sod[i]= -1;
   o->follow_up_was_full_buffer[i]= 0;
   o->follow_up_in_limits[i]= -1.0;
 }
 o->follow_up_fd_counter= 0;
 o->follow_up_fd_idx= -1;
 o->next= o->prev= NULL;
 o->chain_idx= 0;

#ifdef Libburn_has_open_trac_srC
 o->buffer= burn_os_alloc_buffer((size_t) buffer_size, 0);
#else
 o->buffer= TSOB_FELD(char,buffer_size);
#endif /* ! Libburn_has_open_trac_srC */

 if(o->buffer==NULL)
   goto failed;
 return(1);
failed:;
 Cdrfifo_destroy(ff,0);
 return(-1);
}


/** Close any output fds */
int Cdrfifo_close(struct CdrfifO *o, int flag)
{
 int i;

 if(o->dest_fd!=-1)
   close(o->dest_fd);
 o->dest_fd= -1;
 for(i=0; i<o->follow_up_fd_counter; i++) {
   if(o->follow_up_fds[i][1]!=-1)
     close(o->follow_up_fds[i][1]);
   o->follow_up_fds[i][1]= -1;
 }
 return(1);
}


/** Release from memory a fifo object previously created by Cdrfifo_new().
    @param ff The victim (gets returned as NULL, call can stand *ff==NULL))
    @param flag Bitfield for control purposes: 
                bit0= do not close destination fd
*/
int Cdrfifo_destroy(struct CdrfifO **ff, int flag)
/* flag
 bit0= do not close destination fd
*/
{
 struct CdrfifO *o;

 o= *ff;
 if(o==NULL)
   return(0);
 if(o->next!=NULL)
   o->next->prev= o->prev;
 if(o->prev!=NULL)
   o->prev->next= o->next;
 if(!(flag&1))
   Cdrfifo_close(o,0);

 /* eventual closing of source fds is the job of the calling application */

 if(o->iso_fs_descr!=NULL)
   free((char *) o->iso_fs_descr);

 if(o->buffer!=NULL)
#ifdef Libburn_has_open_trac_srC
   burn_os_free_buffer(o->buffer, o->buffer_size, 0);
#else
   free((char *) o->buffer);
#endif /* Libburn_has_open_trac_srC */

 free((char *) o);
 (*ff)= NULL;
 return(1);
}


int Cdrfifo_get_sizes(struct CdrfifO *o, int *chunk_size, int *buffer_size,
                      int flag)
{
 *chunk_size= o->chunk_size;
 *buffer_size= o->buffer_size;
 return(1);
}

/** Set a speed limit for buffer output.
    @param o The fifo object
    @param bytes_per_second >0 catch up slowdowns over the whole run time
                            <0 catch up slowdowns only over one interval
                            =0 disable speed limit
*/
int Cdrfifo_set_speed_limit(struct CdrfifO *o, double bytes_per_second,
                            int flag)
{
 o->speed_limit= bytes_per_second;
 return(1);
}


/** Set a fixed size for input in order to cut off any unwanted tail
    @param o The fifo object
    @param idx index for fds attached via Cdrfifo_attach_follow_up_fds(), 
               first attached is 0,  <0 directs limit to active fd limit
               (i.e. first track is -1, second track is 0, third is 1, ...)
*/
int Cdrfifo_set_fd_in_limit(struct CdrfifO *o, double fd_in_limit, int idx,
                            int flag)
{
 if(idx<0) {
   o->fd_in_limit= fd_in_limit;
   return(1);
 }
 if(idx >= o->follow_up_fd_counter)
   return(0);
 o->follow_up_in_limits[idx]= fd_in_limit;
 return(1);
}


int Cdrfifo_set_fds(struct CdrfifO *o, int source_fd, int dest_fd, int flag)
{
 o->source_fd= source_fd;
 o->dest_fd= dest_fd;
 return(1);
}


int Cdrfifo_get_fds(struct CdrfifO *o, int *source_fd, int *dest_fd, int flag)
{
 *source_fd= o->source_fd;
 *dest_fd= o->dest_fd;
 return(1);
}


/** Attach a further pair of input and output fd which will use the same
    fifo buffer when its predecessors are exhausted. Reading will start as
    soon as reading of the predecessor encounters EOF. Writing will start
    as soon as all pending predecessor data are written.
    @return index number of new item + 1, <=0 indicates error
*/
int Cdrfifo_attach_follow_up_fds(struct CdrfifO *o, int source_fd, int dest_fd,
                                 int flag)
{
 if(o->follow_up_fd_counter>=Cdrfifo_ffd_maX)
   return(0);
  o->follow_up_fds[o->follow_up_fd_counter][0]= source_fd;
  o->follow_up_fds[o->follow_up_fd_counter][1]= dest_fd;
  o->follow_up_fd_counter++;
  return(o->follow_up_fd_counter);
}


/** Attach a further fifo which shall be processed simultaneously with this
    one by Cdrfifo_try_to_work() in fd-to-fd mode.
*/
int Cdrfifo_attach_peer(struct CdrfifO *o, struct CdrfifO *next, int flag)
{
 int idx;
 struct CdrfifO *s;

 for(s= o;s->prev!=NULL;s= s->prev); /* determine start of o-chain */
 for(;o->next!=NULL;o= o->next);          /* determine end of o-chain */
 for(;next->prev!=NULL;next= next->prev); /* determine start of next-chain */
 next->prev= o;
 o->next= next;
 for(idx= 0;s!=NULL;s= s->next)
   s->chain_idx= idx++;
 return(1);
}


static int Cdrfifo_tell_buffer_space(struct CdrfifO *o, int flag)
{
 if(o->buffer_is_full)
   return(0);
 if(o->write_idx>=o->read_idx)
   return((o->buffer_size - o->write_idx) + o->read_idx);
 return(o->read_idx - o->write_idx);
}


/** Obtain buffer state.
    @param o The buffer object
    @param fill Returns the number of pending payload bytes in the buffer
    @param space Returns the number of unused buffer bytes
    @param flag Unused yet
    @return -1=error , 0=inactive , 1=reading and writing ,
            2=reading ended (but still writing)
*/
int Cdrfifo_get_buffer_state(struct CdrfifO *o,int *fill,int *space,int flag)
/* return : 
 -1=error
  0=inactive  
  1=reading and writing
  2=reading ended, still writing
*/
{
 *space= Cdrfifo_tell_buffer_space(o,0);
 *fill= o->buffer_size-(*space);
 if(o->dest_fd==-1)
   return(0);
 if(o->source_fd<0)
   return(2);
 return(1);
}


int Cdrfifo_get_counters(struct CdrfifO *o,
                         double *in_counter, double *out_counter, int flag)
{
 *in_counter= o->in_counter;
 *out_counter= o->out_counter;
 return(1);
}


/** reads min_fill and begins measurement interval for next min_fill */
int Cdrfifo_next_interval(struct CdrfifO *o, int *min_fill, int flag)
{
 struct timezone tz;
 
 o->interval_counter++;
 gettimeofday(&(o->interval_start_time),&tz);
 o->interval_start_counter= o->out_counter;
 *min_fill= o->interval_min_fill;
 o->interval_min_fill= o->buffer_size - Cdrfifo_tell_buffer_space(o,0);
 return(1);
}


int Cdrfifo_get_min_fill(struct CdrfifO *o, int *total_min_fill,
                         int *interval_min_fill, int flag)
{
 *total_min_fill= o->total_min_fill;
 *interval_min_fill= o->interval_min_fill;
 return(1);
}


int Cdrfifo_get_iso_fs_size(struct CdrfifO *o, double *size_in_bytes, int flag)
{
 *size_in_bytes= o->iso_fs_size;
 return(o->iso_fs_size>=2048);
}


int Cdrfifo_adopt_iso_fs_descr(struct CdrfifO *o, char **pt, int flag)
{
 *pt= o->iso_fs_descr;
 o->iso_fs_descr= NULL;
 return(*pt!=NULL);
}


/** Get counters which are mentioned by cdrecord at the end of burning.
    It still has to be examined wether they mean what i believe they do.
*/
int Cdrfifo_get_cdr_counters(struct CdrfifO *o,
                             double *put_counter, double *get_counter,
                             double *empty_counter, double *full_counter,
                             int flag)
{
 *put_counter= o->put_counter;
 *get_counter= o->get_counter;
 *empty_counter= o->empty_counter;
 *full_counter= o->full_counter;
 return(1);
}


/** Adjust a given buffer fill value so it will not cross an eop boundary.
    @param o The fifo to exploit.
    @param buffer_fill The byte count to adjust.
    @param eop_idx If eop boundary exactly hit: index of follow-up fd pair
    @param flag Unused yet.
    @return 0= nothing changed , 1= buffer_fill adjusted 
*/
int Cdrfifo_eop_adjust(struct CdrfifO *o,int *buffer_fill, int *eop_idx,
                       int flag)
{
 int i,eop_is_near= 0,valid_fill;

 *eop_idx= -1;
 valid_fill= *buffer_fill;
 for(i=0; i<=o->follow_up_fd_idx; i++) {
   if(o->follow_up_eop[i]>=0 && o->follow_up_eop[i]>=o->read_idx) {
     eop_is_near= 1;
     if(o->follow_up_eop[i]<o->buffer_size || o->read_idx>0) {
       valid_fill= o->follow_up_eop[i]-o->read_idx;
       o->follow_up_was_full_buffer[i]= 0;
     } else {
       /*
        If an input fd change hit exactly the buffer end then follow_up_eop
        points to buffer_size and not to 0. So it is time to switch output
        pipes unless this is immediately after follow_up_eop was set and
        read_idx was 0 (... if this is possible at all while write_idx is 0).
        follow_up_was_full_buffer was set in this case and gets invalid as
        soon as a non-0 read_idx is detected (see above).
       */
       if(o->follow_up_was_full_buffer[i])
         valid_fill= o->buffer_size;
       else
         valid_fill= 0; /* the current pipe is completely served */
     }
     if(valid_fill==0)
       *eop_idx= i;
     else if(valid_fill<o->chunk_size)
       eop_is_near= 2; /* for debugging. to carry a break point */
 break;
   }
 }
 if(*buffer_fill>valid_fill)
   *buffer_fill= valid_fill;
 return(!!eop_is_near);
}


/* Perform pre-select activities of Cdrfifo_try_to_work() */
static int Cdrfifo_setup_try(struct CdrfifO *o, struct timeval start_tv,
                             double start_out_counter, int *still_to_wait,
                             int *speed_limiter, int *ready_to_write,
                             fd_set *rds, fd_set *wts, int *max_fd, int flag)
/* flag:
 bit0= enable debug pacifier (same with Cdrfifo_debuG)
 bit1= do not write, just fill buffer
 bit2= fd-to-memory mode (else fd-to-fd mode):
       rather than writing a chunk return it and its size in reply_*
 bit3= with bit2: do not check destination fd for readiness
*/
{
 int buffer_space,buffer_fill,eop_reached= -1,eop_is_near= 0,was_closed;
 int fd_buffer_fill, eop_reached_counter= 0;
 struct timeval current_tv;
 struct timezone tz;
 double diff_time,diff_counter,limit,min_wait_time;

setup_try:;
 buffer_space= Cdrfifo_tell_buffer_space(o,0);
 fd_buffer_fill= buffer_fill= o->buffer_size - buffer_space;

#ifdef NIX
 fprintf(stderr,"cdrfifo_debug:  o->write_idx=%d  o->read_idx=%d  o->source_fd=%d\n",o->write_idx,o->read_idx,o->source_fd);
 if(buffer_fill>10)
   sleep(1);
#endif

 if(o->follow_up_fd_idx>=0)
   eop_is_near= Cdrfifo_eop_adjust(o,&fd_buffer_fill,&eop_reached,0);

 if(fd_buffer_fill<=0 && (o->source_fd==-1 || eop_reached>=0) ) { 
   was_closed= 0;
   if(o->dest_fd!=-1 && !(flag&4))
     close(o->dest_fd);
   if(o->dest_fd<0)
     was_closed= 1;
   else
     o->dest_fd= -1;

   if(eop_reached>=0) { /* switch to next output fd */
      o->dest_fd= o->follow_up_fds[eop_reached][1];
       if(Cdrfifo_debuG)
         fprintf(stderr,"\ncdrfifo %d: new fifo destination fd : %d\n",
                        o->chain_idx,o->dest_fd);
      o->read_idx= o->follow_up_sod[eop_reached];
      o->follow_up_eop[eop_reached]= -1;
      eop_is_near= 0;
      eop_reached= -1;
      eop_reached_counter= 0;
      goto setup_try;
   } else {
     /* work is really done */
     if((!was_closed) && ((flag&1)||Cdrfifo_debuG))
       fprintf(stderr,
            "\ncdrfifo %d:  w=%d r=%d | b=%d s=%d | i=%.f o=%.f (done)\n",
            o->chain_idx,o->write_idx,o->read_idx,buffer_fill,buffer_space,
            o->in_counter,o->out_counter);
     return(2);
   }
 } else if(eop_reached>=0)
   eop_reached_counter++;
 if(o->interval_counter>0) {
   if(o->total_min_fill>buffer_fill && o->source_fd>=0)
     o->total_min_fill= buffer_fill;
   if(o->interval_min_fill>buffer_fill)
     o->interval_min_fill= buffer_fill;
 }
 *speed_limiter= 0;
 if(o->speed_limit!=0) {
   gettimeofday(&current_tv,&tz);
   if(o->speed_limit>0) {
     diff_time= ((double) current_tv.tv_sec)-((double) o->start_time.tv_sec)+ 
         (((double) current_tv.tv_usec)-((double) o->start_time.tv_usec))*1e-6;
     diff_counter= o->out_counter;
     limit= o->speed_limit;
   } else if(flag&4) {
     if(o->interval_start_time.tv_sec==0)
       o->interval_start_time= start_tv;
     diff_time= ((double) current_tv.tv_sec) 
              - ((double) o->interval_start_time.tv_sec)
              + (((double) current_tv.tv_usec) 
                 -((double) o->interval_start_time.tv_usec))*1e-6;
     diff_counter= o->out_counter - o->interval_start_counter;
     limit= -o->speed_limit;
   } else {
     diff_time= ((double) current_tv.tv_sec) - ((double) start_tv.tv_sec)
              + (((double) current_tv.tv_usec) 
                 -((double)start_tv.tv_usec))*1e-6;
     diff_counter= o->out_counter - start_out_counter;
     limit= -o->speed_limit;
   }
   if(diff_time>0.0)
     if(diff_counter/diff_time>limit) {
       min_wait_time= (diff_counter/limit - diff_time)*1.0e6;
       if(min_wait_time<*still_to_wait)
         *still_to_wait= min_wait_time;
       if(*still_to_wait>0)
         *speed_limiter= 1;
     }
 }
 if(o->source_fd>=0) {
   if(buffer_space>0) {
     FD_SET((o->source_fd),rds);
     if(*max_fd<o->source_fd)
       *max_fd= o->source_fd;
   } else if(o->interval_counter>0)
     o->full_counter++;
 }
 *ready_to_write= 0;
 if(o->dest_fd>=0 && (!(flag&2)) && !*speed_limiter) {
   if(fd_buffer_fill>=o->chunk_size || o->source_fd<0 || eop_is_near) {
     if((flag&(4|8))==(4|8)) {
       *still_to_wait= 0;
       *ready_to_write= 1;
     } else {
       FD_SET((o->dest_fd),wts);
       if(*max_fd<o->dest_fd)
         *max_fd= o->dest_fd;
     }
   } else if(o->interval_counter>0)
     o->empty_counter++;
 }
 return(1);
}


/* Perform post-select activities of Cdrfifo_try_to_work() */
static int Cdrfifo_transact(struct CdrfifO *o, fd_set *rds, fd_set *wts,
                            char *reply_buffer, int *reply_count, int flag)
/* flag:
 bit0= enable debug pacifier (same with Cdrfifo_debuG)
 bit1= do not write, just fill buffer
 bit2= fd-to-memory mode (else fd-to-fd mode):
       rather than writing a chunk return it and its size in reply_*
 bit3= with bit2: do not check destination fd for readiness
return: <0 = error , 0 = idle , 1 = did some work
*/
{
 double buffer_space;
 int can_read,can_write= 0,ret,did_work= 0,idx,sod, eop_idx;

 buffer_space= Cdrfifo_tell_buffer_space(o,0);
 if(o->dest_fd>=0) if(FD_ISSET((o->dest_fd),wts)) {
   can_write= o->buffer_size - buffer_space;
   if(can_write>o->chunk_size)
     can_write= o->chunk_size;
   if(o->read_idx+can_write > o->buffer_size)
     can_write= o->buffer_size - o->read_idx;
   if(o->follow_up_fd_idx>=0) {
     Cdrfifo_eop_adjust(o,&can_write,&eop_idx,0);
     if(can_write<=0)
       goto after_write;
   }
   if(flag&4) {
     memcpy(reply_buffer,o->buffer+o->read_idx,can_write);
     *reply_count= ret= can_write;
   } else {
     ret= write(o->dest_fd,o->buffer+o->read_idx,can_write);
   }
   if(ret==-1) {

     /* >>> handle broken pipe */;
     fprintf(stderr,"\ncdrfifo %d: on write: errno=%d , \"%s\"\n",
                    o->chain_idx,errno,
                    errno==0?"-no error code available-":strerror(errno));

     if(!(flag&4))
       close(o->dest_fd);
     o->dest_fd= -1;
     {ret= -1; goto ex;}
   }
   did_work= 1;
   o->get_counter++;
   o->out_counter+= can_write;
   o->read_idx+= can_write;
   if(o->read_idx>=o->buffer_size)
     o->read_idx= 0;
   o->buffer_is_full= 0;
 }
after_write:;
 if(o->source_fd>=0) if(FD_ISSET((o->source_fd),rds)) {
   can_read= o->buffer_size - o->write_idx;

#ifdef Libburn_has_open_trac_srC

   /* ts A91115
      This chunksize must be aligned to filesystem blocksize.
    */
#define Cdrfifo_o_direct_chunK 32768

   if(o->write_idx < o->read_idx && o->write_idx + can_read > o->read_idx)
     can_read= o->read_idx - o->write_idx;
   if(o->fd_in_limit>=0.0)
     if(can_read > o->fd_in_limit - o->fd_in_counter)
       can_read= o->fd_in_limit - o->fd_in_counter;
   /* Make sure to read with properly aligned size */
   if(can_read > Cdrfifo_o_direct_chunK)
     can_read= Cdrfifo_o_direct_chunK;
   else if(can_read < Cdrfifo_o_direct_chunK)
     can_read= -1;
   ret= 0;
   if(can_read>0)
     ret= read(o->source_fd,o->buffer+o->write_idx,can_read);
   if(can_read < 0) {
     /* waiting for a full Cdrfifo_o_direct_chunK to fit */
     if(can_write <= 0 && o->dest_fd >= 0) {
        fd_set rds,wts,exs;
        struct timeval wt;

        FD_ZERO(&rds);
        FD_ZERO(&wts);
        FD_ZERO(&exs);
        FD_SET((o->dest_fd),&wts);
        wt.tv_sec=  0;
        wt.tv_usec= 10000;
        select(o->dest_fd + 1,&rds, &wts, &exs, &wt);

     }
   } else

#else /* Libburn_has_open_trac_srC */

   if(can_read>o->chunk_size)
     can_read= o->chunk_size;
   if(o->write_idx<o->read_idx && o->write_idx+can_read > o->read_idx)
     can_read= o->read_idx - o->write_idx;
   if(o->fd_in_limit>=0.0)
     if(can_read > o->fd_in_limit - o->fd_in_counter)
       can_read= o->fd_in_limit - o->fd_in_counter;
   ret= 0;
   if(can_read>0)
     ret= read(o->source_fd,o->buffer+o->write_idx,can_read);

#endif /* ! Libburn_has_open_trac_srC */

   if(ret==-1) {

     /* >>> handle input error */;
     fprintf(stderr,"\ncdrfifo %d: on read: errno=%d , \"%s\"\n",
                    o->chain_idx,errno,
                    errno==0?"-no error code available-":strerror(errno));

     o->source_fd= -1;
   } else if(ret==0) { /* eof */
     /* activate eventual follow-up source fd */
     if(Cdrfifo_debuG || (flag&1))
       fprintf(stderr,"\ncdrfifo %d: on read(%d,buffer,%d): eof\n",
               o->chain_idx,o->source_fd,can_read);
     if(o->follow_up_fd_idx+1 < o->follow_up_fd_counter) {
       idx= ++(o->follow_up_fd_idx);
       o->source_fd= o->follow_up_fds[idx][0];
       /* End-Of-Previous */
       if(o->write_idx==0) {
         o->follow_up_eop[idx]= o->buffer_size;

         /* A70304 : can this happen ? */
         o->follow_up_was_full_buffer[idx]= (o->read_idx==0);

         if(Cdrfifo_debuG || (flag&1))
           fprintf(stderr,"\ncdrfifo %d: write_idx 0 on eop: read_idx= %d\n",
                          o->chain_idx,o->read_idx);

       } else
         o->follow_up_eop[idx]= o->write_idx;
       /* Start-Of-Data . Try to start at next full chunk */
       sod= o->write_idx;
       if(o->write_idx%o->chunk_size)
         sod+= o->chunk_size - (o->write_idx%o->chunk_size);
       /* but do not catch up to the read pointer */
       if((o->write_idx<=o->read_idx && o->read_idx<=sod) || sod==o->read_idx)
         sod= o->write_idx;
       if(sod>=o->buffer_size)
         sod= 0;
       o->follow_up_sod[idx]= sod;
       o->write_idx= sod;
       o->fd_in_counter= 0;
       o->fd_in_limit= o->follow_up_in_limits[idx];
       if(Cdrfifo_debuG || (flag&1))
         fprintf(stderr,"\ncdrfifo %d: new fifo source fd : %d\n",
                        o->chain_idx,o->source_fd);
     } else {
       o->source_fd= -1;
     }
   } else {
     did_work= 1;
     o->put_counter++;
     o->in_counter+= ret;
     o->fd_in_counter+= ret;
     o->write_idx+= ret;
     if(o->write_idx>=o->buffer_size)
       o->write_idx= 0;
     if(o->write_idx==o->read_idx)
       o->buffer_is_full= 1;
   }
 }
 ret= !!did_work;
ex:;
 return(ret);
}


/** Check for pending data at the fifo's source file descriptor and wether the
    fifo is ready to take them. Simultaneously check the buffer for existing
    data and the destination fd for readiness to accept some. If so, a small
    chunk of data is transfered to and/or from the fifo.
    This is done for the given fifo object and all members of its next-chain.
    The check and transactions are repeated until a given timespan has elapsed.
    libburn applications call this function in the burn loop instead of sleep().
    It may also be used instead of read(). Then it returns as soon as an output
    transaction would be performed. See flag:bit2.
    @param o The fifo object
    @param wait_usec The time in microseconds after which the function shall
                     return.
    @param reply_buffer with bit2: Returns write-ready buffer chunk and must
                                   be able to take at least chunk_size bytes
    @param reply_count with bit2: Returns number of writeable bytes in reply
    @param flag Bitfield for control purposes: 
                bit0= Enable debug pacifier (same with Cdrfifo_debuG)
                bit1= Do not write, just fill buffer
                bit2= fd-to-memory mode (else fd-to-fd mode):
                      Rather than writing a chunk return it and its size.
                      No simultaneous processing of chained fifos.
                bit3= With bit2: do not check destination fd for readiness
    @return <0 = error , 0 = idle , 1 = did some work , 2 = all work is done
*/
int Cdrfifo_try_to_work(struct CdrfifO *o, int wait_usec, 
                        char *reply_buffer, int *reply_count, int flag)
{
 struct timeval wt,start_tv,current_tv;
 struct timezone tz;
 fd_set rds,wts,exs;
 int ready,ret,max_fd= -1,buffer_space,dummy,still_active= 0;
 int did_work= 0,elapsed,still_to_wait,speed_limiter= 0,ready_to_write= 0;
 double start_out_counter;
 struct CdrfifO *ff;

 start_out_counter= o->out_counter;
 gettimeofday(&start_tv,&tz);
 still_to_wait= wait_usec;
 if(flag&4)
   *reply_count= 0;

try_again:; 
 /* is there still a destination open ? */
 for(ff= o; ff!=NULL; ff= ff->next)
   if(ff->dest_fd!=-1)
 break;
 if(ff==NULL)
   return(2);
 FD_ZERO(&rds);
 FD_ZERO(&wts);
 FD_ZERO(&exs);

 for(ff= o; ff!=NULL; ff= ff->next) {
   ret= Cdrfifo_setup_try(ff,start_tv,start_out_counter,
                          &still_to_wait,&speed_limiter,&ready_to_write,
                          &rds,&wts,&max_fd,flag&15);
   if(ret<=0)
     return(ret);
   else if(ret==2) {
     /* This fifo is done */;
   } else
     still_active= 1;
   if(flag&2) 
 break;
 }
 if(!still_active)
   return(2);

 if(still_to_wait>0 || max_fd>=0) {
   wt.tv_sec=  still_to_wait/1000000;
   wt.tv_usec= still_to_wait%1000000;
   ready= select(max_fd+1,&rds,&wts,&exs,&wt);
 } else
   ready= 0;
 if(ready<=0) {
   if(!ready_to_write)
     goto check_wether_done;
   FD_ZERO(&rds);
 }
 if(ready_to_write)
   FD_SET((o->dest_fd),&wts);

 for(ff= o; ff!=NULL; ff= ff->next) {
   ret= Cdrfifo_transact(ff,&rds,&wts,reply_buffer,reply_count,flag&15);
   if(ret<0)
     goto ex;
   if(ret>0)
     did_work= 1;
   if(flag&2) 
 break;
 }

check_wether_done:;
 if((flag&4) && *reply_count>0)
   {ret= 1; goto ex;}
 gettimeofday(&current_tv,&tz);
 elapsed= (current_tv.tv_sec-start_tv.tv_sec)*1000000 + 
          (((int) current_tv.tv_usec) - ((int) start_tv.tv_usec));
 still_to_wait= wait_usec-elapsed;
 if(still_to_wait>0)
   goto try_again;
 ret= !!did_work;
ex:;
 if(flag&4) {
   gettimeofday(&current_tv,&tz);
   elapsed= (current_tv.tv_sec - o->interval_start_time.tv_sec)*1000000
          + (((int) current_tv.tv_usec) 
              - ((int) o->interval_start_time.tv_usec));
 } else
   elapsed= wait_usec;
 if(elapsed>=wait_usec) {
   if((flag&1)||Cdrfifo_debuG>=2) {
     fprintf(stderr,"\n");
     for(ff= o; ff!=NULL; ff= ff->next) {
       buffer_space= Cdrfifo_tell_buffer_space(ff,0);
       fprintf(stderr,
               "cdrfifo %d:  w=%d r=%d | b=%d s=%d | i=%.f o=%.f\n",
               ff->chain_idx,ff->write_idx,ff->read_idx,
               ff->buffer_size-buffer_space,buffer_space,
               ff->in_counter,ff->out_counter);
     }
   }
   if(flag&4)
     Cdrfifo_next_interval(o,&dummy,0);
 }
 return(ret);
}


/** Fill the fifo as far as possible without writing to destination fd */
int Cdrfifo_fill(struct CdrfifO *o, int size, int flag)
{
 int ret,fill= 0,space,state;

 while(1) {
   state= Cdrfifo_get_buffer_state(o,&fill,&space,0);
   if(state==-1) {

     /* >>> handle error */;

     return(0);
   } else if(state!=1)
 break;
   if(space<=0)
 break;
   if(size>=0 && fill>=size)
 break;
   ret= Cdrfifo_try_to_work(o,100000,NULL,NULL,2);
   if(ret<0) {

     /* >>> handle error */;

     return(0);
   }
   if(ret==2)
 break;
 }

#ifndef Cdrfifo_standalonE
 if(fill>=32*2048) {
   int Scan_for_iso_size(unsigned char data[2048], double *size_in_bytes,
                         int flag);
   int bs= 16*2048;
   double size;

   /* memorize blocks 16 to 31 */
   if(o->iso_fs_descr!=NULL)
     free((char *) o->iso_fs_descr);
   o->iso_fs_descr= TSOB_FELD(char,bs);
   if(o->iso_fs_descr==NULL)
     return(-1);
   memcpy(o->iso_fs_descr,o->buffer+bs,bs);

   /* try to obtain ISO-9660 file system size from block 16 */
   ret= Scan_for_iso_size((unsigned char *) (o->buffer+bs), &size, 0);
   if(ret>0)
     o->iso_fs_size= size;
 }
#endif

 o->total_min_fill= fill;
 o->interval_min_fill= fill;
 return(1);
}


int Cdrfifo_close_all(struct CdrfifO *o, int flag)
{
 struct CdrfifO *ff;

 if(o==NULL)
   return(0);
 for(ff= o; ff->prev!=NULL; ff= ff->prev);
 for(; ff!=NULL; ff= ff->next)
   Cdrfifo_close(ff,0);
 return(1);
}



#ifdef Cdrfifo_standalonE

/* ---------------------------------------------------------------------- */

/** Application example. See also cdrskin.c */


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


/* This is a hardcoded test mock-up for two simultaneous fifos of which the
   one runs with block size 2048 and feeds the other which runs with 2352.
   Both fifos have the same number of follow_up pipes (tracks) which shall
   be connected 1-to-1.
*/
int Test_mixed_bs(char **paths, int path_count,
                  int fs_size, double speed_limit, double interval, int flag)
/*
 bit0= debugging verbousity
*/
{
 int fd_in[100],fd_out[100],ret,pipe_fds[100][2];
 int i,iv,stall_counter= 0,cycle_counter= 0.0;
 char target_path[80];
 double in_counter, out_counter, prev_in= -1.0, prev_out= -1.0;
 struct CdrfifO *ff_in= NULL, *ff_out= NULL;

 if(path_count<1)
   return(2);
 Cdrfifo_new(&ff_in,fd_in[0],fd_out[0],2048,fs_size,0);
 for(i= 0; i<path_count; i++) {
   fd_in[2*i]= open(paths[i],O_RDONLY);
   if(fd_in[2*i]==-1)
     return(0);
   if(pipe(pipe_fds[2*i])==-1)
     return(-1);
   fd_out[2*i]= pipe_fds[2*i][1];
   if(i==0)
     ret= Cdrfifo_new(&ff_in,fd_in[2*i],fd_out[2*i],2048,fs_size,0);
   else
     ret= Cdrfifo_attach_follow_up_fds(ff_in,fd_in[2*i],fd_out[2*i],0);
   if(ret<=0)
     return(ret);
   fd_in[2*i+1]= pipe_fds[2*i][0];
   sprintf(target_path,"/dvdbuffer/fifo_mixed_bs_test_%d",i);
   fd_out[2*i+1]= open(target_path,O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
   if(i==0)
     ret= Cdrfifo_new(&ff_out,fd_in[2*i+1],fd_out[2*i+1],2352,fs_size,0);
   else
     ret= Cdrfifo_attach_follow_up_fds(ff_out,fd_in[2*i+1],fd_out[2*i+1],0);
   if(ret<=0)
     return(ret);
   fprintf(stderr,"test_mixed_bs: %d : %2d fifo %2d pipe %2d fifo %2d : %s\n",
           i, fd_in[2*i],fd_out[2*i],fd_in[2*i+1],fd_out[2*i+1], target_path);
 }
 Cdrfifo_attach_peer(ff_in,ff_out,0);
 

 /* Let the fifos work */
 iv= interval*1e6;
 while(1) {
   ret= Cdrfifo_try_to_work(ff_in,iv,NULL,NULL,flag&1);
   if(ret<0 || ret==2) { /* <0 = error , 2 = work is done */
     fprintf(stderr,"\ncdrfifo %d: fifo ended work with ret=%d\n",
                    ff_in->chain_idx,ret);
     if(ret<0)
       return(-7);
 break;
   }
   cycle_counter++;
   Cdrfifo_get_counters(ff_in, &in_counter, &out_counter, 0);
   if(prev_in == in_counter && prev_out == out_counter)
     stall_counter++;
   prev_in= in_counter;
   prev_out= out_counter;
 }
 return(1);
}


/* This is a hardcoded test mock-up for two simultaneous fifos of which the
   first one simulates the cdrskin fifo feeding libburn and the second one 
   simulates libburn and the burner at given speed. Both have two fd pairs
   (i.e. tracks). The tracks are read from  /u/test/cdrskin/in_[12]  and
   written to  /u/test/cdrskin/out_[12].
*/
int Test_multi(int fs_size, double speed_limit, double interval, int flag)
/*
 bit0= debugging verbousity
*/
{
 int fd_in[4],fd_out[4],ret,pipe_fds[4][2];
 int i,iv;
 struct CdrfifO *ff1= NULL,*ff2= NULL;

 /* open four pairs of fds */
 fd_in[0]= open("/u/test/cdrskin/in_1",O_RDONLY);
 fd_in[1]= open("/u/test/cdrskin/in_2",O_RDONLY);
 fd_out[2]= open("/u/test/cdrskin/out_1",
                 O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
 fd_out[3]= open("/u/test/cdrskin/out_2",
                 O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
 if(pipe(pipe_fds[0])==-1)
   return(-3);
 if(pipe(pipe_fds[1])==-1)
   return(-3);
 fd_out[0]= pipe_fds[0][1];
 fd_out[1]= pipe_fds[1][1];
 fd_in[2]= pipe_fds[0][0];
 fd_in[3]= pipe_fds[1][0];
 for(i=0;i<4;i++) {
   if(fd_in[i]==-1)
     return(-1);
   if(fd_out[i]==-1)
     return(-2);
 }   

 /* Create two fifos with two sequential fd pairs each and chain them for
    simultaneous usage. */
 Cdrfifo_new(&ff1,fd_in[0],fd_out[0],2048,fs_size,0);
 Cdrfifo_new(&ff2,fd_in[2],fd_out[2],2048,2*1024*1024,0); /*burner cache 2 MB*/
 if(ff1==NULL || ff2==NULL)
   return(-3);
 Cdrfifo_set_speed_limit(ff2,speed_limit,0);
 ret= Cdrfifo_attach_follow_up_fds(ff1,fd_in[1],fd_out[1],0);
 if(ret<=0)
   return(-4);
 ret= Cdrfifo_attach_follow_up_fds(ff2,fd_in[3],fd_out[3],0);
 if(ret<=0)
   return(-4);
 Cdrfifo_attach_peer(ff1,ff2,0);

 /* Let the fifos work */
 iv= interval*1e6;
 while(1) {
   ret= Cdrfifo_try_to_work(ff1,iv,NULL,NULL,flag&1);
   if(ret<0 || ret==2) { /* <0 = error , 2 = work is done */
     fprintf(stderr,"\ncdrfifo %d: fifo ended work with ret=%d\n",
                    ff1->chain_idx,ret);
     if(ret<0)
       return(-7);
 break;
   }
 }
 return(1);
}
 

int main(int argc, char **argv)
{
 int i,ret,exit_value= 0,verbous= 1,fill_buffer= 0,min_fill,fifo_percent,fd;
 double fs_value= 4.0*1024.0*1024.0,bs_value= 2048,in_counter,out_counter;
 double interval= 1.0,speed_limit= 0.0;
 char output_file[4096];
 struct CdrfifO *ff= NULL;

 strcpy(output_file,"-");
 fd= 1;

 for(i= 1; i<argc; i++) {
   if(strncmp(argv[i],"bs=",3)==0) {
     bs_value= Scanf_io_size(argv[i]+3,0);
     if(bs_value<=0 || bs_value>1024.0*1024.0*1024.0) {
       fprintf(stderr,
               "cdrfifo: FATAL : bs=N expects a size between 1 and 1g\n");
       {exit_value= 2; goto ex;}
     }
   } else if(strncmp(argv[i],"fl=",3)==0) {
     sscanf(argv[i]+3,"%d",&fill_buffer);
     fill_buffer= !!fill_buffer;
   } else if(strncmp(argv[i],"fs=",3)==0) {
     fs_value= Scanf_io_size(argv[i]+3,0);
     if(fs_value<=0 || fs_value>1024.0*1024.0*1024.0) {
       fprintf(stderr,
               "cdrfifo: FATAL : fs=N expects a size between 1 and 1g\n");
       {exit_value= 2; goto ex;}
     }
   } else if(strncmp(argv[i],"iv=",3)==0) {
     sscanf(argv[i]+3,"%lf",&interval);
     if(interval<0.001 || interval>1000.0) 
       interval= 1;
   } else if(strncmp(argv[i],"of=",3)==0) {
     if(strcmp(argv[i]+3,"-")==0 || argv[i][3]==0)
 continue;
     fd= open(argv[i]+3,O_WRONLY|O_CREAT);
     if(fd<0) {
       fprintf(stderr,"cdrfifo: FATAL : cannot open output file '%s'\n",
               argv[i]+3);
       fprintf(stderr,"cdrfifo: errno=%d , \"%s\"\n",
               errno,errno==0?"-no error code available-":strerror(errno));
       {exit_value= 4; goto ex;}
     }
   } else if(strncmp(argv[i],"sl=",3)==0) {
     speed_limit= Scanf_io_size(argv[i]+3,0);
   } else if(strncmp(argv[i],"vb=",3)==0) {
     sscanf(argv[i]+3,"%d",&verbous);

   } else if(strcmp(argv[i],"-mixed_bs_test")==0) {

     ret= Test_mixed_bs(argv+i+1,argc-i-1,
                        (int) fs_value,speed_limit,interval,(verbous>=2));
     fprintf(stderr,"Test_mixed_bs(): ret= %d\n",ret);
     exit(ret<0);

   } else if(strcmp(argv[i],"-multi_test")==0) {

     if(speed_limit==0.0)
       speed_limit= 10*150*1024;
     ret= Test_multi((int) fs_value,speed_limit,interval,(verbous>=2));
     fprintf(stderr,"Test_multi(): ret= %d\n",ret);
     exit(ret<0);

   } else {
     fprintf(stderr,"cdrfifo 0.3 : stdin-to-stdout fifo buffer.\n");
     fprintf(stderr,"usage : %s [bs=block_size] [fl=fillfirst]\n",argv[0]);
     fprintf(stderr,"       [fs=fifo_size] [iv=interval] [of=output_file]\n");
     fprintf(stderr,"       [sl=bytes_per_second_limit] [vb=verbosity]\n");
     fprintf(stderr,"fl=1 reads full buffer before writing starts.\n");
     fprintf(stderr,"sl>0 allows catch up for whole run time.\n");
     fprintf(stderr,"sl<0 allows catch up for single interval.\n");
     fprintf(stderr,"vb=0 is silent, vb=2 is debug.\n");
     fprintf(stderr,"example: cdrfifo bs=8k fl=1 fs=32m iv=0.1 sl=-5400k\n");
     if(strcmp(argv[i],"-help")!=0 && strcmp(argv[i],"--help")!=0) {
       fprintf(stderr,"\ncdrfifo: FATAL : option not recognized: '%s'\n",
               argv[i]);
       exit_value= 1;
     }
     goto ex;
   }
 }
 if(verbous>=1) {
   fprintf(stderr,
           "cdrfifo: bs=%.lf fl=%d fs=%.lf iv=%lf of='%s' sl=%.lf vb=%d\n",
           bs_value,fill_buffer,fs_value,interval,output_file,speed_limit,
           verbous);
 }

 ret= Cdrfifo_new(&ff,0,fd,(int) bs_value,(int) fs_value,0);
 if(ret<=0) {
   fprintf(stderr,
           "cdrfifo: FATAL : creation of fifo object with %.lf bytes failed\n",
           fs_value);
   {exit_value= 3; goto ex;}
 }
 if(speed_limit!=0.0)
   Cdrfifo_set_speed_limit(ff,speed_limit,0);
 if(fill_buffer) {
   ret= Cdrfifo_fill(ff,0,0);
   if(ret<=0) {
     fprintf(stderr,
             "cdrfifo: FATAL : initial filling of fifo buffer failed\n");
     {exit_value= 4; goto ex;}
   }
 }
 while(1) {
   ret= Cdrfifo_try_to_work(ff,(int) (interval*1000000.0),
                            NULL,NULL,(verbous>=2));
   if(ret<0) {
     fprintf(stderr,"\ncdrfifo: FATAL : fifo aborted. errno=%d , \"%s\"\n",
             errno,errno==0?"-no error code available-":strerror(errno));
     {exit_value= 4; goto ex;}
   } else if(ret==2) {
     if(verbous>=1) {
       double put_counter,get_counter,empty_counter,full_counter;
       int total_min_fill;
       Cdrfifo_get_counters(ff,&in_counter,&out_counter,0);
       fprintf(stderr,"\ncdrfifo: done : %.lf bytes in , %.lf bytes out\n",
               in_counter,out_counter);
       Cdrfifo_get_min_fill(ff,&total_min_fill,&min_fill,0);
       fifo_percent= 100.0*((double) total_min_fill)/fs_value;
       if(fifo_percent==0 && total_min_fill>0)
         fifo_percent= 1;
       Cdrfifo_get_cdr_counters(ff,&put_counter,&get_counter,
                                &empty_counter,&full_counter,0);
       fprintf(stderr,"cdrfifo: fifo had %.lf puts and %.lf gets.\n",
               put_counter,get_counter);
       fprintf(stderr,
"cdrfifo: fifo was %.lf times empty and %.lf times full, min fill was %d%%.\n",
               empty_counter,full_counter,fifo_percent);
     }
 break;
   }
   Cdrfifo_next_interval(ff,&min_fill,0);
 }

ex:;
 Cdrfifo_destroy(&ff,0);
 exit(exit_value);
}


#endif /* Cdrfifo_standalonE */

