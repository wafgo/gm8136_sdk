/*
 * BD/DVD±RW/-RAM format 7.1 by Andy Polyakov <appro@fy.chalmers.se>.
 *
 * Use-it-on-your-own-risk, GPL bless...
 *
 * For further details see http://fy.chalmers.se/~appro/linux/DVD+RW/.
 *
 * Revision history:
 *
 * 2.0:
 * - deploy "IMMED" bit in "FORMAT UNIT";
 * 2.1:
 * - LP64 fix;
 * - USB workaround;
 * 3.0:
 * - C++-fication for better portability;
 * - SYSV signals for better portability;
 * - -lead-out option for improved DVD+RW compatibility;
 * - tested with SONY DRU-500A;
 * 4.0:
 * - support for DVD-RW formatting and blanking, tool name becomes
 *   overloaded...
 * 4.1:
 * - re-make it work under Linux 2.2 kernel;
 * 4.2:
 * - attempt to make DVD-RW Quick Format truly quick, upon release
 *   is verified to work with Pioneer DVR-x05;
 * - media reload is moved to growisofs where is actually belongs;
 * 4.3:
 * - -blank to imply -force;
 * - reject -blank in DVD+RW context and -lead-out in DVD-RW;
 * 4.4:
 * - support for -force=full in DVD-RW context;
 * - ask unit to perform OPC if READ DISC INFORMATION doesn't return
 *   any OPC descriptors;
 * 4.5:
 * - increase timeout for OPC, NEC multi-format derivatives might
 *   require more time to fulfill the OPC procedure;
 * 4.6:
 * - -force to ignore error from READ DISC INFORMATION;
 * - -force was failing under FreeBSD with 'unable to unmount';
 * - undocumented -gui flag to ease progress indicator parsing for
 *   GUI front-ends;
 * 4.7:
 * - when formatting DVD+RW, Pioneer DVR-x06 remained unaccessible for
 *   over 2 minutes after dvd+rw-format exited and user was frustrated
 *   to poll the unit himself, now dvd+rw-format does it for user;
 * 4.8:
 * - DVD-RW format fails if preceeded by dummy recording;
 * - make sure we talk to MMC unit, be less intrusive;
 * - unify error reporting;
 * - permit for -lead-out even for blank DVD+RW, needed(?) for SANYO
 *   derivatives;
 * 4.9:
 * - permit for DVD-RW blank even if format descriptors are not present;
 * 4.10:
 * - add support for DVD-RAM;
 * 6.0:
 * - versioning harmonization;
 * - WIN32 port;
 * - Initial DVD+RW Double Layer support;
 * - Ignore "WRITE PROTECTED" error in OPC;
 * 6.1:
 * - ± localization;
 * - Treat only x73xx OPC errors as fatal;
 * 7.0:
 * - Blu-ray Disc support;
 * - Mac OS X 10>=2 support;
 * 7.1:
 * - refine x73xx error treatment;
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix) || defined(__unix__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/wait.h>
#endif

#include "transport.hxx"

static void usage (char *prog)
{ fprintf (stderr,"- usage: %s [-force[=full]] [-lead-out|-blank[=full]]\n"
		  "         [-ssa[=none|default|max|XXXm]] /dev/dvd\n",prog),
  exit(1);
}

#ifdef _WIN32
#include <process.h>

# if defined(__GNUC__)
#  define __shared__	__attribute__((section(".shared"),shared))
# elif defined(_MSC_VER)
#  define __shared__
#  pragma data_seg(".shared")
#  pragma comment(linker,"/section:.shared,rws")
# endif
static volatile int shared_progress_indicator __shared__ = 0;
# if defined(_MSC_VER)
#  pragma data_seg()
# endif

static volatile int *progress = &shared_progress_indicator;
static HANDLE Process    = NULL;

BOOL WINAPI GoBackground (DWORD code)
{   if (*progress)
    {	FreeConsole();	/* detach from console upon ctrl-c or window close */
	return TRUE;
    }
  return FALSE;
}

void KillForeground (void)
{   fprintf(stderr,"\n");
    TerminateProcess(Process,0);
}
#else
static volatile int *progress;
#endif

static const char   *gui_action=NULL;
static const char   *str = "|/-\\",*backspaces="\b\b\b\b\b\b\b\b\b\b";

