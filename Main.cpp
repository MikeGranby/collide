
#include "Intern.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Fun with Graphics 
//
// Copyright (c) 2017 Granby Consulting LLC
//
// All Rights Reserved
//

#include "GdiPlus.hpp"

#include "Vector.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Constants
//

#define	BALLS	144	// Ball Count

#define	RADIUS	15	// Ball Size

#define	VARIES	5	// Ball Variation

#define	SPEED	150	// Base Speed

#define	RANGE	50	// Speed Range

#define	POWER	3	// Mass Power

#define	TRACK	FALSE	// Show Next

#define	DOTS	FALSE	// Show Dots

#define	WTC	2	// Worker Threads

//////////////////////////////////////////////////////////////////////////
//
// Ball Object
//

struct CBall
{
	CVector  p;
	CVector  v;
	double   r;
	COLORREF c;
	};

//////////////////////////////////////////////////////////////////////////
//
// Event Object
//

struct CEvent
{
	int    n;
	int    o;
	double t;
	};

//////////////////////////////////////////////////////////////////////////
//
// Wall Codes
//

enum
{
	wallLeft   = -1,
	wallTop    = -2,
	wallRight  = -3,
	wallBottom = -4,
	};

//////////////////////////////////////////////////////////////////////////
//
// Static Data
//

static	HINSTANCE	m_hThis;

static	HWND		m_hWnd;

static	CBall		m_b[BALLS];

static	CBall		m_d[BALLS];

static	RECT		m_box;

static	ULONG_PTR	gdipToken;

static	HANDLE		m_hDraw;

static	HANDLE		m_hWait[WTC];

static	HANDLE		m_hDone[WTC];

static	UINT		m_split[WTC+2];

static	CEvent		m_w    [WTC];

static	CEvent		m_e = { -1, -1, 0 };

static	CEvent		m_f = { -1, -1, 0 };

//////////////////////////////////////////////////////////////////////////
//
// Prototypes
//

global	int	WINAPI	WinMain(HINSTANCE hThis, HINSTANCE hPrev, PSTR pCmdLine, int nCmdShow);
static	bool		AfxTrace(char const *pText, ...);
static	void		GraphInit(void);
static	void		GraphTerm(void);
static	void		MakeWindow(void);
static	void		KillWindow(void);
static	void		PumpWindow(void);
static	LRESULT	WINAPI	WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
static	void		Draw(HDC hDC);
static	void		FindSplits(void);
static	void		MakeEvents(void);
static	void		MakeMainThread(void);
static	void		MakeWorkThreads(void);
static	DWORD	WINAPI	MainThreadProc(LPVOID pParam);
static	DWORD	WINAPI	WorkThreadProc(LPVOID pParam);
static	void		FindNextEvent(void);
static	void		SendUpdate(void);
static	void		InitBalls(void);
static	void		MoveBalls(double dt);
static	double		sq(double x);
static	double		TimeToCollide(CBall const &b, int o, double t);
static	void		Collide(CEvent const &e);
static	void		FindNextEvent(CEvent &e, int n1, int n2);
static	void		ShowDetails(void);

//////////////////////////////////////////////////////////////////////////
//
// Code
//

global int WINAPI WinMain(HINSTANCE hThis, HINSTANCE hPrev, PSTR pCmdLine, int nCmdShow)
{
	m_hThis = hThis;

	GraphInit();

	MakeWindow();

	FindSplits();

	MakeEvents();

	MakeWorkThreads();

	MakeMainThread();

	PumpWindow();

	KillWindow();

	GraphTerm();

	return 1;
	}

static bool AfxTrace(char const *pText, ...)
{
	char s[1024];

	va_list pArgs;

	va_start(pArgs, pText);

	vsprintf(s, pText, pArgs);

	va_end(pArgs);

	OutputDebugString(s);

	return true;
	}

static void GraphInit(void)
{
	GdiplusStartupOutput Output;

	GdiplusStartupInput  Input;

	memset(&Input, 0, sizeof(Input));

	Input.GdiplusVersion = 1;

	GdiplusStartup(&gdipToken, &Input, &Output);
	}

static void GraphTerm(void)
{
	GdiplusShutdown(gdipToken);
	}

