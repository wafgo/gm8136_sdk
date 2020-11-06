/**
 * @file vpdrv.h
 *  vpdrv header file
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.244 $
 * $Date: 2015/01/08 07:42:40 $
 *
 * ChangeLog:
 *  $Log: vpdrv.h,v $
 *  Revision 1.244  2015/01/08 07:42:40  klchu
 *  arrange active and max_disp to original order of vpdLcdSysInfo.
 *
 *  Revision 1.243  2015/01/05 06:31:51  klchu
 *  fix max_resolution error at CVBS, use systeminfo instead.
 *
 *  Revision 1.242  2014/12/11 10:51:09  klchu
 *  add VPBUF_LDC_OUT_POOL.
 *
 *  Revision 1.241  2014/12/10 06:25:51  klchu
 *  add VPD_MAX_VIDEO_BUFFER_NUM for video buffer.
 *
 *  Revision 1.240  2014/12/09 09:26:47  klchu
 *  use rotation instead rot to code.
 *
 *  Revision 1.239  2014/12/09 07:13:33  klchu
 *  add h264e profile and qp_offset setting.
 *
 *  Revision 1.238  2014/12/09 06:59:34  klchu
 *  add enc rotation function.
 *
 *  Revision 1.237  2014/12/09 06:24:15  schumy_c
 *  Add audio underrun/overrun notify.
 *
 *  Revision 1.236  2014/11/28 12:35:35  foster_h
 *  for 1frame or 1frame_1frame mode 60P function
 *
 *  Revision 1.235  2014/11/11 02:34:04  klchu
 *  add group job id for capture.
 *
 *  Revision 1.234  2014/11/07 02:54:43  schumy_c
 *  Support scaler feature for display encode.
 *
 *  Revision 1.233  2014/10/27 08:46:05  foster_h
 *  support mjpeg rate contorl
 *
 *  Revision 1.232  2014/10/14 08:49:36  schumy_c
 *  Support display encode feature.
 *
 *  Revision 1.231  2014/10/02 09:27:47  waynewei
 *  Read the information of osd_border_pixel from vg_info of vcap for supporting GM8136.
 *
 *  Revision 1.230  2014/09/12 10:01:46  schumy_c
 *  Add scaler tag in scl_feature.
 *  Used to know the scaler position in the graph.
 *
 *  Revision 1.229  2014/08/22 10:29:35  waynewei
 *  To suppoer get_rawdata from scaler (whole image).
 *  1.Add interface for enable this feature (GM_WHOLE_FROM_ENC_OBJ)
 *
 *  Revision 1.228  2014/08/04 09:01:43  waynewei
 *  1.Fixed for the issue of error parameter happen of re-calculated crop when decoder error.
 *    To always do crop re-calculated to solve it.
 *
 *  Revision 1.227  2014/08/04 06:00:06  waynewei
 *  To support to set crop once time for different kind of resolution of bitstream on playback mode.
 *
 *  Revision 1.226  2014/07/10 10:10:13  waynewei
 *  Fixed for reset wong path of osg.
 *
 *  Revision 1.225  2014/07/09 08:05:09  foster_h
 *  support playback update crop attr function
 *
 *  Revision 1.224  2014/07/08 07:21:38  waynewei
 *  Fixed for osg disapear during encode without local channel (only remote channel)
 *  on dual_8210.
 *
 *  Revision 1.223  2014/06/20 11:41:24  waynewei
 *  1.Fixed for the issue of loss osg when scaler1_fd was free and allocated.
 *     To add notify_fd_change for scaker1_fd.
 *
 *  Revision 1.222  2014/06/20 07:29:35  waynewei
 *  1.Add the mechanism for identifying playback-datain and audio-datain for osg-enabled playback.
 *
 *  Revision 1.221  2014/06/19 07:20:08  waynewei
 *  To support osg on playback.
 *
 *  Revision 1.220  2014/06/19 06:52:46  ivan
 *  fix to handle gm_clear_bitstream for additional variable attr_vch
 *
 *  Revision 1.219  2014/06/19 02:14:10  ivan
 *  add clear_bitstream
 *
 *  Revision 1.218  2014/06/17 13:08:10  waynewei
 *  Add the mechanism for querying scl1_fd from playback graph.
 *  Function: int vplib_find_pb_scaler1_fd(int attr_vch, unsigned int *fd);
 *  vplib: ver1.0
 *
 *  Revision 1.217  2014/06/12 06:28:34  waynewei
 *  1.Add reset_osg_mark() with CPUSLV_SET_OSG_MARK_CMD for speed up osd-mark re-initiated.
 *     vplib "v0.9" / vpd_slave "v0.8" / vpd_master "v0.6" / usr_osg "v1.2"
 *
 *  Revision 1.216  2014/06/05 07:34:51  ivan
 *  add interference snapshot mode
 *
 *  Revision 1.215  2014/06/04 09:52:02  klchu
 *  add support of multi-slice function, get offset at gm_recv_multi_bitstreams.
 *
 *  Revision 1.214  2014/06/04 06:20:45  schumy_c
 *  Increase max au_grab count to 40.
 *
 *  Revision 1.213  2014/05/20 06:47:09  foster_h
 *  add gm_set_notify_sig(int sig) function for change signal number
 *
 *  Revision 1.212  2014/04/29 12:24:08  klchu
 *  use unsigned int instead of int for vg_serial.
 *
 *  Revision 1.211  2014/04/29 11:47:02  klchu
 *  add vg_serial for scaler on dual 8210.
 *
 *  Revision 1.210  2014/04/25 02:04:59  schumy_c
 *  Support change audio parameters.
 *
 *  Revision 1.209  2014/04/21 02:10:52  foster_h
 *  for gm8210_dual
 *
 *  Revision 1.208  2014/04/17 03:27:57  schumy_c
 *  Add PIP/PAP function.
 *
 *  Revision 1.207  2014/04/14 06:18:17  foster_h
 *  add reserved param for vpdGraph_t structure
 *
 *  Revision 1.205  2014/04/14 02:43:02  foster_h
 *  add reserved param for vpdGraph_t structure
 *
 *  Revision 1.204  2014/04/14 02:26:21  foster_h
 *  for fixed entity encapsulate buffer offset address err (short --> unsigned short)
 *
 *  Revision 1.203  2014/04/11 06:40:58  foster_h
 *  remove resolved param
 *
 *  Revision 1.202  2014/04/11 01:12:09  foster_h
 *  for gm_apply_attr and add resolved param
 *
 *  Revision 1.201  2014/04/10 07:45:18  zhilong
 *  Add file transcode func.
 *
 *  Revision 1.200  2014/04/07 06:24:27  schumy_c
 *  Add g.711 a-law/u-law.
 *
 *  Revision 1.199  2014/04/03 11:29:20  schumy_c
 *  Add g.711 code and interface.
 *
 *  Revision 1.198  2014/03/28 12:02:40  klchu
 *  use frame_type instead key_frame.
 *
 *  Revision 1.197  2014/03/28 11:44:03  klchu
 *  use key_frame, remove unused flag.
 *
 *  Revision 1.196  2014/03/28 10:29:23  klchu
 *  use char instead of int for ref_frame.
 *
 *  Revision 1.195  2014/03/28 10:17:10  klchu
 *  change ref_can_skip to ref_frame.
 *
 *  Revision 1.194  2014/03/28 08:51:42  klchu
 *  add checksum and frame skip functions.
 *
 *  Revision 1.193  2014/03/19 07:53:03  schumy_c
 *  Save notify time and skip it if waiting for a long time due to handler blocked.
 *
 *  Revision 1.192  2014/03/11 11:00:14  ivan
 *  update capture motion parameter interface
 *
 *  Revision 1.191  2014/03/11 06:59:53  waynewei
 *  Add interface for tuning of parameter of cap_motion.
 *
 *  Revision 1.190  2014/03/04 13:31:45  foster_h
 *  for livesound empty graph, do not set_graph and apply_graph
 *
 *  Revision 1.189  2014/02/25 13:21:53  waynewei
 *  Add sw osg for paste font image with RGB1555
 *
 *  Revision 1.188  2014/02/20 03:57:36  schumy_c
 *  Change structure member name.
 *
 *  Revision 1.187  2014/02/12 03:36:06  klchu
 *  fix get raw data method.
 *
 *  Revision 1.186  2014/02/11 11:02:44  klchu
 *  add raw attr of encode for snapshot.
 *
 *  Revision 1.185  2014/02/11 07:21:45  foster_h
 *  for cpu_comm pcie 4-align to 8-align data transfer
 *
 *  Revision 1.184  2014/02/11 06:49:21  foster_h
 *  back to previous version
 *
 *  Revision 1.183  2014/02/11 02:47:06  foster_h
 *  for reserved param
 *
 *  Revision 1.182  2014/02/11 01:28:48  waynewei
 *  Tamper detection interface:
 *  1.Add sensitive_h for homogenous sensitive
 *  2.Rename sensitive to sensitive_b
 *
 *  Revision 1.181  2014/02/07 08:38:36  zhilong
 *  Add profile item to h264e attr.
 *
 *  Revision 1.180  2014/02/06 13:35:53  waynewei
 *  1.Add inserted parameter "font_num" for osd for customizing font number for each window
 *  2.Add cap tamper interface
 *  3.fixed loss fill value from cap.ability when retrive information.
 *
 *  Revision 1.179  2014/02/06 09:34:26  schumy_c
 *  Add vlos info for system info.
 *
 *  Revision 1.178  2014/01/22 10:16:08  schumy_c
 *  Add code for tamper notification.
 *
 *  Revision 1.177  2014/01/22 04:30:18  zhilong
 *  Add mult_cap enc data struct.
 *
 *  Revision 1.176  2014/01/22 03:42:03  ivan
 *  update for new osd api
 *
 *  Revision 1.174  2014/01/22 02:12:24  ivan
 *  update time align stucture
 *
 *  Revision 1.173  2014/01/21 05:20:03  schumy_c
 *  Add time-align function.
 *
 *  Revision 1.172  2014/01/15 08:48:30  klchu
 *  use dst2_fd instead of path2 for capture.
 *
 *  Revision 1.171  2013/12/06 07:59:49  foster_h
 *  for Dual-GM8210
 *
 *  Revision 1.170  2013/11/20 09:03:01  waynewei
 *  Add cas_scaling feature and test file.
 *
 *  Revision 1.169  2013/11/18 08:46:55  schumy_c
 *  Add interface and code for notification flowing from driver to ap.
 *
 *  Revision 1.168  2013/11/14 07:44:40  waynewei
 *  1.Add usr_cap_source for hanfling usr_func
 *     the parameter "usr_func=N usr_param=M, N:func_if M:path_number" are for vpd_slave.ko
 *  2.fixed for redundant allocation of usr_getraw
 *  3.Add get_cap_source sample for ISP
 *
 *  Revision 1.167  2013/11/08 09:30:59  schumy_c
 *  Add frame_samples(only for PCM) at API for user.
 *
 *  Revision 1.166  2013/11/06 05:42:36  waynewei
 *  1.Add cap_feature to user parameter for handling P_to_I or other feature.
 *  2.In cascade_CVBS, to turn on P_to_I with cap_feature=1.
 *  3.Cancel un-used parameter (ch_num) in video_data_cap_t.
 *
 *  Revision 1.165  2013/10/31 08:09:49  schumy_c
 *  Add structures for env_update feature.
 *
 *  Revision 1.164  2013/10/25 07:20:36  klchu
 *  Add disable CVBS scheme.
 *
 *  Revision 1.163  2013/10/24 07:58:57  zhilong
 *  Add VPD_ISP.
 *
 *  Revision 1.162  2013/10/24 02:38:53  foster_h
 *  support GM8139
 *
 *  Revision 1.161  2013/10/23 11:12:39  klchu
 *  Add TYPE_YUV422_FRAME_FRAME for SDI.
 *
 *  Revision 1.160  2013/10/18 10:03:07  waynewei
 *  1.Resolved channel mapping (from user channel to capture vcapch) issue
 *     included cap_md channel.
 *
 *  Revision 1.159  2013/10/18 05:09:30  klchu
 *  add VPD_METHOD_FRAME_FRAME method.
 *
 *  Revision 1.158  2013/10/17 08:43:32  waynewei
 *  1.fixed for typo
 *  2.revise comment for ch and vcapch with the correcctness description
 *
 *  Revision 1.157  2013/10/16 02:58:23  foster_h
 *  modify max bindfd number 64 -> 128
 *
 *  Revision 1.156  2013/10/15 12:05:44  foster_h
 *  support dynamic turn ON/OFF gmlib dbg_level
 *
 *  Revision 1.155  2013/10/15 06:35:45  waynewei
 *  1.Add for checking md_src ability
 *
 *  Revision 1.154  2013/10/14 13:59:11  waynewei
 *  1.Add error message to notify user's wrong parameter to set crop.
 *  2.Fxied for cap_md to add max_x_blk_num ability check.
 *
 *  Revision 1.153  2013/10/14 09:08:53  zhilong
 *  Add max_md_x_blk and md_src to ability.
 *
 *  Revision 1.152  2013/10/09 03:29:15  schumy_c
 *  Add audio proc at master side.
 *
 *  Revision 1.151  2013/10/04 10:24:47  waynewei
 *  Fixed for log format with scaling ability info.
 *
 *  Revision 1.150  2013/10/01 11:02:39  waynewei
 *  fixed for typo
 *
 *  Revision 1.149  2013/10/01 06:14:53  foster_h
 *  modify capture connector channel mapping (cap vginfo: VCH)
 *
 *  Revision 1.148  2013/09/26 07:47:37  zhilong
 *  Remove dbgattr item.
 *
 *  Revision 1.147  2013/09/26 05:44:19  zhilong
 *  Add dbgattr item to vpdGmlibDbg_t.
 *
 *  Revision 1.146  2013/09/17 09:32:16  zhilong
 *  Add bs_width/bs_height to snapshot_t.
 *
 *  Revision 1.145  2013/09/13 12:48:59  waynewei
 *  Add capture ability for detecting error in pif.
 *
 *  Revision 1.144  2013/09/13 04:57:14  waynewei
 *  1.fixed frame_frame to frame_2frames
 *  2.fixed error log info for more readable
 *
 *  Revision 1.143  2013/09/10 11:05:58  waynewei
 *  1.to detect and do error handling when win_attr.rect.width & win_attr.rect.height was assigned under the minimum requirement.
 *  2.to detect and do error handling when crop_attr.src_crop_rect.width & crop_attr.src_crop_rect.height was assigned under the minimum requirement.
 *
 *  Revision 1.142  2013/09/10 06:48:46  foster_h
 *  add didn_mode for dec3DI (on ftdi220)
 *
 *  Revision 1.141  2013/09/10 05:48:44  foster_h
 *  take out useless code, and correct function naming
 *
 *  Revision 1.140  2013/08/30 13:11:54  foster_h
 *  add 3di property ID_CHANGE_SRC_INTERLACE, set src_interlace=0 after finish
 *
 *  Revision 1.139  2013/08/30 06:58:37  zhilong
 *  Change VPD_SPEC_MAX_ITEMS to 10.
 *
 *  Revision 1.138  2013/08/30 05:03:30  zhilong
 *  Add slice type.
 *
 *  Revision 1.137  2013/08/29 08:26:10  ivan
 *  update for 4frames
 *
 *  Revision 1.136  2013/08/29 06:27:01  schumy_c
 *  Support hdmi resample.
 *  Add av-sync code.
 *
 *  Revision 1.135  2013/08/28 01:41:13  ivan
 *  update from 3frames to 4frames
 *
 *  Revision 1.134  2013/08/26 11:31:59  ivan
 *  update to unsigned short length
 *
 *  Revision 1.133  2013/08/26 09:08:42  foster_h
 *  add output property dst_fmt TYPE_YUV422_RATIO_FRAME for 3di when decode field_cording frame (for scaler needed)
 *
 *  Revision 1.132  2013/08/25 07:06:17  foster_h
 *  add structure reserved parameter
 *
 *  Revision 1.131  2013/08/24 08:01:19  foster_h
 *  support adjust duplicate CVBS display vision
 *
 *  Revision 1.130  2013/08/21 09:34:07  waynewei
 *  1.update to support three frame mode
 *
 *  Revision 1.129  2013/08/15 06:03:08  ivan
 *  add snapshot with scaling parameter
 *
 *  Revision 1.128  2013/08/14 02:47:27  foster_h
 *  add DEC_3DI_IN pool
 *
 *  Revision 1.127  2013/08/13 03:40:16  ivan
 *  update from product_type to graph_type
 *
 *  Revision 1.126  2013/08/12 06:53:35  schumy_c
 *  Support livesound.
 *
 *  Revision 1.125  2013/08/02 03:06:29  foster_h
 *  to support codec field decode 3di -> scaling (org: scaling -> 3di)
 *
 *  Revision 1.124  2013/07/25 10:56:55  waynewei
 *  1.remove osd_border interface & structure
 *
 *  Revision 1.123  2013/07/24 06:06:09  foster_h
 *  move parameter reaction_delay_max to reserved
 *
 *  Revision 1.122  2013/07/17 09:15:11  schumy_c
 *  Support 2frame_frame and frame_frame mode.
 *
 *  Revision 1.121  2013/07/12 07:20:29  zhilong
 *  Modify aspect and border data struct.
 *
 *  Revision 1.120  2013/07/12 07:03:41  waynewei
 *  1.fixed for typo
 *  2.fixed for osd_font  definition : foreground to font, background to win
 *
 *  Revision 1.119  2013/07/11 09:57:00  klchu
 *  add buf_fps for setting file_attr.
 *
 *  Revision 1.118  2013/07/09 10:35:23  schumy_c
 *  Modify for audio record/playback.
 *
 *  Revision 1.117  2013/07/04 08:36:03  schumy_c
 *  Support ADPCM.
 *
 *  Revision 1.116  2013/07/03 02:57:56  waynewei
 *  1.remove un-used structure for cap_md
 *  2.revised log message for more friendly  in encode_with_capture_motion_detection
 *
 *  Revision 1.115  2013/07/02 11:53:51  waynewei
 *  1.Add mechanism for identifying cascade_vch.(VPD_CASCADE_CH_IDX)
 *  2.Add pack_cap_md mechanism
 *  3.Remove old cap_md
 *
 *  Revision 1.114  2013/07/02 07:28:50  foster_h
 *  modify VPD_MAX_CPU_NUM_PER_CHIP value (3) -> (2)
 *
 *  Revision 1.113  2013/06/26 07:17:56  zhilong
 *  Add auto aspect attribute to cap property.
 *
 *  Revision 1.112  2013/06/26 06:03:56  zhilong
 *  Add auto aspect attribute.
 *
 *  Revision 1.111  2013/06/24 09:23:40  schumy_c
 *  Update code for audio system info.
 *
 *  Revision 1.110  2013/06/20 10:38:05  schumy_c
 *  Update for audio flow.
 *
 *  Revision 1.109  2013/06/19 14:39:40  waynewei
 *  1.add new osd_mask for specified path from osd_font.
 *
 *  Revision 1.108  2013/06/18 05:47:36  waynewei
 *  1.fixed for update font to keep the same structure of bitmap between lib and driver.
 *
 *  Revision 1.107  2013/06/14 10:45:08  waynewei
 *  1.add update new font feature & test case to test_disp
 *
 *  Revision 1.106  2013/06/07 01:48:12  foster_h
 *  take out cpu_comm 8-bytes align
 *
 *  Revision 1.104  2013/06/06 05:32:07  foster_h
 *  for cpu_comm 8 bytes align
 *
 *  Revision 1.103  2013/06/04 07:28:02  waynewei
 *  1.cancel g_array_size
 *  2.fixed for coding style
 *  3.follow standard to handle pointer delivery to ioctl
 *  4.fixed for print message with "Error! Abnormal AP "
 *  5.dedicated to handle graph_exit without CVBS
 *
 *  Revision 1.102  2013/06/04 02:47:59  waynewei
 *  1.enable graph_exit and print message when doing error handling (graph_exit)
 *      graph_exit : prepare structure and call original pif_apply to keep correct flow.
 *  2.fixed for motion dection to consider timeout case in burn_in_test_1thread
 *  3.fixed for motion dection to consider timeout case in lv_capture_motion_detection
 *  4.fixed for compiled warning in sample_multi_enc_16/disp_16
 *  5.fixed for cap_md to prevent forever loop in some case (0 < timeout < 5) in vpdrv.c
 *  6.Don't get data when query cap_md info fail in vplib
 *  7.After query graph, to return id = 0.
 *
 *  Revision 1.101  2013/05/31 09:23:30  foster_h
 *  add cpucomm/loopcomm pack function
 *
 *  Revision 1.100  2013/05/31 05:56:14  ivan
 *  add videograph verion
 *
 *  Revision 1.99  2013/05/29 09:48:07  waynewei
 *  1.Graph_Exit has been fixed with adding CPUSLV_GET_QUERY_GRAPH
 *  and CPUSLV_PUT_QUERY_GRAPH and CPUSLV_APPLY_GRAPH_FORCE_GRAPH_EXIT
 *  ioctl command for handling is_force_graph_exit flow.
 *
 *  Revision 1.98  2013/05/28 15:02:28  waynewei
 *  cancel graph_exit (update later)
 *
 *  Revision 1.97  2013/05/27 10:02:11  waynewei
 *  1.add graph_exit feature for handling error recovery in user_space error case.
 *  2.fixed for non-initialization in err_code (pif_release)
 *  3.fixed for burn_in_test_1thread and add cap_md case
 *  4.fixed for burn_in_test (compile error)
 *
 *  Revision 1.96  2013/05/13 07:08:03  zhilong
 *  Add vpsSpecInfo struct.
 *
 *  Revision 1.95  2013/05/10 09:36:24  schumy_c
 *  Update audio code.
 *
 *  Revision 1.94  2013/05/10 07:36:57  klchu
 *  add gm_get_rawdata function.
 *
 *  Revision 1.93  2013/05/10 07:29:58  waynewei
 *  1.fixed typo in pif.c
 *  2.add cap_dst_crop for liveview-scaling-up_from_local_area (Default:off,
 *     wait for capture driver ready)
 *
 *  Revision 1.92  2013/04/30 02:01:19  waynewei
 *  1.fixed for lcd.duplicate initialization (default:-1)
 *  2.Add GRAPH_CASCADE_DISPLAY for the duplication_CVBS of cascade_cap
 *     to prevent from group multi-jobs with original GRAPH_DISPLAY.
 *
 *  Revision 1.91  2013/04/24 06:44:39  foster_h
 *  fixed lineidx with entity fd mapping error
 *
 *  Revision 1.90  2013/04/24 03:48:18  waynewei
 *  1.fixed for mark_img.mark_idx to replace from mark_img.exist
 *  2.support when mark_img.exist=0, to disable osd_mark related to mark_idx automatically.
 *
 *  Revision 1.89  2013/04/22 07:36:34  waynewei
 *  1.Restore product_type definition to pif_platform.h
 *  2.Use product_type number(<2) to the condition of duplicate LCD
 *
 *  Revision 1.88  2013/04/22 01:36:32  foster_h
 *  modify gs in/out flow rate
 *
 *  Revision 1.87  2013/04/18 06:37:11  ivan
 *  update align to 8 bytes and some coding style fix
 *
 *  Revision 1.86  2013/04/17 10:46:52  waynewei
 *  1.add duplicate_cvbs to encaps_cap_imm_to_disp
 *
 *  Revision 1.85  2013/04/16 11:50:47  foster_h
 *  fine-tune poll/getcp,\n -
 *
 *  Revision 1.84  2013/04/16 10:47:50  foster_h
 *  for data structure vpdDinBs_t, long long timestamp must be 8-bytes alignment,
 *  to get correct structure size by using differrent toolchain
 *
 *  Revision 1.83  2013/04/12 03:43:29  waynewei
 *  1.fixed for support multi-path for osd_border and osd_mark
 *
 *  Revision 1.82  2013/04/11 05:54:27  foster_h
 *  fix encoder 32ch(main+sub) err
 *
 *  Revision 1.81  2013/04/10 05:46:07  foster_h
 *  add proc file \"confg\", for tuning performance
 *
 *  Revision 1.80  2013/04/09 12:34:10  waynewei
 *  fixed for dynamic decision of micro block of capture motion detection.
 *  1024 > width , 16x16
 *  1024 < width,  32x32
 *
 *  Revision 1.79  2013/04/09 01:48:50  foster_h
 *  modify event poll structure
 *
 *  Revision 1.78  2013/04/08 10:13:48  waynewei
 *  1.remove h-flip and v-flip attribute from cap obj
 *  2.add interface for setting h-flip and v-flip with vch
 *
 *  Revision 1.77  2013/04/08 05:30:24  foster_h
 *  for poll events array rearrange
 *
 *  Revision 1.76  2013/04/04 08:32:00  foster_h
 *  optimize loop_comm command for recording
 *
 *  Revision 1.75  2013/04/03 06:35:05  waynewei
 *  1.add h_flip and v_flip
 *
 *  Revision 1.74  2013/04/03 02:15:51  zhilong
 *  Add h264e didn function.
 *
 *  Revision 1.73  2013/03/26 13:47:22  waynewei
 *  1.add osd_border
 *  2.add font_zoom to osd_font
 *
 *  Revision 1.72  2013/03/26 08:06:18  waynewei
 *  <em_test.c>
 *  1.fixed for test 6 issue in 32_engine of capture
 *  2.fixed for forgeting to free memeory after exit
 *  3.fixed for typo
 *  4.fixed for support more than 32CH (capture has 33CH)
 *  <update for script>
 *  1.add cap_md=0
 *  <cap_md>
 *  1.fixed mb from 16x16 to 32x32(to support 1920x1080)
 *  <vposd>
 *  1.fixed for cpature's bug fixed
 *  2.according to Jerson's suggestion to prevent from set the same value to alpha_t
 *
 *  Revision 1.71  2013/03/23 02:01:26  foster_h
 *  for gs graph name settings
 *
 *  Revision 1.70  2013/03/21 05:56:00  klchu
 *  change file foramt to unix format.
 *
 *  Revision 1.69  2013/03/21 05:14:35  foster_h
 *  remove get data_in mv buf
 *
 *  Revision 1.68  2013/03/19 03:10:59  waynewei
 *  1.rename mark's win_idx to mark_idx
 *  2.rename mark's yuv_buf to mark_yuv_buf
 *  3.Fixed for test_disp to support both lv and pb without re-compiled.
 *
 *  Revision 1.67  2013/03/18 03:04:01  waynewei
 *  1.alpha (transparent to alpha) (Done)
 *  2.add zoom for mark (Done)
 *  3.cancel winidx for mark (Cancel)
 *  4.cancel enum max value (Done)
 *  5.onoff to enabled (Done)
 *  6.default align_on (Done)
 *  7.pallete enum (color_ycucb to index, decided by AP) (Done)
 *  8.add load image for mark (Done)
 *
 *  Revision 1.66  2013/03/13 08:36:34  foster_h
 *  Add a property id "ID_DVR_FEATURE" to tell capture which function (liveview/recording) is running
 *
 *  Revision 1.65  2013/03/12 07:55:43  foster_h
 *  modify VPD_MAX_BUFFER_LEN for encapslate buffer
 *
 *  Revision 1.64  2013/03/07 06:42:04  ivan
 *  update for osd font structure
 *
 *  Revision 1.63  2013/03/04 07:12:54  schumy_c
 *  Add SIGUSR2 interface.
 *
 *  Revision 1.62  2013/03/01 09:10:09  foster_h
 *  for h264 decoder mbinfo pool create/layout
 *
 *  Revision 1.61  2013/02/18 10:20:21  schumy_c
 *  Modify interface for audio.
 *
 *  Revision 1.60  2013/02/07 07:09:17  foster_h
 *  add poll encoder keyframe information
 *
 *  Revision 1.59  2013/02/06 09:51:15  waynewei
 *  1.ch_num to cap_vch
 *  2.use vpslvSysInfo.capInfo to get cap_fd
 *  3.Get scl_fd has not been implemented yet.
 *
 *  Revision 1.58  2013/01/31 02:38:09  waynewei
 *  update for capture motion detection
 *
 *  Revision 1.57  2013/01/24 08:43:25  foster_h
 *  modify naming dispNum --> lcd_vch, ch_num -> cap_vch
 *
 *  Revision 1.56  2013/01/24 03:38:38  schumy_c
 *  Change name from "channels" to "channel_type" for audio.
 *
 *  Revision 1.55  2013/01/24 03:25:39  foster_h
 *  for video codec rate control type define
 *
 *  Revision 1.54  2013/01/23 02:41:39  foster_h
 *  for spec.cfg/gmlib display buffer format
 *
 *  Revision 1.53  2013/01/22 11:10:47  schumy_c
 *  Integrate audio feature.
 *
 *  Revision 1.52  2013/01/22 02:33:49  ivan
 *  add vch index comment
 *
 *  Revision 1.51  2013/01/22 01:19:52  waynewei
 *  1.add property attribute "gm_cap_md_attr_t"
 *  2.add direct access function "gm_get_cap_md_data"
 *
 *  Revision 1.50  2013/01/18 07:15:33  ivan
 *  update parameter word from max_bs_len to bs_buf_len and update decode keyframe structure
 *
 *  Revision 1.49  2013/01/17 11:10:24  foster_h
 *  for query decode_keyframe ready
 *
 *  Revision 1.48  2013/01/17 06:18:50  ivan
 *  Add RGB565 parameter
 *
 */
