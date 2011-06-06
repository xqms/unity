// Represents a client connection
// Author: Max Schwarz <Max@x-quadraht.de>

#include "client.h"
#include "utils.h"

#include <unistd.h>
#include <stdio.h>

Client::Client(int fd, const std::string& host)
 : m_fd(fd), m_host(host)
{
}

Client::~Client()
{
	close(m_fd);
}

int Client::readPacket(proto::Packet* dest)
{
	int ret = read(m_fd, dest, sizeof(proto::Packet));
	
	if(ret == 0)
		return 1; // Client disconnected
	if(ret != sizeof(proto::Packet))
		fatal("Could not read from client %s", m_host.c_str());
	
	return 0;
}

int Client::writePacket(const proto::Packet& packet)
{
	int ret = write(m_fd, (const char*)&packet, sizeof(proto::Packet));
	
	if(ret == 0)
		return 1; // Client disconnected
	if(ret != sizeof(proto::Packet))
		fatal("Could not write to client %s", m_host.c_str());
	
	return 0;
}