static void MakeWindow(void)
{
	WNDCLASS Class;

	Class.style		= CS_DBLCLKS;
	Class.lpfnWndProc	= WndProc;
	Class.cbClsExtra	= 0;
	Class.cbWndExtra	= 0;
	Class.hInstance		= m_hThis;
	Class.hCursor		= LoadCursor(NULL, IDC_ARROW);
	Class.hIcon		= LoadIcon  (NULL, IDI_APPLICATION);
	Class.hbrBackground	= NULL;
	Class.lpszMenuName	= NULL;
	Class.lpszClassName	= "CollideClass";

	if( RegisterClass(&Class) ) {

		int cx = 900;

		int cy = 700;

		int xp = (GetSystemMetrics(SM_CXSCREEN) - cx) / 2;

		int yp = (GetSystemMetrics(SM_CYSCREEN) - cy) / 2;

		m_hWnd = CreateWindow(  "CollideClass",
					"Collide",
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
					xp, yp,
					cx, cy,
					NULL,
					NULL,
					m_hThis,
					NULL
					);

		ShowWindow(m_hWnd, SW_SHOW);

		UpdateWindow(m_hWnd);
		}
	}

static void PumpWindow(void)
{
	SetThreadAffinityMask(GetCurrentThread(), 0x01);

	MSG Msg;

	while( GetMessage(&Msg, NULL, 0, 0) ) {

		TranslateMessage(&Msg);

		DispatchMessage(&Msg);
		}
	}

static void KillWindow(void)
{
	DestroyWindow(m_hWnd);
	}

global LRESULT WINAPI WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if( uMessage == WM_PAINT ) {

		PAINTSTRUCT ps;

		HDC hDC = BeginPaint(hWnd, &ps);

		Draw(hDC);

		EndPaint(hWnd, &ps);

		return 0;
		}

	if( uMessage == WM_COMMAND ) {

		HDC hDC = GetDC(hWnd);

		Draw(hDC);

		SetEvent(m_hDraw);

		ReleaseDC(hWnd, hDC);
		}

	if( uMessage == WM_CLOSE ) {

		PostQuitMessage(0);

		return 0;
		}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

static void Draw(HDC hDC)
{
	HDC     hDev = CreateCompatibleDC(hDC);

	HBITMAP hBit = CreateCompatibleBitmap(hDC, m_box.right, m_box.bottom);

	HBITMAP hWas = HBITMAP(SelectObject(hDev, hBit));

	GpGraphics *pGraph = NULL;

	GdipCreateFromHDC(hDev, &pGraph);

	GdipSetSmoothingMode(pGraph, SmoothingModeAntiAlias);

	for( int n = 0; n < elements(m_d); n++ ) {

		if( true || n == 32 || !(GetKeyState(VK_SHIFT) & 0x8000) ) {

			CBall const &b = m_d[n];

			GpPath *pPath = NULL;

			GdipCreatePath(FillModeAlternate, &pPath);

			GdipAddPathEllipse( pPath,
					    Gdiplus::REAL(b.p.x - b.r),
					    Gdiplus::REAL(b.p.y - b.r),
					    Gdiplus::REAL(2 * b.r),
					    Gdiplus::REAL(2 * b.r)
					    );

			GpSolidFill *pBrush = NULL;

			GdipCreateSolidFill(b.c | ALPHA_MASK, &pBrush);

			GdipFillPath(pGraph, pBrush, pPath);

			GdipDeleteBrush(pBrush);

			if( TRACK ) {

				if( m_f.n == n || m_f.o == n ) {

					GpPen *pPen = NULL;

					COLORREF pc = (m_f.o < 0) ? 0xFF00FF00 : 0xFFFF0000;

					GdipCreatePen1(pc, 2.0, UnitPixel, &pPen);

					GdipDrawPath(pGraph, pPen, pPath);

					GdipDeletePen(pPen);
					}
				}

			GdipDeletePath(pPath);
			}
		}

	if( DOTS ) {

		for( int i = 0; i < 2; i++ ) {

			if( i == 1 && m_f.o < 0 ) {

				break;
				}

			CBall const &b = m_d[i ? m_f.o : m_f.n];

			CVector p = b.p + b.v * m_f.t;

			GpPath *pPath = NULL;

			GdipCreatePath(FillModeAlternate, &pPath);

			GdipAddPathEllipse( pPath,
					    Gdiplus::REAL(p.x-2),
					    Gdiplus::REAL(p.y-2),
					    Gdiplus::REAL(4),
					    Gdiplus::REAL(4)
					    );

			GpSolidFill *pBrush = NULL;

			GdipCreateSolidFill(0xFFFFFFFF, &pBrush);

			GdipFillPath(pGraph, pBrush, pPath);

			GdipDeleteBrush(pBrush);

			GdipDeletePath(pPath);
			}
		}

	GdipDeleteGraphics(pGraph);

	BitBlt(hDC, 0, 0, m_box.right, m_box.bottom, hDev, 0, 0, SRCCOPY);

	SelectObject(hDev, hWas);

	DeleteObject(hBit);

	DeleteObject(hDev);
	}

static void FindSplits(void)
{
	for( UINT n = 0; n < WTC+2; n++ ) {

		double pos = elements(m_b) * sqrt(double(n) / (WTC+1));

		m_split[n] = int(pos + 0.5);
		}
	}

