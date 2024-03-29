#ifndef _WLAN_H_
#define _WLAN_H_

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket close

/*
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h> 
*/
#endif

#include <vector>

#define MAX_PICK 5

struct ConnectionConfig {
	char* name;
	int index;
};

class Socket
{
public:
	int sock;
	struct sockaddr_in addrTo;
	struct sockaddr_in addr;
	bool serverSocket;
	//Socket():  {};
};


static const char* wlanNotInitialized = "WLAN not initialized.";
static bool wlanInitialized = false;
static char resolverBuffer[1024];
static int resolverId;

int WlanInit();
int WlanTerm();
std::vector<ConnectionConfig> GetConnectionConfigs();
int UseConnectionConfig(int config, int state);
char* GetIPAddress();
int SocketFree(Socket* socket);
int SetSockNoBlock(int s, unsigned long val);
int SocketConnect(Socket* socket, char* host, int port);
int SocketConnectUdp(Socket* socket, char* host, int port);
int SocketRecv(Socket* socket, char* buf, int size);
//sockaddr SocketRecvfrom(Socket* socket, char* buf, int size);
int SocketSend(Socket* socket, char* buf, int size);
int SocketSendUdp(Socket* socket, char* buf, int size);
int SocketClose(Socket* socket);

#endif