#ifndef __VPDRV_H__
#define __VPDRV_H__

#include "../VERSION"
#define VPD_VERSION_CODE   VG_VERSION

#define VPD_MAX_POLL_EVENT   2      //must same with 
#define VPD_POLL_READ        1      //event read ID
#define VPD_POLL_WRITE       2      //event write ID 

#define VPD_GET_POLL_IDX(event, eventIdx)   do { \
                                                switch(event) { \
                                                    case VPD_POLL_READ: (eventIdx)=0; break; \
                                                    case VPD_POLL_WRITE: (eventIdx)=1; break; \
                                                    default: \
                                                        dump_stack(); \
                                                        printk("[err]%s:%d event=%d error!\n",__FUNCTION__,__LINE__, (event)); \
                                                        printm("VP","[err]%s:%d event=%d error!\n",__FUNCTION__,__LINE__, (event)); \
                                                        damnit("VP"); \
                                                        break; \
                                                } \
                                              } while(0) 

#define VPD_RUN_STATE                       0
#define VPD_STOP_STATE                      -22

#define VPD_FLAG_NEW_FRAME_RATE  (1 << 3) ///< Indicate the bitstream of new frame rate setting
#define VPD_FLAG_NEW_GOP         (1 << 4) ///< Indicate the bitstream of new GOP setting
#define VPD_FLAG_NEW_DIM         (1 << 6) ///< Indicate the bitstream of new resolution
#define VPD_FLAG_NEW_BITRATE     (1 << 7) ///< Indicate the bitstream of new bitrate setting

