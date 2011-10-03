#include<pthread.h>
#include<string>
#include<malloc.h>
#include<capi20.h>
#include<iostream>

#include"capiapp.h"
#include"tools.h"

				// Hilfsfunktion fuer CAPIApp::start
void * runCAPIApp(void *a)
 {
 ((CAPIApp*)a)->Run();
 pthread_exit(NULL);
 }

				// Laesst CAPIApp::Run() als Thread laufen...
pthread_t CAPIApp::start()
 {
 pthread_t Handle;
 pthread_create(&Handle, NULL,(&runCAPIApp), (void *)this);
 return Handle;
 }

				// Default-Konstruktor
CAPIApp::CAPIApp()
  {
  if(dbg_init)
   cdebug << "init	: CAPIApp() aufgerufen..." << endl;
  
  Appl_Id=0;
  CMSG=&CMSG_Instanz;

  if( CAPI20_ISINSTALLED() != CapiNoError )
   {
   cout << "CAPI ist nicht installiert: " << CAPI20_ISINSTALLED() << endl << "CAPI installieren und neu starten :-)" << endl;
   delete this;	
   ::exit(-1);
   }

  CAPI20_GET_PROFILE(0,(unsigned char *)&cprofile);
  if(dbg_systeminfo)
   cdebug << "sinfo	: Anzahl Controller: " << cprofile.ncontroller <<  endl;
 
  int bchans=0;
  for(int ii=1;ii<=cprofile.ncontroller;ii++)
   {
   if(dbg_systeminfo)
    cdebug << endl << "sinfo	:  Controller Nr." << ii << endl;

   capi_profile dummy;
   CAPI20_GET_PROFILE(ii,(unsigned char*)&dummy);
   bchans+=dummy.nbchannel;
   if(dbg_systeminfo)
    {
    cdebug << "sinfo    : Anzahl B-Kanäle : " << dummy.nbchannel << endl;
    char dummy1[64];
    CAPI20_GET_MANUFACTURER(ii,(unsigned char*)&dummy1);
    cdebug << "sinfo    :  Manufacturer    : " << dummy1 << endl;
    capi20_get_serial_number(ii,(unsigned char*)&dummy1);
    cdebug << "sinfo    :   Serial#        : " << dummy1 << endl;
    capi_version dummy2;
    CAPI20_GET_VERSION(ii,(unsigned char *)&dummy2);
    cdebug << "sinfo    :   Version        : " << dummy2.majorversion << "." << dummy2.minorversion << endl;
    cdebug << "sinfo    :   Firmwarever.   : " << dummy2.majormanuversion << "." << dummy2.minormanuversion << endl << endl;
    }
   }

  if(dbg_systeminfo)
   cdebug << "sinfo    : B-Kanäle gesamt: " << bchans << endl << endl;

  CAPI_REGISTER_ERROR err;
  err=CAPI20_REGISTER(bchans,7,2048,&Appl_Id);
  if(err)
   {
   cout << "CAPI registration error:" << err << endl;
   cout << " Vorschlag: Stimmen die Versionen von isdn4k-utils und isdn (Kerneltreiber)" << endl << "ueberein ? Am besten beide Versionen per CVS ziehen..." << err << endl;
   delete this;	
   }

  if(dbg_fileop)
   {
   cdebug << "fileop	: CAPI FileNr: " << capi20_fileno(Appl_Id) << endl;
   cdebug << "fileop	: ApplicationID=" << Appl_Id << endl << endl; 
   }


  map<string,string> *c;

  MESSAGE_EXCHANGE_ERROR error;

  for (unsigned Controller=1; Controller<=cprofile.ncontroller; Controller++)
   {
   c=cConf->getconf(string(""),string("controller")+(char)(Controller+48));
   if(c->find("dontuse")!=c->end() && (*c)["dontuse"]=="true")
     {
     if(dbg_any)
      cdebug << "Controller " << Controller << " not in use !!!" << endl;
     }
   else
    {
    if(dbg_capireq)
     cdebug << "capireq	: LISTEN_REQ fuer Controller " << Controller << endl;
    LISTEN_REQ(CMSG,Appl_Id,0,Controller,0xff,0x1FFF03FF,0,NULL,NULL);
    }
   }

  exit=false; 
  if(dbg_init)
   cdebug << "init	: CAPIApp() beendet..." << endl;
  
  }

					// Destruktor -> CAPI abmelden usw.
