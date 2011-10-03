#include<sstream>
#include<iostream>
#include<string>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<map>
#include<iterator>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<iomanip>
#include<malloc.h>

#include"ccconf.h"
#include"tools.h"

capircvdConf *cConf;
bool ExitonError;

void * exechandler(void * x)
 {
 if(dbg_cmds)
  cdebug << "Executing: " << ((string *)x)->c_str() << endl;
 system(((string *)x)->c_str());
// execlp("/bin/sh","/bin/sh","-c",((string *)x)->c_str(),NULL,NULL);
 delete (string *)x;
 }

capircvdConf::capircvdConf(string file)
 {
 char buf[2048];
 ifstream i(file.c_str());
 if(i.bad() || i.eof())
  {
  throw((unsigned int)1);
  }
 string section,option,value;

 while(!i.bad() && !i.eof())
  {

  i.getline(buf,2048);

  if(strchr(buf,'#')!=NULL)
   {
   strchr(buf,'#')[0]='\0';
   }

  istringstream s(buf);

  if(buf[0]=='=')
   {
   section=string("") + ((char *)buf+1);
   conf[section]=conf["default"];
   }
  else
   {
   option="#";
   s >> option >> value; 
   if(option!="#")
    (conf[section])[option]=value;
   }
  }

/* if(conf.find("admin")!=conf.end())
  if(conf["admin"].find("telnet")!=conf["admin"].end())
   if(conf["admin"]["telnet"]=="true")
    {
    addrlen=sizeof(sockaddr_in);
    saddr.sin_family= AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(conf["admin"].find("telnetport")!=conf["admin"].end())
     saddr.sin_port = htons(atoi(conf["admin"]["telnetport"].c_str()));
    else
     saddr.sin_port = htons(20013);
   
    sock=socket(AF_INET,SOCK_STREAM,0);

    if(0==bind(sock,(struct sockaddr*)&saddr,sizeof(saddr)))
     {
     listen(sock,10);
     pthread_create(&acceptthread,NULL,acceptthread_func,&sock);
     }
    else
     {
     cdebug << "socket error! closing socket, no telnet sessions !" << endl;
     shutdown(sock,2);
     throw(1);
     }
    }  */
 }

map<string,string>* capircvdConf::getconf(string calling,string called)
 {
 string dummy;
 map<string,string> *c;

 if(dbg_callcontrol || dbg_config)
  cdebug << "configuration for - " << calling << " to " << called << " -" << endl;

 c=&(conf[called]); 

 conf[called]["used"]="true";

 dummy="+";
 while(dummy.length()<=calling.length())
  {
  dummy=dummy+calling.c_str()[dummy.length()-1];
  if( conf.find(called+dummy) != conf.end() )
   {
   if(dbg_config)
    cdebug << "Found!" << called << dummy << endl;
   map<string,string>::iterator i=conf[called+dummy].begin();
   for(;i!=conf[called+dummy].end();i++)
    (*c)[(*i).first]=(*i).second;
   }
  }
 return c;
 }

bool capircvdConf::dumpconf(string file)
 {
 ofstream o(file.c_str());
 map<string,map<string,string> >::iterator ii=conf.begin();
 for(;ii!=conf.end();ii++)
  {
  o << "=" << (*ii).first << endl;
  map<string,string>::iterator i=(*ii).second.begin();
  for(;i!=(*ii).second.end();i++)
   {
   o << (*i).first << "		" << (*i).second << endl;
   }
  }
 o.close();
 return true;
 }

CAPIConn * capircvdConf::getCAPIConn(_cmsg *CMSG,unsigned Appl_Id, map<unsigned long int,CAPIConn*> *c)
 {
 return getCAPIConn(getCallingNumber(CMSG),getCalledNumber(CMSG),CONNECT_IND_PLCI(CMSG),Appl_Id,c);
 }

CAPIConn * capircvdConf::getCAPIConn(string calling1,string called1,int PLCI,unsigned Appl_Id, map<unsigned long int,CAPIConn*> *cc)
 {
 map<string,string> *c;
 CAPIConn *dummy;
 char xx[10];

   c=cConf->getconf(string(""),string("controller")+(char)((PLCI & 0x7f)+48) );

   sprintf(xx,"%d",atoi((*c)["usagecount"].c_str())+1);
   (*c)["usagecount"]=string(xx);

   if((*c).find("prefix")!=(*c).end())
    {
    if(dbg_config || dbg_callcontrol)
     cdebug << "Prefix " << (*c)["prefix"] << " wurde gesetzt..." << endl;
    called1=(*c)["prefix"];
    }
   if((*c)["numlength"]!="")
    {
    if(atoi((*c)["numlength"].c_str()) > called1.length())
     {
     if(dbg_config || dbg_callcontrol)
      cdebug << "Nummernlänge nicht erreicht! " << (*c)["numlength"] << ">" << called1.length() << endl;
     dummy=new CAPIConnDDI(Appl_Id,called1,calling1,c,cc);
     return dummy;
     }
    else
     if(dbg_callcontrol || dbg_config)
      cdebug << "Nummernlänge erreicht! " << (*c)["numlength"] << "<=" << called1.length() << endl;
    }
   c=cConf->getconf(calling1,called1);

   sprintf(xx,"%d",atoi((*c)["usagecount"].c_str())+1);
   (*c)["usagecount"]=string(xx);

   int iii;
   string x[]={string("ignore"),string("voice"),string("fax"),string("deflect"),string("reject"),string("ddi"),string("ping")};
   for(iii=0;iii<7 && x[iii]!=(*c)["mode"];iii++);
   if(dbg_callcontrol || dbg_config)
    cdebug << "Dienst ist Nr:" << iii << endl;

   switch(iii) {
    case 0:
      dummy=new CAPIConnIgnore(Appl_Id);
     break;
    case 1:
      dummy=new CAPIConnVoice(Appl_Id,called1,calling1,c);
     break;
    case 2:
      dummy=new CAPIConnFax(Appl_Id,called1,calling1,c);
     break;
    case 3:
      dummy=new CAPIConnDeflect(Appl_Id,c);
     break;
    case 4:
      dummy=new CAPIConnReject(Appl_Id,c);
     break;
    case 5:
      dummy=new CAPIConnDDI(Appl_Id,called1,calling1,c,cc);
     break;
    case 6:
      dummy=new CAPIConnPing(Appl_Id,called1,calling1,c);
     break;
    default:
      dummy=new CAPIConnIgnore(Appl_Id);
    break;
    }
 if((*c).find("exec")!=(*c).end())
  {
  pthread_t p;
  string *x=new string((*c)["exec"]+" exec \""+called1+"\" \""+calling1+"\" \"\" \""+(*c)["recipient"]+"\"");
  pthread_create(&p,NULL,&exechandler,(void *)x);
  }
 return dummy;
 }

void printconf(map<string,string> c)
 {
 map<string,string>::iterator i=c.begin();
 for(;i!=c.end();i++)
    {
    cout << (*i).first << " " << (*i).second << endl;
    }
 }