#define VPD_MAX_BUFFER_LEN                      1024
#define VPD_MAX_STRING_LEN                      32
#define VPD_MAX_ENTITY_NUM                      10
#define VPD_MAX_CPU_NUM_PER_CHIP                2
#define VPD_MAX_CHIP_NUM                        4

#define VPD_MAX_GRAPH_NUM                       136

#define VPD_MAX_GROUP_NUM                       VPD_MAX_GROUP_NUM  // must same with GM_MAX_GROUP_NUM
#define VPD_MAX_BIND_NUM_PER_GROUP              128  // must same with GM_MAX_BIND_NUM_PER_GROUP
#define VPD_MAX_DUP_LINE_ENTITY_NUM             6   // must same with GM_MAX_DUP_LINE_ENTITY_NUM
#define VPD_MAX_LINE_ENTITY_NUM                 16  // must same with GM_MAX_LINE_ENTITY_NUM

#define VPD_GRAPH_LIST_MAX_ARRAY                (VPD_MAX_GRAPH_NUM+31)/32
#define VPD_MAX_GRAPH_APPLY_NUM                 2
#define VPD_MAX_BUFFER_INFO_NUM                 64
#define VPD_MAX_VIDEO_BUFFER_NUM                128
#define VPD_MAX_CAPTURE_PATH_NUM                4
#define VPD_MAX_CAPTURE_CHANNEL_NUM             128
#define VPD_MAX_LCD_NUM                         6
#define VPD_MAX_LCD_NUM_PER_CHIP                3
#define VPD_MAX_AU_GRAB_NUM                     40
#define VPD_MAX_AU_RENDER_NUM                   32
#define VPD_MAX_OSD_FONT_WINDOW                 8
#define VPD_MAX_OSD_MASK_WINDOW                 8
#define VPD_MAX_OSD_MARK_WINDOW                 4
#define VPD_MAX_OSD_PALETTE_IDX_NUM             16
#define VPD_MAX_OSD_MARK_IMG_NUM                4
#define VPD_MAX_OSD_MARK_IMG_SIZE               (2048*8)
#define VPD_MAX_OSD_FONT_STRING_LEN             256 
#define VPD_MAX_OSD_SUB_STREAM_SCALER           4   //need_porting, we don't have this hw_info, how to get
#define VPD_MAX_OSG_MARK_WINDOW                 64  // for each CH
#define VPD_MAX_CAP_MD_DATA_SIZE                4096 //4K
#define VPD_CAP_MD_MICRO_BLOCK_WIDTH_32         32
#define VPD_CAP_MD_MICRO_BLOCK_HEIGHT_32        32
#define VPD_CAP_MD_MICRO_BLOCK_WIDTH_16         16
#define VPD_CAP_MD_MICRO_BLOCK_HEIGHT_16        16
#define VPD_CAP_MD_MICRO_BLOCK_CONDITION_WIDTH  1024
#define VPD_SEPC_MAX_ITEMS           10
#define VPD_SPEC_MAX_RESOLUTION      10
#define VPD_CASCADE_CH_IDX                      32
#define VPD_3DI_WIDTH_MIN                       64
#define VPD_3DI_HEIGHT_MIN                      64
#define VPD_SCL_WIDTH_MIN                       64
#define VPD_SCL_HEIGHT_MIN                      64
#define VPD_MAX_USR_FUNC                        6
#define VPD_CrCbY_to_YCrYCb(x)                  ((((x) & 0xff) << 24) | ((x) & 0xff0000) | \
                                                (((x) & 0xff00) >> 8) | (((x) & 0xff) << 8))
#define VPD_MAX_CAP_MD_DATA_SIZE                4096 //4K
#define VPD_MAX_DATAIN_CHANNEL                  32

#define VPD_ALIGN8_UP(x)    (((x + 7) >> 3) << 3)

#define VPD_INT_MSB(value)              (((unsigned int)(value)>>16) & 0x0000FFFF)  
#define VPD_INT_LSB(value)              (((unsigned int)(value)) & 0x0000FFFF)

#define VPD_GET_LINK_MAGIC(value)       VPD_INT_MSB(value)
#define VPD_GET_LINK_LEN(value)         VPD_INT_LSB(value)

#define VPD_GET_CHIP_ID(value)          (((unsigned int)(value) >> 24) & 0x000000FF)
#define VPD_GET_CPU_ID(value)           (((unsigned int)(value) >> 16) & 0x000000FF)
#define VPD_GET_GRAPH_ID(value)         VPD_INT_LSB(value)

#define VPD_SET_GRAPH_ID(id, value)     ((id) & 0xFFFF0000) | ((value) & 0x0000FFFF)

#define VPD_GET_ENTITY_LEN(value)       (((unsigned int)(value)) & 0x000003FF)
#define VPD_GET_ENTITY_TYPE(value)      (((unsigned int)(value) >> 10) & 0x0000003F)

#define VPD_SET_LINK_HEAD(magic, len)   \
    (((((unsigned int)(magic)) << 16) & 0xFFFF0000) | (((unsigned int)(len)) & 0x0000FFFF))
#define VPD_SET_ENTITY_HEAD(magic, type, len) \
    (((((unsigned int)(magic)) << 16) & 0xFFFF0000) | (((unsigned int)(type) << 10) & 0x0000FC00)\
        |(((unsigned int)(len)) & 0x000003FF))
                    
#define VPD_SET_ID_FOR_CHIP_CPU_GRAPH(chipid, cpuid, graphid) \
    (((((unsigned int)(chipid)) << 24) & 0xFF000000) | ((((unsigned int)(cpuid)) << 16) & 0x00FF0000)|\
        (((unsigned int)(graphid)) & 0x0000FFFF))



/* refer to structure vpdGraph_t */
/* entityMagic(2) + nextEntityOffs(2) + sizeof(vpdEntity_t) + entityStructSize */
#define	VPD_ENTITY_MAGIC_LEN    sizeof(short) /* VPD_ENTITY_MAGIC_NUM */
#define	VPD_ENTITY_OFFSET_LEN   sizeof(short) 
#define	VPD_ENTITY_HEAD_LEN     (VPD_ENTITY_MAGIC_LEN + VPD_ENTITY_OFFSET_LEN) 

#define VPD_GET_ENTITY_HEAD_ADDR_BY_ENTITY(entity_addr) \
    (void *)((void *)(entity_addr) - VPD_ENTITY_HEAD_LEN)
#define VPD_GET_ENTITY_HEAD_ADDR(addr, offset)       (void *)((void *)(addr) + (offset))
#define VPD_GET_ENTITY_ENTRY_ADDR(head_addr)         \
    (void *)((void *)(head_addr) + VPD_ENTITY_HEAD_LEN)
#define VPD_GET_ENTITY_MAGIC_VAL(head_addr)          *(unsigned short *)(head_addr)
#define VPD_GET_NEXT_ENTITY_OFFSET_VAL(head_addr)    \
    *(unsigned short *)((void *)(head_addr) + VPD_ENTITY_MAGIC_LEN)
#define VPD_GET_NEXT_ENTITY_OFFSET_ADDR(head_addr)   \
    (void *)((void *)(head_addr) + VPD_ENTITY_MAGIC_LEN)

#define VPD_SET_ENTITY_MAGIC(head_addr)     *(unsigned short *)(head_addr) = VPD_ENTITY_MAGIC_NUM
#define VPD_SET_NEXT_ENTITY_OFFSET(head_addr, offs) \
    *(unsigned short *)((head_addr) + VPD_ENTITY_MAGIC_LEN) = (unsigned short) ((offs) & 0x0000ffff)

#define VPD_CALC_ENTITY_OFFSET(addr, entity_addr) \
    ((void *)(entity_addr) - (void *)(addr) - VPD_ENTITY_HEAD_LEN) 


