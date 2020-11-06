/**
 * @file audio_vg.c
 *  audio vg
 * Copyright (C) 2010 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.189 $
 * $Date: 2015/01/08 03:43:47 $
 *
 * ChangeLog:
 *  $Log: audio_vg.c,v $
 *  Revision 1.189  2015/01/08 03:43:47  andy_hsu
 *  fix compile error
 *
 *  Revision 1.188  2015/01/08 03:15:12  andy_hsu
 *  1.record/playback with different SSP will failed
 *  2.support record/playback with different sample rate
 *  3.add stereo GM8138 & GM8137 support
 *  4.change bitrate variable size
 *
 *  Revision 1.187  2015/01/07 03:44:12  harry_hs
 *  fix remove/insert bug
 *
 *  Revision 1.186  2015/01/07 03:02:55  andy_hsu
 *  add flush_workqueue before destroy
 *
 *  Revision 1.185  2015/01/05 08:13:11  andy_hsu
 *  print audio driver version on console when booting
 *
 *  Revision 1.184  2015/01/05 07:48:12  andy_hsu
 *  fix workqueue not remove when driver exit
 *
 *  Revision 1.183  2014/12/26 03:40:32  andy_hsu
 *  fix audio driver memory leak
 *
 *  Revision 1.182  2014/12/22 07:43:23  andy_hsu
 *  sync DH patch and fis known issues
 *
 *  Revision 1.181  2014/12/03 07:57:53  andy_hsu
 *  1. fix g711 encode issue
 *  2. remove cpu_comm send/receive timeout message, replace with GM_LOG
 *
 *  Revision 1.180  2014/12/01 09:26:25  andy_hsu
 *  fix issues with FC7500
 *
 *  Revision 1.179  2014/11/06 13:41:32  andy_hsu
 *  add null pointer check
 *
 *  Revision 1.178  2014/11/06 03:11:32  andy_hsu
 *  add timeout job handle
 *
 *  Revision 1.177  2014/10/31 09:54:36  andy_hsu
 *  modify audio time stamp mechanism
 *
 *  Revision 1.176  2014/10/23 01:13:33  andy_hsu
 *  add 828x SSP2 resample function
 *
 *  Revision 1.175  2014/10/22 07:28:59  andy_hsu
 *  fix remove audio driver caused crash issue
 *
 *  Revision 1.174  2014/10/21 08:51:50  andy_hsu
 *  add fc7500 state debug tool
 *
 *  Revision 1.173  2014/10/20 12:27:23  andy_hsu
 *  fix issue; variable audio_in_enable caused FC7500 panic
 *
 *  Revision 1.172  2014/10/20 01:37:11  andy_hsu
 *  add function for detect nChannels changed when AAC decode
 *
 *  Revision 1.171  2014/10/15 02:28:00  andy_hsu
 *  add audio_in_enable parameter for au_grab channel count check
 *
 *  Revision 1.170  2014/10/14 12:50:40  andy_hsu
 *  remove error restriction on GM8136
 *
 *  Revision 1.169  2014/10/08 08:10:07  andy_hsu
 *  add out_enable check for counting record channel on GM8136
 *
 *  Revision 1.168  2014/09/19 12:21:41  andy_hsu
 *  FC7500 eCos Image version change
 *
 *  Revision 1.167  2014/09/16 18:38:51  andy_hsu
 *  fix error
 *
 *  Revision 1.166  2014/09/16 17:43:09  andy_hsu
 *  add bypass FC7500 function
 *
 *  Revision 1.165  2014/09/16 07:00:20  andy_hsu
 *  modify eCos version for duplicate-sound issue(r/w index mechanism)
 *
 *  Revision 1.164  2014/08/21 02:37:16  andy_hsu
 *  add GM8136 support
 *
 *  Revision 1.163  2014/08/19 05:29:59  andy_hsu
 *  modify eCos version
 *
 *  Revision 1.162  2014/08/14 06:45:31  easonli
 *  *:[audio] support the dual playback mode
 *
 *  Revision 1.161  2014/07/25 03:33:37  easonli
 *  *:[audio] move frame count from codec/audio_main.h to audio_vg.c
 *
 *  Revision 1.160  2014/07/18 09:24:52  easonli
 *  *:[audio] update audio driver version
 *
 *  Revision 1.159  2014/07/18 05:18:05  easonli
 *  *:[audio] modify code style
 *
 *  Revision 1.158  2014/07/17 05:45:45  easonli
 *  *:[audio] remove useless definition and code
 *
 *  Revision 1.157  2014/07/16 01:36:53  easonli
 *  *:[audio] 1. fixed memcpy issue in audio_vg.c 2. add ssp1 support for GM8139 3. fixed live sound chan num.
 *
 *  Revision 1.156  2014/07/10 06:53:35  easonli
 *  *:[audio] rename of some functions, fixed memcpy bug when cpu_comm timeout, enable ssp1 rx in GM8139
 *
 *  Revision 1.155  2014/06/20 08:29:36  schumy_c
 *  Add log for job buffer.
 *
 *  Revision 1.154  2014/06/05 10:03:14  easonli
 *  *:[audio] fixed ssp lose bug
 *
 *  Revision 1.153  2014/06/05 03:53:11  easonli
 *  *:[audio] fixed record channel bugs
 *
 *  Revision 1.152  2014/06/04 09:59:01  easonli
 *  *:[audio] fixed remove module bug
 *
 *  Revision 1.151  2014/06/03 09:46:03  easonli
 *  *:[audio] add adda160 for recording
 *
 *  Revision 1.150  2014/05/26 08:02:58  easonli
 *  *:[audio] fixed 2 different audio sources record in GM8139, and add save pcm, resample flag
 *
 *  Revision 1.149  2014/05/23 07:54:54  easonli
 *  *:[audio] fixed frame sample entity
 *
 *  Revision 1.148  2014/05/21 13:30:00  easonli
 *  *:[audio] update version of eCOS
 *
 *  Revision 1.147  2014/05/21 08:38:23  easonli
 *  *:[audio] upgrade eCos version
 *
 *  Revision 1.146  2014/05/13 03:59:40  schumy_c
 *  Add property message for debug.
 *
 *  Revision 1.145  2014/05/11 03:52:15  easonli
 *  *:[audio] replace __cancel_delayed_work() by cancel_delayed_work_sync()
 *
 *  Revision 1.144  2014/05/07 08:23:55  easonli
 *  *:[audio] increase driver version
 *
 *  Revision 1.143  2014/04/24 09:59:15  easonli
 *  *:[audio] add live path for EP GM8210
 *
 *  Revision 1.142  2014/04/23 10:37:18  easonli
 *  *:[audio] modify vch check for set_live_sound()
 *
 *  Revision 1.141  2014/04/23 09:54:45  easonli
 *  *:[audio] restrict audio_drv.ko on fa626
 *
 *  Revision 1.140  2014/04/17 10:09:07  ivan
 *  panic if version mismatch
 *
 *  Revision 1.139  2014/04/16 05:30:36  harry_hs
 *  eason add
 *
 *  Revision 1.138  2014/04/16 04:57:25  harry_hs
 *  eason version
 *
 *  Revision 1.137  2014/04/11 07:40:03  easonli
 *  *:[audio] upadate ecos version
 *
 *  Revision 1.136  2014/04/09 06:04:48  easonli
 *  *:[audio] remove Schumy's FIXME
 *
 *  Revision 1.135  2014/04/08 23:09:53  easonli
 *  *:[audio] fixed wrong usage of delayed work
 *
 *  Revision 1.134  2014/04/07 09:31:01  klchu
 *  use get_gm_jiffies() when HZ=100, compatible with old kernel.
 *
 *  Revision 1.133  2014/04/07 06:26:38  schumy_c
 *  Add g.711 a-law/u-law.
 *
 *  Revision 1.132  2014/04/03 09:26:39  easonli
 *  *:[audio] add AULAW frame size for Schumy
 *
 *  Revision 1.131  2014/04/03 08:58:09  easonli
 *  *:[audio] fixed compiling error bcuz GM828x and GM813x don't use delay work
 *
 *  Revision 1.130  2014/04/03 08:52:55  easonli
 *  *:[audio] 1. replace timer by delayed work queue bcuz mutex can't be used in soft irq context.
 *            2. add nr_threshold control
 *
 *  Revision 1.129  2014/03/25 10:55:50  easonli
 *  *:[audio] 1. Dynamic request DMA channel (add FIXED_DMA_CHAN flag) 2. Put overrun and underrun info to /proc/audio 3. Add DMA channel request failed count to /proc/audio
 *
 *  Revision 1.128  2014/03/19 10:01:28  easonli
 *  *:[audio] update av sync parameter by Schumy
 *
 *  Revision 1.127  2014/03/18 16:39:57  easonli
 *  *:[audio] fixed compiling error for eCos version check
 *
 *  Revision 1.126  2014/03/18 16:38:19  easonli
 *  *:[audio] add version check from 7500 and modify av sync time for Schumy's request
 *
 *  Revision 1.125  2014/03/18 14:05:44  harry_hs
 *  remove memset for live job
 *
 *  Revision 1.124  2014/03/18 08:41:48  schumy_c
 *  Turn on resample(speed-up) when a-v latency is too large.
 *
 *  Revision 1.123  2014/03/18 06:30:57  schumy_c
 *  Turn off resample(speed-up) when a-v latency is too large.
 *
 *  Revision 1.122  2014/03/18 02:18:32  easonli
 *  *:[audio] add initialization for LIVESOUND_ENGINE_NUM
 *
 *  Revision 1.121  2014/03/17 09:23:00  easonli
 *  *:[audio] fixed compiling error
 *
 *  Revision 1.120  2014/03/17 08:56:21  easonli
 *  *:[audio] fixed bug of send job to fc7500 when it's died
 *
 *  Revision 1.119  2014/03/14 08:55:50  easonli
 *  *:[audio] fixed compiled warning
 *
 *  Revision 1.118  2014/03/14 06:18:54  harry_hs
 *  remove mutex while error
 *
 *  Revision 1.117  2014/03/14 05:56:57  harry_hs
 *  add more debug message
 *
 *  Revision 1.115  2014/03/14 02:30:55  harry_hs
 *  add -ERESTARTSYS check
 *
 *  Revision 1.114  2014/03/12 08:19:54  easonli
 *  *:[audio] remove useless printk
 *
 *  Revision 1.113  2014/03/09 15:23:59  easonli
 *  *:[audio] add cpu_comm failed count and fixed re-enter set_live_sound()
 *
 *  Revision 1.112  2014/03/07 08:07:26  easonli
 *  *:[audio] remove dummy error message
 *
 *  Revision 1.111  2014/03/07 07:53:32  easonli
 *  *:[audio] add more error handling for GM8210
 *
 *  Revision 1.110  2014/03/07 06:06:35  easonli
 *  *:[audio] add error recovery for set_live_sound()
 *
 *  Revision 1.109  2014/03/05 03:39:28  easonli
 *  *:[audio] fixed audio noise
 *
 *  Revision 1.108  2014/03/04 07:42:43  easonli
 *  *:[audio] 1. fixed bug when no job but set_live_sound() for FC7500 2. add playback + live sound at same time
 *
 *  Revision 1.107  2014/03/04 02:58:06  easonli
 *  *:[audio] fixed set_live_sound bug
 *
 *  Revision 1.106  2014/03/03 19:13:24  easonli
 *  *:[audio] remove dummy code and update ecos version
 *
 *  Revision 1.105  2014/03/03 13:31:58  easonli
 *  *:[audio] add fault tolerance
 *
 *  Revision 1.104  2014/02/25 09:16:17  easonli
 *  *:[audio] modify corresponding ecos version
 *
 *  Revision 1.103  2014/02/25 09:13:34  easonli
 *  *:[audio] add 2 path of live sound with multi-chan record and debug control in proc
 *
 *  Revision 1.102  2014/02/24 12:07:34  easonli
 *  *:[audio] remove useless function
 *
 *  Revision 1.101  2014/02/24 11:11:25  easonli
 *  *:[audio] add record + live sound at the same time and merge harry bug fixed version
 *
 *  Revision 1.100  2014/02/24 07:11:50  schumy_c
 *  Update default value to avoid unexpect flow.
 *
 *  Revision 1.99  2014/02/15 12:33:52  easonli
 *  *:[audio] fixed error job flow
 *
 *  Revision 1.98  2014/02/13 03:22:24  easonli
 *  *:[audio] add live sound from different i2s
 *
 *  Revision 1.97  2014/02/10 17:54:52  easonli
 *  *:[audio] remove useless variable
 *
 *  Revision 1.96  2014/02/10 17:36:18  easonli
 *  *:[audio] update eCos version
 *
 *  Revision 1.95  2014/02/10 16:11:55  easonli
 *  *:[audio] pass audio_out_enable[] to lower driver for saving DMA channels
 *
 *  Revision 1.94  2014/02/10 15:01:39  easonli
 *  *:[audio] add job entity check in driver_putjob() and fixed offset ssp grab bug in ssp_dma.c
 *
 *  Revision 1.93  2014/02/06 09:12:47  easonli
 *  *:[audio] add pci_id check
 *
 *  Revision 1.92  2014/01/27 05:53:17  easonli
 *  *:[audio] add eCos version string
 *
 *  Revision 1.91  2014/01/26 17:07:57  easonli
 *  *:[audio] no use of (total_ssp_cnt - 1) as HDMI interface, it's dangerous
 *
 *  Revision 1.90  2014/01/24 14:43:19  easonli
 *  *:[audio] digital_codec_name is unused now. open it when being used
 *
 *  Revision 1.89  2014/01/24 13:40:41  easonli
 *  *:[audio] modified audio driver behavior and add getting channel data by offset
 *
 *  Revision 1.88  2014/01/16 10:40:07  easonli
 *  *:[audio] add digital_codec_name
 *
 *  Revision 1.87  2014/01/14 17:40:54  easonli
 *  *:[audio] add stop command for discarding buffered audio data
 *
 *  Revision 1.86  2014/01/13 05:30:25  easonli
 *  *:[audio] rename tw2968 to tw2964
 *
 *  Revision 1.85  2014/01/10 08:30:43  easonli
 *  *:[audio] fixed crash and improve resample performance
 *
 *  Revision 1.84  2014/01/09 07:59:10  easonli
 *  *:[audio] fixed cacheable dma issue
 *
 *  Revision 1.83  2014/01/08 14:02:33  easonli
 *  *:[audio] modify HDMI sample rate for avoiding playback buffer underrun
 *
 *  Revision 1.82  2014/01/08 12:16:19  easonli
 *  *:[audio] add new platform
 *
 *  Revision 1.81  2013/12/31 05:43:50  easonli
 *  *:[audio] add support of audio channels not equal to video channels and fixed bugs
 *
 *  Revision 1.80  2013/12/17 07:47:59  easonli
 *  *:[audio] add dma memcpy mode for single audio channel copy
 *
 *  Revision 1.79  2013/12/13 06:11:33  schumy_c
 *  Update log message and disable sync_speed_change feature as default.
 *
 *  Revision 1.78  2013/12/12 07:01:42  schumy_c
 *  Add switch for sync_speed_change.
 *
 *  Revision 1.77  2013/12/12 03:19:41  schumy_c
 *  Disable all case of resample(for sync) when user disable buffer chase.
 *
 *  Revision 1.76  2013/12/11 03:39:49  easonli
 *  *:[audio] add GM8139 live sound and fixed codec warning
 *
 *  Revision 1.75  2013/12/10 04:01:05  schumy_c
 *  Fix bug. check for all bits when job is finished.
 *
 *  Revision 1.74  2013/12/06 07:17:23  easonli
 *  *:[audio] add settings of GM8139
 *
 *  Revision 1.73  2013/12/04 18:50:31  easonli
 *  *:[audio] fixed audio playback issue and add live sound for tv decoder on GM8287
 *
 *  Revision 1.72  2013/12/03 16:40:58  easonli
 *  *:[audio] fixed GM8287 hdmi playback and live sound bugs
 *
 *  Revision 1.71  2013/12/03 08:15:06  schumy_c
 *  Fix wrong count for buffer chase in proc log.
 *
 *  Revision 1.70  2013/11/29 07:26:05  schumy_c
 *  Fix bug for av-sync issue.
 *
 *  Revision 1.69  2013/11/26 10:02:03  easonli
 *  *:[audio] add module parameters and set ssp paramters
 *
 *  Revision 1.68  2013/11/18 08:39:32  easonli
 *  *:[audio] add plat_audio_get_codec_name() for checking codec
 *
 *  Revision 1.67  2013/11/18 05:52:36  schumy_c
 *  Get video latency to control audio buffer.
 *
 *  Revision 1.66  2013/11/13 10:14:36  schumy_c
 *  Add buffer chase feature.
 *
 *  Revision 1.65  2013/11/13 09:19:33  easonli
 *  *:[audio] add get codec name function and modified audio job member for av sync
 *
 *  Revision 1.64  2013/11/12 09:47:59  schumy_c
 *  Move internal variables from proc to vg file.
 *
 *  Revision 1.63  2013/11/11 08:38:22  schumy_c
 *  Add proc node for debug.
 *
 *  Revision 1.62  2013/11/11 03:05:04  schumy_c
 *  Support frame_sample property for pcm.
 *
 *  Revision 1.61  2013/11/01 07:03:11  easonli
 *  *:[audio] remove dummy code and fixed GM8287 playback bug
 *
 *  Revision 1.60  2013/10/28 07:58:05  easonli
 *  *:[audio] add mute buffer method for audio dma buffer
 *
 *  Revision 1.59  2013/10/25 06:02:58  schumy_c
 *  To queue buffer, resample for the first job and the job delayed for specified duration.
 *
 *  Revision 1.58  2013/10/24 07:48:21  schumy_c
 *  Add proc for setting queued buffer and time-adjust.
 *  For record, add compensation by queue buffer and time-adjust values.
 *  For playback, resample the first job by queue buffer value.
 *
 *  Revision 1.57  2013/10/23 08:40:22  easonli
 *  *:[audio] fixed bug when GM8210 only do playback
 *
 *  Revision 1.56  2013/10/22 09:56:55  easonli
 *  *:[audio] fixed compiling error
 *
 *  Revision 1.55  2013/10/20 16:00:24  easonli
 *  *:[audio] change flow for single CPU
 *
 *  Revision 1.54  2013/10/16 10:33:16  easonli
 *  *:[audio] fixed bug of auto_mode_chk_audio_type() by passing physical address to it
 *
 *  Revision 1.53  2013/10/15 08:55:23  schumy_c
 *  Set root_time for encode case.
 *
 *  Revision 1.52  2013/10/15 05:36:55  easonli
 *  *:[audio] modify iterator type of auto_mode_chk_audio_type() to prevent memory error
 *
 *  Revision 1.51  2013/10/14 09:25:15  easonli
 *  *:[audio] add protection of auto_mode_chk_audio_type()
 *
 *  Revision 1.50  2013/10/09 06:00:18  easonli
 *  *:[audio] use list_for_each_entry_safe() to replace list_for_each_entry()
 *
 *  Revision 1.49  2013/10/08 09:02:41  easonli
 *  *:[audio] fixed set_live_sound() job error
 *
 *  Revision 1.48  2013/10/08 07:46:10  schumy_c
 *  Fix build error.
 *
 *  Revision 1.47  2013/10/07 08:36:19  easonli
 *  *:[audio] add version control
 *
 *  Revision 1.46  2013/10/07 03:49:31  easonli
 *  *:[audio] add protection of set live audio
 *
 *  Revision 1.45  2013/10/04 10:02:56  easonli
 *  *:[audio] fixed ssp_dma bug and modified audio process flow
 *
 *  Revision 1.44  2013/10/04 07:23:02  schumy_c
 *  Add latency compensation.
 *
 *  Revision 1.43  2013/10/03 06:42:56  schumy_c
 *  Fix bug for resample.
 *
 *  Revision 1.42  2013/10/03 04:08:33  easonli
 *  *:[audio] add default value and count HDMI resample ratio
 *
 *  Revision 1.41  2013/09/30 02:40:43  easonli
 *  *:[audio] seperate record and playback to different channel
 *
 *  Revision 1.40  2013/09/27 09:04:25  easonli
 *  *:[audio] seperate cpu_comm for two channels to improve performance
 *
 *  Revision 1.39  2013/09/26 09:46:13  easonli
 *  *:[audio] modified jiffies_addr to jiffies_fa626, and no need to pass physical address to FC7500 bcuz this work is done in tick ISR
 *
 *  Revision 1.38  2013/09/24 09:57:21  schumy_c
 *  Update speed change level.
 *
 *  Revision 1.37  2013/09/17 09:52:32  schumy_c
 *  Get jiffies time by 7500.
 *
 *  Revision 1.36  2013/09/17 08:36:49  easonli
 *  *:[audio] modify slice_type to jiffies_addr
 *
 *  Revision 1.35  2013/09/05 10:08:24  easonli
 *  *:[audio_drv] add auto check function
 *
 *  Revision 1.34  2013/09/04 09:37:58  easonli
 *  *:[aduio_drv] add audio flow for single processor
 *
 *  Revision 1.33  2013/08/29 06:27:45  schumy_c
 *  Support hdmi resample.
 *  Add av-sync code.
 *
 *  Revision 1.32  2013/08/26 09:38:55  easonli
 *  *:[audio] in set_live_sound(), use ssp_idx not bitmap
 *
 *  Revision 1.31  2013/08/26 09:19:36  easonli
 *  *:[audio] add ssp_idx for audio job to do live sound
 *
 *  Revision 1.30  2013/08/23 10:03:05  easonli
 *  *:[audio] add hook function for front_end decoder
 *
 *  Revision 1.29  2013/08/20 03:44:58  easonli
 *  *:[audio] fixed live sound bug
 *
 *  Revision 1.28  2013/08/20 03:11:45  schumy_c
 *  Set root time before processing the audio job.
 *
 *  Revision 1.27  2013/08/15 09:53:49  easonli
 *  *:[audio] add set nvp1918 code for live sound
 *
 *  Revision 1.26  2013/08/14 13:31:12  easonli
 *  *:[audio] add audio live
 *
 *  Revision 1.25  2013/08/06 07:48:56  easonli
 *  *:[audio] roll back code for releasing to customer
 *
 *  Revision 1.24  2013/08/06 06:41:03  easonli
 *  *:[audio] temporarily add new feature
 *
 *  Revision 1.23  2013/07/31 09:19:51  schumy_c
 *  Add workaround for setting enc_type twice.
 *
 *  Revision 1.22  2013/07/29 08:52:53  easonli
 *  *:[audio] modify member name
 *
 *  Revision 1.21  2013/07/18 10:20:16  easonli
 *  *:[audio] add more info for job dump
 *
 *  Revision 1.20  2013/07/18 06:16:21  easonli
 *  *:[audio] modify audio_job's buffer size and remove useless file
 *
 *  Revision 1.19  2013/07/16 05:42:58  schumy_c
 *  Fix bug.
 *
 *  Revision 1.18  2013/07/11 10:08:23  schumy_c
 *  set block mode when calling cpu_comm recv/send.
 *
 *  Revision 1.17  2013/07/09 10:50:29  schumy_c
 *  Get block_size/block_count property.
 *
 *  Revision 1.16  2013/07/09 06:47:27  easonli
 *  *:[audio] modified audio buffer size code
 *
 *  Revision 1.15  2013/07/05 07:09:10  easonli
 *  *:[audio] add enc_blk_cnt and dec_blk_sz for use
 *
 *  Revision 1.14  2013/07/04 11:40:34  easonli
 *  *:[audio] add buffer modification for AAC
 *
 *  Revision 1.13  2013/07/04 11:33:07  easonli
 *  *:[audio] add buffer modification for IMA-ADPCM
 *
 *  Revision 1.12  2013/07/04 10:58:42  easonli
 *  *:[audio] rename enc_type to codec_type
 *
 *  Revision 1.11  2013/07/03 15:17:08  easonli
 *  *:[audio] redefine AUDIO_SSP_CHAN to AUDIO_MAX_CHAN
 *
 *  Revision 1.10  2013/07/03 09:55:19  easonli
 *  *:[audio] fixed audio_job data is not sync
 *
 *  Revision 1.9  2013/07/03 07:58:49  schumy_c
 *  Update identifier offset.
 *
 *  Revision 1.8  2013/06/24 09:26:51  schumy_c
 *  Update log message and format.
 *
 *  Revision 1.7  2013/06/20 10:11:30  schumy_c
 *  Fix audio flow.
 *
 *  Revision 1.6  2013/06/20 04:16:19  easonli
 *  *:[audio] add JOB_STS_ERROR etc. for error handling
 *
 *  Revision 1.5  2013/06/19 07:00:46  schumy_c
 *  Reassign address for each audio channel.
 *
 *  Revision 1.4  2013/06/18 09:50:15  easonli
 *  *:[audio_drv] modify cpu_comm channel base
 *
 *  Revision 1.3  2013/06/10 17:26:39  easonli
 *  *:[audio] fixed audio_job bug in rcv_handler
 *
 *  Revision 1.2  2013/06/10 10:25:26  schumy_c
 *  Modify makefile and fix bug.
 *
 *  Revision 1.1  2013/06/10 07:48:59  easonli
 *  *:[audio] audio entity driver for gm_graph
 *
 *  Revision 1.11  2013/06/10 07:38:58  easonli
 *  *:[audio] add receive thread
 *
 *  Revision 1.10  2013/05/14 06:20:33  easonli
 *  *:[audio] new version of audio vg
 *
 *  Revision 1.9  2013/02/21 02:14:59  waynewei
 *  1.fixed for multi-channel property info in LCD's property
 *  2.fixed for arm-linux-3.3
 *  3.fixed for audio path issue
 *
 *  Revision 1.8  2013/02/18 10:13:39  schumy_c
 *  Add fake driver info for vg.
 *
 *  Revision 1.7  2013/01/24 11:11:57  schumy_c
 *  Add more recognized property for audio.
 *
 *  Revision 1.6  2013/01/24 07:37:55  schumy_c
 *  Modify fake audio pattern and send bs offset to next entity.
 *
 *  Revision 1.5  2013/01/24 03:38:37  schumy_c
 *  Change name from "channels" to "channel_type" for audio.
 *
 *  Revision 1.4  2013/01/22 11:10:46  schumy_c
 *  Integrate audio feature.
 *
 *  Revision 1.3  2012/11/13 02:01:55  waynewei
 *  Fixed for confilcted declaration of proc of audio  with vcap.
 *
 *  Revision 1.2  2012/11/07 10:01:05  ivan
 *  fixup audio compiler error
 *
 *  Revision 1.1  2012/11/07 09:50:56  ivan
 *  add new audio
 *
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/mempool.h>
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>

#include "log.h"    //include log system "printm","damnit"...
#include "audio_vg.h"
#include "audio_flow.h"
#include "audio_proc.h"
#include "../front_end/platform.h"

#if defined(CONFIG_PLATFORM_GM8210)
#include <mach/fmem.h>
#endif

#if (HZ == 1000)
    #define get_gm_jiffies()                (jiffies)
#else
    #include <mach/gm_jiffies.h>
#endif
#define ENTITY_ENGINES  2
#define ENTITY_MINORS   0x7f
#define COMM_NAME       "AU_ENTITY"
#define VG_ERROR        "(VG JOB error)"
#define COMM_TIMEOUT    2000    // 2000 ms
#define RCV_TIMEOUT     4000    // 4000 ms

#define JIFFIES_DIFF(t2, t1)    ((long)(t2) - (long)(t1))   /* t1 is prior to t2 */
#define PLAYBACK_FILE_PATH      "/mnt/nfs/origin.pcm"

