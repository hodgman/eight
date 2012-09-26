#include <eight/core/alloc/new.h>
#include <eight/core/alloc/scope.h>
#include <eight/core/os/window.h>
#include <eight/core/os/win32.h>
#include <eight/core/timer/timer.h>
#include <eight/core/thread/tasksection.h>
#include <ctime>
#include <vector>
#include <mmsystem.h>

using namespace eight;

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


//========================================================================
// Hack: Define things that some <windows.h>'s do not define
//========================================================================

// Some old versions of w32api (used by MinGW and Cygwin) define
// WH_KEYBOARD_LL without typedef:ing KBDLLHOOKSTRUCT (!)
#if defined(__MINGW32__) || defined(__CYGWIN__)
#include <w32api.h>
#if defined(WH_KEYBOARD_LL) && (__W32API_MAJOR_VERSION == 1) && (__W32API_MINOR_VERSION <= 2)
#undef WH_KEYBOARD_LL
#endif
#endif

//------------------------------------------------------------------------
// ** NOTE **  If this gives you compiler errors and you are using MinGW
// (or Dev-C++), update to w32api version 1.3 or later:
// http://sourceforge.net/project/showfiles.php?group_id=2435
//------------------------------------------------------------------------
#ifndef WH_KEYBOARD_LL
#define WH_KEYBOARD_LL 13
typedef struct tagKBDLLHOOKSTRUCT {
  DWORD   vkCode;
  DWORD   scanCode;
  DWORD   flags;
  DWORD   time;
  DWORD   dwExtraInfo;
} KBDLLHOOKSTRUCT, FAR *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;
#endif // WH_KEYBOARD_LL

#ifndef LLKHF_ALTDOWN
#define LLKHF_ALTDOWN  0x00000020
#endif

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 97
#endif
#ifndef SPI_GETANIMATION
#define SPI_GETANIMATION 72
#endif
#ifndef SPI_SETANIMATION
#define SPI_SETANIMATION 73
#endif
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#endif
#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
#endif

#ifndef CDS_FULLSCREEN
#define CDS_FULLSCREEN 4
#endif

#ifndef PFD_GENERIC_ACCELERATED
#define PFD_GENERIC_ACCELERATED 0x00001000
#endif
#ifndef PFD_DEPTH_DONTCARE
#define PFD_DEPTH_DONTCARE 0x20000000
#endif

#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS -1
#endif
#ifndef ENUM_REGISTRY_SETTINGS
#define ENUM_REGISTRY_SETTINGS -2
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif
#ifndef XBUTTON1
#define XBUTTON1 1
#endif
#ifndef XBUTTON2
#define XBUTTON2 2
#endif

// wglSwapIntervalEXT typedef (Win32 buffer-swap interval control)
typedef int (APIENTRY * WGLSWAPINTERVALEXT_T) (int interval);



//========================================================================
// Global variables (GLFW internals)
//========================================================================

//------------------------------------------------------------------------
// Window structure
//------------------------------------------------------------------------
typedef struct _GLFWwin_struct _GLFWwin;

struct _GLFWwin_struct {

// ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // User callback functions
	void*                      UserData;
    OsWindow::FnWindowSize*    WindowSizeCallback;
    OsWindow::FnMouseButton*   MouseButtonCallback;
    OsWindow::FnMousePos*      MousePosCallback;
    OsWindow::FnMouseWheel*    MouseWheelCallback;
    OsWindow::FnKey*           KeyCallback;
    OsWindow::FnChar*          CharCallback;

    // User selected window settings
    bool      Fullscreen;      // Fullscreen flag
    bool      MouseLock;       // Mouse-lock flag
    bool      AutoPollEvents;  // Auto polling flag
    bool      SysKeysDisabled; // System keys disabled flag

    // Window status & parameters
    int       Width, Height;   // Window width and heigth
    int       RefreshRate;     // Vertical monitor refresh rate
    bool      Opened;          // Flag telling if window is opened or not
    bool      Active;          // Application active flag
    bool      Iconified;       // Window iconified flag


// ========= PLATFORM SPECIFIC PART ======================================

    // Platform specific window resources
    HDC       DC;              // Private GDI device context
    HGLRC     RC;              // Permanent rendering context
    HWND      Wnd;             // Window handle
    HINSTANCE Instance;        // Instance of the application
    int       ModeID;          // Mode ID for fullscreen mode
    HHOOK     KeyboardHook;    // Keyboard hook handle
    DWORD     dwStyle;         // Window styles used for window creation
    DWORD     dwExStyle;       // --"--

    // Platform specific extensions (context specific)
    WGLSWAPINTERVALEXT_T SwapInterval;

    // Various platform specific internal variables
    int       OldMouseLock;    // Old mouse-lock flag (used for remembering
                               // mouse-lock state when iconifying)
    int       OldMouseLockValid;
    int       DesiredRefreshRate; // Desired vertical monitor refresh rate

};


//------------------------------------------------------------------------
// User input status (most of this should go in _GLFWwin)
//------------------------------------------------------------------------
struct GlfwInput{

// ========= PLATFORM INDEPENDENT MANDATORY PART =========================

    // Mouse status
    int  MousePosX, MousePosY;
    int  WheelPos;
	char MouseButton[ MouseInput::Last+1 ];

    // Keyboard status
    char Key[ Key::Last+1 ];
    int  LastChar;

    // User selected settings
    int  StickyKeys;
    int  StickyMouseButtons;
    int  KeyRepeat;


// ========= PLATFORM SPECIFIC PART ======================================

    // Platform specific internal variables
    int  MouseMoved, OldMouseX, OldMouseY;

};


//------------------------------------------------------------------------
// Timer status
//------------------------------------------------------------------------
struct GlfwTimer{
    int          HasPerformanceCounter;
    int          HasRDTSC;
    double       Resolution;
    unsigned int t0_32;
    __int64      t0_64;
};



//------------------------------------------------------------------------
// System information
//------------------------------------------------------------------------
struct GlfwSys{
    int     WinVer;
    int     HasUnicode;
    DWORD   ForegroundLockTimeout;
};

//========================================================================
// DLLs that are loaded at glfwInit()
//========================================================================

// gdi32.dll function pointer typedefs
#ifndef _GLFW_NO_DLOAD_GDI32
typedef int  (WINAPI * CHOOSEPIXELFORMAT_T) (HDC,CONST PIXELFORMATDESCRIPTOR*);
typedef int  (WINAPI * DESCRIBEPIXELFORMAT_T) (HDC,int,UINT,LPPIXELFORMATDESCRIPTOR);
typedef int  (WINAPI * GETPIXELFORMAT_T) (HDC);
typedef BOOL (WINAPI * SETPIXELFORMAT_T) (HDC,int,const PIXELFORMATDESCRIPTOR*);
typedef BOOL (WINAPI * SWAPBUFFERS_T) (HDC);
#endif // _GLFW_NO_DLOAD_GDI32