static void MakeEvents(void)
{
	for( UINT n = 0; n < WTC; n++ ) {

		m_hWait[n] = CreateEvent(NULL, FALSE, FALSE, NULL);

		m_hDone[n] = CreateEvent(NULL, FALSE, FALSE, NULL);
		}

	m_hDraw = CreateEvent(NULL, FALSE, TRUE,  NULL);
	}

static void MakeMainThread(void)
{
	CreateThread( NULL,
		      0,
		      MainThreadProc,
		      NULL,
		      0,
		      NULL
		      );
	}

static void MakeWorkThreads(void)
{
	for( UINT n = 0; n < WTC; n++ ) {

		CreateThread( NULL,
			      0,
			      WorkThreadProc,
			      LPVOID(n),
			      0,
			      NULL
			      );
		}
	}

static DWORD WINAPI MainThreadProc(LPVOID pParam)
{
	SetThreadAffinityMask(GetCurrentThread(), 0x02);

	srand(GetTickCount());

	GetClientRect(m_hWnd, &m_box);

	InitBalls();

	SendUpdate();

	Sleep(1000);

	FindNextEvent();

	LARGE_INTEGER freq;

	QueryPerformanceFrequency(&freq);

	LARGE_INTEGER prev;

	QueryPerformanceCounter(&prev);

	for(;;) {

		LARGE_INTEGER time;

		QueryPerformanceCounter(&time);

		double dt = double(time.QuadPart - prev.QuadPart) / double(freq.QuadPart);

		prev      = time;

		if( dt > 0.02 ) {
			
			dt = 0.02;
			}

		while( dt >= m_e.t ) {

			if( m_e.t ) {

				MoveBalls(m_e.t);

				dt = dt - m_e.t;
				}

			Collide(m_e);

			FindNextEvent();
			}

		MoveBalls(dt);

		m_e.t = m_e.t - dt;

		ShowDetails();

		SendUpdate();
		}

	return 0;
	}

static DWORD WINAPI WorkThreadProc(LPVOID pParam)
{
	UINT nt = UINT(pParam);

	SetThreadAffinityMask(GetCurrentThread(), 0x04 << nt);

	for(;;) {

		WaitForSingleObject(m_hWait[nt], INFINITE);

		FindNextEvent(m_w[nt], m_split[nt+1], m_split[nt+2]);

		SetEvent(m_hDone[nt]);
		}
	}

static	void	FindNextEvent(void)
{
	for( UINT n = 0; n < WTC; n++ ) {

		SetEvent(m_hWait[n]);
		}

	FindNextEvent(m_e, m_split[0], m_split[1]);

	WaitForMultipleObjects(WTC, m_hDone, TRUE, INFINITE);

	for( UINT n = 0; n < WTC; n++ ) {

		if( m_w[n].t < m_e.t ) {

			m_e = m_w[n];
			}
		}
	}

static void SendUpdate(void)
{
	WaitForSingleObject(m_hDraw, INFINITE);

	memcpy(m_d, m_b, sizeof(m_b));

	m_f = m_e;

	PostMessage(m_hWnd, WM_COMMAND, IDOK, 0);
	}

static void InitBalls(void)
{
	int e = elements(m_b);

	int c = int(sqrt(double(e))+0.9999);

	int r = (e + c - 1) / c;

	for( int n = 0; n < e; n++ ) {

		CBall &b = m_b[n];

		b.p.x = (1 + (n % c)) * (m_box.right  - m_box.left) / (c + 1);

		b.p.y = (1 + (n / c)) * (m_box.bottom - m_box.top ) / (r + 1);

		b.v.x = ((rand() % 2) ? -1 : +1) * (SPEED + rand() % RANGE);

		b.v.y = ((rand() % 2) ? -1 : +1) * (SPEED + rand() % RANGE);
		
		b.r   = RADIUS - (VARIES / 2) + (rand() % (1+VARIES));

		b.c   = RGB( 96 + rand() % 128, 
			     96 + rand() % 128, 
			     96 + rand() % 128
			     );
		}
	}

static	void	MoveBalls(double dt)
{
	for( int n = 0; n < elements(m_b); n++ ) {

		CBall &b = m_b[n];

		b.p += b.v * dt;
		}
	}

static	double	sq(double x)
{
	return x*x;
	}

