//*********************************
// Uber-Pong by Sean Gilleran
// (C)2003 Anti-Mass Studios
// All rights reserved
//*********************************


//====================================================
// D3D Libraries
//====================================================

#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3d8.lib" )
#pragma comment( lib, "d3dx8.lib" )
#pragma comment( lib, "winmm.lib" )


//====================================================
// Preprocessor Directives
//====================================================

#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <cstdlib>
#include <windows.h>
#include <mmsystem.h>
#include <d3d8.h>
#include <d3dx8.h>
#include "engine.h"
#include "resource.h"

// Namespace Declaration
using namespace std;


//====================================================
// Constants
//====================================================

// Paddle & Ball Parameters
#define PADDLE_WIDTH		19	// Width of the paddle bitmap
#define PADDLE_HEIGHT		79	// Height of the paddle bitmap
#define PADDLE_INITIAL_X	15	// Distance of the paddle from the side of the screen
#define PADDLE_SPEED		6	// Number of pixels per key press the paddle moves
#define BALL_WIDTH		30	// Width of the ball bitmap
#define BALL_HEIGHT		30	// Height of the ball bitmap
#define BALL_SPEED		1	// Number of pixels per tick the ball moves

// Controls
#define START			VK_SPACE		// 'START' Button
#define P1_UP			VK_UP			// Player One's 'UP' Button
#define P1_DOWN			VK_DOWN			// Player One's 'DOWN' Button
#define P2_UP			VK_TAB			// Player Two's 'UP' Button
#define P2_DOWN			VK_LCONTROL		// Player Two's 'DOWN' Button

// Window Resolution
#define RES_WIDTH	640		// Window Width
#define RES_HEIGHT	480		// Window Height

// Font Parameters
#define FONT_LETTERW	8						// Width of each letter
#define FONT_LETTERH	16						// Height of each letter
#define FONT_LPR		10						// Number of letters per row
#define FONT_WIDTH		80						// Width of the font pallate
#define FONT_HEIGHT		160						// Height of the font pallate

// Miscellanious
#define MAX_SCORE	10		// How many points it takes to win

const char g_AppName[] = "Sean's UBER_PONG v1.0";	// Application Name CHAR


//====================================================
// Global Variables
//====================================================

POINT g_Paddle1;		// Global paddle structure
POINT g_Paddle2;		// Global paddle structure
POINT g_Ball;		// Global ball structure

HWND g_hWndMain;	// Global window handle
HDC g_hDC;			// Global device context

int g_p1Score = 0;	// Player one's score
int g_p2Score = 0;	// Player two's score

int g_BallSpeed = BALL_SPEED;	// Variable to change ball speed

int MultiplierX;	// Controls ball's x motion
int MultiplierY;	// Controls ball's y motion
int g_BounceCount;	// Controls ball's speed

int g_PlayWinSound = 2;		// Controls winning sound

// Surfaces
LPDIRECT3DSURFACE8 g_pBgSurf = 0;
LPDIRECT3DSURFACE8 g_pPaddle1Surf = 0;
LPDIRECT3DSURFACE8 g_pPaddle2Surf = 0;
LPDIRECT3DSURFACE8 g_pBallSurf = 0;


//====================================================
// Function Prototypes
//====================================================

// Basic Game Functions
int GameInit( void );
int GameLoop( void );
int GameShutdown( void );
int Render( void );
void MoveBall( void );
void RandomDirection( void );

// Miscellanious
void Debug( char* String );

// Pauses for specified number of ms
void Pause( int Milliseconds )
{
	int Time = GetTickCount( );
	while( (UINT)GetTickCount( ) - (UINT)Time < (UINT)Milliseconds ) {}
}


//====================================================
// Windows Procedure Loop & Entry Point
//====================================================

// Windows proc. loop
long CALLBACK WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT PaintStruct;	// Holds info for the GDI
	HDC hDC;					// Local HDC

	switch( uMessage )
	{
		case WM_CREATE:		// Windows has called CreateWindow()
			return 0;
		case WM_PAINT:		// Windows has called RedrawWindow()
		{
			ValidateRect( hWnd, NULL );
			return 0;
		}
		case WM_DESTROY:	// The main window is about to be closed
		{
			PostQuitMessage( 0 );
			return 0;
		}

		default:			// Let windows handle an unknown message
			return (long)DefWindowProc( hWnd, uMessage, wParam, lParam );
	}
}