#ifdef CONFIG_FC7500
#define FC7500_ECOS_VERSION         "2015_0108_1051" //"2014_1219_1438"
#define VERSION_LEN                 15

#define CPUCOMM_RECORD_CH       CHAN_1_USR0
#define CPUCOMM_PLAYBACK_CH     CHAN_1_USR1
#define CPUCOMM_RECEIVE_CH      CPUCOMM_RECORD_CH
#define CPUCOMM_LIVE_CH         CPUCOMM_RECORD_CH
#define CPUCOMM_HEARTBEAT_CH    CHAN_1_USR2
#endif /* CONFIG_FC7500 */

#define FA626_AU_DRV_MINOR_VERSION   "0108_1051" //"0108_0934"

/* platform spec global parameters */
static audio_param_t au_param;
static int total_rec_chan = 0; /* total_rec_chan means maximum record channel count */
static int total_ssp_cnt = 0; /* total number of used SSP */
static int audio_ssp_chan[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(audio_ssp_chan, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_ssp_chan, "audio channel count for each audio interface");
static uint bit_clock[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(bit_clock, uint, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bit_clock, "bit clock for each audio interface");
static uint sample_rate[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(sample_rate, uint, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sample_rate, "sample rate of for each audio interface");
static int audio_ssp_num[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(audio_ssp_num, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_ssp_num, "SSP number for each audio interface");
static int audio_out_enable[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(audio_out_enable, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_out_enable, "is output enable for each audio interface");
static int audio_in_enable[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(audio_in_enable, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_in_enable, "is input enable for each audio interface");
static int audio_nr_enable[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
module_param_array(audio_nr_enable, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_nr_enable, "is noise reduction enable for each audio interface");
static int audio_is_stereo[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 1};
module_param_array(audio_is_stereo, int, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(audio_is_stereo, "is stereo on for each audio interface");

int au_get_max_chan(void) {return total_rec_chan;}
EXPORT_SYMBOL(au_get_max_chan);
int *au_get_ssp_chan(void) {return audio_ssp_chan;}
EXPORT_SYMBOL(au_get_ssp_chan);
uint *au_get_bit_clock(void) {return bit_clock;}
EXPORT_SYMBOL(au_get_bit_clock);
uint *au_get_sample_rate(void) {return sample_rate;}
EXPORT_SYMBOL(au_get_sample_rate);
int *au_get_ssp_num(void) {return audio_ssp_num;}
EXPORT_SYMBOL(au_get_ssp_num);
int *au_get_out_enable(void) {return audio_out_enable;}
EXPORT_SYMBOL(au_get_out_enable);
int au_get_ssp_max(void) {return total_ssp_cnt;}
EXPORT_SYMBOL(au_get_ssp_max);
audio_param_t *au_get_param(void) {return &au_param;}
EXPORT_SYMBOL(au_get_param);
int *au_get_is_stereo(void) {return audio_is_stereo;}
EXPORT_SYMBOL(au_get_is_stereo);
int *au_get_in_enable(void) {return audio_in_enable;}
EXPORT_SYMBOL(au_get_in_enable);


#ifdef CONFIG_FC7500
static int fc7500_stuck = 0;
#endif
static void mark_engine_finish(void *job_param);
static int driver_postprocess(void *parm, void *priv);

enum {
    RECORD_ENGINE_NUM = 0,
    PLAYBACK_ENGINE_NUM,
    LIVESOUND_ENGINE_NUM,
};

// for timeout job handle
#define HANDLE_TIMEOUT_JOB
//#undef HANDLE_TIMEOUT_JOB
#if defined(CONFIG_FC7500) && defined(HANDLE_TIMEOUT_JOB)
#define MAX_CB_LOG_CNT              32
#define MAX_CB_LOG_MASK             (MAX_CB_LOG_CNT - 1)
#define MAX_JOB_TIMEOUT_LIMIT       1 //20
#define MAX_JID_LOG_ENGINE          2
#define rec_jid_log                 jid_log[RECORD_ENGINE_NUM]
#define pb_jid_log                  jid_log[PLAYBACK_ENGINE_NUM]
#define AU_REC_JOB_LOG_SEMA_LOCK    down(&jid_log[RECORD_ENGINE_NUM].sema_lock)
#define AU_REC_JOB_LOG_SEMA_UNLOCK  up(&jid_log[RECORD_ENGINE_NUM].sema_lock)
#define AU_PB_JOB_LOG_SEMA_LOCK     down(&jid_log[PLAYBACK_ENGINE_NUM].sema_lock)
#define AU_PB_JOB_LOG_SEMA_UNLOCK   up(&jid_log[PLAYBACK_ENGINE_NUM].sema_lock)
enum {
    RECORD_JOB = 1,
    PLAYBACK_JOB = 2
};
struct jid_log_info {
    u32 callbacked_seq_num[MAX_CB_LOG_CNT];
    u32 hit_cntr; // how many times the received job has been callbacked?
    u32 job_timeout_cntr;
    u32 reset_cntr;
    struct semaphore sema_lock;
};
static struct jid_log_info jid_log[MAX_JID_LOG_ENGINE]; //RECORD_ENGINE_NUM:0, PLAYBACK_ENGINE_NUM:1 
static u32 record_seq_num = 0, playback_seq_num = 0; 
#endif

#define IGNORE_7500
//#undef IGNORE_7500

extern unsigned int ecos_log_level;

#if defined(CONFIG_FC7500)
static unsigned int record_timeout = 0, playback_timeout = 0;
#endif

audio_register_t audio_register;

/*
 * generate file, return value: >= 0 means success, others means failed
 */
int audio_write_file(char *filename, void *data, size_t count, unsigned long long *offset)
{
	mm_segment_t fs;
	struct file *filp = NULL;
    int ret = 0;

	/* get current->addr_limit. It should be 0-3G */
	fs = get_fs();
	/* set current->addr_limit to 4G */
	set_fs(get_ds());

	filp = filp_open(filename, O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);

	if (IS_ERR(filp)) {
        ret = PTR_ERR(filp);
	    set_fs(fs);
		printk("%s fails to open file %s(errno = %d)\n", __func__, filename, ret);
        ret = -1;
        goto exit;
	}

    ret = vfs_write(filp, (const char __user *)data, count, offset);

    if (ret <= 0) {
        printk("%s: Fail to write file %s(errno = %d)!\n", __func__, filename, ret);
        ret = -1;
        goto exit;
    }

exit:
    if (!IS_ERR(filp))
	    filp_close(filp, NULL);

	/* restore the addr_limit */
	set_fs(fs);

	return ret;
}
EXPORT_SYMBOL(audio_write_file);

static void do_engine(job_item_t *job_item);

static audio_engine_t   audio_engine[ENTITY_ENGINES + 1]; /* plus one for live sound control */
static struct kmem_cache *job_item_cache = NULL;
#ifdef CONFIG_FC7500
static unsigned int heart_beat_cnt = 0, heart_beat_fail = 0;
#endif

/* Audio Job management:
   1. record path: single job for multiple audio channels,
      all job belongs to one engine(0) list
   2. playback path: single job for one audio channel,
      all jobs belongs to one engine(1) list
*/

/* property */
struct video_entity_t *driver_entity;

enum property_id {
    ID_START = (MAX_WELL_KNOWN_ID_NUM + 1),
    ID_SPEED_CHANGE_LEVEL,
    ID_VIDEO_LATENCY,
    ID_MAX
};

struct property_map_t property_map[] = {
    {ID_SPEED_CHANGE_LEVEL, "speed_change_level", "speed change level"},
    {ID_VIDEO_LATENCY, "video_latency", "video latency"},
    {ID_AU_CHANNEL_BMP, "chan_bitmap", "audio channel bitmap"},
    {ID_BITRATE, "bitrate", "audio bitrate"},
    {ID_AU_RESAMPLE_RATIO, "resample_ratio", "audio resample ratio"},
    {ID_AU_SAMPLE_RATE, "sample_rate", "audio sampe rate"},
    {ID_AU_SAMPLE_SIZE, "sample_size", "audio sampe size"},
    {ID_AU_CHANNEL_TYPE, "channel_type", "audio channel type"},
    {ID_AU_ENC_TYPE, "codec_type", "audio encode type"},
    {ID_AU_BLOCK_SIZE, "block_size", "block size"},
    {ID_AU_BLOCK_COUNT, "block_count", "block count"},
    {ID_AU_FRAME_SAMPLES, "frame_samples", "frame samples"},
    {ID_BITSTREAM_SIZE, "bitstream_size", "audio bitstream size"},
    {ID_AU_DATA_LENGTH_0, "data_length[0]", "audio channel data length[0]"},
    {ID_AU_DATA_LENGTH_1, "data_length[1]", "audio channel data length[1]"},
    {ID_AU_DATA_OFFSET_0, "data_offset[0]", "audio channel data offset[0]"},
    {ID_AU_DATA_OFFSET_1, "data_offset[1]", "audio channel data offset[1]"},
};

/* mempool related */
static struct kmem_cache *audio_job_cache;
static mempool_t *audio_job_pool;

/* scheduler */
#ifndef CONFIG_FC7500
static struct workqueue_struct *audio_in_wq, *audio_out_wq;
#endif

/* proc system */
static struct proc_dir_entry *entity_proc, *infoproc;
static struct proc_dir_entry *propertyproc, *jobproc;
static struct proc_dir_entry *vg_dbg_proc, *au_job_proc, *levelproc;

/* variable */
static int engine_busy[ENTITY_ENGINES];
static bool is_first_dec_job = true;
static unsigned int target_queued_time = 0;
static unsigned long last_decode_jiffies = 0;
static struct audio_job_t dump_job;

/* live sound related */
static int *live_chan_num = NULL;
static int *live_on_ssp = NULL;
int *au_get_live_chan_num(void) {return live_chan_num;}
EXPORT_SYMBOL(au_get_live_chan_num);
int *au_get_live_on_ssp(void) {return live_on_ssp;}
EXPORT_SYMBOL(au_get_live_on_ssp);
static atomic_t is_live_cmd_enable;
static atomic_t audio_job_memory_cnt;
static atomic_t job_item_memory_cnt;


static struct au_codec_info_st au_codec_info_ary[MAX_CODEC_TYPE_COUNT] = {
    { "", 0, 0},           // CODEC_TYPE_NONE
    { "PCM", 0, 1024},     // CODEC_TYPE_PCM
    { "AAC", -50, 1024},   // CODEC_TYPE_AAC
    { "ADPCM", -50, 505},  // CODEC_TYPE_ADPCM
    { "ALAW", -50, 320},   // CODEC_TYPE_ALAW
    { "ULAW", -50, 320}    // CODEC_TYPE_ULAW
};
static u32 audio_frame_samples = 0;

#ifdef CONFIG_FC7500
static unsigned int au_user_queued_time = 240;
static unsigned int au_info_one_block_time = 120;
#else
static unsigned int au_user_queued_time = 256;
static unsigned int au_info_one_block_time = 128;
#endif
static unsigned int au_info_mute_counts = 0;
static unsigned int au_info_mute_samples = 0;
static unsigned int au_info_sync_resample_up_count = 0;
static unsigned int au_info_sync_resample_down_count = 0;
static unsigned int au_info_hw_queue_time = 0;
static unsigned int au_info_speedup_ratio = 95;
static unsigned int au_info_buffer_chase_count = 0;
static unsigned int au_info_dec_video_latency = 0;
static unsigned int au_info_sync_change_level = 0;

#ifdef CONFIG_FC7500
static struct task_struct *rcv_rec_task = NULL;
static struct task_struct *heart_beat_task = NULL;
static int rcv_handler(void *data);
static int heart_beat_handler(void *data);
static void audio_send_timeout_handler(struct work_struct *work);

// for log system
static unsigned int log_buffer_info[4];
unsigned int log_buffer_vbase = 0;
unsigned int fc7500_counter_vbase = 0;
unsigned long *pkt_flow = NULL;
unsigned long pkt_flow_sz = 0;
bool enable_fc7500_log = true;
#endif

/* function prototype */
static void work_scheduler(int engine);
static void send_handler(job_item_t *job_item);

/* utilization */
static unsigned int engine_start[ENTITY_ENGINES], engine_end[ENTITY_ENGINES];
static unsigned int engine_time[ENTITY_ENGINES];

/* property lastest record */
struct property_record_t *property_record;

/* log & debug message */
#define MAX_FILTER  5
static unsigned int log_level = 0;    //larger level, more message
static int include_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR
static int exclude_filter_idx[MAX_FILTER]; //init to -1, ENGINE<<16|MINOR

#ifdef CONFIG_FC7500
#include <cpu_comm.h>

void dump_pkt_flow(void)
{
    int i;

    printk("fc7500 code flow dump: \n");
    for (i = 0; i < (pkt_flow_sz / 4); i ++) {
        printk("bitmap[%d] = 0x%x    ", i, (u32)pkt_flow[i]);
        if (i && ((i % 4) == 0))
            printk("\n");
    }
    printk("\n");
}

void freeze_fc7500(void)
{
    static int stuck_print = 0;
    u32 tmp = 0;
    void __iomem *scu_base = 0;

    if (tmp || scu_base) {}

#ifdef IGNORE_7500
    if (stuck_print)
        return;

    printk("Enter 7500 Stuck State!\n");

    stuck_print = 1;

#if 0
    scu_base = ioremap_nocache(0x99000000, 0x00001000);
    if (scu_base) {
        // turn off SSP0~3 clock
        tmp = *(volatile unsigned int *)(scu_base + 0xb8);
        tmp |= ((0x1<<22) | (0x7<<6));
        *(volatile unsigned int *)(scu_base + 0xb8) = tmp;

        // turn off SSP4~5 clock
        tmp = 0;
        tmp = *(volatile unsigned int *)(scu_base + 0xbc);
        tmp |= 0x3 << 18;
        *(volatile unsigned int *)(scu_base + 0xbc) = tmp;

        // reset 7500
        tmp = 0;
        tmp = *(volatile unsigned int *)(scu_base + 0x04);
        tmp &= ~(0x1 << 18);
        *(volatile unsigned int *)(scu_base + 0x04) = tmp;

        iounmap(scu_base);
    }
#endif    
#endif /* IGNORE_7500 */
}

void contruct_persudo_job(job_item_t *job_item)
{
    int ch;
    struct audio_job_t *audio_job;
    struct video_entity_job_t *job;

    if (job_item->private == NULL) {
        job_item->status = DRIVER_STATUS_ONGOING;
        do_engine(job_item);
    }

    job_item->status = DRIVER_STATUS_FINISH;
    audio_job = (struct audio_job_t *)job_item->private;
    job = (struct video_entity_job_t *)job_item->job;
    audio_job->status = JOB_STS_ACK;
    job->root_time = get_gm_jiffies();

    for (ch = 0; ch < 32; ch ++)
        audio_job->data_length[0][ch] = 640; //fixed value

    mark_engine_finish((void *)job_item);
    driver_postprocess((void *)job_item, 0);
}


/* timeout_ms: 0 for not wait, -1 for wait forever, >= 1 means a timeout value
 * return value: 0 for success, < 0 for failed
 */
static int cpu_comm_tranceiver(chan_id_t target, unsigned char *data, int length, bool is_send, int timeout_ms)
{
    cpu_comm_msg_t msg;
    int ret = 0;

    memset(&msg, 0, sizeof(cpu_comm_msg_t));

    /* only for send. for rcv_handler it is a busy routine and may casue cpu infinite loop */
    //if (is_send && (fc7500_rec_stuck | fc7500_pb_stuck))
    if (is_send && (heart_beat_fail >= 4)) {
        return -EAGAIN;
    }

    do {
        msg.target   = target;
        msg.msg_data = data;
        msg.length   = length;

        /* cpucomm has -ERESTARTSYS error code while sending data */
        is_send ? (ret = cpu_comm_msg_snd(&msg, timeout_ms)) : (ret = cpu_comm_msg_rcv(&msg, timeout_ms)) ;
    } while (ret == -ERESTARTSYS);

    return ret;
}
#endif /* CONFIG_FC7500 */

struct au_codec_info_st* au_vg_get_codec_info_array(void)
{
    return au_codec_info_ary;
}


void au_vg_set_log_info(int which, int value)
{
    switch(which) {
    case LOG_VALUE_MUTE:
        au_info_mute_counts ++;
        au_info_mute_samples += value;
        break;
    case LOG_VALUE_SYNC_RESAMPLE_UP:
        au_info_sync_resample_up_count ++;
        break;
    case LOG_VALUE_SYNC_RESAMPLE_DOWN:
        au_info_sync_resample_down_count ++;
        break;
    case LOG_VALUE_HARDWARE_QUEUE_TIME:
        au_info_hw_queue_time = (unsigned int) value;
        break;
    case LOG_VALUE_USER_QUEUE_TIME:
        au_user_queued_time = (unsigned int) value;
        break;
    case LOG_VALUE_SPEEDUP_RATIO:
        au_info_speedup_ratio = (unsigned int) value;
        break;
    case LOG_VALUE_BUFFER_CHASE:
        au_info_buffer_chase_count ++;
        break;
    case LOG_VALUE_ONE_BLOCK_TIME:
        au_info_one_block_time = (unsigned int) value;
        break;
    case LOG_VALUE_DEC_VIDEO_LATENCY:
        au_info_dec_video_latency = (unsigned int) value;
        break;
    case LOG_VALUE_SYNC_CHANGE_LEVEL:
        au_info_sync_change_level = (unsigned int) value;
        break;
    default:
        printk("Invalid log info type(%d).\n", which);
    }
}

unsigned int au_vg_get_log_info(int which, int which2)
{
    unsigned int value = 0;
    switch(which) {
    case LOG_VALUE_MUTE:
        value = which2 ? au_info_mute_counts : au_info_mute_samples;
        break;
    case LOG_VALUE_SYNC_RESAMPLE_UP:
        value = au_info_sync_resample_up_count;
        break;
    case LOG_VALUE_SYNC_RESAMPLE_DOWN:
        value = au_info_sync_resample_down_count;
        break;
    case LOG_VALUE_HARDWARE_QUEUE_TIME:
        value = au_info_hw_queue_time;
        break;
    case LOG_VALUE_USER_QUEUE_TIME:
        value = au_user_queued_time;
        break;
    case LOG_VALUE_SPEEDUP_RATIO:
        value = au_info_speedup_ratio;
        break;
    case LOG_VALUE_BUFFER_CHASE:
        value = au_info_buffer_chase_count;
        break;
    case LOG_VALUE_ONE_BLOCK_TIME:
        value = au_info_one_block_time;
        break;
    case LOG_VALUE_DEC_VIDEO_LATENCY:
        value = au_info_dec_video_latency;
        break;
    case LOG_VALUE_SYNC_CHANGE_LEVEL:
        value = au_info_sync_change_level;
        break;
    default:
        printk("Invalid log info type(%d).\n", which);
    }
    return value;
}


/* Peter's helper function */
static int auto_mode_chk_audio_type(unsigned int *addr, unsigned int EncDataSize, unsigned int *skip_data, int *dec_sample_rate)
{
    unsigned int i;
    unsigned char *op_ptr;
    int got_correct_frame_header = 0;
    unsigned int frame_length;
    unsigned char  id, layer, sr_idx;
    int aac_sample_rates_tbl[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};

    sr_idx = 0;
    op_ptr = (unsigned char *)(*addr);

    for(i = 0;i < EncDataSize;i ++) {
        if( (unsigned short)(op_ptr[0] << 8 | (op_ptr[1] & 0xf0)) == 0xfff0) {

            id      = (op_ptr[1]&0x8)>>3;
            layer   = (op_ptr[1]&0x6)>>1;
            sr_idx  = (op_ptr[2]&0x3c)>>2;

            if( (id!=1)||(layer!=0)||(sr_idx>11) ) {
                op_ptr = op_ptr+1;
            } else {
                frame_length = ((op_ptr[3]&0x3)<<11) | (op_ptr[4]<<3) | ((op_ptr[5]&0xe0)>>5);
                //if( (j+frame_length)> op_size )
                //   got_correct_frame_header = 1;
                //else
                //{
                if ( (*(op_ptr + frame_length)==0xff)&&((*(op_ptr + frame_length+1)&0xf0)==0xf0) ) {
                    got_correct_frame_header = 1;
                    break;
                } else {
                    op_ptr = op_ptr+1;
                }
                //}
            }
        } else {
            op_ptr = op_ptr+1;      //shift one byte
        }
    }
    if (sr_idx > 15) {
        printk("%s: Over range of aac_sample_rates_tbl[](%u), and set as 0\n", __func__, sr_idx);
        sr_idx = 15;
    }

    *addr = (unsigned int)op_ptr;
    *skip_data = i;
    *dec_sample_rate =  aac_sample_rates_tbl[sr_idx];

    return got_correct_frame_header;
}
static int is_print(int engine, int minor)
{
    int i;
    if (include_filter_idx[0] >= 0) {
        for (i = 0; i<MAX_FILTER; i++)
            if (include_filter_idx[i] == ((engine << 16) | minor))
                return 1;
    }

    if (exclude_filter_idx[0] >= 0) {
        for (i = 0; i < MAX_FILTER; i++)
            if (exclude_filter_idx[i] == ((engine << 16) | minor))
                return 0;
    }
    return 1;
}

#if 1
#define DEBUG_M(level, engine, minor, fmt, args...) { \
    if ((log_level >= level) && is_print(engine, minor)) \
        printm(MODULE_NAME, fmt, ## args); }
#else
#define DEBUG_M(level, engine, minor, fmt, args...) { \
    if ((log_level >= level) && is_print(engine, minor)) \
        printk(fmt, ## args);}
#endif

static int driver_parse_in_property(void *param, struct video_entity_job_t *job)
{
    int i = 0;
    struct audio_job_t *audio_job = (struct audio_job_t *)param;
    int idx = MAKE_IDX(ENTITY_MINORS, ENTITY_FD_ENGINE(job->fd), ENTITY_FD_MINOR(job->fd));
    struct video_property_t *property = job->in_prop;
    int enc_type_done = 0;

    while (property[i].id != 0) {
        switch (property[i].id) {
            case ID_AU_CHANNEL_BMP:
                audio_job->chan_bitmap = property[i].value;
                break;
            case ID_BITRATE:
                audio_job->shared.bitrate = property[i].value;
                break;
            case ID_AU_RESAMPLE_RATIO:
                audio_job->resample_ratio = property[i].value;
                break;
            case ID_AU_SAMPLE_RATE:
                audio_job->sample_rate = property[i].value;
                break;
            case ID_AU_SAMPLE_SIZE:
                audio_job->sample_size = property[i].value;
                break;
            case ID_AU_CHANNEL_TYPE:
                audio_job->channel_type = property[i].value;
                break;
            case ID_AU_ENC_TYPE:
                if (!enc_type_done) {
                    enc_type_done = 1;
                    audio_job->codec_type = property[i].value;
                }
                break;

            case ID_AU_BLOCK_SIZE:
                audio_job->dec_blk_sz = property[i].value;
                break;
            case ID_AU_BLOCK_COUNT:
                audio_job->enc_blk_cnt = property[i].value;
                break;
            case ID_AU_FRAME_SAMPLES:
                if (property[i].value)
                    audio_frame_samples = property[i].value;
                    //au_codec_info_ary[CODEC_TYPE_PCM].frame_samples = property[i].value;
                break;

            case ID_BITSTREAM_SIZE:
                audio_job->bitstream_size = property[i].value;
                break;
            case ID_AU_DATA_LENGTH_0: /* data length of encode type 0 */
                audio_job->data_length[0][property[i].ch-1] = property[i].value;
                break;
            case ID_AU_DATA_LENGTH_1: /* data length of encode type 1 */
                audio_job->data_length[1][property[i].ch-1] = property[i].value;
                break;
            case ID_AU_DATA_OFFSET_0: /* data offset of encode type 0 */
                audio_job->data_offset[0][property[i].ch-1] = property[i].value;
                break;
            case ID_AU_DATA_OFFSET_1: /* data offset of encode type 1 */
                audio_job->data_offset[1][property[i].ch-1] = property[i].value;
                break;
            default:
                break;
        }
        if (i < MAX_RECORD) {
            property_record[idx].property[i].id = property[i].id;
            property_record[idx].property[i].value = property[i].value;
        }
        i++;
    }
    property_record[idx].property[i].id = property_record[idx].property[i].value = 0;
    property_record[idx].entity = job->entity;
    property_record[idx].job_id = job->id;

    return 1;
}


static int driver_set_out_property(void *param, struct video_entity_job_t *job)
{
    int i = 0, ch = 0;
    struct audio_job_t *audio_job = (struct audio_job_t *)param;
    struct video_property_t *property = job->out_prop;
    unsigned int start_addr;
#if defined(CONFIG_FC7500)
    start_addr = job->out_buf.buf[0].addr_pa;
#else
    start_addr = job->out_buf.buf[0].addr_va;
#endif

    property[i].id = ID_AU_CHANNEL_TYPE;
    property[i].value = audio_job->chan_bitmap;
    i++;
    property[i].id = ID_BITRATE;
    property[i].value = audio_job->shared.bitrate;
    i++;
    property[i].id = ID_AU_RESAMPLE_RATIO;
    property[i].value = audio_job->resample_ratio;
    i++;
    property[i].id = ID_AU_SAMPLE_RATE;
    property[i].value = audio_job->sample_rate;
    i++;
    property[i].id = ID_AU_SAMPLE_SIZE;
    property[i].value = audio_job->sample_size;
    i++;
    property[i].id = ID_AU_CHANNEL_TYPE;
    property[i].value = audio_job->channel_type;
    i++;
    property[i].id = ID_AU_ENC_TYPE;
    property[i].value = audio_job->codec_type;
    i++;
    property[i].id = ID_BITSTREAM_SIZE;
    property[i].value = audio_job->bitstream_size;
    i++;

    for (ch = 0; ch < 32; ch ++) {
        if (!(audio_job->chan_bitmap & (1 << ch)))
            continue;

        property[i].ch = ch + 1;
        property[i].id = ID_AU_DATA_LENGTH_0;
        property[i].value = audio_job->data_length[0][ch];
        i ++;
        property[i].ch = ch + 1;
        property[i].id = ID_AU_DATA_OFFSET_0;
        property[i].value = audio_job->data_offset[0][ch] - start_addr;
        i ++;


        if (audio_job->codec_type & BITMAP_LAST_WORD_MASK(16)) {
            property[i].ch = ((1 << 6) | ch) + 1;
            property[i].id = ID_AU_DATA_LENGTH_1;
            property[i].value = audio_job->data_length[1][ch];
            i ++;
            property[i].ch = ((1 << 6) | ch) + 1;
            property[i].id = ID_AU_DATA_OFFSET_1;
            property[i].value = audio_job->data_offset[1][ch] - start_addr;
            i ++;
        }
    }

    property[i].id = ID_NULL;
    return 1;
}


static void mark_engine_start(void *param)
{
    return;
}


/*
void ISR()
{
    mark_engine_finish(void *job_param); /// <<<<<< Add mark_engine_end in your ISR handler >>>>>>
    ....
}
 */
static void mark_engine_finish(void *job_param)
{
    int engine, minor;
    struct audio_job_t *audio_job;
    job_item_t *job_item = (job_item_t *)job_param;

    engine = job_item->engine;
    minor = job_item->minor;
    audio_job = (struct audio_job_t *)job_item->private;

    driver_set_out_property(audio_job, job_item->job);

    return;
}


static int driver_preprocess(void *parm, void *priv, int *p_video_latency)
{
    job_item_t *job_item = (job_item_t *)parm;
    struct video_property_t property[MAX_PROCESS_PROPERTYS];
    struct audio_job_t *audio_job = job_item->private;
    int i = 0;
    int ret = 0;

    ret = video_preprocess(job_item->job, (void *)property);
    if (ret < 0)
        return ret;

    for (i = 0; i < MAX_PROCESS_PROPERTYS; i++) {
        if (ID_NULL == property[i].id)
            break;
        if (ID_SPEED_CHANGE_LEVEL == property[i].id) {
            if (au_info_sync_change_level != 0) {
                if (property[i].value == 1) {    //need to increase speed
                    audio_job->resample_ratio -= au_info_sync_change_level;
                    au_vg_set_log_info(LOG_VALUE_SYNC_RESAMPLE_UP, 0);
                } else if (property[i].value == -1) {  //need to slow down
                    audio_job->resample_ratio += au_info_sync_change_level;
                    au_vg_set_log_info(LOG_VALUE_SYNC_RESAMPLE_DOWN, 0);
                }
            }
        } else if (ID_VIDEO_LATENCY == property[i].id) {
            *p_video_latency = property[i].value;
        }
    }
    return 0;
}


static int driver_postprocess(void *parm, void *priv)
{
    job_item_t *job_item = (job_item_t *)parm;

    return video_postprocess(job_item->job, priv);
}


static void callback_scheduler(int engine)
{
    int minor;
    char *p;
    struct video_entity_job_t *job;
    job_item_t *job_item, *job_item_next;
    audio_engine_t  *pEngine = &audio_engine[engine];

    list_for_each_entry_safe(job_item, job_item_next, &pEngine->list, engine_list) {
        minor = job_item->minor;
        job = (struct video_entity_job_t *)job_item->job;

        if (job_item->status == DRIVER_STATUS_FINISH) {
            job->status = JOB_STATUS_FINISH;
        } else if (job_item->status == DRIVER_STATUS_FAIL) {
            job->status = JOB_STATUS_FAIL;
        } else {
            //continue; /* it may cause out of order */
            break;
        }

        list_del(&job_item->engine_list);
        pEngine->callback_cnt ++;

        DEBUG_M(LOG_WARNING, job_item->engine, minor, "(%d,%d) job %d status %d callback 0x%x\n",
                job_item->engine, minor, job->id, job->status, (int)jiffies&0xffff);

        if (ENTITY_FD_TYPE(job->fd) == TYPE_AU_GRAB) {
            p = (char *) job->out_buf.buf[0].addr_va;
            DEBUG_M(LOG_DETAIL, job_item->engine, job_item->minor, "CB jid(%d) out-buf[0] va(%p) len(%d): "
                    "%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n",
                    job->id, job->out_buf.buf[0].addr_va, job->out_buf.buf[0].size, p[0], p[1], p[2], p[3],
                    p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        }

        job->callback(job_item->root_job); //callback root job

        GM_LOG((GM_LOG_REC_PKT_FLOW | GM_LOG_PB_PKT_FLOW), "job(%d) callbacked\n", job_item->job_id);

        if (job_item->private != NULL) { // means that this job has been constructed!
#if defined(CONFIG_FC7500) && defined(HANDLE_TIMEOUT_JOB)
            //printk("callbacked job id: %d, fd_type: %d\n", job_item->job_id, ENTITY_FD_TYPE(job->fd));
            if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_ENC) {
                u32 job_seq_num = ((struct audio_job_t *)(job_item->private))->job_seq_num;

                AU_REC_JOB_LOG_SEMA_LOCK;
                rec_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK] = job_seq_num;
                GM_LOG(GM_LOG_REC_PKT_FLOW,"record seq num(%d) callbacked, ts(%lu)\n", ((struct audio_job_t *)(job_item->private))->job_seq_num, get_gm_jiffies());
                AU_REC_JOB_LOG_SEMA_UNLOCK;
            } else if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_DEC) {
                u32 job_seq_num = ((struct audio_job_t *)(job_item->private))->job_seq_num;

                AU_PB_JOB_LOG_SEMA_LOCK;
                pb_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK] = job_seq_num;
                GM_LOG(GM_LOG_PB_PKT_FLOW,"playback seq num(%d) callbacked, ts(%lu)\n", ((struct audio_job_t *)(job_item->private))->job_seq_num, get_gm_jiffies());
                AU_PB_JOB_LOG_SEMA_UNLOCK;
            }
#endif        

        if (job_item->private) {
            mempool_free(job_item->private, audio_job_pool);
            atomic_dec(&audio_job_memory_cnt);
        }
        }
        
        job_item->private = NULL;   /* purpose: if job_item is reused due to bug, we can easy debug */
        kmem_cache_free(job_item_cache, job_item);
        atomic_dec(&job_item_memory_cnt);
    }
}

static void trigger_callback_finish(void *job_param, int status)
{
    job_item_t *job_item = (job_item_t *)job_param;

    job_item->status = status;

    callback_scheduler(job_item->engine);
}

#define AAC_FRAME_SZ    	1024
#define IMA_BLK_SZ_MONO		505
#define IMA_BLK_SZ_DUAL		498
#define AULAW_BLK_SZ     320

void construct_audio_job(job_item_t *job_item)
{
    struct audio_job_t *audio_job;
    struct video_entity_job_t *job;
    int engine, minor, i, j, queued_samples, frame_samples;
    int fmt_cnt;
    unsigned int one_fmt_size, one_ch_size, diff_time;
    unsigned codec_type[SUPPORT_TYPE] = {0};
    const char *codec_name = plat_audio_get_codec_name();
    unsigned int block_time;
    int total_chan_cnt = 0;
    int codec_for_checked = 0;

    job = (struct video_entity_job_t *)job_item->job;
    engine = job_item->engine;
    minor = job_item->minor;

    audio_job = mempool_alloc(audio_job_pool, GFP_ATOMIC);
    memset(audio_job, 0, sizeof(struct audio_job_t));
    atomic_inc(&audio_job_memory_cnt);
    job_item->private = (void *)audio_job;

    driver_parse_in_property((void *)audio_job, job);
    driver_preprocess(job_item, 0, &au_info_dec_video_latency);

    switch (ENTITY_FD_TYPE(job->fd)) {
        case TYPE_AUDIO_ENC:

            GM_LOG(GM_LOG_REC_PKT_FLOW,"construct record job(%d)\n", job_item->job_id);

            audio_job->status = JOB_STS_REC_RUN;
            audio_job->out_pa = 0;
            audio_job->out_size = 0;
            audio_job->bitstream_size = 0;
#if 1
            /**
             * VG always allocates buffer for 'two' formats, even if user requests for only one.
             */
            fmt_cnt = 2;
#else
            fmt_cnt = (audio_job->codec_type & BITMAP_LAST_WORD_MASK(16)) ? 2 : 1; //check if high word(type2) exist
#endif
            one_fmt_size = job->out_buf.buf[0].size / fmt_cnt;
            one_ch_size = one_fmt_size / total_rec_chan;

            if (job->out_buf.count != 1)
                panic("%s: error! buf.count=%d"VG_ERROR, __func__, job->out_buf.count);
            if (SUPPORT_TYPE > 2)
                panic("%s: too many types(%d)"VG_ERROR, __func__, SUPPORT_TYPE);
            if (job->out_buf.buf[0].size % fmt_cnt)
                panic("%s: spec calculation error! size(%d) fmt_cnt(%d)"VG_ERROR,
                      __func__, job->out_buf.buf[0].size, fmt_cnt);
            if (one_fmt_size % total_rec_chan)
                panic("%s: spec calculation error! fmt_size(%d) ssp_ch(%d)"VG_ERROR,
                      __func__, one_fmt_size, total_rec_chan);

            codec_type[0] = (audio_job->codec_type & BITMAP_FIRST_WORD_MASK(16)) >> 16;
            codec_type[1] = audio_job->codec_type & BITMAP_LAST_WORD_MASK(16);

            if (codec_type[0] > EM_AUDIO_NONE)
                codec_for_checked = codec_type[0];
            else if (codec_type[1] > EM_AUDIO_NONE)
                codec_for_checked = codec_type[1];
            else
                printk("%s: one of the codec_type should be PCM!!\n", __func__);

            switch (codec_for_checked) {
                case EM_AUDIO_PCM:
                    /* do nothing */
                    break;

                case EM_AUDIO_AAC:
                    if (audio_frame_samples % AAC_FRAME_SZ)
                        printk("%s: frame samples(%u) is not multiple of %d\n", __func__, audio_frame_samples, AAC_FRAME_SZ);
                    break;

                case EM_AUDIO_ADPCM:
                    if (audio_job->channel_type > 1) {
                        if (audio_frame_samples % IMA_BLK_SZ_DUAL)
                            printk("%s: frame samples(%u) is not multiple of %d\n", __func__, audio_frame_samples, IMA_BLK_SZ_DUAL);
                    } else {
                        if (audio_frame_samples % IMA_BLK_SZ_MONO)
                            printk("%s: frame samples(%u) is not multiple of %d\n", __func__, audio_frame_samples, IMA_BLK_SZ_MONO);
                    }
                    break;

                case EM_AUDIO_G711_ALAW:
                case EM_AUDIO_G711_ULAW:
                    if (audio_frame_samples % AULAW_BLK_SZ)
                        printk("%s: frame samples(%u) is not multiple of %d\n", __func__, audio_frame_samples, AULAW_BLK_SZ);
                    break;

                default:
                    printk("%s: no such codec type!\n", __func__);
                    break;
            }

            one_ch_size = audio_job->enc_blk_cnt * audio_frame_samples * (audio_job->sample_size / BITS_PER_BYTE);

            if (one_ch_size > (one_fmt_size / total_rec_chan))
                panic("%s: one_ch_size is too large. one_ch_size(%d) fmt_size(%d) max_chan(%d)"VG_ERROR,
                      __func__, one_ch_size, one_fmt_size, total_rec_chan);

            for (i = 0; i < SUPPORT_TYPE; i ++) {
#if defined(CONFIG_FC7500)
                audio_job->in_pa[i] = job->out_buf.buf[0].addr_pa + one_fmt_size * i;
#else
                audio_job->in_pa[i] = job->out_buf.buf[0].addr_va + one_fmt_size * i;
#endif
                audio_job->in_size[i] = one_fmt_size;
                for (j = 0;j < total_rec_chan;j++) {
                    audio_job->data_offset[i][j] = audio_job->in_pa[i] + j * (one_fmt_size / total_rec_chan);
                    audio_job->data_length[i][j] = one_ch_size;
                }
            }

            audio_job->ssp_bitmap = 0;
            for (i = 0;i < total_ssp_cnt;i ++) {
#if defined(CONFIG_PLATFORM_GM8210)
                //if (audio_ssp_num[i] == 3) /* SSP3 of GM8210 is connected to HDMI */
                //    continue;
#endif
#if defined(CONFIG_PLATFORM_GM8287)
                if (audio_ssp_num[i] == 2) /* SSP2 of GM8287 is connected to HDMI */
                    continue;
#endif
                if (audio_job->chan_bitmap & BITMASK(total_chan_cnt, audio_ssp_chan[i]))
                    audio_job->ssp_bitmap |= 1 << i;
                total_chan_cnt += audio_ssp_chan[i];
            }

            if (audio_job->sample_rate == 0)
                audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
            if (audio_job->sample_size == 0)
                audio_job->sample_size = DEFAULT_SAMPLE_SIZE;
            break;
        case TYPE_AUDIO_DEC:

            GM_LOG(GM_LOG_PB_PKT_FLOW,"construct playback job(%d)\n", job_item->job_id);

            if (!job->in_buf.buf[0].addr_pa || !job->in_buf.buf[0].size)
                panic("%s: error! out_size[0] = %u, out_pa[0] = %u"VG_ERROR, __func__,
                    job->in_buf.buf[0].addr_pa, job->in_buf.buf[0].size);
            audio_job->status = JOB_STS_PLAY_RUN;
#if defined(CONFIG_FC7500)
            audio_job->out_pa = job->in_buf.buf[0].addr_pa;
#else
            audio_job->out_pa = job->in_buf.buf[0].addr_va;
#endif
            audio_job->out_size = job->in_buf.buf[0].size;
            audio_job->in_pa[0] = 0;
            audio_job->in_pa[1] = 0;
            audio_job->in_size[0] = 0;
            audio_job->in_size[1] = 0;
            audio_job->ssp_bitmap = audio_job->chan_bitmap;

            if (audio_job->channel_type <= 0 || audio_job->channel_type > 2)
                audio_job->channel_type = 0x1;

            /* Default sample rate and size */
            if (audio_job->sample_rate == 0)
                audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
            if (audio_job->sample_size == 0)
                audio_job->sample_size = DEFAULT_SAMPLE_SIZE;

            if (codec_name) {
                if (strncmp(codec_name, "nvp1918", sizeof("nvp1918") - 1) == 0)
                    audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
                if (strncmp(codec_name, "cx25930", sizeof("cx25930") - 1) == 0)
                    audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
                if (strncmp(codec_name, "cx20811", sizeof("cx20811") - 1) == 0)
                    audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
                if (strncmp(codec_name, "tw2964", sizeof("tw2964") - 1) == 0)
                    audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
            }

            if (!audio_job->sample_rate && is_first_dec_job) {
                unsigned int preceeding_garbage_data = 0;
                if (auto_mode_chk_audio_type(&job->in_buf.buf[0].addr_va, audio_job->bitstream_size, &preceeding_garbage_data, &audio_job->sample_rate)) {
                    audio_job->bitstream_size -= preceeding_garbage_data;
                    audio_job->out_pa += preceeding_garbage_data;
                    audio_job->codec_type = EM_AUDIO_AAC;
                } else {
                    /* check failed ==> set default value */
                    audio_job->sample_rate = DEFAULT_SAMPLE_RATE;
                    audio_job->codec_type = EM_AUDIO_PCM;
                }
            }

            if (*audio_drv_get_pcm_save_flag()) {
                static int stored_sz = DEFAULT_SAMPLE_SIZE * DEFAULT_SAMPLE_RATE * PLAYBACK_SEC / BITS_PER_BYTE;
                static unsigned long long offset = 0;
                stored_sz -= audio_job->bitstream_size;
                if (stored_sz >= 0) {
                    int ret = 0;
                    ret = audio_write_file(PLAYBACK_FILE_PATH, (void *)job->in_buf.buf[0].addr_va, audio_job->bitstream_size, &offset);
                    if (ret < 0)
                        printk("%s: write PCM error!\n", __func__);
                } else {
                    /* reset */
                    audio_drv_set_pcm_save_flag(0);
                    stored_sz = DEFAULT_SAMPLE_SIZE * DEFAULT_SAMPLE_RATE * PLAYBACK_SEC / BITS_PER_BYTE;
                    offset = 0;
                    printk("save %s done!\n", PLAYBACK_FILE_PATH);
                }
            }

            target_queued_time = au_vg_get_log_info(LOG_VALUE_USER_QUEUE_TIME, 0);
            diff_time = jiffies_to_msecs(JIFFIES_DIFF(jiffies, last_decode_jiffies));
            if (is_first_dec_job || diff_time > (target_queued_time * 10)) {
                queued_samples = (target_queued_time * audio_job->sample_rate) / 1000;
                frame_samples = au_codec_info_ary[audio_job->codec_type].frame_samples;
#define USE_MUTE_BUFFER
#if defined(USE_MUTE_BUFFER)
                audio_job->time_sync.mute_samples = queued_samples;
                au_vg_set_log_info(LOG_VALUE_MUTE, queued_samples);
#else
                audio_job->resample_ratio = (queued_samples * 100) / frame_samples;
#endif
            }

            block_time = au_vg_get_log_info(LOG_VALUE_ONE_BLOCK_TIME, 0);
            if (au_info_dec_video_latency > 0 && (au_info_speedup_ratio != 100) &&
                au_info_hw_queue_time > au_vg_get_log_info(LOG_VALUE_ONE_BLOCK_TIME, 0) && /* at least one block is required */
                au_info_hw_queue_time > (au_info_dec_video_latency + 40)) {
                audio_job->resample_ratio = (audio_job->resample_ratio * au_info_speedup_ratio) / 100;
                au_vg_set_log_info(LOG_VALUE_BUFFER_CHASE, 0);
            }

            is_first_dec_job = false;
            last_decode_jiffies = jiffies;
            break;
        default:
            GM_LOG(GM_LOG_ERROR,"%s: it is not audio job!!!\n", __func__);
            printk(KERN_ERR"%s: it is not audio job!!!\n", __func__);
            break;
    }

    audio_job->ssp_chan = audio_ssp_chan[0];
    audio_job->data = (void *)job_item;
#if defined(CONFIG_FC7500) && defined(HANDLE_TIMEOUT_JOB)
    if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_ENC) {
        audio_job->job_seq_num = record_seq_num++;
        audio_job->job_type = (ENTITY_FD_TYPE(job->fd)==TYPE_AUDIO_ENC)?(RECORD_JOB):(PLAYBACK_JOB);
        GM_LOG(GM_LOG_REC_PKT_FLOW,"%s job(%d) with seq(%u) constructed!\n", ((audio_job->job_type)==RECORD_JOB)?("record"):("playback"), job_item->job_id, audio_job->job_seq_num);
    } else if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_DEC) {
        audio_job->job_seq_num = playback_seq_num++;
        audio_job->job_type = (ENTITY_FD_TYPE(job->fd)==TYPE_AUDIO_ENC)?(RECORD_JOB):(PLAYBACK_JOB);
        GM_LOG(GM_LOG_PB_PKT_FLOW,"%s job(%d) with seq(%u) constructed!\n", ((audio_job->job_type)==RECORD_JOB)?("record"):("playback"), job_item->job_id, audio_job->job_seq_num);
    } else
        panic("%s, known job type(%d)!", __func__, ENTITY_FD_TYPE(job->fd));
#endif    

    DEBUG_M(LOG_WARNING, engine, minor,"(%d,%d) job %d va 0x%x trigger\n",
        job_item->engine, job_item->minor, job->id, job->out_buf.buf[0].addr_va);

    GM_LOG((GM_LOG_PB_PKT_FLOW | GM_LOG_REC_PKT_FLOW), "%s job(%d) consturcted\n", 
           (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_ENC)?("record"):("playback"), job->id);

}

void set_live_sound(int ssp_idx, int chan_num, bool is_on)
{
    bool is_set = false;
    int ssp_ch_num = ssp_and_chan_translate(chan_num, false);
#ifdef CONFIG_FC7500
    int ret = 0;

    /* for DUAL GM8210 */
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);

    if (pci_id != FMEM_PCI_HOST) {
        int i;
        for (i = 0;i < total_ssp_cnt;i ++)
            if (audio_ssp_num[i] == 3)
                ssp_idx = i;
    }
#endif

    if (ssp_idx >= SSP_MAX_USED)
        printk(KERN_ERR"%s: No such SSP(%d)\n", __func__, ssp_idx);

    if (audio_out_enable[ssp_idx] == false) {
        printk(KERN_ERR"%s, vch %d is not supported!\n", __func__, ssp_idx);
        return;
    }

    /* why needs so complicated checking? bcuz videograph's strange behavior */
    if (is_on == true) {
        if (live_on_ssp[ssp_idx] == true) {
            if (live_chan_num[ssp_idx] != ssp_ch_num) {
                live_chan_num[ssp_idx] = ssp_ch_num;
                is_set = true;
            }
        } else {
            live_on_ssp[ssp_idx] = true;
            live_chan_num[ssp_idx] = ssp_ch_num;
            is_set = true;
        }
    } else {
        if (live_on_ssp[ssp_idx] == true) {
            live_on_ssp[ssp_idx] = false;
            if (live_chan_num[ssp_idx] == ssp_ch_num) {
                live_chan_num[ssp_idx] = -1;
                is_set = true;
            }
        }
    }

    if (is_set) {
        int rec_ssp_idx = 0;

        rec_ssp_idx = ssp_and_chan_translate(chan_num, true);

        if (1 /* no hw bypass live */) {
            static struct audio_job_t live_job;

            while (atomic_read(&is_live_cmd_enable) == 0)
                msleep(1);

            atomic_set(&is_live_cmd_enable, 0);

            live_job.chan_bitmap = chan_num; /* use chan_bitmap to send channel number */
            live_job.ssp_bitmap = ssp_idx;
            is_on ? (live_job.status = JOB_STS_LIVE_RUN) : (live_job.status = JOB_STS_LIVE_STOP);
#ifdef CONFIG_FC7500
#if 0 // move to driver_vg_init()
            if (!rcv_rec_task) {
                rcv_rec_task = kthread_run(rcv_handler, NULL, "rcv_rec_task");
                if (IS_ERR(rcv_rec_task)) {
                    panic("%s: unable to create kernel thread: %ld\n",
                            __func__, PTR_ERR(rcv_rec_task));
                }
            }
#endif
            /* send live sound command to fc7500 */
            ret = cpu_comm_tranceiver(CPUCOMM_LIVE_CH, (unsigned char *)&live_job, sizeof(struct audio_job_t), true, COMM_TIMEOUT);
            if (ret != 0) {
                audio_engine_t  *pEngine = &audio_engine[LIVESOUND_ENGINE_NUM];
                pEngine->comm_live_failed_cnt ++;
                printk("%s: cpu_comm failed(cnt:%u)\n", __func__, pEngine->comm_live_failed_cnt);
                atomic_set(&is_live_cmd_enable, 1);
            } else {
                audio_engine_t  *pEngine = &audio_engine[LIVESOUND_ENGINE_NUM];
                /*
                 * Notice: the context is under mutex protection
                 */
                if (delayed_work_pending(&pEngine->write_work)) {
                    printk("%s, bug! cancel delayed work! \n", __func__);
                    cancel_delayed_work_sync(&pEngine->write_work);
                }
                PREPARE_DELAYED_WORK(&pEngine->write_work, audio_send_timeout_handler);
                schedule_delayed_work(&pEngine->write_work, msecs_to_jiffies(RCV_TIMEOUT));
            }
#else
            INIT_WORK(&live_job.in_work, audio_in_handler);
            queue_work(audio_in_wq, &live_job.in_work);
#endif
        } else {
#if defined(CONFIG_PLATFORM_GM8210) || defined(CONFIG_PLATFORM_GM8287)
            plat_audio_sound_switch(ssp_idx, chan_num, is_on);
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
            printk(KERN_ERR"%s, no such ssp index!\n", __func__);
#endif
        }
    }
}
EXPORT_SYMBOL(set_live_sound);

/* process one job_item and wake up the worker for processing
 * it is under mutex protection.
 */
static inline void send_handler(job_item_t *job_item)
{
    struct audio_job_t *audio_job = (struct audio_job_t *)job_item->private;
    struct video_entity_job_t *job;
    unsigned long ssp_idx = 0;
    int engine = 0;
#ifdef CONFIG_FC7500
    int ret = 0;
    unsigned long t1, t2;
#endif
    audio_engine_t  *pEngine = NULL;

    engine = job_item->engine;
    pEngine = &audio_engine[engine];
    if (engine_busy[engine] == 0) {
        //printk("%s: engine(%d) should not be idle\n", __func__, engine);
        GM_LOG(GM_LOG_ERROR, "%s: engine(%d) should not be idle\n", __func__, engine);
        engine_busy[engine] = 1;
    }

    job = (struct video_entity_job_t *)job_item->job;

#ifdef CONFIG_FC7500
#if 0 // move to driver_vg_init()
    if (!rcv_rec_task) {
        rcv_rec_task = kthread_run(rcv_handler, NULL, "rcv_rec_task");
        if (IS_ERR(rcv_rec_task)) {
            panic("%s: unable to create kernel thread: %ld\n",
                    __func__, PTR_ERR(rcv_rec_task));
        }
    }
#endif
    /* when fc7500 is stuck, we hope the job can return to videograph as soon as possible */
    if (fc7500_stuck) {
        if (!delayed_work_pending(&pEngine->write_work)) {
            PREPARE_DELAYED_WORK(&pEngine->write_work, audio_send_timeout_handler);
            schedule_delayed_work(&pEngine->write_work, msecs_to_jiffies(10));               
        }        
        return;
    }

    /*
     * Notice: the context is under mutex protection
     */
    if (delayed_work_pending(&pEngine->write_work)) {
        //printk("%s, bug! cancel delayed work! \n", __func__);
        GM_LOG(GM_LOG_ERROR, "%s, bug! cancel delayed work! \n", __func__);
        cancel_delayed_work_sync(&pEngine->write_work);
    }
    PREPARE_DELAYED_WORK(&pEngine->write_work, audio_send_timeout_handler);
    schedule_delayed_work(&pEngine->write_work, msecs_to_jiffies(RCV_TIMEOUT));
#endif

    switch (audio_job->status) {
        case JOB_STS_REC_RUN:
#ifdef CONFIG_FC7500
            /* send buffer to fc7500 for audio recording */
            t1 = get_gm_jiffies();
            ret = cpu_comm_tranceiver(CPUCOMM_RECORD_CH, (unsigned char *)audio_job, sizeof(struct audio_job_t), true, COMM_TIMEOUT);
            t2 = get_gm_jiffies();

            if (JIFFIES_DIFF(t2,t1) > 1000) {
                //printk("%s, send record job more than 1s\n", __func__);
                GM_LOG(GM_LOG_ERROR, "%s, send record job more than 1s\n", __func__);
            }

            if (ret != 0) {
                job_item_t *job_item, *job_item_next;

                if ((pEngine->comm_snd_failed_cnt % 500) == 0) {
                    GM_LOG(GM_LOG_ERROR, "%s: JOB_STS_REC_RUN send cpu_comm channel%d failed!\n", __func__, CPUCOMM_RECORD_CH);
                    //printk("%s: JOB_STS_REC_RUN send cpu_comm channel%d failed!\n", __func__, CPUCOMM_RECORD_CH);
                }

                pEngine->comm_snd_failed_cnt ++;

                list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
                    if (job_item->status != DRIVER_STATUS_FINISH)
                        job_item->status = DRIVER_STATUS_FAIL;
                }
                /* cancel delayed_work */
                if (delayed_work_pending(&pEngine->write_work))
                    cancel_delayed_work_sync(&pEngine->write_work);

                callback_scheduler(engine);
                engine_busy[engine] = 0;
            }
#else
            GM_LOG(GM_LOG_REC_PKT_FLOW,"put job(%d) in record wq\n", job_item->job_id);
            INIT_WORK(&audio_job->in_work, audio_in_handler);
            queue_work(audio_in_wq, &audio_job->in_work);
#endif
            break;
        case JOB_STS_PLAY_RUN:
            for_each_set_bit(ssp_idx, (const unsigned long *)&audio_job->ssp_bitmap, total_ssp_cnt) {
                if (audio_out_enable[ssp_idx] == false || live_on_ssp[ssp_idx]) {
                    printk(KERN_ERR"%s, vch %lu is in now use or not supported!\n", __func__, ssp_idx);
                    audio_job->status = JOB_STS_DECERR;
                }
            }
#ifdef CONFIG_FC7500
            /* send audio data to fc7500 for playing */
            t1 = get_gm_jiffies();
            ret = cpu_comm_tranceiver(CPUCOMM_PLAYBACK_CH, (unsigned char *)audio_job, sizeof(struct audio_job_t), true, COMM_TIMEOUT);
            t2 = get_gm_jiffies();
            
            if (JIFFIES_DIFF(t2,t1) > 1000) {
                GM_LOG(GM_LOG_ERROR, "%s, send playback job more than 1s\n", __func__);
                //printk("%s, send playback job more than 1s\n", __func__);
            }

            if (ret != 0) {
                job_item_t *job_item, *job_item_next;

                if ((pEngine->comm_snd_failed_cnt % 500) == 0)
                    printk("%s: JOB_STS_PLAY_RUN send cpu_comm channel(%d) failed!\n", __func__, CPUCOMM_PLAYBACK_CH);

                pEngine->comm_snd_failed_cnt ++;

                list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
                    if (job_item->status != DRIVER_STATUS_FINISH)
                        job_item->status = DRIVER_STATUS_FAIL;
                }

                /* cancel delayed_work */
                if (delayed_work_pending(&pEngine->write_work))
                    cancel_delayed_work_sync(&pEngine->write_work);

                callback_scheduler(engine);
                engine_busy[engine] = 0;
            }
#else
            GM_LOG(GM_LOG_PB_PKT_FLOW,"put job(%d) in playback wq\n", job_item->job_id);
            INIT_WORK(&audio_job->out_work, audio_out_handler);
            queue_work(audio_out_wq, &audio_job->out_work);
#endif
            break;
        default:
            printk("%s: Error job status(%d)\n", __func__, audio_job->status);
            break;
    }
}

static int audio_enc_timeout[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static int audio_dec_timeout[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static u32 audio_enc_overrun[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static u32 audio_dec_underrun[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static u32 au_enc_req_dma_chan_failed_cnt[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
static u32 au_dec_req_dma_chan_failed_cnt[SSP_MAX_USED] = {[0 ... (SSP_MAX_USED - 1)] = 0};
int *audio_get_enc_timeout(void) {return audio_enc_timeout;}
EXPORT_SYMBOL(audio_get_enc_timeout);
int *audio_get_dec_timeout(void) {return audio_dec_timeout;}
EXPORT_SYMBOL(audio_get_dec_timeout);
int *audio_get_enc_overrun(void) {return audio_enc_overrun;}
EXPORT_SYMBOL(audio_get_enc_overrun);
int *audio_get_dec_underrun(void) {return audio_dec_underrun;}
EXPORT_SYMBOL(audio_get_dec_underrun);
int *au_get_enc_req_dma_chan_failed_cnt(void) {return au_enc_req_dma_chan_failed_cnt;}
EXPORT_SYMBOL(au_get_enc_req_dma_chan_failed_cnt);
int *au_get_dec_req_dma_chan_failed_cnt(void) {return au_dec_req_dma_chan_failed_cnt;}
EXPORT_SYMBOL(au_get_dec_req_dma_chan_failed_cnt);

/* when audio driver finished VG job,
 * use this function to do audio_job check */
void audio_job_check(struct audio_job_t *audio_job)
{
    int codec_type;
    unsigned int block_time;
    job_item_t *job_item = NULL, *job_item_next;
    struct video_entity_job_t *job;
    audio_engine_t  *pEngine;
    int get_job = 0, engine = 0;
    int status = DRIVER_STATUS_FAIL;
    unsigned long now_jiffies = get_gm_jiffies();

    if (!audio_job)
        panic("%s: audio_job is NULL\n", __func__);

    switch (audio_job->status) {
        case JOB_STS_ACK:
            /* correct status */
            status = DRIVER_STATUS_FINISH;
            break;
        case JOB_STS_LIVE_STOP:
        case JOB_STS_LIVE_RUN:
            atomic_set(&is_live_cmd_enable, 1);
            /* cancel delayed_work */
            pEngine = &audio_engine[LIVESOUND_ENGINE_NUM];
            if (delayed_work_pending(&pEngine->write_work))
                cancel_delayed_work_sync(&pEngine->write_work);
            return;
        case JOB_STS_ERROR:
            printk("%s: JOB_STS_ERROR: error in audio flow\n", __func__);
            memcpy(&dump_job, audio_job, sizeof(struct audio_job_t));
            break;
        case JOB_STS_ENCERR:
            printk("%s: JOB_STS_ENCERR: error in audio encode flow\n", __func__);
            memcpy(&dump_job, audio_job, sizeof(struct audio_job_t));
            break;
        case JOB_STS_DECERR:
            printk("%s: JOB_STS_DECERR: error in audio decode flow\n", __func__);
            memcpy(&dump_job, audio_job, sizeof(struct audio_job_t));
            break;
        case JOB_STS_ENC_TIMEOUT:
            audio_enc_timeout[audio_job->ssp_chan] = true;
            memcpy(&dump_job, audio_job, sizeof(struct audio_job_t));
            break;
        case JOB_STS_DEC_TIMEOUT:
            audio_dec_timeout[audio_job->ssp_chan] = true;
            memcpy(&dump_job, audio_job, sizeof(struct audio_job_t));
            break;
        default:
            printk("No such job status: %d\n", audio_job->status);
            break;
    }

    if (audio_job->out_pa != 0)
        engine = PLAYBACK_ENGINE_NUM;

    pEngine = &audio_engine[engine];

    mutex_lock(&pEngine->mutex);

    /* here check whether audio_send_timeout_handler()
     * is executed before
     */
    if (list_empty(&pEngine->list)) {
        engine_busy[engine] = 0;
        mutex_unlock(&pEngine->mutex);
        return;
    }

    if (!audio_job->data)
        panic("%s: job_item is NULL\n", __func__);

    job_item = (job_item_t *)audio_job->data;
    engine = job_item->engine;
    job = (struct video_entity_job_t *)job_item->job;

    GM_LOG((GM_LOG_REC_PKT_FLOW | GM_LOG_PB_PKT_FLOW), "job(%d) with status(%d) start to callback\n", job->id, audio_job->status);

    block_time = au_vg_get_log_info(LOG_VALUE_ONE_BLOCK_TIME, 0);
    if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_ENC) {
        codec_type = (audio_job->codec_type & BITMAP_FIRST_WORD_MASK(16)) >> 16;
        job->root_time = audio_job->time_sync.jiffies_fa626;
#if 0
        job->root_time -= block_time;
        job->root_time += au_codec_info_ary[codec_type].time_adjust;
#endif
        if (JIFFIES_DIFF(now_jiffies, job->root_time) > 2000)
            job->root_time = now_jiffies;
    } else {
        if (audio_job->shared.buf_time > block_time)
            au_info_hw_queue_time = audio_job->shared.buf_time - block_time;
    }

    if (job_item->private == NULL) {
        printk("%s: job must not be freed here\n", __func__);
        GM_LOG(GM_LOG_ERROR, "job(%d) must not be freed here\n", job->id);
        engine_busy[engine] = 0;
        mutex_unlock(&pEngine->mutex);
        return;
    }

    memcpy(job_item->private, audio_job, sizeof(struct audio_job_t));
    mark_engine_finish((void *)job_item);
    driver_postprocess((void *)job_item, 0);

    /* cancel delayed_work */
    if (delayed_work_pending(&pEngine->write_work))
        cancel_delayed_work_sync(&pEngine->write_work);

    trigger_callback_finish((void *)job_item, status);
    pEngine->processed_ack ++;

#if defined(HANDLE_TIMEOUT_JOB) && defined(CONFIG_FC7500)     
    if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_ENC) {
        AU_REC_JOB_LOG_SEMA_LOCK;
        if (rec_jid_log.job_timeout_cntr > 0) {
            rec_jid_log.job_timeout_cntr = 0;
            rec_jid_log.reset_cntr++;
            GM_LOG(GM_LOG_REC_PKT_FLOW,"record timeout counter reset!\n");
        }
        AU_REC_JOB_LOG_SEMA_UNLOCK;
    } else if (ENTITY_FD_TYPE(job->fd) == TYPE_AUDIO_DEC) {
        AU_PB_JOB_LOG_SEMA_LOCK;
        if (pb_jid_log.job_timeout_cntr > 0) {
            pb_jid_log.job_timeout_cntr = 0;
            pb_jid_log.reset_cntr++;
            GM_LOG(GM_LOG_PB_PKT_FLOW,"playback timeout counter reset!\n");
        }
        AU_PB_JOB_LOG_SEMA_UNLOCK;
    }
#endif

    if (list_empty(&pEngine->list)) {
        engine_busy[engine] = 0;
        mutex_unlock(&pEngine->mutex);
        return;
    }
    /* get one from queue */
    list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
        if (job_item->status == DRIVER_STATUS_STANDBY) {
            job_item->status = DRIVER_STATUS_ONGOING;
            do_engine(job_item);
            get_job = 1;
            break;
        }

        /* flush all packets with driver_stop() */
        if (job_item->status == DRIVER_STATUS_FLUSH) {
            trigger_callback_finish((void *)job_item, DRIVER_STATUS_FAIL);
        }
    }

    /* no job anymore */
    if (!get_job)
        engine_busy[engine] = 0;

    mutex_unlock(&pEngine->mutex);
}
EXPORT_SYMBOL(audio_job_check);

#ifdef CONFIG_FC7500

static void audio_send_timeout_handler(struct work_struct *work)
{
    struct delayed_work *d_work = to_delayed_work(work);
    audio_engine_t  *pEngine = container_of(d_work, audio_engine_t, write_work);
    int engine = pEngine->num;
    job_item_t *job_item, *job_item_next;

    if (engine > ENTITY_ENGINES)
        panic("%s, bug, engine = %d \n", __func__, engine);

    if (engine == LIVESOUND_ENGINE_NUM) {
        /* it means that now it is available to receive live command. */
        if (atomic_read(&is_live_cmd_enable) == 0)
            atomic_set(&is_live_cmd_enable, 1);
    } else {
        //audio_drv_set_ecos_state_flag(true);
        
        mutex_lock(&pEngine->mutex);

        /* stop fc7500 logging and dump log data */
        if (enable_fc7500_log) {
            audio_proc_set_fc7500_log_level(0);
            audio_proc_dump_fc7500_log();
            enable_fc7500_log = false;
        }

        /* stuck fc7500 */
#if defined(HANDLE_TIMEOUT_JOB)        
        if (engine == RECORD_ENGINE_NUM) {
            AU_REC_JOB_LOG_SEMA_LOCK;
            //printk("rec job_timeout_cntr(%d)\n", rec_jid_log.job_timeout_cntr);
            if (rec_jid_log.job_timeout_cntr++ >= MAX_JOB_TIMEOUT_LIMIT) {
                GM_LOG(GM_LOG_ERROR,"record jod timeout!\n");
            	//freeze_fc7500();
            }
            AU_REC_JOB_LOG_SEMA_UNLOCK;

            if (record_timeout == 0) {
                record_timeout = 1;
                dump_pkt_flow();
            }
        } 
        else if (engine == PLAYBACK_ENGINE_NUM) {
            AU_PB_JOB_LOG_SEMA_LOCK;
            //printk("pb job_timeout_cntr(%d)\n", pb_jid_log.job_timeout_cntr);
            if (pb_jid_log.job_timeout_cntr++ >= MAX_JOB_TIMEOUT_LIMIT) {
                GM_LOG(GM_LOG_ERROR,"playback jod timeout!\n");
            	//freeze_fc7500();
            }
            AU_PB_JOB_LOG_SEMA_UNLOCK;    

            if (playback_timeout == 0) {
                playback_timeout = 1;
                dump_pkt_flow();
            }
        }
#else
        freeze_fc7500();
#endif
        if (!list_empty(&pEngine->list)) {
            pEngine->comm_rcv_failed_cnt ++;
            //printk("engine[%d]: cpu_comm rcv fail! FC7500 is still alive? \n", engine);

            list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
                if (job_item->status != DRIVER_STATUS_FINISH)
                    job_item->status = DRIVER_STATUS_FAIL;

                /* return all jobs are good */
                if (fc7500_stuck) {
                    contruct_persudo_job(job_item);
                }
            }
            callback_scheduler(engine);
        }

        engine_busy[engine] = 0;
        mutex_unlock(&pEngine->mutex);
    }
}

#if defined(HANDLE_TIMEOUT_JOB)
static bool audio_check_is_job_callbacked(struct audio_job_t *audio_job, int *job_type)
{    
    bool hit = false;
    u32 job_seq_num = audio_job->job_seq_num;

    *job_type = audio_job->job_type;

    if (*job_type == RECORD_JOB) {

        GM_LOG(GM_LOG_REC_PKT_FLOW,"record seq num check(%d), ts(%lu)\n", audio_job->job_seq_num, get_gm_jiffies());
        AU_REC_JOB_LOG_SEMA_LOCK;
        if (rec_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK] == audio_job->job_seq_num) {
            GM_LOG(GM_LOG_REC_PKT_FLOW,"%d -> record hit\n", rec_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK]);
            hit = true;
        }
        AU_REC_JOB_LOG_SEMA_UNLOCK;
    } else if (*job_type == PLAYBACK_JOB) {
        GM_LOG(GM_LOG_PB_PKT_FLOW,"playback jid check(%d), ts(%lu)\n", audio_job->job_seq_num, get_gm_jiffies());
        AU_PB_JOB_LOG_SEMA_LOCK;
        if (pb_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK] == audio_job->job_seq_num) {
            GM_LOG(GM_LOG_PB_PKT_FLOW,"%d -> play hit\n", pb_jid_log.callbacked_seq_num[job_seq_num & MAX_CB_LOG_MASK]);
                hit = true;
        }
        AU_PB_JOB_LOG_SEMA_UNLOCK;
    } else {
        GM_LOG(GM_LOG_ERROR,"%s, unknown job type : %d\n", __func__, *job_type);
    }

    return hit;
}
#endif