// winmm.dll function pointer typedefs
#ifndef _GLFW_NO_DLOAD_WINMM
typedef MMRESULT (WINAPI * JOYGETDEVCAPSA_T) (UINT,LPJOYCAPSA,UINT);
typedef MMRESULT (WINAPI * JOYGETPOS_T) (UINT,LPJOYINFO);
typedef MMRESULT (WINAPI * JOYGETPOSEX_T) (UINT,LPJOYINFOEX);
typedef DWORD (WINAPI * TIMEGETTIME_T) (void);
#endif // _GLFW_NO_DLOAD_WINMM

// Library handles and function pointers
struct GlfwLibs{
#ifndef _GLFW_NO_DLOAD_GDI32
    // gdi32.dll
    HINSTANCE             gdi32;
    CHOOSEPIXELFORMAT_T   ChoosePixelFormat;
    DESCRIBEPIXELFORMAT_T DescribePixelFormat;
    GETPIXELFORMAT_T      GetPixelFormat;
    SETPIXELFORMAT_T      SetPixelFormat;
    SWAPBUFFERS_T         SwapBuffers;
#endif // _GLFW_NO_DLOAD_GDI32

    // winmm.dll
#ifndef _GLFW_NO_DLOAD_WINMM
    HINSTANCE             winmm;
    JOYGETDEVCAPSA_T      joyGetDevCapsA;
    JOYGETPOS_T           joyGetPos;
    JOYGETPOSEX_T         joyGetPosEx;
    TIMEGETTIME_T         timeGetTime;
#endif // _GLFW_NO_DLOAD_WINMM
};

// gdi32.dll shortcuts
#ifndef _GLFW_NO_DLOAD_GDI32
#define _glfw_ChoosePixelFormat   _glfwLibs.ChoosePixelFormat
#define _glfw_DescribePixelFormat _glfwLibs.DescribePixelFormat
#define _glfw_GetPixelFormat      _glfwLibs.GetPixelFormat
#define _glfw_SetPixelFormat      _glfwLibs.SetPixelFormat
#define _glfw_SwapBuffers         _glfwLibs.SwapBuffers
#else
#define _glfw_ChoosePixelFormat   ChoosePixelFormat
#define _glfw_DescribePixelFormat DescribePixelFormat
#define _glfw_GetPixelFormat      GetPixelFormat
#define _glfw_SetPixelFormat      SetPixelFormat
#define _glfw_SwapBuffers         SwapBuffers
#endif // _GLFW_NO_DLOAD_GDI32

// winmm.dll shortcuts
#ifndef _GLFW_NO_DLOAD_WINMM
#define _glfw_joyGetDevCaps _glfwLibs.joyGetDevCapsA
#define _glfw_joyGetPos     _glfwLibs.joyGetPos
#define _glfw_joyGetPosEx   _glfwLibs.joyGetPosEx
#define _glfw_timeGetTime   _glfwLibs.timeGetTime
#else
#define _glfw_joyGetDevCaps joyGetDevCapsA
#define _glfw_joyGetPos     joyGetPos
#define _glfw_joyGetPosEx   joyGetPosEx
#define _glfw_timeGetTime   timeGetTime
#endif // _GLFW_NO_DLOAD_WINMM




//========================================================================
//------------------------------------------------------------------------
//========================================================================
class OsWindowWin32 : public OsWindow
{
public:
	OsWindowWin32( const ThreadGroup& );
	~OsWindowWin32();
	const ThreadGroup& Thread() const { return m_thread; }
	void ClearInput( void );
	void InputKey( int key, int action );
	void InputChar( int character, int action );
	void InputMouseClick( int button, int action );
	bool OpenWindow( int width, int height, int mode, const char* title, const Callbacks& );
	bool IsWindowOpen() { return _glfwWin.Opened; }
	void CloseWindow();
	void SetWindowTitle( const char *title );
	void GetWindowSize( int *width, int *height );
	void SetWindowSize( int width, int height );
	void SetWindowPos( int x, int y );
	void IconifyWindow();
	void RestoreWindow();

	bool PollEvents(uint maxMessages, const Timer* t, float maxTime);
	//void WaitEvents();
	void ShowMouseCursor( bool );

	void DestroyWindow();
	int  TranslateKey( DWORD wParam, DWORD lParam );
	void TranslateChar( DWORD wParam, DWORD lParam, int action );

