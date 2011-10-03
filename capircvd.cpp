#include"tools.h"
#include"capiapp.h"

#include<unistd.h>
#include<iostream>
#include<time.h>
#include<sys/stat.h>
#include<signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

string iprefix("49");
string nprefix("5921");

// directory containing "capircvd.conf"
string configdir("/etc/capircvd");

// directory containing the recorded files and faxes
string datadir("/var/spool/capircvd");

// the logfile (when not using "-L"
string logfile("/var/log/capircvd");

// does capircvd play "Namen_"-files 
bool playaudio=false;

// the debug-mode and the debug outputstream
unsigned short int debug=0;

// signalhandler for reloading configuration...
void sigreload(int x)
 {
 cdebug << "SIGHUP reloading configuration from \"" << configdir << "/capircvd.conf\" " << endl;

 capircvdConf *c,*d;
 c=new capircvdConf(configdir+"/capircvd.conf");

 d=cConf;
 cConf=c;
 delete d;
 }

void sigusr1(int x)
 {
 signal(SIGUSR1,SIG_IGN);
 cdebug << "signal USR1 received! dumping..." << endl;

 if(cConf==NULL || ! cConf->dumpconf(configdir+"/capircvd.state"))
  cdebug << "dump to capircvd.state failed !!!" << endl;
 signal(SIGUSR1,sigusr1);
 }

void sigterm(int x)
 {
 signal(SIGHUP,SIG_IGN);
 signal(SIGUSR1,SIG_IGN);
 signal(SIGTERM,SIG_IGN);
 signal(SIGINT,SIG_IGN);
 signal(SIGABRT,SIG_IGN);
 signal(SIGPIPE,SIG_IGN);

 cdebug << "signal received! exiting..." << endl;

 if(cConf==NULL || ! cConf->dumpconf(configdir+"/capircvd.state"))
  cdebug << "dump to capircvd.state failed !!!" << endl;

 exit(0);
 }