/*
 * Receive related
 */
static int rcv_handler(void *data)
{
    struct audio_job_t rcv_job;
    struct audio_job_t *audio_job = NULL;
    int ret = 0;
    int job_type = 0;

    if (data)   {}

    /* a forever loop to receive the respond from FC7500 */
    while (!kthread_should_stop()) {
        /* receive (1) the jobs for both playback and recorded jobs which will return to videograph
         *         (2) live sound response
         */

        ret = cpu_comm_tranceiver(CPUCOMM_RECEIVE_CH, (unsigned char *)&rcv_job, sizeof(struct audio_job_t), false, 10000); /* 10 seconds */

        if (ret)
            continue;

#if defined(HANDLE_TIMEOUT_JOB)
        if (audio_check_is_job_callbacked(&rcv_job, &job_type) == false) {
            audio_job = &rcv_job;
            audio_job_check(audio_job);
        } else {
            // the received job from 7500 which has been timeout and callbacked early
            if (job_type == RECORD_JOB) {
                rec_jid_log.hit_cntr++;
                GM_LOG(GM_LOG_REC_PKT_FLOW,"record job hit!\n");
        }
            else if (job_type == PLAYBACK_JOB) {
                pb_jid_log.hit_cntr++;
                GM_LOG(GM_LOG_PB_PKT_FLOW,"playback job hit!\n");
            } else {
                GM_LOG(GM_LOG_ERROR,"%s, unknown job type : %d\n", __func__, job_type);
            }
        }

        if (job_type == RECORD_JOB)
            record_timeout = 0;
        else if (job_type == PLAYBACK_JOB)
            playback_timeout = 0;
#else
        audio_job = &rcv_job;
        audio_job_check(audio_job);
#endif
    };

    return 0;
}

