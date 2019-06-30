#include "UdpManager.h"
#include "GameServer.h"

//------------------------------------------------------------------------------------------------
UdpManager::UdpManager(int sock, float *time,  int *roundbit)
{
	mSock = sock;
	mTimer = 0.0f;
	//ackcounter = 0;
	mTime = time;
	mRoundBit = roundbit;
	mClock = 0.0f;
}

//------------------------------------------------------------------------------------------------
UdpManager::~UdpManager()
{
}

//------------------------------------------------------------------------------------------------
void UdpManager::Send(Packet packet, Connection* connection, bool round)
{
	//Connection* connection = GetConnection(to);
	if (connection != NULL) {
		int roundbit = *mRoundBit;
		if (!round) roundbit ^= 128;

		packet.Data()[0] |= roundbit;
		//connection->packet.WritePacketData(packet);
		connection->packets.push_back(packet);
	}
	//mPackets.push_back(packet);
	//mPacket.WritePacketData(packet);
}

//------------------------------------------------------------------------------------------------
void UdpManager::SendReliable(Packet packet, Connection* connection, bool ordered, bool round)
{
	//Connection* connection = GetConnection(to);
	if (connection != NULL) {
		int roundbit = *mRoundBit;
		if (!round) roundbit ^= 128;

		if (!ordered) {
			connection->ackcounter++;
			if (connection->ackcounter > 32767) {
				connection->ackcounter = -32768;
			}
			packet.WriteId(connection->ackcounter);
			//connection->packet.WritePacketData(packet);
			packet.Data()[0] |= roundbit;
			packet.SetTime(mClock);
			connection->packets.push_back(packet);
			connection->reliablePackets.push_back(packet);
		}
		else {
			connection->orderedackcounter++;
			if (connection->orderedackcounter > 32767) {
				connection->orderedackcounter = -32768;
			}
			packet.WriteId(connection->orderedackcounter);
			packet.Data()[0] |= roundbit;
			packet.SetTime(mClock);
			//connection->packet.WritePacketData(packet);
			connection->packets.push_back(packet);
			connection->orderedPackets.push_back(packet);
		}
	}
	//mReliablePackets.push_back(packet);
	//mPacket.WritePacketData(packet);
}

//------------------------------------------------------------------------------------------------
void UdpManager::SendAck(int ackid, Connection* connection, bool ordered)
{
	Packet sendpacket = Packet();
	if (!ordered) {
		sendpacket.WriteInt8(ACK);
	}
	else {
		sendpacket.WriteInt8(ORDEREDACK);
	}
	sendpacket.WriteInt16(ackid);
	Send(sendpacket,connection);
}

//------------------------------------------------------------------------------------------------
void UdpManager::ReceiveAck(int id, Connection* connection, bool ordered)
{
	//Connection* connection = GetConnection(from);
	if (connection != NULL) {
		std::vector<Packet>* packets;
		if (!ordered) {
			packets = &connection->reliablePackets;
		}
		else {
			packets = &connection->orderedPackets;
		}
		for (int i=0; i<packets->size(); i++) {
			if ((*packets)[i].GetId() == id) {
				packets->erase(packets->begin()+i);
				//i--;
				//printf("ack: %d\n",id);
				return;
			}
		}
	}
	//mReliablePackets.push_back(packet);
	//mPacket.WritePacketData(packet);
}