	static LRESULT CALLBACK s_WindowCallback( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	LRESULT WindowCallback( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

	ThreadGroup m_thread;
	GlfwLibs _glfwLibs;
	GlfwSys _glfwSys;
	GlfwTimer _glfwTimer;
	GlfwInput _glfwInput;
	_GLFWwin _glfwWin;
};

HWND GetHwnd(const OsWindow& wnd ) { return ((OsWindowWin32&)wnd)._glfwWin.Wnd; }

OsWindowWin32::OsWindowWin32( const ThreadGroup& t ) : m_thread(t)
{
	memset( &_glfwLibs, 0, sizeof(GlfwLibs) );
	memset( &_glfwSys, 0, sizeof(GlfwSys) );
	memset( &_glfwTimer, 0, sizeof(GlfwTimer) );
	memset( &_glfwInput, 0, sizeof(GlfwInput) );
	memset( &_glfwWin, 0, sizeof(_GLFWwin) );
}

OsWindowWin32::~OsWindowWin32()
{
	if( IsWindowOpen() )
		CloseWindow();
}

OsWindow* OsWindow::New( Scope& a, const ThreadGroup& thread, int width, int height, WindowMode::Type mode, const char* title, const Callbacks& c )
{
	eiASSERT( thread.Current() );
	OsWindowWin32* win = eiNew( a, OsWindowWin32 )( thread );
	bool ok = win->OpenWindow( width, height, mode, title, c );
	eiASSERT( ok );
	return win;
}

      OsWindowWin32& Downcast(      OsWindow*p) { return *(OsWindowWin32*)p; }
const OsWindowWin32& Downcast(const OsWindow*p) { return *(OsWindowWin32*)p; }

bool OsWindow::PollEvents(uint maxMessages, const Timer* t, float maxTime)
{
	return Downcast(this).PollEvents(maxMessages, t, maxTime);
}
const ThreadGroup& OsWindow::Thread() const
{
	return Downcast(this).Thread();
}

//========================================================================
//------------------------------------------------------------------------
//========================================================================


//========================================================================
// _glfwClearInput() - Clear all input state
//========================================================================

void OsWindowWin32::ClearInput( void )
{
    int i;

    // Release all keyboard keys
    for( i = 0; i <= Key::Last; i ++ )
    {
        _glfwInput.Key[ i ] = 0;
    }

    // Clear last character
    _glfwInput.LastChar = 0;

    // Release all mouse buttons
    for( i = 0; i <= MouseInput::Last; i ++ )
    {
        _glfwInput.MouseButton[ i ] = 0;
    }

    // Set mouse position to (0,0)
    _glfwInput.MousePosX = 0;
    _glfwInput.MousePosY = 0;

    // Set mouse wheel position to 0
    _glfwInput.WheelPos = 0;

    // The default is to use non sticky keys and mouse buttons
    _glfwInput.StickyKeys = false;
    _glfwInput.StickyMouseButtons = false;

    // The default is to disable key repeat
    //_glfwInput.KeyRepeat = false;
    _glfwInput.KeyRepeat = true;
}


//========================================================================
// InputKey() - Register keyboard activity
//========================================================================

void OsWindowWin32::InputKey( int key, int action )
{
    int keyrepeat = 0;

    if( key >= 0 && key <= Key::Last )
    {
        // Are we trying to release an already released key?
        if( action == ButtonState::Release && _glfwInput.Key[ key ] != 1 )
        {
            return;
        }

        // Register key action
        if( action == ButtonState::Release && _glfwInput.StickyKeys )
        {
            _glfwInput.Key[ key ] = 2;
        }
        else
        {
            keyrepeat = (_glfwInput.Key[ key ] == ButtonState::Press) &&
                        (action == ButtonState::Press);
            _glfwInput.Key[ key ] = (char) action;
        }

        // Call user callback function
        if( _glfwWin.KeyCallback && (_glfwInput.KeyRepeat || !keyrepeat) )
        {
            _glfwWin.KeyCallback( _glfwWin.UserData, (Key::Type)key, (ButtonState::Type)action );
        }
    }
}


//========================================================================
// _glfwInputChar() - Register (keyboard) character activity
//========================================================================

void OsWindowWin32::InputChar( int character, int action )
{
    int keyrepeat = 0;

    // Valid Unicode (ISO 10646) character?
    if( !( (character >= 32 && character <= 126) || character >= 160 ) )
    {
        return;
    }

    // Is this a key repeat?
    if( action == ButtonState::Press && _glfwInput.LastChar == character )
    {
        keyrepeat = 1;
    }

    // Store this character as last character (or clear it, if released)
    if( action == ButtonState::Press )
    {
        _glfwInput.LastChar = character;
    }
    else
    {
        _glfwInput.LastChar = 0;
    }

    // Call user callback function
    if( _glfwWin.CharCallback && (_glfwInput.KeyRepeat || !keyrepeat) )
    {
        _glfwWin.CharCallback( _glfwWin.UserData, character, (ButtonState::Type)action );
    }
}


//========================================================================
// _glfwInputMouseClick() - Register mouse button clicks
//========================================================================

void OsWindowWin32::InputMouseClick( int button, int action )
{
    if( button >= 0 && button <= MouseInput::Last )
    {
        // Register mouse button action
        if( action == ButtonState::Release && _glfwInput.StickyMouseButtons )
        {
            _glfwInput.MouseButton[ button ] = 2;
        }
        else
        {
            _glfwInput.MouseButton[ button ] = (char) action;
        }

        // Call user callback function
        if( _glfwWin.MouseButtonCallback )
        {
            _glfwWin.MouseButtonCallback( _glfwWin.UserData, (MouseInput::Type)button, (ButtonState::Type)action );
        }
    }
}



static void _glfwGetFullWindowSize( DWORD dwStyle, DWORD dwExStyle, int w, int h, int *w2, int *h2 )
{
    RECT rect;

    // Create a window rectangle
    rect.left   = (long)0;
    rect.right  = (long)w-1;
    rect.top    = (long)0;
    rect.bottom = (long)h-1;

    // Adjust according to window styles
    AdjustWindowRectEx( &rect, dwStyle, FALSE, dwExStyle );

    // Calculate width and height of full window
    *w2 = rect.right-rect.left+1;
    *h2 = rect.bottom-rect.top+1;
}

//========================================================================
// _glfwMinMaxAnimations() - Enable/disable minimize/restore animations
//========================================================================

static int _glfwMinMaxAnimations( int enable )
{
    ANIMATIONINFO AI;
    int old_enable;

    // Get old animation setting
    AI.cbSize = sizeof( ANIMATIONINFO );
    SystemParametersInfo( SPI_GETANIMATION, AI.cbSize, &AI, 0 );
    old_enable = AI.iMinAnimate;

    // If requested, change setting
    if( old_enable != enable )
    {
        AI.iMinAnimate = enable;
        SystemParametersInfo( SPI_SETANIMATION, AI.cbSize, &AI,
                              SPIF_SENDCHANGE );
    }

    return old_enable;
}

//========================================================================
// _glfwSetForegroundWindow() - Function for bringing a window into focus
// and placing it on top of the window z stack. Due to some nastiness
// with how Win98/ME/2k/XP handles SetForegroundWindow, we have to go
// through some really bizarre measures to achieve this (thanks again, MS,
// for making life so much easier)!
//========================================================================

static void _glfwSetForegroundWindow( HWND hWnd )
{
    int try_count = 0;
    int old_animate;

    // Try the standard approach first...
    BringWindowToTop( hWnd );
    SetForegroundWindow( hWnd );

    // If it worked, return now
    if( hWnd == GetForegroundWindow() )
    {
        // Try to modify the system settings (since this is the foreground
        // process, we are allowed to do this)
        SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)0,
                              SPIF_SENDCHANGE );
        return;
    }

    // For other Windows versions than 95 & NT4.0, the standard approach
    // may not work, so if we failed we have to "trick" Windows into
    // making our window the foureground window: Iconify and restore
    // again. It is ugly, but it seems to work (we turn off those annoying
    // zoom animations to make it look a bit better at least).

    // Turn off minimize/restore animations
    old_animate = _glfwMinMaxAnimations( 0 );

    // We try this a few times, just to be on the safe side of things...
    do
    {
        // Iconify & restore
        ShowWindow( hWnd, SW_HIDE );
        ShowWindow( hWnd, SW_SHOWMINIMIZED );
        ShowWindow( hWnd, SW_SHOWNORMAL );

        // Try to get focus
        BringWindowToTop( hWnd );
        SetForegroundWindow( hWnd );

        try_count ++;
    }
    while( hWnd != GetForegroundWindow() && try_count <= 3 );

    // Restore the system minimize/restore animation setting
    (void) _glfwMinMaxAnimations( old_animate );

    // Try to modify the system settings (since this is now hopefully the
    // foreground process, we are probably allowed to do this)
    SystemParametersInfo( SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)0, SPIF_SENDCHANGE );
}

