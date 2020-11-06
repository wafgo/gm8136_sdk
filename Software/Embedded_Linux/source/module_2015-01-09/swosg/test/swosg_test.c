#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <poll.h>
#include <asm/ioctl.h>
#include <frammap/frammap_if.h>
#include <swosg/swosg_if.h>

typedef struct __swosg_blit_param {
    unsigned int               dest_bg_addr;   /*destination address*/
    unsigned short             dest_bg_w;      /*destination width*/   
    unsigned short             dest_bg_h;      /*destination height*/
    unsigned int               src_addr;       /*patten source address*/
    unsigned short             src_w;          /*patten source width*/
    unsigned short             src_h;          /*patten source height*/
    unsigned int               bg_color;       /*patten background color (UYVY U<<24|Y<<16|V<<8|Y)*/
    unsigned short             target_x;       /*blit target x*/
    unsigned short             target_y;       /*blit target y*/   
    unsigned short             blending;       /*alpha blending (0:0% 1:25% 2:37% 3:50% 4:62% 5:75% 6:87% 7:100%)*/
    SWOSG_WIN_ALIGN_T          align_type;     /*align type*/
    unsigned short             src_patten_idx; 
}swosg_blit_param_t;

swosg_if_mask_param_t mask_cfg[2];
swosg_blit_param_t    blit_cfg[2];



#define MAX_CFG 1
char filename[100];

int read_param_line(char *line, int size, FILE *fd, unsigned long long *offset)
{
    int ret;
    while((ret=(int)fgets(line,size,fd))!=0) {
        if(strchr(line,(int)';'))
            continue;
        if(strchr(line,(int)'='))
            return 0;
        if(strchr(line,(int)'\n'))
            continue;
    }
    return -1;
}

int parse_blit(void)
{
    int i=1,j=0,k,len;
    FILE *fd;
    char *next_str;
    int value,type,capvlaue;
    char line[1024];
    unsigned long long offset;   

    for(i=0;i<MAX_CFG;i++) {
        sprintf(filename,"%d.blit",i);
        fd=fopen(filename,"r");        
        if (fd <= 0)
            break;  
        
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
       
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "dest_w=0x%hx\n", &blit_cfg[i].dest_bg_w);
        else
            sscanf(line, "dest_w=%hd\n", &blit_cfg[i].dest_bg_w);
            
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "dest_h=0x%hx\n", &blit_cfg[i].dest_bg_h);
        else
            sscanf(line, "dest_h=%hd\n", &blit_cfg[i].dest_bg_h);
       

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "src_w=0x%hx\n", &blit_cfg[i].src_w);
        else
            sscanf(line, "src_w=%hd\n", &blit_cfg[i].src_w);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "src_h=0x%hx\n", &blit_cfg[i].src_h);
        else
            sscanf(line, "src_h=%hd\n", &blit_cfg[i].src_h);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[9]=='0')&&(line[10]=='x'))
            sscanf(line, "bg_color=0x%x\n", &blit_cfg[i].bg_color);
        else
            sscanf(line, "bg_color=%d\n", &blit_cfg[i].bg_color);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_x=0x%hx\n", &blit_cfg[i].target_x);
        else
            sscanf(line, "target_x=%hd\n", &blit_cfg[i].target_x);
        
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_y=0x%hx\n", &blit_cfg[i].target_y);
        else
            sscanf(line, "target_y=%hd\n", &blit_cfg[i].target_y);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "blending=0x%hx\n", &blit_cfg[i].blending);
        else
            sscanf(line, "blending=%hd\n", &blit_cfg[i].blending);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "align_type=0x%hx\n", &blit_cfg[i].align_type);
        else
            sscanf(line, "align_type=%hd\n", &blit_cfg[i].align_type);

        
        printf("blit%d dest_bg_w=%d dest_bg_h=%d src_w=%d" 
               "src_h=%d bg_color=%x \n",
               i,blit_cfg[i].dest_bg_w,blit_cfg[i].dest_bg_h,blit_cfg[i].src_w
               ,blit_cfg[i].src_h,blit_cfg[i].bg_color);fflush(stdout);

        printf("blit%d target_x=%d target_y=%d blending=%d align_type=%d\n",
               i,blit_cfg[i].target_x,blit_cfg[i].target_y,blit_cfg[i].blending,blit_cfg[i].align_type);fflush(stdout);
 
    }
    if(i==0)
        return -1;
    return 0;
}

