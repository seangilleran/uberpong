//*********************************
// Uber-Pong by Sean Gilleran
// (C)2003 Anti-Mass Studios
// All rights reserved
//*********************************

HRESULT RestoreGraphics();

LPDIRECT3DSURFACE8 g_pBackSurface = 0;

int g_DeviceWidth = 0;
int g_DeviceHeight = 0;

const int MAX_CHARSPERLINE = 256;

#define MATCH(a, b) (!strcmp( a, b ))

D3DPRESENT_PARAMETERS g_SavedPresParams;

//====================================================
// Generic Functions
//====================================================

// Output an error to the debug window
void Debug( char* String )
{
	OutputDebugString( "ERROR: " );
	OutputDebugString( String );
	OutputDebugString( "\n" );
}

//====================================================
// D3D Initialization Code
//====================================================

// Initializes the Direct3D device
int InitDirect3DDevice( HWND hWndTarget, int Width, int Height, BOOL bWindowed, D3DFORMAT FullScreenFormat, LPDIRECT3D8 pD3D, LPDIRECT3DDEVICE8* ppDevice )
{
	// Structure to hold information about the rendering method
	D3DPRESENT_PARAMETERS d3dpp;
	// Structure to hold information about the current display mode
	D3DDISPLAYMODE d3ddm;

	HRESULT r = 0;
	
	if( *ppDevice )
		(*ppDevice)->Release();

	// Initialize the structure to 0
	ZeroMemory( &d3dpp, sizeof( D3DPRESENT_PARAMETERS ) );

	// Get the settings for the current display mode
	r = pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );
	if( FAILED( r ) )
	{
		Debug( "Could not get display adapter information" );
		return E_FAIL;
	}
	
	// The width of the back buffer in pixels
	d3dpp.BackBufferWidth = Width;
	// The height of the buffer in pixels
	d3dpp.BackBufferHeight = Height;
	// The format of the back buffer
	d3dpp.BackBufferFormat = bWindowed ? d3ddm.Format : FullScreenFormat;

	// The number of back buffers
	d3dpp.BackBufferCount = 1;

	// The type of multisampling
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	// The swap effect
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	
	// The handle to the window that we want to render to 
	d3dpp.hDeviceWindow = hWndTarget;	
	// Windowed or fullscreen
	d3dpp.Windowed = bWindowed;

	// Let Direct3D manage the depth buffer
	d3dpp.EnableAutoDepthStencil = TRUE;
	// Set the depth buffer format to 16 bits
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	
	// Use the default refresh rate available
	d3dpp.FullScreen_RefreshRateInHz = 0;
	
	// Present the information as fast as possible
	d3dpp.FullScreen_PresentationInterval = bWindowed ? 0 : D3DPRESENT_INTERVAL_IMMEDIATE;
	// Allow the back buffer to be accessed for 2D work
	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	// Acquire a pointer to IDirect3DDevice8
	r = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndTarget,
							D3DCREATE_SOFTWARE_VERTEXPROCESSING, 
							&d3dpp, ppDevice );
	if( FAILED( r ) )
	{
		Debug( "Could not create the render device" );
		return E_FAIL;
	}

	// Save global copies of the device dimensions
	g_DeviceHeight = Height;
	g_DeviceWidth = Width;

	// Save a copy of the pres params for use in device validation later
	g_SavedPresParams = d3dpp;

	return S_OK;
}

//====================================================
// 2D Rendering Functions
//====================================================

// Set a pixel to specified color
void SetPixel32( int x, int y, DWORD Color, int Pitch, DWORD* pData )
{
	// Make sure the pixel is within screen boundaries
	if( x > g_DeviceWidth || x < 0 )
		return;

	if( y > g_DeviceHeight || y < 0 )
		return;
		
	// Set the pixel
	pData[ ((Pitch/4) * y) + x ] = Color;
}

// Draw a rectangle
void Rectangle32( D3DRECT* pRect, DWORD Color, int Pitch, DWORD* pData )
{
	// Use local variables to avoid dereferencing inside the loop
	int y1 = pRect->y1;
	int y2 = pRect->y2;
	int x1 = pRect->x1;
	int x2 = pRect->x2;

	// Convert the pitch from bytes to DWORDS
	int Pitch32 = Pitch / 4;
	
	// Get the offset into the target
	DWORD Offset = y1 * Pitch32 + x1;

	// Loop for each row of the rectangle
	for( int y = y1 ; y < y2 ; y++ )
	{
		// Loop for each column in the image
		for( int x = x1 ; x < x2 ; x++ )
		{
			// Set the pixel to the correct color
			pData[ Offset + x ] = Color;			
		}

		// Increment the offset to the next row of the rectangle
		Offset += Pitch32;
	}
}

