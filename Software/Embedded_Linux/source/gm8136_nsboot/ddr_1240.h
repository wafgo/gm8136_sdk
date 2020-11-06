#ifndef __DDR_1240_H__
#define __DDR_1240_H__

#ifdef DDR_1240

#ifdef SOCKET_EVB
#if defined(GM8136) && defined(DDR_SZ_8136_2Gbx1)

#endif
#endif  //SOCKET_EVB

#ifdef SYSTEM_EVB
#if defined(GM8136) && defined(DDR_SZ_8136_2Gbx1)
#endif
#endif  //SYSTEM_EVB

#if defined(GM8136S) && defined(DDR_SZ_8136_1Gbx1)
    unsigned int DDR_arg[][2] ={{0x00, 0x09009D02},{0x2C, 0x00000000},{0x68, 0x00000000},
                                {0x08, 0x00401A40},{0x30, 0x00000000},{0x6C, 0x00000022},
                                {0x0C, 0x00000090},{0x34, 0x06060606},{0x70, 0x00000003},
                                {0x10, 0x00004343},{0x38, 0x06060606},{0x74, 0x00000022},
                                {0x14, 0x220E0F0B},{0x3C, 0x00630012},{0x78, 0x00000000},
                                {0x18, 0x42951424},{0x40, 0x00000000},{0x7C, 0xFFFFFFFF},
                                {0x1C, 0x0000204B},{0x48, 0x000000FF},{0xA0, 0x87878787},
                                {0x20, 0x00006F01},{0x4C, 0x00000000},{0xA4, 0x87878787},
                                {0x24, 0x00000022},{0x5C, 0x00000004},{0xA8, 0x00030000},
                                {0x28, 0x000010C8},{0x60, 0x0003000E},{0xAC, 0x00062A80}};
#endif
#if defined(GM8135S) && defined(DDR_SZ_8135_512Mbx1)

#endif

#endif  //DDR_1240
#endif