int parse_cfg(void)
{
    int i=1,j=0,k,len;
    FILE *fd;
    char *next_str;
    int value,type,capvlaue;
    char line[1024];
    unsigned long long offset;   

    for(i=0;i<MAX_CFG;i++) {
        sprintf(filename,"%d.cfg",i);
        fd=fopen(filename,"r");        
        if (fd <= 0)
            break;  
        
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
       
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "dest_w=0x%hx\n", &mask_cfg[i].dest_bg_w);
        else
            sscanf(line, "dest_w=%hd\n", &mask_cfg[i].dest_bg_w);
            
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "dest_h=0x%hx\n", &mask_cfg[i].dest_bg_h);
        else
            sscanf(line, "dest_h=%hd\n", &mask_cfg[i].dest_bg_h);
       

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "border_size=0x%hx\n", &mask_cfg[i].border_size);
        else
            sscanf(line, "border_size=%hd\n", &mask_cfg[i].border_size);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "palette_idx=0x%hx\n", &mask_cfg[i].palette_idx);
        else
            sscanf(line, "palette_idx=%hd\n", &mask_cfg[i].palette_idx);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "border_type=0x%x\n", &mask_cfg[i].border_type);
        else
            sscanf(line, "border_type=%d\n", &mask_cfg[i].border_type);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "blending=0x%hx\n", &mask_cfg[i].blending);
        else
            sscanf(line, "blending=%hd\n", &mask_cfg[i].blending);
        
        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "align_type=0x%x\n", &mask_cfg[i].align_type);
        else
            sscanf(line, "align_type=%d\n", &mask_cfg[i].align_type);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_x=0x%hx\n", &mask_cfg[i].target_x);
        else
            sscanf(line, "target_x=%hd\n", &mask_cfg[i].target_x);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_y=0x%hx\n", &mask_cfg[i].target_y);
        else
            sscanf(line, "target_y=%hd\n", &mask_cfg[i].target_y);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_w=0x%hx\n", &mask_cfg[i].target_w);
        else
            sscanf(line, "target_w=%hd\n", &mask_cfg[i].target_w);

        if (read_param_line(line, sizeof(line), fd, &offset) < 0)
            break;
        if((line[10]=='0')&&(line[11]=='x'))
            sscanf(line, "target_h=0x%hx\n", &mask_cfg[i].target_h);
        else
            sscanf(line, "target_h=%hd\n", &mask_cfg[i].target_h);

        printf("config%d dest_bg_w=%d dest_bg_h=%d border_size=%d" 
               "palette_idx=%d border_type=%d blending=%d align_type=%d\n",
               i,mask_cfg[i].dest_bg_w,mask_cfg[i].dest_bg_h,mask_cfg[i].border_size
               ,mask_cfg[i].palette_idx,mask_cfg[i].border_type,mask_cfg[i].blending,mask_cfg[i].align_type);fflush(stdout);

        printf("config%d target_x=%d target_y=%d target_w=%d target_h=%d\n",
               i,mask_cfg[i].target_x,mask_cfg[i].target_y,mask_cfg[i].target_w,mask_cfg[i].target_h);fflush(stdout);
 
    }
    if(i==0)
        return -1;
    return 0;
}
void prepare_srcimg_buffer(const char *src_file,unsigned int start,unsigned int buffer_sz)
{
    int i;
    FILE *pattern_fd=0;
    char pattern_name[100];
    unsigned int addr, sz;
    unsigned char * buff_s = (unsigned char*)start;
    unsigned char * buff_c;
    /* prepare pattern 1 */        
    sprintf(pattern_name,src_file);
    pattern_fd=fopen(pattern_name,"r");
    if(pattern_fd <= 0){
        printf("open %s fail!\n",pattern_name);
        return ;
    }
        
    printf("open %s %x\n",pattern_name,buff_s);
    fflush(stdout);
    fseek(pattern_fd, 0, SEEK_END);
    printf("%s %d\n",__func__,__LINE__);
    sz = ftell(pattern_fd);
    printf("%s %d\n",__func__,__LINE__);
    fseek(pattern_fd, 0, SEEK_SET);
    printf("%s %d\n",__func__,__LINE__);

    printf("buffer=%x \n",(unsigned int)buff_s);
    fflush(stdout);
    if(pattern_fd > 0) {
        fread((void *)buff_s,1, buffer_sz, pattern_fd);
        printf("Read %x Done!\n",(unsigned int*)buff_s);
    }
    else
        printf("open %s fail!\n",pattern_name);

    
    for(i=1;i<MAX_CFG;i++) {
        
        buff_c=buff_s+(i*buffer_sz);
        memcpy((void *)buff_c, (void *)buff_s, sz);
    }   
    fflush(stdout);
    fclose(pattern_fd);
#if 0
    for(i = 0;i<TEST_NUM;i++)
    {
        buff_c=buff_s+(i*buffer_sz);
        do_record(TYPE_YUV422_FRAME,(unsigned int)buff_c,i,sz);
    }
#endif  

}
void prepare_input_buffer(const char *dest_file,unsigned int start,unsigned int buffer_sz)
{
    int i;
    FILE *pattern_fd=0;
    char pattern_name[100];
    unsigned int addr, sz;
    unsigned char * buff_s = (unsigned char*)start;
    unsigned char * buff_c;
    /* prepare pattern 1 */        
    sprintf(pattern_name,dest_file);
    pattern_fd=fopen(pattern_name,"r");
    if(pattern_fd <= 0){
    	printf("open %s fail!\n",pattern_name);
        return ;
    }
        
    printf("open %s %x\n",pattern_name,buff_s);
    fflush(stdout);
    fseek(pattern_fd, 0, SEEK_END);
    printf("%s %d\n",__func__,__LINE__);
    sz = ftell(pattern_fd);
    printf("%s %d\n",__func__,__LINE__);
    fseek(pattern_fd, 0, SEEK_SET);
    printf("%s %d\n",__func__,__LINE__);

    printf("buffer=%x \n",(unsigned int)buff_s);
    fflush(stdout);
    if(pattern_fd > 0) {
      	fread((void *)buff_s,1, buffer_sz, pattern_fd);
    	printf("Read %x Done!\n",(unsigned int*)buff_s);
    }
    else
      	printf("open %s fail!\n",pattern_name);

    
    for(i=1;i<MAX_CFG;i++) {
        
    	buff_c=buff_s+(i*buffer_sz);
    	memcpy((void *)buff_c, (void *)buff_s, sz);
    }   
    fflush(stdout);
    fclose(pattern_fd);
#if 0
    for(i = 0;i<TEST_NUM;i++)
    {
        buff_c=buff_s+(i*buffer_sz);
        do_record(TYPE_YUV422_FRAME,(unsigned int)buff_c,i,sz);
    }
#endif    
}