/*
 * a thread to receive heart beat from fc7500
 */
static int heart_beat_handler(void *data)
{
    cpu_comm_msg_t  msg;
    u32 command;

    if (data)   {}

    memset(&msg, 0, sizeof(cpu_comm_msg_t));
    msg.target   = CPUCOMM_HEARTBEAT_CH;
    msg.msg_data = (unsigned char *)&command;
    msg.length   = sizeof(command);

    do {
        /* FC7500 sends heart beat per second */
        if (!cpu_comm_msg_rcv(&msg, 3000)) {
            heart_beat_cnt ++;
            heart_beat_fail = 0;
            fc7500_stuck = 0;
            if (*audio_drv_get_ecos_state_flag())
                printk("FC7500 current state: 0x%08X\n", command);
        } else if (heart_beat_cnt) {   /* it was successful to receive the heart beat from fc7500 */
            heart_beat_fail ++;
            if (heart_beat_fail > 4) {
                fc7500_stuck = 1;    
                freeze_fc7500();
            }
        }
    } while(!kthread_should_stop());

    return 0;
}

#endif /* CONFIG_FC7500 */

/* under mutex protection */
static inline void do_engine(job_item_t *job_item)
{
    audio_engine_t  *pEngine = &audio_engine[job_item->engine];

    construct_audio_job(job_item);
    mark_engine_start(job_item);
    send_handler(job_item);

    pEngine->processed_cnt ++;
}

