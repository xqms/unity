// Unity communication protocol
// Author: Max Schwarz <Max@x-quadraht.de>

#ifndef PROTO_H
#define PROTO_H

namespace proto
{
	struct MotionInfo
	{
		int32_t dx;
		int32_t dy;
	};
	
	struct IdentifyInfo
	{
		unsigned char direction; //!< Direction of the sender
	};
	
	struct ActivateInfo
	{
		int32_t last_x;
		int32_t last_y;
	};
	
	struct ButtonInfo
	{
		uint32_t number;
	};
	
	struct KeyInfo
	{
		uint32_t keycode;
	};
	
	struct Packet
	{
		enum Type
		{
			T_IDENTIFY,
			T_ACTIVATE,
			T_MOTION,
			T_BUTTONPRESS,
			T_BUTTONRELEASE,
			T_KEYPRESS,
			T_KEYRELEASE
		};
		
		unsigned char type;
		
		union
		{
			MotionInfo motion;
			IdentifyInfo identify;
			ActivateInfo activate;
			ButtonInfo button;
			KeyInfo key;
		};
	};
	
	// Shortcuts
	inline Packet motionPacket(int dx, int dy)
	{
		Packet ret;
		ret.type = Packet::T_MOTION;
		ret.motion.dx = dx;
		ret.motion.dy = dy;
		
		return ret;
	}
	
	inline Packet identifyPacket(unsigned char direction)
	{
		Packet ret;
		ret.type = Packet::T_IDENTIFY;
		ret.identify.direction = direction;
		
		return ret;
	}
	
	inline Packet activatePacket(int last_x, int last_y)
	{
		Packet ret;
		ret.type = Packet::T_ACTIVATE;
		ret.activate.last_x = last_x;
		ret.activate.last_y = last_y;
		
		return ret;
	}
	
	inline Packet buttonEvent(Packet::Type type, uint32_t button)
	{
		Packet ret;
		ret.type = type;
		ret.button.number = button;
		
		return ret;
	}
	
	inline Packet buttonPress(uint32_t button)
	{
		return buttonEvent(Packet::T_BUTTONPRESS, button);
	}
	
	inline Packet buttonRelease(uint32_t button)
	{
		return buttonEvent(Packet::T_BUTTONRELEASE, button);
	}
	
	inline Packet keyEvent(Packet::Type type, uint32_t keycode)
	{
		Packet ret;
		ret.type = type;
		ret.key.keycode = keycode;
		
		return ret;
	}
	
	inline Packet keyPress(uint32_t keycode)
	{
		return keyEvent(Packet::T_KEYPRESS, keycode);
	}
	
	inline Packet keyRelease(uint32_t keycode)
	{
		return keyEvent(Packet::T_KEYRELEASE, keycode);
	}
}

#endif
