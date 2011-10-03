#include<capi20.h>
#include<iostream>
#include<string>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<malloc.h>
#include"tools.h"
#include"ccconf.h"

void * WaitConnectThread(void *a)
 {
 int dummy;
 pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&dummy);
 pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&dummy);

 if(dbg_callcontrol)
  cdebug << "Sleeping " << ((CAPIConn*)a)->WaitConnectTime << " usec..." << endl;

 usleep( ((CAPIConn*)a)->WaitConnectTime ); 

 if(dbg_callcontrol)
  cdebug << "WaitConnect: Sleep beendet..." << endl;
 

 pthread_testcancel();

 if(dbg_callcontrol)
  cdebug << "WaitConnect: Nicht gecancelt, also weiter..." << endl;
 

 ((CAPIConn*)a)->ConnectIt();

 if(dbg_callcontrol)
  cdebug << "WaitConnect: Connect ausgefuehrt!" << endl;
 

 pthread_exit(NULL); 
 }

CAPIConn::CAPIConn(unsigned applid,map<string,string> *c)
 {
 C=c;
 Appl_Id=applid;
 WaitingforConnect=false;
 PinOK=false;
 ExecHandler=false;
 RunningCmd=0;
 WaitConnectTime=10000000;
 B1=0;
 B2=0;
 B3=0;
 dtmf="";
 if(dbg_init)
  cdebug << "CAPIConn Init"<<endl;
 buf=NULL;
 receivelater=false;
 }

CAPIConn::~CAPIConn()
 {
 if(WaitingforConnect)
  {
  if(dbg_callcontrol)
   cdebug << "WaitforConnectThread canceln..." << endl;
  
  pthread_cancel(WaitConnect);
  }
 }

void CAPIConn::ChangeFiles(string i,string o)
 {
 close(i_fd);
 close(o_fd);
 ofile=o;
 ifile=i;
 sending=(i!="");
 receiving=(o!="");
 OpenDatastream();
 }

void CAPIConn::InfoInd(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "InfoInd erhalten... INFO_NUMBER=" << INFO_IND_INFONUMBER(CMSG) << endl;
 
 _cmsg CMSG1;
 INFO_RESP(&CMSG1,Appl_Id,CMSG->Messagenumber,INFO_IND_PLCI(CMSG));
 }

void CAPIConn::Connect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Connect_Ind" << endl << "Nothing to do -> Ignoring Call..." << endl;
 
 PLCI=CONNECT_IND_PLCI(CMSG);
 MsgNr=CMSG->Messagenumber;
 Reject=1;
 ConnectIt();
 }

void CAPIConn::ConnectActive(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "ConnectActive..." << PLCI << endl;
 
 _cmsg CMSG1;
 CONNECT_ACTIVE_RESP(&CMSG1, Appl_Id, CMSG->Messagenumber, PLCI);
 }

void CAPIConn::ConnectB3(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "ConnectB3..." << endl;
 
 NCCI=CONNECT_B3_IND_NCCI(CMSG);
 CONNECT_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber, NCCI,0,(_cstruct)NULL);
 }

void CAPIConn::ConnectB3Active(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "ConnectB3Active...(CAPIConn)" << endl;
 
 CONNECT_B3_ACTIVE_RESP(CMSG, Appl_Id, CMSG->Messagenumber, NCCI);
 OpenDatastream();
 }

void CAPIConn::OpenDatastream()
 {
 if(dbg_fileop) 
  cdebug << "ERROR: this connection has no fileoperation..." << endl;
 _cmsg CMSG1;
 DISCONNECT_REQ(&CMSG1,Appl_Id,0,PLCI,NULL,NULL,NULL,NULL);
 if(ExitonError)
	exit(1);
 }

void CAPIConn::B3Data(_cmsg *CMSG)
 {
 if(dbg_capiind) 
  cdebug << "Received Data..." << endl;
 
 DATA_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber, NCCI, DATA_B3_IND_DATAHANDLE(CMSG));
 }

void CAPIConn::DataConf(_cmsg *CMSG)
 {
 if(dbg_capiconf)
  cdebug << "DataConf()" << endl; 
 
 }