CAPIApp::~CAPIApp()
 {
 if(dbg_init)
  cdebug << "init	: ~CAPIApp() aufgerufen..." << endl;

 MESSAGE_EXCHANGE_ERROR ErrorCode;
 ErrorCode = CAPI20_RELEASE(Appl_Id);
 if (ErrorCode != 0)
  cout << "Fehler: ReleaseCAPI error: 0x" << ErrorCode << endl;

 if(dbg_init)
  cdebug << "init	: ~CAPIApp() beendet..." << endl;
 
 }

bool CAPIApp::Run()
  {
  while(!exit)
   {
   pthread_testcancel();			// Thread beenden wenn gewuenscht

   struct timeval tv;
   unsigned int Info;
   tv.tv_sec = 600;
   tv.tv_usec = 0;
   CAPI20_WaitforMessage(Appl_Id, &tv);
   switch(Info = capi_get_cmsg(CMSG, Appl_Id))
    {
    case 0x0000:
     switch (CMSG->Subcommand)
      {
      case CAPI_CONF:
	if(dbg_capiconf)
          cdebug << "capiconf	: Received CAPI_CONF!" << endl;
	
          HandleConf();
          break;
      case CAPI_IND:
	if(dbg_capiind)
          cdebug << "capiind	: Received CAPI_IND!" << endl;
	
          HandleInd();
          break;
      default:
          cdebug << "error	: unknown subcommand " << CMSG->Subcommand << endl;
          break;
      }
      break;
    case 0x1104:
      if(dbg_capireq || dbg_capiresp || dbg_capiconf || dbg_capiind || dbg_systeminfo)
       {
       time_t t;
       time(&t);
       cdebug << "info	: nothing received @" << ctime(&t) << endl;
       if(dbg_systeminfo)
        malloc_stats();
       }
      
     break;
    default:
     cdebug << "error	: ignoring CAPI_GET_CMSG Info == " << Info << endl;
     break;
    }

   } // while !exit
  exit=false;
  return 0;
  }

CAPIConn * CAPIApp::byPLCI(unsigned long int plci)
 {
 CAPIConn *x=C[plci];
 if(x==NULL)
  throw 1;
 return C[plci];
 }

CAPIConn * CAPIApp::byNCCI(unsigned long int ncci)
 {
 return byPLCI(ncci & 0x0000ffff);
 }


void CAPIApp::HandleConf()
 {
 switch(CMSG->Command)
  {
  case CAPI_FACILITY:
   if(dbg_capiconf)
    cdebug << "capiconf	: FAC_CONF " << FACILITY_CONF_INFO(CMSG) << " x " << endl;
//   for(int i=1;i<=(unsigned char)(FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(CMSG)[0]);i++)
//    cdebug << (unsigned short int )FACILITY_CONF_FACILITYCONFIRMATIONPARAMETER(CMSG)[i] << " ";
//   cdebug << endl;
   
//   Conns[((FACILITY_CONF_NCCI(CMSG))&0x0000ffff)]->FacilityConf(CMSG);
   break;

  case CAPI_LISTEN:
   if(dbg_capiconf)
    cdebug << "capiconf	: LISTEN_CONF" << endl;
   
   break;

  case CAPI_ALERT:
   if(dbg_capiconf)
    cdebug << "capiconf	: ALERT_CONF" << endl;
//   Conns[ALERT_CONF_PLCI(CMSG)]->AlertConf(CMSG);
   break;

  case CAPI_DATA_B3:
   if(dbg_capiconf)
    cdebug << "capiconf	: DATA_B3_CONF Conf_Info:" << DATA_B3_CONF_INFO(CMSG) << " Handle:" << DATA_B3_CONF_DATAHANDLE(CMSG) << endl;
   
   ((CAPIConn*)byNCCI(DATA_B3_CONF_NCCI(CMSG)))->DataConf(CMSG);
   break;

 case CAPI_INFO:
   if(dbg_capiconf)
    cdebug << "capiconf	: INFO_CONF Info:" << INFO_CONF_INFO(CMSG) << endl;
   
  break;

  default:
   if(dbg_capiconf)
    cdebug << "capiconf	: Warning! CAPI_CONF Cmd=" << (unsigned int)CMSG->Command << endl;
   break;
  }
 }

