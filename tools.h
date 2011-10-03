#include<string>
#include<capi20.h>
#include<fstream>



using namespace std;


int decode(unsigned char sample);
void * PlayNameThread(void *a);
string getCalledNumber(_cmsg *CMSG);
string getCallingNumber(_cmsg *CMSG);

extern string iprefix;
extern string nprefix;
extern string configdir;
extern bool playaudio;
extern string datadir;
extern unsigned short int debug;

#define cdebug cout

#define dbg_any			(debug        != 0)
#define dbg_init		(debug &    1 ) != 0
#define dbg_systeminfo		(debug &    2 ) != 0
#define dbg_fileop		(debug &    4 ) != 0
#define dbg_cmds		(debug &    8 ) != 0
#define dbg_config		(debug &   16 ) != 0
#define dbg_callcontrol		(debug &   32 ) != 0
#define dbg_datatransfer	(debug &   64 ) != 0
#define dbg_capireq		(debug &  128 ) != 0
#define dbg_capiresp		(debug &  256 ) != 0
#define dbg_capiind		(debug &  512 ) != 0
#define dbg_capiconf		(debug & 1024 ) != 0
#define dbg_telnet		(debug & 2048 ) != 0 