void CAPIConn::setWaitforConnect()
 {
 WaitingforConnect=true;
 pthread_create(&WaitConnect,NULL,&WaitConnectThread,this);
 }

void CAPIConn::Facility(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Facility Indication..." << endl;

 switch(FACILITY_IND_FACILITYSELECTOR(CMSG))
  {
  case 1:
   {
   if(dbg_capiind)
    cdebug << " ... DTMF-Tone received :";
   for(int i=1;i<=(unsigned char)(FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG)[0]);i++)
    dtmf+=(unsigned char)(FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG)[i]);
   if(dbg_capiind)
    cdebug << " :" << dtmf << endl;

   int dummy; 
   if((dummy=dtmf.find(string("#")))>=0 && dummy<dtmf.length())
    {
    if(!PinOK)
     {
     if(C->find("pin")!=C->end() && ((*C)["pin"]+"#")==dtmf)
      { 
      PinOK=true;
      ExecHandler=false;
      ChangeFiles(datadir+"/ctrl_pinok.alaw","");
      if(dbg_callcontrol)
       cdebug << "Pin entered is OK!" << endl;
      }
     else
      {
      ChangeFiles(datadir+"/ctrl_pinwrong.alaw","");
      if(dbg_callcontrol)
       cdebug << "Pin entered is wrong!" << endl;
      } 
     }
    else
     {
     if(dbg_callcontrol)
      cdebug << "RunningCmd:" << RunningCmd << endl;
     switch(RunningCmd)
      {
      case 11:
        if(dbg_callcontrol)
         cdebug << "recording of announcement stopped!" << endl;
        ChangeFiles("","");
        RunningCmd=0;
       break;
      default:
        {
        string x[]={"11","12"};
        int ii;
        for(ii=0;ii<3 && dtmf.substr(0,2)!=x[ii];ii++);
        switch(ii)
         {
         case 0:
           if(dbg_callcontrol)
            cdebug << "Command 11#: Record announcement..." << endl;
           ChangeFiles("",datadir+"/"+(*C)["announcement"]);
           RunningCmd=11; 
          break;
         case 1:
           if(dbg_callcontrol)
            cdebug << "Command 11#: Play announcement..." << endl;
           ChangeFiles(datadir+"/"+(*C)["announcement"],"");
           RunningCmd=0; 
          break;
         default:
           if(dbg_callcontrol)
            cdebug << "DTMF-Command unknown; " << dtmf << endl;
         }
        }
      break;
      }
     }
     dtmf=""; 
    }
   break;
  default:
   if(dbg_capiind)
    cdebug << " facilityselector:" << FACILITY_IND_FACILITYSELECTOR(CMSG) << endl;
  }
 }
 
 _cmsg CMSG1;
 FACILITY_RESP(&CMSG1,Appl_Id,0,PLCI,FACILITY_IND_FACILITYSELECTOR(CMSG),FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG));
 }

void CAPIConn::Disconnect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Call disconnected..." << endl;
 
 if(buf!=NULL)
  free(buf);
 if(WaitingforConnect)
  {
  if(dbg_callcontrol)
   cdebug << "WaitforConnectThread canceln..." << endl;
  
  pthread_cancel(WaitConnect);
  WaitingforConnect=false;
  }
 _cmsg CMSG1;
 DISCONNECT_RESP(&CMSG1,Appl_Id,0,PLCI);
 }

void CAPIConn::DisconnectB3(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "BChannel disconnected..." << endl;
 
 _cmsg CMSG1;
 DISCONNECT_B3_RESP(&CMSG1,Appl_Id,0,NCCI);
 }

void CAPIConn::ConnectIt()
 {
 if(dbg_callcontrol || dbg_capiresp)
  cdebug << "Klingeln beenden, Ruf annehmen..." << endl;
 
 WaitingforConnect=false;
 _cmsg CMSG1;
 CONNECT_RESP(&CMSG1,Appl_Id,MsgNr,PLCI,Reject,B1,B2,B3,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

 }

CAPIConnIgnore::CAPIConnIgnore(unsigned applid):CAPIConn(applid,NULL)
 {
 if(dbg_init)
  cdebug << "ConnIgnore Init" << endl; 
 
 }

void CAPIConnIgnore::Connect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Ignoring Call..." << endl;
 
 PLCI=CONNECT_IND_PLCI(CMSG);
 Reject=1;
 MsgNr=CMSG->Messagenumber;
 CAPIConn::ConnectIt();
 }