// This uses hardware accelaration to draw a rectangle
void Rectangle32Fast( D3DRECT* pRect, DWORD Color, LPDIRECT3DDEVICE8 pDevice )
{
	pDevice->Clear( 1, pRect, D3DCLEAR_TARGET, Color, 0.0f, 0 );
}

// Loads a bitmap to a surface
int LoadBitmapToSurface( char* PathName, LPDIRECT3DSURFACE8* ppSurface, LPDIRECT3DDEVICE8 pDevice )
{
	HRESULT r;
	HBITMAP hBitmap;
	BITMAP Bitmap;
	
	// Load the bitmap first using the GDI to get info about it
	hBitmap = (HBITMAP)LoadImage( NULL, PathName, IMAGE_BITMAP, 0, 0, 
									LR_LOADFROMFILE | LR_CREATEDIBSECTION );
	if( hBitmap == NULL )
	{
		// The file probably does not exist
		Debug( "Unable to load bitmap" );
		return E_FAIL;
	}

	// Get information about the object
	GetObject( hBitmap, sizeof( BITMAP ), &Bitmap );
	// Unload the bitmap from memory
	DeleteObject( hBitmap );

	// Create a surface using the information gained from the previous load
	r = pDevice->CreateImageSurface( Bitmap.bmWidth, Bitmap.bmHeight, D3DFMT_X8R8G8B8, ppSurface );
	if( FAILED( r ) )
	{
		Debug( "Unable to create surface for bitmap load" );
		return E_FAIL;
	}

	// Load the image again, this time using Direct3D to load it directly to the new surface
	r = D3DXLoadSurfaceFromFile( *ppSurface, NULL, NULL, PathName, 
									NULL, D3DX_FILTER_NONE, 0, NULL );
	if( FAILED( r ) )
	{
		Debug( "Unable to load file to surface" );
		return E_FAIL;
	}

	return S_OK;
}

// Copy a surface to another surface (with transparency!)
HRESULT CopySurfaceToSurface( RECT* pSourceRect, LPDIRECT3DSURFACE8 pSourceSurf, POINT* pDestPoint, LPDIRECT3DSURFACE8 pDestSurf, BOOL bTransparent, D3DCOLOR ColorKey )
{
	// Holds error return values
	HRESULT r = 0;	
	
	// Holds information about the surfaces when they are locked
	D3DLOCKED_RECT LockedSource;
	D3DLOCKED_RECT LockedDest;

	// Make sure the source surface is valid
	if( !pSourceSurf )
		return E_FAIL;

	// Make sure the destination surface is valid
	if( !pDestSurf )
		return E_FAIL;

	// The source rectangle
	RECT SourceRect;
	// The destination point
	POINT DestPoint;

	// Holds information about the source surface
	D3DSURFACE_DESC d3dsdSource;
	// Get information about the source surface
	pSourceSurf->GetDesc( &d3dsdSource );

	// Holds information about the destination surface
	D3DSURFACE_DESC d3dsdDest;
	// Get information about the destination surface
	pDestSurf->GetDesc( &d3dsdDest );

	// If a source rectangle was specified then copy it into the local RECT structure
	if( pSourceRect )
		SourceRect = *pSourceRect;
	else
		// Otherwise set the rectangle to encompas the entire source surface
		SetRect( &SourceRect, 0, 0, d3dsdSource.Width, d3dsdSource.Height );
	
	// If a destination point was specified then copy it to the local POINT structure
	if( pDestPoint )
		DestPoint = *pDestPoint;
	else
	{
		// Otherwise set the point to (0,0)
		DestPoint.x = DestPoint.y = 0;
	}

	// Lock the source surface.
	r = pSourceSurf->LockRect( &LockedSource, 0, D3DLOCK_READONLY  );
	if( FAILED( r ) )
		// Fatal Error
		return E_FAIL;

	// Lock the destination surface
	r = pDestSurf->LockRect( &LockedDest, 0, 0 );
	if( FAILED( r ) )
	{
		pSourceSurf->UnlockRect();
		return E_FAIL;
	}

	// Modify the pitch to be DWORD compatible
	LockedSource.Pitch /= 4;
	LockedDest.Pitch /= 4;

	// Get 32bit pointers to the surface data
	DWORD* pSourceData = (DWORD*)LockedSource.pBits;
	DWORD* pDestData = (DWORD*)LockedDest.pBits;

	// Get the offset into the source surface
	int SourceOffset = SourceRect.top * LockedSource.Pitch + SourceRect.left;
	// Get the offset into the destination surface
	int DestOffset = DestPoint.y * LockedDest.Pitch + DestPoint.x;

	// Loop for each row of the source image
	for( int y = 0 ; y < SourceRect.bottom ; y++ )
	{
		// Loop for each column of the source image
		for( int x = 0 ; x < SourceRect.right ; x++ )
		{
			// If transparency was requested
			if( bTransparent )
			{
				if( pSourceData[ SourceOffset ] != ColorKey )
				{
					pDestData[ DestOffset ] = pSourceData[ SourceOffset ];
				}
			}
			else // Transparency was not requested
			{
				pDestData[ DestOffset ] = pSourceData[ SourceOffset ];
			}

			// Increment to the next pixel in the destination
			DestOffset++;
			// Increment to the next pixel in the source
			SourceOffset++;
		}
		
		SourceOffset += LockedSource.Pitch - SourceRect.right;
		DestOffset += LockedDest.Pitch - SourceRect.right;
	}

	// Copying is complete so unlock the surfaces
	pSourceSurf->UnlockRect();
	pDestSurf->UnlockRect();
	
	// Return success
	return S_OK;
}