/* refer to structure vpdGraph_t */
/* lineMagic(2) + nextlineOffset(2) + lineidx(1) + linknr(1) */
/* + {inEntityOffset(2) + outEntityOffset(2)}  + ... + {inEntityOffset(2) + outEntityOffset(2)} */
#define	VPD_LINE_MAGIC_LEN    sizeof(short) /* VPD_ENTITY_MAGIC_NUM */
#define	VPD_LINE_OFFSET_LEN   sizeof(short) 
#define	VPD_LINE_IDX_LEN      sizeof(char) 
#define	VPD_LINK_NUM_LEN      sizeof(char) 
#define	VPD_LINE_HEAD_LEN     \
            (VPD_LINE_MAGIC_LEN + VPD_LINE_OFFSET_LEN + VPD_LINE_IDX_LEN + VPD_LINK_NUM_LEN)
#define	VPD_LINK_ENTRY_LEN    sizeof(short)            /* in/out */
#define	VPD_LINK_LEN          (2 * VPD_LINK_ENTRY_LEN)   /* in + out */
#define	VPD_LINE_LEN(linknr)  VPD_LINE_HEAD_LEN + (VPD_LINK_LEN * (linknr));          

#define VPD_GET_LINE_HEAD_ADDR(addr, offset)     (void *)((void *)(addr) + (offset))
#define VPD_GET_LINE_MAGIC_VAL(head_addr)        *(unsigned short *)(void *)(head_addr)
#define VPD_GET_LINE_MAGIC_ADDR(head_addr)       (void *)(void *)(head_addr)
#define VPD_GET_NEXT_LINE_OFFSET_VAL(head_addr) \
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN)
#define VPD_GET_NEXT_LINE_OFFSET_ADDR(head_addr) \
            (void *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN)
#define VPD_GET_LINE_IDX_VAL(head_addr) \
            *(char *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN + VPD_LINE_OFFSET_LEN)
#define VPD_GET_LINK_NUM_VAL(head_addr) \
            *(char *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN + VPD_LINE_OFFSET_LEN + \
                               VPD_LINE_IDX_LEN)
#define VPD_GET_LINK_IN_VAL(head_addr, linkidx) \
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_HEAD_LEN + ((linkidx) * VPD_LINK_LEN))
#define VPD_GET_LINK_OUT_VAL(head_addr, linkidx) \
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_HEAD_LEN + \
                       ((linkidx) * VPD_LINK_LEN) + VPD_LINK_ENTRY_LEN) 

#define VPD_CALC_LINE_OFFSET(addr, head_addr)   \
            ((void *)(head_addr) - VPD_LINE_HEAD_LEN - (void *)(addr))

#define VPD_SET_LINE_MAGIC(head_addr) \
            *(unsigned short *)(head_addr) = VPD_LINE_MAGIC_NUM
#define VPD_SET_NEXT_LINE_OFFSET(head_addr, offs) \
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN) = \
                                            (unsigned short) ((offs) & 0x0000ffff)
                                            
#define VPD_SET_LINE_IDX_VAL(head_addr, lineidx) \
            *(char *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN + VPD_LINE_OFFSET_LEN) = (lineidx)
#define VPD_SET_LINK_NUM_VAL(head_addr, linknr) \
            *(char *)((void *)(head_addr) + VPD_LINE_MAGIC_LEN + VPD_LINE_OFFSET_LEN + \
                              VPD_LINE_IDX_LEN) = (linknr)
#define VPD_SET_LINK_IN_VAL(head_addr, linkidx, inoffs) \
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_HEAD_LEN + ((linkidx) * VPD_LINK_LEN)) = \
                (unsigned short)((inoffs) & 0x0000ffff)
#define VPD_SET_LINK_OUT_VAL(head_addr, linkidx, outoffs)\
            *(unsigned short *)((void *)(head_addr) + VPD_LINE_HEAD_LEN + ((linkidx) * VPD_LINK_LEN) + \
                VPD_LINK_ENTRY_LEN) = (unsigned short)((outoffs) & 0x0000ffff)

#define	VPD_LINK_MAGIC_NUM                  0x4c4B  /* char: "LK" */
#define	VPD_LINE_MAGIC_NUM                  0x4c4e  /* char: "LN" */
#define	VPD_ENTITY_MAGIC_NUM                0x4554  /* char: "ET" */

#define VPD_REC_CAP_GROUP                   0X1    /* FIXME: schumy added */
#define VPD_LV_CAP_GROUP                    0X2    /* FIXME: schumy added */
#define VPD_DISP_SCL_GROUP                  0X3    /* FIXME: schumy added */
#define VPD_AU_GRAB_GROUP                   0X4    /* FIXME: schumy added */

typedef enum vpdEntityFlowMode { 
    VPD_ENTITY_RUN_NORMAL,  // define entity flow status, normal operation
    VPD_ENTITY_NO_PJ,       // entity node disable and does not put job
    VPD_ENTITY_DIRECT_CB,   // entity node disable and direct call back
    VPD_ENTITY_DIRECT_CB_WITH_BUF,
} vpdEntityFlowMode_t;

typedef enum vpdScalerFeature { 
    //bit0~15 will pass to scaler entity
    VPD_SCALER_NOTHING    = 0,
    VPD_SCALER_SCALE      = (1 << 0),
    VPD_SCALER_CORRECTION = (1 << 1),
    VPD_SCALER_ZOOM       = (1 << 2),
    VPD_SCALER_PIPMODE    = (1 << 3),
    VPD_SCALER_RC_PCIE_ADDR_HDL = (1 << 4),
    VPD_SCALER_EP_PCIE_ADDR_HDL = (1 << 5),

    //bit16~31 will reset to 0 before send to scaler entity
    VPD_SCALER_LCD_SCALER         = (1 << 22),
    VPD_SCALER_PIC_SCALER         = (1 << 23),
    VPD_SCALER_IN_TRANSCODE_GRAPH = (1 << 24),
    VPD_SCALER_IN_ENC_GRAPH       = (1 << 25),
    VPD_SCALER_IN_MULTIWIN_GRAPH  = (1 << 26),
    VPD_SCALER_IN_DISP_GRAPH      = (1 << 27),
    VPD_SCALER_CLEAR_UPSTREAM_POS_PROP = (1 << 28),
} vpdScalerFeature_t;

typedef enum vpdBufType { 
    VPD_YUV422_FIELDS=1,
    VPD_YUV422_FRAME,
    VPD_YUV422_FRAME_2FRAMES,
    VPD_YUV422_RATIO_FRAME,
    VPD_YUV422_2FRAMES,
    VPD_YUV422_2FRAMES_2FRAMES,
    VPD_YUV422_FRAME_FRAME
} vpdBufType_t;

typedef enum {
    VPD_VRC_CBR,    ///< Constant Bitrate
    VPD_VRC_VBR,    ///< Variable Bitrate
    VPD_VRC_ECBR,   ///< Enhanced Constant Bitrate
    VPD_VRC_EVBR,   ///< Enhanced Variable Bitrate
} vpdVrcMode_t;  /* video codec rate control mode */

typedef enum vpdAuEncType { 
    VPD_AU_NONE = 0,
    VPD_AU_PCM,
    VPD_AU_ACC,
    VPD_AU_ADPCM,
    VPD_AU_G711_ALAW,
    VPD_AU_G711_ULAW,
} vpdAuEncType_t;

typedef enum vpdAuChannelType { 
    VPD_AU_MONO = 1,
    VPD_AU_STEREO = 2,
} vpdAuChannelType_t;

typedef enum vpdVideoScanMethod { 
    VPD_INTERLACED,
    VPD_PROGRESSIVE,
    VPD_RGB888,
    VPD_ISP,
} vpdVideoScanMethod_t;

typedef enum vpdGraphType {
    GRAPH_INDEPENDENCE=1, 
//    GRAPH_COMPOSITE_DISPLAY_1, 
//    GRAPH_COMPOSITE_DISPLAY_2,
    GRAPH_DISPLAY,
    GRAPH_CASCADE_DISPLAY,
    GRAPH_AU_GRAB,
    GRAPH_AU_RENDER,
    GRAPH_MULTI_ENC,
    GRAPH_AU_LIVESOUND,
    GRAPH_FILE_ENC
} vpdGraphType_t;

typedef enum vpdEntityType {
    VPD_NONE_ENTITY_TYPE=0, 
    VPD_CAP_ENTITY_TYPE, 
    VPD_DEC_ENTITY_TYPE,    // 2
    VPD_SCL_ENTITY_TYPE,
    VPD_3DI_ENTITY_TYPE,    // 4
    VPD_H264E_ENTITY_TYPE, 
    VPD_MPEG4E_ENTITY_TYPE, //6
    VPD_MJPEGE_ENTITY_TYPE,  
    VPD_DISP_ENTITY_TYPE,   // 8
    VPD_DIN_ENTITY_TYPE,    
    VPD_DOUT_ENTITY_TYPE,   // 10
    VPD_AU_GRAB_ENTITY_TYPE,
    VPD_AU_RENDER_ENTITY_TYPE,   // 12
    VPD_3DNR_ENTITY_TYPE,
    VPD_ROTATION_ENTITY_TYPE,   // 14
    VPD_TOTAL_ENTITY_CNT    /* Last one, Don't remove it */
} vpdEntityType_t;

typedef enum vpbufPoolType {
    VPBUF_DISP0_IN_POOL=0, 
    VPBUF_DISP1_IN_POOL, 
    VPBUF_DISP2_IN_POOL, 
    VPBUF_DISP_SCL_IN_POOL, 
    VPBUF_DEC_3DI_IN_POOL, 
    VPBUF_DEC_IN_POOL, 
    VPBUF_DEC_RECON_POOL, 
    VPBUF_DEC_MBINFO_POOL,
    VPBUF_ENC_CAP_OUT_POOL, 
    VPBUF_ENC_REFER_POOL, 
    VPBUF_ENC_BS_OUT_POOL, 
    VPBUF_ENC_SCL_OUT_POOL, 
    VPBUF_ENC_3DI_OUT_POOL, 
    VPBUF_AU_ENC_AU_GRAB_OUT_POOL, 
    VPBUF_AU_DEC_AU_RENDER_IN_POOL, 
    VPBUF_ENC_MULTICAP_OUT_POOL, 
    VPBUF_RAW_OUT_POOL,
    VPBUF_LDC_OUT_POOL,
    VPBUF_END_POOL      /* Last one, Don't remove it */
} vpbufPoolType_t;

typedef enum vpdClrWinMode {
    VPD_ACTIVE_BY_APPLY,
    VPD_ACTIVE_IMMEDIATELY,
} vpdClrWinMode_t;

typedef enum vpdOsdMethod {
    VPD_OSD_METHOD_CAP,
    VPD_OSD_METHOD_SCL,
    VPD_OSD_METHOD_TOTAL_CNT, /* Last one, Don't remove it */
} vpdOsdMethod_t;

typedef enum {
    VPD_SNAPSHOT_ENC_TYPE,
    VPD_SNAPSHOT_DISP_WINDOW_TYPE,
} vpdSnapshotType_t;

typedef enum vpdChannelFdStatus { 
    VPD_CHANNELFD_UNBIND_INCOMPLETE,
    VPD_CHANNELFD_UNBIND_COMPLETE,
    VPD_CHANNELFD_BIND_INCOMPLETE,
    VPD_CHANNELFD_BIND_COMPLETE,
} vpdChannelFdStatus_t;

typedef enum {
    VPD_METHOD_FRAME = 0,
    VPD_METHOD_FRAME_2FRAMES,
    VPD_METHOD_2FRAMES,
    VPD_METHOD_2FRAMES_2FRAMES,
    VPD_METHOD_FRAME_FRAME
} vpdDispGroupMethod_t;

typedef enum {
    VPD_SLICE_TYPE_P = 0,
    VPD_SLICE_TYPE_B = 1,
    VPD_SLICE_TYPE_I = 2,
    VPD_SLICE_TYPE_AUDIO = 5,
    VPD_SLICE_TYPE_RAW = 6,
} vpdSliceType_t;

/************************** sys information **************************/
typedef struct {
    unsigned int version;   // VPD_VERSION_CODE 0x0001 --> "00.01"
    char compilerDate[16];
    char compilerTime[16];
} vpdVersion_t;

typedef struct {
    char name[48]; 
    unsigned int id;
    unsigned int width;
    unsigned int height;
} vpdLcdResInfo_t;

typedef struct {
    /* 1st U32_int integer */
    int lcdid:4;    //lcdid < 0 : the entry is invalid
    unsigned int chipid:2;
    unsigned int cpuid:2;
    unsigned int is3di:1;
    unsigned int fps:9;
    unsigned int lcdType:8;
    unsigned int timestampByAp:1;
    unsigned int pooltype:5; 
    /* 2nd U32_int integer */
    int duplicate:5;  /* duplicate = -1: not duplicate, duplicate = y: duplicate of lcdid y */
    unsigned int method:4; /* vpdDispGroupMethod_t */
    int reserved2:23; /* reserved */
    /* others */
    unsigned int engineMinor;
    vpdLcdResInfo_t input_res;
    vpdLcdResInfo_t outputType;
    unsigned int fb_vaddr;
    int active;
    unsigned int max_disp_width:16;
    unsigned int max_disp_height:16;
    int reserved[3];         ///< Reserved words
} vpdLcdSysInfo_t;

