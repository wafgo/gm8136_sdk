#!/bin/bash
# Grain Media Video Graph Cleanup Script

video_frontend=ov2715
# GM8210, GM828x: nvp1918, nvp1118, cx26848, cx26848_960h, tw2968, mv4101
# GM8139, GM8136: ov2715, ov9715, ov5653, ar0331, mt9m034, ov9714
# GM8139, GM8136: imx122, imx222, imx138, imx236, ar0835, ar0330, mt9p031, imx238, mn34220

chipver=`head -1 /proc/pmu/chipver`
chipid=`echo $chipver | cut -c 1-4`
cpu_enum=`grep -A 3 'cpu_enum' /proc/pmu/attribute | grep 'Attribute value' | cut -c 18`
# cpu_enum = 0(host_fa726), 1(host_fa626), 2(host_7500), 3(dev_fa726), 4(dev_fa626)

/sbin/rmmod /lib/modules/vpd_master.ko
/sbin/rmmod /lib/modules/vpd_slave.ko
/sbin/rmmod /lib/modules/loop_comm.ko
/sbin/rmmod /lib/modules/gs.ko
/sbin/rmmod /lib/modules/audio_drv.ko
/sbin/rmmod /lib/modules/codec.ko
/sbin/rmmod /lib/modules/osd_dispatch.ko
/sbin/rmmod /lib/modules/fscaler300.ko
/sbin/rmmod /lib/modules/sw_osg.ko
/sbin/rmmod /lib/modules/mp4e_rc.ko
/sbin/rmmod /lib/modules/fmpeg4_drv.ko
/sbin/rmmod /lib/modules/fmjpeg_drv.ko
/sbin/rmmod /lib/modules/decoder.ko
/sbin/rmmod /lib/modules/favc_rc.ko
/sbin/rmmod /lib/modules/favc_enc.ko
/sbin/rmmod /lib/modules/fmcp_drv.ko

if [ "$chipid" == "8139" ] || [ "$chipid" == "8138" ] || [ "$chipid" == "8137" ]; then
    /sbin/rmmod /lib/modules/vcap300_isp.ko
    /sbin/rmmod /lib/modules/vcap0.ko
    /sbin/rmmod /lib/modules/vcap300_common.ko
    /sbin/rmmod /lib/modules/fisp_$video_frontend.ko
    /sbin/rmmod /lib/modules/fisp_algorithm.ko		
    /sbin/rmmod /lib/modules/fisp320.ko		
    /sbin/rmmod /lib/modules/ft3dnr.ko		
    /sbin/rmmod /lib/modules/adda302.ko		
    /sbin/rmmod /lib/modules/fe_common.ko		
    /sbin/rmmod /lib/modules/ftpwmtmr010.ko		
    /sbin/rmmod /lib/modules/think2d.ko		
    /sbin/rmmod /lib/modules/sar_adc.ko		
elif [ "$chipid" == "8136" ] || [ "$chipid" == "8135" ]; then
    /sbin/rmmod /lib/modules/vcap300_isp.ko
    /sbin/rmmod /lib/modules/vcap0.ko
    /sbin/rmmod /lib/modules/vcap300_common.ko
    /sbin/rmmod /lib/modules/fisp_$video_frontend.ko
    /sbin/rmmod /lib/modules/fisp_algorithm.ko
    /sbin/rmmod /lib/modules/fisp328.ko
    /sbin/rmmod /lib/modules/ft3dnr200.ko
    /sbin/rmmod /lib/modules/adda308.ko
    /sbin/rmmod /lib/modules/fe_common.ko
    /sbin/rmmod /lib/modules/ftpwmtmr010.ko
    /sbin/rmmod /lib/modules/think2d.ko
    /sbin/rmmod /lib/modules/sar_adc.ko
fi
