/**
 * @file vg_verify.c
 * VideoGrpah Verify Application
 * Copyright (C) 2013 GM Corp. (http://www.grain-media.com)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "frammap/frammap_if.h"
#include "config.h"
#include "vg_verify.h"

#define ARRAY_SIZE(arr) ((sizeof(arr) / sizeof((arr)[0])))

static unsigned int lcd_fb_used[3] = {0, 0, 0};
static struct buf_cfg_t g_buf_cfg;
static struct job_cfg_t g_job_cfg[MAX_JOBS];
static struct vg_info_t vg_info_entity[VG_INFO_ENTITY_MAX];

static char *job_property_name[] = {
    /* Public Property */
    "src_fmt",
    "src_xy",
    "src_bg_dim",
    "src_dim",
    "src_interlace",
    "dst_fmt",
    "dst_xy",
    "dst_bg_dim",
    "dst_dim",
    "dst_crop_xy",
    "dst_crop_dim",
    "dst2_crop_xy",
    "dst2_crop_dim",
    "dst2_bg_dim",
    "dst2_xy",
    "dst2_dim",
    "dst2_fd",

    "target_frame_rate",
    "fps_ratio",
    "runtime_frame_rate",
    "src_frame_rate",
    "dvr_feature",
    "auto_aspect_ratio",
    "auto_border",
    "special_func",

    /* Private Property */
    "cap_feature",
    "md_enb",
    "md_xy_start",
    "md_xy_size",
    "md_xy_num",
};

static void build_property(struct video_property_t *property, struct video_param_t *param)
{
    int i;
    for(i=0; i<MAX_PROPERTYS; i++) {
        property[i].id    = param[i].id;
        property[i].value = param[i].value;
        if(param[i].id==0)
            break;
    }
}

static void do_record(FILE **fd, int type, unsigned int addr_va, int size, int name_type)
{
    unsigned int len;

    if(*fd == 0) {
        char fname[64];

        if (type == TYPE_YUV422_2FRAMES) {
            switch(name_type) {
                case 1:
                    strcpy(fname, "output_2frame_main.yuv");
                    break;
                case 2:
                    strcpy(fname, "output_2frame_sub.yuv");
                    break;
                default:
                    strcpy(fname, "output_2frame.yuv");
                    break;
            }
        }
        else if(type == TYPE_YUV422_FRAME) {
            switch(name_type) {
                case 1:
                    strcpy(fname, "output_frame_main.yuv");
                    break;
                case 2:
                    strcpy(fname, "output_frame_sub.yuv");
                    break;
                default:
                    strcpy(fname, "output_frame.yuv");
                    break;
            }
        }
        else {
            switch(name_type) {
                case 1:
                    strcpy(fname, "output_field_main.yuv");
                    break;
                case 2:
                    strcpy(fname, "output_field_sub.yuv");
                    break;
                default:
                    strcpy(fname, "output_field.yuv");
                    break;
            }
        }
        *fd = fopen(fname, "wb");
    }

    if(*fd) {
        len = fwrite((void *)addr_va, 1, size, *fd);
        printf("Record from addr_va = 0x%08x, size = 0x%x\n", addr_va, len);
        printf("Record Done!\n");
    }
}

static void print_help(void)
{
    fprintf(stdout, "\nUsage: vg_verify <case> [seconds] [msg_disp]"
                    "\n[1] Single Job Test: Put 10 jobs + stop + record"
                    "\n[2] Single Job Test: Put 10 jobs + stop + Put 10 jobs + stop"
                    "\n[3] Single Job Test: Continue Put jobs (HW util/CPU util/ISR Lock/Perf)"
                    "\n[4] Multi  Job Test: Put 5 jobs + Put 5 jobs + stop + record"
                    "\n[5] Multi  Job Test: Put 5 jobs + stop + Put 5 jobs"
                    "\n[6] Multi  Job Test: Continue Put jobs (HW util/CPU util/ISR Lock/Perf)"
                    "\n[7] Single Job Test: Output to LCD Frame Buffer"
                    "\n[8] Multi  Job Test: Output to LCD Frame Buffer"
                    "\n\n");
}

