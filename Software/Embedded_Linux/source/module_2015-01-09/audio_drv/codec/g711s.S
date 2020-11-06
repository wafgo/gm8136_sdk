.global G711_alaw2linear	
.global G711_linear2alaw	
.global G711_ulaw2linear	
.global G711_linear2ulaw		
.global G711_alaw2ulaw	
.global G711_ulaw2alaw	

G711_alaw2linear:	
   eor      r2,r0,#0x55
   and      r0,r2,#0xf
   mov      r0,r0,lsl #4
   mov      r1,r2,lsr #4
   and      r1,r1,#7
   subs     r1,r1,#1
   addlt    r0,r0,#8
   addge    r0,r0,#0x108
   movgt    r0,r0,lsl r1
   tst      r2,#0x80
   rsbeq    r0,r0,#0
   mov      pc,r14

G711_linear2alaw:	
   rsbs     r1,r0,#0
   movle    r3,#0xd5
   movgt    r3,#0x55
   subgt    r0,r1,#8
   mov      r1,#0
   movs     r2,r0,lsr #4
   ble      bench0 @0xb2c8  @ (G711_linear2alaw + 0x48)
   cmp      r0,#0x1000
   movge    r0,r0,lsr #4
   addge    r1,r1,#4
   cmp      r0,#0x400
   movge    r0,r0,lsr #2
   addge    r1,r1,#2
   cmp      r0,#0x100
   movge    r0,r0,lsr #1
   addge    r1,r1,#1
   cmp      r0,#0x100
   addge    r1,r1,#1
bench0:
   cmp      r1,#8
   movge    r0,#0x7f
   movlt    r0,r1,lsl #4
   subs     r1,r1,#1
   movgt    r2,r2,lsr r1
   and      r2,r2,#0xf
   orr      r0,r0,r2
   eor      r0,r0,r3  
   mov      pc,r14

G711_linear2ulaw:	
   cmp      r0,#0
   rsblt    r0,r0,#0x84
   movlt    r1,#0x7f
   addge    r0,r0,#0x84
   movge    r1,#0xff
   mov      r2,#0
   movs     r3,r0,lsr #3
   ble      bench1 @0xb338  @ (G711_linear2ulaw + 0x4c)
   cmp      r0,#0x1000
   movge    r0,r0,lsr #4
   addge    r2,r2,#4
   cmp      r0,#0x400
   movge    r0,r0,lsr #2
   addge    r2,r2,#2
   cmp      r0,#0x100
   movge    r0,r0,lsr #1
   addge    r2,r2,#1
   cmp      r0,#0x100
   addge    r2,r2,#1
bench1:
   cmp      r2,#8
   movge    r0,#0x7f
   movlt    r0,r3,asr r2
   andlt    r0,r0,#0xf
   orrlt    r0,r0,r2,lsl #4
   eor      r0,r0,r1 
   mov      pc,r14

G711_ulaw2linear:
   mvns     r1,r0,lsl #25
   mov      r1,r1,lsr #29
   mvn      r0,r0,lsl #3
   and      r0,r0,#0x78
   add      r0,r0,#0x84
   mov      r0,r0,lsl r1
   subcs    r0,r0,#0x84
   rsbcc    r0,r0,#0x84  
   mov      pc,r14 
   
G711_alaw2ulaw:	
   b        a2u_skip_table
G711_a2u_lookup_ptr:	
   .word      0x0000b524  
a2u_skip_table:	
   ldr      r2,G711_a2u_lookup_ptr @ = #G711_a2u_lookup_ptr
   and      r1,r0,#0x7f
   eor      r1,r1,#0x55
   ldrb     r1,[r2,r1]
   eor      r1,r1,#0x7f
   and      r0,r0,#0x80
   orr      r0,r0,r1  
   mov      pc,r14

G711_ulaw2alaw:	
   b        u2a_skip_table
G711_u2a_lookup_ptr:	
  .word      0x0000b5a4  
u2a_skip_table:	
   ldr      r2,G711_u2a_lookup_ptr @ = #G711_u2a_lookup_ptr
   and      r1,r0,#0x7f
   eor      r1,r1,#0x7f
   ldrb     r1,[r2,r1]
   sub      r1,r1,#1
   eor      r1,r1,#0x55
   and      r0,r0,#0x80
   orr      r0,r0,r1  
   mov      pc,r14