CAPIConnDeflect::CAPIConnDeflect(unsigned applid,map<string,string> *c):CAPIConn(applid,c)
 {
 if(dbg_init)
  cdebug << "ConnDeflect Init" << endl;
 
 WaitConnectTime=atoi((*c)["delay"].c_str())*1000000;
 RedirectTo=(*c)["destination"];
 }

void CAPIConnDeflect::Connect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Answering DeflectCall..." << endl;
 
 PLCI=CONNECT_IND_PLCI(CMSG);
 _cmsg CMSG1;
 ALERT_REQ (&CMSG1, Appl_Id, 0, PLCI, NULL, NULL, NULL, NULL,NULL);
 MsgNr=CMSG->Messagenumber;
 setWaitforConnect();
 }

void CAPIConnDeflect::ConnectIt()
 {
 if(dbg_capiresp)
  cdebug << "Klingeln beenden, Ruf umleiten..." << RedirectTo << endl;
 
 _cmsg CMSG1;
 _cbyte fac[60];
 fac[0]=0; // len
 fac[1]=0; //len
 fac[2]=0x01; // Use D-Chan
 fac[3]=0; // Keypad len
 fac[4]=31; 
 fac[5]=0x1c; 
 fac[6]=0x1d; 
 fac[7]=0x91;
 fac[8]=0xA1;
 fac[9]=0x1A;
 fac[10]=0x02;
 fac[11]=0x01;
 fac[12]=0x70;
 fac[13]=0x02;
 fac[14]=0x01;
 fac[15]=0x0d;
 fac[16]=0x30;
 fac[17]=0x12;
 fac[18]=0x30;
 fac[19]=0x0d;
 fac[20]=0x80;
 fac[21]=0x0b;
 fac[22]=0x30;
 fac[23]=0x31;
 fac[24]=0x37;
 fac[25]=0x32;
 fac[26]=0x39;
 fac[27]=0x37;
 fac[28]=0x37;
 fac[29]=0x36;
 fac[30]=0x36;
 fac[31]=0x36;
 fac[32]=0x37;
 fac[33]=0x01;
 fac[34]=0x01;
 fac[35]=0x01;

 int dummy1=RedirectTo.length();
 memcpy((unsigned char *)fac+22,RedirectTo.c_str(),dummy1);
 fac[22+dummy1]=0x01;
 fac[23+dummy1]=0x01;
 fac[24+dummy1]=0x01;
 fac[25+dummy1]=0x01;
 fac[26+dummy1]=0x01;

 fac[6]=18+dummy1;
 fac[9]=15+dummy1;
 fac[17]=7+dummy1;
 fac[19]=2+dummy1;
 fac[21]=dummy1;

 INFO_REQ(&CMSG1,Appl_Id,0,PLCI,(unsigned char *)fac,(unsigned char *)fac+2,(unsigned char *)fac,(unsigned char *)fac,(unsigned char *)fac+4,NULL);

// int dummy2=RedirectTo.length();
// fac[0]=7+dummy2; // len
// fac[1]=0xd; 
// fac[2]=0;
// fac[3]=4+dummy2; // len
// fac[4]=1;
// fac[5]=0;
// fac[6]=dummy2; // len
// memcpy((unsigned char *)fac+7,RedirectTo.c_str(),dummy2);
// fac[7+dummy2]=0;
// FACILITY_REQ(&CMSG1,Appl_Id,0,PLCI,0x003,(unsigned char *)fac);

 if(dbg_callcontrol)
  cdebug << "Deflection requested!" << endl;
 

 WaitingforConnect=false;
 }

CAPIConnReject::CAPIConnReject(unsigned applid,map<string,string> *c):CAPIConn(applid,c)
 {
 if(dbg_init)
  cdebug << "ConnReject Init" << endl;
 
 WaitConnectTime=atoi((*c)["delay"].c_str())*1000000;
 }

