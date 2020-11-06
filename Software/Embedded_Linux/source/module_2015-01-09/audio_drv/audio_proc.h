#ifndef __AUDIO_PROC_H_
#define __AUDIO_PROC_H_

int audio_proc_register(const char *name);
void audio_proc_unregister(void);
int audio_proc_init(void);
void audio_proc_remove(void);
void audio_drv_set_pcm_data_flag(bool is_on);
int *audio_drv_get_pcm_data_flag(void);
void audio_drv_set_pcm_save_flag(bool is_on);
int *audio_drv_get_pcm_save_flag(void);
void audio_drv_set_resample_save_flag(bool is_on);
int *audio_drv_get_resample_save_flag(void);
void audio_drv_set_play_buf_dbg(bool is_on);
int *audio_drv_get_play_buf_dbg(void);
int *audio_drv_get_nr_threshold(void);
void audio_drv_set_ecos_state_flag(bool is_on);
int *audio_drv_get_ecos_state_flag(void);
u32 *audio_flow_get_rec_stage(void);
u32 *audio_flow_get_play_stage(void);
unsigned long audio_proc_get_pcm_show_flag_paddr(void);
unsigned long audio_proc_get_play_buf_dbg_paddr(void);
unsigned long audio_proc_get_rec_dbg_stage_paddr(void);
unsigned long audio_proc_get_play_dbg_stage_paddr(void);
unsigned long audio_proc_get_nr_threshold_paddr(void);
#if defined(CONFIG_PLATFORM_GM8210)
void audio_proc_set_7500log_start_address(u32 phy_addr,u32 size);
void audio_proc_set_fc7500_log_level(unsigned int log_level);
void audio_proc_dump_fc7500_log(void);
#endif

//FIXME: The "au_codec_type_tag" must be the same as "enum audio_encode_type_t" type(defined by vg).
#endif  /* __VCAP_PROC_H_ */
