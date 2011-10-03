#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>

typedef struct{
  char riff[4];           
  u_int32_t filelen;
  char wave[4];
  char  fmt[4];
  u_int32_t hdrlen;
  u_int16_t  wFormatTag;
  u_int16_t  wChannels;
  u_int32_t dwSamplesPerSec;
  u_int32_t dwAvgBytesPerSec;
  u_int16_t  blockAlign;
  u_int16_t  bitsPerSample;
  char  data[4];
  u_int32_t datalen;
} wave;

int decode(unsigned char sample)
 {


 int amplitude;
 unsigned char absval,lower;
 unsigned char outb=0, i;

 for(i=0;i<8;i++)
  {
  outb<<=1;
  if(sample&1)outb = outb | 1;
  sample>>=1;
  }
 sample = outb ^ 0x55;
 absval = (sample>>4) & 7;
 lower  = (sample & 0x0F)<<1;
 switch(absval)
  {
  case 0: amplitude =   lower|0x01;     break;
  case 1:     amplitude =   lower|0x21;     break;
  case 2:     amplitude =  (lower|0x21)<<1; break;
  case 3:     amplitude =  (lower|0x21)<<2; break;
  case 4:     amplitude =  (lower|0x21)<<3; break;
  case 5:     amplitude =  (lower|0x21)<<4; break;
  case 6:     amplitude =  (lower|0x21)<<5; break;
  case 7:     amplitude =  (lower|0x21)<<6; break;
  }
 if(sample & 0x80) amplitude = -amplitude;
 return amplitude*16;
 }

int main(int argc,char* arg[])
 {
 unsigned int i,o; 
 unsigned char c;
 signed int x;
 wave w;
 unsigned long count=0;

 strncpy(w.riff,"RIFF", 4);
 strncpy(w.wave,"WAVE", 4);
 strncpy(w.fmt,"fmt ", 4);
 strncpy(w.data,"data", 4);
 w.wFormatTag=1;
 w.wChannels=1;
 w.dwSamplesPerSec=8000;
 w.dwAvgBytesPerSec=16000;
 w.hdrlen=16;
 w.bitsPerSample=16;
 w.blockAlign=2;

 if(argc!=3)
  {
  printf("Usage: isdn2raw  infile outfile\n\n");
  return(1);
  }

 i=open(arg[1],O_RDONLY);
 o=open(arg[2],O_RDWR | O_CREAT | O_TRUNC,0666);

 write(o,&w,sizeof(w));
 while(read(i,&c,1)>0)
  {
  x=decode(c);
  write(o,&x,2);
  count+=2;
  }
 w.datalen=count;	  
 w.filelen=sizeof(w)-8+count;
 lseek(o,0,SEEK_SET);
 write(o,&w,sizeof(w));
 close(i);
 close(o);
 }



