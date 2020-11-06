video_frontend=ov2715
video_system=NTSC

# Support video_front_end: ov2710, ov2715, ov9712, ov9715, ov9714, ov5653
# Support video_front_end: mt9m034, ar0130, ar0140, ar0330, ar0331
# Support video_front_end: imx222, imx238, imx236, imx238

chipver=`head -1 /proc/pmu/chipver`
chipid=`echo $chipver | cut -c 1-4`

if [ "$chipid" != "8136" ] && [ "$chipid" != "8135" ]; then
    echo "Error! Not support chip version $chipver."
    exit
fi

if [ "$video_system" != "NTSC" ] && [ "$video_system" != "PAL" ]; then
    echo "Invalid argument for NTSC/PAL."
    exit
fi

if [ "$1" != "" ] ; then
    video_frontend=$1
fi

if [ "$video_frontend" != "ov2715" ] && [ "$video_frontend" != "ov2710" ] &&
   [ "$video_frontend" != "ov9710" ] && [ "$video_frontend" != "ov9712" ] &&
   [ "$video_frontend" != "ov9715" ] && [ "$video_frontend" != "ov9714" ] &&
   [ "$video_frontend" != "ar0130" ] && [ "$video_frontend" != "mt9m034" ] &&
   [ "$video_frontend" != "ar0140" ] && [ "$video_frontend" != "ar0141" ] &&
   [ "$video_frontend" != "ar0330" ] && [ "$video_frontend" != "ar0331" ] &&   
   [ "$video_frontend" != "imx222" ] && [ "$video_frontend" != "imx124" ] &&
   [ "$video_frontend" != "imx238" ] && [ "$video_frontend" != "imx236" ]; then
     echo "Invalid argument for video frontend: $video_frontend"
     exit
fi

/sbin/insmod /lib/modules/frammap.ko
cat /proc/frammap/ddr_info

/sbin/insmod /lib/modules/log.ko mode=0 log_ksize=256
/sbin/insmod /lib/modules/ms.ko
/sbin/insmod /lib/modules/em.ko
/sbin/insmod /lib/modules/flcd200-common.ko
/sbin/insmod /lib/modules/flcd200-pip.ko output_type=0 fb0_fb1_share=1    # CVBS display
/sbin/insmod /lib/modules/sar_adc.ko
/sbin/insmod /lib/modules/think2d.ko
/sbin/insmod /lib/modules/ftpwmtmr010.ko
/sbin/insmod /lib/modules/fe_common.ko
/sbin/insmod /lib/modules/adda308.ko input_mode=0 single_end=1
/sbin/insmod /lib/modules/ft3dnr200.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1

case "$video_frontend" in
    "ov2715"|"ov2710")
        codec_max_width=1920
        codec_max_height=1080
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ov2715.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_ov2715.ko sensor_w=1920 sensor_h=1080 interface=1 fps=30 mirror=1 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ov2715.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_ov2715.ko sensor_w=1920 sensor_h=1080 interface=1 fps=25 mirror=1 flip=1
        fi        
        ;;
    "ov9715"|"ov9712"|"ov9710")
        codec_max_width=1280
        codec_max_height=720
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ov9715.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_ov9715.ko sensor_w=1280 sensor_h=720 fps=30 mirror=1 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ov9715.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_ov9715.ko sensor_w=1280 sensor_h=720 fps=25 mirror=1 flip=1
        fi        
        ;;     
    "ar0330")
        codec_max_width=1920
        codec_max_height=1080
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0330.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_ar0330.ko sensor_w=1920 sensor_h=1080 mirror=1 flip=1 interface=1 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0330.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_ar0330.ko sensor_w=1920 sensor_h=1080 mirror=1 flip=1 interface=1 fps=25
        fi        
        ;;         
    "ar0331")
        codec_max_width=1920
        codec_max_height=1080
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0331.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_ar0331.ko sensor_w=1920 sensor_h=1080 interface=0 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0331.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_ar0331.ko sensor_w=1920 sensor_h=1080 interface=0 fps=25
        fi        
        ;;      
    "mt9m034")
        codec_max_width=1280
        codec_max_height=720
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_mt9m034.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_mt9m034.ko sensor_w=1280 sensor_h=720 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_mt9m034.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_mt9m034.ko sensor_w=1280 sensor_h=720 fps=25
        fi        
        ;;
    "ar0140"|"ar0141")
        codec_max_width=1280
        codec_max_height=720
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0140.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_ar0140.ko sensor_w=1280 sensor_h=720 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_ar0140.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_ar0140.ko sensor_w=1280 sensor_h=720 fps=25
        fi        
        ;;
    "imx238")                                                                              
        codec_max_width=1280                                                             
        codec_max_height=720                                                              
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx238.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_imx138.ko sensor_w=1280 sensor_h=720 fps=30        
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx238.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_imx138.ko sensor_w=1280 sensor_h=720 fps=25        
        fi        
        ;;           
    "imx222")
        codec_max_width=1920
        codec_max_height=1080
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx222.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx222.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=25
        fi        
        ;;        
    "imx236")
        codec_max_width=1920
        codec_max_height=1080
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx236.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /lib/modules/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /lib/modules/fisp328.ko cfg_path=/mnt/mtd/isp328_imx236.cfg
            /sbin/insmod /lib/modules/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /lib/modules/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=25
        fi        
        ;;
    *)
        echo "Invalid argument for video frontend: $video_frontend"
        exit
        ;;
