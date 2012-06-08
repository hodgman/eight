//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
namespace eight {
//------------------------------------------------------------------------------

class Scope;
class Timer;
class ThreadGroup;

class OsWindow : NonCopyable
{
public:
	enum WindowMode
	{
		GLFW_WINDOW               = 0x00010001,
		GLFW_FULLSCREEN           = 0x00010002,
	};

	static OsWindow* New( Scope& a, const ThreadGroup&, int width, int height, WindowMode, const char* title );

	bool PollEvents(uint maxMessages=0, const Timer* t=0, float maxTime=0);//returns true if application exit event occurred

	const ThreadGroup& Thread() const;

	enum ButtonState
	{
		GLFW_RELEASE,
		GLFW_PRESS,
	};

	// Keyboard key definitions: 8-bit ISO-8859-1 (Latin 1) encoding is used
	// for printable keys (such as A-Z, 0-9 etc), and values above 256
	// represent special (non-printable) keys (e.g. F1, Page Up etc).
	enum Key
	{
		GLFW_KEY_UNKNOWN      = -1,
		GLFW_KEY_SPACE        = 32,
		GLFW_KEY_SPECIAL      = 256,
		GLFW_KEY_ESC          = (GLFW_KEY_SPECIAL+1),
		GLFW_KEY_F1           = (GLFW_KEY_SPECIAL+2),
		GLFW_KEY_F2           = (GLFW_KEY_SPECIAL+3),
		GLFW_KEY_F3           = (GLFW_KEY_SPECIAL+4),
		GLFW_KEY_F4           = (GLFW_KEY_SPECIAL+5),
		GLFW_KEY_F5           = (GLFW_KEY_SPECIAL+6),
		GLFW_KEY_F6           = (GLFW_KEY_SPECIAL+7),
		GLFW_KEY_F7           = (GLFW_KEY_SPECIAL+8),
		GLFW_KEY_F8           = (GLFW_KEY_SPECIAL+9),
		GLFW_KEY_F9           = (GLFW_KEY_SPECIAL+10),
		GLFW_KEY_F10          = (GLFW_KEY_SPECIAL+11),
		GLFW_KEY_F11          = (GLFW_KEY_SPECIAL+12),
		GLFW_KEY_F12          = (GLFW_KEY_SPECIAL+13),
		GLFW_KEY_F13          = (GLFW_KEY_SPECIAL+14),
		GLFW_KEY_F14          = (GLFW_KEY_SPECIAL+15),
		GLFW_KEY_F15          = (GLFW_KEY_SPECIAL+16),
		GLFW_KEY_F16          = (GLFW_KEY_SPECIAL+17),
		GLFW_KEY_F17          = (GLFW_KEY_SPECIAL+18),
		GLFW_KEY_F18          = (GLFW_KEY_SPECIAL+19),
		GLFW_KEY_F19          = (GLFW_KEY_SPECIAL+20),
		GLFW_KEY_F20          = (GLFW_KEY_SPECIAL+21),
		GLFW_KEY_F21          = (GLFW_KEY_SPECIAL+22),
		GLFW_KEY_F22          = (GLFW_KEY_SPECIAL+23),
		GLFW_KEY_F23          = (GLFW_KEY_SPECIAL+24),
		GLFW_KEY_F24          = (GLFW_KEY_SPECIAL+25),
		GLFW_KEY_F25          = (GLFW_KEY_SPECIAL+26),
		GLFW_KEY_UP           = (GLFW_KEY_SPECIAL+27),
		GLFW_KEY_DOWN         = (GLFW_KEY_SPECIAL+28),
		GLFW_KEY_LEFT         = (GLFW_KEY_SPECIAL+29),
		GLFW_KEY_RIGHT        = (GLFW_KEY_SPECIAL+30),
		GLFW_KEY_LSHIFT       = (GLFW_KEY_SPECIAL+31),
		GLFW_KEY_RSHIFT       = (GLFW_KEY_SPECIAL+32),
		GLFW_KEY_LCTRL        = (GLFW_KEY_SPECIAL+33),
		GLFW_KEY_RCTRL        = (GLFW_KEY_SPECIAL+34),
		GLFW_KEY_LALT         = (GLFW_KEY_SPECIAL+35),
		GLFW_KEY_RALT         = (GLFW_KEY_SPECIAL+36),
		GLFW_KEY_TAB          = (GLFW_KEY_SPECIAL+37),
		GLFW_KEY_ENTER        = (GLFW_KEY_SPECIAL+38),
		GLFW_KEY_BACKSPACE    = (GLFW_KEY_SPECIAL+39),
		GLFW_KEY_INSERT       = (GLFW_KEY_SPECIAL+40),
		GLFW_KEY_DEL          = (GLFW_KEY_SPECIAL+41),
		GLFW_KEY_PAGEUP       = (GLFW_KEY_SPECIAL+42),
		GLFW_KEY_PAGEDOWN     = (GLFW_KEY_SPECIAL+43),
		GLFW_KEY_HOME         = (GLFW_KEY_SPECIAL+44),
		GLFW_KEY_END          = (GLFW_KEY_SPECIAL+45),
		GLFW_KEY_KP_0         = (GLFW_KEY_SPECIAL+46),
		GLFW_KEY_KP_1         = (GLFW_KEY_SPECIAL+47),
		GLFW_KEY_KP_2         = (GLFW_KEY_SPECIAL+48),
		GLFW_KEY_KP_3         = (GLFW_KEY_SPECIAL+49),
		GLFW_KEY_KP_4         = (GLFW_KEY_SPECIAL+50),
		GLFW_KEY_KP_5         = (GLFW_KEY_SPECIAL+51),
		GLFW_KEY_KP_6         = (GLFW_KEY_SPECIAL+52),
		GLFW_KEY_KP_7         = (GLFW_KEY_SPECIAL+53),
		GLFW_KEY_KP_8         = (GLFW_KEY_SPECIAL+54),
		GLFW_KEY_KP_9         = (GLFW_KEY_SPECIAL+55),
		GLFW_KEY_KP_DIVIDE    = (GLFW_KEY_SPECIAL+56),
		GLFW_KEY_KP_MULTIPLY  = (GLFW_KEY_SPECIAL+57),
		GLFW_KEY_KP_SUBTRACT  = (GLFW_KEY_SPECIAL+58),
		GLFW_KEY_KP_ADD       = (GLFW_KEY_SPECIAL+59),
		GLFW_KEY_KP_DECIMAL   = (GLFW_KEY_SPECIAL+60),
		GLFW_KEY_KP_EQUAL     = (GLFW_KEY_SPECIAL+61),
		GLFW_KEY_KP_ENTER     = (GLFW_KEY_SPECIAL+62),
		GLFW_KEY_LAST         = GLFW_KEY_KP_ENTER,
	};

