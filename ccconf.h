#include<string>
#include<map>
#include<sys/socket.h>
#include<arpa/inet.h>

#include"cconn.h"

extern bool ExitonError;

class capircvdConf
 {
private: 
 sockaddr_in saddr;
 sockaddr_in caddr;
 socklen_t addrlen;
 pthread_t acceptthread;
 int sock;
public:
 map<string,map<string,string> > conf;
 capircvdConf(string file);
 map<string,string>* getconf(string calling,string called);
 bool dumpconf(string file);

 CAPIConn * getCAPIConn(_cmsg *CMSG,unsigned Appl_Id, map<unsigned long int,CAPIConn*> *c);
 CAPIConn * getCAPIConn(string calling1,string called1,int PLCI,unsigned Appl_Id, map<unsigned long int,CAPIConn*> *cc);

 }; // class configuration

void printconf(map<string,string> c);
extern capircvdConf *cConf;