void CAPIApp::HandleInd()
 {
 pthread_t dummyh;

 if(dbg_capiind)
  cdebug << "IND MsgNumber:" << CMSG->Messagenumber << endl;
 

 switch(CMSG->Command)
  {
  case CAPI_CONNECT:
   if(dbg_capiind)
    cdebug << "IND CONNECT PLCI=" << CONNECT_IND_PLCI(CMSG) << endl;
   

   if(playaudio)
    pthread_create(&dummyh,NULL,&PlayNameThread,new string(getCallingNumber(CMSG)));

   // Konfiguration lesen
   C[CONNECT_IND_PLCI(CMSG)]=cConf->getCAPIConn(CMSG,Appl_Id,&C);

   if( ((CAPIConn*)byPLCI(CONNECT_IND_PLCI(CMSG))) !=NULL )
    ((CAPIConn*)byPLCI(CONNECT_IND_PLCI(CMSG)))->Connect(CMSG); 
   else
    cdebug << "Unbekannte PLCI..." << endl;
   break;

  case CAPI_CONNECT_ACTIVE:
   if(dbg_capiind)
    cdebug << "IND CONNECT_ACTIVE" << endl;
   
   ((CAPIConn*)byPLCI(CONNECT_ACTIVE_IND_PLCI(CMSG)))->ConnectActive(CMSG);
   break;

  case CAPI_CONNECT_B3:
   if(dbg_capiind)
    cdebug << "IND CONNECT_B3 " << endl;
   
   ((CAPIConn*)byNCCI(CONNECT_B3_IND_NCCI(CMSG)))->ConnectB3(CMSG);
   break;

  case CAPI_CONNECT_B3_ACTIVE:
   if(dbg_capiind)
    cdebug << "IND CONNECT_B3_ACTIVE " << endl;
   
   ((CAPIConn*)byNCCI(CONNECT_B3_ACTIVE_IND_NCCI(CMSG)))->ConnectB3Active(CMSG);
   break;

  case CAPI_DATA_B3:
   if(dbg_capiind)
    cdebug << "IND DATA_B3 " << endl;
   
   ((CAPIConn*)byNCCI(DATA_B3_IND_NCCI(CMSG)))->B3Data(CMSG);
   break;

  case CAPI_FACILITY:
   if(dbg_capiind)
    cdebug << "IND FACILITY " << endl;
   
   ((CAPIConn*)byPLCI(FACILITY_IND_PLCI(CMSG)))->Facility(CMSG);
   break;

  case CAPI_INFO:
   if(dbg_capiind)
    cdebug << "IND INFO" << endl;
   
   try{
    ((CAPIConn*)byPLCI(INFO_IND_PLCI(CMSG)))->InfoInd(CMSG);
    }
   catch(int i) {
    if(i==1)
     cdebug << "Info_Ind unbekannte PLCI=" << INFO_IND_PLCI(CMSG) << endl; 
    } 
   break;

  case CAPI_DISCONNECT:
    if(dbg_capiind)
     cdebug << "IND DISCONNECT reason=" << DISCONNECT_IND_REASON(CMSG) << endl;
   
    byPLCI(DISCONNECT_IND_PLCI(CMSG))->Disconnect(CMSG); 
    delete byPLCI(DISCONNECT_IND_PLCI(CMSG));
   break;

  case CAPI_DISCONNECT_B3:
   if(dbg_capiind)
    cdebug << "IND DISCONNECT_B3 reason=" << DISCONNECT_B3_IND_REASON_B3(CMSG) << endl;
   
   ((CAPIConn*)byNCCI(DISCONNECT_B3_IND_NCCI(CMSG)))->DisconnectB3(CMSG); 
   break;

  default:
   cdebug << "Unhandled Ind. Cmd=" << (unsigned int) CMSG->Command << endl;
  }
 }