int main(int argc,char *arg[])
 {
// catch following signals
 signal(SIGHUP,sigreload);
 signal(SIGUSR1,sigusr1);

 signal(SIGTERM,sigterm);
 signal(SIGINT,sigterm);
 signal(SIGABRT,sigterm);
 signal(SIGPIPE,SIG_IGN);

// some startup-configuration
 bool become_daemon=false,restoredump=false;

 for(int i=0;i<argc;i++)
  {
  if(arg[i][0]=='-')
   switch(arg[i][1])
    {
    case 'D':
     debug=atoi(arg[i+1]);
     if(debug==0) 
      debug=65535;
     i++;
     cout << "debuglevel set to 0d0" << debug << endl; 
     break;
    case 'L':
     logfile=string(arg[i+1]);
     i++;
     break;
    case 'b':
     become_daemon=true;
     break;
    case 'r':
     restoredump=true;
     break;
    case 'd':
     datadir=string(arg[i+1]);
     i++;
     break;
    case 'c':
     configdir=string(arg[i+1]); 
     i++;
     break;
    case 'a':
     playaudio=true;
     break;
    case 'V':
     cout << " capircvd. Receive voice and fax, use call deflection ... " << endl;
     cout << "  Using Linux/AVM B1 or Fritzcard with CAPI"<< endl;
     cout << "  Written by alexander@brickwedde.de, see http://www.capircvd.de" << endl << endl;
     cout << " Version:" << VERSION << endl;
     cout << "  Copyright (C) 2000 Alexander Brickwedde" << endl << endl;
     cout << "  This program is free software; you can redistribute it and/or modify" << endl;
     cout << "  it under the terms of the GNU General Public License as published by" << endl;
     cout << "  the Free Software Foundation; either version 2 of the License, or" << endl;
     cout << "  (at your option) any later version." << endl << endl;
     cout << "  This program is distributed in the hope that it will be useful," << endl;
     cout << "  but WITHOUT ANY WARRANTY; without even the implied warranty of" << endl;
     cout << "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the" << endl;
     cout << "  GNU General Public License for more details." << endl << endl;
     cout << "  You should have received a copy of the GNU General Public License" << endl;
     cout << "  along with this program; if not, write to the Free Software" << endl;
     cout << "  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA." << endl << endl;
     cout << "   Program not starting...!" << endl;
     return 0;
     break;
    default:
     cout << endl << 
             "Usage:     capircvd [-D debuglevel] [-L logfile] [-b] [-d datenverzeichnis] [-f konfigdatei] [-a] [-V]" << endl << endl;
     cout << "Startet capircvd, ein Programm zum Empfang von Faxen und Voice..." << endl << endl;
     cout << " 'capircvd -V' zeigt Lizenzinfo/shows license" << endl << endl;
     cout << "  -D debuglevel          debuglevel ist eine Bitmaske, default ist keine" << endl <<
             "                         Debugausgabe! 0 bedeutet alles ausgeben, alles" << endl <<
             "                         andere wird aus der tools.h ersichtlich!" << endl;
     cout << "  -L logfile             Default für die Logdatei ist /var/log/capircvd," << endl <<
             "                         -L - verwendet die Konsole" << endl;
     cout << "  -b                     Startet capircvd als Daemon, als im Hintergrund" << endl;
     cout << "  -d datenverzeichnis    In diesem Verzeichnis werden Aufnahmen und Faxe" << endl <<
             "                         gespeichert, default ist /var/spool/capircvd" << endl;
     cout << "  -c konfigverzeichnis   Das konfigverzeichnis enthaelt die Konfigrationsdatei capircvd.conf," << endl <<
             "                         default ist /etc/capircvd" << endl <<
             "                         -f ? gibt Hilfe zur Konfiguration aus" << endl;
     cout << "  -r                     Versucht die Konfiguration aus der capircvd.state wiederherzustellen," << endl <<
             "                         wenn nicht erfolgreich wird capircvd.conf verwendet!" << endl;
     cout << "  -a                     bewirkt das Abspielen der Namen_*-Dateien im" << endl <<
             "                         Datenverzeichnis auf /dev/audio sobald ein" << endl << 
             "                         Ruf eintrifft." << endl;
     cout << "  -V                     zeigt die aktuelle Version an" << endl << endl;
     cout << "    Weitere Hilfe im README oder bei alex@brickwedde.de" << endl;
     cout << "    For help on configuring capircvd please contact alex@brickwedde.de" << endl << endl ;

     return 1;
     break;
    }
  }

 int co;
 if(logfile!="-")
  {
  close(1);
  co=open(logfile.c_str(),O_RDWR | O_CREAT | O_APPEND);
  cout << "co=" << co << " " << logfile << endl;
  }

 if(become_daemon)
  {
  pid_t p;
  p=fork();
  switch(p)
   {
   case 0:
    cdebug << "This is the child..." << endl;
    break;

   case -1: 
    cout << "Cannot fork!" << endl;
    cdebug << "Cannot fork!" << endl;
    exit(1);
    break;

   default: 
    cout << "capircvd forked..." << endl;
    exit(0);
    break;
   }
  }

 if(dbg_init)
  {
  time_t t;
  time(&t);
  cdebug << "capircvd " << VERSION << " started @ " << ctime(&t) << endl;
  }

 bool ex;

 do
  {
  ex=true;
  cConf=NULL;

  if(restoredump)
   {
   if(dbg_fileop)
    cdebug << "Trying to load " << configdir << "/capircvd.state" << endl;
   try
    {
    cConf=new capircvdConf(configdir+"/capircvd.state");
    }
   catch(unsigned int i)
    {
    cConf=NULL;
    cdebug << "can't load capircvd.state..." << endl;
    }
   }
  if(cConf==NULL)
   { 
   if(dbg_fileop)
    cdebug << "Trying to load " << configdir << "/capircvd.conf" << endl;
   try
    {
    cConf=new capircvdConf(configdir+"/capircvd.conf");
    }
   catch(unsigned int i)
    {
    cout << "can't load configfile..." << endl;
    return 1;
    }
   if(dbg_fileop)
    cdebug << "fileop	: configfile is '" << configdir+"/capircvd.conf" << "'" << endl;
   }
  else
   {
   if(dbg_fileop)
    cdebug << "fileop	: configfile is '" << configdir+"/capircvd.state" << "'" << endl;
   }

  map<string,map<string,string> > *conf=&(cConf->conf);

  if(debug==0 && conf->find("admin")!=conf->end() && (*conf)["admin"].find("debug")!=(*conf)["admin"].end() )
   {
   debug=atoi( (*conf)["admin"]["debug"].c_str() );
   cdebug << "Debugmode changed (by conf-file) to 0x" << hex << debug << dec << endl;
   }

  if(conf->find("global")!=conf->end() && (*conf)["global"].find("onerror")!=(*conf)["global"].end() )
   {
   ExitonError=(strcmp((*conf)["global"]["onerror"].c_str(),"exit")==0);
   cdebug << "'Exit on Error' is activated!" << endl;
   }

  if(conf->find("global")!=conf->end())
   {
   if((*conf)["global"].find("iprefix")!=(*conf)["global"].end())
    {
    iprefix=(*conf)["global"]["iprefix"];
    cdebug << "sinfo	: Using " << iprefix << " as international prefix" << endl;
    } 
   if((*conf)["global"].find("nprefix")!=(*conf)["global"].end())
    {
    nprefix=(*conf)["global"]["nprefix"];
    cdebug << "sinfo	: Using " << nprefix << " as national prefix" << endl;
    }
   }
  if(dbg_fileop)
   cdebug << "fileop	: spool-directory:" << datadir << endl;
 
  struct stat buf;
  stat(datadir.c_str(),&buf);
 
  if(!S_ISDIR(buf.st_mode))
   {
   cdebug << "spool-directory does not exist!!!" << endl << "Exit...(1)" << endl;
   return 1;
   }

  CAPIApp App;
  if(dbg_init)
   cdebug << "init	: CAPIApp.Run() will be called" << endl; 

  App.Run();
  } 
 while(!ex);

// cdebug_file.close();
// close(co);
 delete cConf;

 return 0;

 }