static inline void work_scheduler(int engine)
{
    audio_engine_t  *pEngine = &audio_engine[engine];
    job_item_t *job_item, *job_item_next;

    mutex_lock(&pEngine->mutex);

    /* the other thread wake up and got job already! */
    if (list_empty(&pEngine->list)) {
        mutex_unlock(&pEngine->mutex);
        return;
    }

    if (engine_busy[engine] == 0) {
        /* get one from queue */
        list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
            if (job_item->status == DRIVER_STATUS_STANDBY) {
                engine_busy[engine] = 1;
                job_item->status = DRIVER_STATUS_ONGOING;
                do_engine(job_item);
                break;
            }
        }
        /* process soon when VG puts job */
        pEngine->soon_process_cnt ++;
    } else {
        /* process later */
        pEngine->post_process_cnt ++;
    }
    mutex_unlock(&pEngine->mutex);
}

/*
 * driver_putjob()
 */
static int driver_putjob(void *parm)
{
    struct video_entity_job_t *job = (struct video_entity_job_t *)parm;
    job_item_t *job_item;
    int type = ENTITY_FD_TYPE(job->fd);
    audio_engine_t  *pEngine;
    char *p;

    if (!job->entity) {
        printk("%s: invalid job pointer\n", __func__);
        return JOB_STATUS_FAIL;
    }

    if (type != TYPE_AU_RENDER && type != TYPE_AU_GRAB) {
        printk("%s: not belong to audio type!\n", __func__);
        return JOB_STATUS_FAIL;
    }

    GM_LOG((GM_LOG_REC_PKT_FLOW | GM_LOG_PB_PKT_FLOW),"receive job(%d)\n", job->id);

    job_item = (job_item_t *)kmem_cache_alloc(job_item_cache, GFP_KERNEL);
    if (job_item == 0)
        panic("Error to putjob %s\n", MODULE_NAME);
    atomic_inc(&job_item_memory_cnt);
    memset(job_item, 0, sizeof(job_item_t));
    job_item->job = job;
    job_item->job_id = job->id;
    job_item->engine = ENTITY_FD_ENGINE(job->fd); // record engine = 0, play engine = 1
    job_item->minor = ENTITY_FD_MINOR(job->fd); // record minor = 0, play minor = ssp number from videograph

    if (job_item->engine > (ENTITY_ENGINES - 1)) {
        printk("Error to use %s engine %d, max is %d\n", MODULE_NAME, job_item->engine, ENTITY_ENGINES - 1);
        GM_LOG(GM_LOG_ERROR, "Error to use %s engine %d, max is %d\n", MODULE_NAME, job_item->engine, (ENTITY_ENGINES - 1));
        return JOB_STATUS_FAIL;
    }

    job_item->minor = ENTITY_FD_MINOR(job->fd);
    if (job_item->minor > ENTITY_MINORS) {
        printk("Error to use %s minor %d, max is %d\n", MODULE_NAME, job_item->minor, ENTITY_MINORS);
        GM_LOG(GM_LOG_ERROR, "Error to use %s minor %d, max is %d\n", MODULE_NAME, job_item->minor, ENTITY_MINORS);
        return JOB_STATUS_FAIL;
    }

    if (type == TYPE_AU_RENDER) {
        p = (char *) job->in_buf.buf[0].addr_va;
        DEBUG_M(LOG_DETAIL, job_item->engine, job_item->minor, "PJ jid(%d) in-buf[0] va(%p) len(%d): "
                "%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n",
                job->id, job->in_buf.buf[0].addr_va, job->in_buf.buf[0].size, p[0], p[1], p[2], p[3],
                p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        GM_LOG(GM_LOG_DETAIL, "PJ jid(%d) in-buf[0] va(%p) len(%d): "
                "%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x\n",
                job->id, job->in_buf.buf[0].addr_va, job->in_buf.buf[0].size, p[0], p[1], p[2], p[3],
                p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    }

    job_item->status = DRIVER_STATUS_STANDBY;
    job_item->puttime = jiffies;
    job_item->starttime = job_item->finishtime = 0;
    INIT_LIST_HEAD(&job_item->engine_list);

    if (job->next_job == 0) //last job
        job_item->need_callback = 1;
    else {
        printk("%s, audio get multi job! \n", __func__);
        return JOB_STATUS_FAIL;
    }
    job_item->root_job = (void *)job;
    job_item->type = TYPE_NORMAL_JOB;

    pEngine = &audio_engine[job_item->engine];

    mutex_lock(&pEngine->mutex);
    list_add_tail(&job_item->engine_list, &pEngine->list);
    pEngine->put_cnt ++;
    mutex_unlock(&pEngine->mutex);

    work_scheduler(job_item->engine);

    return JOB_STATUS_ONGOING;
}

/*
 * driver_stop()
 */
static int driver_stop(void *parm, int engine, int minor)
{
    audio_engine_t  *pEngine;
    job_item_t *job_item, *job_item_next = NULL;
    int i, count = 0;
    static struct audio_job_t command_job;
#if !defined(CONFIG_FC7500)
    struct workqueue_struct *audio_wq = (engine > 0 ? audio_out_wq : audio_in_wq);
    struct work_struct *work = (engine > 0 ? &command_job.out_work : &command_job.in_work);
    void (*handler)(struct work_struct *) = (engine > 0 ? audio_out_handler : audio_in_handler);
#endif

    if (engine >= ENTITY_ENGINES) {
        printk("%s, error engine: %d \n", __func__, engine);
        return -1;
    }

    memset(&command_job, 0, sizeof(struct audio_job_t));

    pEngine = &audio_engine[engine];

    is_first_dec_job = true;

    mutex_lock(&pEngine->mutex);

    pEngine->drvstop_cnt ++;

    /* go through all job list */
    list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
        if (job_item->status == DRIVER_STATUS_STANDBY) {
            job_item->status = DRIVER_STATUS_FLUSH;
            count ++;
            pEngine->flush_cnt ++;
        }
    }

    mutex_unlock(&pEngine->mutex);

    /* to ensure the previous jobs are finished */
#if defined(CONFIG_FC7500)
    if (count > 0) {
        printk("audio engne%d, flushed job list due to stop.\n", engine);
        while (!list_empty_careful(&pEngine->list))
            schedule_timeout_interruptible(5);
    }
#else
    flush_workqueue(audio_wq);
#endif

    command_job.status = (engine > 0 ? JOB_STS_PLAY_STOP : JOB_STS_REC_STOP);
    command_job.ssp_bitmap = 0;

    switch (engine) {
        case RECORD_ENGINE_NUM:
            for (i = 0;i < total_ssp_cnt;i ++)
                set_bit(i, (unsigned long *)&command_job.ssp_bitmap);
            break;
        case PLAYBACK_ENGINE_NUM:
            /* minor is ssp bitmap when driver_stop is for playback */
            command_job.ssp_bitmap = minor;
            break;
        default:
            break;
    }
#if defined(CONFIG_FC7500)
    if (!fc7500_stuck) {
        i = cpu_comm_tranceiver((engine > 0 ? CPUCOMM_PLAYBACK_CH : CPUCOMM_RECEIVE_CH),
                        (unsigned char *)&command_job, sizeof(struct audio_job_t), true, COMM_TIMEOUT);
    }
#else
    INIT_WORK(work, handler);
    queue_work(audio_wq, work);
    /* wait this work finish */
    flush_workqueue(audio_wq);
#endif

    return 0;
}

static struct property_map_t *driver_get_propertymap(int id)
{
    int i;

    for (i = 0; i < sizeof(property_map)/sizeof(struct property_map_t); i++) {
        if (property_map[i].id == id) {
            return &property_map[i];
        }
    }
    return 0;
}


static int driver_queryid(void *parm, char *str)
{
    int i;
    for (i = 0; i < sizeof(property_map)/sizeof(struct property_map_t); i++) {
        if (strcmp(property_map[i].name,str) == 0) {
            return property_map[i].id;
        }
    }
    printk("audio_queryid: Error to find name %s\n", str);
    return -1;
}


static int driver_querystr(void *parm, int id, char *string)
{
    int i;
    for (i = 0; i < sizeof(property_map)/sizeof(struct property_map_t); i++) {
        if (property_map[i].id == id) {
            memcpy(string, property_map[i].name, sizeof(property_map[i].name));
            return 0;
        }
    }
    printk("audio_querystr: Error to find id %d\n", id);
    return -1;
}

int print_info(char *page)
{
     return 0;
}

static unsigned int property_engine = 0, property_minor = 0;
static int proc_property_write_mode(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int mode_engine = 0, mode_minor = 0;
    sscanf(buffer, "%d %d", &mode_engine, &mode_minor);
    property_engine = mode_engine;
    property_minor = mode_minor;

    printk("\nLookup engine=%d minor=%d\n", property_engine, property_minor);
    return count;
}


static int proc_property_read_mode(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int i = 0;
    struct property_map_t *map;
    unsigned int id, value;
    int idx = MAKE_IDX(ENTITY_MINORS, property_engine, property_minor);


    printk("\n%s engine%d ch%d job %d\n", property_record[idx].entity->name,
        property_engine, property_minor, property_record[idx].job_id);
    printk("=============================================================\n");
    printk("ID  Name(string) Value(hex)  Readme\n");
    do {
        id = property_record[idx].property[i].id;
        if (id == ID_NULL)
            break;
        value = property_record[idx].property[i].value;
        map = driver_get_propertymap(id);
        if (map) {
            printk("%02d  %12s  %08x  %s\n", id, map->name, value, map->readme);
        }
        i++;
    } while (1);

    *eof = 1;
    return 0;
}


static unsigned int job_engine = 0, job_minor = 0;
//value=999 means all
static int proc_job_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    sscanf(buffer, "%d %d", &job_engine, &job_minor);
    printk("\nEngine=%d Minor=%d (999 means all)\n", job_engine, job_minor);
    return count;
}