#if defined(__unix) || defined(__unix__)
extern "C" void alarm_handler (int no)
{ static int         i=0,len=1,old_progress=0;
  int new_progress = *progress;

    if (gui_action)
    {	fprintf (stderr,"* %s %.1f%%\n",gui_action,
			100.0*new_progress/65536.0);
	alarm(3);
	return;
    }

    if (new_progress != old_progress)
        len = fprintf (stderr,"%.*s%.1f%%",len,backspaces,
				100.0*new_progress/65536.0) - len,
	old_progress = new_progress;
    else
        fprintf (stderr,"\b%c",str[i]),
	i++, i&=3;

    alarm(1);
}
#endif

int main (int argc, char *argv[])
{ unsigned char	formats[260],dinfo[32],inq[128];
  char		*dev=NULL,*p;
  unsigned int	capacity,lead_out,mmc_profile,err;
  int		len,i;
  int		force=0,full=0,compat=0,blank=0,ssa=0,do_opc=0,gui=0,
		blu_ray=0,not_pow=0;
#ifdef _WIN32
  DWORD		ppid;
  char		filename[MAX_PATH],*progname;

    /*
     * Foreground process spawns itself and simply waits for shared
     * progress indicator to increment...
     */
    if (sscanf(argv[0],":%u:",&ppid) != 1)
    { int  i=0,len=1,old_progress=0,new_progress;

	sprintf (filename,":%u:",GetCurrentProcessId());
	progname = argv[0];
	argv[0]  = filename;
	Process  = (HANDLE)_spawnv (_P_NOWAIT,progname,argv);
	if (Process == (HANDLE)-1)
	    perror("_spawnv"), ExitProcess(errno);

	while(1)
	{   if (WaitForSingleObject(Process,1000) == WAIT_OBJECT_0)
	    {	ppid = 0; /* borrow DWORD variable */
		GetExitCodeProcess(Process,&ppid);
		ExitProcess(ppid);
	    }

	    new_progress = *progress;
	    if (new_progress != old_progress)
		len = fprintf (stderr,"%.*s%.1f%%",len,backspaces,
					100.0*new_progress/65536.0) - len,
		old_progress = new_progress;
	    else if (new_progress)
		fprintf (stderr,"\b%c",str[i]),
		i++, i&=3;
	}
    }

    /*
     * ... while background process does *all* the job...
     */
    Process = OpenProcess (PROCESS_TERMINATE,FALSE,ppid);
    if (Process == NULL)
	perror("OpenProcess"), ExitProcess(errno);

    atexit (KillForeground);
    SetConsoleCtrlHandler (GoBackground,TRUE);

    GetModuleFileName (NULL,filename,sizeof(filename));
    progname = strrchr(filename,'\\');
    if (progname)	argv[0] = progname+1;
    else		argv[0] = filename;
#elif defined(__unix) || defined(__unix__)
  pid_t		pid;

    { int fd;
      char *s;

	if ((fd=mkstemp (s=strdup("/tmp/dvd+rw-format.XXXXXX"))) < 0)
	    fprintf (stderr,":-( unable to mkstemp(\"%s\")",s),
	    exit(1);

	ftruncate(fd,sizeof(*progress));
	unlink(s);

	progress = (int *)mmap(NULL,sizeof(*progress),PROT_READ|PROT_WRITE,
				MAP_SHARED,fd,0);
	close (fd);
	if (progress == MAP_FAILED)
            perror (":-( unable to mmap anonymously"),
	    exit(1);
    }
    *progress = 0;

    if ((pid=fork()) == (pid_t)-1)
	perror (":-( unable to fork()"),
	exit(1);

    if (pid)
    { struct sigaction sa;

	sigaction (SIGALRM,NULL,&sa);
	sa.sa_flags &= ~SA_RESETHAND;
	sa.sa_flags |= SA_RESTART;
	sa.sa_handler = alarm_handler;
	sigaction (SIGALRM,&sa,NULL);
	alarm(1);
	while ((waitpid(pid,&i,0) != pid) && !WIFEXITED(i)) ;
	if (WEXITSTATUS(i) == 0) fprintf (stderr,"\n");
	exit (0);
    }
#endif

    fprintf (stderr,"* BD/DVD%sRW/-RAM format utility by <appro@fy.chalmers.se>, "
		    "version 7.1.\n",plusminus_locale());

    for (i=1;i<argc;i++) {
	if (*argv[i] == '-')
	    if (argv[i][1] == 'f')	// -format|-force
	    {	force = 1;
		if ((p=strchr(argv[i],'=')) && p[1]=='f') full=1;
	    }
	    else if (argv[i][1] == 'l')	// -lead-out
		force = compat = 1;
	    else if (argv[i][1] == 'b')	// -blank
	    {	blank=0x11;
		if ((p=strchr(argv[i],'=')) && p[1]=='f') blank=0x10;
		force=1;
	    }
	    else if (argv[i][1] == 'p')	// -pow
	    {	not_pow=1;		// minus pow
	    }
	    else if (argv[i][1] == 's')	// -ssa|-spare
	    {	force=ssa=1;
		if ((p=strchr(argv[i],'=')))
		{   if (p[1]=='n')	ssa=-1;	// =none
		    else if (p[3]=='n') ssa=1;	// =min
		    else if (p[1]=='d')	ssa=2;	// =default
		    else if (p[3]=='x')	ssa=3;	// =max
		    else if (p[1]=='.' || (p[1]>='0' && p[1]<='9'))
		    { char  *s=NULL;
		      double a=strtod(p+1,&s);

			if (s)
			{   if      (*s=='g' || *s=='G')    a *= 1e9;
			    else if (*s=='m' || *s=='M')    a *= 1e6;
			    else if (*s=='k' || *s=='K')    a *= 1e3;
			}
			ssa = (int)(a/2e3);	// amount of 2K
		    }
		    else		ssa=0;	// invalid?
		}
	    }
	    else if (argv[i][1] == 'g')	gui=1;
	    else			usage(argv[0]);
#ifdef _WIN32
	else if (argv[i][1] == ':')
#else
	else if (*argv[i] == '/')
#endif
	    dev = argv[i];
	else
	    usage (argv[0]);
    }

    if (dev==NULL) usage (argv[0]);

    Scsi_Command	cmd;

    if (!cmd.associate(dev))
	fprintf (stderr,":-( unable to open(\"%s\"): ",dev), perror (NULL),
	exit(1);

    cmd[0] = 0x12;	// INQUIRY
    cmd[4] = 36;
    cmd[5] = 0;
    if ((err=cmd.transport(READ,inq,36)))
	sperror ("INQUIRY",err), exit (1);

    if ((inq[0]&0x1F) != 5)
	fprintf (stderr,":-( not an MMC unit!\n"),
	exit (1);

    cmd[0] = 0x46;		// GET CONFIGURATION
    cmd[8] = 8;
    cmd[9] = 0;
    if ((err=cmd.transport(READ,formats,8)))
	sperror ("GET CONFIGURATION",err), exit (1);

    mmc_profile = formats[6]<<8|formats[7];

    blu_ray = ((mmc_profile&0xF0)==0x40);

    if (mmc_profile!=0x1A && mmc_profile!=0x2A
	&& mmc_profile!=0x14 && mmc_profile!=0x13
	&& mmc_profile!=0x12
	&& !blu_ray)
	fprintf (stderr,":-( mounted media doesn't appear to be "
			"DVD%sRW, DVD-RAM or Blu-ray\n",plusminus_locale()),
	exit (1);

    /*
     * First figure out how long the actual list is. Problem here is
     * that (at least Linux) USB units absolutely insist on accurate
     * cgc.buflen and you can't just set buflen to arbitrary value
     * larger than actual transfer length.
     */
    int once=1;
    do
    {	cmd[0] = 0x23;		// READ FORMAT CAPACITIES
	cmd[8] = 4;
	cmd[9] = 0;
	if ((err=cmd.transport(READ,formats,4)))
	{   if (err==0x62800 && once)	// "MEDIUM MAY HAVE CHANGED"
	    {	cmd[0] = 0;		// TEST UNIT READY
		cmd[5] = 0;
		cmd.transport();	// just swallow it...
		continue;
	    }
	    sperror ("READ FORMAT CAPACITIES",err), exit (1);
	}
    } while (once--);

    len = formats[3];
    if (len&7 || len<8)
	fprintf (stderr,":-( allocation length isn't sane\n"),
	exit(1);

    cmd[0] = 0x23;		// READ FORMAT CAPACITIES
    cmd[7] = (4+len)>>8;	// now with real length...
    cmd[8] = (4+len)&0xFF;
    cmd[9] = 0;
    if ((err=cmd.transport(READ,formats,4+len)))
	sperror ("READ FORMAT CAPACITIES",err), exit (1);

    if (len != formats[3])
	fprintf (stderr,":-( parameter length inconsistency\n"),
	exit(1);

    if (mmc_profile==0x1A || mmc_profile==0x2A)	// DVD+RW
    {	for (i=8;i<len;i+=8)	// look for "DVD+RW Full" descriptor
	    if ((formats [4+i+4]>>2) == 0x26) break;
    }
    else if (mmc_profile==0x12)	// DVD-RAM
    { unsigned int   v,ref;
      unsigned char *f,descr=0x01;
      int j;

	switch (ssa)
	{   case -1:	// no ssa
		for (ref=0,i=len,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x00)
		    {	v=f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (v>ref) ref=v,i=j;
		    }
		}
		break;
	    case 1:	// first ssa
		for (i=8;i<len;i+=8)
		    if ((formats[4+i+4]>>2) == 0x01) break;
		break;
	    case 2:	// default ssa
		descr=0x00;
	    case 3:	// max ssa
		for (ref=0xFFFFFFFF,i=len,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == descr)
		    {	v=f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (v<ref) ref=v,i=j;
		    }
		}
		break;
	    default:
		i=8;	// just grab the first descriptor?
		break;
	}
    }
    else if (mmc_profile==0x41)		// BD-R
    { unsigned int   max,min,cap;
      unsigned char *f;
      int j;

	switch (ssa)
	{   case -1:	// no spare -> nothing to do
	    case 0:
		exit (0);
		break;
	    case 1:	// min spare <- max capacity
		i=len;
		for (max=0,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x32)
		    {	cap = f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (max < cap) max=cap,i=j;
		    }
		}
		break;
	    case 2:	// default ssa
		i=8;	// just grab first descriptor
		break;
	    case 3:	// max, ~10GB, is too big to trust user
		fprintf (stderr,"- -ssa=max is not supported for BD-R, "
				"specify explicit size with -ssa=XXXG\n");
		exit (1);
	    default:
		i=len;
		for (max=0,min=0xffffffff,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x32)
		    {	cap = f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (max < cap) max=cap;
			if (min > cap) min=cap;
		    }
		}
		if (max==0) break;
 
		// for simplicity adjust spare size according to DL(!) rules
		ssa += 8192, ssa &= ~16383;	// i.e. in 32MB increments

		f = formats+4;
		capacity = f[0]<<24|f[1]<<16|f[2]<<8|f[3];

		cap = capacity - ssa;
		// place it within given boundaries
		if      (cap < min)	cap = min;
		else if (cap > max)	cap = max;

		i = 8;
		f = formats+4+i;
		f[0] = cap>>24;
		f[1] = cap>>16;
		f[2] = cap>>8;
		f[3] = cap;
		f[4] = 0x32<<2 | not_pow;
		f[5] = 0;
		f[6] = 0;
		f[7] = 0;
		break;
	}
	
	if (i<len)
	{   f = formats+4+i;
	    f[4] &= ~3;		// it's either SRM+POW ...
	    f[4] |= not_pow;	// ... or SRM-POW
	}
    }
    else if (mmc_profile==0x43)		// BD-RE
    { unsigned int   max,min,cap;
      unsigned char *f;
      int j;

	switch (ssa)
	{   case -1:	// no spare
		for (i=8;i<len;i+=8)	// look for descriptor 0x31
		    if ((formats [4+i+4]>>2) == 0x31) break;
		break;
	    case 0:
		i = 8;
		if ((formats[4+4]&3)==2)// same capacity for already formatted
		{   f = formats+4+i;
		    memcpy (f,formats+4,4);
		    f[4] = 0x30<<2;
		    f[5] = 0;
		    f[6] = 0;
		    f[7] = 0;
		}
		break;
	    case 1:	// min spare <- max capacity
		i=len;
		for (max=0,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x30)
		    {	cap = f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (max < cap) max=cap,i=j;
		    }
		}
		break;
	    case 2:	// default ssa
		i=8;	// just grab first descriptor
		break;
	    case 3:	// max spare <- min capacity
		i=len;
		for (min=0xffffffff,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x30)
		    {	cap=f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (min > cap) min=cap,i=j;
		    }
		}
		break;
	    default:
		i=len;
		capacity=0;
		for (max=0,min=0xffffffff,j=8;j<len;j+=8)
		{   f = formats+4+j;
		    if ((f[4]>>2) == 0x30)
		    {	cap = f[0]<<24|f[1]<<16|f[2]<<8|f[3];
			if (max < cap) max=cap;
			if (min > cap) min=cap;
		    }
		    else if ((f[4]>>2) == 0x31)
			capacity = f[0]<<24|f[1]<<16|f[2]<<8|f[3];
		}
		if (max==0 || capacity==0) break;
 
		// for simplicity adjust spare size according to DL(!) rules
		ssa += 8192, ssa &= ~16383;	// i.e. in 32MB increments

		cap = capacity - ssa;
		// place it within given boundaries
		if      (cap < min)	cap = min;
		else if (cap > max)	cap = max;

		i = 8;
		f = formats+4+i;
		f[0] = cap>>24;
		f[1] = cap>>16;
		f[2] = cap>>8;
		f[3] = cap;
		f[4] = 0x30<<2;
		f[5] = 0;
		f[6] = 0;
		f[7] = 0;
		break;
	}

	if (i<len)
	{   f = formats+4+i;
	    f[4] &= ~3;
	    if ((f[4]>>2)==0x30)
		f[4] |= full?2:3;	// "Full" or "Quick Certification"
	}
    }
    else			// DVD-RW
    { int descr=full?0x10:0x15;

	for (i=8;i<len;i+=8)	// look for "DVD-RW Quick" descriptor
	    if ((formats [4+i+4]>>2) == descr) break;
	if (descr==0x15 && i==len)
	{   fprintf (stderr,":-( failed to locate \"Quick Format\" descriptor.\n");
	    for (i=8;i<len;i+=8)// ... alternatively for "DVD-RW Full"
		if ((formats [4+i+4]>>2) == 0x10) break;
	}
    }

    if (i==len)
    {	fprintf (stderr,":-( can't locate appropriate format descriptor\n");
	if (blank)	i=0;
	else		exit(1);
    }

    capacity = 0;
    if (blu_ray)
    {	capacity |= formats[4+0], capacity <<= 8;
	capacity |= formats[4+1], capacity <<= 8;
	capacity |= formats[4+2], capacity <<= 8;
	capacity |= formats[4+3];
    }
    else
    {	capacity |= formats[4+i+0], capacity <<= 8;
	capacity |= formats[4+i+1], capacity <<= 8;
	capacity |= formats[4+i+2], capacity <<= 8;
	capacity |= formats[4+i+3];
    }

    if (mmc_profile==0x1A || mmc_profile==0x2A)	// DVD+RW
	fprintf (stderr,"* %.1fGB DVD+RW media detected.\n",
			2048.0*capacity/1e9);
    else if (mmc_profile==0x12)			// DVD-RAM
	fprintf (stderr,"* %.1fGB DVD-RAM media detected.\n",
			2048.0*capacity/1e9);
    else if (blu_ray)				// BD
	fprintf (stderr,"* %.1fGB BD media detected.\n",
			2048.0*capacity/1e9);
    else					// DVD-RW
	fprintf (stderr,"* %.1fGB DVD-RW media in %s mode detected.\n",
			2048.0*capacity/1e9,
			mmc_profile==0x13?"Restricted Overwrite":"Sequential");

    lead_out = 0;
    lead_out |= formats[4+0], lead_out <<= 8;
    lead_out |= formats[4+1], lead_out <<= 8;
    lead_out |= formats[4+2], lead_out <<= 8;
    lead_out |= formats[4+3];

    cmd[0] = 0x51;		// READ DISC INFORMATION
    cmd[8] = sizeof(dinfo);
    cmd[9] = 0;
    if ((err=cmd.transport(READ,dinfo,sizeof(dinfo))))
    {	sperror ("READ DISC INFORMATION",err);
	if (!force) exit(1);
	memset (dinfo,0xff,sizeof(dinfo));
	cmd[0] = 0x35;
	cmd[9] = 0;
	cmd.transport();
    }

    do_opc = ((dinfo[0]<<8|dinfo[1])<=0x20);

    if (dinfo[2]&3)		// non-blank media
    {	if (!force)
	{   if (mmc_profile==0x1A || mmc_profile==0x2A || mmc_profile==0x13 || mmc_profile==0x12)
		fprintf (stderr,"- media is already formatted, lead-out is currently at\n"
				"  %d KiB which is %.1f%% of total capacity.\n",
				lead_out*2,(100.0*lead_out)/capacity);
	    else
		fprintf (stderr,"- media is not blank\n");
	offer_options:
	    if (mmc_profile == 0x1A || mmc_profile == 0x2A)
		fprintf (stderr,"- you have the option to re-run %s with:\n"
				"  -lead-out  to elicit lead-out relocation for better\n"
				"             DVD-ROM compatibility, data is not affected;\n"
				"  -force     to enforce new format (not recommended)\n"
				"             and wipe the data.\n",
				argv[0]);
	    else if (mmc_profile == 0x12)
		fprintf (stderr,"- you have the option to re-run %s with:\n"
				"  -format=full  to perform full (lengthy) reformat;\n"
				"  -ssa[=none|default|max]\n"
				"                to grow, eliminate, reset to default or\n"
				"                maximize Supplementary Spare Area.\n",
				argv[0]);
	    else if (mmc_profile == 0x43)
		fprintf (stderr,"- you have the option to re-run %s with:\n"
				"  -format=full  to perform (lengthy) Full Certification;\n"
				"  -ssa[=none|default|max|XXX[GM]]\n"
				"                to eliminate or adjust Spare Area.\n",
				argv[0]);
	    else if (blu_ray)	// BD-R
		fprintf (stderr,"- BD-R can be pre-formatted only once\n");
	    else
	    {	fprintf (stderr,"- you have the option to re-run %s with:\n",
				argv[0]);
		if (i) fprintf (stderr,
				"  -force[=full] to enforce new format or mode transition\n"
				"                and wipe the data;\n");
		fprintf (stderr,"  -blank[=full] to change to Sequential mode.\n");
	    }
	    exit (0);
	}
	else if (cmd.umount())
	    perror (errno==EBUSY ? ":-( unable to proceed with format" :
				   ":-( unable to umount"),
	    exit (1);
    }
    else
    {	if (mmc_profile==0x14 && blank==0 && !force)
	{   fprintf (stderr,"- media is blank\n");
	    fprintf (stderr,"- given the time to apply full blanking procedure I've chosen\n"
			    "  to insist on -force option upon mode transition.\n");
	    exit (0);
	}
	else if (mmc_profile==041 && !force)
	{   fprintf (stderr,"- media is blank\n");
	    fprintf (stderr,"- as BD-R can be pre-formance only once, I've chosen\n"
			    "  to insist on -force option.\n");
	}
	force = 0;
    }

    if (((mmc_profile == 0x1A || mmc_profile == 0x2A) && blank)
	|| (mmc_profile != 0x1A && compat)
	|| (mmc_profile != 0x12 && mmc_profile != 0x43 && ssa) )
    {	fprintf (stderr,"- illegal command-line option for this media.\n");
	goto offer_options;
    }
    else if ((mmc_profile == 0x1A || mmc_profile == 0x2A) && full)
    {	fprintf (stderr,"- unimplemented command-line option for this media.\n");
	goto offer_options;
    }