static void _glfwPlatformRefreshWindowParams( _GLFWwin& _glfwWin )
{
    // Get refresh rate
    DWORD dmode = _glfwWin.Fullscreen ? _glfwWin.ModeID : ENUM_CURRENT_SETTINGS;
	DEVMODE dm = {};
    dm.dmSize = sizeof( DEVMODE );
    int success = EnumDisplaySettings( NULL, dmode, &dm );
    if( success )
    {
        _glfwWin.RefreshRate = dm.dmDisplayFrequency;
        if( _glfwWin.RefreshRate <= 1 )
            _glfwWin.RefreshRate = 0;
    }
    else
        _glfwWin.RefreshRate = 0;
}

//************************************************************************
//****                    GLFW user functions                         ****
//************************************************************************

//========================================================================
// glfwOpenWindow() - Here is where the window is created, and the OpenGL
// rendering context is created
//========================================================================

static void* g_window;//todo
bool OsWindowWin32::OpenWindow( int width, int height, int mode, const char* title, const Callbacks& c )
{
	eiASSERT( !_glfwWin.Opened );
	eiASSERT( mode == WindowMode::Window || mode == WindowMode::FullScreen );

    // Clear GLFW window state
    _glfwWin.Active            = true;
    _glfwWin.Iconified         = false;
    _glfwWin.MouseLock         = false;
    _glfwWin.AutoPollEvents    = true;
    ClearInput();

    // Unregister all callback functions
    _glfwWin.UserData              = c.userData;
    _glfwWin.WindowSizeCallback    = c.windowSize;
    _glfwWin.MouseButtonCallback   = c.mouseButton;
    _glfwWin.MousePosCallback      = c.mousePos;
    _glfwWin.MouseWheelCallback    = c.mouseWheel;
    _glfwWin.KeyCallback           = c.key;
    _glfwWin.CharCallback          = c.chars;

    eiASSERT( width > 0 && height > 0 );

    // Remeber window settings
    _glfwWin.Width      = width;
    _glfwWin.Height     = height;
    _glfwWin.Fullscreen = (mode == WindowMode::FullScreen ? 1 : 0);

    // Clear platform specific GLFW window state
    _glfwWin.DC                = NULL;
    _glfwWin.RC                = NULL;
    _glfwWin.Wnd               = NULL;
    _glfwWin.Instance          = NULL;
    _glfwWin.OldMouseLockValid = false;

    // Remember desired refresh rate for this window (used only in fullscreen mode)
    _glfwWin.DesiredRefreshRate = 60;

    // Grab an instance for our window
    _glfwWin.Instance = GetModuleHandle( NULL );

    // Set window class parameters
    WNDCLASS    wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)s_WindowCallback;
    wc.cbClsExtra    = 0;
	eiSTATIC_ASSERT(sizeof(void*)<=sizeof(int));
	                 g_window = this;
	wc.cbWndExtra    = 0;//(int)this;
    wc.hInstance     = _glfwWin.Instance;
    wc.hIcon         = LoadIcon( NULL, IDI_WINLOGO );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "GLFW";

    // Register the window class
    if( !RegisterClass( &wc ) )
    {
        _glfwWin.Instance = NULL;
        DestroyWindow();
        return false;
    }

    // Set common window styles
    DWORD dwStyle   = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
    DWORD dwExStyle = WS_EX_APPWINDOW;

    // Set window style, depending on fullscreen mode
    if( _glfwWin.Fullscreen )
    {
        dwStyle   |= WS_POPUP;
    }
    else
    {
        dwStyle   |= WS_OVERLAPPEDWINDOW;
        dwExStyle |= WS_EX_WINDOWEDGE;
    }

    // Remember window styles
    _glfwWin.dwStyle   = dwStyle;
    _glfwWin.dwExStyle = dwExStyle;

    // Set window size to true requested size (adjust for window borders)
	int full_width, full_height;
    _glfwGetFullWindowSize( dwStyle, dwExStyle, _glfwWin.Width, _glfwWin.Height, &full_width, &full_height );

   // Adjust window position to working area (e.g. if the task bar is at
   // the top of the display). Fullscreen windows are always opened in
   // the upper left corner regardless of the desktop working area. 
    RECT wa;
    if( _glfwWin.Fullscreen )
        wa.left = wa.top = 0;
    else
        SystemParametersInfo( SPI_GETWORKAREA, 0, &wa, 0 );

    // Create window
    _glfwWin.Wnd = CreateWindowEx(
               dwExStyle,                 // Extended style
               "GLFW",                    // Class name
               title,                     // Window title
               dwStyle,                   // Defined window style
               wa.left, wa.top,           // Window position
               full_width,                // Decorated window width
               full_height,               // Decorated window height
               NULL,                      // No parent window
               NULL,                      // No menu
               _glfwWin.Instance,         // Instance
               NULL );                    // Nothing to WM_CREATE
    if( !_glfwWin.Wnd )
    {
		DWORD error = GetLastError();
        DestroyWindow();
        return false;
    }

    // Get a device context
    _glfwWin.DC = GetDC( _glfwWin.Wnd );
    if( !_glfwWin.DC )
    {
        DestroyWindow();
        return false;
    }

    // Make sure that our window ends up on top of things
    if( _glfwWin.Fullscreen )
    {
        // Place the window above all topmost windows
		::SetWindowPos( _glfwWin.Wnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE );
    }
    _glfwSetForegroundWindow( _glfwWin.Wnd );
    SetFocus( _glfwWin.Wnd );
//!!!

    // Flag that window is now opened
    _glfwWin.Opened = true;

	_glfwPlatformRefreshWindowParams( _glfwWin );

    // If full-screen mode was requested, disable mouse cursor
    if( _glfwWin.Fullscreen )
        ShowMouseCursor( false );

    return true;
}


//========================================================================
// glfwCloseWindow() - Properly kill the window / video display
//========================================================================

void OsWindowWin32::CloseWindow()
{
	eiASSERT( IsWindowOpen() );

    ShowMouseCursor( true );

	DestroyWindow();

    _glfwWin.Opened = false;
}

void OsWindowWin32::DestroyWindow()
{
    if( _glfwWin.DC )
    {
        ReleaseDC( _glfwWin.Wnd, _glfwWin.DC );
        _glfwWin.DC = NULL;
    }

    if( _glfwWin.Wnd )
    {
		::DestroyWindow( _glfwWin.Wnd );
        _glfwWin.Wnd = NULL;
    }

    if( _glfwWin.Instance )
    {
        UnregisterClass( "GLFW", _glfwWin.Instance );
        _glfwWin.Instance = NULL;
    }

    if( _glfwWin.Fullscreen )
    {
        ChangeDisplaySettings( NULL, CDS_FULLSCREEN );
    }
}


//========================================================================
// glfwSetWindowTitle() - Set the window title
//========================================================================

void OsWindowWin32::SetWindowTitle( const char *title )
{
	eiASSERT( IsWindowOpen() );
    SetWindowText( _glfwWin.Wnd, title );
}


//========================================================================
// glfwGetWindowSize() - Get the window size
//========================================================================