static int proc_job_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int engine;
    audio_engine_t  *pEngine;
    struct video_entity_job_t *job;
    job_item_t *job_item, *job_item_next;
    char *st_string[] = {"STANDBY", "ONGOING", " FINISH", "   FAIL"};
    char *type_string[] = {"   ","(M)","(*)"}; //type==0,1 && need_callback=0, type==1 && need_callback=1
    //int idx = MAKE_IDX(ENTITY_MINORS, job_engine, job_minor);

    printk("\nEngine=%d Minor=%d (999 means all) System ticks=0x%x\n",
        job_engine, job_minor, (int)jiffies&0xffff);

    if ((job_minor == 999) || (job_engine == 999)) {
        printk("Engine Minor  Job_ID   Status   Puttime    start   end\n");
        printk("===========================================================\n");
        for (engine = 0; engine < ENTITY_ENGINES; engine++) {
            pEngine = &audio_engine[engine];

            list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list) {
                job = (struct video_entity_job_t *)job_item->job;
                printk("%d      %02d       %s%04d   %s   0x%04x   0x%04x  0x%04x\n",job_item->engine,
                    job_item->minor, type_string[(job_item->type * 0x3) & (job_item->need_callback + 1)],
                    job->id, st_string[job_item->status - 1], job_item->puttime & 0xffff,
                    job_item->starttime & 0xffff, (int)job_item->finishtime & 0xffff);
            }
        }
    }

    *eof = 1;
    return 0;
}