//------------------------------------------------------------------------------------------------
bool UdpManager::HandleSequence(int ackid, Packet &packet, int start, Connection* connection) {
	if (connection != NULL) { 
		int d = ackid-connection->orderid;
		if (d < -32768) d += 65536;
		if (d > 1) {
			bool exists = false;
			for (int i=0; i<connection->bufferedPackets.size(); i++) {
				if (connection->bufferedPackets[i].GetId() == ackid) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				Packet bufferedpacket = Packet();
				bufferedpacket.SetId(ackid);
				packet.CopyPacket(bufferedpacket,start,packet.Index());
				connection->bufferedPackets.push_back(bufferedpacket);
			}
			return false;
		}
		else if (d <= 0) {
			return false;
		}
		for (int i=0; i<connection->bufferedPackets.size(); i++) {
			if (connection->bufferedPackets[i].GetId() == ackid) {
				connection->bufferedPackets.erase(connection->bufferedPackets.begin()+i);
				break;
			}
		}
		connection->orderid = ackid;
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------------------------
void UdpManager::Update(float dt)
{
	mTimer += dt;
	mClock += dt;
	if (mClock > 1000*1000) {
		mClock = 0.0f;
	}
	
	for (int i=0; i<mConnections.size(); i++) {
		Connection *connection = mConnections[i];
		connection->sendtimer += dt;
		connection->pingtimer += dt;
		connection->ping = connection->ping*0.99f + connection->sping*0.01f;
		if (mTimer > 40) {
			//Packet packet2;
			float latency = connection->ping*0.5f;
			if (latency > 400.0f) latency = 400.0f;
			int largerSize = connection->orderedPackets.size();
			if (connection->reliablePackets.size() > largerSize) {
				largerSize = connection->reliablePackets.size();
			}
			for (int j=0; j<largerSize; j++) {
				if (j < connection->orderedPackets.size()) {
					float packetTime = connection->orderedPackets[j].GetTime();
					if (mClock-packetTime > latency || mClock < packetTime) {
						connection->packets.push_back(connection->orderedPackets[j]);
						connection->orderedPackets[j].SetTime(mClock);
					}
				}
				if (j < connection->reliablePackets.size()) { 
					float packetTime = connection->reliablePackets[j].GetTime();
					if (mClock-packetTime > latency || mClock < packetTime) {
						connection->packets.push_back(connection->reliablePackets[j]);
						connection->reliablePackets[j].SetTime(mClock);
					}
				}
				/*if (packet2.Length() < 450) {
					packet2.WritePacketData(mConnections[i]->orderedPackets[j]);
				}
				else {
					break;
				}*/
			}
			connection->sendtimer = 0;
			//printf("%d %d,%d,%d,%d\n",mConnections[i]->playerid,mConnections[i]->reliablePackets.size(),mConnections[i]->orderedPackets.size(),mConnections[i]->orderid,mConnections[i]->packets.size());

			for (int k=0; k<2; k++) {
				Packet packet;
				packet.WriteInt8(NETVERSION);

				if (k == 0) {
					packet.WriteInt8(TIME);
					packet.WriteFloat(connection->time);
					packet.WriteFloat(*mTime);
				}

				for (int j=0; j<connection->packets.size(); j++) {
					if (packet.Length()+connection->packets[j].Length() < 500) {
						packet.WritePacketData(connection->packets[j]);
						connection->packets.erase(connection->packets.begin()+j);
						j--;
					}
					else {
						//printf("D:\n");
						break;
					}
				}
				//cout << packet.Length();
				//cout << "\n";
				/*if (packet2.Length() > 0) {
					int n = sendto(mSock,packet2.Data(),packet2.Length(), 0,(struct sockaddr *)&mConnections[i]->addr,sizeof(mConnections[i]->addr));
					//if (n == -1) error("sendto");
					//mConnections[i]->packet.Clear();
				}*/
				if (packet.Length() > 0) {
					int n = sendto(mSock,packet.Data(),packet.Length(), 0,(struct sockaddr *)&connection->addr,sizeof(connection->addr));
					//printf("%d\n",n);
					//if (n == -1) error("sendto");
					//mConnections[i]->packet.Clear();
				}
				else {
					break;
				}
				
				
				/*if (mConnections[i]->packet.Length() > 0) {
					int n = sendto(mSock,mConnections[i]->packet.Data(),mConnections[i]->packet.Length(), 0,(struct sockaddr *)&mConnections[i]->addr,sizeof(mConnections[i]->addr));
					//if (n == -1) error("sendto");
					mConnections[i]->packet.Clear();
				}*/
			}
			connection->packets.clear();
		}
	}
	if (mTimer > 40) {
		mTimer = 0.0f;
	}

	//SocketSend(mSocket,packet.Data(), packet.Length());
	//mPacket.Clear();
}

//------------------------------------------------------------------------------------------------
Connection* UdpManager::GetConnection(sockaddr_in to)
{
	for (int i=0; i<mConnections.size(); i++) {
		if (CompareIP(mConnections[i]->addr,to)) {
			return mConnections[i];
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------------------------
Connection* UdpManager::GetConnection(int playerid)
{
	for (int i=0; i<mConnections.size(); i++) {
		if (mConnections[i]->playerid == playerid) {
			return mConnections[i];
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------------------------
void UdpManager::AddConnection(sockaddr_in to)
{
	Connection* connection = new Connection();
	connection->addr = to;
	connection->packet = Packet();
	connection->orderid = 0;
	connection->playerid = -1;
	connection->timer = 0;
	connection->sendtimer = 0;
	connection->ackcounter = 0;
	connection->orderedackcounter = 0;
	connection->ping = 200.0f;
	connection->sping = 200.0f;
	connection->pingtimer = 0;
	connection->reconnecting = false;
	connection->time = 0.0f;
	mConnections.push_back(connection);
}

//------------------------------------------------------------------------------------------------
void UdpManager::RemoveConnection(Connection* connection)
{
	for (int i=0; i<mConnections.size(); i++) {
		if (mConnections[i] == connection) {//CompareIP(mConnections[i]->addr,to)) {
			mConnections[i]->packets.clear();
			mConnections[i]->reliablePackets.clear();
			mConnections[i]->orderedPackets.clear();
			mConnections[i]->bufferedPackets.clear();
			delete mConnections[i];
			mConnections.erase(mConnections.begin()+i);
			return;
		}
	}
}

//------------------------------------------------------------------------------------------------
bool UdpManager::CompareIP(sockaddr_in ptAddr1, sockaddr_in ptAddr2)
{
	if (ptAddr1.sin_addr.S_un.S_addr == ptAddr2.sin_addr.S_un.S_addr 
		&& ptAddr1.sin_port == ptAddr2.sin_port) return true;

	return false;
}