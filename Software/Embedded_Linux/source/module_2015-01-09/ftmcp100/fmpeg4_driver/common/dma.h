#ifndef __MDMATYPE__
#define __MDMATYPE__

#ifdef LINUX
#include "../common/portab.h"
#else
#include "portab.h"
#endif

typedef struct MDMATag
{
  uint32_t SMaddr;		// 00
  uint32_t LMaddr;		// 04
  uint32_t BlkWidth;		// 08
  uint32_t Control;		// 0C
  uint32_t CCA;			// 10
  uint32_t Status;		// 14
  uint32_t CCA2;		// 18	new feature from 2004 0507
  uint32_t GRPC;		// 1C	Group execution control
  uint32_t GRPS;		// 20	Group sync control
  uint32_t ABFCtrl;     // 24   Auto-buffer control
  //uint32_t THRESHOLD;   // 28   DMA threshold value
  //uint32_t AUTOINT;     // 2C   auto send interrupt even last command is skipped or disabled
} MDMA;


#endif