#define VPD_CAP_SD_OUT_MODE 0
#define VPD_CAP_TC_OUT_MODE 1

typedef struct {
    unsigned int cascade_cnt:4;
    unsigned int mask_win_cnt:4;
    unsigned int osd_win_cnt:4;
    unsigned int mark_win_cnt:4;
    unsigned int is_scl_up_pbits:4;
    unsigned int is_scl_dn_pbits:4;    
    unsigned int max_md_x_blk:8;
    unsigned int md_src:4;              /*SD_Out:0, TC_Out:1*/
    unsigned int osd_border_pixel:4;
    unsigned int reserved: 24;
} vpdCapAbilityInfo_t;

typedef struct {
    /* 1st U32_int integer */
    int vcapch:9;   // vcapch < 0 : the entry is invalid, capture internal ch number
    unsigned int chipid:2;
    unsigned int cpuid:2;
    unsigned int numOfSplit:8;
    unsigned int numOfPath:4;
    unsigned int viMode:4;
    unsigned int scan_method:3;
    /* 2nd U32_int integer */
    unsigned int fps:9;
    int ch:9; // ch < 0 : the entry is invalid, front_end channel
    unsigned int vlos_sts:2;
    unsigned int reserved2:12;
    /* 3nd U32_int integer */
    unsigned int width:16;
    unsigned int height:16;
    /* others */
    unsigned int engineMinor;
    vpdCapAbilityInfo_t ability; 
    int reserved[4];         ///< Reserved words
} vpdCapSysInfo_t;

#define VPD_MAX_NOTIFY_FD_CHANGE  10
typedef int (*vpd_notify_fd_change_t)(int attr_vch, int fd);  //the fd is datain_fd or scaler1_fd
typedef struct { 
    int attr_vch;            ///< from file attribute
    unsigned int fd;         ///< it is a fd of datain.
    unsigned int scl1_fd;
    vpd_notify_fd_change_t   notify_fd_change[VPD_MAX_NOTIFY_FD_CHANGE];
   int reserved[10];         ///< Reserved words
} vpdDataInInfo_t;

#define VPD_SAMPLE_RATE_8K          (1 << 0)    /*  8000 Hz */
#define VPD_SAMPLE_RATE_11K         (1 << 1)    /* 11025 Hz */
#define VPD_SAMPLE_RATE_16K         (1 << 2)    /* 16000 Hz */
#define VPD_SAMPLE_RATE_22K         (1 << 3)    /* 22050 Hz */
#define VPD_SAMPLE_RATE_32K         (1 << 4)    /* 32000 Hz */
#define VPD_SAMPLE_RATE_44K         (1 << 5)    /* 44100 Hz */
#define VPD_SAMPLE_RATE_48K         (1 << 6)    /* 48000 Hz */

#define VPD_SAMPLE_SIZE_8BIT        (1 << 0)
#define VPD_SAMPLE_SIZE_16BIT       (1 << 1)

#define VPD_CHANNELS_1CH            (1 << 0)    /* mono   */
#define VPD_CHANNELS_2CH            (1 << 1)    /* stereo */

#define STRING_LEN     31
typedef struct {
    /* 1st U32_int integer */
    int ch:10;
    unsigned int chipid:2;
    unsigned int cpuid:2;
    unsigned int reserved:18;
    unsigned int frame_samples;
/*
    unsigned int sample_rate;
    unsigned int sample_size;
    unsigned int channels;
*/
    unsigned int sample_rate_type_bmp;
    unsigned int sample_size_type_bmp;
    unsigned int channels_type_bmp;
    unsigned int ssp;
    char description[STRING_LEN + 1];
    /* others */
    unsigned int engineMinor;
    int reserved2[4];         ///< Reserved words
} vpdAuGrabSysInfo_t;

typedef struct {
    /* 1st U32_int integer */
    int ch:10;
    unsigned int chipid:2;
    unsigned int cpuid:2;
    unsigned int reserved:18;
    /* 2nd U32_int integer */
    unsigned int frame_samples;
    unsigned int sample_rate_type_bmp;
    unsigned int sample_size_type_bmp;
    unsigned int channels_type_bmp;

    unsigned int ssp;
    char description[STRING_LEN + 1];
    /* others */
    unsigned int engineMinor;
    int reserved2[4];         ///< Reserved words
} vpdAuRenderSysInfo_t;

typedef struct {
    /* 1st U32_int integer */
    int b_frame_nr:4;
    unsigned int reserved:28;
    int reserved2[2];         ///< Reserved words
} vpdDecSysInfo_t;

typedef struct {
    /* 1st U32_int integer */
    int b_frame_nr:4;
    unsigned int reserved:28;
    int reserved2[2];         ///< Reserved words
} vpdEncSysInfo_t;

typedef struct  {    
    char res_type[VPD_SPEC_MAX_RESOLUTION][7];
    int max_channels[VPD_SPEC_MAX_RESOLUTION];
    int max_fps[VPD_SPEC_MAX_RESOLUTION];
}vpd_res_item_t;
typedef struct {
    char pool_name[VPD_SEPC_MAX_ITEMS][20];
    int total_channel[VPD_SEPC_MAX_ITEMS][2];  
    vpd_res_item_t res_item[VPD_SEPC_MAX_ITEMS][2];
    int reserved[4];         ///< Reserved words
} vpdSpecInfo_t;

typedef struct {
    unsigned int font_num[VPD_MAX_OSD_FONT_WINDOW];
    unsigned int reserved[8];
} vpdOsdSysInfo_t;

#define PLATFORM_INFO_ALL    0
typedef struct {
    int which;  // PLATFORM_INFO_ALL: all info
} __attribute__((packed, aligned(8))) vpdPlatformInfoRequest_t;

typedef struct {
    unsigned int chipNum;   
    unsigned int graph_type;
    char graph_name[48];
    vpdCapSysInfo_t capInfo[VPD_MAX_CAPTURE_CHANNEL_NUM];
    vpdLcdSysInfo_t lcdInfo[VPD_MAX_LCD_NUM];
    vpdAuGrabSysInfo_t auGrabInfo[VPD_MAX_AU_GRAB_NUM];
    vpdAuRenderSysInfo_t auRenderInfo[VPD_MAX_AU_RENDER_NUM];
    vpdDecSysInfo_t decInfo;
    vpdEncSysInfo_t encInfo;
    vpdSpecInfo_t specInfo;
    vpdOsdSysInfo_t osdInfo;
} __attribute__((packed, aligned(8))) vpdSysInfo_t;

typedef struct {
    int which;
} __attribute__((packed, aligned(8))) vpdEnvupdate_t;

#define INFO_TYPE_UNKNOWN               0
#define INFO_TYPE_PERF_LOG              1
#define INFO_TYPE_GMLIB_DBGMODE         3
#define INFO_TYPE_GMLIB_DBGLEVEL        4
#define INFO_TYPE_SIGNAL_LOSS           5
#define INFO_TYPE_SIGNAL_PRESENT        6
#define INFO_TYPE_FRAMERATE_CHANGE      7
#define INFO_TYPE_HW_CONFIG_CHANGE      8
#define INFO_TYPE_TAMPER_ALARM          9
#define INFO_TYPE_TAMPER_ALARM_RELEASE 10
#define INFO_TYPE_AU_BUF_UNDERRUN       11
#define INFO_TYPE_AU_BUF_OVERRUN        12

typedef struct {
    int info_type;
    unsigned long sig_jiffies;      //the time when this sig is generated
    unsigned long query_jiffies;    //the time when user get it from user space
    union {
        struct {
            unsigned int observe_time;
        } perfLogInfo;
        struct {
            unsigned int dbg_mode;
        } gmlibDbgMode;
        struct {
            unsigned int dbg_level;   
        } gmlibDbgLevel;
        struct {
            vpdEntityType_t entity_type;
            unsigned int gmlib_vch;
        } generalNotify, signalLossInfo, signalPresetInfo, framerateChangeInfo, hwConfigChange;
    };
    int reserved[4];         ///< Reserved words
} vpdSigInfo_t;

/**************************               **************************/
typedef struct vpdChannelFd {
    int lineidx; // mapping to line index of graph
    int id;     //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15) 
    int reserve[2]; // NOTE: vpdChannelFD total len = GM_FD_MAX_PRIVATE_DATA 
} vpdChannelFd_t;// NOTE: relative to gm_pollfd_t

/************************** Audio operation **************************/
#define AUCMD_BYPASS    0   //use: in_vch, out_vch, value
#define AUCMD_VOLUME    1   //use: out_vch, value
#define AUCMD_RESAMPLE  2   //use: out_vch, value
typedef struct vpdAudioCommand {
    int id;
    int cmd;
    int in_vch;
    int out_vch;
    int value;
    int reserved[4];         ///< Reserved words
}__attribute__((packed, aligned(8))) vpdAudioCommand_t;

/************************** clear window  **************************/
typedef struct vpdClrWin {
    int id; // set by gmlib, id=chipid(bit24_31)+cpuid(bit16_23), graphid(bit-0_15) is not in use
    int in_width;
    int in_height;
    vpdBufType_t    in_fmt;
    unsigned char *in_buf; 
    int lcd_vch;
    int out_x;
    int out_y;
    int out_width;
    int out_height;
    vpdClrWinMode_t  mode;
    unsigned int attr_vch;
    int reserved[3];         ///< Reserved words
}__attribute__((packed, aligned(8))) vpdClrWin_t;

/************************** CVBS display vision  **************************/
typedef struct vpdCvbsVision {
    int lcd_vch;
    int id;
    int sn;
    int out_x;
    int out_y;
    int out_width;
    int out_height;
    int reserved[4];         ///< Reserved words
}__attribute__((packed, aligned(8))) vpdCvbsVision_t;

/************************** snapshot  **************************/
typedef struct {
    vpdChannelFd_t channelfd;  // associate with library struct gm_pollfd_t:int fd_private[4]
    vpdSnapshotType_t type;
    char *bs_buf;   /* buffer point, driver will put bitstream in this buf */
    int bs_len;     /* buffer length, to tell driver the length of buf. */
    int image_quality;
    int timeoutMillisec;
    int bs_width;
    int bs_height;
    unsigned int mode; ///< for interference snapshot mode
    int reserved[3];         ///< Reserved words
}__attribute__((packed, aligned(8))) vpdSnapshot_t;

typedef struct {
   int  id;
   int  lcd_vch;
   char *bs_buf;
   int  bs_len;
   int  image_quality;
   int  timeout_millisec;
   unsigned int bs_width;
   unsigned int bs_height;
   unsigned int bs_type;    ///< 0:jpeg  1:h264 I
   unsigned int slice_type;
   int reserved[4];         ///< Reserved words
}__attribute__((packed, aligned(8))) vpdDispSnapshot_t;

/*****************************  cap motion detection (Multi) *****************************/
typedef struct vpdGetCopyCapMd {
    vpdChannelFd_t channelfd;   
    char *md_buf;                                   ///< buffer point of cap_md, set by AP
    int md_buf_len;                                 ///< length of md_buf, for check buf length whether it is enough. set the value by AP.
    int ch_num;                                     ///< capture physical channel number, set by pif from bindfd
    int is_valid;
    int md_len;                                     ///< real cap_md info size
    int md_dim_width;                               ///< MD Region width
    int md_dim_height;                              ///< MD Region height
    int md_mb_width;                                ///< MD Macro block width
    int md_mb_height;                               ///< MD Macro block height
    unsigned int md_buf_va;                         ///< MD buffer virtual addr
    int priv[2];                                    /* internal use */
}__attribute__((packed, aligned(8))) vpdGetCopyCapMd_t;

#define VPD_MD_USER_PRIVATE_DATA   10
typedef struct vpdMultiCapMd {
    int user_private[VPD_MD_USER_PRIVATE_DATA];     ///< Library user data, don't use it!, 
                                                    ///< size need same with gm_multi_cap_md_t 
                                                    ///< + bindfd + retval
    vpdGetCopyCapMd_t cap_md_vpd;
} vpdMultiCapMd_t;

typedef struct vpdCapMdInfo {
    unsigned int nr_cap_md;
    vpdMultiCapMd_t *multi_cap_md;
} vpdCapMdInfo_t;

typedef struct vpdCapMdFromDrv {
    unsigned int nr_cap_md;
    vpdGetCopyCapMd_t *multi_cap_md;
} vpdCapMdFromDrvM_t;

/*****************************  cap flip  *****************************/
typedef struct {
    int     id;                 ///<id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15)
    int     cap_vch;            ///<user channel number
    int     h_flip_enabled;
    int     v_flip_enabled;
}__attribute__((packed, aligned(8))) vpdCapFlip_t;

/*****************************  cap tamper  *****************************/
typedef struct {
    int     id;                 ///<id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15)
    int     cap_vch;            ///<user channel number
    unsigned short tamper_sensitive_b;
    unsigned short tamper_threshold;
    unsigned short tamper_sensitive_h;
}__attribute__((packed, aligned(8))) vpdCapTamper_t;

typedef struct {
    int     id;                 ///<id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15)
    int     cap_vch;            ///<user channel number
    int     motion_id;
    int     motion_value;
}__attribute__((packed, aligned(8))) vpdCapMotion_t;

