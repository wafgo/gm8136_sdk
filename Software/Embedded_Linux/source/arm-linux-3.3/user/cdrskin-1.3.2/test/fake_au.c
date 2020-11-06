
/* fake_au
   Fakes a file in SUN .au format from a raw little-endian PCM audio file
   (e.g. a file extracted from .wav by test/dewav). The input data are assumed
   to be 16 bit, stereo, 44100 Hz.
   Copyright (C) 2006 Thomas Schmitt <scdbackup@gmx.net>, provided under GPL

   Info used: http://www.opengroup.org/public/pubs/external/auformat.html
*/


#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


int fake_write(unsigned char *buf, size_t size, FILE *fp)
{
 int ret;

 ret= fwrite(buf,size,1,fp);
 if(ret==1)
   return(1);
 fprintf(stderr,"Error %d while writing: '%s'\n",errno,strerror(errno));
 return(0);
}


int main(int argc, char **argv)
{
 int ret, i;
 unsigned data_size= 0,byte_count,exit_value= 0;
 FILE *fp_out= stdout,*fp_in= stdin;
 unsigned char buf[4];
 char out_path[4096],in_path[4096];
 struct stat stbuf;

 strcpy(out_path,"-");
 strcpy(in_path,"");
 if(argc < 2) {
   exit_value= 1;
   goto help;
 }
 for(i= 1; i<argc; i++) {
   if(strcmp(argv[i],"-o")==0) {
     if(i>=argc-1) {
       fprintf(stderr,"%s: option -o needs a file address as argument.\n",
               argv[0]);
       exit(1);
     }
     i++;
     strcpy(out_path, argv[i]);
   } else if(strcmp(argv[i],"--stdin_size")==0) {
     if(i>=argc-1) {
       fprintf(stderr,"%s: option --stdin_size needs a number as argument.\n",
               argv[0]);
       exit(1);
     }
     i++;
     sscanf(argv[i],"%u",&data_size);
   } else if(strcmp(argv[i],"--help")==0) {
     exit_value= 0;
help:;
     fprintf(stderr,"usage: %s \\\n", argv[0]);
     fprintf(stderr,"       [-o output_path|\"-\"] [source_path | --stdin_size size]\n");
     fprintf(stderr,
         "Disguises an extracted .wav stream as .au stereo, 16bit, 44100Hz\n");
     fprintf(stderr,
      "stdin gets byte-swapped and appended up to the announced data_size.\n");
     exit(exit_value);
   } else {
     if(in_path[0]!=0) {
       fprintf(stderr,"%s: only one input file is allowed.\n", argv[0]);
       exit(1);
     }
     strcpy(in_path, argv[i]);
   }
 }

 if(strcmp(in_path,"-")==0 || in_path[0]==0) {
   if(data_size==0) {
     fprintf(stderr,"%s: input from stdin needs option --stdin_size.\n",
             argv[0]);
     exit(6);
   }
   fp_in= stdin; 
 } else {
   fp_in= fopen(in_path,"r");
   if(stat(in_path,&stbuf)!=-1)
     data_size= stbuf.st_size;
 }
 if(fp_in==NULL) {
   fprintf(stderr,"Error %d while fopen(\"%s\",\"r\") : '%s'\n",
           errno,in_path,strerror(errno));
   exit(2);
 }

 if(strcmp(out_path,"-")==0) {
   fp_out= stdout; 
 } else {
   if(stat(out_path,&stbuf)!=-1) {
     fprintf(stderr,"%s: file '%s' already existing\n",argv[0],out_path);
     exit(4);
   }
   fp_out= fopen(out_path,"w");
 }
 if(fp_out==NULL) {
   fprintf(stderr,"Error %d while fopen(\"%s\",\"w\") : '%s'\n",
           errno,out_path,strerror(errno));
   exit(2);
 }

 fake_write((unsigned char *) ".snd",4,fp_out);        /* magic number */
 buf[0]= buf[1]= buf[2]= 0;
 buf[3]= 32;
 fake_write(buf,4,fp_out);           /* data_location */
 buf[0]= (data_size>>24)&0xff;
 buf[1]= (data_size>>16)&0xff;
 buf[2]= (data_size>>8)&0xff;
 buf[3]= (data_size)&0xff;
 fake_write(buf,4,fp_out);           /* data_size */
 buf[0]= buf[1]= buf[2]= 0;
 buf[3]= 3;
 fake_write(buf,4,fp_out);           /* encoding 16 Bit PCM */
 buf[0]= buf[1]= 0;
 buf[2]= 172;
 buf[3]= 68;
 fake_write(buf,4,fp_out);           /* sample rate 44100 Hz */
 buf[0]= buf[1]= buf[2]= 0;
 buf[3]= 2;
 fake_write(buf,4,fp_out);           /* number of channels */
 buf[0]= buf[1]= buf[2]= buf[3]= 0;
 fake_write(buf,4,fp_out);           /* padding */
 fake_write(buf,4,fp_out);           /* padding */

 for(byte_count= 0; byte_count<data_size; byte_count+=2) {
   ret= fread(buf,2,1,fp_in);
   if(ret<=0) {
     fprintf(stderr,"Premature end end of input\n");
     exit_value= 5;
 break;
   }
   buf[3]= buf[0];
   buf[2]= buf[1];
   ret= fake_write(buf+2,2,fp_out);
   if(ret<=0) {
     exit_value= 3;
 break;
   }
 }
 if(fp_out!=stdout)
   fclose(fp_out);
 if(fp_in!=stdin)
   fclose(fp_in);
 fprintf(stderr, "Swapped and appended: %u stdin bytes\n",byte_count);
 exit(exit_value);
}