int print_filter(char *page)
{
    int i, len = 0;
    len += sprintf(page + len, "\nUsage:\n#echo [0:exclude/1:include] Engine Minor > filter\n");
    len += sprintf(page + len, "Driver log Include:");
    for (i = 0; i < MAX_FILTER; i++)
        if (include_filter_idx[i] >= 0)
            len += sprintf(page + len, "{%d,%d},", IDX_ENGINE(include_filter_idx[i], ENTITY_MINORS),
                IDX_MINOR(include_filter_idx[i], ENTITY_MINORS));

    len += sprintf(page + len, "\nDriver log Exclude:");
    for (i = 0; i < MAX_FILTER; i++)
        if (exclude_filter_idx[i] >= 0)
            len += sprintf(page + len, "{%d,%d},", IDX_ENGINE(exclude_filter_idx[i], ENTITY_MINORS),
                IDX_MINOR(exclude_filter_idx[i], ENTITY_MINORS));
    len += sprintf(page + len, "\n");
    return len;
}

static int vg_dbg_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int engine, len = 0;
    audio_engine_t  *pEngine;

    for (engine = 0; engine < ENTITY_ENGINES; engine ++) {
        pEngine = &audio_engine[engine];

        len += sprintf(page + len, "engine:%d put job cnt: %d \n", engine, pEngine->put_cnt);
        len += sprintf(page + len, "engine:%d processed cnt: %d \n", engine, pEngine->processed_cnt);
        len += sprintf(page + len, "engine:%d processed dma ack: %d \n", engine, pEngine->processed_ack);
        len += sprintf(page + len, "engine:%d post_process cnt: %d \n", engine, pEngine->post_process_cnt);
        len += sprintf(page + len, "engine:%d soon_process cnt: %d \n", engine, pEngine->soon_process_cnt);
        len += sprintf(page + len, "engine:%d flush cnt: %d \n", engine, pEngine->flush_cnt);
        len += sprintf(page + len, "engine:%d drv stop: %d \n", engine, pEngine->drvstop_cnt);
        len += sprintf(page + len, "engine:%d callback cnt: %d \n", engine, pEngine->callback_cnt);
        len += sprintf(page + len, "engine:%d comm snd failed cnt: %d \n", engine, pEngine->comm_snd_failed_cnt);
        len += sprintf(page + len, "engine:%d comm rcv failed cnt: %d \n", engine, pEngine->comm_rcv_failed_cnt);
        len += sprintf(page + len, "engine:%d comm live failed cnt: %d \n", engine, pEngine->comm_live_failed_cnt);
        len += sprintf(page + len, "engine:%d fc7500 delay work pending: %s \n", engine, (delayed_work_pending(&pEngine->write_work)) ? "yes" : "no");

        mutex_lock(&pEngine->mutex);
        if (!list_empty(&pEngine->list)) {
            job_item_t *job_item = NULL, *job_item_next;

            list_for_each_entry_safe (job_item, job_item_next, &pEngine->list, engine_list)
                len += sprintf(page + len, "    ---> job status: %d \n", job_item->status);
        }
        mutex_unlock(&pEngine->mutex);

        len += sprintf(page + len, "\n");
    }

    len += sprintf(page + len, "audio_job_memory_cnt: %d\njob_item_memory_cnt: %d\n", atomic_read(&audio_job_memory_cnt), atomic_read(&job_item_memory_cnt));

#ifdef CONFIG_FC7500
    len += sprintf(page + len, "fc7500_stuck: %d \n", fc7500_stuck);
    len += sprintf(page + len, "FC7500 heart beat: %d, heart beat stuck: %d \n\n", heart_beat_cnt, heart_beat_fail);
#if defined(HANDLE_TIMEOUT_JOB)
    len += sprintf(page + len, "job_log_info (REC)timeout cnt: %d\n", rec_jid_log.job_timeout_cntr);
    len += sprintf(page + len, "job_log_info (REC)hit cnt: %d\n", rec_jid_log.hit_cntr);
    len += sprintf(page + len, "job_log_info (REC)reset cnt: %d\n", rec_jid_log.reset_cntr);
    len += sprintf(page + len, "job_log_info (PB)timeout cnt: %d\n", pb_jid_log.job_timeout_cntr);
    len += sprintf(page + len, "job_log_info (PB)hit cnt: %d\n", pb_jid_log.hit_cntr);
    len += sprintf(page + len, "job_log_info (PB)reset cnt: %d\n\n", pb_jid_log.reset_cntr);
#endif

    {
        int i;
        audio_counter_t *fc7500_counter = NULL;
        fc7500_counter = (audio_counter_t *)fc7500_counter_vbase;

        len += sprintf(page + len, "FC7500 major version: %s\n", fc7500_counter->fc7500_major_ver);
        len += sprintf(page + len, "FC7500 minor version: %s\n", fc7500_counter->fc7500_minor_ver);
        len += sprintf(page + len, "DMA LISR count: %u\n", fc7500_counter->dma_lisr_cntr);
        len += sprintf(page + len, "record HISR index remain count: %u\n", fc7500_counter->record_hisr_index_remain_cntr);
        len += sprintf(page + len, "mute samples count: %u\n", fc7500_counter->mute_samples_cntr);
        //len += sprintf(page + len, "record r/w index distance: %d\n", fc7500_counter->record_rw_index_dist);
        //len += sprintf(page + len, "playback r/w index distance: %d\n", fc7500_counter->playback_rw_index_dist);
        len += sprintf(page + len, "playback hold count: %u\n", fc7500_counter->playback_hold_cntr); 
        //len += sprintf(page + len, "playback write recover count: %d\n", fc7500_counter->pb_write_recover_cntr);
        len += sprintf(page + len, "playback write recover count: ");
        for (i = 0;i < au_get_ssp_max();i ++)
            len += sprintf(page + len, "%-8u", fc7500_counter->pb_write_recover_cntr[i]);
        len += sprintf(page + len, "\n");
        len += sprintf(page + len, "playback hisr cntr: ");
        for (i = 0;i < au_get_ssp_max();i ++)
            len += sprintf(page + len, "0x%-8x  ", fc7500_counter->playback_hisr_cntr[i]);
        len += sprintf(page + len, "\n");

        len += sprintf(page + len, "record hisr cntr:   ");
        for (i = 0;i < au_get_ssp_max();i ++)
            len += sprintf(page + len, "0x%-8x  ", fc7500_counter->record_hisr_cntr[i]);
        len += sprintf(page + len, "\n");
        len += sprintf(page + len, "\n");

        for (i = 0;i < au_get_ssp_max();i ++)
            len += sprintf(page + len, "vch(%d): audio playback underrun count = %u\n", i, fc7500_counter->playback_underrun_cntr[i]);
        for (i = 0;i < au_get_ssp_max();i ++)
            len += sprintf(page + len, "vch(%d): audio record overrun count = %u\n", i, fc7500_counter->record_overrun_cntr[i]);

        len += sprintf(page + len, "\npacket flow bitmap:\n");  
        i = 0;
        do {
            len += sprintf(page + len, "0x%-8x    ", (u32)pkt_flow[i]);  
            i ++;
            if ((i % 4) == 0)
                len += sprintf(page + len, "\n");  
        } while (i < pkt_flow_sz/sizeof(unsigned long));
        len += sprintf(page + len, "\n");
    }
#else
    len += sprintf(page + len, "\n");
#endif

    *eof = 1;

    return len;
}

static int au_job_read(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    int len = 0;
    len += sprintf(page + len, "AUDIO driver_data:\n");
    len += sprintf(page + len, "ssp_chan = %d\n", dump_job.ssp_chan);
    len += sprintf(page + len, "sample_rate = %d\n", dump_job.sample_rate);
    len += sprintf(page + len, "sample_size = %d\n", dump_job.sample_size);
    len += sprintf(page + len, "channel_type = %d\n", dump_job.channel_type);
    len += sprintf(page + len, "resample_ratio = %d\n", dump_job.resample_ratio);
    len += sprintf(page + len, "status = %d\n", dump_job.status);
    len += sprintf(page + len, "jiffies_fa626 = 0x%x\n", dump_job.time_sync.jiffies_fa626);
    len += sprintf(page + len, "bitrate = 0x%x\n", dump_job.shared.bitrate);
    len += sprintf(page + len, "ssp_bitmap = 0x%x\n", dump_job.ssp_bitmap);
    len += sprintf(page + len, "codec_type = 0x%x\n", dump_job.codec_type);
    len += sprintf(page + len, "enc_blk_cnt = 0x%x\n", dump_job.enc_blk_cnt);
    len += sprintf(page + len, "dec_blk_sz = 0x%x\n", dump_job.dec_blk_sz);
    len += sprintf(page + len, "chan_bitmap = 0x%x\n", dump_job.chan_bitmap);
    len += sprintf(page + len, "bitstream_size = 0x%x\n", dump_job.bitstream_size);
    len += sprintf(page + len, "in_pa[0] = 0x%x, in_pa[1] = 0x%x\n", dump_job.in_pa[0], dump_job.in_pa[1]);
    len += sprintf(page + len, "in_size[0] = 0x%x, in_size[1] = 0x%x\n", dump_job.in_size[0], dump_job.in_size[1]);
    len += sprintf(page + len, "out_pa = 0x%x\n", dump_job.out_pa);
    len += sprintf(page + len, "out_size = 0x%x\n", dump_job.out_size);
    len += sprintf(page + len, "data ptr = %p\n", dump_job.data);
    *eof = 1;
    return len;
}

static int proc_level_write_mode(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int level;

    sscanf(buffer,"%d",&level);
    log_level=level;
    printk("\nLog level =%d (1:emerge 1:error 2:detail)\n",log_level);
    return count;
}


static int proc_level_read_mode(char *page, char **start, off_t off, int count,int *eof, void *data)
{
    printk("\nLog level =%d (1:emerge 1:error 2:detail)\n",log_level);
    *eof = 1;
    return 0;
}


struct video_entity_ops_t driver_ops ={
    putjob:      &driver_putjob,
    stop:        &driver_stop,
    queryid:     &driver_queryid,
    querystr:    &driver_querystr,
};


struct video_entity_t audio_dec_entity={
    type:       TYPE_AUDIO_DEC,
    name:       "audioDecode",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};

struct video_entity_t audio_enc_entity={
    type:       TYPE_AUDIO_ENC,
    name:       "audioEncode",
    engines:    ENTITY_ENGINES,
    minors:     ENTITY_MINORS,
    ops:        &driver_ops
};


#define ENTITY_PROC_NAME "videograph/audio"
#define DRIVER_PROC_NAME "audio"

/* behavior bitmap:
#define BEHAVIOR_BUFFER_KEEP    0
 */
int driver_vg_init(char *name, int max_num, int behavior_bitmap)
{
    int i;

    driver_entity = kzalloc(sizeof(struct video_entity_t), GFP_KERNEL);
    property_record = kzalloc(sizeof(struct property_record_t) * ENTITY_ENGINES * ENTITY_MINORS, GFP_KERNEL);

    video_entity_register(&audio_dec_entity);
    video_entity_register(&audio_enc_entity);

    memset(&audio_engine[0], 0, sizeof(audio_engine_t) * ENTITY_ENGINES);
    for (i = 0; i < ENTITY_ENGINES + 1; i++) {
        audio_engine[i].num = i;
        mutex_init(&audio_engine[i].mutex);
        INIT_LIST_HEAD(&audio_engine[i].list);
        INIT_DELAYED_WORK(&audio_engine[i].write_work, NULL);
    }

#ifdef CONFIG_FC7500
    if (!heart_beat_task) {
        heart_beat_task = kthread_run(heart_beat_handler, NULL, "heart_beat_task");
        if (IS_ERR(heart_beat_task)) {
            panic("%s: unable to create kernel thread: %ld\n",
                    __func__, PTR_ERR(heart_beat_task));
        }
    }
    if (!rcv_rec_task) {
        rcv_rec_task = kthread_run(rcv_handler, NULL, "rcv_rec_task");
        if (IS_ERR(rcv_rec_task)) {
            panic("%s: unable to create kernel thread: %ld\n",
                    __func__, PTR_ERR(rcv_rec_task));
        }
    }    
#else
    audio_in_wq = create_workqueue("audio_in");
    if (!audio_in_wq) {
        printk("%s: Error to create audio_in_wq.\n", __func__);
        return -EFAULT;
    }

    audio_out_wq = create_workqueue("audio_out");
    if (!audio_out_wq) {
        printk("%s: Error to create audio_out_wq.\n", __func__);
        return -EFAULT;
    }
#endif

    entity_proc = create_proc_entry(ENTITY_PROC_NAME, S_IFDIR | S_IRUGO | S_IXUGO, NULL);
    if (entity_proc == NULL) {
        printk("Error to create driver proc, please insert log.ko first.\n");
        return -EFAULT;
    }

    propertyproc = create_proc_entry("property", S_IRUGO | S_IXUGO, entity_proc);
    if (propertyproc == NULL)
        return -EFAULT;
    propertyproc->read_proc = (read_proc_t *)proc_property_read_mode;
    propertyproc->write_proc = (write_proc_t *)proc_property_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,33))
    propertyproc->owner = THIS_MODULE;
