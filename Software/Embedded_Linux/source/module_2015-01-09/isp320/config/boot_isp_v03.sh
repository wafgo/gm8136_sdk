video_frontend=imx138
video_system=NTSC

# Support video_front_end: ov2715, mt9m034, imx136, ov16820

chipver=`head -1 /proc/pmu/chipver`
chipid=`echo $chipver | cut -c 1-4`

if [ "$chipid" != "8139" ] && [ "$chipid" != "8138" ]; then
    echo "Error! Not support chip version $chipver."
    exit
fi

if [ "$video_system" != "NTSC" ] && [ "$video_system" != "PAL" ]; then
    echo "Invalid argument for NTSC/PAL."
    exit
fi

if [ "$video_frontend" != "ov2715" ] && [ "$video_frontend" != "ov9715" ] && 
   [ "$video_frontend" != "ov5653" ] && [ "$video_frontend" != "ov16820" ] && 
   [ "$video_frontend" != "ar0331" ] && [ "$video_frontend" != "mt9m034" ] &&
   [ "$video_frontend" != "imx122" ] && [ "$video_frontend" != "imx222" ] && 
   [ "$video_frontend" != "imx124" ] && [ "$video_frontend" != "imx136" ] && 
   [ "$video_frontend" != "imx138" ] && [ "$video_frontend" != "imx236" ] && 
   [ "$video_frontend" != "GC1004" ] && [ "$video_frontend" != "ov4689" ] && 
   [ "$video_frontend" != "ar0835" ] && [ "$video_frontend" != "ar0330" ] &&
   [ "$video_frontend" != "ov9713" ]; then
     echo "Invalid argument for video frontend: $video_frontend"
     exit
fi

if [ "$1" != "" ] ; then
    video_frontend=$1
fi

/sbin/insmod /lib/modules/frammap.ko
cat /proc/frammap/ddr_info

/sbin/insmod /lib/modules/log.ko mode=0 log_ksize=8192
/sbin/insmod /lib/modules/ms.ko
/sbin/insmod /lib/modules/em.ko
/sbin/insmod /lib/modules/flcd200-common.ko
/sbin/insmod /lib/modules/flcd200-pip.ko output_type=0 #D1 cvbs
/sbin/insmod /lib/modules/sar_adc.ko
/sbin/insmod /lib/modules/think2d.ko
/sbin/insmod /lib/modules/ftpwmtmr010.ko
/sbin/insmod /lib/modules/fe_common.ko
/sbin/insmod /lib/modules/adda302.ko input_mode=0 single_end=1

case "$video_frontend" in
    "ov16820")
        codec_max_width=4096
        codec_max_height=2048
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov16820.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ov16820.ko sensor_w=4096 sensor_h=2048 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov16820.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov16820.ko sensor_w=4096 sensor_h=2048 fps=25
        fi
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        echo w start >/proc/isp320/command
        echo w ae_en 0 > /proc/isp320/command
        echo w sen_exp 133 > /proc/isp320/command
        echo w fps 15 > /proc/isp320/command
        ;;
    "ov2715")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov2715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ov2715.ko sensor_w=1920 sensor_h=1080 interface=1 fps=30 mirror=1 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov2715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov2715.ko sensor_w=1920 sensor_h=1080 interface=1 fps=25 mirror=1 flip=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;
    "ov9715")
        codec_max_width=1280
        codec_max_height=720
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ov9715.ko sensor_w=1280 sensor_h=720 fps=30 mirror=1 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov9715.ko sensor_w=1280 sensor_h=720 fps=25 mirror=1 flip=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;     
    "ov5653")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov5653.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ov5653.ko sensor_w=1920 sensor_h=1080 interface=1 fps=30 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov9715.ko sensor_w=1280 sensor_h=720 interface=1 fps=25 flip=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;  
    "ar0331")
        codec_max_width=2048
        codec_max_height=1536
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0331.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ar0331.ko sensor_w=1920 sensor_h=1080 interface=2  fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0331.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ar0331.ko sensor_w=1920 sensor_h=1080 interface=2  fps=25
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;      
                  
    "mt9m034")
        codec_max_width=1280
        codec_max_height=720
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_mt9m034.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_mt9m034.ko sensor_w=1280 sensor_h=720 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_mt9m034.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_mt9m034.ko sensor_w=1280 sensor_h=720 fps=25
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;
    "imx122")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx122.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx22.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=25
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;        
    "imx124")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx124.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_imx124.ko sensor_w=1920 sensor_h=1080 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx124.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx124.ko sensor_w=1920 sensor_h=1080 fps=25
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;        
    "imx136")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx136.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=60
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx136.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=50
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;
    "imx138")                                                                              
        codec_max_width=1280                                                             
        codec_max_height=720                                                              
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1      
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx138.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_imx138.ko sensor_w=1280 sensor_h=720 fps=30        
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx138.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx138.ko sensor_w=1280 sensor_h=720 fps=25        
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko                                  
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1                             
        ;;   
    "imx222")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx222.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx222.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx122.ko sensor_w=1920 sensor_h=1080 fps=25
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;        
    "imx236")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx236.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=60
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_imx236.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_imx136.ko sensor_w=1920 sensor_h=1080 fps=50
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;
    "ar0835")
        codec_max_width=3264
        codec_max_height=2448
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0835.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ar0835.ko sensor_w=3264 sensor_h=2448 fps=30 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0835.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1
            /sbin/insmod /mnt/mtd/ko/fisp_ar0835.ko sensor_w=3264 sensor_h=2448 fps=25 flip=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;   
    "ar0330")
        codec_max_width=2048;codec_max_height=1536
        #codec_max_width=2304;codec_max_height=1296
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0330.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0
            /sbin/insmod /mnt/mtd/ko/fisp_ar0330.ko sensor_w=2048 sensor_h=1536 fps=30 flip=1 mirror=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ar0330.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1
            /sbin/insmod /mnt/mtd/ko/fisp_ar0330.ko sensor_w=2048 sensor_h=1536 fps=25 flip=1 mirror=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;           
    "ov4689")
        codec_max_width=1920
        codec_max_height=1080
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov4689.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov4689.ko sensor_w=1920 sensor_h=1080 fps=30 flip=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov4689.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov4689.ko sensor_w=1920 sensor_h=1080 fps=20 flip=1  
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;  	      
    "GC1004")
        codec_max_width=2048;codec_max_height=1280
        #codec_max_width=2304;codec_max_height=720
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then    
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0
            /sbin/insmod /mnt/mtd/ko/fisp_GC1004.ko sensor_w=1280 sensor_h=720 fps=30 flip=1 mirror=1
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9715.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1
            /sbin/insmod /mnt/mtd/ko/fisp_GC1004.ko sensor_w=1280 sensor_h=720 fps=25 flip=1 mirror=1
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;                                                                                                       
    "ov9713")
        codec_max_width=1280
        codec_max_height=720
        /sbin/insmod /lib/modules/ft3dnr.ko src_yc_swap=1 dst_yc_swap=1 ref_yc_swap=1
        if [ "$video_system" == "NTSC" ]  ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9713.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=0            
            /sbin/insmod /mnt/mtd/ko/fisp_ov9713.ko sensor_w=1280 sensor_h=720 fps=30
        elif [ "$video_system" == "PAL" ] ; then
            /sbin/insmod /mnt/mtd/ko/fisp320.ko cfg_path=/mnt/mtd/isp320_ov9713.cfg
            /sbin/insmod /mnt/mtd/ko/fisp_algorithm.ko pwr_freq=1            
            /sbin/insmod /mnt/mtd/ko/fisp_ov9713.ko sensor_w=1280 sensor_h=720 fps=25  
        fi        
        /sbin/insmod /lib/modules/vcap300_common.ko
        /sbin/insmod /lib/modules/vcap0.ko vi_mode=0,1 sync_time_div=60 ext_irq_src=1
        /sbin/insmod /lib/modules/vcap300_isp.ko ch_id=0 range=1
        ;;  	      
                                                                                            
    *)
        echo "Invalid argument for video frontend: $video_frontend"
        exit
        ;;