void OsWindowWin32::GetWindowSize( int *width, int *height )
{
    if( width != NULL )
        *width = _glfwWin.Width;
    if( height != NULL )
        *height = _glfwWin.Height;
}


int _glfwGetClosestVideoModeBPP( int *w, int *h, int *bpp, int *refresh )
{
    int     mode, bestmode, match, bestmatch, rr, bestrr, success;
    DEVMODE dm;

    // Find best match
    bestmatch = 0x7fffffff;
    bestrr    = 0x7fffffff;
    mode = bestmode = 0;
    do
    {
        dm.dmSize = sizeof( DEVMODE );
        success = EnumDisplaySettings( NULL, mode, &dm );
        if( success )
        {
            match = dm.dmBitsPerPel - *bpp;
            if( match < 0 ) match = -match;
            match = ( match << 25 ) |
                    ( (dm.dmPelsWidth - *w) *
                      (dm.dmPelsWidth - *w) +
                      (dm.dmPelsHeight - *h) *
                      (dm.dmPelsHeight - *h) );
            if( match < bestmatch )
            {
                bestmatch = match;
                bestmode  = mode;
                bestrr = (dm.dmDisplayFrequency - *refresh) *
                         (dm.dmDisplayFrequency - *refresh);
            }
            else if( match == bestmatch && *refresh > 0 )
            {
                rr = (dm.dmDisplayFrequency - *refresh) *
                     (dm.dmDisplayFrequency - *refresh);
                if( rr < bestrr )
                {
                    bestmatch = match;
                    bestmode  = mode;
                    bestrr    = rr;
                }
            }
        }
        mode ++;
    }
    while( success );

    // Get the parameters for the best matching display mode
    dm.dmSize = sizeof( DEVMODE );
    (void) EnumDisplaySettings( NULL, bestmode, &dm );

    // Fill out actual width and height
    *w = dm.dmPelsWidth;
    *h = dm.dmPelsHeight;

    // Return bits per pixel
    *bpp = dm.dmBitsPerPel;

    // Return vertical refresh rate
    *refresh = dm.dmDisplayFrequency;

    return bestmode;
}

void _glfwSetVideoModeMODE( int mode, _GLFWwin& _glfwWin )
{
    // Get the parameters for the best matching display mode
	DEVMODE dm = {};
    dm.dmSize = sizeof( DEVMODE );
    (void) EnumDisplaySettings( NULL, mode, &dm );

    // Set which fields we want to specify
    dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

    if( _glfwWin.DesiredRefreshRate > 0 )// Do we have a prefered refresh rate?
        dm.dmFields = dm.dmFields | DM_DISPLAYFREQUENCY;

    // Change display setting
    dm.dmSize = sizeof( DEVMODE );
    int success = ChangeDisplaySettings( &dm, CDS_FULLSCREEN );

    // If the mode change was not possible, query the current display
    // settings (we'll use the desktop resolution for fullscreen mode)
    if( success == DISP_CHANGE_SUCCESSFUL )
        _glfwWin.ModeID = mode;
    else
    {
        _glfwWin.ModeID = ENUM_REGISTRY_SETTINGS;
        EnumDisplaySettings( NULL, ENUM_REGISTRY_SETTINGS, &dm );
    }

    // Set the window size to that of the display mode
    _glfwWin.Width  = dm.dmPelsWidth;
    _glfwWin.Height = dm.dmPelsHeight;
}

void OsWindowWin32::SetWindowSize( int width, int height )
{
	eiASSERT( IsWindowOpen() );

    if( _glfwWin.Iconified )
        return;

    // Don't do anything if the window size did not change
    if( width == _glfwWin.Width && height == _glfwWin.Height )
        return;

    int  bpp, mode = 0, refresh;
    bool sizechanged = false;

    // If we are in fullscreen mode, get some info about the current mode
    if( _glfwWin.Fullscreen )
    {
        DEVMODE dm;
        int success;
        // Get current BPP settings
        dm.dmSize = sizeof( DEVMODE );
        success = EnumDisplaySettings( NULL, _glfwWin.ModeID, &dm );
        if( success )
        {
            bpp = dm.dmBitsPerPel;
            // Get closest match for target video mode
            refresh = _glfwWin.DesiredRefreshRate;
            mode = _glfwGetClosestVideoModeBPP( &width, &height, &bpp, &refresh );
        }
        else
            mode = _glfwWin.ModeID;
    }
    else
    {
        // If we are in windowed mode, adjust the window size to compensate for window decorations
        _glfwGetFullWindowSize( _glfwWin.dwStyle, _glfwWin.dwExStyle, width, height, &width, &height );
    }

    // Change window size before changing fullscreen mode?
    if( _glfwWin.Fullscreen && (width > _glfwWin.Width) )
    {
		::SetWindowPos( _glfwWin.Wnd, HWND_TOP, 0, 0, width, height,
                        SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER );
        sizechanged = true;
    }

    // Change fullscreen video mode?
    if( _glfwWin.Fullscreen && mode != _glfwWin.ModeID )
        _glfwSetVideoModeMODE( mode, _glfwWin );

    if( !sizechanged )
    {
		::SetWindowPos( _glfwWin.Wnd, HWND_TOP, 0, 0, width, height,
                        SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER );
    }

    _glfwPlatformRefreshWindowParams( _glfwWin );
}


//========================================================================
// glfwSetWindowPos() - Set the window position
//========================================================================

void OsWindowWin32::SetWindowPos( int x, int y )
{
	eiASSERT( IsWindowOpen() );

    if( _glfwWin.Fullscreen || _glfwWin.Iconified )
        return;
	
	::SetWindowPos( _glfwWin.Wnd, HWND_TOP, x, y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER );
}


//========================================================================
// glfwIconfyWindow() - Window iconification
//========================================================================

void OsWindowWin32::IconifyWindow()
{
	eiASSERT( IsWindowOpen() );

    if( _glfwWin.Iconified )
        return;

    // Iconify window
	::CloseWindow( _glfwWin.Wnd );

    // Window is now iconified
    _glfwWin.Iconified = true;

    // If we are in fullscreen mode we need to change video modes
    if( _glfwWin.Fullscreen ) // Change display settings to the desktop resolution
        ChangeDisplaySettings( NULL, CDS_FULLSCREEN );

    // Unlock mouse
    if( !_glfwWin.OldMouseLockValid )
    {
        _glfwWin.OldMouseLock = _glfwWin.MouseLock;
        _glfwWin.OldMouseLockValid = true;
		ShowMouseCursor( true );
    }
}


//========================================================================
// glfwRestoreWindow() - Window un-iconification
//========================================================================

