#include<linux/capi.h>
#include<capi20.h>
#include<pthread.h>
#include<map>
#include"ccconf.h"

class CAPIApp
 {

private:
 unsigned Appl_Id; 
 struct capi_profile cprofile;

 _cmsg CMSG_Instanz;
 _cmsg *CMSG;

 void HandleConf();
 void HandleInd();
 CAPIConn * byPLCI(unsigned long int plci);
 CAPIConn * byNCCI(unsigned long int ncci);

 map<unsigned long int,CAPIConn*> C;

public:
 bool exit;

 CAPIApp();
 ~CAPIApp();
 bool Run();
 pthread_t start();
 };

void isRunning(CAPIApp *c);

