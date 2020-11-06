#ifndef   _VG_VERIFY_CONFIG_H__
#define   _VG_VERIFY_CONFIG_H__

/*
 *  Set line length in configuration files
 */
#define   LINE_LEN      256

/*
 *  Define return error code value
 */
#define   ERR_NONE      0       /* read configuration file successfully */
#define   ERR_NOFILE    2       /* not find or open configuration file */
#define   ERR_READFILE  3       /* error occur in reading configuration file */
#define   ERR_FORMAT    4       /* invalid format in configuration file */
#define   ERR_NOTHING   5       /* not find section or key name in configuration file */

/*
 *  Read the value of key name in string form
 */
extern int getconfigstr(const char* section,        /* points to section name */
		                const char* keyname,        /* points to key name */
		                char* keyvalue,             /* points to destination buffer */
		                unsigned int len,           /* size of destination buffer */
		                const char* filename);      /* points to configuration filename */


/*
 *  Read the value of key name in integer form
 */
extern int getconfigint(const char* section,        /* points to section name */
		                const char* keyname,        /* points to key name */
		                int* keyvalue,              /* points to destination address */
		                const char* filename);      /* points to configuration filename */

/*
 *  Read the value of key name in unsign integer form
 */
extern int getconfiguint(const char* section,       /* points to section name */
		                 const char* keyname,       /* points to key name */
		                 unsigned int* keyvalue,    /* points to destination address */
		                 const char* filename);     /* points to configuration filename */

#endif  /* _VG_VERIFY_CONFIG_H__ */
