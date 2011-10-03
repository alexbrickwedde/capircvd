#include<capi20.h>
#include<string>
#include<string.h>
#include<sys/ioctl.h>
#include<sys/soundcard.h>
#include<fcntl.h>
#include<fstream>
#include<iostream>
#include<unistd.h>
#include"tools.h"

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

void * PlayNameThread(void *a)
 {
// Testen ob zu der Rufnummer eine Audiodatei existiert

  string s(*((string*)a));
  if(dbg_cmds)
   cdebug << "Playing..." << s << "!" << endl;
  int fd;

  ifstream *i=NULL;

  s=s+"x";

  do
   {
   if(i!=NULL)
    delete i;

   s.erase(s.length()-1,1);

   i=new ifstream((datadir+"/Namen_"+s+".alaw").c_str());

   if(dbg_fileop)
    cdebug << "Trying $DATA/Namen_" << s+".alaw" << " " << " good:" << (int)i->good() << endl;
   
   }
  while( (!i->good()) && s.length()>1);

  if(i->good())
   {
// Namensdatei existiert, auf /dev/audio ausgeben...
   char x;
   int dummy;
   unsigned long int c=0;
   int format = AFMT_S16_LE;
   int stereo = 0;
   int speed = 8192;

   if((fd=open("/dev/audio",O_RDWR | O_NONBLOCK | O_NDELAY,0))==-1)
    return NULL;
   if (ioctl(fd, SNDCTL_DSP_SETFMT, &format)==-1)
    return NULL;
   if (ioctl(fd, SNDCTL_DSP_STEREO, &stereo)==-1)
    return NULL;
   if (ioctl(fd, SNDCTL_DSP_SPEED, &speed)==-1)
    return NULL;

   while(!i->eof())
    {
    i->read(&x,1);
    c=(c+1)%8192;
    dummy=decode(x);
    write(fd,&dummy,2);
    }
   dummy=0;
   for(;c<=8192;c++)
    write(fd,&dummy,2);
   fsync(fd);
   }
  close(fd);
  if(dbg_cmds)
   cdebug << "Sound ausgegeben..." << endl;
  delete i;
  delete (string*)a;
 }                  

string getCalledNumber(_cmsg *CMSG)
 {
 unsigned char *x=CONNECT_IND_CALLEDPARTYNUMBER(CMSG);
 char s[256];
 s[0]=0;
 if(dbg_callcontrol)
  cdebug << "Typ der Callednumber: " << (unsigned int) x[1] << endl; 
 switch(x[1] & 112)
  {
  case 16:
   break;
  case 32:
   strcat(s,iprefix.c_str());
   break;
  case 64:
   strcat(s,iprefix.c_str());
   strcat(s,nprefix.c_str());
   break;
  default:
   break;
  }
 if(x[0]!=0)
  {
  s[strlen(s)+(size_t)x[0]]=0;
  s[strlen(s)+(size_t)x[0]-1]=0;
  memcpy(s+strlen(s),x+2,(size_t)(x[0]-1));
  }
 return string(s);
 }

string getCallingNumber(_cmsg *CMSG)
 {
 unsigned char *x=CONNECT_IND_CALLINGPARTYNUMBER(CMSG);
 char s[256];
 s[0]=0;
 if(dbg_callcontrol)
  cdebug << "Typ der Callingnumber: " << (unsigned int) x[1] << endl; 
 switch(x[1] & 112)
  {
  case 16:
   break;
  case 32:
   strcat(s,iprefix.c_str());
   break;
  case 64:
   strcat(s,iprefix.c_str());
   strcat(s,nprefix.c_str());
   break;
  default:
   break;
  }

 if(x[0]!=0)
  {
  if(x[2]&128)
   {
   s[strlen(s)+x[0]-1]=0;
   s[strlen(s)+x[0]-2]=0;
   memcpy(s+strlen(s),x+3,(size_t)(x[0]-2));
   }
  else
   {
   s[strlen(s)+x[0]]=0;
   s[strlen(s)+x[0]-1]=0;
   memcpy(s+strlen(s),x+2,(size_t)(x[0]-1));
   }
  }
 return string(s);
 }

