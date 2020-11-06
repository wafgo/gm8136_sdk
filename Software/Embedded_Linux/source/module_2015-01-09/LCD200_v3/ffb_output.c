/*
 * The following just lists the output name description.
 */

static ffb_output_name_t ffb_output_name[] = {
    /* D1 NTSC */
    {
     .output_type = OUTPUT_TYPE_DAC_D1_0,
     .name = "D1(NTSC)"},
    /* D1 PAL */
    {
     .output_type = OUTPUT_TYPE_PAL_1,
     .name = "D1(PAL)"},
    /* CS4954 */
    {
     .output_type = OUTPUT_TYPE_CS4954_2,
     .name = "CS4954"},
    /* PVI2003A */
    {
     .output_type = OUTPUT_TYPE_PVI2003A_3,
     .name = "PVI2003A"},
    /* FS453 */
    {
     .output_type = OUTPUT_TYPE_FS453_4,
     .name = "FS453"},
    /* MDIN200_HD */
    {
     .output_type = OUTPUT_TYPE_MDIN200_HD_5,
     .name = "MDIN200_HD(1440x1080)"},
    /* MDIN200_D1 */
    {
     .output_type = OUTPUT_TYPE_MDIN200_D1_6,
     .name = "MDIN200_D1"},
    /* VGA_1024x768 */
    {
     .output_type = OUTPUT_TYPE_VGA_1024x768_8,
     .name = "VGA_1024x768"},
    /* VGA_1280x800 */
    {
     .output_type = OUTPUT_TYPE_VGA_1280x800_9,
     .name = "VGA_1280x800"},
    /* VGA_1280x960 */
    {
     .output_type = OUTPUT_TYPE_VGA_1280x960_10,
     .name = "VGA_1280x960"},
    /* VGA_1920x1080_RGB */
    {
     .output_type = OUTPUT_TYPE_VGA_1920x1080_11,
     .name = "VGA_1920x1080"},
    /* VGA_1440x900_RGB */
    {
     .output_type = OUTPUT_TYPE_VGA_1440x900_12,
     .name = "VGA_1440x900"},
    /* VGA_1360x768 */
    {
     .output_type = OUTPUT_TYPE_VGA_1360x768_14,
     .name = "VGA_1360x768"},
    /* VGA_1280x1024 */
    {
     .output_type = OUTPUT_TYPE_VGA_1280x1024_13,
     .name = "VGA_1280x1024"},
    /* VGA_800x600 */
    {
     .output_type = OUTPUT_TYPE_VGA_800x600_15,
     .name = "VGA_800x600"},
    /* VGA_1600x1200 */
    {
     .output_type = OUTPUT_TYPE_VGA_1600x1200_16,
     .name = "VGA_1600x1200"},
    /* VGA_1680x1050 */
    {
     .output_type = OUTPUT_TYPE_VGA_1680x1050_17,
     .name = "VGA_1680x1050"},
    /* Cascase 1024x768x25 */
    {
     .output_type = OUTPUT_TYPE_VGA_1280x720_18,
     .name = "VGA_1280x720"},
    /* Cascase 1024x768x30 */
    {
     .output_type = OUTPUT_TYPE_CAS_1024x768x30_19,
     .name = "Cascase 1024x768x30"},
    /* CAT6611_480P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_480P_20,
     .name = "CAT6611_480P"},
    /* CAT6611_576P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_576P_21,
     .name = "CAT6611_576P"},
    /* CAT6611_720P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_720P_22,
     .name = "CAT6611_720P"},
    /* CAT6611_1080P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1080P_23,
     .name = "CAT6611_1080P"},
    /* CAT6611_1024x768 */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1024x768_24,
     .name = "CAT6611_1024x768_RGB"},
    /* CAT6611_1024x768P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1024x768P_25,
     .name = "CAT6611_1024x768P"},
    /* CAT6611_1280x1204 */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1280x1024P_26,
     .name = "CAT6611_1280x1024P"},
    /* CAT6611_1680x1050P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1680x1050P_27,
     .name = "CAT6611_1680x1050P"},
    /* CAT6611_1280x1024_RGB */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1280x1024_28,
     .name = "CAT6611_1280x1024_RGB"},
    /* CAT6611_1920x1080 */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1920x1080_29,
     .name = "CAT6611_1920x1080_RGB"},
    /* CAT6611_1440x900P */
    {
     .output_type = OUTPUT_TYPE_CAT6611_1440x900P_30,
     .name = "CAT6611_1440x900P"},
    /* Cascase 1920x1080x30 gm8210 cascase */
    {
     .output_type = OUTPUT_TYPE_CAS_1920x1080x30_33,
     .name = "1920x1080x30_BT1120"},
    /* Cascase 1920x1080x25 gm8210 cascase */
    {
     .output_type = OUTPUT_TYPE_CAS_1920x1080x25_34,
     .name = "1920x1080x25_BT1120"},
    /* MDIN240_HD(1440x1080) */
    {
     .output_type = OUTPUT_TYPE_MDIN240_HD_35,
     .name = "MDIN240_HD(1440x1080)"},
    /* MDIN240_SD */
    {
     .output_type = OUTPUT_TYPE_MDIN240_SD_36,
     .name = "MDIN240_SD"},
    /* MDIN240_1024x768_60I */
    {
     .output_type = OUTPUT_TYPE_MDIN240_1024x768I_38,
     .name = "MDIN240_1024x768_60I"},
    /* cascase 1440x960x30 interlace */
    {
     .output_type = OUTPUT_TYPE_CAS_1440x960I_40,
     .name = "Cascase 1440x960x30I"},
    /* cascase 1440x1152x25 interlace */
    {
     .output_type = OUTPUT_TYPE_CAS_1440x1152I_41,
     .name = "Cascase 1440x1152x30I"},

    /* SLI10121_480P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_480P_42,
        .name        = "SLI10121_480P"
    },
    /* SLI10121_576P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_576P_43,
        .name        = "SLI10121_576P"
    },
    /* SLI10121_720P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_720P_44,
        .name        = "SLI10121_720P"
    },
    /* SLI10121_1080P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1080P_45,
        .name        = "SLI10121_1080P"
    },
    /* SLI10121_1024x768_RGB */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1024x768_46,
        .name        = "_SLI10121_1024x768_RGB_"
    },
    /* SLI10121_1024x768P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1024x768P_47,
        .name        = "SLI10121_1024x768P"
    },
    /* SLI10121_1280x1024P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1280x1024P_48,
        .name        = "SLI10121_1280x1024P"
    },
    /* SLI10121_1680x1050P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1680x1050P_49,
        .name        = "SLI10121_1680x1050P"
    },
    /* SLI10121_1280x1024P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1280x1024_50,
        .name        = "SLI10121_1280x1024_RGB"
    },
    /* SLI10121_1920x1080_RGB */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1920x1080_51,
        .name        = "SLI10121_1920x1080_RGB"
    },
    /* SLI10121_1440x900P */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1440x900P_52,
        .name        = "SLI10121_1440x900P"
    },
    /* SLI10121_1280x720_RGB */
    {
        .output_type = OUTPUT_TYPE_SLI10121_1280x720_53,
        .name        = "SLI10121_1280x720_RGB"
    },
    /* 1280x1024x25 cascade */
    {
        .output_type = OUTPUT_TYPE_CAS_1280x1024x25_31,
        .name        = "Cascase 1280x1024x25I"
    },
    /* 1280x1024x30 cascade */
    {
        .output_type = OUTPUT_TYPE_CAS_1280x1024x30_32,
        .name        = "Cascase 1280x1024x30I"
    },
    /* MDIN340 480P */
    {
        .output_type = OUTPUT_TYPE_MDIN340_480P_60,
        .name = "MDIN340_BT1120_480P"
    },
    /* MDIN340 576P */
    {
        .output_type = OUTPUT_TYPE_MDIN340_576P_61,
        .name = "MDIN340_BT1120_576P"
    },
    /* MDIN340 1080P */
    {
        .output_type = OUTPUT_TYPE_MDIN340_1920x1080P_62,
        .name = "MDIN340_BT1120_1920x1080P"
    },
    /* MDIN340 1080I */
    {
        .output_type = OUTPUT_TYPE_MDIN340_1920x1080_63,
        .name = "MDIN340_BT1120_1920x1080I"
    },
};

ffb_output_name_t *ffb_get_output_name(OUTPUT_TYPE_T output_type)
{
    int i;
    ffb_output_name_t *ret = NULL;

    for (i = 0; i < ARRAY_SIZE(ffb_output_name); i++) {
        if (ffb_output_name[i].output_type == output_type) {
            ret = &ffb_output_name[i];
            break;
        }
    }

    return ret;
}

//EXPORT_SYMBOL(ffb_get_output_name);
//MODULE_LICENSE("GPL");