static int parse_buf_cfg(int test_idx)
{
    int  ret;
    struct stat stat_buf;
    char *buf_cfg_file = "buf.cfg";

    ret = stat(buf_cfg_file, &stat_buf);
    if(ret < 0) {
        perror("buf.cfg file not exist!\n");
        return -1;
    }

    /* clear buffer config */
    memset(&g_buf_cfg, 0, sizeof(struct buf_cfg_t));

    ret = getconfigint(NULL, "in_buf_ddr", &g_buf_cfg.in_buf_ddr, buf_cfg_file);
    if(ret!=0) {
      printf("%s: in_buf_ddr not exist!(%d)\n", buf_cfg_file, ret);
      return -1;
    }
    if(g_buf_cfg.in_buf_ddr >= 2)
        g_buf_cfg.in_buf_ddr = 0;

    ret = getconfigint(NULL, "in_buf_type", &g_buf_cfg.in_buf_type, buf_cfg_file);
    if(ret!=0) {
      printf("%s: in_buf_type not exist!\n", buf_cfg_file);
      return -1;
    }

    ret = getconfigint(NULL, "max_in_frame_size", &g_buf_cfg.in_size, buf_cfg_file);
    if(ret!=0) {
      printf("%s: max_in_frame_size not exist!\n", buf_cfg_file);
      return -1;
    }

    ret = getconfigint(NULL, "out_buf_ddr", &g_buf_cfg.out_buf_ddr, buf_cfg_file);
    if(ret!=0) {
      printf("%s: out_buf_ddr not exist!\n", buf_cfg_file);
      return -1;
    }
    if(g_buf_cfg.out_buf_ddr >= 2)
        g_buf_cfg.out_buf_ddr = 0;

    ret = getconfigint(NULL, "out_buf_type", &g_buf_cfg.out_buf_type, buf_cfg_file);
    if(ret!=0) {
        printf("%s: out_buf_type not exist!\n", buf_cfg_file);
        return -1;
    }

    ret = getconfigint(NULL, "max_out_frame_size", &g_buf_cfg.out_size, buf_cfg_file);
    if(ret!=0) {
        printf("%s: max_out_frame_size not exist!\n", buf_cfg_file);
        return -1;
    }

    if(g_buf_cfg.out_buf_type == TYPE_YUV422_2FRAMES_2FRAMES || g_buf_cfg.out_buf_type == TYPE_YUV422_FRAME_2FRAMES) {
        ret = getconfigint(NULL, "max_out2_frame_size", &g_buf_cfg.out2_size, buf_cfg_file);
        if(ret!=0) {
            printf("%s: max_out2_frame_size not exist!\n", buf_cfg_file);
            return -1;
        }

        if(g_buf_cfg.out2_size <= 0) {
            printf("%s: max_out2_frame_size invalid!\n", buf_cfg_file);
            return -1;
        }
    }
    else
        g_buf_cfg.out2_size = 0;

    ret = getconfigint(NULL, "multi_job_pack_num", &g_buf_cfg.multi_job_pack_num, buf_cfg_file);
    if(ret!=0) {
      printf("%s: multi_job_pack_num not exist!\n", buf_cfg_file);
      return -1;
    }
    if(!g_buf_cfg.multi_job_pack_num || test_idx <= 3 || test_idx == 7) ///< Single-Job
        g_buf_cfg.multi_job_pack_num = 1;

    ret = getconfigint(NULL, "cfg_round_set", &g_buf_cfg.round_set, buf_cfg_file);
    if(ret!=0) {
      printf("%s: cfg_round_set not exist!\n", buf_cfg_file);
      return -1;
    }

    getconfigint(NULL, "split_multi_job", &g_buf_cfg.split_multi_job, buf_cfg_file);
    if(g_buf_cfg.split_multi_job < 0)
        g_buf_cfg.split_multi_job = 0;

    ret = getconfigint(NULL, "ch_from_vg_info", &g_buf_cfg.ch_from_vg_info, buf_cfg_file);
    if(ret != 0 || g_buf_cfg.ch_from_vg_info < 0)
        g_buf_cfg.ch_from_vg_info = 0;

    return 0;
}