void prepare_to_convert_canvas(int i ,swosg_if_canvas_t *canvas,swosg_blit_param_t* blit_src )
{
    canvas->idx = i;
    canvas->src_cava_addr = blit_src->src_addr;
    canvas->src_w = blit_src->src_w;
    canvas->src_h = blit_src->src_h;
    printf("addr=%x,w=%d,h=%d\n",canvas->src_cava_addr,canvas->src_w,canvas->src_h);fflush(stdout);
}

void prepare_to_convert_blit_param(int i,swosg_if_blit_param_t* blit_rel ,swosg_blit_param_t* blit_src)
{
    blit_rel->dest_bg_addr =  blit_src->dest_bg_addr;
    blit_rel->dest_bg_w =  blit_src->dest_bg_w; 
    blit_rel->dest_bg_h =  blit_src->dest_bg_h;
    blit_rel->target_x =  blit_src->target_x; 
    blit_rel->target_y =  blit_src->target_y; 
    blit_rel->align_type =  blit_src->align_type; 
    blit_rel->src_patten_idx = i;;   

}

FILE *record_fd[MAX_CFG]={0};//{0,0};
void do_record(unsigned int addr_va,int num,int size)
{
    if(record_fd[num]==0) {
        sprintf(filename,"output_%d.yuv",num);
        record_fd[num]=fopen(filename,"wb");
    }

    if(record_fd[num]) {
        fwrite((void *)addr_va,1,size,record_fd[num]);
        printf("Record to addr = 0x%x, size = 0x%x\n",(void *)addr_va,size);
        //printf("Record to %s Done!\n",filename);
    }
}