/*****************************  cap get raw data  *****************************/
typedef enum {
    VPD_PARTIAL_FROM_CAP_OBJ = 0xFBFB9933,
    VPD_WHOLE_FROM_ENC_OBJ = 0xFBFC9934,
} vpdRawDataMode_t;

typedef struct {
    int                 id;
    int                 vch;
    int                 path;
    unsigned int        x;
    unsigned int        y;
    unsigned int        width;
    unsigned int        height;
    char                *yuv_buf;
    unsigned int        yuv_buf_len;
    int                 timeout_millisec;
    vpdChannelFd_t      channelfd;
    vpdRawDataMode_t    rawdata_mode;
}__attribute__((packed, aligned(8))) vpdRegionRawdata_t;

/*****************************  get cap source  *****************************/
typedef struct {
    int             cap_vch;        //user channel number
    char            *yuv_buf;       //allocatd buffer by AP
    unsigned int    yuv_buf_len;    //the length of allocated buffer
    unsigned int    width;          //output the real width from system
    unsigned int    height;         //output the real height from system
    int             timeout_millisec;   //timeout vlaue
}__attribute__((packed, aligned(8))) vpdCapSource_t;

/*****************************  get cascade scaling  *****************************/
typedef struct {
    int             cap_vch;        //user channel number
    unsigned int    ratio;          //value range:80~91, it means 80%~91%
    unsigned int    first_width;    //the width of first frame, input by AP
    unsigned int    first_height;   //the height of first frame, input by AP
    char            *yuv_buf;       //allocatd buffer by AP
    unsigned int    yuv_buf_len;    //the length of allocated buffer
    int             timeout_millisec;   //timeout value
}__attribute__((packed, aligned(8))) vpdCasScaling_t;

/*****************************  Keyframe  *****************************/
typedef struct {
   int  id;
   int  bs_width;
   int  bs_height;
   char *bs_buf;
   int  bs_len;
   int  rgb_width;
   int  rgb_height;
   char *rgb_buf;
   int  rgb_buf_len;
   int timeout_millisec;
}__attribute__((packed, aligned(8))) vpdDecodeKeyframe_t;

/*****************************  OSD  *****************************/
typedef enum {
    VPD_OSD_ALIGN_NONE = 0,
    VPD_OSD_ALIGN_TOP_LEFT,
    VPD_OSD_ALIGN_TOP_CENTER,
    VPD_OSD_ALIGN_TOP_RIGHT,
    VPD_OSD_ALIGN_BOTTOM_LEFT,
    VPD_OSD_ALIGN_BOTTOM_CENTER,
    VPD_OSD_ALIGN_BOTTOM_RIGHT,
    VPD_OSD_ALIGN_CENTER,
    VPD_OSD_ALIGN_MAX,
} vpdOsdAlignType_t;

typedef enum {
    VPD_OSD_FONT_ZOOM_NONE = 0,        ///< disable zoom
    VPD_OSD_FONT_ZOOM_2X,              ///< horizontal and vertical zoom in 2x
    VPD_OSD_FONT_ZOOM_3X,              ///< horizontal and vertical zoom in 3x
    VPD_OSD_FONT_ZOOM_4X,              ///< horizontal and vertical zoom in 4x
    VPD_OSD_FONT_ZOOM_ONE_HALF,        ///< horizontal and vertical zoom out 2x

    VPD_OSD_FONT_ZOOM_H2X_V1X = 8,     ///< horizontal zoom in 2x
    VPD_OSD_FONT_ZOOM_H4X_V1X,         ///< horizontal zoom in 4x
    VPD_OSD_FONT_ZOOM_H4X_V2X,         ///< horizontal zoom in 4x and vertical zoom in 2x

    VPD_OSD_FONT_ZOOM_H1X_V2X = 12,    ///< vertical zoom in 2x
    VPD_OSD_FONT_ZOOM_H1X_V4X,         ///< vertical zoom in 4x
    VPD_OSD_FONT_ZOOM_H2X_V4X,         ///< horizontal zoom in 2x and vertical zoom in 4x
    VPD_OSD_FONT_ZOOM_MAX
} vpdOsdFontZoom_t;

typedef struct {
    int vch;                           ///< user channel
    int ch;                            ///< capture's vcapch, it is a physical channel.
    int path;
    int is_group_job;
    int winidx;
    vpdOsdMethod_t method;
    int fd; //fd would be got from vplib.c in ioctl layer.
} vpdOsdMg_input_t;

typedef enum {
    VPD_OSD_PRIORITY_MARK_ON_OSD = 0,   ///< Mark above OSD window
    VPD_OSD_PRIORITY_OSD_ON_MARK,       ///< Mark below OSD window
    VPD_OSD_PRIORITY_MAX
} vpdOsdPriority_t;

typedef enum {
    VPD_OSD_FONT_SMOOTH_LEVEL_WEAK = 0,     ///< weak smoothing effect
    VPD_OSD_FONT_SMOOTH_LEVEL_STRONG,       ///< strong smoothing effect
    VPD_OSD_FONT_SMOOTH_LEVEL_MAX
} vpdOsdSmoothLevel_t;

typedef struct {
    int                         onoff;  ///< OSD font smooth enable/disable 0 : enable, 1 : disable
    vpdOsdSmoothLevel_t         level;  ///< OSD font smooth level
} vpdOsdSmooth_t;

typedef enum {
    VPD_OSD_MARQUEE_MODE_NONE = 0,         ///< no marueee effect
    VPD_OSD_MARQUEE_MODE_HLINE,            ///< one horizontal line marquee effect
    VPD_OSD_MARQUEE_MODE_VLINE,            ///< one vertical line marquee effect
    VPD_OSD_MARQUEE_MODE_HFLIP,            ///< one horizontal line flip effect
    VPD_OSD_MARQUEE_MODE_MAX
} vpdOsdMarqueeMode_t;

typedef enum {
    VPD_OSD_MARQUEE_LENGTH_8192 = 0,       ///< 8192 step
    VPD_OSD_MARQUEE_LENGTH_4096,           ///< 4096 step
    VPD_OSD_MARQUEE_LENGTH_2048,           ///< 2048 step
    VPD_OSD_MARQUEE_LENGTH_1024,           ///< 1024 step
    VPD_OSD_MARQUEE_LENGTH_512,            ///< 512  step
    VPD_OSD_MARQUEE_LENGTH_256,            ///< 256  step
    VPD_OSD_MARQUEE_LENGTH_128,            ///< 128  step
    VPD_OSD_MARQUEE_LENGTH_64,             ///< 64   step
    VPD_OSD_MARQUEE_LENGTH_32,             ///< 32   step
    VPD_OSD_MARQUEE_LENGTH_16,             ///< 16   step
    VPD_OSD_MARQUEE_LENGTH_8,              ///< 8    step
    VPD_OSD_MARQUEE_LENGTH_4,              ///< 4    step
    VPD_OSD_MARQUEE_LENGTH_MAX
} vpdOsdMarqueeLength_t;

typedef enum {
    VPD_FONT_ALPHA_0 = 0,               ///< alpha 0%
    VPD_FONT_ALPHA_25,                  ///< alpha 25%
    VPD_FONT_ALPHA_37_5,                ///< alpha 37.5%
    VPD_FONT_ALPHA_50,                  ///< alpha 50%
    VPD_FONT_ALPHA_62_5,                ///< alpha 62.5%
    VPD_FONT_ALPHA_75,                  ///< alpha 75%
    VPD_FONT_ALPHA_87_5,                ///< alpha 87.5%
    VPD_FONT_ALPHA_100,                 ///< alpha 100%
    VPD_FONT_ALPHA_MAX
} vpdFontAlpha_t;

typedef struct {
    vpdOsdMarqueeMode_t          mode;
    vpdOsdMarqueeLength_t        length; ///< OSD marquee length control
    int                          speed;  ///< OSD marquee speed  control, 0~3, 0:fastest, 3:slowest
} vpdOsdMarqueeParam_t;

typedef enum {
    VPD_OSD_BORDER_TYPE_WIN = 0,    ///< treat border as background
    VPD_OSD_BORDER_TYPE_FONT,              ///< treat border as font
    VPD_OSD_BORDER_TYPE_MAX
} vpdOsdBorderType_t;

typedef struct {
    int                     enable;    ///1:enable,0:disable
    int                     width;     ///< OSD window border width, 0~7, border width = 4x(n+1) pixels
    vpdOsdBorderType_t      type;      ///< OSD window border transparency type
    int                     color;     ///< OSD window border color, palette index 0~15
} vpdOsdBorderParam_t;

typedef struct {
    int id;
    int palette_table[VPD_MAX_OSD_PALETTE_IDX_NUM];
}__attribute__((packed, aligned(8))) vpdOsdPaletteTable_t;

typedef enum {
    VPD_MARK_DIM_16 = 2,       ///< 16  pixel/line
    VPD_MARK_DIM_32,           ///< 32  pixel/line
    VPD_MARK_DIM_64,           ///< 64  pixel/line
    VPD_MARK_DIM_128,          ///< 128 pixel/line
    VPD_MARK_DIM_256,          ///< 256 pixel/line
    VPD_MARK_DIM_512,          ///< 512 pixel/line
    VPD_MARK_DIM_MAX
} vpdMarkDim_t;

typedef struct {
    int  mark_exist;
    int  mark_yuv_buf_len;
    vpdMarkDim_t mark_width;
    vpdMarkDim_t mark_height;
    int  osg_tp_color;
    unsigned short osg_mark_idx;
} vpdOsdImgParam_t;

typedef struct {
    int  id;
    int  is_force_reset;
    int  ch_attr_vch;
    int  path;
    int  reserved[8];
}__attribute__((packed, aligned(8))) vpdOsgMarkCmd_t;

typedef struct {
    int id;
    vpdOsdImgParam_t mark_img[VPD_MAX_OSD_MARK_IMG_NUM];
    char *yuv_buf_total;
    int yuv_buf_total_len;
}__attribute__((packed, aligned(8))) vpdOsdMarkImgTable_t;

typedef struct {
    int id;                     //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15) 
    int linkIdx;
    vpdOsdMethod_t method; 
    int cap_vch;                // (ch_num >=0) --> captureOsd, (ch_num =-1) --> scalarOsd.
    vpdOsdMg_input_t mg_input;  // osd management
    int win_idx;
    int enabled;
    vpdOsdAlignType_t align_type;
    unsigned int x;             // x_coordinate of OSD window
    unsigned int y;             // y_coordinate of OSD window
    unsigned int h_words;       // the horizontal number of words of OSD window
    unsigned int v_words;       // the vertical number of words of OSD window
    unsigned int h_space;       //The vertical space between charater and charater
    unsigned int v_space;       //The horizontal space between charater and charater

    unsigned int    font_index_len;    // the max number of words of  OSD window
    unsigned short  *font_index;      // string

    vpdFontAlpha_t font_alpha;
    vpdFontAlpha_t win_alpha;
    int win_palette_idx;
    int font_palette_idx;
    vpdOsdPriority_t priority;
    vpdOsdSmooth_t smooth;
    vpdOsdMarqueeParam_t marquee;
    vpdOsdBorderParam_t border;
    vpdOsdFontZoom_t font_zoom;
} __attribute__((packed, aligned(8))) vpdOsdFont_t;

#define VPD_OSD_FONT_MAX_ROW 18
typedef struct {
    int   id;
    int   font_idx;                                 ///< font index, 0 ~ (osd_char_max - 1)
    unsigned short  bitmap[VPD_OSD_FONT_MAX_ROW];   ///< GM8210 only 18 row (12 bits data + 4bits reserved)
}__attribute__((packed, aligned(8))) vpdOsdFontUpdate_t;

typedef enum {
    VPD_MASK_ALPHA_0 = 0,               ///< alpha 0%
    VPD_MASK_ALPHA_25,                  ///< alpha 25%
    VPD_MASK_ALPHA_37_5,                ///< alpha 37.5%
    VPD_MASK_ALPHA_50,                  ///< alpha 50%
    VPD_MASK_ALPHA_62_5,                ///< alpha 62.5%
    VPD_MASK_ALPHA_75,                  ///< alpha 75%
    VPD_MASK_ALPHA_87_5,                ///< alpha 87.5%
    VPD_MASK_ALPHA_100,                 ///< alpha 100%
    VPD_MASK_ALPHA_MAX
} vpdMaskAlpha_t;

typedef enum {
    VPD_MASK_BORDER_TYPE_HOLLOW = 0,
    VPD_MASK_BORDER_TYPE_TRUE
} vpdMaskBorderType_t;

typedef struct {
    int                      width;      ///< Mask window border width when hollow on, 0~15, border width = 2x(n+1) pixels
    vpdMaskBorderType_t      type;       ///< Mask window hollow/true
} vpdMaskBorder_t;

