#Transfer vg_boot.sh spec.cfg param.cfg from master to slave site (/mnt/mtd)
boot_actor=single

/sbin/insmod /lib/modules/frammap.ko
cat /proc/frammap/ddr_info

# for get pci_epcnt/cpu_enum
# pci_epcnt = n, the GM8210_EP count.
# cpu_enum = 0(host_fa726), 1(host_fa626), 2(host_7500), 3(dev_fa726), 4(dev_fa626)

if [ "$boot_actor" == "master" ] ; then
    pci_epcnt=`grep -A 3 'pci_epcnt' /proc/pmu/attribute | grep 'Attribute value' | cut -c 18`
    cpu_enum=`grep -A 3 'cpu_enum' /proc/pmu/attribute | grep 'Attribute value' | cut -c 18`
    echo "cpu_enum=$cpu_enum, pci_epcnt=$pci_epcnt"
    /sbin/insmod /lib/modules/cpu_comm_fa726.ko
    mdev -s
    echo ""
    read -t 2 -p "   Press q -> ENTER to exit boot procedure? " exit_boot
    if [ "$exit_boot" == "q" ] ; then
        echo "0" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w
        if [ "$pci_epcnt" == "1" ] ; then
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
        fi
        exit
    fi

    if [ -e /mnt/mtd/vg_boot.sh ] ; then
        echo "1" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /mnt/mtd/vg_boot.sh -w
    fi

    if [ -e /mnt/mtd/gmlib.cfg ] ; then
        echo "1" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /mnt/mtd/gmlib.cfg -w
    fi

    if [ -e /mnt/mtd/param.cfg ] ; then
        echo "1" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /mnt/mtd/param.cfg -w
    fi

    if [ -e /mnt/mtd/spec.cfg ] ; then
        echo "1" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w
        cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /mnt/mtd/spec.cfg -w
    fi

    echo "0" > /tmp/transfer_number
    cpucomm_file -c /dev/cpucomm_FA626_chan0 -f /tmp/transfer_number -w

    if [ "$pci_epcnt" == "1" ] ; then
        if [ -e /mnt/mtd/vg_boot.sh ] ; then
            echo "1" > /tmp/transfer_number
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /mnt/mtd/vg_boot.sh -w
        fi
    
        if [ -e /mnt/mtd/gmlib.cfg ] ; then
            echo "1" > /tmp/transfer_number
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /mnt/mtd/gmlib.cfg -w
        fi

        if [ -e /mnt/mtd/param.cfg ] ; then
            echo "1" > /tmp/transfer_number
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /mnt/mtd/param.cfg -w
        fi
    
        if [ -e /mnt/mtd/spec.cfg ] ; then
            echo "1" > /tmp/transfer_number
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
            cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /mnt/mtd/spec.cfg -w
        fi
    
        echo "0" > /tmp/transfer_number
        cpucomm_file -c /dev/cpucomm_DEV0_FA626_chan0 -f /tmp/transfer_number -w
    fi

elif [ "$boot_actor" == "slave" ] ; then
    pci_epcnt=`grep -A 3 'pci_epcnt' /proc/pmu/attribute | grep 'Attribute value' | cut -c 18`
    cpu_enum=`grep -A 3 'cpu_enum' /proc/pmu/attribute | grep 'Attribute value' | cut -c 18`
    echo "cpu_enum=$cpu_enum, pci_epcnt=$pci_epcnt"
    /sbin/insmod /lib/modules/cpu_comm_fa626.ko
    mdev -s
    if [ "$cpu_enum" == "1" ] ; then
        cd /mnt/mtd
        cpucomm_file -c /dev/cpucomm_FA726_chan0  -r
    
        transfer_number=`cat /tmp/transfer_number`
        while [ "$transfer_number" == "1" ]
        do
            echo do next
            cpucomm_file -c /dev/cpucomm_FA726_chan0 -r
            cpucomm_file -c /dev/cpucomm_FA726_chan0 -r
            transfer_number=`cat /tmp/transfer_number`
        done
        sed -i 's/cpu_actor=master/cpu_actor=slave/' /mnt/mtd/vg_boot.sh
    elif [ "$cpu_enum" == "4" ] ; then
        cd /mnt/mtd
        cpucomm_file -c /dev/cpucomm_HOST_FA726_chan0  -r
    
        transfer_number=`cat /tmp/transfer_number`
        while [ "$transfer_number" == "1" ]
        do
            echo do next
            cpucomm_file -c /dev/cpucomm_HOST_FA726_chan0 -r
            cpucomm_file -c /dev/cpucomm_HOST_FA726_chan0 -r
            transfer_number=`cat /tmp/transfer_number`
        done
        sed -i 's/cpu_actor=master/cpu_actor=slave_ep/' /mnt/mtd/vg_boot.sh
    fi    
else
    echo ""
    read -t 2 -p "   Press q -> ENTER to exit boot procedure? " exit_boot
    if [ "$exit_boot" == "q" ] ; then
        exit
    fi
fi

sh /mnt/mtd/vg_boot.sh