int swosg_test_do_blit(const char *dest_file,const char *src_file)
{

    int mem_fd[2],i,osd_fd,ret;
    unsigned int total_sz=0 , in_user_va,in_size=0,src_va,src_size=0,total_src_size=0;
    frmmap_meminfo_t mem_info;
    alloc_type_t allocate_type;
    swosg_if_blit_param_t real_blit_cfg;
    swosg_if_canvas_t     real_cavs_cfg; 

    osd_fd = open("/dev/swosg_t", O_RDWR);
    if(osd_fd == 0) {
        perror("open /dev/swosg_t failed");
        return -1;
    }
    mem_fd[0] = open("/dev/frammap0", O_RDWR);
    if(mem_fd == 0) {
        perror("open /dev/frammap0 failed");
        return -1;
    }

    mem_fd[1] = open("/dev/frammap0", O_RDWR);
    if(mem_fd == 0) {
        close(osd_fd);
        close(mem_fd[0]);
        perror("open /dev/frammap0 mem_fd1 failed");
        return -1;
    }
    
    allocate_type = ALLOC_CACHEABLE;
        
    ioctl(mem_fd[0],FRM_ALLOC_TYPE,&allocate_type);
        
    ioctl(mem_fd[0],FRM_IOGBUFINFO, &mem_info);
    printf("Available memory size is %d bytes.\n",mem_info.aval_bytes);fflush(stdout);

    //allocate_type = ALLOC_CACHEABLE;
        
    //ioctl(mem_fd[1],FRM_ALLOC_TYPE,&allocate_type);
        
    //ioctl(mem_fd[1],FRM_IOGBUFINFO, &mem_info);
    
    printf("Available memory size is %d bytes.\n",mem_info.aval_bytes);fflush(stdout);

    if(parse_blit()<0){
        printf("parsecfg is error\n");
        close(osd_fd);
        close(mem_fd[0]);
        close(mem_fd[1]);
        return -1;
    }
    in_size = blit_cfg[0].dest_bg_w * blit_cfg[0].dest_bg_h*2;
    total_sz = in_size*MAX_CFG;

    in_user_va=(unsigned int)mmap(0, total_sz,PROT_READ|PROT_WRITE, MAP_SHARED,mem_fd[0], 0);
    printf("in_user_va=%x\n",(unsigned int)in_user_va);
    if (in_user_va <= 0) {
        printf("Error to mmap in_user_va\n");fflush(stdout);
        close(osd_fd);
        close(mem_fd[0]);
        close(mem_fd[1]);
        return -1;
    }
    printf("MMAP in_va=0x%x/0x%x\n",in_user_va,total_sz);fflush(stdout);
    memset((void *)in_user_va,0,total_sz);  

    src_size = blit_cfg[0].src_w* blit_cfg[0].src_h*2;
    total_src_size = src_size*2;
    
    src_va =(unsigned int)mmap(0, total_src_size,PROT_READ|PROT_WRITE,MAP_SHARED,mem_fd[1], 0);
    printf("src_va=%x\n",(unsigned int)src_va);
    if (src_va <= 0) {
        printf("Error to mmap src_va\n");fflush(stdout);
        close(osd_fd);
        close(mem_fd[0]);
        close(mem_fd[1]);
        return -1;
    }
    printf("MMAP in_va src=0x%x/0x%x\n",src_va,total_src_size);fflush(stdout);
    memset((void *)src_va,0,total_src_size);  

    
    prepare_input_buffer(dest_file,in_user_va,in_size);
    prepare_srcimg_buffer(src_file,src_va,src_size);

   
    
    for(i=0;i<MAX_CFG;i++){
        blit_cfg[i].dest_bg_addr = in_user_va+(i*in_size);
        blit_cfg[i].src_addr= src_va+(i*src_size);
    }
    for(i=0;i<MAX_CFG;i++){
        prepare_to_convert_canvas( i ,&real_cavs_cfg,&blit_cfg[i]);
        ret = ioctl(osd_fd, SWOSG_CMD_CANVAS_SET, &real_cavs_cfg);
        if(ret<0) 
            printf("AP SWOSG_CMD_CANVAS_SET failed!\n");
    }
    printf("Press ENTER to start...\n");getchar();

    for(i=0;i<MAX_CFG;i++){
        prepare_to_convert_blit_param(i,&real_blit_cfg,&blit_cfg[i]);
        ret = ioctl(osd_fd, SWOSG_CMD_BLIT,&real_blit_cfg );
        if(ret<0) 
            printf("AP IOCTL_TEST_CASES failed!\n");
    }

    /* result */
    for(i=0;i<MAX_CFG;i++) {
        int frame_size=0;     
        do_record(blit_cfg[i].dest_bg_addr,i,in_size);         
    }
    
    munmap((void *)in_user_va,total_sz);
    munmap((void *)src_va,src_size);
    close(osd_fd);
    close(mem_fd[0]);
    close(mem_fd[1]);
    for(i=0;i<MAX_CFG;i++){
    if(record_fd[i])
        fclose(record_fd[i]);
    }

    printf("\n###### Test Done ######\n");
    return 0;
  
}

