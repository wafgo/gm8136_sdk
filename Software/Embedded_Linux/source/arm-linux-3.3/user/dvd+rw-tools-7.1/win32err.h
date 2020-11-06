# define EINVAL		ERROR_BAD_ARGUMENTS
# define ENOMEM		ERROR_OUTOFMEMORY
# define EMEDIUMTYPE	ERROR_MEDIA_INCOMPATIBLE
# define ENOMEDIUM	ERROR_MEDIA_OFFLINE
# define ENODEV		ERROR_BAD_COMMAND
# define EAGAIN		ERROR_NOT_READY
# define ENOSPC		ERROR_DISK_FULL
# define EIO		ERROR_NOT_SUPPORTED
# define ENXIO		ERROR_GEN_FAILURE
# define EBUSY		ERROR_SHARING_VIOLATION
# define ENOENT		ERROR_FILE_NOT_FOUND
# define ENOTDIR	ERROR_NO_VOLUME_LABEL
# define EINTR		ERROR_OPERATION_ABORTED
# define FATAL_START(e)	(0x10000|(e))
# define FATAL_MASK	 0x0FFFF

#ifdef errno
# undef errno
#endif

#ifdef __cplusplus
static class __win32_errno__ {
    public:
	operator int()		{ return GetLastError(); }
	int operator=(int e)	{ SetLastError(e); return e; }
} win32_errno;
# define errno		win32_errno
#else
# define errno		(GetLastError())
#endif

#define set_errno(e)	(SetLastError(e),e)

#ifdef perror
#undef perror
#endif
#define perror win32_perror
static void win32_perror (const char *str)
{ LPSTR lpMsgBuf;

    FormatMessageA( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER |
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	0, // Default language
	(LPSTR) &lpMsgBuf,
	0,
	NULL 
	);
    if (str)
	fprintf (stderr,"%s: %s",str,lpMsgBuf);
    else
	fprintf (stderr,"%s",lpMsgBuf);

    LocalFree(lpMsgBuf);
}