typedef struct {
    int id;                             //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15) 
    int linkIdx;
    vpdOsdMethod_t method; 
    int cap_vch;                        // (ch_num >=0) --> captureOsd, (ch_num = obj) --> scalarOsd.
    vpdOsdMg_input_t mg_input;          // osd management
    int mask_idx;                       // the mask window index
    int enabled;  
    unsigned int x;                     // left position of mask window
    unsigned int y;                     // top position of mask window
    unsigned int width;                 // the dimension width of mask window
    unsigned int height;                // the dimension height of mask window
    vpdMaskAlpha_t alpha;
    int palette_idx;                    // the mask color index that assign by FIOSDS_PALTCOLOR
    vpdMaskBorder_t border;
    vpdOsdAlignType_t align_type;
    int is_all_path;
}__attribute__((packed, aligned(8))) vpdOsdMask_t;

typedef enum {
    VPD_MARK_ALPHA_0 = 0,               ///< alpha 0%
    VPD_MARK_ALPHA_25,                  ///< alpha 25%
    VPD_MARK_ALPHA_37_5,                ///< alpha 37.5%
    VPD_MARK_ALPHA_50,                  ///< alpha 50%
    VPD_MARK_ALPHA_62_5,                ///< alpha 62.5%
    VPD_MARK_ALPHA_75,                  ///< alpha 75%
    VPD_MARK_ALPHA_87_5,                ///< alpha 87.5%
    VPD_MARK_ALPHA_100,                 ///< alpha 100%
    VPD_MARK_ALPHA_MAX
} vpdMarkAlpha_t;

typedef enum {
    VPD_MARK_ZOOM_1X = 0,      ///< zoom in lx
    VPD_MARK_ZOOM_2X,          ///< zoom in 2X
    VPD_MARK_ZOOM_4X,          ///< zoom in 4x
    VPD_MARK_ZOOM_MAX
} vpdMarkZoom_t;

typedef struct {
    int id;                     //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15)
    int linkIdx;
    vpdOsdMethod_t method; 
    int attr_vch;                // capture_object: 0 ~ 127, file_object: 128 ~ 
    vpdOsdMg_input_t mg_input;  // osd management
    int mark_idx;               // the mask window index
    int enabled;  
    unsigned int x;             // left position of mask window
    unsigned int y;             // top position of mask window
    vpdMarkAlpha_t alpha;
    vpdMarkZoom_t zoom;
    vpdOsdAlignType_t align_type;
    unsigned short osg_mark_idx;
}__attribute__((packed, aligned(8))) vpdOsdMark_t;

typedef struct vpdSendLog {
    unsigned int length;   
    unsigned char *str;
} vpdSendLog_t;

typedef struct vpdEntityNotify {
    int fd;
    int type;   
} vpdEntityNotify_t;

typedef struct vpdGmlibDbg {
    unsigned int dbglevel;   
    unsigned int dbgmode;
} vpdGmlibDbg_t;

typedef struct vpdDecEntity {
    int dst_fmt;   
    int dst_bg_dim;
    int yuv_width_threshold;
    int sub_yuv_ratio;   
    int max_recon_dim;
} vpdDecEntity_t;
/* Rectangle definition */
typedef struct vpdRect {
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
} vpdRect_t;

typedef struct vpdH264eEntity {
    int dim;
    vpdVrcMode_t rate_ctl_mode;
    int init_quant;
    int max_quant;
    int min_quant;
    int bitrate;
    int bitrate_max;
    int reserved;
    int idr_interval;
    int b_frame_num;
    int enable_mv_data;
    int slice_type;
    int fps_ratio;
    int graph_ratio;
    int roi_enabled;
    int roi_x;
    int roi_y;
    int roi_w;
    int roi_h;
    int watermark_pattern;
    int vui_param_value;
    int vui_sar_value;
    int multi_slice;
    unsigned int slice_offset[3];
    int field_coding;
    int gray_scale;
    int matrix_coefficient;
    vpdRect_t roi_qp_region[8];
    int transfer_characteristics;
    int colour_primaries;
    int full_range ; 
    int video_format ;
    int timing_info_present_flag ;   
    int didn_mode;
    unsigned int profile;
    unsigned int qp_offset;
    int checksum_type;
    int fast_forward;
    int target_frame;
} vpdH264eEntity_t;

typedef struct vpdMpeg4eEntity {
    int dim;
    vpdVrcMode_t rate_ctl_mode;
    int init_quant;
    int max_quant;
    int min_quant;
    int bitrate;
    int bitrate_max;
    int reserved;
    int idr_interval;
    int slice_type;
    int fps_ratio;
    int graph_ratio;
    int roi_enabled;
    int roi_x;
    int roi_y;
    int roi_w;
    int roi_h;
    int target_frame;
} vpdMpeg4eEntity_t;

typedef struct vpdMjpegeEntity {
    int dim;
    vpdVrcMode_t rate_ctl_mode;
    int quality;
    int bitrate;
    int bitrate_max;
    int fps_ratio;
    int roi_enabled;
    int roi_x;
    int roi_y;
    int roi_w;
    int roi_h;
    int target_frame;
} vpdMjpegeEntity_t;

typedef struct vpdDataOutEntity {
    int width;
    unsigned int id;
    unsigned int slice_type;
    unsigned int bs_size;
} vpdDoutEntity_t;

typedef struct vpdDataInEntity {
    int buf_width;
    int buf_height;
    int buf_fps;
    int buf_format;
    int is_timestamp_by_ap;
    int attr_vch;
    int is_audio;
} vpdDinEntity_t;

#define MAX_PIP_WIN_COUNT   10

#define SCL_BUF_FRAME1          0x01
#define SCL_BUF_FRAME2          0x02
#define SCL_BUF_FRAME3          0x04
#define SCL_BUF_FRAME4          0x08
#define SCL_BUF_INTERNAL1       0x11
#define SCL_BUF_INTERNAL2       0x12

#define SCL_INFO_NONE           0x0
#define SCL_INFO_COPY_WHOLE     0x1
#define SCL_INFO_VALUE(from, to, info)          ((from) | ((to) << 8) | ((info) << 16))

#define LOWEST_PRIORITY           0xFF
typedef struct vpdSclEntity {
    vpdEntityFlowMode_t flowMode;
    int auto_param_enable;  //0:disable  1:enable set param from last entity result property
    vpdScalerFeature_t  scl_feature;
    int is_timesync;
    int is_osd;
    int is_crop;
    int src_xy;
    int src_dim;
    int dst_fmt;
    int dst_xy;
    int dst_bg_dim;
    int dst_dim;
    int auto_aspect;
    int border;    
    int win_xy; // noly for dec, enc don't need it
    int win_dim;// noly for dec, enc don't need it
    int dst2_bg_dim;
    int dst2_xy;
    int dst2_dim;
    unsigned int vg_serial;

    int priority;
    int pip_count;
    struct {
        int scl_info;
        int from_xy;
        int from_dim;
        int to_xy;
        int to_dim;
    } pip[MAX_PIP_WIN_COUNT];
    int attr_vch;
    int crop_xy;
    int crop_dim;
    int max_dim;
    int is_crop_enable;
    
} vpdSclEntity_t;

typedef struct vpd3DiEntity {
    int function_auto; //1:enable function, 0:disable function
    int dst_fmt;
    int change_src_interlace; //field_coding case(dec->3di): set change_src_interlace = 1
                              //to tell 3di change SRC_INTERLACE = 0, after finish 3DI 
    int didn_mode;
    unsigned int one_frame_mode_60i;  // 1: method 0 and 4,  enabled 1_frame 60i display, 
                                      // others: disabled
} vpd3DiEntity_t;

typedef struct vpd3DnrEntity {
  //  int function_auto; 
    int dst_fmt;
} vpd3DnrEntity_t;

typedef struct vpdRotationEntity {
    int clockwise;
} vpdRotationEntity_t;

typedef struct vpdDispEntity {
    int lcd_vch;
    int videograph_plane;    
    int dst_dim;
    int src_xy;
    int src_dim;
    int src_frame_rate;
} vpdDispEntity_t;

typedef struct vpdCapEntity {
    int cap_vch;        /* user channel number */
    int path;
    vpdEntityFlowMode_t flowMode;
    int is_head;        /* which means this cap is the first entity in the graph. */
    int src_fmt;
    int src_xy;
    int src_bg_dim;
    int src_dim;
    int dst_fmt;
    int dst_xy;
    int dst_bg_dim;
    int dst_dim;
    int dst_crop_xy;
    int dst_crop_dim;
    unsigned int dst2_fd; /* trigger from path and path2 for '2frame_frame' or 'frame_frame' mode and get fd*/
    int win_xy;
    int win_dim;
    int target_frame_rate;
    int fps_ratio;
    int dvr_feature;    /* 0:liveview 1:recording. To tell capture which feature is running. */
    int need_scaling;   /* do scaling by scaler entity instead of capture */
    int md_enb;         /* motion detection */
    int md_xy_start;    /* motion detection */
    int md_xy_size;     /* motion detection */
    int md_xy_num;      /* motion detection */
    int auto_aspect;
    int border;
    int dst2_crop_xy;
    int dst2_crop_dim;
    unsigned int dst2_bg_dim;
    int dst2_xy;
    int dst2_dim;
    int win2_xy;
    int win2_dim;
    int need_scaling_2;  /* do scaling by scaler entity instead of capture */
    int cap_feature;
    unsigned int one_frame_mode_60i;  // 1: method 0 and 4,  enabled 1_frame 60i display, 
                                      // others: disabled
} vpdCapEntity_t;

typedef struct vpdAuGrabEntity {
    int ch_bmp;
    int sample_rate;
    int sample_size;
    vpdAuChannelType_t channel_type;
    vpdAuEncType_t audio_type1;
    vpdAuEncType_t audio_type2;
    int bitrate1;
    int bitrate2;
    int frame_samples;
    int block_count;
    vpdEntityFlowMode_t flowMode;
} vpdAuGrabEntity_t;

typedef struct vpdAuRenderEntity {
    int ch_bmp;
    int src_frame_rate;
    int dst_frame_rate;
    int encode_type;
    int block_size;
    int sync_with_lcd_vch;
    int sample_rate;
    int sample_size;
    vpdAuChannelType_t channel_type;
    vpdEntityFlowMode_t flowMode;
} vpdAuRenderEntity_t;


typedef struct {
    unsigned short inoffs; 
    unsigned short outoffs; 
} vpdLinkEntityOffs_t;

typedef struct {
    short line_magic;
    unsigned short next_lineoffs;
    char lineidx;
    char linknr;
    vpdLinkEntityOffs_t link[VPD_MAX_LINE_ENTITY_NUM];
} vpdLine_t;

typedef struct vpdEntity {
    vpdEntityType_t type;   /* entity type, set by gmlib */
    vpbufPoolType_t poolType;
    int poolWidth;      //for video only
    int poolHeight;     //for video only
    int poolFps;        //for video only
    int gsflowRate;
    int sn;     /* sequence number, set by gmlib */
    void *e;    /* entity, set by gmlib */
    void *priv; /* entity private data (vplibEntityPriv_t), for vpd use */  
    unsigned int ap_bindfd; // application bindfd
    int reserve[3];
} vpdEntity_t;

typedef struct {
    vpdEntity_t *in; 
    vpdEntity_t *out; 
} vpdLinkEntityAddr_t;

typedef struct {
    void *line_head;
    char lineidx;
    char linknr;
    vpdLinkEntityAddr_t link[VPD_MAX_LINE_ENTITY_NUM];
} vpdGetLineInfo_t;

typedef struct {
    int valid;      // set by vpd: to check this graph is in use. 
                    // set by gmlib: bit_31(0x80000000) for gmlib force_graph_exit(abnormal graph)
                    //               bit_00 for check this graph is in use.
                    // gmlib and vpd set/maintain the valid by himself. 
    char name[20];
    int len;        // set by gmlib, total graph data size
    int id;         // set by gmlib, return a real used graphid, id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15) 
    int buf_size;   // buffer size for this group, dynamically alloc and grow up.
    vpdGraphType_t type;  // set by gmlib
    int entitynr;
    int linenr;
    void *group;    // group address, gmlib use only
    void *gPriv;    // set by vpd, GS info private data (vplibGs_t), static memory
    void *entityPriv;    // set by vpd, entity private data (vplibEntityPriv_t), dynamic alloc memory
    unsigned short entity_offs; // set by gmlib, entity data offset
    unsigned short line_offs;   // set by gmlib, line data offset
    /*  
    {entityMagic(2) + nextEntityOffs(2) + sizeof(vpdEntity_t) + entityStructSize} 
    {entityMagic(2) + nextEntityOffs(2) + sizeof(vpdEntity_t) + entityStructSize} 
        :
    lineMagic(2) + nextlineOffset(2) + lineidx(1) + linknr(1)
    + {inEntityOffset(2) + outEntityOffset(2)}  + ... + {inEntityOffset(2) + outEntityOffset(2)} 
    lineMagic(2) + nextlineOffset(2) + lineidx(1) + linknr(1)
    + {inEntityOffset(2) + outEntityOffset(2)}  + ... + {inEntityOffset(2) + outEntityOffset(2)} 
        :
    */
}__attribute__((packed, aligned(8))) vpdGraph_t;