int swosg_test_do_mask(const char *filename)
{
    int mem_fd,i,osd_fd,ret;
    unsigned int total_sz=0 , in_user_va,in_size=0;
    frmmap_meminfo_t mem_info;
    alloc_type_t allocate_type;

    osd_fd = open("/dev/swosg_t", O_RDWR);
    if(osd_fd == 0) {
        perror("open /dev/swosg_t failed");
        return -1;
    }
    mem_fd = open("/dev/frammap0", O_RDWR);
    if(mem_fd == 0) {
        perror("open /dev/frammap0 failed");
        return -1;
    }   
    
    allocate_type = ALLOC_CACHEABLE;
        
    ioctl(mem_fd,FRM_ALLOC_TYPE,&allocate_type);
        
    ioctl(mem_fd,FRM_IOGBUFINFO, &mem_info);
    printf("Available memory size is %d bytes.\n",mem_info.aval_bytes);fflush(stdout);

    if(parse_cfg()<0){
        printf("parsecfg is error\n");
        close(osd_fd);
        close(mem_fd);
        return -1;
    }
    in_size = mask_cfg[0].dest_bg_w * mask_cfg[0].dest_bg_h*2;
    total_sz = in_size*MAX_CFG;

    in_user_va=(unsigned int)mmap(0, total_sz,PROT_READ|PROT_WRITE, MAP_SHARED,mem_fd, 0);
    printf("in_user_va=%x\n",(unsigned int)in_user_va);
    if (in_user_va <= 0) {
        printf("Error to mmap in_user_va\n");fflush(stdout);
        close(osd_fd);
        close(mem_fd);
        return -1;
    }
    printf("MMAP in_va=0x%x/0x%x\n",in_user_va,total_sz);fflush(stdout);
    memset((void *)in_user_va,0,total_sz);  

    prepare_input_buffer(filename,in_user_va,in_size);

    for(i=0;i<MAX_CFG;i++){
        mask_cfg[i].dest_bg_addr = in_user_va+(i*in_size);;
    }

    printf("Press ENTER to start...\n");getchar();

    for(i=0;i<MAX_CFG;i++){
        ret = ioctl(osd_fd, SWOSG_CMD_MASK, &mask_cfg[i]);
        if(ret<0) 
            printf("AP IOCTL_TEST_CASES failed!\n");
    }

    /* result */
    for(i=0;i<MAX_CFG;i++) {
        int frame_size=0;     
        do_record(mask_cfg[i].dest_bg_addr,i,in_size);         
    }
    
    munmap((void *)in_user_va,total_sz);
    close(osd_fd);
    close(mem_fd);
    for(i=0;i<MAX_CFG;i++){
    if(record_fd[i])
        fclose(record_fd[i]);
    }

    printf("\n###### Test Done ######\n");
    return 0;
}

int main(int argc,char *argv[])
{
    char *g_destimagefile;
    char *g_srcimagefile;
    int n,dx,dy;
    int do_mask = 0,do_blit=0,is_destimg=0,is_srcimg=0;
    
    for (n = 1; n < argc; n++) {
          if (strncmp (argv[n], "--", 2) == 0) {
              if (strcmp (argv[n] + 2, "domask") == 0 ){
                    
                    do_mask = 1; 
                    continue; 
              }
              else if (strcmp (argv[n] + 2, "doblit") == 0 ){
                      do_blit = 1; 
                      continue;
              }
              else if (strcmp (argv[n] + 2, "dest_img") == 0 &&
                     ++n < argc && argv[n]){
                     g_destimagefile = argv[n];
                     is_destimg = 1; 
                      continue;
              }else if (strcmp (argv[n] + 2, "src_img") == 0 &&
                     ++n < argc && argv[n]){
                     g_srcimagefile = argv[n];
                     is_srcimg = 1; 
                      continue;
              }
          }
    }
    if(do_mask && do_blit){
        printf("can't do mask and blit at the same time\n");
        return 0;
    }

    if(do_mask && !is_destimg){
        printf("please add dest image\n");
        return 0;
    }

    if(do_blit && (!is_destimg|| !is_srcimg)){
        printf("please add dest or src image\n");
        return 0;
    }
    if(do_mask)
        swosg_test_do_mask(g_destimagefile);

    if(do_blit)
        swosg_test_do_blit(g_destimagefile,g_srcimagefile);
}

