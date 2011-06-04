// Represents a client connection
// Author: Max Schwarz <Max@x-quadraht.de>

#include "client.h"

#include <unistd.h>

Client::Client(int fd, in_addr addr)
 : m_fd(fd), m_addr(addr)
{
}

Client::~Client()
{
	close(m_fd);
}
