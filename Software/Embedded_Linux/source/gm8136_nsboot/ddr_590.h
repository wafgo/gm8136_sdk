#ifndef __DDR_590_H__
#define __DDR_590_H__

#ifdef DDR_590

#if defined(GM8136S) && defined(DDR_SZ_8136_1Gbx1)
    unsigned int DDR_arg[][2] ={{0x00, 0x09009D02},{0x2C, 0x00000000},{0x68, 0x00000000}, 
                                {0x08, 0x00401210},{0x30, 0x00000000},{0x6C, 0x00000022}, 
                                {0x0C, 0x00000080},{0x34, 0x06060606},{0x70, 0x00000003}, 
                                {0x10, 0x00004343},{0x38, 0x06060606},{0x74, 0x00000022}, 
                                {0x14, 0x10070705},{0x3C, 0x00610011},{0x78, 0x00000000}, 
                                {0x18, 0x31451212},{0x40, 0x00000000},{0x7C, 0xFFFFFFFF}, 
                                {0x1C, 0x00002022},{0x48, 0x000000FF},{0xA0, 0x87878787}, 
                                {0x20, 0x00006F01},{0x4C, 0x00000000},{0xA4, 0x87878787}, 
                                {0x24, 0x00000022},{0x5C, 0x00000004},{0xA8, 0x00030000}, 
                                {0x28, 0x000010C8},{0x60, 0x0003000E},{0xAC, 0x00062A80}};
#endif

#if defined(GM8135S) && defined(DDR_SZ_8135_512Mbx1)
    unsigned int DDR_arg[][2] ={{0x00, 0x09039D02},{0x2C, 0x00000000},{0x68, 0x00000000}, 
                                {0x08, 0x00020853},{0x30, 0x00000000},{0x6C, 0x00000022}, 
                                {0x0C, 0x00000000},{0x34, 0x06060606},{0x70, 0x00000003}, 
                                {0x10, 0x00001212},{0x38, 0x06060606},{0x74, 0x00000022}, 
                                {0x14, 0x0F070806},{0x3C, 0x00610011},{0x78, 0x00000033}, 
                                {0x18, 0x31450212},{0x40, 0x00000000},{0x7C, 0xFFFFFFFF}, 
                                {0x1C, 0x00002022},{0x48, 0x000000FF},{0xA0, 0x87878787}, 
                                {0x20, 0x00006F40},{0x4C, 0x00000000},{0xA4, 0x87878787}, 
                                {0x24, 0x00000000},{0x5C, 0x00000007},{0xA8, 0x00030000}, 
                                {0x28, 0x000010C8},{0x60, 0x0003000E},{0xAC, 0x00062A80}};
#endif
#endif  //DDR_590

#endif
