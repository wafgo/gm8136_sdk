
#define GPIO11_ID               11
#define GPIO11_DET_HIGH         (1<<GPIO11_ID)
#define GPIO11_DET_LOW          0




SYS_STATUS I2C_ByteWrite(unsigned char addr, unsigned char dat);
unsigned char I2C_ByteRead(unsigned char addr);
void TI_ByteWrite(unsigned char addr, unsigned char dat);
void I2C_ReadArray (unsigned char* dest_addr, unsigned char src_addr,
                       unsigned char len);

void Delay(void);
void DelayMs(unsigned ms);

void sli10121_timer_init(void);
void sli10121_timer_destroy(void);
void sli10121_i2c_init(void);
void sli10121_i2c_exit(void);
void timer_process(void *parm);
void start_timer(void);
void stop_timer(void);
void sli10121_reset_audio(void);