void CAPIConnReject::Connect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Answering RejectCall... delay=" << WaitConnectTime << endl;
 
 PLCI=CONNECT_IND_PLCI(CMSG);
 ALERT_REQ (CMSG, Appl_Id, 0, CONNECT_IND_PLCI(CMSG), NULL, NULL, NULL, NULL,NULL);
 MsgNr=CMSG->Messagenumber;
 setWaitforConnect();
 }

void CAPIConnReject::ConnectIt()
 {
 if(dbg_capiresp || dbg_callcontrol)
  cdebug << "Klingeln beenden, Ruf abweisen..." << endl;
 
 WaitingforConnect=false;
 Reject=3;
 CAPIConn::ConnectIt();
 }

CAPIConnDDI::CAPIConnDDI(unsigned applid,string n,string caller,map<string,string> *c, map<unsigned long int,CAPIConn*> *cc):CAPIConn(applid,c)
 {
 if(dbg_init)
  cdebug << "ConnDDI Init" << endl;
 
 Conns=cc;
 anzahlnr=atoi((*c)["numlength"].c_str());
 nr=n;
 ca=caller;
 }

void CAPIConnDDI::Connect(_cmsg *CMSG)
 {
 ConnInd=CMSG;

 if(dbg_capiind)
  cdebug << "Answering DDICall..." << endl;

 if(nr.length()>=anzahlnr)
  ConnectIt();
 }

void CAPIConnDDI::ConnectIt()
 {
 cdebug << " Neues Objekt erzeugen...--------------- " << endl;
 cdebug << " initConn " << endl;

 CAPIConn *dummy=cConf->getCAPIConn(ca,nr,0,Appl_Id,Conns);

 cdebug << " Jetzt muss Objekt ersetzt werden! " << endl;

 (*Conns)[CONNECT_IND_PLCI(ConnInd)]=dummy;

 dummy->Connect(ConnInd);

 cdebug << " Objekt ersetzt! " << endl;

 delete this;
 return;
 }


void CAPIConnDDI::InfoInd(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "(DDIConn) InfoInd erhalten..." << endl;
 
 _cmsg CMSG1;
 if(INFO_IND_PLCI(CMSG)>255)
  INFO_RESP(&CMSG1,Appl_Id,CMSG->Messagenumber,INFO_IND_PLCI(CMSG));
 if(INFO_IND_INFONUMBER(CMSG)==0x0070)
  {
  if(dbg_callcontrol)
   cdebug << "   DDI: Ziffer erhalten..." << endl;
  
  unsigned char *x=INFO_IND_INFOELEMENT(CMSG);
  char s[128];
  memcpy(s,x+2,(size_t)(x[0]-1));
  s[x[0]-1]='\0';
  s[x[0]]='\0';
  nr+=s;
  if(dbg_callcontrol)
   cdebug << "   DDI: Rufnummer ist jetzt:" << nr << endl;

  if(nr.length()>=anzahlnr)
   ConnectIt();
  }
 }

CAPIConnDatastream::CAPIConnDatastream(unsigned applid,string ii,string oo,map<string,string> *c):CAPIConn(applid,c)
 {
 mailto=(*c)["recipient"];
 called=ii;
 calling=oo;
 buf=(unsigned char *)malloc((size_t)8*2048);
 receiving=true;
 sending=true;
 Hdl=0;
 ocmd=(*c)["handler"];
 ifile=datadir+"/"+(*c)["announcement"];
 
 time_t t;
 tm *t_local;
 time(&t);
 t_local=localtime(&t);
 
 timeval tv;
 gettimeofday(&tv,NULL);

 char dummy[1024];

 if( (*c).find("filename")!=(*c).end() )
  sprintf(dummy,(*c)["filename"].c_str(),ii.c_str(),oo.c_str(),atol((*c)["usagecount"].c_str()),tv.tv_sec,tv.tv_usec,t_local->tm_year+1900,t_local->tm_mon+1,t_local->tm_mday,t_local->tm_hour,t_local->tm_min,t_local->tm_sec);
 else
  sprintf(dummy,"%s-%06d-%s--%d-%d.alaw",ii.c_str(),atol((*c)["usagecount"].c_str()),oo.c_str(),tv.tv_sec,tv.tv_usec);

 ofile=datadir+"/"+dummy;
 }