// Windows Entry Point
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pstrCmdLine, int iCmdShow )
{
	HWND hWnd;		// Main Window Handle
	MSG msg;		// Local Procedure Message
	WNDCLASSEX wc;	// Main Window Class

	// Define the window
	wc.cbSize			= sizeof( WNDCLASSEX );									// The size of the window class (in bytes)
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;					// Windows style flags
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.lpfnWndProc		= WndProc;												// Name of the procedure loop
	wc.hInstance		= hInstance;											// Instance Handle name
	wc.hbrBackground	= (HBRUSH)GetStockObject( BLACK_BRUSH );				// Make the actual window black
	wc.hIcon			= LoadIcon( NULL, MAKEINTRESOURCE( IDI_UBERPONG ) );	// Load the icon
	wc.hIconSm			= LoadIcon( NULL, MAKEINTRESOURCE( IDI_UBERPONG ) );	// Load the smaller version
	wc.hCursor			= LoadCursor( NULL, IDC_CROSS );						// Load the cursor
	wc.lpszMenuName		= NULL;													// Handle to the menu
	wc.lpszClassName	= g_AppName;											// The window's name

	// Register the class with windows
	RegisterClassEx( &wc );

	// Create the window
	hWnd = CreateWindowEx(  NULL,		
							g_AppName,					// The name of the class
							g_AppName,					// The window caption
							WS_POPUP | WS_EX_TOPMOST,	// Window flags (no border, menu, or title bar, and keep on top)
							CW_USEDEFAULT,				// Initial x position
							CW_USEDEFAULT,				// Initial y position
							RES_WIDTH, RES_HEIGHT,		// Window Resolution (x, y)
							NULL,						// Handle to parent window
							NULL,						// Menu Handle
							hInstance,					// Instance handle
							NULL  );							

	// Save the window handle as a global variable
	g_hWndMain = hWnd;

	// Display and initialize the primary window
	ShowWindow( hWnd, iCmdShow );
	UpdateWindow( hWnd );

	GameInit( );

	// Start the message loop
	while( TRUE )
	{
		// Check for a message
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( msg.message == WM_QUIT )
				break;

			// Format and send the message
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
			// Nothing else is happening
			GameLoop( );
	}
	
	GameShutdown();

	// Return control to windows with the exit code
	return msg.wParam;
}

//====================================================
// Game Initialization, Loop, and Shutdown
//====================================================

int GameInit()
{
	HRESULT r = 0;
	
	// Acquire a pointer to IDirect3D8
	g_pD3D = Direct3DCreate8( D3D_SDK_VERSION );
	if( g_pD3D == NULL )
	{
		Debug( "Could not create IDirect3D8 object" );
		return E_FAIL;
	}

	// Create the device
	r = InitDirect3DDevice( g_hWndMain, 640, 480, FALSE, D3DFMT_X8R8G8B8, g_pD3D, &g_pDevice );
	if( FAILED( r ) )
	{
		Debug( "Initialization of the device failed" );
		return E_FAIL;
	}

	// Clear the back buffer
	g_pDevice->Clear( 0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 0, 0, 55 ), 1.0f, 0 );

	// Get a pointer to the back buffer and save it in a global variable
	r = g_pDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackSurface );
	if( FAILED( r ) )
	{
		Debug( "Couldnt get backbuffer" );
		return E_FAIL;
	}	

	srand( GetTickCount() );
	InitTiming( );

	char PaddleImage[] = "graphics\\paddle.bmp";
	char BallImage[] = "graphics\\ball.bmp";
	char BgImage[] = "graphics\\space.bmp";
	char FontImage[] = "graphics\\font.bmp";

	// Load graphics
	LoadBitmapToSurface( BgImage, &g_pBgSurf, g_pDevice );			// Background
	LoadBitmapToSurface( PaddleImage, &g_pPaddle1Surf, g_pDevice );			// Paddle 1
	LoadBitmapToSurface( PaddleImage, &g_pPaddle2Surf, g_pDevice );			// Paddle 2
	LoadBitmapToSurface( BallImage, &g_pBallSurf, g_pDevice );		// Ball

	// Load font engine
	LoadAlphabet( FontImage, FONT_LETTERW, FONT_LETTERH );

	// Initialize player one's paddle
	g_Paddle1.x = PADDLE_INITIAL_X;
	g_Paddle1.y = ( RES_HEIGHT / 2 );

	// Initialize player two's paddle
	g_Paddle2.x = ( RES_WIDTH - PADDLE_INITIAL_X - PADDLE_WIDTH );
	g_Paddle2.y = ( RES_HEIGHT / 2 );

	// Initialize the Ball
	RandomDirection( );
	g_BounceCount = 0;
			
	return S_OK;
}