	enum MouseButton
	{
		GLFW_MOUSE_BUTTON_1,
		GLFW_MOUSE_BUTTON_2,
		GLFW_MOUSE_BUTTON_3,
		GLFW_MOUSE_BUTTON_4,
		GLFW_MOUSE_BUTTON_5,
		GLFW_MOUSE_BUTTON_6,
		GLFW_MOUSE_BUTTON_7,
		GLFW_MOUSE_BUTTON_8,
		GLFW_MOUSE_BUTTON_LAST   = GLFW_MOUSE_BUTTON_8,
		GLFW_MOUSE_BUTTON_LEFT   = GLFW_MOUSE_BUTTON_1,
		GLFW_MOUSE_BUTTON_RIGHT  = GLFW_MOUSE_BUTTON_2,
		GLFW_MOUSE_BUTTON_MIDDLE = GLFW_MOUSE_BUTTON_3,
	};

	enum Joystick
	{
		GLFW_JOYSTICK_1,
		GLFW_JOYSTICK_2,
		GLFW_JOYSTICK_3,
		GLFW_JOYSTICK_4,
		GLFW_JOYSTICK_5,
		GLFW_JOYSTICK_6,
		GLFW_JOYSTICK_7,
		GLFW_JOYSTICK_8,
		GLFW_JOYSTICK_9,
		GLFW_JOYSTICK_10,
		GLFW_JOYSTICK_11,
		GLFW_JOYSTICK_12,
		GLFW_JOYSTICK_13,
		GLFW_JOYSTICK_14,
		GLFW_JOYSTICK_15,
		GLFW_JOYSTICK_16,
		GLFW_JOYSTICK_LAST       = GLFW_JOYSTICK_16,
	};

	enum Token
	{
		GLFW_MOUSE_CURSOR         = 0x00030001,
		GLFW_STICKY_KEYS          = 0x00030002,
		GLFW_STICKY_MOUSE_BUTTONS = 0x00030003,
		GLFW_SYSTEM_KEYS          = 0x00030004,
		GLFW_KEY_REPEAT           = 0x00030005,
		GLFW_AUTO_POLL_EVENTS     = 0x00030006,
		GLFW_PRESENT              = 0x00050001,
		GLFW_AXES                 = 0x00050002,
		GLFW_BUTTONS              = 0x00050003,
	};

	struct VidMode{
		int Width, Height;
	};

	typedef void (*windowsizefun)(int,int);
	typedef bool (*windowclosefun)(void);
	typedef void (*windowrefreshfun)(void);
	typedef void (*mousebuttonfun)(int,int);
	typedef void (*mouseposfun)(int,int);
	typedef void (*mousewheelfun)(int);
	typedef void (*keyfun)(int,int);
	typedef void (*charfun)(int,int);
};

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