void CAPIConnDatastream::DataConf(_cmsg *CMSG)
 {
 if(dbg_capiconf)
  cdebug << "DataConf(), next one..." << endl; 
 
 Send();
 }

void CAPIConnDatastream::ConnectB3Active(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "ConnectB3Active...(CAPIConnDatastream)" << endl;

 CONNECT_B3_ACTIVE_RESP(CMSG, Appl_Id, CMSG->Messagenumber, NCCI);
 OpenDatastream();
 }

 void CAPIConnDatastream::Send()
 {
// Senden wenn Daten vorhanden sind...
// Hdl%8 bewirkt, dass 8 verschiedene Puffer zum Senden verwendet werden.
// Somit koennen maximal 8 Bloecke gesendet werden ohne ein DATA_CONF
// erhalten zu haben...

 _cmsg CMSG1;
 unsigned int len=2048;


 if( sending )
  {
  if(dbg_datatransfer)
   cdebug << "Sende Datenpaket: " << Hdl << endl;

  len=read(i_fd,buf+((Hdl%8)*2048),2048);

  if(len==0)
   {
   cdebug << "Ansage beendet!" << endl;
   sending=false;
   if(!receiving)
    if(receivelater)
     {
     cdebug << "Announcement ended, starting to record" << endl; 
     receiving=true;
     }
    else
     {
     cdebug << "No recording -> drop call" << endl; 
     DISCONNECT_REQ(&CMSG1,Appl_Id,0,PLCI,NULL,NULL,NULL,NULL);
     }
   return;
   }

  DATA_B3_REQ(&CMSG1,Appl_Id,0,NCCI,buf+((Hdl%8)*2048),len,Hdl,0);
  Hdl++;
  }
 }

void CAPIConnDatastream::OpenDatastream()
 {
 cdebug << "Open ifile " << ifile.c_str() << "!" << endl; 
 i_fd=open(ifile.c_str(),O_RDONLY);
 cdebug << "I-Dateihandle " << i_fd << endl;
 if(i_fd<0)
  {
  if(dbg_fileop)
   cdebug << "Ansage nicht vorhanden!!! errno=" << errno << endl;
  
  sending=false;
  }

 if(dbg_fileop)
  cdebug << "Open ofile " << ofile.c_str() << "!" << endl; 

 o_fd=open(ofile.c_str(),O_WRONLY | O_CREAT | O_TRUNC,0666);
 if(dbg_fileop)
  cdebug << "O-Dateihandle " << o_fd << endl;
 if(o_fd<0)
  {
  receiving=false;
  if(dbg_fileop)
   cdebug << "Not receiving !!!!!! errno=" << errno << endl;
  }

 if( (*C).find("record")!=(*C).end() && (*C)["record"]=="off")
  {
  receiving=false;
  }

 if( (*C).find("record")!=(*C).end() && (*C)["record"]=="later" && receiving)
  {
  receiving=false;
  receivelater=true;
  }

 if(sending)
  {
  Send();
  Send();
  Send();
  Send();
  Send();
  Send();
  }
 }

void CAPIConnDatastream::B3Data(_cmsg *CMSG)
 {
 if(dbg_capiind || dbg_datatransfer ) 
  cdebug << "Received Data... Handle=" << DATA_B3_IND_DATAHANDLE(CMSG) << " Length=" << DATA_B3_IND_DATALENGTH(CMSG) << endl;
 
 if(receiving)
  {
  if(dbg_fileop)
   cdebug << "Writing " << DATA_B3_IND_DATALENGTH(CMSG) << " Bytes to file..." << endl;
  write(o_fd,(unsigned char *)DATA_B3_IND_DATA(CMSG),DATA_B3_IND_DATALENGTH(CMSG));
  }

 DATA_B3_RESP(CMSG, Appl_Id, CMSG->Messagenumber, NCCI, DATA_B3_IND_DATAHANDLE(CMSG));
 }