void OsWindowWin32::RestoreWindow()
{
	eiASSERT( IsWindowOpen() );

    if( !_glfwWin.Iconified )
        return;

    // If we are in fullscreen mode we need to change video modes
    if( _glfwWin.Fullscreen )
    {
        // Change display settings to the user selected mode
        _glfwSetVideoModeMODE( _glfwWin.ModeID, _glfwWin );
    }

    // Un-iconify window
    OpenIcon( _glfwWin.Wnd );

    // Make sure that our window ends up on top of things
    ShowWindow( _glfwWin.Wnd, SW_SHOW );
    _glfwSetForegroundWindow( _glfwWin.Wnd );
    SetFocus( _glfwWin.Wnd );

    // Window is no longer iconified
    _glfwWin.Iconified = false;

    // Lock mouse, if necessary
    if( _glfwWin.OldMouseLockValid && _glfwWin.OldMouseLock )
        ShowMouseCursor( false );
    _glfwWin.OldMouseLockValid = false;

    // Refresh window parameters
    _glfwPlatformRefreshWindowParams( _glfwWin );
}

//========================================================================
// glfwPollEvents() - Poll for new window and input events
//========================================================================

eiInfoGroup(OsWindow, false);

bool OsWindowWin32::PollEvents(uint maxMessages, const Timer* timer, float maxTime)
{
	eiASSERT( IsWindowOpen() );

    bool gotQuitMessage = false;

    // Flag: mouse was not moved (will be changed by _glfwGetNextEvent if there was a mouse move event)
    _glfwInput.MouseMoved = false;
    if( _glfwWin.MouseLock )
    {
        _glfwInput.OldMouseX = _glfwWin.Width>>1;
        _glfwInput.OldMouseY = _glfwWin.Height>>1;
    }
    else
    {
        _glfwInput.OldMouseX = _glfwInput.MousePosX;
        _glfwInput.OldMouseY = _glfwInput.MousePosY;
    }

    // Check for new window messages
	MSG msg;
	uint msgCount = 0;
	double startTime = timer ? timer->Elapsed() : 0;
	double endTime = startTime+maxTime;
	eiASSERT( !timer || maxTime > 0 );
    while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
		double elapsed1 = timer->Elapsed();
        if( msg.message == WM_QUIT )
            gotQuitMessage = true;
		else
            DispatchMessage( &msg );

		//eiInfo(OsWindow, "msg.wParam == %d", msg.wParam);

		msgCount++;
		if( maxMessages && msgCount >= maxMessages )
		{
			eiInfo(OsWindow, "Hit max messages/frame count (%d)", msgCount);
			break;
		}
		double elapsed2 = timer->Elapsed();
		if( timer && elapsed2 >= endTime )
		{
			eiInfo(OsWindow, "Hit max messages/frame time-out (%.2fms, %.2fms)", (elapsed2-startTime)*1000, (elapsed2-elapsed1)*1000);
			break;
		}
    }
	if( msgCount )
	{
		if(timer)
			eiInfo(OsWindow, "Processed %d messages in %.2fms", msgCount, (timer->Elapsed()-startTime)*1000);
		else
			eiInfo(OsWindow, "Processed %d messages", msgCount);
	}

    // LSHIFT/RSHIFT fixup (keys tend to "stick" without this fix)
    // This is the only async event handling in GLFW, but it solves some nasty problems.
    // Get current state of left and right shift keys
    int lshift_down = (GetAsyncKeyState( VK_LSHIFT ) >> 15) & 1;
    int rshift_down = (GetAsyncKeyState( VK_RSHIFT ) >> 15) & 1;
    // See if this differs from our belief of what has happened (we only have to check for lost key up events)
    if( !lshift_down && _glfwInput.Key[ Key::LShift ] == 1 )
        InputKey( Key::LShift, ButtonState::Release );
    if( !rshift_down && _glfwInput.Key[ Key::RShift ] == 1 )
        InputKey( Key::RShift, ButtonState::Release );

    // Did we have mouse movement in locked cursor mode?
    if( _glfwInput.MouseMoved && _glfwWin.MouseLock )
	{
		POINT pos = { _glfwWin.Width>>1, _glfwWin.Height>>1 };
	    ClientToScreen( _glfwWin.Wnd, &pos );
	    SetCursorPos( pos.x, pos.y );
	}

//    if( winclosed && _glfwWin.WindowCloseCallback )// Was there a window close request?
//        winclosed = _glfwWin.WindowCloseCallback();// Check if the program wants us to close the window
    
    return gotQuitMessage;
}


//========================================================================
// glfwWaitEvents() - Wait for new window and input events
//========================================================================
/*
void OsWindowWin32::WaitEvents()
{
	eiASSERT( IsWindowOpen() );
	::WaitMessage();
    PollEvents();
}*/


void OsWindowWin32::ShowMouseCursor( bool enable )
{
	eiASSERT( IsWindowOpen() );

	if( enable )
	{
		ReleaseCapture();// Un-capture cursor
		ShowCursor( TRUE );
		_glfwWin.MouseLock = false;// From now on the mouse is unlocked
	}
	else
	{
		ShowCursor( FALSE );
		SetCapture( _glfwWin.Wnd );// Capture cursor to user window
		// Move cursor to the middle of the window
		POINT pos = { _glfwWin.Width>>1, _glfwWin.Height>>1 };
		ClientToScreen( _glfwWin.Wnd, &pos );
		SetCursorPos( pos.x, pos.y );
		_glfwWin.MouseLock = true;// From now on the mouse is locked
	}
}




//========================================================================
// _glfwWindowCallback() - Window callback function (handles window
// events)
//========================================================================

LRESULT CALLBACK OsWindowWin32::s_WindowCallback( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//	int extra = GetClassLong( hWnd, GCL_CBWNDEXTRA );
	OsWindowWin32* self = reinterpret_cast<OsWindowWin32*>(g_window);//extra);
	return self->WindowCallback( hWnd, uMsg, wParam, lParam );
}

