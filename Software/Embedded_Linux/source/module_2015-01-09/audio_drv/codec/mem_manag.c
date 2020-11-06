/*
***********************************************************************************
** FARADAY(MTD)/GRAIN CONFIDENTIAL
** Copyright FARADAY(MTD)/GRAIN Corporation All Rights Reserved.
**
** The source code contained or described herein and all documents related to
** the source code (Material) are owned by FARADAY(MTD)/GRAIN Corporation Title to the
** Material remains with FARADAY(MTD)/GRAIN Corporation. The Material contains trade
** secrets and proprietary and confidential information of FARADAY(MTD)/GRAIN. The
** Material is protected by worldwide copyright and trade secret laws and treaty
** provisions. No part of the Material may be used, copied, reproduced, modified,
** published, uploaded, posted, transmitted, distributed, or disclosed in any way
** without FARADAY(MTD)/GRAIN prior express written permission.
**
** No license under any patent, copyright, trade secret or other intellectual
** property right is granted to or conferred upon you by disclosure or delivery
** of the Materials, either expressly, by implication, inducement, estoppel or
** otherwise. Any license under such intellectual property rights must be express
** and approved by FARADAY(MTD)/GRAIN in writing.
**
**
***********************************************************************/



/*******************************************************************************
	File:		mem_manag.c

	Content:	Memory alloc alignments functions

*******************************************************************************/


#include	"mem_manag.h"
#ifdef _MSC_VER
//#include	<stddef.h>
#else
//#include	<stdint.h>
#endif

#include <linux/slab.h>

/*****************************************************************************
*
* function name: Alignm_mem
* description:  malloc the alignments memory
* returns:      the point of the memory
*
**********************************************************************************/
void *
Alignm_mem(unsigned int size, unsigned char alignment)
{
	//int ret;
	unsigned char *mem_ptr;

	if (!alignment) {

		mem_ptr = (AAC_PTR) kmalloc(size + 1, GFP_KERNEL);
		if(mem_ptr == 0)
			return 0;
		memset(mem_ptr, 0, size + 1);
		*mem_ptr = (unsigned char)1;
		return ((void *)(mem_ptr+1));
	} else {
		unsigned char *tmp;

		mem_ptr = (AAC_PTR) kmalloc(size + alignment, GFP_KERNEL);
		if(mem_ptr == 0)
			return 0;

		tmp = (unsigned char *)mem_ptr;

		memset(tmp, 0, size + alignment);

		mem_ptr =
			(unsigned char *) ((uintptr_t) (tmp + alignment - 1) &
					(~((uintptr_t) (alignment - 1))));

		if (mem_ptr == tmp)
			mem_ptr += alignment;

		*(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp);

		return ((void *)mem_ptr);
	}

	return(0);
}


/*****************************************************************************
*
* function name: Aligmem_free
* description:  free the memory
*
*******************************************************************************/
void
Aligmem_free(void *mem_ptr)
{

	unsigned char *ptr;

	if (mem_ptr == 0)
		return;

	ptr = mem_ptr;

	ptr -= *(ptr - 1);

	kfree(ptr);
}