int GameLoop( )
{
	// Used for object synchronization
	static Paddle1Time = 0;
	static Paddle2Time = 0;
	static BallTime = 0;

	// Check if this is the first time the loop has run
	if( Paddle1Time == 0 )
		Paddle1Time = 1;
	if( Paddle2Time == 0 )
		Paddle2Time = 1;

	// Check for keyboard input, and screen input to prevent breakage or tearing

	// Player One Controls
	if( GetAsyncKeyState( P1_UP ) )
	{
		if( !( g_Paddle1.y == 15 ) )
		{
			if( ( GetTickCount( ) - Paddle1Time ) > 1 )
			{
				Paddle1Time = GetTickCount( );
				g_Paddle1.y -= PADDLE_SPEED;
			}
		}
		else
			g_Paddle1.y = 15;
	}
	if( GetAsyncKeyState( P1_DOWN ) )
	{
		if( !( g_Paddle1.y == ( RES_HEIGHT - PADDLE_HEIGHT - 15 ) ) )
		{
			if( ( GetTickCount( ) - Paddle1Time ) > 1 )
			{
				Paddle1Time = GetTickCount( );
				g_Paddle1.y += PADDLE_SPEED;
			}
		}
		else
			g_Paddle1.y = ( RES_HEIGHT + PADDLE_HEIGHT + 15 );
	}

	// Player Two Controls
	if( GetAsyncKeyState( P2_UP ) )
	{
		if( !( g_Paddle2.y == 15 ) )
		{
			if( ( GetTickCount( ) - Paddle2Time ) > 1 )
			{
				Paddle2Time = GetTickCount( );
				g_Paddle2.y -= PADDLE_SPEED;
			}
		}
		else
			g_Paddle2.y = 15;
	}
	if( GetAsyncKeyState( P2_DOWN ) )
	{
		if( !( g_Paddle2.y == ( RES_HEIGHT + PADDLE_HEIGHT + 15 ) ) )
		{
			if( ( GetTickCount( ) - Paddle2Time ) > 1 )
			{
				Paddle2Time = GetTickCount( );
				g_Paddle2.y += PADDLE_SPEED;
			}
		}
		else
			g_Paddle2.y = ( RES_HEIGHT + PADDLE_HEIGHT + 15 );
	}

	// Adjust Ball Speed through F-Keys
	if( GetAsyncKeyState( VK_F1 ) )
		g_BallSpeed = 1;
	if( GetAsyncKeyState( VK_F2 ) )
		g_BallSpeed = 2;
	if( GetAsyncKeyState( VK_F3 ) )
		g_BallSpeed = 3;
	if( GetAsyncKeyState( VK_F4 ) )
		g_BallSpeed = 4;
	if( GetAsyncKeyState( VK_F5 ) )
		g_BallSpeed = 5;

	// Escape
	if( GetAsyncKeyState( VK_ESCAPE ) )
		PostQuitMessage( 0 );

	// Update the ball every 10 ms
	if( ( GetTickCount( ) - BallTime ) > 10 )
	{
		BallTime = GetTickCount( );
		//MoveBall( );
	}

	Render( );		// Render images to the back buffer
	MoveBall( );	// Move the ball
	FrameCount( );	// Count FPS
	
	return S_OK;
}

