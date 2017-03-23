//------------------------------------------------------------------------------
#pragma once
#include <eight/core/noncopyable.h>
#include <eight/core/types.h>
#include <rdestl/vector.h>
#include "../thread/fifo_spsc.h"
namespace eight {
//------------------------------------------------------------------------------

class Scope;
class Timer;
class SingleThread;

namespace MouseInput
{
	enum Type
	{
		Btn1 = 0,
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
	int refresh, adapter;
};

class OsWindow : NonCopyable
{
public:
	typedef void (FnWindowSize  )(void*, int width, int height);
	typedef void (FnMouseButton )(void*, MouseInput::Type, ButtonState::Type);
	typedef void (FnMousePos    )(void*, int x, int y, bool relative);
	typedef void (FnMouseWheel  )(void*, int z);
	typedef bool (FnRequestMouse)(void*);
	typedef void (FnKey         )(void*, Key::Type, ButtonState::Type);
	typedef void (FnChar        )(void*, int character, ButtonState::Type);
	struct Callbacks
	{
		void*            userData;
		FnWindowSize*    windowSize;
		FnMouseButton*   mouseButton;
		FnMousePos*      mousePos;
		FnMouseWheel*    mouseWheel;
		FnRequestMouse*  requestMouse;
		FnKey*           key;
		FnChar*          chars;
	};

	uint Width() const;
	uint Height() const;

	static OsWindow* New( Scope& a, const SingleThread&, const OsWindowArgs&, const Callbacks& );

	void SetCallbackUser( void* );
	bool PollEvents(uint maxMessages=0, const Timer* t=0, float maxTime=0);//returns true if application exit event occurred

	const SingleThread& Thread() const;
	
	void ShowMouseCursor( bool b );
	bool IsMouseCursorShown() const;

	void RestoreWindow();

	void Close();

	void* NativeHandle();
};

template<class T>
struct BindToOsWindow
{
	static void s_WindowSize  (void* o, int width, int height)                   { ((T*)o)->WindowSize(width, height); }
	static void s_MouseButton (void* o, MouseInput::Type b, ButtonState::Type s) { ((T*)o)->MouseButton(b, s); }
	static void s_MousePos    (void* o, int x, int y, bool r)                    { ((T*)o)->MousePos(x, y, r); }
	static void s_MouseWheel  (void* o, int z)                                   { ((T*)o)->MouseWheel(z); }
	static bool s_RequestMouse(void* o)                                          { return ((T*)o)->RequestMouse(); }
	static void s_Key         (void* o, Key::Type k, ButtonState::Type s)        { ((T*)o)->Key(k, s); }
	static void s_Char        (void* o, int character, ButtonState::Type s)      { ((T*)o)->Char(character, s); }
};
template<class T>
OsWindow::Callbacks OsWindowCallbacks(T* user=0)
{
	OsWindow::Callbacks c = 
	{
		user,
		&BindToOsWindow<T>::s_WindowSize,
		&BindToOsWindow<T>::s_MouseButton,
		&BindToOsWindow<T>::s_MousePos,
		&BindToOsWindow<T>::s_MouseWheel,
		&BindToOsWindow<T>::s_RequestMouse,
		&BindToOsWindow<T>::s_Key,
		&BindToOsWindow<T>::s_Char
	};
	return c;
}

class LayeredInputHandler
{
public:
	typedef bool (FnWindowSize  )(void*, int width, int height);
	typedef bool (FnMouseButton )(void*, MouseInput::Type, ButtonState::Type);
	typedef bool (FnMousePos    )(void*, int x, int y, bool relative);
	typedef bool (FnMouseWheel  )(void*, int z);
	typedef bool (FnRequestMouse)(void*);
	typedef bool (FnKey         )(void*, Key::Type, ButtonState::Type);
	typedef bool (FnChar        )(void*, int character, ButtonState::Type);
	struct Callbacks
	{
		bool             active;
		void*            userData;
		FnWindowSize*    windowSize;
		FnMouseButton*   mouseButton;
		FnMousePos*      mousePos;
		FnMouseWheel*    mouseWheel;
		FnRequestMouse*  requestMouse;
		FnKey*           key;
		FnChar*          chars;
	};

	OsWindow::Callbacks Bind();

	void SetActive( void* key_userData, bool value );
	void PushBack( const Callbacks& c ) { m_layers.push_back( c ); }
	void PopBack() { m_layers.pop_back(); }

private:
	friend struct BindToOsWindow<LayeredInputHandler>;
	void WindowSize( int w, int h );
	void MouseButton( MouseInput::Type b, ButtonState::Type s );
	void MousePos( int x, int y, bool relative );
	void MouseWheel( int z );
	bool RequestMouse();
	void Key( Key::Type k, ButtonState::Type s );
	void Char( int character, ButtonState::Type s );

