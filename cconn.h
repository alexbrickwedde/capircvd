#include<capi20.h>
#include<string>
#include<map>
#include<fstream>

void * WaitConnectThread(void *a);
void * ExternalHandler(void *a);

using namespace std;

class CAPIConn
 {
protected:

// for DataStreams:
 bool receiving,sending,receivelater;
 string ofile,ifile,ocmd,called,calling,mailto;
 int i_fd;
 int o_fd;

 int RunningCmd;

 unsigned char *buf;

 map<string,string> *C;
 unsigned Appl_Id;
 bool WaitingforConnect,PinOK,ExecHandler;
 pthread_t WaitConnect;
 unsigned char B1,B2,B3,Reject; 
 unsigned long MsgNr;
 u_int32_t NCCI;
 u_int32_t PLCI;
 string dtmf;
public:
 unsigned long WaitConnectTime;
 ~CAPIConn();
 CAPIConn(unsigned applid,map<string,string> *c); 
 virtual void Connect(_cmsg *CMSG);
 virtual void Facility(_cmsg *CMSG);
 virtual void Disconnect(_cmsg *CMSG);
 virtual void DisconnectB3(_cmsg *CMSG);
 virtual void setWaitforConnect();
 virtual void ConnectIt();
 virtual void ConnectActive(_cmsg *CMSG);
 virtual void ConnectB3(_cmsg *CMSG);
 virtual void ConnectB3Active(_cmsg *CMSG);
 virtual void InfoInd(_cmsg *CMSG);

 virtual void ChangeFiles(string i,string o);
 virtual void OpenDatastream();

 virtual void B3Data(_cmsg *CMSG);
 virtual void DataConf(_cmsg *CMSG);
 };

class CAPIConnDatastream: public CAPIConn
 {
protected:
 unsigned long int Hdl,len;
public:
 CAPIConnDatastream(unsigned applid,string ii,string oo,map<string,string> *c); 
 virtual void OpenDatastream();
 virtual void ConnectB3Active(_cmsg *CMSG);
 virtual void B3Data(_cmsg *CMSG);
 virtual void DataConf(_cmsg *CMSG);
 virtual void DisconnectB3(_cmsg *CMSG);
 void Send();
 };

class CAPIConnIgnore: public CAPIConn
 {
public: 
 CAPIConnIgnore(unsigned applid);
 virtual void Connect(_cmsg *CMSG);
 };

class CAPIConnVoice: public CAPIConnDatastream
 {
public: 
 CAPIConnVoice(unsigned applid,string ii,string oo,map<string,string> *c);
// ~CAPIConnVoice();
 virtual void Connect(_cmsg *CMSG);
 virtual void Disconnect(_cmsg *CMSG);
 virtual void ConnectB3(_cmsg *CMSG);
 };

class CAPIConnFax: public CAPIConnDatastream
 {
public:
 string remoteid;
 CAPIConnFax(unsigned applid,string ii,string oo,map<string,string> *c);
 virtual void Connect(_cmsg *CMSG);
 virtual void ConnectIt();
 virtual void Disconnect(_cmsg *CMSG);
 virtual void DisconnectB3(_cmsg *CMSG);
 };

class CAPIConnPing: public CAPIConnVoice
 {
public:
 CAPIConnPing(unsigned applid,string ii,string oo,map<string,string> *c);
 virtual void Facility(_cmsg *CMSG);
 };

class CAPIConnFaxpoll: public CAPIConnDatastream
 {
public: 
 string remoteid;
 CAPIConnFaxpoll(unsigned applid,string ii,string oo,map<string,string> *c);
 virtual void Connect(_cmsg *CMSG);
 virtual void ConnectIt();
 virtual void ConnectActive(_cmsg *CMSG);
 virtual void Disconnect(_cmsg *CMSG);
 virtual void DisconnectB3(_cmsg *CMSG);
 };

class CAPIConnDeflect: public CAPIConn
 {
 string RedirectTo;
public: 
 CAPIConnDeflect(unsigned applid,map<string,string> *c);
 virtual void Connect(_cmsg *CMSG);
 virtual void ConnectIt();
 };

class CAPIConnReject: public CAPIConn
 {
public: 
 CAPIConnReject(unsigned applid,map<string,string> *c);
 virtual void Connect(_cmsg *CMSG);
 virtual void ConnectIt();
 };

class CAPIConnDDI: public CAPIConn
 {
private:
 string nr,ca; 
 int anzahlnr;
 map<unsigned long int,CAPIConn*> *Conns;
 _cmsg *ConnInd;
public: 
 CAPIConnDDI(unsigned applid,string n,string caller,map<string,string> *c, map<unsigned long int,CAPIConn*> *cc);
 virtual void Connect(_cmsg *CMSG);
 virtual void ConnectIt();
 virtual void InfoInd(_cmsg *CMSG);
 };