int GameShutdown()
{
	// Release graphics pointers
	g_pBgSurf->Release( );
	g_pPaddle1Surf->Release( );
	g_pPaddle2Surf->Release( );
	g_pBallSurf->Release( );

	// Release font pointer
	UnloadAlphabet( );

	// Release the pointer to the back surface
	if( g_pBackSurface )
		g_pBackSurface->Release( );
	
	// Release the pointer to IDirect3DDevice8
	if( g_pDevice )
		g_pDevice->Release( );
	
	// Release the pointer to IDirect3D8
	if( g_pD3D )
		g_pD3D->Release( );

	return S_OK;
}


//====================================================
// Rendering Function
//====================================================

int Render( )
{
	HRESULT r = 0;

	// Clear the back buffer
	g_pDevice->Clear( 0, 0, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 0, 0, 25 ), 1.0f, 0 );
	
	// Make sure the device is valid
	if( !g_pDevice )
	{
		Debug( "Cannot render because there is no device" );
		return E_FAIL;
	}

	// Return if the device is not ready;
	r = ValidateDevice();
	if( FAILED( r ) )
		return E_FAIL;

	D3DLOCKED_RECT Locked;

	// Draw the Background
	CopySurfaceToSurface( NULL, g_pBgSurf, 0, g_pBackSurface, FALSE, D3DCOLOR_ARGB( 0, 255, 0, 255 ) );

	// Draw the Paddles
	CopySurfaceToSurface( NULL, g_pPaddle1Surf, &g_Paddle1, g_pBackSurface, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ) );
	CopySurfaceToSurface( NULL, g_pPaddle2Surf, &g_Paddle2, g_pBackSurface, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ) );

	// Draw the Ball
	CopySurfaceToSurface( NULL, g_pBallSurf, &g_Ball, g_pBackSurface, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ) );

	// Lock the primary surface
	g_pBackSurface->LockRect( &Locked, 0, 0 );

	// Convert Player One's score from an int to a string
	char p1_TextScore[5], p1_Output[30] = "Player 1: ";
	itoa( g_p1Score, p1_TextScore, 10 );
	strcat( p1_Output, p1_TextScore );

	// Convert Player Two's score from an int to a string
	char p2_TextScore[5], p2_Output[30] = "Player 2: ";
	itoa( g_p2Score, p2_TextScore, 10 );
	strcat( p2_Output, p2_TextScore );

	// Print the scores
	PrintString( 10, 10, p1_Output, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
	PrintString( ( RES_WIDTH - 106 ), 10, p2_Output, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

	// Program Heading
	PrintString( ( ( RES_WIDTH / 2 ) - 104 ), 10, "UBER-PONG by Sean Gilleran", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

	// DEBUG INFORMATION
	// Print FPS to Screen
	PrintString( ( RES_WIDTH - 92 ), ( RES_HEIGHT - 26 ), "FPS: ", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
	PrintFrameRate( ( RES_WIDTH - 42 ), ( RES_HEIGHT - 26 ), (DWORD*)Locked.pBits, Locked.Pitch );

	// Print Ball Speed to the screen
	char BallSpeed[5];
	itoa( g_BallSpeed, BallSpeed, 10 );
	PrintString( 10, ( RES_HEIGHT - 26 ), "Ball Speed: ", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
	PrintString( 106, ( RES_HEIGHT - 26 ), BallSpeed, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

	// Prints bounce count to the screen
	char BounceCount[5];
	itoa( g_BounceCount, BounceCount, 10 );
	PrintString( ( ( RES_WIDTH / 2 ) - 64 ), ( RES_HEIGHT - 26 ), "Bounce Count: ", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
	PrintString( ( ( RES_WIDTH / 2 ) + 48 ), ( RES_HEIGHT - 26 ), BounceCount, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

	// Player One Wins
	if( g_p1Score >= MAX_SCORE )
	{
		PrintString( ( ( RES_WIDTH / 2 ) - 72 ), ( ( RES_HEIGHT / 2 ) - 18 ), "PLAYER ONE WINS!!!", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
		PrintString( ( ( RES_WIDTH / 2 ) - 88 ), ( ( RES_HEIGHT / 2 ) + 18 ), "Press Start to Quit...", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

		if( GetAsyncKeyState( START ) )
			PostQuitMessage( 0 );

		g_PlayWinSound--;
	}

	// Player Two Wins
	else if( g_p2Score >= MAX_SCORE )
	{
		PrintString( ( ( RES_WIDTH / 2 ) - 72 ), ( ( RES_HEIGHT / 2 ) - 18 ), "PLAYER TWO WINS!!!", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );
		PrintString( ( ( RES_WIDTH / 2 ) - 88 ), ( ( RES_HEIGHT / 2 ) + 18 ), "Press Start to Quit...", TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), (DWORD*)Locked.pBits, Locked.Pitch );

		if( GetAsyncKeyState( START ) )
			PostQuitMessage( 0 );

		g_PlayWinSound--;
	}

	// Unlock the surface
	g_pBackSurface->UnlockRect();
	
	// Transfer back buffer to primary display memory
	r = g_pDevice->Present( NULL, NULL, NULL, NULL );

	if( g_PlayWinSound == 1 && ( g_p1Score >= MAX_SCORE || g_p2Score >= MAX_SCORE ) )
			// PlaySound( "sound\\win.wav", NULL, SND_FILENAME | SND_SYNC );

	return S_OK;
}

// Finds a completely random direction for the ball to move
void RandomDirection( )
{
	if( ( rand( ) % 2 ) == 1)
		MultiplierX = 1;
	else
		MultiplierX = -1;

	if( ( rand( ) % 2 ) == 1 )
		MultiplierY = 1;
	else
		MultiplierY = -1;
}

// Moves the ball and checks for a score or paddle
void MoveBall( )
{
	if( g_Ball.x <= 0 )
	{
		g_p2Score++;
		// PlaySound( "sound\\score.wav", NULL, SND_FILENAME | SND_ASYNC );
		Pause( 250 );
		g_Ball.x += 5;
		MultiplierX = 1;
	}
	else if( g_Ball.x >= ( RES_WIDTH - BALL_WIDTH ) )
	{
		g_p1Score++;
		// PlaySound( "sound\\score.wav", NULL, SND_FILENAME | SND_ASYNC );
		Pause( 250 );
		g_Ball.x -= 5;
		MultiplierX = -1;
	}
	if( g_Ball.y <= 0 )
	{
		// PlaySound( "sound\\wall.wav", NULL, SND_FILENAME | SND_ASYNC );
		MultiplierY = 1;
	}
	else if( g_Ball.y >= ( RES_HEIGHT - BALL_HEIGHT ) )
	{
		// PlaySound( "sound\\wall.wav", NULL, SND_FILENAME | SND_ASYNC );
		MultiplierY = -1;
	}

	// Paddle RECT structs
	RECT Paddle1Rect = { g_Paddle1.x, g_Paddle1.y, ( g_Paddle1.x + PADDLE_WIDTH ), ( g_Paddle1.y + PADDLE_HEIGHT ) };
	RECT Paddle2Rect = { g_Paddle2.x, g_Paddle2.y, ( g_Paddle2.x + PADDLE_WIDTH ), ( g_Paddle2.y + PADDLE_HEIGHT ) };

	if( PtInRect( &Paddle1Rect, g_Ball ) )
	{
		// PlaySound( "sound\\wall.wav", NULL, SND_FILENAME | SND_ASYNC );
		MultiplierX *= -1;
		g_Ball.x += 10;
		g_BounceCount++;
	}

	// New ball point for the second paddle
	POINT Paddle2Ball = { g_Ball.x + BALL_WIDTH, g_Ball.y + BALL_HEIGHT };

	if( PtInRect( &Paddle2Rect, Paddle2Ball ) )
	{
		// PlaySound( "sound\\wall.wav", NULL, SND_FILENAME | SND_ASYNC );
		MultiplierX *= -1;
		g_Ball.x -= 10;
		g_BounceCount++;
	}

	/*
	// Increase ball speed after 10 reflects
	if( g_BounceCount == 10 )
	{
		g_BallSpeed++;
		g_BounceCount = 0;
	}
	*/

	g_Ball.x += ( MultiplierX * g_BallSpeed );
	g_Ball.y += ( MultiplierY * g_BallSpeed );
}