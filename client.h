// Represents a client connection
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>

#include "utils.h"

class Client
{
	public:
		Client(int fd, in_addr addr);
		virtual ~Client();
		
		inline in_addr addr() const
		{ return m_addr; }
		
		inline int fd() const
		{ return m_fd; }
		
		inline Direction direction() const
		{ return m_direction; }
		
		inline void setDirection(Direction dir)
		{ m_direction = dir; }
	private:
		int m_fd;
		in_addr m_addr;
		Direction m_direction;
};

#endif