esac

/sbin/insmod /lib/modules/favc_enc.ko h264e_max_b_frame=0 h264e_max_width=$codec_max_width h264e_max_height=$codec_max_height
/sbin/insmod /lib/modules/favc_rc.ko
/sbin/insmod /lib/modules/decoder.ko
/sbin/insmod /lib/modules/fmcp_drv.ko
/sbin/insmod /lib/modules/fmjpeg_drv.ko
/sbin/insmod /lib/modules/fmpeg4_drv.ko mp4_max_width=$codec_max_width mp4_max_height=$codec_max_height

# Encode 4CH + Cascade YUV 1CH
/sbin/insmod /lib/modules/sw_osg.ko
/sbin/insmod /lib/modules/fscaler300.ko max_vch_num=5 max_minors=5
/sbin/insmod /lib/modules/ftdi220.ko
/sbin/insmod /lib/modules/osd_dispatch.ko
/sbin/insmod /lib/modules/codec.ko
/sbin/insmod /lib/modules/audio_drv.ko audio_ssp_num=0,1 audio_ssp_chan=1,1 bit_clock=400000,400000 sample_rate=8000,8000 audio_out_enable=1,1
/sbin/insmod /lib/modules/gs.ko reserved_ch_cnt=4 alloc_unit_size=262144
/sbin/insmod /lib/modules/loop_comm.ko
/sbin/insmod /lib/modules/vpd_slave.ko vpslv_dbglevel=0 ddr0_sz=0 ddr1_sz=0 config_path="/mnt/mtd/" usr_func=1 usr_param=1
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

# H264 codec setting
# DefaultCfg: 1. Performance mode 2. Quality mode
echo DefaultCfg 2 > /proc/videograph/h264e/param
# CABAC 1. Enable 2. Disable (enhance bitrate reduction)
echo SymbolMode 1 > /proc/videograph/h264e/param
# delta QP: 4. Enable 5. Disable
echo DeltaQPWeight 4 > /proc/videograph/h264e/param
# MCNR enhance bitrate reduction
echo MCNREnable 1 > /proc/videograph/h264e/param

echo =========================================================================
grep ";" /mnt/mtd/spec.cfg |sed -n '2,6p'

rootfs_date=`ls /|grep 00`
mtd_date=`ls /mnt/mtd|grep 00`
echo =========================================================================
echo "  Video Front End: $video_frontend"
echo "  Chip Version: $chipver"
echo "  RootFS Version: $rootfs_date"
echo "  MTD Version: $mtd_date"
echo =========================================================================

#devmem 0x96105440 32 0x01500000
#devmem 0x96105438 32 0x01500000
#echo 1 0x50 > /proc/3dnr/dma/param
#echo w ae_en 0 > /proc/isp320/command
#echo w sen_exp 133 > /proc/isp320/command
#echo w fps 15 > /proc/isp320/command

echo 5 >/proc/isp320/ae/auto_drc
/mnt/mtd/isp_demon