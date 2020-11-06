/*+++ *******************************************************************\
*
*  Copyright and Disclaimer:
*
*     ---------------------------------------------------------------
*     This software is provided "AS IS" without warranty of any kind,
*     either expressed or implied, including but not limited to the
*     implied warranties of noninfringement, merchantability and/or
*     fitness for a particular purpose.
*     ---------------------------------------------------------------
*
*     Copyright (c) 2012 Conexant Systems, Inc.
*     All rights reserved.
*
\******************************************************************* ---*/

#ifndef __COMM_H__
#define __COMM_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef BOOL
typedef int BOOL;
#endif  /* BOOL */

#ifndef TRUE
#define TRUE    1                    /* from msdn: The bool type participates in integral promotions.*/
#define FALSE   0                    /* ... An r-value of type bool can be converted to an r-value */
#endif                               /* ... of type int, with false becoming zero and true becoming one.*/


/*******************************************************************************************************/
/* Function prototypes passed by software */
/*******************************************************************************************************/

/* type pointer to function to read byte to communication bus */
typedef unsigned int (*CXCOMM_RD)(unsigned char, unsigned short, void *, int);

/* type pointer to function to write byte to communication bus */
typedef unsigned int (*CXCOMM_WR)(unsigned i2c_addr, unsigned register_addr, int value, int size);

/* type pointer to function to burst read serial bytes to communication bus */
typedef unsigned int (*CXCOMM_BURST_RD)(unsigned i2c_addr, unsigned register_addr, void *value, int size);  ///< Jerson

/* type pointer to function to burst write serial bytes to communication bus */
typedef unsigned int (*CXCOMM_BURST_WR)(unsigned i2c_addr, unsigned register_addr, void *value, int size);  ///< Jerson

/* type pointer to function to sleep, since sleep is an os dependent function */
typedef void (*CXSLEEP)(unsigned long);

/*******************************************************************************************************/
/* CX_COMMUNICATION */
/*******************************************************************************************************/
typedef struct _cx_communication_
{
    void*           p_handle;
    unsigned char   i2c_addr;
    CXCOMM_WR       write;
    CXCOMM_RD       read;
    CXCOMM_BURST_WR burst_write;      ///< for burst write, Jerson
    CXCOMM_BURST_RD burst_read;       ///< for burst read,  Jerson
    CXSLEEP         sleep;
}   CX_COMMUNICATION;


int writeByte(const CX_COMMUNICATION *p_comm,
              const unsigned long registerAddress,
              const unsigned char value);

int readByte(const CX_COMMUNICATION *p_comm,
             const unsigned long registerAddress,
             unsigned char *p_value);

int writeDword(const CX_COMMUNICATION *p_comm,
               const unsigned long registerAddress,
               const unsigned long value);

int readDword(const CX_COMMUNICATION *p_comm,
              const unsigned long registerAddress,
              unsigned long *p_value);

int WriteBitField(const CX_COMMUNICATION *p_comm,
                  const unsigned long registerAddress,
                  const unsigned char bitEnd,
                  const unsigned char bitBegin,
                  const unsigned long bitVal);

int ReadBitField(const CX_COMMUNICATION *p_comm,
                 const unsigned long registerAddress,
                 const unsigned char bitEnd,
                 const unsigned char bitBegin,
                 unsigned long* bitVal);

int burst_writeByte(const CX_COMMUNICATION *p_comm,
                    const unsigned long registerAddress,
                    unsigned char *parray,
                    const int array_size);

int burst_readByte(const CX_COMMUNICATION *p_comm,
                   const unsigned long registerAddress,
                   unsigned char *p_value,
                   const int size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __COMM_H__ */
