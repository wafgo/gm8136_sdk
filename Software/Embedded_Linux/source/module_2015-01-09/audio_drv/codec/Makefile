#
# Makefile for GM Audio Codec
#
subdir-ccflags-y += -DARM_ASM -O2 -finline-functions

codec-y += EQ.o cfft_all.o imdct.o filterbank.o scalefactor.o is_asm.o tns_asm.o g711s.o
codec-y += aac_enc_api.o aac_main_intfac.o aac_table.o buf_manag.o gm_aacTop.o \
		huffman_1.o perceptual_cfg.o perceptual_main.o enc_mdct.o perceptual_tool.o mem_manag.o \
		quantizer_main.o quantizer_Tool.o quantizer_tool_2.o noiseless_code.o threshold_tool.o tns_tool.o \
		out_bitstrm.o oper_32b.o \
		output.o drc.o pulse.o pns.o tns.o \
		filtbank.o cfft.o mdct.o ms.o is.o common.o huffman.o decoder.o specrec.o syntax.o bits.o aac_dec_api.o \
		adpcm.o ima_adpcm_dec_api.o ima_adpcm_enc_api.o \
		g711_init.o mainap_g711.o
obj-m += codec.o
