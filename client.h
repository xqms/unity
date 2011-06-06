// Represents a client connection
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>

#include <string>

#include "utils.h"
#include "proto.h"

class Client
{
	public:
		Client(int fd, const std::string& host);
		virtual ~Client();
		
		inline const std::string& host() const
		{ return m_host; }
		
		inline int fd() const
		{ return m_fd; }
		
		inline Direction direction() const
		{ return m_direction; }
		
		inline void setDirection(Direction dir)
		{ m_direction = dir; }
		
		int writePacket(const proto::Packet& packet);
		int readPacket(proto::Packet* dest);
	private:
		int m_fd;
		std::string m_host;
		Direction m_direction;
};

#endif
