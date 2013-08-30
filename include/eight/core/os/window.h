//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
namespace eight {
//------------------------------------------------------------------------------

class Scope;
class Timer;
class SingleThread;

namespace MouseInput
{
	enum Type
	{
		Btn1,
		Btn2,
		Btn3,
		Btn4,
		Btn5,
		Btn6,
		Btn7,
		Btn8,
		Last   = Btn8,
		Left   = Btn1,
		Right  = Btn2,
		Middle = Btn3,
	};
}

// Keyboard key definitions: 8-bit ISO-8859-1 (Latin 1) encoding is used
// for printable keys (such as A-Z, 0-9 etc), and values above 256
// represent special (non-printable) keys (e.g. F1, Page Up etc).
namespace Key
{
	enum Type
	{
		Unknown      = -1,
		Space        = 32,
		Special      = 256,
		Esc          = (Special+1),
		F1           = (Special+2),
		F2           = (Special+3),
		F3           = (Special+4),
		F4           = (Special+5),
		F5           = (Special+6),
		F6           = (Special+7),
		F7           = (Special+8),
		F8           = (Special+9),
		F9           = (Special+10),
		F10          = (Special+11),
		F11          = (Special+12),
		F12          = (Special+13),
		F13          = (Special+14),
		F14          = (Special+15),
		F15          = (Special+16),
		F16          = (Special+17),
		F17          = (Special+18),
		F18          = (Special+19),
		F19          = (Special+20),
		F20          = (Special+21),
		F21          = (Special+22),
		F22          = (Special+23),
		F23          = (Special+24),
		F24          = (Special+25),
		F25          = (Special+26),
		Up           = (Special+27),
		Down         = (Special+28),
		Left         = (Special+29),
		Right        = (Special+30),
		LShift       = (Special+31),
		RShift       = (Special+32),
		LCtrl        = (Special+33),
		RCtrl        = (Special+34),
		LAlt         = (Special+35),
		RAlt         = (Special+36),
		Tab          = (Special+37),
		Enter        = (Special+38),
		Backspace    = (Special+39),
		Insert       = (Special+40),
		Del          = (Special+41),
		PageUp       = (Special+42),
		PageDown     = (Special+43),
		Home         = (Special+44),
		End          = (Special+45),
		KP_0         = (Special+46),
		KP_1         = (Special+47),
		KP_2         = (Special+48),
		KP_3         = (Special+49),
		KP_4         = (Special+50),
		KP_5         = (Special+51),
		KP_6         = (Special+52),
		KP_7         = (Special+53),
		KP_8         = (Special+54),
		KP_9         = (Special+55),
		KP_Divide    = (Special+56),
		KP_Multiply  = (Special+57),
		KP_Subtract  = (Special+58),
		KP_Add       = (Special+59),
		KP_Decimal   = (Special+60),
		KP_Equal     = (Special+61),
		KP_Enter     = (Special+62),
		Last         = KP_Enter,
	};
}

namespace ButtonState
{
	enum Type
	{
		Release,
		Press,
	};
}
namespace WindowMode
{
	enum Type
	{
		Window               = 0x00010001,
		FullScreen           = 0x00010002,
	};
}

struct OsWindowArgs
{
	int width, height;
	WindowMode::Type mode;
	const char* title;
};

class OsWindow : NonCopyable
{
public:
	typedef void (FnWindowSize )(void*, int width, int height);
	typedef void (FnMouseButton)(void*, MouseInput::Type, ButtonState::Type);
	typedef void (FnMousePos   )(void*, int x, int y, bool relative);
	typedef void (FnMouseWheel )(void*, int z);
	typedef void (FnKey        )(void*, Key::Type, ButtonState::Type);
	typedef void (FnChar       )(void*, int character, ButtonState::Type);
	struct Callbacks
	{
		void*            userData;
		FnWindowSize*    windowSize;
		FnMouseButton*   mouseButton;
		FnMousePos*      mousePos;
		FnMouseWheel*    mouseWheel;
		FnKey*           key;
		FnChar*          chars;
	};

	uint Width() const;
	uint Height() const;

	static OsWindow* New( Scope& a, const SingleThread&, const OsWindowArgs&, const Callbacks& );

	bool PollEvents(uint maxMessages=0, const Timer* t=0, float maxTime=0);//returns true if application exit event occurred

	const SingleThread& Thread() const;
	
	void ShowMouseCursor(bool b);

	void Close();
};

template<class T>
struct BindToOsWindow
{
	static void s_WindowSize (void* o, int width, int height)                   { ((T*)o)->WindowSize(width, height); }
	static void s_MouseButton(void* o, MouseInput::Type b, ButtonState::Type s) { ((T*)o)->MouseButton(b, s); }
	static void s_MousePos   (void* o, int x, int y, bool r)                    { ((T*)o)->MousePos(x, y, r); }
	static void s_MouseWheel (void* o, int z)                                   { ((T*)o)->MouseWheel(z); }
	static void s_Key        (void* o, Key::Type k, ButtonState::Type s)        { ((T*)o)->Key(k, s); }
	static void s_Char       (void* o, int character, ButtonState::Type s)      { ((T*)o)->Char(character, s); }
};
template<class T>
OsWindow::Callbacks OsWindowCallbacks(T& o)
{
	OsWindow::Callbacks c = 
	{
		&o,
		&BindToOsWindow<T>::s_WindowSize,
		&BindToOsWindow<T>::s_MouseButton,
		&BindToOsWindow<T>::s_MousePos,
		&BindToOsWindow<T>::s_MouseWheel,
		&BindToOsWindow<T>::s_Key,
		&BindToOsWindow<T>::s_Char
	};
	return c;
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------

//========================================================================
// This work is derived from "GLFW - An OpenGL framework".
// GLFW's original copyright notice is reproduced below.
//------------------------------------------------------------------------
// GLFW - An OpenGL framework
// API version: 2.5
// Author:      Marcus Geelnard (marcus.geelnard at home.se)
// WWW:         http://glfw.sourceforge.net
//------------------------------------------------------------------------
// Copyright (c) 2002-2005 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
// Marcus Geelnard
// marcus.geelnard at home.se
//========================================================================