typedef struct vpdUpdateEntity {
    int len;
    int id;  //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15) 
    vpdChannelFd_t channelfd;  // associate with library struct gm_pollfd_t:int fd_private[4]
    vpdEntity_t entity; 
    unsigned short entity_offs; // set by gmlib, entity data offset
}__attribute__((packed, aligned(8))) vpdUpdateEntity_t;

typedef struct {
    int id;
    int *g_array;
}__attribute__((packed, aligned(8))) vpdQueryGraph_t;

typedef struct vpdCpuGraphList {
    int array[VPD_GRAPH_LIST_MAX_ARRAY];              ///< graph list
}__attribute__((packed, aligned(8))) vpdCpuGraphList_t;

typedef struct vpdGraphList {
    vpdCpuGraphList_t list[VPD_MAX_CHIP_NUM][VPD_MAX_CPU_NUM_PER_CHIP];              ///< graph list
} vpdGraphList_t;

typedef struct vpdPollRetEvents {
    unsigned int event;
    unsigned int bs_len;
    unsigned int mv_len;
    unsigned int keyframe;  // 1: keyframe, 0: non-keyframe
} vpdPollRetEvents_t;

typedef struct vpdPollObj {
    void *fd;
    unsigned int event;
    vpdPollRetEvents_t revent;
    /* fd private data */  
    vpdChannelFd_t channelfd;  // associate with library struct gm_pollfd_t:int fd_private[4]
} vpdPollObj_t;

typedef struct vpdPoll {
    int timeoutMsec;   // -1 means forever
    unsigned int nrPoll;
    vpdPollObj_t *pollObjs;
} vpdPoll_t;

typedef struct vpdGraphPollDINObj {
    /* data_in and data_out entity FD */
    unsigned int fd;
} vpdGraphPollDINObj_t;

typedef struct vpdGraphPollDIN {
    /* must be set by user */
    unsigned int nr_poll;
    vpdGraphPollDINObj_t *poll_objs;
    int timeout_msec; // -1 means forever
    /* will be set by ioctl layer */
    unsigned int nr_detected;
    vpdGraphPollDINObj_t *detected_objs;
} vpdGraphPollDIN_t;

typedef struct {
    unsigned int va; // kernel virtual address of this bs
    unsigned int size; // max allowable length of bs to be copied
    unsigned int used_size; // in the end, how much memory used. should be set by user
    // NOTE: Be careful, this long long timestamp must 4-bytes alignment. for other toolchain. 
    unsigned long long timestamp __attribute__((packed, aligned(4))); 
} vpdDinBs_t; 

typedef struct {
    /* for datain users to check and set */
    vpdDinBs_t bs;
    /* internal use */
    void    *priv;
} vpdDinBuf_t;

typedef struct {
    unsigned int mv_va; // kernel virtual address of this bs
    unsigned int mv_len;
    unsigned int bs_va; // kernel virtual address of this bs
    unsigned int bs_len;
    unsigned int new_bs; // new bitstream flag
    unsigned int bistream_type;
    unsigned char frame_type;
    unsigned char ref_frame;
    unsigned char reserved[2];
    unsigned int checksum;
    unsigned int timestamp;// user's bs timestamp must be set to this field
    unsigned int slice_offset[3]; //multi-slice offset 1~3 (first offset is 0) 
    unsigned int reserved2;
} vpdDoutBs_t; 

typedef struct {
    /* for dataout users to check and set */
    vpdDoutBs_t bs;
    /* internal use */
    void    *priv; /* internal use */
} vpdDoutBuf_t;

typedef struct vpdPutCopyDin {
    vpdChannelFd_t channelfd;
    unsigned int bs_len;     // length of bitstream from AP
    char *bs_buf;   // buffer point of bitstream from AP
    unsigned int time_align_by_user; // time align value in micro-second, 0 means disable
    int timeout_msec; // timeout in ms, -1 means no poll 
    vpdDinBuf_t dinBuf;  // data_in handle
    unsigned int reserved[8];
} vpdPutCopyDin_t;

#define VPD_DEC_USER_PRIVATE_DATA         10
typedef struct vpdMultiDin {
    // NOTE: check 8bytes align for (* vpdDinBs_t)->timestamp
    int user_private[VPD_DEC_USER_PRIVATE_DATA];   ///< Library user data, don't use it!, 
                                                   ///< size user part of gm_dec_multi_bitstream_t 
                                                   ///<  *bindfd + *buf + length + retval = 4 (int)
    vpdPutCopyDin_t din;
} vpdMultiDin_t;

typedef struct vpdPutCopyMultiBs {
    // NOTE: check 8bytes align for (* vpdDinBs_t)->timestamp
    unsigned int nr_bs;
    int timeout_millisec;
    vpdMultiDin_t *multi_bs;
} vpdPutCopyMultiBs_t;

typedef struct vpdPutCopyMultiDin {
    // NOTE: check 8bytes align for (* vpdDinBs_t)->timestamp
    unsigned int nr_bs;
    unsigned int reserved;  // for long long timestamp 8 bytes alignment
    vpdPutCopyDin_t *multi_din;
} vpdPutCopyMultiDin_t;

typedef struct vpdGetCopyDout {
    vpdChannelFd_t channelfd;
    unsigned int bs_buf_len;    // length of bs_buf, for check buf length whether it is enough. set the value by AP.
    char *bs_buf;            // buffer point of bitstream, set by AP
    unsigned int mv_buf_len;    // length of mv_buf, for check buf length whether it is enough. set the value by AP.
    char *mv_buf;            // mv buffer point of bitstream, set by AP
    vpdDoutBuf_t doutBuf;
    /* internal use */
    int    priv[2]; /* internal use */
    unsigned int reserved[4];
} vpdGetCopyDout_t;

#define VPD_ENC_USER_PRIVATE_DATA         27
typedef struct vpdMultiDout {
    int user_private[VPD_ENC_USER_PRIVATE_DATA];   ///< Library user data, don't use it!, 
                                                   ///< size need same with gm_enc_bitstream_t 
                                                   ///< + bindfd + retval
    vpdGetCopyDout_t dout;
} vpdMultiDout_t;

typedef struct vpdGetCopyMultiBs {
    unsigned int nr_bs;
    vpdMultiDout_t *multi_bs;
} vpdGetCopyMultiBs_t;

typedef struct vpdGetCopyMultiDout {
    unsigned int nr_bs;
    vpdGetCopyDout_t *multi_dout;
} vpdGetCopyMultiDout_t;

typedef struct vpdGraphGetDIN {
    /* must be set by user */
    vpdGraphPollDINObj_t which;
    /* will be set by ioctl layer */
    unsigned int offset; // start address to fill bs
    unsigned int length; // max allowable length of bs
} vpdGraphGetDIN_t;

typedef struct vpdGraphPutDIN {
    /* must be set by user */
    vpdGraphPollDINObj_t which;
    // the length filled for bs, this can not be over vpdGraphGetDti_t::length
    unsigned int used_length; 
    unsigned int timestamp;// user's bs timestamp must be set to this field
} vpdGraphPutDIN_t;

typedef struct vpdGraphiPutCopyDIN {
    /* must be set by user */
    vpdGraphPollDINObj_t which;
    unsigned char *bs_buf;
    unsigned int bs_length;
} vpdGraphPutCopyDIN_t;

typedef struct vpdGraphGetVecDIN {
    unsigned int nr_get; 
    vpdGraphGetDIN_t *to_get_vec;
} vpdGraphGetVecDIN_t;

typedef struct vpdGraphPutVecDIN {
    unsigned int nr_put; 
    vpdGraphPutDIN_t *to_put_vec;
} vpdGraphPutVecDIN_t;

typedef struct vpdGraphPutCopyVecDIN {
    unsigned int nr_put_copy; 
    vpdGraphPutCopyDIN_t *to_put_copy_vec;
} vpdGraphPutCopyVecDIN_t;

#define VPD_IOC_MAGIC  'V' 
#define VPD_SET_GRAPH                           _IOWR(VPD_IOC_MAGIC, 1, vpdGraph_t)
#define VPD_APPLY                               _IOW(VPD_IOC_MAGIC, 2, vpdGraphList_t)
#define VPD_POLL                                _IOWR(VPD_IOC_MAGIC, 5, vpdPoll_t)
#define VPD_GET_COPY_MULTI_DOUT                 _IOWR(VPD_IOC_MAGIC, 6, vpdGetCopyMultiBs_t) 
#define VPD_PUT_COPY_MULTI_DIN                  _IOWR(VPD_IOC_MAGIC, 7, vpdPutCopyMultiBs_t) 
#define VPD_UPDATE_ENTITY_SETTING               _IOWR(VPD_IOC_MAGIC, 8, vpdUpdateEntity_t) 

#define VPD_REQUEST_KEYFRAME                    _IOW(VPD_IOC_MAGIC, 20, vpdChannelFd_t)
#define VPD_REQUEST_SNAPSHOT                    _IOWR(VPD_IOC_MAGIC, 21, vpdSnapshot_t)
#define VPD_REQUEST_DISP_SNAPSHOT               _IOWR(VPD_IOC_MAGIC, 22, vpdDispSnapshot_t)
#define VPD_DECODE_KEYFRAME                     _IOW(VPD_IOC_MAGIC, 23, vpdDecodeKeyframe_t)
#define VPD_REGION_RAWDATA                      _IOWR(VPD_IOC_MAGIC, 24, vpdRegionRawdata_t)
#define VPD_GET_CAP_SOURCE                      _IOWR(VPD_IOC_MAGIC, 25, vpdCapSource_t)

#define VPD_SET_OSD_FONT                        _IOWR(VPD_IOC_MAGIC, 30, vpdOsdFont_t) 
#define VPD_SET_OSD_MASK                        _IOWR(VPD_IOC_MAGIC, 31, vpdOsdMask_t)
#define VPD_SET_OSD_MARK                        _IOWR(VPD_IOC_MAGIC, 33, vpdOsdMark_t) 
#define VPD_SET_OSD_PALETTE_TABLE               _IOWR(VPD_IOC_MAGIC, 34, vpdOsdPaletteTable_t)
#define VPD_SET_OSD_MARK_IMG                    _IOWR(VPD_IOC_MAGIC, 35, vpdOsdMarkImgTable_t)

#define VPD_CLEAR_WINDOW                        _IOW(VPD_IOC_MAGIC, 40, vpdClrWin_t) 
#define VPD_ADJUST_CVBS_VISION                  _IOW(VPD_IOC_MAGIC, 41, vpdCvbsVision_t) 
#define VPD_CLEAR_BITSTREAM                     _IOW(VPD_IOC_MAGIC, 42, vpdClrWin_t) 

#define VPD_AUDIO_COMMAND                       _IOW(VPD_IOC_MAGIC, 45, vpdAudioCommand_t) 

#define VPD_GET_GMLIB_DBG_MODE                  _IOR(VPD_IOC_MAGIC, 50, vpdGmlibDbg_t)
#define VPD_SEND_LOGMSG                         _IOW(VPD_IOC_MAGIC, 51, vpdSendLog_t)
#define VPD_GET_PLATFORM_INFO                   _IOR(VPD_IOC_MAGIC, 52, vpdSysInfo_t) 
#define VPD_UPDATE_PLATFORM_INFO                _IOW(VPD_IOC_MAGIC, 53, vpdSysInfo_t) 
#define VPD_GET_PLATFORM_VERSION                _IOR(VPD_IOC_MAGIC, 54, vpdVersion_t) 
#define VPD_GET_SIG_INFO                        _IOR(VPD_IOC_MAGIC, 55, vpdSigInfo_t) 
#define VPD_ENV_UPDATE                          _IOR(VPD_IOC_MAGIC, 56, vpdEnvupdate_t)
#define VPD_GET_SIG_NR                          _IOWR(VPD_IOC_MAGIC, 57, int) 
#define VPD_SET_SIG_NR                          _IOWR(VPD_IOC_MAGIC, 58, int) 

#define VPD_SET_CAP_FLIP                        _IOWR(VPD_IOC_MAGIC, 70, vpdCapFlip_t)
#define VPD_QUERY_GRAPH                         _IOR(VPD_IOC_MAGIC, 71, vpdQueryGraph_t)
#define VPD_SET_OSD_NEW_FONT                    _IOWR(VPD_IOC_MAGIC, 72, vpdOsdFontUpdate_t)
#define VPD_GET_COPY_MULTI_CAP_MD               _IOWR(VPD_IOC_MAGIC, 73, vpdCapMdInfo_t) 
#define VPD_GET_CAS_SCALING                     _IOWR(VPD_IOC_MAGIC, 74, vpdCasScaling_t)
#define VPD_SET_CAP_TAMPER                      _IOWR(VPD_IOC_MAGIC, 75, vpdCapTamper_t)
#define VPD_SET_OSG_MARK_IMG                    _IOWR(VPD_IOC_MAGIC, 76, vpdOsdMarkImgTable_t)
#define VPD_SET_CAP_MOTION                      _IOWR(VPD_IOC_MAGIC, 77, vpdCapMotion_t)
#define VPD_SET_OSG_MARK_CMD                    _IOWR(VPD_IOC_MAGIC, 78, vpdOsgMarkCmd_t)


#endif /* __VPDRV_H__ */

