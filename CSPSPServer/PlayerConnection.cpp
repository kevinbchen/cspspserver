//#include "PlayerConnection.h"

PlayerConnection::PlayerConnection(struct sockaddr_in from, std::vector<Person*>* players)
{
	addrTo = from;
	this->players = players;
	

}

PlayerConnection::~PlayerConnection()
{
}