//====================================================
// DC to Surface code
//====================================================

// Creates a GDI DC that can be copied to a surface later
HDC CreateD3DCompatibleDC( int Width, int Height, HBITMAP* phDibSection )
{
	// Create a new DC that is compatible with the current display mode
	HDC hDC = CreateCompatibleDC(0);

	// Temp pointer to the DIB data
	void* pDibSection = 0;

	// Structure to hold information about the bitmap
	BITMAPINFO bi;
	ZeroMemory( &bi, sizeof( BITMAPINFO ) );

	// The size of this structure in bytes
	bi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	// The width of the new bitmap
	bi.bmiHeader.biWidth = Width;
	// The height of the new bitmap( negative for a top down image )
	bi.bmiHeader.biHeight = -Height;
	// Obselete - just set to one
	bi.bmiHeader.biPlanes = 1;
	// The bit depth of the surface
	bi.bmiHeader.biBitCount = 32;
	// The compression format.  BI_RGB indicates none
	bi.bmiHeader.biCompression = BI_RGB;
	
	// Create a new DIB
	HBITMAP hDibSection = CreateDIBSection( hDC, &bi, DIB_RGB_COLORS, &pDibSection, NULL, NULL );

	// Select the bitmap into the new DC
	SelectObject( hDC, hDibSection );

	// Update the pointer to the handle to the bitmap
	*phDibSection = hDibSection;

	// Return the handle to the device context
	return hDC;
}

