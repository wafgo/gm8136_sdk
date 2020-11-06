
/*
 cdrfifo.c , Copyright 2006 Thomas Schmitt <scdbackup@gmx.net>

 A fd-to-fd or fd-to-memory fifo to be used within cdrskin or independently.
 By chaining of fifo objects, several fifos can be run simultaneously
 in fd-to-fd mode. Modes are controlled by parameter flag of
 Cdrfifo_try_to_work().

 Provided under GPL license within cdrskin and under BSD license elsewise.
*/

#ifndef Cdrfifo_headerfile_includeD
#define Cdrfifo_headerfile_includeD


/** The fifo buffer which will smoothen the data stream from data provider
    to data consumer. Although this is not a mandatory lifesaver for modern
    burners any more, a fifo can speed up burning of data which is delivered
    with varying bandwidths (e.g. compressed archives created on the fly
    or mkisofs running at its speed limit.).
    This structure is opaque to applications and may only be used via
    the Cdrfifo*() methods described in cdrfifo.h .
*/
struct CdrfifO;


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
                int chunk_size, int buffer_size, int flag);

/** Release from memory a fifo object previously created by Cdrfifo_new().
    @param ff The victim (gets returned as NULL, call can stand *ff==NULL)
    @param flag Bitfield for control purposes:
                bit0= do not close destination fd
*/
int Cdrfifo_destroy(struct CdrfifO **ff, int flag);

/** Close any output fds */
int Cdrfifo_close(struct CdrfifO *o, int flag);

/** Close any output fds of o and its chain peers */
int Cdrfifo_close_all(struct CdrfifO *o, int flag);

int Cdrfifo_get_sizes(struct CdrfifO *o, int *chunk_size, int *buffer_size,
                      int flag);

/** Set a speed limit for buffer output.
    @param o The fifo object
    @param bytes_per_second >0 catch up slowdowns over the whole run time
                            <0 catch up slowdowns only over one interval
                            =0 disable speed limit
*/
int Cdrfifo_set_speed_limit(struct CdrfifO *o, double bytes_per_second,
                            int flag);

/** Set a fixed size for input in order to cut off any unwanted tail
    @param o The fifo object
    @param idx index for fds attached via Cdrfifo_attach_follow_up_fds(),
               first attached is 0,  <0 directs limit to active fd limit 
               (i.e. first track is -1, second track is 0, third is 1, ...)
*/
int Cdrfifo_set_fd_in_limit(struct CdrfifO *o, double fd_in_limit, int idx,
                            int flag);


int Cdrfifo_set_fds(struct CdrfifO *o, int source_fd, int dest_fd, int flag);
int Cdrfifo_get_fds(struct CdrfifO *o, int *source_fd, int *dest_fd, int flag);


/** Attach a further pair of input and output fd which will use the same
    fifo buffer when its predecessors are exhausted. Reading will start as
    soon as reading of the predecessor encounters EOF. Writing will start
    as soon as all pending predecessor data are written.
    @return index number of new item + 1, <=0 indicates error
*/
int Cdrfifo_attach_follow_up_fds(struct CdrfifO *o, int source_fd, int dest_fd,
                                 int flag);

/** Attach a further fifo which shall be processed simultaneously with this
    one by Cdrfifo_try_to_work() in fd-to-fd mode. 
*/
int Cdrfifo_attach_peer(struct CdrfifO *o, struct CdrfifO *next, int flag);


/** Obtain buffer state.
    @param o The buffer object
    @param fill Returns the number of pending payload bytes in the buffer
    @param space Returns the number of unused buffer bytes 
    @param flag unused yet
    @return -1=error , 0=inactive , 1=reading and writing ,
            2=reading ended (but still writing)
*/
int Cdrfifo_get_buffer_state(struct CdrfifO *o,int *fill,int *space,int flag);

int Cdrfifo_get_counters(struct CdrfifO *o,
                         double *in_counter, double *out_counter, int flag);

/** reads min_fill and begins measurement interval for next min_fill */
int Cdrfifo_next_interval(struct CdrfifO *o, int *min_fill, int flag);

int Cdrfifo_get_min_fill(struct CdrfifO *o, int *total_min_fill,
                         int *interval_min_fill, int flag);

int Cdrfifo_get_cdr_counters(struct CdrfifO *o,
                             double *put_counter, double *get_counter,
                             double *empty_counter, double *full_counter, 
                             int flag);

/** Inquire the eventually detected size of an eventual ISO-9660 file system
    @return 0=no ISO resp. size detected, 1=size_in_bytes is valid
*/
int Cdrfifo_get_iso_fs_size(struct CdrfifO *o, double *size_in_bytes,int flag);


/** Take over the eventually memorized blocks 16 to 31 of input (2 kB each).
    The fifo forgets the blocks by this call. I.e. a second one will return 0.
    After this call it is the responsibility of the caller to dispose the
    retrieved memory via call free().
    @param pt Will be filled either with NULL or a pointer to 32 kB of data
    @return 0=nothing is buffered, 1=pt points to valid freeable data
*/
int Cdrfifo_adopt_iso_fs_descr(struct CdrfifO *o, char **pt, int flag);


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
    @param reply_count with bit2: Returns number of writeable bytes in reply_pt
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
                        char *reply_buffer, int *reply_count, int flag);

/** Fill the fifo as far as possible without writing to destination fd.
    @param size if >=0 : end filling after the given number of bytes
    @return 1 on success, <=0 on failure
*/
int Cdrfifo_fill(struct CdrfifO *o, int size, int flag);


#endif /* Cdrfifo_headerfile_includeD */

