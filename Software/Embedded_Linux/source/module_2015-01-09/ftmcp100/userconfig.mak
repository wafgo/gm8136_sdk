#------------------------------------
# Select RSIC/non_RISC/Seq. version for linux 2.6
#------------------------------------
ifeq ($(CONFIG_PLATFORM_GM8120),y)
    CONFIG_FTMCP100_CORE=HOST
    SUPPORT_VG_IOCTL=IOCTL
endif
ifeq ($(CONFIG_PLATFORM_FIE8150),y)
    CONFIG_FTMCP100_CORE=EMBEDDED
#    CONFIG_FTMCP100_CORE=HOST
    SUPPORT_VG_IOCTL=IOCTL
endif
ifeq ($(CONFIG_PLATFORM_GM8180),y)
    CONFIG_FTMCP100_CORE=EMBEDDED
    SUPPORT_VG_IOCTL=IOCTL
#    SUPPORT_VG_IOCTL=VG
endif
ifeq ($(CONFIG_PLATFORM_GM8185),y)
    CONFIG_FTMCP100_CORE=EMBEDDED
endif
ifeq ($(CONFIG_PLATFORM_GM8185_v2),y)
    CONFIG_FTMCP100_CORE=EMBEDDED
#    CONFIG_FTMCP100_CORE=HOST
#    SUPPORT_VG_IOCTL=VG
    SUPPORT_VG_IOCTL=IOCTL
endif
ifeq ($(CONFIG_PLATFORM_GM8181),y)
    CONFIG_FTMCP100_CORE=SEQ
    SUPPORT_VG_IOCTL=VG
#    SUPPORT_VG_IOCTL=IOCTL
#only configure this for SEQ ->
    SUPPORT_VG_422T=YES
    SUPPORT_VG_422P=YES
#only configure this for SEQ <-
#use work queue to trigger next job ->
    CONFIG_USE_WORKQUEUE=YES
    CONFIG_FRAMMAP=YES
#use work queue to trigger next job <-
    CONFIG_MULTI_FORMAT_DECODER=yes
endif
ifeq ($(CONFIG_PLATFORM_GM8126),y)
    CONFIG_FTMCP100_CORE=EMBEDDED
#    SUPPORT_VG_IOCTL=IOCTL
    SUPPORT_VG_IOCTL=VG
    CONFIG_USE_WORKQUEUE=YES
endif

#-------------------------------
# Select Large or normal resolution
#-------------------------------
CONFIG_2592x1944_RESOLUTION = no
CONFIG_1600x1200_RESOLUTION = no
CONFIG_720x576_RESOLUTION = yes

#-------------------------------
# HW Busy Performance Evaluation
#-------------------------------
CONFIG_HW_BUSY_PERFORMANCE = no

#--------------------------------------
# MPEG4 Customer Quant Table
#--------------------------------------
MPEG4_CUSTOMER_QUANT_TABLE_SUPPORT = yes

#--------------------------------------
# VG mode, max frame no in Q, include all codec in ftmcp100
#--------------------------------------
QUEUE_MAX_FRAME_NUM = 64