	rde::vector<Callbacks> m_layers;
};

template<class T>
struct BindToLayeredInputHandler
{
	static bool s_WindowSize  (void* o, int width, int height)                   { return ((T*)o)->WindowSize(width, height); }
	static bool s_MouseButton (void* o, MouseInput::Type b, ButtonState::Type s) { return ((T*)o)->MouseButton(b, s); }
	static bool s_MousePos    (void* o, int x, int y, bool r)                    { return ((T*)o)->MousePos(x, y, r); }
	static bool s_MouseWheel  (void* o, int z)                                   { return ((T*)o)->MouseWheel(z); }
	static bool s_RequestMouse(void* o)                                          { return ((T*)o)->RequestMouse(); }
	static bool s_Key         (void* o, Key::Type k, ButtonState::Type s)        { return ((T*)o)->Key(k, s); }
	static bool s_Char        (void* o, int character, ButtonState::Type s)      { return ((T*)o)->Char(character, s); }
};
template<class T>
LayeredInputHandler::Callbacks LayeredInputHandlerCallbacks( T* user=0, bool active=true )
{
	LayeredInputHandler::Callbacks c =
	{
		active,
		user,
		&BindToLayeredInputHandler<T>::s_WindowSize,
		&BindToLayeredInputHandler<T>::s_MouseButton,
		&BindToLayeredInputHandler<T>::s_MousePos,
		&BindToLayeredInputHandler<T>::s_MouseWheel,
		&BindToLayeredInputHandler<T>::s_RequestMouse,
		&BindToLayeredInputHandler<T>::s_Key,
		&BindToLayeredInputHandler<T>::s_Char
	};
	return c;
}


//todo move
inline void LayeredInputHandler::SetActive( void* key_userData, bool value )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( c.userData == key_userData )
		{
			c.active = value;
			break;
		}
	}
}
inline void LayeredInputHandler::WindowSize( int w, int h )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.windowSize && c.windowSize(c.userData, w, h) )
			break;
	}
}
inline void LayeredInputHandler::MouseButton( MouseInput::Type b, ButtonState::Type s )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.mouseButton && c.mouseButton( c.userData, b, s ) )
			break;
	}
}
inline void LayeredInputHandler::MousePos( int x, int y, bool relative )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.mousePos && c.mousePos( c.userData, x, y, relative ) )
			break;
	}
}
inline void LayeredInputHandler::MouseWheel( int z )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.mouseWheel && c.mouseWheel( c.userData, z) )
			break;
	}
}
inline bool LayeredInputHandler::RequestMouse()
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.requestMouse && c.requestMouse( c.userData ) )
			return true;
	}
	return false;
}
inline void LayeredInputHandler::Key( Key::Type k, ButtonState::Type s )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.key && c.key( c.userData, k, s ) )
			break;
	}
}
inline void LayeredInputHandler::Char( int character, ButtonState::Type s )
{
	for( uint i=0, end=m_layers.size(); i!=end; ++i )
	{
		Callbacks& c = m_layers[i];
		if( !c.active )
			continue;
		if( c.chars && c.chars( c.userData, character, s ) )
			break;
	}
}



class OsWindowMessageBuffer
{
public:
	OsWindowMessageBuffer(Scope& a, uint maxMessages) : m_messages(a, maxMessages), m_capacity(maxMessages), m_hack() {}
	uint MaxMessages() const { return m_capacity; }

	//Producer thread only:
	OsWindow::Callbacks Bind();
	void PostQuitMessage();

	//Consumer thread only:
	bool PopTo(OsWindow::Callbacks); // returns true if a quit message has been posted
private:
	friend struct BindToOsWindow<OsWindowMessageBuffer>;
	void WindowSize( int w, int h );
	void MouseButton( MouseInput::Type b, ButtonState::Type s );
	void MousePos( int x, int y, bool relative );
	void MouseWheel( int z );
	void Key( Key::Type k, ButtonState::Type s );
	void Char( int character, ButtonState::Type s );
	bool RequestMouse();

	struct Message
	{
		enum Type
		{
			WindowSize,
			MouseButton,
			MousePos,
			MouseWheel,
			Key,
			Char,
		} type;
		int a,b,c;
	};
	FifoSpsc<Message> m_messages;
	Atomic m_quit;
	uint m_capacity;
	Atomic m_hack;
};

//todo move
inline void OsWindowMessageBuffer::PostQuitMessage()
{
	m_quit = true;
}
inline bool OsWindowMessageBuffer::PopTo(OsWindow::Callbacks who)
{
	Message m;
	while( m_messages.Pop(m) )
	{
		switch(m.type)
		{
		case Message::WindowSize:
			if( who.windowSize )
				who.windowSize(who.userData, m.a, m.b);
			break;
		case Message::MouseButton:
			if( who.mouseButton )
				who.mouseButton(who.userData, (MouseInput::Type)m.a, (ButtonState::Type)m.b);
			break;
		case Message::MousePos:
			if( who.mousePos )
				who.mousePos(who.userData, m.a, m.b, !!m.c);
			break;
		case Message::MouseWheel:
			if( who.mouseWheel )
				who.mouseWheel(who.userData, m.a);
			break;
		case Message::Key:
			if( who.key )
				who.key(who.userData, (Key::Type)m.a, (ButtonState::Type)m.b);
			break;
		case Message::Char:
			if( who.chars )
				who.chars(who.userData, m.a, (ButtonState::Type)m.b);
			break;
		}
	}
	if( who.requestMouse )
		m_hack = who.requestMouse(who.userData) ? 1 : 0;
	return m_quit != 0;
}
inline void OsWindowMessageBuffer::WindowSize( int w, int h )
{
	Message m = { Message::WindowSize, w, h };
	m_messages.Push(m);
}
inline void OsWindowMessageBuffer::MouseButton( MouseInput::Type b, ButtonState::Type s )
{
	Message m = { Message::MouseButton, b, s };
	m_messages.Push(m);
}
inline void OsWindowMessageBuffer::MousePos( int x, int y, bool relative )
{
	Message m = { Message::MousePos, x, y, relative?1:0 };
	m_messages.Push(m);
}
inline void OsWindowMessageBuffer::MouseWheel( int z )
{
	Message m = { Message::MouseWheel, z };
	m_messages.Push(m);
}
inline bool OsWindowMessageBuffer::RequestMouse()
{
	return !!m_hack;
}
inline void OsWindowMessageBuffer::Key( Key::Type k, ButtonState::Type s )
{
	Message m = { Message::Key, k, s };
	m_messages.Push(m);
}
inline void OsWindowMessageBuffer::Char( int character, ButtonState::Type s )
{
	Message m = { Message::Char, character, s };
	m_messages.Push(m);
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