// Copies a GDI DC to Direct3D surface
HRESULT CopyDCToSurface( LPDIRECT3DSURFACE8 pDestSurf, POINT* pDestPoint, HDC hDCSource, HBITMAP hDibSection, RECT* pSrcRect, COLORREF ColorKey )
{
	HRESULT r = 0;
	
	// The source rectangle
	RECT SourceRect;
	// The destination origin point
	POINT DestPoint;

	// Holds information about the bitmap
	DIBSECTION DibSection;

	// Holds information about the surface when locked
	D3DLOCKED_RECT LockedRect;

	// Get information about the bitmap in the DC
	GetObject( hDibSection, sizeof( DIBSECTION ), &DibSection );

	int SrcTotalWidth = DibSection.dsBm.bmWidth;
	int SrcTotalHeight = DibSection.dsBm.bmHeight;

	// If no source rectangle was specified then this indicates that the entire bitmap in the DC is to be copied
	if( !pSrcRect )
		SetRect( &SourceRect, 0, 0, SrcTotalWidth, SrcTotalWidth);
	else
		SourceRect = *(pSrcRect);

	// If no destination point was specified then the origin is set to (0,0)
	if( !pDestPoint )
		DestPoint.x = DestPoint.y = 0;
	else
		DestPoint = *(pDestPoint);

	// Return failure if a valid destination surface was not specified
	if( !pDestSurf )
		return E_FAIL;

	// Return failure if a valid source DC was not specified
	if( !hDCSource )
		return E_FAIL;

	// Lock the source surface
	r = pDestSurf->LockRect( &LockedRect, 0, 0  );
	if( FAILED( r ) )
	{
		Debug( "Unable to lock the surface for GDI transfer" );
		return E_FAIL;
	}

	D3DSURFACE_DESC d3dsd;
	pDestSurf->GetDesc( &d3dsd );

	if( (UINT)(SourceRect.bottom + DestPoint.y) > (UINT)d3dsd.Height )
		SourceRect.bottom = d3dsd.Height;

	if( (UINT)(SourceRect.right + DestPoint.x) > (UINT)d3dsd.Width )
		SourceRect.right = d3dsd.Width;

	// Convert the source and destination data pointers to DWORD( 32 bit) values
	DWORD* pSrcData = (DWORD*)(DibSection.dsBm.bmBits);
	DWORD* pDestData = (DWORD*)(LockedRect.pBits);

	// Convert the pitch to a 32 bit value
	int Pitch32 = LockedRect.Pitch/4;

	// Compute the dimensions for the copy
	int SrcHeight = SourceRect.bottom - SourceRect.top;
	int SrcWidth = SourceRect.right - SourceRect.left;

	// Compute the index into memory
	DWORD SrcOffset = SourceRect.top * SrcTotalWidth + SourceRect.left;
	DWORD DestOffset = DestPoint.y * Pitch32 + DestPoint.x;

	// If not using a color key then a faster copy can be done
	if( ColorKey == -1 )
	{
		// Loop for each row in the image
		for( int y = 0 ; y < SrcHeight ; y++ )
		{
			// Copy this line of the image
			memcpy( (void*)&(pDestData[ DestOffset ]), (void*)&(pSrcData[ SrcOffset ]), SrcWidth*4);

			// Increase the destation pointer by the pitch
			DestOffset+=Pitch32;
			// Increase the source pointer by the total width
			SrcOffset +=SrcTotalWidth;
		}
	}
	else	// a color key was specified
	{
		// Loop for each row in the image
		for( int y = 0 ; y < SrcHeight ; y++ )
		{
			// Loop for each column
			for( int x = 0 ; x < SrcWidth ; x++ )
			{
						
				// If the source pixel is not the same as the color key
				if( pSrcData[ SrcOffset ] != ColorKey )
					// Then copy the pixel to the destination
					pDestData[ DestOffset ] = pSrcData[ SrcOffset ];

				// Move to the next pixel in the source
				SrcOffset++;
				// Move to the next pixel in the destination
				DestOffset++;
			}
			
			DestOffset += Pitch32 - SrcWidth;
		}
	}

	// Unlock the surface
	pDestSurf->UnlockRect();

	// Return success
	return S_OK;
}

// Deletes a GDI DC that was created with CreateD3DCompatibleDC()
void DeleteD3DCompatibleDC( HDC hDC, HBITMAP hDibSection )
{
	// Delete the bitmap
	DeleteObject( hDibSection );
	// Delete the DC
	DeleteDC( hDC );
}

//====================================================
// Device Validation Code
//====================================================

LPDIRECT3D8 g_pD3D = 0;
LPDIRECT3DDEVICE8 g_pDevice = 0;

// Call every frame to check if the device is valid.  If it is not then the it is reaquired if possible
HRESULT ValidateDevice()
{
	HRESULT r = 0;
	
	// Test the current state of the device
	r = g_pDevice->TestCooperativeLevel();
	if( FAILED( r ) )
	{
		// If the device is lost then return failure
		if( r == D3DERR_DEVICELOST )
			return E_FAIL;

		// If the device is ready to be reset then attempt to do so
		if( r == D3DERR_DEVICENOTRESET )
		{
			// Release the back surface so it can be recreated
			g_pBackSurface->Release();

			// Reset the device
			r = g_pDevice->Reset( &g_SavedPresParams );
			if( FAILED( r ) )
			{
				// If the device was not reset then exit
				Debug( "Could not reset device" );
				PostQuitMessage( E_FAIL );
				return E_FAIL;
			}
			
			// Reaquire a pointer to the new back buffer
			r = g_pDevice->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &g_pBackSurface );
			if( FAILED( r ) )
			{
				Debug( "Unable to reaquire the back buffer" );
				PostQuitMessage( 0 );
				return E_FAIL;
			}
			g_pDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB( 0, 0, 0 ), 0.0f, 0 );

			RestoreGraphics();
		}
	}

	return S_OK;
}