#endif

    jobproc = create_proc_entry("job", S_IRUGO | S_IXUGO, entity_proc);
    if (jobproc == NULL)
        return -EFAULT;
    jobproc->read_proc = (read_proc_t *)proc_job_read_mode;
    jobproc->write_proc = (write_proc_t *)proc_job_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,33))
    jobproc->owner = THIS_MODULE;
#endif

    vg_dbg_proc = create_proc_entry("debug", S_IRUGO | S_IXUGO, entity_proc);
    if (vg_dbg_proc == NULL)
        return -EFAULT;
    vg_dbg_proc->read_proc = (read_proc_t *)vg_dbg_read;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,33))
    vg_dbg_proc->owner = THIS_MODULE;
#endif

    levelproc = create_proc_entry("level", S_IRUGO | S_IXUGO, entity_proc);
    if(levelproc == NULL)
        return -EFAULT;
    levelproc->read_proc = (read_proc_t *)proc_level_read_mode;
    levelproc->write_proc = (write_proc_t *)proc_level_write_mode;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,33))
    levelproc->owner = THIS_MODULE;
#endif

    au_job_proc = create_proc_entry("au_job", S_IRUGO | S_IXUGO, entity_proc);
    if (au_job_proc == NULL)
        return -EFAULT;
    au_job_proc->read_proc = (read_proc_t *)au_job_read;
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,33))
    au_job_proc->owner = THIS_MODULE;
#endif

    memset(engine_time, 0, sizeof(unsigned int) * ENTITY_ENGINES);
    memset(engine_start, 0, sizeof(unsigned int) * ENTITY_ENGINES);
    memset(engine_end, 0, sizeof(unsigned int) * ENTITY_ENGINES);

    audio_job_cache = kmem_cache_create("audio_job_data", sizeof(struct audio_job_t), 0, 0, NULL);
    if (!audio_job_cache) {
        printk(KERN_ERR "%s: can't create audio_job_data cache\n", __func__);
        return -ENOMEM;
    }

    job_item_cache = kmem_cache_create("audio_job_item", sizeof(job_item_t), 0, 0, NULL);
    if (!job_item_cache) {
        printk(KERN_ERR "%s: can't create job_item_cache!\n", __func__);
        return -ENOMEM;
    }

    audio_job_pool = mempool_create_slab_pool(ENTITY_MINORS*ENTITY_ENGINES, audio_job_cache);
    if (!audio_job_pool) {
        kmem_cache_destroy(audio_job_cache);
        printk(KERN_ERR "%s: can't create mempool\n", __func__);
        return -ENOMEM;
    }

    for (i = 0; i < MAX_FILTER; i++) {
        include_filter_idx[i] = -1;
        exclude_filter_idx[i] = -1;
    }

    return 0;
}


void driver_vg_close(void)
{
#ifndef CONFIG_FC7500
    if (audio_in_wq) {
        flush_workqueue(audio_in_wq);
        destroy_workqueue(audio_in_wq);
    }
    if (audio_out_wq) {
        flush_workqueue(audio_out_wq);
        destroy_workqueue(audio_out_wq);
    }
#endif

    video_entity_deregister(&audio_dec_entity);
    video_entity_deregister(&audio_enc_entity);
    kfree(driver_entity);
    driver_entity = NULL;
    kfree(property_record);
    property_record = NULL;
    mempool_destroy(audio_job_pool);
    kmem_cache_destroy(audio_job_cache);
    if (job_item_cache)
        kmem_cache_destroy(job_item_cache);

    if (propertyproc != 0)
        remove_proc_entry(propertyproc->name, entity_proc);
    if (jobproc != 0)
        remove_proc_entry(jobproc->name, entity_proc);
    if (vg_dbg_proc != 0)
        remove_proc_entry(vg_dbg_proc->name, entity_proc);
    if (levelproc != 0)
        remove_proc_entry(levelproc->name, entity_proc);
    if (au_job_proc != 0)
        remove_proc_entry(au_job_proc->name, entity_proc);
    if (infoproc != 0)
        remove_proc_entry(infoproc->name, entity_proc);
    if (entity_proc != 0)
        remove_proc_entry(ENTITY_PROC_NAME, 0);
}

static dma_addr_t live_chan_num_paddr = 0;
static dma_addr_t live_on_ssp_paddr = 0;
static int __init audio_init(void)
{
    int i, ret;
    const char *codec_name = plat_audio_get_codec_name();
#ifdef CONFIG_FC7500
    const char fa626_version[VERSION_LEN] = FC7500_ECOS_VERSION;
    char ecos_version[VERSION_LEN] = {[0 ... (VERSION_LEN - 1)] = 0};
#endif
    //const char *digital_codec_name = plat_audio_get_digital_codec_name();
#if defined(CONFIG_PLATFORM_GM8210)
    fmem_pci_id_t   pci_id;
    fmem_cpu_id_t   cpu_id;

    fmem_get_identifier(&pci_id, &cpu_id);
    /* audio_drv.ko shouldn't work in RC of dual GM8210 */
    if (cpu_id != FMEM_CPU_FA626)
        return 0;
#endif

    memset(&audio_register, 0, sizeof(audio_register_t));
#if defined(CONFIG_PLATFORM_GM8210)   
    audio_register.dmac_base = ioremap_nocache(DMAC_FTDMAC020_PA_BASE, IOREMAP_SZ);
    audio_register.ssp0_base = ioremap_nocache(SSP_FTSSP010_0_PA_BASE, IOREMAP_SZ);
    audio_register.ssp1_base = ioremap_nocache(SSP_FTSSP010_1_PA_BASE, IOREMAP_SZ);
    audio_register.ssp2_base = ioremap_nocache(SSP_FTSSP010_2_PA_BASE, IOREMAP_SZ);
    audio_register.ssp3_base = ioremap_nocache(SSP_FTSSP010_3_PA_BASE, IOREMAP_SZ);
    audio_register.ssp4_base = ioremap_nocache(SSP_FTSSP010_4_PA_BASE, IOREMAP_SZ);
    audio_register.ssp5_base = ioremap_nocache(SSP_FTSSP010_5_PA_BASE, IOREMAP_SZ);
#elif defined(CONFIG_PLATFORM_GM8287)
    audio_register.dmac_base = ioremap_nocache(DMAC_FTDMAC020_PA_BASE, IOREMAP_SZ);
    audio_register.ssp0_base = ioremap_nocache(SSP_FTSSP010_0_PA_BASE, IOREMAP_SZ);
    audio_register.ssp1_base = ioremap_nocache(SSP_FTSSP010_1_PA_BASE, IOREMAP_SZ);
    audio_register.ssp2_base = ioremap_nocache(SSP_FTSSP010_2_PA_BASE, IOREMAP_SZ);
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    audio_register.dmac_base = ioremap_nocache(DMAC_FTDMAC020_PA_BASE, IOREMAP_SZ);
    audio_register.ssp0_base = ioremap_nocache(SSP_FTSSP010_0_PA_BASE, IOREMAP_SZ);
    audio_register.ssp1_base = ioremap_nocache(SSP_FTSSP010_1_PA_BASE, IOREMAP_SZ);
#endif

    atomic_set(&audio_job_memory_cnt, 0);
    atomic_set(&job_item_memory_cnt, 0);

    printk("%s: audio driver is inserted, minor version: %s\n", __func__, FA626_AU_DRV_MINOR_VERSION);

    live_chan_num = (int *)dma_alloc_writecombine(NULL, SSP_MAX_USED * sizeof(int), &live_chan_num_paddr, GFP_KERNEL);
    live_on_ssp = (int *)dma_alloc_writecombine(NULL, SSP_MAX_USED * sizeof(int), &live_on_ssp_paddr, GFP_KERNEL);

    memset(&au_param, 0, sizeof(audio_param_t));

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    // Enable SSP0 RX function as default
    audio_in_enable[0] = 1;
#endif

    for (i = 0;i < SSP_MAX_USED;i ++) {
        if (audio_ssp_chan[i] > 0) {
            total_ssp_cnt ++;
#if defined(CONFIG_PLATFORM_GM8210)
            if (pci_id == FMEM_PCI_DEV0 && audio_ssp_num[i] == 3)
                continue;
#elif defined(CONFIG_PLATFORM_GM8287)
            if (audio_ssp_num[i] == 2)
                continue;
#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
            if (audio_in_enable[i] == 0)
                continue;
#endif
            total_rec_chan += audio_ssp_chan[i];
        }
    }

    if (total_rec_chan > AUDIO_MAX_CHAN)
        total_rec_chan = AUDIO_MAX_CHAN;

    au_param.max_chan_cnt = total_rec_chan;
    memcpy(au_param.ssp_chan, audio_ssp_chan, SSP_MAX_USED * sizeof(int));
    memcpy(au_param.ssp_num, audio_ssp_num, SSP_MAX_USED * sizeof(int));
    memcpy(au_param.bit_clock, bit_clock, SSP_MAX_USED * sizeof(uint));
    memcpy(au_param.sample_rate, sample_rate, SSP_MAX_USED * sizeof(uint));
    memcpy(au_param.out_enable, audio_out_enable, SSP_MAX_USED * sizeof(int));
    memcpy(au_param.nr_enable, audio_nr_enable, SSP_MAX_USED * sizeof(int));
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    memcpy(au_param.in_enable, audio_in_enable, SSP_MAX_USED * sizeof(int));
#endif
    au_param.max_ssp_num = total_ssp_cnt;
    au_param.live_on_ssp_paddr = (u32)live_on_ssp_paddr;
    au_param.live_chan_num_paddr = (u32)live_chan_num_paddr;
    if (codec_name)
        strcpy(au_param.codec_name, codec_name);
    //if (digital_codec_name)
    //    strcpy(au_param.digital_codec_name, digital_codec_name);

    /* register proc */
    ret = audio_proc_register("audio");
    if(ret < 0) {
        printk("register audio proc node failed!\n");
        return ret;
    }

    /* proc init */
    ret = audio_proc_init();
    if (ret < 0) {
	    printk(KERN_ERR"audio_proc_init failed\n");
        audio_proc_unregister();
        return ret;
    }

#if defined(CONFIG_FC7500) && defined(HANDLE_TIMEOUT_JOB)
    memset(&jid_log, 0, sizeof(struct jid_log_info)*MAX_JID_LOG_ENGINE);
    memset(&rec_jid_log.callbacked_seq_num, 0x7FFFFF, sizeof(unsigned int)*MAX_CB_LOG_CNT);
    memset(&pb_jid_log.callbacked_seq_num, 0x7FFFFF, sizeof(unsigned int)*MAX_CB_LOG_CNT);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
    sema_init(&rec_jid_log.sema_lock, 1);
    sema_init(&pb_jid_log.sema_lock, 1);
#else
    init_MUTEX(&rec_jid_log.sema_lock);
    init_MUTEX(&pb_jid_log.sema_lock);
#endif
#endif

    memset(&dump_job, 0, sizeof(struct audio_job_t));

    au_param.pcm_show_flag_paddr = audio_proc_get_pcm_show_flag_paddr();
    au_param.play_buf_dbg_paddr = audio_proc_get_play_buf_dbg_paddr();
    au_param.rec_dbg_stage_paddr = audio_proc_get_rec_dbg_stage_paddr();
    au_param.play_dbg_stage_paddr = audio_proc_get_play_dbg_stage_paddr();
    au_param.nr_threshold_paddr = audio_proc_get_nr_threshold_paddr();

#ifdef CONFIG_FC7500
    /* open cpu comm */
    cpu_comm_open(CPUCOMM_RECEIVE_CH, COMM_NAME);
    cpu_comm_open(CPUCOMM_PLAYBACK_CH, COMM_NAME);
    cpu_comm_open(CPUCOMM_HEARTBEAT_CH, "AU_Heart");    /* heart beat channel */

    ret = cpu_comm_tranceiver(CPUCOMM_LIVE_CH, (unsigned char *)&i, sizeof(int), true, COMM_TIMEOUT);
    if (ret != 0)
        printk("%s: cpu_comm is failed for sending audio driver ready\n", __func__);

    ret = cpu_comm_tranceiver(CPUCOMM_LIVE_CH, (unsigned char *)ecos_version, VERSION_LEN * sizeof(char), false, COMM_TIMEOUT);
    if (ret != 0) {
        printk("%s: cpu_comm is failed for receiving version\n", __func__);
    } else {
        if (strncmp(fa626_version, ecos_version, VERSION_LEN) == 0)
            printk("%s: eCos and fa626 version(%s) matched, fa626 minor version(%s)\n", 
                      __func__, FC7500_ECOS_VERSION, FA626_AU_DRV_MINOR_VERSION);
        else
            panic("%s: eCos(%s) and fa626(%s) version mismatched!\n", __func__, ecos_version, fa626_version);
    }

    ret = cpu_comm_tranceiver(CPUCOMM_LIVE_CH, (unsigned char *)&log_buffer_info[0], sizeof(log_buffer_info), false, COMM_TIMEOUT);
    if (ret != 0) {
        printk("%s: cpu_comm is failed for receiving log buffer info\n", __func__);
    } else {
        //printk("%s: log buffer address: 0x%x, size: %d bytes\n", __func__, log_buffer_info[0], log_buffer_info[1]);
    }
    audio_proc_set_7500log_start_address(log_buffer_info[0], log_buffer_info[1]);
    log_buffer_vbase = (u32)ioremap_nocache(log_buffer_info[0], log_buffer_info[1]);
    if (!log_buffer_vbase)
        panic("no virtual memory for log system! pbase = 0x%x, size = %d\n", log_buffer_info[0], log_buffer_info[1]);

    // set the default FC7500 log level at first 4byte of log buffer
    *((unsigned int *)log_buffer_vbase) = ecos_log_level;

    // get fc7500 counter vbase adress
    fc7500_counter_vbase = (u32)((u32)log_buffer_vbase + sizeof(ecos_log_level));

    // get pkt_flow address
    pkt_flow = (unsigned long *)ioremap_nocache(log_buffer_info[2], log_buffer_info[3]);
    pkt_flow_sz = log_buffer_info[3];
    if (!pkt_flow)
        panic("no virtual memory for pkt_flow! pbase = 0x%x, size = %d\n", log_buffer_info[2], log_buffer_info[3]);

    ret = cpu_comm_tranceiver(CPUCOMM_LIVE_CH, (unsigned char *)&au_param, sizeof(audio_param_t), true, COMM_TIMEOUT);
    if (ret != 0)
        printk("%s: cpu_comm is failed for sending driver parameters\n", __func__);
#else

    ret = audio_flow_init();
    if(ret < 0) {
        printk(KERN_ERR"audio_flow_init() failed\n");
        audio_proc_unregister();
        return ret;
    }
#endif

    /* audio live sound initialize */
    for (i = 0;i < SSP_MAX_USED;i ++) {
        live_chan_num[i] = -1;
        live_on_ssp[i] = false;
    }
    atomic_set(&is_live_cmd_enable, 1);

    for (i = 0; i < ENTITY_ENGINES; i++)
        engine_busy[i] = 0;
    return driver_vg_init("au", 4, 0);
}

static void __exit audio_exit(void)
{
#ifdef CONFIG_FC7500
	if (!IS_ERR(rcv_rec_task))
		kthread_stop(rcv_rec_task);

    if (!IS_ERR(heart_beat_task))
		kthread_stop(heart_beat_task);

    /* close cpu comm */
    cpu_comm_close(CPUCOMM_RECEIVE_CH);
    cpu_comm_close(CPUCOMM_PLAYBACK_CH);
    cpu_comm_close(CPUCOMM_HEARTBEAT_CH);

#if defined(CONFIG_PLATFORM_GM8210)
    if (audio_register.dmac_base)
        iounmap(audio_register.dmac_base);
    if (audio_register.ssp0_base)
        iounmap(audio_register.ssp0_base);
    if (audio_register.ssp1_base)
        iounmap(audio_register.ssp1_base);
    if (audio_register.ssp2_base)
        iounmap(audio_register.ssp2_base);
    if (audio_register.ssp3_base)
        iounmap(audio_register.ssp3_base);
    if (audio_register.ssp4_base)
        iounmap(audio_register.ssp4_base);
    if (audio_register.ssp5_base)
        iounmap(audio_register.ssp5_base);
#endif
#else
    audio_flow_exit();
#endif

#if defined(CONFIG_PLATFORM_GM8287)
    if (audio_register.dmac_base)
        iounmap(audio_register.dmac_base);
    audio_register.dmac_base = NULL;

    if (audio_register.ssp0_base)
        iounmap(audio_register.ssp0_base);
    audio_register.ssp0_base = NULL;

    if (audio_register.ssp1_base)
        iounmap(audio_register.ssp1_base);
    audio_register.ssp1_base = NULL;

    if (audio_register.ssp2_base)
        iounmap(audio_register.ssp2_base);
    audio_register.ssp2_base = NULL;

#elif defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    if (audio_register.dmac_base)
        iounmap(audio_register.dmac_base);
    audio_register.dmac_base = NULL;

    if (audio_register.ssp0_base)
        iounmap(audio_register.ssp0_base);
    audio_register.ssp0_base = NULL;

    if (audio_register.ssp1_base)
        iounmap(audio_register.ssp1_base);
    audio_register.ssp1_base = NULL;
#endif

    dma_free_writecombine(NULL, SSP_MAX_USED * sizeof(int), live_chan_num, live_chan_num_paddr);
    dma_free_writecombine(NULL, SSP_MAX_USED * sizeof(int), live_on_ssp, live_on_ssp_paddr);
    audio_proc_remove();
    audio_proc_unregister();
    driver_vg_close();
}

module_init(audio_init);
module_exit(audio_exit);

MODULE_AUTHOR("Grain Media Corp.");
MODULE_LICENSE("GPL");
