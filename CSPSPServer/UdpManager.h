#pragma once
#ifndef _UDPMANAGER_H_
#define _UDPMANAGER_H_

#include "Packet.h"
#include <vector>

#ifdef _WIN32
#include <winsock.h>
#else
//#include <sys/socket.h>
#include <netinet/in.h>
#endif

enum {
	GAMEINFO = 0,
	GAMEINFOEND,
	GUNINFO,
	NEWMAP,
	NEWPLAYER,
	REMOVEPLAYER,
	CONNECT,
	SWITCHTEAM,
	PLAYERMOVE,
	RESPAWN,
	MOVE,
	SWITCHGUN,
	SPAWNGUN,
	PICKUPGUN,
	DROPGUN,
	DROPGUNDEAD,
	NEWGUN,
	MOVEGUN,
	STARTFIRE,
	ENDFIRE,
	NEWBULLET,
	NEWSHOTGUNBULLET,
	REMOVEBULLET,
	STARTRELOAD,
	ENDRELOAD,
	HIT,
	HITINDICATOR,
	DAMAGEINDICATOR,
	KILLEVENT,
	WINEVENT,
	RESETROUND,
	RESETROUNDEND,
	CHAT,
	BUY,
	ERROR1,
	PING,
	PLAYERPING,
	SERVERINFO,
	MAPFILE,
	SERVERMESSAGE,
	ACK,
	ORDEREDACK,
	NEWGRENADE,
	EXPLODEGRENADE,
	RECEIVEFLASH,
	MESSAGE,
	TIMEMULTIPLIER,
	PLAYERICON,
	RESETPLAYERS,
	CTFINFO,
	PICKUPFLAG,
	DROPFLAG,
	RETURNFLAG,
	CAPTUREFLAG,
	TIME
};

struct Connection {
	struct sockaddr_in addr;
	Packet packet;
	std::vector<Packet> packets;
	std::vector<Packet> reliablePackets;
	std::vector<Packet> orderedPackets;
	std::vector<Packet> bufferedPackets;
	int orderid;
	float timer;
	float sendtimer;
	int playerid;
	int ackcounter;
	int orderedackcounter;
	float pingtimer;
	std::vector<float> pings;
	float ping;
	float sping;
	bool reconnecting;
	float time;
	std::vector<int> acks;
	std::vector<int> orderedAcks;

};

class Packet;
//------------------------------------------------------------------------------------------------
class UdpManager
{
private:
	//Packet mPacket;
	int mSock;
	//int ackcounter;
	float mTimer;
	int *mRoundBit;
	float *mTime;
	float mClock;

protected:

public:
	std::vector<Connection*> mConnections;

	UdpManager(int sock, float *time, int *roundbit);
	~UdpManager();
	void Send(Packet packet, Connection* connection, bool round = true);
	void SendReliable(Packet packet, Connection* connection, bool ordered = false, bool round = true);
	void SendAck(int ackid, Connection* connection, bool ordered = false);
	void ReceiveAck(int id, Connection* connection, bool ordered = false);
	bool HandleSequence(int ackid, Packet &packet, int start, Connection* connection);
	//void SendAll();
	void Update(float dt);
	Connection* GetConnection(sockaddr_in to);
	Connection* GetConnection(int playerid);
	Connection* AddConnection(int playerid, sockaddr_in to);
	void RemoveConnection(Connection* connection);
	bool CompareIP(sockaddr_in ptAddr1, sockaddr_in ptAddr2);
};

#endif