// Use this function to reinit any surfaces that were lost when the device was lost.
HRESULT RestoreGraphics()
{
	
	return S_OK;
}

//====================================================
// Font Engine Code
//====================================================

int g_AlphabetWidth = 0;			// The width of the Alphabet bitmap
int g_AlphabetHeight = 0;			// The height of the Alphabet bitmap
int g_AlphabetLetterWidth = 0;		// The width of a letter
int g_AlphabetLetterHeight = 0;		// The height of a letter
int g_AlphabetLettersPerRow = 0;	// The number of letters per row

// The surface holding the alphabet bitmap
LPDIRECT3DSURFACE8 g_pAlphabetSurface = 0;

// Has the alphabet bitmap been loaded yet?
BOOL g_bAlphabetLoaded = FALSE;

// Used to load an alphabet bitmap into memory
HRESULT LoadAlphabet( char* strPathName, int LetterWidth, int LetterHeight )
{
	// Make sure a valid path was specified
	if( !strPathName )
		return E_FAIL;

	// Make sure the size of the letters is greater than 0
	if( !LetterWidth || !LetterHeight )
		return E_FAIL;

	HRESULT r = 0;
	
	// Load the bitmap into memory
	r = LoadBitmapToSurface( strPathName, &g_pAlphabetSurface, g_pDevice );
	if( FAILED( r ) )
	{
		Debug( "Unable to load alphabet bitmap" );
		return E_FAIL;
	}

	// Holds information about the alpahbet surface
	D3DSURFACE_DESC d3dsd;

	// Get information about the alphabet surface
	g_pAlphabetSurface->GetDesc( &d3dsd );

	// Update globals with the letter dimensions
	g_AlphabetWidth = d3dsd.Width;			
	g_AlphabetHeight = d3dsd.Height;
	g_AlphabetLetterWidth = LetterWidth;
	g_AlphabetLetterHeight = LetterHeight;

	// Compute the number of letters in a row
	g_AlphabetLettersPerRow = g_AlphabetWidth / g_AlphabetLetterWidth;

	// Set the loaded flag to TRUE
	g_bAlphabetLoaded = TRUE;

	return S_OK;
}

// Unloads the alphabet from memory
HRESULT UnloadAlphabet()
{
	// Check if the alphabet exists
	if( g_pAlphabetSurface )
	{
		// Release the surface
		g_pAlphabetSurface->Release();
		// NULL the pointer
		g_pAlphabetSurface = 0;
		// Set the loaded flag to FALSE
		g_bAlphabetLoaded = FALSE;
	}

	return S_OK;
}