void CAPIConnDatastream::DisconnectB3(_cmsg *CMSG)
 {
 CAPIConn::DisconnectB3(CMSG);
 if(dbg_fileop)
  cdebug << "Closing file!" << endl;
 close(o_fd);
 }

CAPIConnVoice::CAPIConnVoice(unsigned applid,string ii,string oo,map<string,string> *c):CAPIConnDatastream(applid,ii,oo,c)
 {
 if(dbg_init)
  cdebug << "ConnVoice Init" << endl;
 B1=1;
 B2=1;
 B3=0;
 Reject=0;
 receiving=true;
 WaitConnectTime=atoi((*c)["delay"].c_str())*1000000;
 if(dbg_callcontrol)
  cdebug << "WaitConnectTime: " << (*c)["delay"] << endl;
 
 }

void CAPIConnVoice::Connect(_cmsg *CMSG)
 {
 if(dbg_callcontrol || dbg_capiresp)
  cdebug << "Answering VoiceCall..." << endl;
 PLCI=CONNECT_IND_PLCI(CMSG);
 _cmsg CMSG1;
 ALERT_REQ (&CMSG1, Appl_Id, CMSG->Messagenumber, PLCI, NULL, NULL, NULL, NULL,NULL);
 MsgNr=CMSG->Messagenumber;
 setWaitforConnect();
 }

void CAPIConnVoice::ConnectB3(_cmsg *CMSG)
 {
 CAPIConn::ConnectB3(CMSG);
 ExecHandler=true;
 // DTMF-erkennung einschalten!
 _cmsg CMSG1;
 _cbyte fac[8];
 fac[0]=0x07;
 fac[1]=0x01;
 fac[2]=0x00;
 fac[3]=0x30;
 fac[4]=0x00;
 fac[5]=0x30;
 fac[6]=0x00;
 fac[7]=0x00;
 FACILITY_REQ(&CMSG1,Appl_Id,0,NCCI,1,(unsigned char *)fac);
 }

void * ExternalHandler(void *a)
 {
 string s=(*((string*)a));
 if(dbg_cmds)
  cdebug << "System(" << ((string*)a)->c_str() << ")" << endl;
 system(((string*)a)->c_str());
 delete (string*)a;
 }

void CAPIConnVoice::Disconnect(_cmsg *CMSG)
 {
 CAPIConn::Disconnect(CMSG);
 if(receiving && ExecHandler)
  {
  if(dbg_cmds)
   cdebug << "Calling Voice-Handler: " << ocmd << endl;
  string *x=new string(ocmd.c_str());
  string mailformat("");
  if(C->find("mailformat")!=C->end())
   mailformat=(*C)["mailformat"];
  (*x)+=" voice \"" + called + "\" \"" + calling + "\" \"" + ofile.c_str() + "\" \"" + mailto.c_str() +"\" \""+mailformat+"\"";
  pthread_t p;
  pthread_create(&p,NULL,&ExternalHandler,x);
  }
 }

CAPIConnFax::CAPIConnFax(unsigned applid,string ii,string oo,map<string,string> *c) :CAPIConnDatastream(applid,ii,oo,c)
 {
 if(dbg_init)
  cdebug << "ConnFax Init" << endl;
 
 ExecHandler=true;
 receiving=true;
 WaitConnectTime=atoi((*c)["delay"].c_str()) * 1000000;
 Reject=0;
 B1=4;
 B2=4;
 B3=4;
 sending=false;

 time_t t;
 tm *t_local;
 time(&t);
 t_local=localtime(&t);

 timeval tv;
 gettimeofday(&tv,NULL);

 char dummy[1024];

 if( (*c).find("filename")!=(*c).end() )
  sprintf(dummy,(*c)["filename"].c_str(),ii.c_str(),oo.c_str(),atol((*c)["usagecount"].c_str()),tv.tv_sec,tv.tv_usec,t_local->tm_year+1900,t_local->tm_mon+1,t_local->tm_mday,t_local->tm_hour,t_local->tm_min,t_local->tm_sec);
 else
  sprintf(dummy,"%s-%06d-%s--%d-%d.sff",ii.c_str(),atol((*c)["usagecount"].c_str()),oo.c_str(),tv.tv_sec,tv.tv_usec);


 ofile=datadir+"/"+dummy;
 }