esac

/sbin/insmod /lib/modules/vcap300_common.ko
/sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 ext_irq_src=1
/sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
/sbin/insmod /lib/modules/fmcp_drv.ko mp4_tight_buf=1
/sbin/insmod /lib/modules/favc_enc.ko h264e_max_b_frame=0 h264e_one_ref_buf=1 h264e_tight_buf=1 h264e_max_width=$codec_max_width h264e_max_height=$codec_max_height h264e_slice_offset=1
/sbin/insmod /lib/modules/favc_rc.ko
/sbin/insmod /lib/modules/decoder.ko
/sbin/insmod /lib/modules/fmjpeg_drv.ko
/sbin/insmod /lib/modules/fmpeg4_drv.ko mp4_max_width=$codec_max_width mp4_max_height=$codec_max_height
/sbin/insmod /lib/modules/mp4e_rc.ko

# Encode 4CH + Cascade YUV 1CH
/sbin/insmod /lib/modules/sw_osg.ko
/sbin/insmod /lib/modules/fscaler300.ko max_vch_num=5 max_minors=5 temp_width=0 temp_height=0
#/sbin/insmod /lib/modules/ftdi220.ko
/sbin/insmod /lib/modules/osd_dispatch.ko
/sbin/insmod /lib/modules/codec.ko
/sbin/insmod /lib/modules/audio_drv.ko audio_ssp_num=0,1 audio_ssp_chan=1,1 bit_clock=400000,400000 sample_rate=8000,8000 audio_out_enable=1,0
/sbin/insmod /lib/modules/gs.ko reserved_ch_cnt=2 alloc_unit_size=262144 flow_mode=1
/sbin/insmod /lib/modules/loop_comm.ko
/sbin/insmod /lib/modules/vpd_slave.ko vpslv_dbglevel=0 ddr0_sz=0 ddr1_sz=0 config_path="/mnt/mtd/" usr_func=0 usr_param=0 datain_minors=4 dataout_minors=8
/sbin/insmod /lib/modules/vpd_master.ko vpd_dbglevel=0 gmlib_dbglevel=0

echo /mnt/nfs > /proc/videograph/dumplog   #configure log path
mdev -s
cat /proc/modules
echo 0 > /proc/frammap/free_pages   #should not free DDR1 for performance issue
echo 1 > /proc/vcap300/vcap0/dbg_mode  #need debug mode to detect capture overflow
echo 0 > /proc/videograph/em/dbglevel
echo 0 > /proc/videograph/gs/dbglevel
echo 0 > /proc/videograph/ms/dbglevel
echo 0 > /proc/videograph/datain/dbglevel
echo 0 > /proc/videograph/dataout/dbglevel
echo 0 > /proc/videograph/vpd/dbglevel
echo 0 > /proc/videograph/gmlib/dbglevel

echo =========================================================================
if [ -e /mnt/mtd/gmlib.cfg ] ; then
grep ";" /mnt/mtd/gmlib.cfg |sed -n '1,1p'
else
grep ";" /mnt/mtd/spec.cfg |sed -n '2,6p'
fi

rootfs_date=`ls /|grep 00_2`
mtd_date=`ls /mnt/mtd|grep 00_2`
echo =========================================================================
echo "  Video Front End: $video_frontend"
echo "  Chip Version: $chipver"
echo "  RootFS Version: $rootfs_date"
echo "  MTD Version: $mtd_date"
echo =========================================================================

devmem 0x9a1000a0 32 0x87878587
devmem 0x9a100034 32 0x061f0606
devmem 0x9a1000c4 32 0x08000f08
devmem 0x9a1000c8 32 0x061f0606
devmem 0x9a100030 32 0xDF000f04

#devmem 0x96105440 32 0x01500000
#devmem 0x96105438 32 0x01500000
#echo 1 0x50 > /proc/3dnr/dma/param
#echo w ae_en 0 > /proc/isp320/command
#echo w sen_exp 133 > /proc/isp320/command
#echo w fps 15 > /proc/isp320/command

# force max CPU performance
#echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