#if defined(__unix) || defined(__unix__)
    /*
     * You can suspend, terminate, etc. the parent. We will keep on
     * working in background...
     */
    setsid();
    signal(SIGHUP,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
#endif

    if (compat && force)	str="relocating lead-out";
    else if (blank)		str="blanking";
    else			str="formatting";
    if (gui)			gui_action=str;
    else			fprintf (stderr,"* %s .",str);

    *progress = 1;

    // formats[i] becomes "Format Unit Parameter List"
    formats[i+0] = 0;		// "Reserved"
    formats[i+1] = 2;		// "IMMED" flag
    formats[i+2] = 0;		// "Descriptor Length" (MSB)
    formats[i+3] = 8;		// "Descriptor Length" (LSB)

    handle_events(cmd);

    if (mmc_profile==0x1A || mmc_profile==0x2A)	// DVD+RW
    {	if (compat && force && (dinfo[2]&3))
	    formats[i+4+0]=formats[i+4+1]=formats[i+4+2]=formats[i+4+3]=0,
	    formats[i+4+7] = 1;	// "Restart format"

	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData" and "Format Code"
	cmd[5] = 0;
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit (1);

	if (!compat)
	{   cmd[0] = 0x5B;	// CLOSE TRACK/SESSION
	    cmd[1] = 1;		// "IMMED" flag on
	    cmd[2] = 0;		// "Stop De-Icing"
	    cmd[9] = 0;
	    if ((err=cmd.transport()))
		sperror ("STOP DE-ICING",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);
	}

	cmd[0] = 0x5B;		// CLOSE TRACK/SESSION
	cmd[1] = 1;		// "IMMED" flag on
	cmd[2] = 2;		// "Close Session"
	cmd[9] = 0;
	if ((err=cmd.transport()))
	    sperror ("CLOSE SESSION",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit (1);
    }
    else if (mmc_profile==0x12)	// DVD-RAM
    {	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData"|"Format Code"
	cmd[5] = 0;
	formats[i+1] = 0x82;	// "FOV"|"IMMED"
	if ((formats[i+4+4]>>2) != 0x01 && !full)
	    formats[i+1] |= 0x20,// |"DCRT"
	    cmd[1] |= 0x08;	// |"CmpLst"
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit(1);
    }
    else if (mmc_profile==0x43)	// BD-RE
    {	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData"|"Format Code"
	cmd[5] = 0;
	formats[i+1] = 0x82;	// "FOV"|"IMMED"
	if (full && (formats[i+4+4]>>2)!=0x31)
	    formats[i+4+4] |= 2;// "Full Certificaton"
	else if ((formats[i+4+4]>>2)==0x30)
	    formats[i+4+4] |= 3;// "Quick Certification"
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit(1);
    }
    else if (mmc_profile==0x41)	// BD-R
    {	cmd[0] = 0x04;		// FORMAT UNIT
	cmd[1] = 0x11;		// "FmtData"|"Format Code"
	cmd[5] = 0;
	formats[i+1] = 0x2;	// IMMED"
	if ((err=cmd.transport(WRITE,formats+i,12)))
	    sperror ("FORMAT UNIT",err), exit(1);

	if (wait_for_unit (cmd,progress)) exit(1);
    }
    else			// DVD-RW
    {	page05_setup (cmd,mmc_profile);

	if (do_opc)
	{   cmd[0] = 0x54;	// SEND OPC INFORMATION
	    cmd[1] = 1;		// "Perform OPC"
	    cmd[9] = 0;
	    cmd.timeout(120);	// NEC units can be slooo...w
	    if ((err=cmd.transport()))
	    {	if (err==0x17301)	// "POWER CALIBRATION AREA ALMOST FULL"
		    fprintf (stderr,":-! WARNING: Power Calibration Area "
					"is almost full\n");
		else
		{   sperror ("PERFORM OPC",err);
		    if ((err&0x0FF00)==0x07300 && (err&0xF0000)!=0x10000)
			exit (1);
		    /* The rest of errors are ignored, see groisofs_mmc.cpp
		     * for further details... */
		}
	    }
	}

	if (blank)		// DVD-RW blanking procedure
    	{   cmd[0] = 0xA1;	// BLANK
	    cmd[1] = blank;
	    cmd[11] = 0;
	    if ((err=cmd.transport()))
		sperror ("BLANK",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);
	}
	else			// DVD-RW format
	{   if ((formats[i+4+4]>>2)==0x15)	// make it really quick
		formats[i+4+0]=formats[i+4+1]=formats[i+4+2]=formats[i+4+3]=0;

	    cmd[0] = 0x04;	// FORMAT UNIT
	    cmd[1] = 0x11;	// "FmtData" and "Format Code"
	    cmd[5] = 0;
	    if ((err=cmd.transport(WRITE,formats+i,12)))
		sperror ("FORMAT UNIT",err), exit(1);

	    if (wait_for_unit (cmd,progress)) exit (1);

	    cmd[0] = 0x35;	// FLUSH CACHE
	    cmd[9] = 0;
	    cmd.transport ();
	}
    }

    pioneer_stop (cmd,progress);

#if 0
    cmd[0] = 0x1E;	// ALLOW MEDIA REMOVAL
    cmd[5] = 0;
    if (cmd.transport ()) return 1;

    cmd[0] = 0x1B;	// START/STOP UNIT
    cmd[4] = 0x2;	// "Eject"
    cmd[5] = 0;
    if (cmd.transport()) return 1;

    cmd[0] = 0x1B;	// START/STOP UNIT
    cmd[1] = 0x1;	// "IMMED"
    cmd[4] = 0x3;	// "Load"
    cmd[5] = 0;
    cmd.transport ();
#endif

  return 0;
}