// Print a character to a surface using the loaded alphabet
void PrintChar( int x, int y, char Character, BOOL bTransparent, D3DCOLOR ColorKey, DWORD* pDestData, int DestPitch )
{
	HRESULT r = 0;
	
	div_t Result;	// Holds the result of divisions

	// The offset into the alphabet image
	int OffsetX = 0, OffsetY = 0;

	POINT LetterDestPoint = { 0, 0 };	// The destination point for the letter
	RECT LetterRect = { 0, 0, 0, 0 };	// The source rectangle for the letter

	// If the alphabet has not been loaded yet then exit
	if( !g_bAlphabetLoaded )
		return;

	// The characters are specified in ASCII code, which begins at 32 so we want to decrement this value by 32 to make it zero based
	Character -= 32;

	// Avoid divide by 0 errors
	if( Character == 0 )
		return;

	// Divide the character code by the number of letters per row.
	// The quotient will help get the vertical offset and the remainder will help get the horizontal offset
	Result = div( Character, g_AlphabetLettersPerRow );

	// Get the horizontal offset by multiplying the remainder by the width of the Letter
	OffsetX = Result.rem * g_AlphabetLetterWidth;
	// Get the vertical offset by multiplying the quotient by the height of the letter
	OffsetY = Result.quot * g_AlphabetLetterHeight;

	// Fill in the source rectangle with the computed offsets
	SetRect( &LetterRect, OffsetX, OffsetY, 
	OffsetX + g_AlphabetLetterWidth, OffsetY + g_AlphabetLetterHeight );
	
	// Fill in the destination point
	LetterDestPoint.x = x;
	LetterDestPoint.y = y;
	
	D3DLOCKED_RECT LockedAlphabet;	// Holds info about the alphabet surface

	// Lock the source surface
	r = g_pAlphabetSurface->LockRect( &LockedAlphabet, 0, D3DLOCK_READONLY  );
	if( FAILED( r ) )
	{
		Debug( "Couldnt lock alphabet surface for PrintChar()" );
		return;
	}
	
	// Get a DWORD pointer to each surface
	DWORD* pAlphaData = (DWORD*)LockedAlphabet.pBits;

	// Convert the BYTE pitch pointer to a DWORD ptr
	LockedAlphabet.Pitch /=4;
	DestPitch /= 4;

	// Compute the offset into the alphabet
	int AlphaOffset = OffsetY * LockedAlphabet.Pitch + OffsetX;
	// Compute the offset into the destination surface
	int DestOffset = y * DestPitch + x;

	// Loop for each row in the letter
	for( int cy = 0 ; cy < g_AlphabetLetterHeight ; cy++ )
	{
		// Loop for each column in the letter
		for( int cx = 0 ; cx < g_AlphabetLetterWidth ; cx++ )
		{
			if( bTransparent )
			{
				// If this alphabet pixel is not transparent
				if( pAlphaData[ AlphaOffset ] != ColorKey )
				{
					// Then copy the pixel to the destination
					pDestData[ DestOffset ] = pAlphaData[ AlphaOffset ];
				}

				// Increment the offsets to the next pixel
				AlphaOffset++;
				DestOffset++;
			}
			else
				pDestData[ DestOffset ] = pAlphaData[ AlphaOffset ];
		}

		// Move the offsets to the start of the next row
		DestOffset += DestPitch - g_AlphabetLetterWidth;
		AlphaOffset += LockedAlphabet.Pitch - g_AlphabetLetterWidth;
	}
	
	// Unlock the surface
	g_pAlphabetSurface->UnlockRect();
	
}



void PrintString( int x, int y, char* String, BOOL bTransparent, D3DCOLOR ColorKey, DWORD* pDestData, int DestPitch )
{
	// Loop for each character in the string
	for( UINT i = 0 ; i < strlen( String ) ; i++ )
	{
		// Print the current character
		PrintChar( x + (g_AlphabetLetterWidth * i), y, String[i], 
							bTransparent, ColorKey, pDestData, DestPitch );
	}	
}

//====================================================
// Timing Code
//====================================================

// The number of high performance ticks per second
INT64 g_Frequency = 0;
// The number of elapsed frames this counting period
int g_FrameCount = 0;
// The number of elapsed frames this second
int g_FrameRate = 0;

HRESULT InitTiming()
{
	// Get the number of counts per second
	QueryPerformanceFrequency( (LARGE_INTEGER*)&g_Frequency );

	// If the frequency is 0 then this system does not have high performance timers
	if( g_Frequency == 0 )
	{
		Debug( "The system does not support high resolution timing" );
		return E_FAIL;
	}

	return S_OK;
}
	
void FrameCount()
{
	INT64 NewCount = 0;			// The current count
	static INT64 LastCount = 0;	// The last count
	INT64 Difference = 0;		// The differnce since the last count

	// Get the current count
	QueryPerformanceCounter( (LARGE_INTEGER*)&NewCount );
	
	// If the count is 0 then this system does not have high performance timers
	if( NewCount == 0 )
		Debug( "The system does not support high resolution timing" );

	// Increase the frame count
	g_FrameCount++;
	
	// Compute the difference since the last count
	Difference = NewCount - LastCount;
	
	// If more than a second has passed
	if( Difference >= g_Frequency )
	{
		// Record the number of elapsed frames
		g_FrameRate = g_FrameCount;
		// Reset the counter 
		g_FrameCount = 0;

		// Update the last count
		LastCount = NewCount;
	}

}

// Prints the frame rate to the screen
void PrintFrameRate( int x, int y, DWORD* pDestData, int DestPitch )
{
	char string[4];	// String to hold the frame rate
	
	// Zero out the string
	ZeroMemory( &string, sizeof( string ) );

	// Convert the frame rate to a string
	itoa( g_FrameRate, string, 10 );

	// Output the string to the back surface
	PrintString( x, y, string, TRUE, D3DCOLOR_ARGB( 0, 255, 0, 255 ), pDestData, DestPitch );
}