static int parse_vg_info(void)
{
    int ret = 0;
    int num = 0;
    int arg_num;
    FILE *vg_file = 0;
    char line[256];

    vg_file = fopen("/proc/vcap300/vcap0/vg_info", "r");
    if(vg_file == 0) {
        perror("open /proc/vcap300/vcap0/vg_info failed");
        ret = -1;
        goto err;
    }

    memset(vg_info_entity, 0 , sizeof(struct vg_info_t)*VG_INFO_ENTITY_MAX);

    while(!feof(vg_file)) {
        if(fgets(line, sizeof(line), vg_file) == NULL)
            break;

        if(line[0] == '|')  ///< skip comment line
            continue;

        if(num >= VG_INFO_ENTITY_MAX)
            break;

        arg_num = sscanf(line, "%d %d %d %d %d %x %dx%d\n",
                               &vg_info_entity[num].vch,
                               &vg_info_entity[num].vcap_ch,
                               &vg_info_entity[num].n_split,
                               &vg_info_entity[num].n_path,
                               &vg_info_entity[num].vi_mode,
                               &vg_info_entity[num].fd_start,
                               &vg_info_entity[num].width,
                               &vg_info_entity[num].height);
        if(arg_num != 8)
            continue;

        vg_info_entity[num++].valid = 1;
    }

err:
    if(vg_file)
        fclose(vg_file);

    return ret;
}