LRESULT OsWindowWin32::WindowCallback( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    int WheelDelta;
	bool Iconified;

	//eiInfo( OsWindow, "uMsg == %d", uMsg );

    // Handle certain window messages
    switch( uMsg )
    {
        // Window activate message? (iconification?)
        case WM_ACTIVATE:
            _glfwWin.Active = LOWORD(wParam) != WA_INACTIVE ? true :
                                                              false;
            Iconified = HIWORD(wParam) ? true : false;

            // Were we deactivated/iconified?
            if( (!_glfwWin.Active || Iconified) && !_glfwWin.Iconified )
            {
                // If we are in fullscreen mode we need to iconify
                if( _glfwWin.Fullscreen )
                {
                    // Do we need to manually iconify?
                    if( !Iconified )
                    {
                        // Minimize window
						::CloseWindow( _glfwWin.Wnd );

                        // The window is now iconified
                        Iconified = true;
                    }

                    // Change display settings to the desktop resolution
                    ChangeDisplaySettings( NULL, CDS_FULLSCREEN );
                }

                // Unlock mouse
                if( !_glfwWin.OldMouseLockValid )
                {
                    _glfwWin.OldMouseLock = _glfwWin.MouseLock;
                    _glfwWin.OldMouseLockValid = true;
                    ShowMouseCursor( true );
                }
            }
            else if( _glfwWin.Active || !Iconified )
            {
                // If we are in fullscreen mode we need to maximize
                if( _glfwWin.Fullscreen && _glfwWin.Iconified )
                {
                    // Change display settings to the user selected mode
                    _glfwSetVideoModeMODE( _glfwWin.ModeID, _glfwWin );

                    // Do we need to manually restore window?
                    if( Iconified )
                    {
                        // Restore window
                        OpenIcon( _glfwWin.Wnd );

                        // The window is no longer iconified
                        Iconified = false;

                        // Activate window
                        ShowWindow( hWnd, SW_SHOW );
                        _glfwSetForegroundWindow( _glfwWin.Wnd );
                        SetFocus( _glfwWin.Wnd );
                    }
                }

                // Lock mouse, if necessary
                if( _glfwWin.OldMouseLockValid && _glfwWin.OldMouseLock )
                {
                    ShowMouseCursor( false );
                }
                _glfwWin.OldMouseLockValid = false;
            }

            _glfwWin.Iconified = Iconified;
            return 0;

        // Intercept system commands (forbid certain actions/events)
        case WM_SYSCOMMAND:
            switch( wParam )
            {
                // Screensaver trying to start or monitor trying to enter
                // powersave?
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                    if( _glfwWin.Fullscreen )
                    {
                        return 0;
                    }
                    else
                    {
                        break;
                    }

                // User trying to access application menu using ALT?
                case SC_KEYMENU:
                    return 0;
            }
            break;

        // Did we receive a close message?
        case WM_CLOSE:
            PostQuitMessage( 0 );
            return 0;

        // Is a key being pressed?
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            // Translate and report key press
            InputKey( TranslateKey( wParam, lParam ), ButtonState::Press );

            // Translate and report character input
            if( _glfwWin.CharCallback )
                TranslateChar( wParam, lParam, ButtonState::Press );
            return 0;

        // Is a key being released?
        case WM_KEYUP:
        case WM_SYSKEYUP:
            // Special trick: release both shift keys on SHIFT up event
            if( wParam == VK_SHIFT )
            {
                InputKey( Key::LShift, ButtonState::Release );
                InputKey( Key::RShift, ButtonState::Release );
            }
            else // Translate and report key release
                InputKey( TranslateKey( wParam, lParam ), ButtonState::Release );

            // Translate and report character input
            if( _glfwWin.CharCallback )
                TranslateChar( wParam, lParam, ButtonState::Release );
            return 0;

        // Were any of the mouse-buttons pressed?
        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            InputMouseClick( MouseInput::Left, ButtonState::Press );
            return 0;
        case WM_RBUTTONDOWN:
            SetCapture(hWnd);
            InputMouseClick( MouseInput::Right, ButtonState::Press );
            return 0;
        case WM_MBUTTONDOWN:
            SetCapture(hWnd);
            InputMouseClick( MouseInput::Middle, ButtonState::Press );
            return 0;
        case WM_XBUTTONDOWN:
            if( HIWORD(wParam) == XBUTTON1 )
            {
                SetCapture(hWnd);
                InputMouseClick( MouseInput::Btn4, ButtonState::Press );
            }
            else if( HIWORD(wParam) == XBUTTON2 )
            {
                SetCapture(hWnd);
                InputMouseClick( MouseInput::Btn5, ButtonState::Press );
            }
            return 1;

        // Were any of the mouse-buttons released?
        case WM_LBUTTONUP:
            ReleaseCapture();
            InputMouseClick( MouseInput::Left, ButtonState::Release );
            return 0;
        case WM_RBUTTONUP:
            ReleaseCapture();
            InputMouseClick( MouseInput::Right, ButtonState::Release );
            return 0;
        case WM_MBUTTONUP:
            ReleaseCapture();
            InputMouseClick( MouseInput::Middle, ButtonState::Release );
            return 0;
        case WM_XBUTTONUP:
            if( HIWORD(wParam) == XBUTTON1 )
            {
                ReleaseCapture();
                InputMouseClick( MouseInput::Btn4, ButtonState::Release );
            }
            else if( HIWORD(wParam) == XBUTTON2 )
            {
                ReleaseCapture();
                InputMouseClick( MouseInput::Btn5, ButtonState::Release );
            }
            return 1;

        // Did the mouse move?
        case WM_MOUSEMOVE:
            {
                int NewMouseX, NewMouseY;

                // Get signed (!) mouse position
                NewMouseX = (int)((short)LOWORD(lParam));
                NewMouseY = (int)((short)HIWORD(lParam));

                if( NewMouseX != _glfwInput.OldMouseX ||
                    NewMouseY != _glfwInput.OldMouseY )
                {
                    if( _glfwWin.MouseLock )
                    {
                        _glfwInput.MousePosX += NewMouseX -
                                                _glfwInput.OldMouseX;
                        _glfwInput.MousePosY += NewMouseY -
                                                _glfwInput.OldMouseY;
                    }
                    else
                    {
                        _glfwInput.MousePosX = NewMouseX;
                        _glfwInput.MousePosY = NewMouseY;
                    }
                    _glfwInput.OldMouseX = NewMouseX;
                    _glfwInput.OldMouseY = NewMouseY;
                    _glfwInput.MouseMoved = true;
    
                    // Call user callback function
                    if( _glfwWin.MousePosCallback )
                    {
                        _glfwWin.MousePosCallback( _glfwWin.UserData, _glfwInput.MousePosX, _glfwInput.MousePosY, _glfwWin.MouseLock );
                    }
                }
            }
            return 0;

        // Mouse wheel action?
        case WM_MOUSEWHEEL:
            WheelDelta = (((int)wParam) >> 16) / WHEEL_DELTA;
            _glfwInput.WheelPos += WheelDelta;
            if( _glfwWin.MouseWheelCallback )
            {
                _glfwWin.MouseWheelCallback( _glfwWin.UserData, _glfwInput.WheelPos );
            }
            return 0;

        case WM_MOVE:
            return 0;

        // Resize the window?
        case WM_SIZE:
            _glfwWin.Width  = LOWORD(lParam);
            _glfwWin.Height = HIWORD(lParam);
            if( _glfwWin.WindowSizeCallback )
            {
                _glfwWin.WindowSizeCallback( _glfwWin.UserData, LOWORD(lParam), HIWORD(lParam) );
            }
            return 0;

        // Was the window contents damaged?
        case WM_PAINT:
            break;

        default:
            break;
    }

    // Pass all unhandled messages to DefWindowProc
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}