static	double	TimeToCollide(CBall const &b1, int o, double t)
{
	if( o < 0 ) {

		switch( o ) {

			case wallLeft:

				if( b1.v.x < 0 ) {

					return (m_box.left - b1.p.x + b1.r) / b1.v.x;
					}
				break;

			case wallTop:

				if( b1.v.y < 0 ) {

					return (m_box.top - b1.p.y + b1.r) / b1.v.y;
					}
				break;

			case wallRight:

				if( b1.v.x > 0 ) {

					return (m_box.right - b1.p.x - b1.r) / b1.v.x;
					}
				break;

			case wallBottom:

				if( b1.v.y > 0 ) {

					return (m_box.bottom - b1.p.y - b1.r) / b1.v.y;
					}
				break;
			}
		}
	else {
		CBall const &b2   = m_b[o];

		double       rr   = b1.r + b2.r;

		bool         cull = false;

		if( !cull ) {

			double dx = b1.p.x - b2.p.x;

			if( dx < -rr || dx > +rr ) {

				double vx = b2.v.x - b1.v.x;

				double tx = (dx > 0 ? dx - rr : dx + rr) / vx;

				if( tx < 0 || tx > t ) {

					cull = true;
					}
				}
			}

		if( !cull ) {

			double dy = b1.p.y - b2.p.y;

			if( dy < -rr || dy > +rr ) {

				double vy = b2.v.y - b1.v.y;

				double ty = (dy > 0 ? dy - rr : dy + rr) / vy;

				if( ty < 0 || ty > t ) {

					cull = true;
					}
				}
			}

		if( !cull ) {

			double a = sq(b1.v.x)
				 + sq(b2.v.x)
				 + sq(b1.v.y)
				 + sq(b2.v.y)
				 - 2*b1.v.x*b2.v.x
				 - 2*b1.v.y*b2.v.y;

			if( a ) {

				double b = 2 * ( b1.v.x*b1.p.x 
					       + b2.v.x*b2.p.x 
					       + b1.v.y*b1.p.y 
					       + b2.v.y*b2.p.y
					       - b1.v.x*b2.p.x 
					       - b2.v.x*b1.p.x 
					       - b1.v.y*b2.p.y 
					       - b2.v.y*b1.p.y );

				double c = sq(b1.p.x)
					 + sq(b2.p.x)
					 + sq(b1.p.y)
					 + sq(b2.p.y)
					 - 2*b1.p.y*b2.p.y
					 - 2*b1.p.x*b2.p.x
					 - sq(rr);

				double d = sq(b) - 4*a*c;

				if( d > 0 ) {

					double dr = sqrt(d);

					double t1 = (-b + dr) / (2 * a);

					double t2 = (-b - dr) / (2 * a);

					double ta = min(t1, t2);

					double tb = max(t1, t2);

					if( ta >= 0 && tb > 0 ) {

						return ta;
						}
					}
				}
			}
		}

	return -1;
	}

static	void	Collide(CEvent const &e)
{
	CBall &b1 = m_b[e.n];

	if( m_e.o < 0 ) {

		switch( e.o ) {

			case wallLeft:
			case wallRight:

				b1.v.x *= -1;

				break;

			case wallTop:
			case wallBottom:

				b1.v.y *= -1;

				break;
			}
		}
	else {
		CBall & b2 = m_b[e.o];

		double  m1 = pow(b1.r, POWER);

		double  m2 = pow(b2.r, POWER);

		CVector d1 = b1.p - b2.p;

		CVector d2 = b2.p - b1.p;
		
		double  k1 = ((b1.v - b2.v) * d1) / d1.ModSq();

		double  k2 = ((b2.v - b1.v) * d2) / d2.ModSq();
		
		CVector v1 = b1.v - d1 * (k1 * (m2 + m2) / (m1 + m2));

		CVector v2 = b2.v - d2 * (k2 * (m1 + m1) / (m1 + m2));

		b1.v = v1;
		
		b2.v = v2;
		}
	}

static	void	FindNextEvent(CEvent &e, int n1, int n2)
{
	e.n = -1;
	e.o = -1;
	e.t = 1E10;

	int c = 0;

	for( int n = n1; n < n2; n++ ) {

		CBall const &b = m_b[n];

		for( int o = -4; o < n; o++ ) {

			double t = TimeToCollide(b, o, e.t);

			if( t >= 0 && t < e.t ) {

				e.n = n;
				e.o = o;
				e.t = t;
				}
			}
		}
	}

static	void	ShowDetails(void)
{
	if( false ) {

		double  e = 0;

		CVector p = { 0, 0 };

		for( int n = 0; n < elements(m_b); n++ ) {

			CBall const &b = m_b[n];

			double       m = pow(b.r, POWER);

			e += 0.5 * m * b.v.ModSq();

			p += b.v * m;
			}

		AfxTrace("Energy is %.0f   ", e);

		AfxTrace("Momentum is (%0.f, %0.f) = %0.f%   ", p.x, p.y, p.Mod());

		AfxTrace("\n");
		}
	}

// End of File