void CAPIConnFax::Connect(_cmsg *CMSG)
 {
 if(dbg_capireq || dbg_callcontrol)
  cdebug << "Answering FaxCall..." << endl;
 
 PLCI=CONNECT_IND_PLCI(CMSG);
 ALERT_REQ (CMSG, Appl_Id, 0, CONNECT_IND_PLCI(CMSG), NULL, NULL, NULL, NULL,NULL);
 MsgNr=CMSG->Messagenumber;
 setWaitforConnect();
 }

void CAPIConnFax::ConnectIt()
 {
 if(dbg_callcontrol || dbg_capiresp)
  cdebug << "Klingeln beenden, Ruf annehmen..." << endl;

 WaitingforConnect=false;
 _cmsg CMSG1;

 string faxid;
 if(C->find("faxid")!=C->end())
  {
  faxid=(*C)["faxid"];
  long a;
  while((a=faxid.find('_'))>=0)
   faxid.replace(a,1," ");
  }
 else
  faxid=string("+")+called;

 unsigned char buf[256];
 buf[0]=faxid.length()+5;
 buf[1]=0;
 buf[2]=0;
 buf[3]=0;
 buf[4]=0;
 buf[5]=faxid.length()+1;
 memcpy(buf+6,faxid.c_str(),faxid.length());
 buf[6+faxid.length()]=0;
 
 CONNECT_RESP(&CMSG1,Appl_Id,MsgNr,PLCI,Reject,B1,B2,B3,NULL,NULL,buf,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
 }

void CAPIConnFax::DisconnectB3(_cmsg *CMSG)
 {
 remoteid=string("");
 CAPIConn::DisconnectB3(CMSG);
 for(int i=0;i<DISCONNECT_B3_IND_NCPI(CMSG)[9];i++)
  remoteid = remoteid + (char)(DISCONNECT_B3_IND_NCPI(CMSG)[10+i]); 
 if(dbg_callcontrol)
  cdebug << "RemoteID war: " << remoteid << endl;
 }

void CAPIConnFax::Disconnect(_cmsg *CMSG)
 {
 CAPIConn::Disconnect(CMSG);
 if(ExecHandler)
  {
  if(dbg_cmds)
   cdebug << "Calling Fax-Handler: " << ocmd << endl;
  string *x=new string(ocmd.c_str());
  string mailformat("");
  if(C->find("mailformat")!=C->end())
   mailformat=(*C)["mailformat"];
  (*x)+=" fax \"" + called + "\" \"" + calling + "\" \"" + ofile + "\" \"" + mailto + "\" \"" + remoteid + "\" \""+mailformat+"\"";
  pthread_t p;
  pthread_create(&p,NULL,&ExternalHandler,x);
  }
 }

CAPIConnPing::CAPIConnPing(unsigned applid,string ii,string oo,map<string,string> *c):CAPIConnVoice(applid,ii,oo,c)
 {
 if(dbg_init)
  cdebug << "ConnPing Init" << endl;

 B1=1;
 B2=1;
 B3=0;
 Reject=0;
 receiving=true;
 receivelater=true;
 ifile=datadir+"/ping_ansage.alaw";
 ofile="/dev/null";
 WaitConnectTime=atoi((*c)["delay"].c_str())*1000000;
 if(dbg_callcontrol)
  cdebug << "WaitConnectTime: " << (*c)["delay"] << endl;

 }

void CAPIConnPing::Facility(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Facility Indication..." << endl;

 _cmsg CMSG1;
 FACILITY_RESP(&CMSG1,Appl_Id,0,PLCI,FACILITY_IND_FACILITYSELECTOR(CMSG),FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG));

 switch(FACILITY_IND_FACILITYSELECTOR(CMSG))
  {
  case 1:
   if(dbg_capiind || dbg_callcontrol)
    cdebug << " ... DTMF-Tone received :";
   for(int i=1;i<=(unsigned char)(FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG)[0]);i++)
    dtmf+=(unsigned char)(FACILITY_IND_FACILITYINDICATIONPARAMETER(CMSG)[i]);
   if(dbg_capiind || dbg_callcontrol)
    cdebug << " :" << dtmf << endl;
   if(dtmf.find(string("#"))>=0 && dtmf.find(string("#")) < dtmf.length())
    {
    string dummy=dtmf.substr(0,dtmf.find(string("#")));
    int pos=0;
    while( (pos=dummy.find(string("*")))>=0 && pos < dummy.length() )
     dummy=dummy.substr(0,pos)+"."+dummy.substr(pos+1);

    string p=string("/usr/sbin/fping ") + dummy;

    if(dbg_callcontrol)
     cdebug << "process: " << p << endl;

    int result=system(p.c_str());
    if(result!=0)
     {
     if(dbg_callcontrol)
      cdebug << " Result!=0 " << result << endl;
     ChangeFiles(datadir+"/ping_noreply.alaw","");
     }
    else 
     {
     if(dbg_callcontrol)
      cdebug << " Result==0" << endl;
     ChangeFiles(datadir+"/ping_reply.alaw","");
     }
    dtmf="";
    }
   break;
 default:
   if(dbg_capiind)
    cdebug << " facilityselector:" << FACILITY_IND_FACILITYSELECTOR(CMSG) << endl;
  }

 }