int OsWindowWin32::TranslateKey( DWORD wParam, DWORD lParam )
{
    MSG next_msg;
    DWORD msg_time;
    DWORD scan_code;

    // Check which key was pressed or released
    switch( wParam )
    {
        // The SHIFT keys require special handling
        case VK_SHIFT:
            // Compare scan code for this key with that of VK_RSHIFT in
            // order to determine which shift key was pressed (left or
            // right)
            scan_code = MapVirtualKey( VK_RSHIFT, 0 );
            if( ((lParam & 0x01ff0000) >> 16) == scan_code )
            {
                return Key::RShift;
            }
            return Key::LShift;

        // The CTRL keys require special handling
        case VK_CONTROL:
            // Is this an extended key (i.e. right key)?
            if( lParam & 0x01000000 )
            {
                return Key::RCtrl;
            }
            // Here is a trick: "Alt Gr" sends LCTRL, then RALT. We only
            // want the RALT message, so we try to see if the next message
            // is a RALT message. In that case, this is a false LCTRL!
            msg_time = GetMessageTime();
            if( PeekMessage( &next_msg, NULL, 0, 0, PM_NOREMOVE ) )
            {
                if( next_msg.message == WM_KEYDOWN ||
                    next_msg.message == WM_SYSKEYDOWN )
                {
                    if( next_msg.wParam == VK_MENU &&
                        (next_msg.lParam & 0x01000000) &&
                        next_msg.time == msg_time )
                    {
                        // Next message is a RALT down message, which
                        // means that this is NOT a proper LCTRL message!
                        return Key::Unknown;
                    }
                }
            }
            return Key::LCtrl;

        // The ALT keys require special handling
        case VK_MENU:
            // Is this an extended key (i.e. right key)?
            if( lParam & 0x01000000 )
            {
                return Key::RAlt;
            }
            return Key::LAlt;

        // The ENTER keys require special handling
        case VK_RETURN:
            // Is this an extended key (i.e. right key)?
            if( lParam & 0x01000000 )
            {
                return Key::KP_Enter;
            }
            return Key::Enter;

        // Special keys (non character keys)
        case VK_ESCAPE:        return Key::Esc;
        case VK_TAB:           return Key::Tab;
        case VK_BACK:          return Key::Backspace;
        case VK_HOME:          return Key::Home;
        case VK_END:           return Key::End;
        case VK_PRIOR:         return Key::PageUp;
        case VK_NEXT:          return Key::PageDown;
        case VK_INSERT:        return Key::Insert;
        case VK_DELETE:        return Key::Del;
        case VK_LEFT:          return Key::Left;
        case VK_UP:            return Key::Up;
        case VK_RIGHT:         return Key::Right;
        case VK_DOWN:          return Key::Down;
        case VK_F1:            return Key::F1;
        case VK_F2:            return Key::F2;
        case VK_F3:            return Key::F3;
        case VK_F4:            return Key::F4;
        case VK_F5:            return Key::F5;
        case VK_F6:            return Key::F6;
        case VK_F7:            return Key::F7;
        case VK_F8:            return Key::F8;
        case VK_F9:            return Key::F9;
        case VK_F10:           return Key::F10;
        case VK_F11:           return Key::F11;
        case VK_F12:           return Key::F12;
        case VK_F13:           return Key::F13;
        case VK_F14:           return Key::F14;
        case VK_F15:           return Key::F15;
        case VK_F16:           return Key::F16;
        case VK_F17:           return Key::F17;
        case VK_F18:           return Key::F18;
        case VK_F19:           return Key::F19;
        case VK_F20:           return Key::F20;
        case VK_F21:           return Key::F21;
        case VK_F22:           return Key::F22;
        case VK_F23:           return Key::F23;
        case VK_F24:           return Key::F24;
        case VK_SPACE:         return Key::Space;

        // Numeric keypad
        case VK_NUMPAD0:       return Key::KP_0;
        case VK_NUMPAD1:       return Key::KP_1;
        case VK_NUMPAD2:       return Key::KP_2;
        case VK_NUMPAD3:       return Key::KP_3;
        case VK_NUMPAD4:       return Key::KP_4;
        case VK_NUMPAD5:       return Key::KP_5;
        case VK_NUMPAD6:       return Key::KP_6;
        case VK_NUMPAD7:       return Key::KP_7;
        case VK_NUMPAD8:       return Key::KP_8;
        case VK_NUMPAD9:       return Key::KP_9;
        case VK_DIVIDE:        return Key::KP_Divide;
        case VK_MULTIPLY:      return Key::KP_Multiply;
        case VK_SUBTRACT:      return Key::KP_Subtract;
        case VK_ADD:           return Key::KP_Add;
        case VK_DECIMAL:       return Key::KP_Decimal;

        // The rest (should be printable keys)
        default:
            // Convert to printable character (ISO-8859-1 or Unicode)
            wParam = MapVirtualKey( wParam, 2 ) & 0x0000FFFF;

            // Make sure that the character is uppercase
            if( _glfwSys.HasUnicode )
            {
                wParam = (DWORD) CharUpperW( (LPWSTR) wParam );
            }
            else
            {
                wParam = (DWORD) CharUpperA( (LPSTR) wParam );
            }

            // Valid ISO-8859-1 character?
            if( (wParam >=  32 && wParam <= 126) ||
                (wParam >= 160 && wParam <= 255) )
            {
                return (int) wParam;
            }
            return Key::Unknown;
    }
}


void OsWindowWin32::TranslateChar( DWORD wParam, DWORD lParam, int action )
{
    BYTE  keyboard_state[ 256 ];
    UCHAR char_buf[ 10 ];
    WCHAR unicode_buf[ 10 ];
    UINT  scan_code;
    int   i, num_chars, unicode;

    // Get keyboard state
    GetKeyboardState( keyboard_state );

    // Derive scan code from lParam and action
    scan_code = (lParam & 0x01ff0000) >> 16;
    if( action == ButtonState::Release )
    {
        scan_code |= 0x8000000;
    }

    // Do we have Unicode support?
    if( _glfwSys.HasUnicode )
    {
        // Convert to Unicode
        num_chars = ToUnicode(
            wParam,          // virtual-key code
            scan_code,       // scan code
            keyboard_state,  // key-state array
            unicode_buf,     // buffer for translated key
            10,              // size of translated key buffer
            0                // active-menu flag
        );
        unicode = 1;
    }
    else
    {
        // Convert to ISO-8859-1
        num_chars = ToAscii(
            wParam,            // virtual-key code
            scan_code,         // scan code
            keyboard_state,    // key-state array
            (LPWORD) char_buf, // buffer for translated key
            0                  // active-menu flag
        );
        unicode = 0;
    }

    // Report characters
    for( i = 0; i < num_chars; i ++ )
    {
        // Get next character from buffer
        if( unicode )
        {
            InputChar( (int) unicode_buf[ i ], action );
        }
        else
        {
            InputChar( (int) char_buf[ i ], action );
        }
    }
}
