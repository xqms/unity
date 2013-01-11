// Unity: a mouse/keyboard sharing solution
// Author: Max Schwarz <Max@x-quadraht.de>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <set>

#include "utils.h"
#include "grabwindow.h"
#include "edgewindow.h"
#include "proto.h"
#include "client.h"
#include "injector.h"

bool grabbing = false;
Client* current_client;

Display* display;
GrabWindow* grabber;
EdgeWindow* edges[4];
Injector* injector;
std::set<Client*> clients;

void pass_to(Direction dir, int last_x, int last_y)
{
	Client* client = 0;
	for(std::set<Client*>::iterator it = clients.begin();
	  it != clients.end(); ++it)
	{
		Client* c = *it;
		
		if(c->direction() == dir)
		{
			client = c;
			break;
		}
	}
	if(!client)
	{
		printf("No client for this direction\n");
		return;
	}
	
	printf("Pointer is on %s screen\n", DIRECTION_STR[dir]);
	
	grabber->grab();
	current_client = client;
	
	grabbing = true;
	
	client->writePacket(proto::activatePacket(last_x, last_y));
}

void distributePacket(const proto::Packet& packet)
{
	current_client->writePacket(packet);
}

void handleActivate(Client* client, const proto::Packet& packet)
{
	Screen* screen = DefaultScreenOfDisplay(display);
	int sw = WidthOfScreen(screen);
	int sh = HeightOfScreen(screen);
	
	// Calculate initial mouse position
	int x, y;
	
	switch(client->direction())
	{
		case D_LEFT:   x =    1; y = packet.activate.last_y; break;
		case D_RIGHT:  x = sw-2; y = packet.activate.last_y; break;
		case D_TOP:    y =    1; x = packet.activate.last_x; break;
		case D_BOTTOM: y = sh-2; x = packet.activate.last_x; break;
		default:
			fprintf(stderr, "Can't activate an unidentified client\n");
			return;
	}
	
	if(grabbing)
		grabber->release();
	
	grabbing = false;
	injector->injectMotionAbsolute(x, y);
}

bool directionUsed(Direction dir)
{
	for(std::set<Client*>::iterator it = clients.begin();
	  it != clients.end(); ++it)
	{
		if((*it)->direction() == dir)
			return true;
	}
	
	return false;
}

void disconnectClient(Client* client)
{
	printf("Client %s has disconnected\n", client->host().c_str());
	clients.erase(client);
	
	if(grabbing && current_client == client)
	{
		grabbing = false;
		grabber->release();
	}
	
	// Test wether another client uses this direction
	if(!directionUsed(client->direction()))
	{
		delete edges[client->direction()];
		edges[client->direction()] = 0;
	}
	
	delete client;
}

void handleIdentify(Client* client, Direction dir)
{
	client->setDirection(dir);
	
	// Check if we need to create the corresponding egde window
	if(dir >= 4)
		return;
	
	if(!edges[dir])
		edges[dir] = new EdgeWindow(display, dir);
}

void handlePacket(const proto::Packet& packet, Client* client)
{
	switch(packet.type)
	{
		case proto::Packet::T_MOTION:
			injector->injectMotionRelative(packet.motion.dx, packet.motion.dy);
			break;
		case proto::Packet::T_IDENTIFY:
			printf("Client '%s' is in direction %s\n", client->host().c_str(), DIRECTION_STR[packet.identify.direction]);
			handleIdentify(client, (Direction)packet.identify.direction);
			break;
		case proto::Packet::T_ACTIVATE:
			printf("Pointer is on my screen\n");
			handleActivate(client, packet);
			break;
		case proto::Packet::T_BUTTONPRESS:
			injector->injectButtonEvent(packet.button.number, true);
			break;
		case proto::Packet::T_BUTTONRELEASE:
			injector->injectButtonEvent(packet.button.number, false);
			break;
		case proto::Packet::T_KEYPRESS:
			injector->injectKeyEvent(packet.key.keysym, true);
			break;
		case proto::Packet::T_KEYRELEASE:
			injector->injectKeyEvent(packet.key.keysym, false);
			break;
	}
}

void handleXEvents()
{
	while(XPending(display))
	{
		XEvent event;
		XNextEvent(display, &event);
		
		switch(event.type)
		{
			case EnterNotify:
				for(int i = 0; i < 4; ++i)
				{
					if(!edges[i])
						continue;
					
					if(edges[i]->window() == event.xcrossing.window)
					{
						printf("Switch to %s\n", DIRECTION_STR[i]);
						pass_to((Direction)i, event.xcrossing.x_root, event.xcrossing.y_root);
						break;
					}
				}
				break;
			case MotionNotify:
				if(grabbing)
				{
					int dx, dy;
					grabber->processMotion(&event.xmotion, &dx, &dy);
					
					if(dx || dy)
					{
						// Inject pointer motion packet
						distributePacket(proto::motionPacket(dx, dy));
					}
				}
				break;
			case ButtonPress:
				distributePacket(proto::buttonPress(event.xbutton.button));
				break;
			case ButtonRelease:
				distributePacket(proto::buttonRelease(event.xbutton.button));
				break;
			case KeyPress:
			case KeyRelease:
				{
					int keysym = XLookupKeysym(&event.xkey, 0);
					distributePacket(
						proto::keyEvent(
							(event.type == KeyPress) ? proto::Packet::T_KEYPRESS : proto::Packet::T_KEYRELEASE,
							keysym
						)
					);
				}
				break;
		}
	}
}

