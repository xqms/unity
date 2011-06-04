// Unity: a mouse/keyboard sharing solution
// Author: Max Schwarz <Max@x-quadraht.de>

#include <QtGui/QApplication>
#include <QtCore/QList>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>

#include "utils.h"
#include "grabwindow.h"
#include "edgewindow.h"
#include "proto.h"
#include "client.h"

bool grabbing = false;
Direction current_direction;

Display* display;
GrabWindow* grabber;
EdgeWindow* edges[4];
QList<Client*> clients;

void injectMotion(int dx, int dy)
{
	XTestFakeRelativeMotionEvent(display, dx, dy, 0);
}

void warpTo(int x, int y)
{
	XWarpPointer(display, None, DefaultRootWindow(display), 0, 0, 0, 0, x, y);
}

void sendPacket(int fd, const proto::Packet& packet)
{
	if(write(fd, (char*)&packet, sizeof(packet)) != sizeof(packet))
		fatal("Could not write");
}

void pass_to(Direction dir, int last_x, int last_y)
{
	Client* client = 0;
	foreach(Client* c, clients)
	{
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
	current_direction = dir;
	
	grabbing = true;
	
	sendPacket(client->fd(), proto::activatePacket(last_x, last_y));
}

void distributePacket(const proto::Packet& packet)
{
	foreach(Client* client, clients)
	{
		int fd = client->fd();
		sendPacket(fd, packet);
	}
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
	warpTo(x, y);
}

void readPacket(Client* client)
{
	int fd = client->fd();
	proto::Packet packet;
	
	int ret = read(fd, &packet, sizeof(proto::Packet));
	
	if(ret == 0)
	{
		printf("Client %s has disconnected\n", inet_ntoa(client->addr()));
		clients.removeOne(client);
		
		if(grabbing && current_direction == client->direction())
		{
			grabbing = false;
			grabber->release();
		}
		
		return;
	}
	
	if(ret < 0)
		fatal("Could not read from socket %d", fd);
	
	switch(packet.type)
	{
		case proto::Packet::T_MOTION:
			injectMotion(packet.motion.dx, packet.motion.dy);
			break;
		case proto::Packet::T_IDENTIFY:
			printf("Client '%s' is in direction %s\n", inet_ntoa(client->addr()), DIRECTION_STR[packet.identify.direction]);
			client->setDirection((Direction)packet.identify.direction);
			break;
		case proto::Packet::T_ACTIVATE:
			printf("Pointer is on my screen\n");
			handleActivate(client, packet);
			break;
		case proto::Packet::T_BUTTONPRESS:
			XTestFakeButtonEvent(display, packet.button.number, True, 0);
			break;
		case proto::Packet::T_BUTTONRELEASE:
			XTestFakeButtonEvent(display, packet.button.number, False, 0);
			break;
		case proto::Packet::T_KEYPRESS:
			XTestFakeKeyEvent(display, packet.key.keycode, True, 0);
			break;
		case proto::Packet::T_KEYRELEASE:
			XTestFakeKeyEvent(display, packet.key.keycode, False, 0);
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
				distributePacket(proto::keyPress(event.xkey.keycode));
				break;
			case KeyRelease:
				distributePacket(proto::keyRelease(event.xkey.keycode));
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
	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int client_fd = accept(fd, (sockaddr*)&addr, &addrlen);
	if(client_fd < 0)
		fatal("Could not accept()");
	
	printf("Got new client '%s' on fd %d\n", inet_ntoa(addr.sin_addr), client_fd);
	
	clients.append(new Client(client_fd, addr.sin_addr));
}

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	
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
		
		int client_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(client_fd < 0)
			fatal("Could not allocate client socket");
		
		sockaddr_in saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(addr);
		saddr.sin_port = htons(6030);
		
		if(connect(client_fd, (const sockaddr*)&saddr, sizeof(saddr)) != 0)
			fatal("Could not connect to client '%s'", addr);
		
		printf("Connected to client '%s' on fd %d\n", addr, client_fd);
		
		Client* client = new Client(client_fd, saddr.sin_addr);
		clients.append(client);
		
		// Send identify packet
		sendPacket(client_fd, proto::identifyPacket(oppositeDirection(dir)));
		client->setDirection(dir);
	}
	
	// Get X screen
	display = XOpenDisplay(0);
	if(!display)
		fatal("Could not open display");
	
	// We need the XTest extension
	int opcode, event, error;
	if(!XQueryExtension(display, XTestExtensionName, &opcode, &event, &error))
		fatal("X server does not have XTest extension, aborting");
	
	// Setup grab window
	grabber = new GrabWindow(display);
	
	// Setup edge windows
	for(int i = 0; i < 4; ++i)
		edges[i] = new EdgeWindow(display, (Direction)i);
	
	int x11_fd = ConnectionNumber(display);
	
	fd_set fds;
	
	while(1)
	{
		int max = 0;
		
		FD_ZERO(&fds);
		add_to_fdset(&fds, x11_fd, &max);
		add_to_fdset(&fds, server_fd, &max);
		
		foreach(Client* client, clients)
			add_to_fdset(&fds, client->fd(), &max);
		
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
		
		foreach(Client* client, clients)
		{
			if(FD_ISSET(client->fd(), &fds))
			{
				readPacket(client);
			}
		}
	}
}