CAPIConnFaxpoll::CAPIConnFaxpoll(unsigned applid,string ii,string oo,map<string,string> *c):CAPIConnDatastream(applid,ii,oo,c)
 {
 if(dbg_init)
  cdebug << "ConnFaxpoll Init" << endl;

// receiving=true;
 WaitConnectTime=atoi((*c)["delay"].c_str()) * 1000000;
 B1=0;
 B2=0;
 B3=0;
 }

void CAPIConnFaxpoll::Connect(_cmsg *CMSG)
 {
 if(dbg_capiind)
  cdebug << "Answering FaxpollCall..." << endl;

 PLCI=CONNECT_IND_PLCI(CMSG);
 ALERT_REQ (CMSG, Appl_Id, 0, CONNECT_IND_PLCI(CMSG), NULL, NULL, NULL, NULL,NULL);
 MsgNr=CMSG->Messagenumber;
 setWaitforConnect();
 }

void CAPIConnFaxpoll::ConnectIt()
 {
 if(dbg_capiresp || dbg_callcontrol)
  cdebug << "Klingeln beenden, Ruf annehmen..." << endl;

 WaitingforConnect=false;
 _cmsg CMSG1;

 CONNECT_RESP(&CMSG1,Appl_Id,MsgNr,PLCI,Reject,B1,B2,B3,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
 }

void CAPIConnFaxpoll::ConnectActive(_cmsg *CMSG)
 {
 CAPIConn::ConnectActive(CMSG);

 unsigned char buf[256];
 buf[0]=called.length()+6;
 buf[1]=0;
 buf[2]=0;
 buf[3]=0;
 buf[4]=0;
 buf[5]=called.length();
 memcpy(buf+6,called.data(),called.length());
 buf[6+called.length()]=0;
 _cmsg CMSG2;

 SELECT_B_PROTOCOL_REQ(&CMSG2,Appl_Id,0,PLCI,4,4,5,NULL,NULL,buf,NULL);

 }

void CAPIConnFaxpoll::DisconnectB3(_cmsg *CMSG)
 {
 remoteid=string("");
 CAPIConn::DisconnectB3(CMSG);
 for(int i=0;i<DISCONNECT_B3_IND_NCPI(CMSG)[9];i++)
  remoteid = remoteid + (char)(DISCONNECT_B3_IND_NCPI(CMSG)[10+i]);
 if(dbg_callcontrol)
  cdebug << "RemoteID war: " << remoteid << endl;
 }

void CAPIConnFaxpoll::Disconnect(_cmsg *CMSG)
 {
 CAPIConn::Disconnect(CMSG);
 if(dbg_cmds)
  cdebug << "Calling Fax-Handler: " << ocmd << endl;
  string x(ocmd);
 x+=" " + called + " " + calling + " " + ofile + " " + remoteid;
 pthread_t p;
 pthread_create(&p,NULL,&ExternalHandler,&x);
 }