static int parse_job_cfg(int test_idx, int msg_disp)
{
    int  i, j, k, n, ret = 0;
    char fname[128];
    unsigned int value;
    struct video_param_t *param;
    struct stat stat_buf;
    int em_fd      = 0;
    int job_cnt    = 0;

    em_fd = open("/dev/em", O_RDWR);
    if(em_fd == 0) {
        perror("open /dev/em failed");
        return -1;
    }

    /* clear job config */
    memset(g_job_cfg, 0, sizeof(struct job_cfg_t)*MAX_JOBS);

    /* parse job config file */
    for(i=0; i<MAX_JOBS; i++) {
        sprintf(fname, "%d.cfg", i);

        ret = stat(fname, &stat_buf);
        if(ret < 0) {
            ret = 0;
            goto end;
        }

        ret = getconfigint(NULL, "round_set", &g_job_cfg[i].round_set, fname);
        if(ret != 0) {
            printf("%s: round_set not exist!\n", fname);
            ret = -1;
            goto err;
        }

        if(g_job_cfg[i].round_set <= 0)
            continue;

        if((test_idx >= 4 && test_idx <= 6) || (test_idx == 8))
           g_job_cfg[i].round_set = 1;

        getconfigint(NULL, "lcd_fb_idx", &g_job_cfg[i].lcd_fb_idx, fname);
        if(g_job_cfg[i].lcd_fb_idx > 2)
            g_job_cfg[i].lcd_fb_idx = 0;

        lcd_fb_used[g_job_cfg[i].lcd_fb_idx]++;

        ret = getconfigint(NULL, "type", &g_job_cfg[i].type, fname);
        if(ret != 0) {
            printf("%s: type not exist!\n", fname);
            ret = -1;
            goto err;
        }

        ret = getconfigint(NULL, "engine", &g_job_cfg[i].engine, fname);
        if(ret != 0) {
            printf("%s: engine not exist!\n", fname);
            ret = -1;
            goto err;
        }
        if(g_buf_cfg.ch_from_vg_info) {
            for(n=0; n<VG_INFO_ENTITY_MAX; n++) {
                if(!vg_info_entity[n].valid)
                    break;
                if(vg_info_entity[n].vch == g_job_cfg[i].engine) {
                    g_job_cfg[i].engine = vg_info_entity[n].vcap_ch;
                    break;
                }
            }
        }

        ret = getconfigint(NULL, "minor", &g_job_cfg[i].minor, fname);
        if(ret != 0) {
            printf("%s: minor not exist!\n", fname);
            ret = -1;
            goto err;
        }

        if(g_buf_cfg.split_multi_job) {
            ret = getconfigint(NULL, "split_ch", &g_job_cfg[i].split_ch, fname);
            if(ret != 0) {
                printf("%s: split_ch not exist!\n", fname);
                ret = -1;
                goto err;
            }
        }

        if(msg_disp) {
            if(g_buf_cfg.split_multi_job) {
                printf("<< Config %d Round = %d Type = 0x%x Engine:%d Minor:%d SplitCH:%d >>\n", i,
                       g_job_cfg[i].round_set, g_job_cfg[i].type, g_job_cfg[i].engine, g_job_cfg[i].minor, g_job_cfg[i].split_ch);
            }
            else {
                printf("<< Config %d Round = %d Type = 0x%x Engine:%d Minor:%d >>\n", i,
                       g_job_cfg[i].round_set, g_job_cfg[i].type, g_job_cfg[i].engine, g_job_cfg[i].minor);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////

        j=0;
        param = g_job_cfg[i].param;
        for(k=0; k<ARRAY_SIZE(job_property_name); k++) {
            if(k < MAX_PROPERTYS) {
                ret = getconfiguint(NULL, job_property_name[k], &value, fname);
                if(ret == 0) {
                    strcpy(param[j].name, job_property_name[k]);
                    param[j].value = value;
                    param[j].type  = g_job_cfg[i].type;
                    j++;
                }
            }
            else
                break;
        }

        ////////////////////////////////////////////////////////////////////////////////

        /* query property id via property name */
        ioctl(em_fd, IOCTL_QUERY_ID_SET, param);
        if(msg_disp) {
            for(k=0; k<j; k++) {
                printf("  (%02d)ID:%03d Name:%-20s Value:0x%08x\n", k, param[k].id, param[k].name, param[k].value);
            }
        }

        g_job_cfg[i].enable = 1;

        job_cnt++;
    }

end:
    if(!job_cnt) {
        printf("no any test job!\n");
        ret = -1;
    }

err:
    if(em_fd)
        close(em_fd);

    return ret;
}

int main(int argc, char *argv[])
{
    int i, j, k;
    int ret, job_num;
    frmmap_meminfo_t mem_info;
    struct test_case_t test_cases;
    struct job_case_t *job_cases;
    unsigned int out_user_va  = 0;
    unsigned int out_user_va1 = 0;
    unsigned int out_user_va2 = 0;
    unsigned int lcd_va       = 0;
    unsigned int lcd_va1      = 0;
    unsigned int lcd_va2      = 0;
    unsigned int total_sz     = 0;
    int test_idx              = 1;
    int burnin_seconds        = 0;
    int frammap_fd[2]         = {0, 0};
    FILE *rec_fd[2]           = {0, 0};
    int em_fd                 = 0;
    int mem_fd                = 0;
    int msg_disp              = 0;

    if(argc < 2) {
        print_help();
        return -1;
    }

    test_idx = atoi(argv[1]);
    if(test_idx <= 0 || test_idx > 8) {
        print_help();
        return -1;
    }

    if(argc > 2)
        burnin_seconds = atoi(argv[2]);

    if(argc > 3)
        msg_disp = atoi(argv[3]);

    if(msg_disp)
        printf("Test Case %d startup\n", test_idx);

    /* parse buf config file */
    ret = parse_buf_cfg(test_idx);
    if(ret < 0)
        return ret;

    if(g_buf_cfg.ch_from_vg_info) {
        ret = parse_vg_info();
        if(ret < 0)
            return ret;
    }

    /* parse job config file */
    ret = parse_job_cfg(test_idx, msg_disp);
    if(ret < 0)
        return ret;

    em_fd = open("/dev/em", O_RDWR);
    if(em_fd == 0) {
        perror("open /dev/em failed");
        goto exit;
    }

    if(test_idx == 7 || test_idx == 8) {
        /* open mem device */
        if((mem_fd = open("/dev/mem", O_RDWR)) < 0) {
            perror("open /dev/mem failed");
            goto exit;
        }
    }
    else {
        frammap_fd[0] = open("/dev/frammap0", O_RDWR);
        if(frammap_fd[0] == 0) {
            perror("open /dev/frammap0 failed");
            goto exit;
        }
        ioctl(frammap_fd[0], FRM_IOGBUFINFO, &mem_info);
        printf("Frammap#0 Available memory size is %d bytes.\n", mem_info.aval_bytes);

        frammap_fd[1] = open("/dev/frammap1", O_RDWR);
        if(frammap_fd[1] == 0) {
            perror("open /dev/frammap1 failed");
            goto exit;
        }
        ioctl(frammap_fd[1], FRM_IOGBUFINFO, &mem_info);
        printf("Frammap#1 Available memory size is %d bytes.\n", mem_info.aval_bytes);
    }

    /* caculate output buffer size */
    if(test_idx <= 6) {
        for(k=0; k<g_buf_cfg.round_set; k++) {
            for(i=0; i<MAX_JOBS; i++) {
                if(g_job_cfg[i].enable)
                    total_sz  += ((g_buf_cfg.out_size+g_buf_cfg.out2_size)*g_job_cfg[i].round_set);
            }
        }
        total_sz /= g_buf_cfg.multi_job_pack_num;
    }
    else {
        total_sz  = g_buf_cfg.out_size + g_buf_cfg.out2_size;
    }

    if(total_sz == 0) {
        printf("no job buffer request!!\n");
        goto exit;
    }

    if(test_idx == 7 || test_idx == 8) {
        if(lcd_fb_used[0] && LCD0_BASE_ADDR) {
            lcd_va = (unsigned int)mmap(0, LCD_REG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, LCD0_BASE_ADDR);
            if((int)lcd_va == -1) {
                printf("error to mmap lcd_va\n");
                goto exit;
            }
            out_user_va = (unsigned int)mmap(0, total_sz, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, *((unsigned int *)(lcd_va+LCD_FB_ADDR_REG)));      ///< LCD#0 FB Base Address

            if((int)out_user_va == -1) {
                printf("error to mmap out_user_va\n");
                goto exit;
            }
        }

        if(lcd_fb_used[1] && LCD1_BASE_ADDR) {
            lcd_va1 = (unsigned int)mmap(0, LCD_REG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, LCD1_BASE_ADDR);
            if((int)lcd_va1 == -1) {
                printf("error to mmap lcd_va1\n");
                goto exit;
            }

            out_user_va1 = (unsigned int)mmap(0, total_sz, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, *((unsigned int *)(lcd_va1+LCD_FB_ADDR_REG)));    ///< LCD#1 FB Base Address
            if((int)out_user_va1 == -1) {
                printf("error to mmap out_user_va1\n");
                goto exit;
            }
        }

        if(lcd_fb_used[2] && LCD2_BASE_ADDR) {
            lcd_va2 = (unsigned int)mmap(0, LCD_REG_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, LCD2_BASE_ADDR);
            if((int)lcd_va2 == -1) {
                printf("error to mmap lcd_va2\n");
                goto exit;
            }

            out_user_va2 = (unsigned int)mmap(0, total_sz, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, *((unsigned int *)(lcd_va2+LCD_FB_ADDR_REG)));    ///< LCD#2 FB Base Address
            if((int)out_user_va2 == -1) {
                printf("error to mmap out_user_va2\n");
                goto exit;
            }
        }
    }
    else {
        if(total_sz) {
            out_user_va = (unsigned int)mmap(0, total_sz, PROT_READ|PROT_WRITE, MAP_SHARED, frammap_fd[g_buf_cfg.out_buf_ddr], 0);
            if ((int)out_user_va == -1) {
                printf("error to mmap out_user_va\n");
                goto exit;
            }
        }
    }

    if(msg_disp) {
        if(out_user_va)
            printf("Memory Map out_va  = 0x%08x, size = 0x%08x\n", out_user_va, total_sz);

        if(out_user_va1)
            printf("Memory Map out_va1 = 0x%08x, size = 0x%08x\n", out_user_va1, total_sz);

        if(out_user_va2)
            printf("Memory Map out_va2 = 0x%08x, size = 0x%08x\n", out_user_va2, total_sz);
    }

    /* clear output buffer */
    if(out_user_va)
        memset((void *)out_user_va,  0, total_sz);

    if(out_user_va1)
        memset((void *)out_user_va1, 0, total_sz);

    if(out_user_va2)
        memset((void *)out_user_va2, 0, total_sz);

    /* prepare test job */
    memset(&test_cases, 0, sizeof(test_cases));
    switch(test_idx) {
        case 7: /* Single Job Output to LCD */
            test_cases.case_item = 3;
            break;
        case 8: /* Multi  Job Output to LCD */
            test_cases.case_item = 6;
            break;
        default:
            test_cases.case_item = test_idx;
            break;
    }
    test_cases.burnin_secs = burnin_seconds;
    test_cases.type        = TYPE_CAPTURE;
    job_cases              = test_cases.job_cases;
    job_num                = 0;
    for(k=0; k<g_buf_cfg.round_set; k++) {
        for(i=0; i<MAX_JOBS; i++) {
            if(g_job_cfg[i].enable) {
                for(j=0; j<g_job_cfg[i].round_set; j++) {
                    if(job_num >= MAX_JOBS) {
                        printf("job count#%d over size(MAX: %d)\n", job_num, MAX_JOBS);
                        goto exit;
                    }

                    job_cases[job_num].enable = 1;
                    if(g_buf_cfg.split_multi_job) {
                        job_cases[job_num].engine = (g_job_cfg[i].engine | VCAP_VG_FD_ENGINE_VIMODE_SPLIT_IDX);
                        job_cases[job_num].minor  = VCAP_VG_FD_MINOR_WITH_SPLIT_CH(g_job_cfg[i].minor, g_job_cfg[i].split_ch);
                    }
                    else {
                        job_cases[job_num].engine = g_job_cfg[i].engine;
                        job_cases[job_num].minor  = g_job_cfg[i].minor;
                    }
                    job_cases[job_num].status       = 0;
                    job_cases[job_num].in_buf_type  = g_buf_cfg.out_buf_type;
                    job_cases[job_num].out_buf_type = g_buf_cfg.out_buf_type;
                    if((test_idx == 7) || (test_idx == 8)) {
                        switch(g_job_cfg[i].lcd_fb_idx) {
                            case 1:
                                job_cases[job_num].in_mmap_va[0]  = out_user_va1;
                                job_cases[job_num].out_mmap_va[0] = out_user_va1;
                                break;
                            case 2:
                                job_cases[job_num].in_mmap_va[0]  = out_user_va2;
                                job_cases[job_num].out_mmap_va[0] = out_user_va2;
                                break;
                            default:
                                job_cases[job_num].in_mmap_va[0]  = out_user_va;
                                job_cases[job_num].out_mmap_va[0] = out_user_va;
                                break;
                        }
                    }
                    else {
                        job_cases[job_num].in_mmap_va[0]  = out_user_va + ((job_num/g_buf_cfg.multi_job_pack_num)*(g_buf_cfg.out_size+g_buf_cfg.out2_size));
                        job_cases[job_num].out_mmap_va[0] = out_user_va + ((job_num/g_buf_cfg.multi_job_pack_num)*(g_buf_cfg.out_size+g_buf_cfg.out2_size));
                    }

                    build_property(job_cases[job_num].in_prop, g_job_cfg[i].param);

                    job_num++;
                }
            }
        }
    }

    if(msg_disp) {
        printf("Press ENTER to start...\n");
        getchar();
    }

    /* Pub Test Job to Test Enging */
    ret = ioctl(em_fd, IOCTL_TEST_CASES, &test_cases);
    if(ret < 0) {
        printf("AP IOCTL_TEST_CASES failed!\n");
        goto exit;
    }

    /* result */
    if((test_cases.case_item==1) || (test_cases.case_item==4)) {
        for(i=0; i<job_num; i++) {
            struct video_param_t param;

            printf("Job#%d output Property (status: %08x)>>>>\n", test_cases.job_cases[i].id, test_cases.job_cases[i].status);

            for(j=0; j<MAX_PROPERTYS; j++) {
                if(test_cases.job_cases[i].out_prop[j].id == 0) // last property
                    break;

                param.type  = test_cases.type;
                param.id    = test_cases.job_cases[i].out_prop[j].id;
                param.value = test_cases.job_cases[i].out_prop[j].value;

                /* query string of property id */
                ioctl(em_fd, IOCTL_QUERY_STR, &param);
                printf("    %s: 0x%x\n", param.name, param.value);
            }

            /* write raw data to file */
            if((i%g_buf_cfg.multi_job_pack_num) == 0) {
                if(g_buf_cfg.out_buf_type == TYPE_YUV422_2FRAMES_2FRAMES) {
                    do_record(&rec_fd[0], TYPE_YUV422_2FRAMES, test_cases.job_cases[i].out_mmap_va[0], g_buf_cfg.out_size, 1);
                    do_record(&rec_fd[1], TYPE_YUV422_FRAME,   test_cases.job_cases[i].out_mmap_va[0]+g_buf_cfg.out_size,  g_buf_cfg.out2_size, 2);
                }
                else if(g_buf_cfg.out_buf_type == TYPE_YUV422_FRAME_2FRAMES) {
                    do_record(&rec_fd[0], TYPE_YUV422_FRAME, test_cases.job_cases[i].out_mmap_va[0], g_buf_cfg.out_size, 1);
                    do_record(&rec_fd[1], TYPE_YUV422_FRAME, test_cases.job_cases[i].out_mmap_va[0]+g_buf_cfg.out_size,  g_buf_cfg.out2_size, 2);
                }
                else if(g_buf_cfg.out_buf_type == TYPE_YUV422_FRAME_FRAME) {    ///< only main job
                    do_record(&rec_fd[0], TYPE_YUV422_FRAME, test_cases.job_cases[i].out_mmap_va[0], g_buf_cfg.out_size, 1);
                }
                else
                    do_record(&rec_fd[0], test_cases.job_cases[i].out_buf_type, test_cases.job_cases[i].out_mmap_va[0], g_buf_cfg.out_size, 0);
            }
        }
    }

    printf("\n###### Test Done ######\n");

exit:
    if(lcd_va)
        munmap((void *)lcd_va, LCD_REG_SIZE);

    if(lcd_va1)
        munmap((void *)lcd_va1, LCD_REG_SIZE);

    if(lcd_va2)
        munmap((void *)lcd_va2, LCD_REG_SIZE);

    if(out_user_va)
        munmap((void *)out_user_va, total_sz);

    if(out_user_va1)
        munmap((void *)out_user_va1, total_sz);

    if(out_user_va2)
        munmap((void *)out_user_va2, total_sz);

    if(em_fd)
        close(em_fd);

    if(frammap_fd[0])
        close(frammap_fd[0]);

    if(frammap_fd[1])
        close(frammap_fd[1]);

    if(mem_fd)
        close(mem_fd);

    if(rec_fd[0])
        fclose(rec_fd[0]);

    if(rec_fd[1])
        fclose(rec_fd[1]);

    return 0;
}