static void add_to_fdset(fd_set* fds, int fd, int* max)
{
	FD_SET(fd, fds);
	if(fd > *max)
		*max = fd;
}

void handleNewClient(int fd)
{
	sockaddr_in6 addr;
	socklen_t addrlen = sizeof(addr);
	int client_fd = accept(fd, (sockaddr*)&addr, &addrlen);
	if(client_fd < 0)
		fatal("Could not accept()");
	
	// Get IPv4/6 address
	char host[100];
	if(getnameinfo((const sockaddr*)&addr, addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST) != 0)
		fatal("Could not convert IP address to string");
	
	printf("Got new client '%s' on fd %d\n", host, client_fd);
	
	clients.insert(new Client(client_fd, host));
}

void connectTo(Direction dir, const char* host)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_UNSPEC;
	
	addrinfo* addr_result;
	if(getaddrinfo(host, "6030", &hints, &addr_result) != 0)
		fatal("Could not resolve host '%s'", host);
	
	int fd = -1;
	addrinfo* loop;
	
	for(loop = addr_result; loop; loop = loop->ai_next)
	{
		fd = socket(loop->ai_family, loop->ai_socktype, loop->ai_protocol);
		if(fd < 0)
			continue;
		
		if(connect(fd, loop->ai_addr, loop->ai_addrlen) != 0)
		{
			close(fd);
			continue;
		}
		
		break;
	}
	
	if(!loop)
		fatal("Could not connect to client '%s'", host);
	
	freeaddrinfo(addr_result);
	
	printf("Connected to client '%s'\n", host);
	
	Client* client = new Client(fd, host);
	clients.insert(client);
	
	// Setup edge window
	if(!edges[dir])
		edges[dir] = new EdgeWindow(display, dir);
	
	// Send identify packet
	client->writePacket(proto::identifyPacket(oppositeDirection(dir)));
	client->setDirection(dir);
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	
	// Get X screen
	display = XOpenDisplay(0);
	if(!display)
		fatal("Could not open display");
	
	// Setup server socket
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
		fatal("Could not allocate server socket");
	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(6030);
	
	if(bind(server_fd, (const sockaddr*)&addr, sizeof(addr)) != 0)
		fatal("Could not bind to port 6030");
	
	if(listen(server_fd, 15) != 0)
		fatal("Could not listen()");
	
	// Setup grab window
	grabber = new GrabWindow(display);
	
	// Setup event injection
	injector = new Injector(display);
	
	memset(edges, 0, sizeof(edges));
	
	// Connect to listed clients
	for(int i = 1; i < argc-1; ++i)
	{
		const char* str_dir = argv[i];
		const char* addr = argv[i+1];
		
		Direction dir = D_UNKNOWN;
		
		for(int j = 0; j < 4; ++j)
		{
			if(strcasecmp(str_dir, DIRECTION_STR[j]) == 0)
			{
				dir = (Direction)j;
				break;
			}
		}
		if(dir == D_UNKNOWN)
			fatal("Unknown direction %s", str_dir);
		
		connectTo(dir, addr);
	}
	
	int x11_fd = ConnectionNumber(display);
	
	proto::Packet packet;
	fd_set fds;
	
	while(1)
	{
		int max = 0;
		
		FD_ZERO(&fds);
		add_to_fdset(&fds, x11_fd, &max);
		add_to_fdset(&fds, server_fd, &max);
		
		for(std::set<Client*>::iterator it = clients.begin();
		  it != clients.end(); ++it)
			add_to_fdset(&fds, (*it)->fd(), &max);
		
		XPending(display);
		int ret = select(max + 1, &fds, 0, 0, 0);
		if(ret < 0)
			fatal("Could not select()");
		if(ret == 0)
			continue;
		
		if(FD_ISSET(x11_fd, &fds))
			handleXEvents();
		if(FD_ISSET(server_fd, &fds))
			handleNewClient(server_fd);
		
		for(std::set<Client*>::iterator it = clients.begin();
		  it != clients.end(); ++it)
		{
			Client* client = *it;
			
			if(FD_ISSET(client->fd(), &fds))
			{
				if(client->readPacket(&packet) != 0)
					disconnectClient(client);
				else
					handlePacket(packet, client);
			}
		}
	}
}
