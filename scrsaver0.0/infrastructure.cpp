#include <windows.h>

#include <scrnsave.h>
#include "resource.h"

#include <stdio.h>
#include <string>
// opengl headers
#include <gl/gl.h>
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This global stores the last time that the WM_PAINT function was called.
// More on it later.
DWORD g_LastTime = 0;
// The default frame rate for the screensaver.  Really this is the minimum 
// number of milliseconds to lock the loop execution to.
DWORD g_FrameRate = 30;
// global used for when we need to redraw the entire window next time.
bool g_RedrawAll = FALSE;

// window variables
HWND myHwnd;
HGLRC myRC;
HDC myDC;

// windowGL variables
long Height, Width;
double zDist;

// graphic variables
// set to false to turn off double buffering
bool doubleBuffer = true;
int maxSide = 30;
float zoom = -66.0;
void Draw(void);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

HRESULT YourInitialize(HWND hWnd);
HRESULT YourFinalize();
HRESULT YourMain();

// temporarily store the cursor position, not real important
POINT InitCursorPos;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Register sub class dialogs, which we don't have.
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT message, 
WPARAM wParam, LPARAM lParam)
{

	switch ( message ) 
	{

	case WM_INITDIALOG:

		//get configuration from the registry

		return TRUE;

	case WM_COMMAND:
		switch( LOWORD( wParam ) ) 
		{ 

			//handle various cases sent when controls
			//are checked, unchecked, etc

			//when OK is pressed, write the configuration
			//to the registry and end the dialog box

		case IDOK:
			//write configuration

			EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
			return TRUE; 

		case IDCANCEL: 
			EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
			return TRUE;   
		}

	}	//end command switch

	return FALSE; 
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Windows will call this function when it wants us to actually execute the
// screensaver.  This is the place where we do all our magic to draw on the
// screen.
LONG WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// is the password verified?
	BOOL canClose = TRUE;

	// totalTime will store how much time has elapsed from the last time Windows called
	// us with the WM_PAINT message to the current WM_PAINT message call.  We will
	// use this with the g_FrameRate to know whether we need to Sleep for a while so
	// as not to hog the cpu and to draw at a consistent frame rate.
	DWORD totalTime = ::GetTickCount();
	switch (msg)
	{
		// They are telling us to create our screensaver window.  Here we want to initialize
		// all data, libraries, etc., as well as get our settings and pass that in to
		// our code that uses it to do cool things.
	case WM_CREATE:
		OutputDebugString("WM_Create called\n");
		// get size of window we are called in to detect if we are being put in that little
		// preview window in display properties.  When your screensaver is installed, if a 
		// user selects it in the Display properties box, that little bitty preview window
		// will call this function on us with a *really* small client area window.  In order
		// to know this, we get the size of the client rectangle.  We may need to do some
		// special drawing or tweak parameters to fit stuff in this small area.
		RECT screenSize;
		GetClientRect(hWnd, &screenSize);
		// Initialize the g_LastTime variable with the current tick count.  We will be using
		// this variable to store the last time that we were called by Windows for
		// consistent frame rate drawing.
		g_LastTime = GetTickCount();
		// if we are in that little preview screen mode, we want to detect that and just
		// solution that drawing the same smaller. Put your solution here.
		if (screenSize.bottom < 300 && screenSize.right < 300) {
		}		
		// Call our YourInitialize function and pass in the hWnd parameter Windows 
		// gave us as our windows handle; in it we will initialize our opengl graphics
		// Note that if we want to use the custom settings the user has set we could
		// pass in the cmdParam variable to this function as well.  In this simple example
		// we do not pass it in.
		YourInitialize(hWnd);

		return 0;
	case WM_DESTROY:
		// You should free libraries, delete memory, and cleanup whatever you have allocated here.
		YourFinalize();
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		OutputDebugString("WM_CLOSE called\n");
		// Windows is telling our screensaver to close, so validate password.
		SendMessage(hWnd,WM_SETCURSOR,0,0);
		SendMessage(hWnd,WM_SETCURSOR,0,0); 
		GetCursorPos(&InitCursorPos);
		if (canClose) {
			// Password validated, so call DestroyWindow which will make
			// Windows call us again with the WM_DESTROY message.
			OutputDebugString("WM_CLOSE: doing a DestroyWindow"); 
			DestroyWindow(hWnd);
		}
		else {
			OutputDebugString("WM_CLOSE: but failed password, so doing nothing");
		}
		return 0;
	case WM_PAINT:
		// Ah yes, Windows will call this on us over and over again very rapidly.  We
		// will do all our drawing here.  However, we want to lock the frame rate so
		// it is consistent and not hog the cpu, so get the last time that we were
		// called, compare it to the current time, and then sleep if less time has
		// elapsed than the desired frame rate.
		// To make the screensaver run faster, DECREASE g_FrameRate.  To make it
		// run slower, INCREASE g_FrameRate.
		totalTime = GetTickCount() - g_LastTime;
		if (totalTime < g_FrameRate) {
			// Not enough time has elapsed, so go to Sleep for the remaining time.
			return 0;
		}
		// Call your main screensaver logic and drawing function here. YourMain();
		YourMain();
		// save off this current time so that the next time Windows calls us we can
		// compare again.
		g_LastTime = GetTickCount();
		return 0;
	case WM_ERASEBKGND:
		// this handles the cases when something gets drawn behind us like annoying windows xp
		// "You have inactive notifications" or "updates ready" then erases our background, so
		// we have to redraw all otherwise graphics look bad.
		// Devin's note: You may or may not care about this WM_ERASEBKGND, but it messed me
		// up pretty bad for a while.  I was not using double buffered drawing, but rather
		// completely controlling what was drawn on the screen and erasing it myself. Well,
		// Windows called this on me and erase my background automatically.  Why? Because
		// when I created my window I passed in a background brush like this:
		// _class.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		// To be safe, if it makes sense for your program, tell it to redraw everything here.
		g_RedrawAll = TRUE;
		return 0;
	}
	// All the unprocessed messages go there, to be dealt in some default way
	return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Initialize opengl graphics here.
HRESULT YourInitialize(HWND hWnd)
{
	HRESULT result = S_OK;
	// save off this hwnd; this is the window we will be working with, drawing to, etc.
	myHwnd = hWnd;
	// get a windows device context from this hwnd.  We need to get one of these and
	// pass it to opengl so we can get a special opengl render context.
	myDC = GetDC(myHwnd);
	// Initialize the OpenGL pixel format
	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering // remove this tag for simple buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		24,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	// ask windows to give us a pixel format that matches our specs
	int iFormat = ChoosePixelFormat(myDC, &pfd);

	// set this pixel format we got back from windows.
	if (!SetPixelFormat(myDC, iFormat, &pfd))
	{
		MessageBox(NULL,"Set Pixel Format Failed.","SETUP ERROR",MB_OK | MB_ICONINFORMATION);
		return FALSE;	
	}
	// create opengl render context from the device context.
	if (!(myRC=wglCreateContext(myDC)))				// Are We Able To Get A Rendering Context?
	{
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}  
	// make context current
	if(!wglMakeCurrent(myDC,myRC))					// Try To Activate The Rendering Context
	{
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	} 
	// get screen size
	RECT screenSize;
	GetClientRect(myHwnd, &screenSize);
	Height = screenSize.bottom;
	Width = screenSize.right;

	// Reset The Current Viewport
	glViewport(0,0,Width,Height);						
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height, 0.1, 100);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix

	// Configure graphics
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	//sets specular highlight
	GLfloat spec[]={1.0f, 1.0f ,1.0f ,1.0f};      
	glMaterialfv(GL_FRONT,GL_SPECULAR,spec);
	float df=100.0;   
	glMaterialf(GL_FRONT,GL_SHININESS,df);

	// position of light source, a full screen width to the left,
	// then up a full screen height, at 0 distance in the z direction,
	// so right above the camera, and with a "w" value of 1.0, indicating
	// a non-directional light source.  Set this last parameter to 0
	// for a directional light source.
	GLfloat posl[]={-Width,Height,0.0, 1.0};
	GLfloat amb[]={0.1f, 0.1f, 0.1f ,1.0f};   //global ambient
	GLfloat amb2[]={0.3f, 0.3f, 0.3f ,1.0f};  //ambient of lightsource

	// enable lighting in general
	glEnable(GL_LIGHTING);
	// set light position for this specific light
	glLightfv(GL_LIGHT0,GL_POSITION,posl);
	// set ambient of light, how much light bounces off objects to hit other objects.
	glLightfv(GL_LIGHT0,GL_AMBIENT,amb2);
	// enable this particular light
	glEnable(GL_LIGHT0);

	// enable ambient, and material properties for objects we draw
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

	if (!doubleBuffer) {
		// enable scissor testing for window updates to only a portion of the entire screen
		glEnable(GL_SCISSOR_TEST);
		// since we're not double buffering, make sure everything is clear and black to
		// start with
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
		glLoadIdentity();								// Reset The View Before We Draw
	}
	else {
		// for double buffering, in our YourMain function we will translate into the
		// screen each time we're called.  We must do this each time since we draw to
		// a new buffer each loop, then flip the buffer to the screen with SwapBuffers.
	}
	// we are going to set the frame rate to only once every 1 second.  Normally this
	// will be MUCH faster, like 30 times per second.
	// 1 seconds * 1000 milliseconds / 1 second = 1000 milliseconds to wait in between
	// each frame.  For "normal" fast frame rate, select 30.
	g_FrameRate = 1*1000;
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// here we do all our logic and drawing.
HRESULT YourMain()
{
	HRESULT result = S_OK;
	if (doubleBuffer) {
		// Erase the previous image
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Reset The View Before We Draw our new scene
		glLoadIdentity();								

		// draw stuff here.
		// we are going to draw some random stuff each time.
		for (int i = 0; i < 5; i++) {
			Draw();
		}
	}
	else {
		// non double buffering, draw your stuff here.
		// Note that it will not erase automatically like with double buffering.
		// You must erase it somehow.  See www.heroicvirtuecreations.com/MakingAbound.html
		// for some tips on this.
		Draw();
	}
	// flush opengl pipeline since we have finished drawing
	glFlush();
	if (doubleBuffer) {
		// for double buffering, swap the back buffer where all our drawing has been
		// to the display so the user will see it.
		SwapBuffers(myDC);
	}
	return result;
}

HRESULT YourFinalize()
{
	if (myRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(myRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		myRC=NULL;										// Set RC To NULL
	}

	if (myDC && !ReleaseDC(myHwnd,myDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		myDC=NULL;										// Set DC To NULL
	}

	if (myHwnd && !DestroyWindow(myHwnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		myHwnd=NULL;										// Set hWnd To NULL
	}
	return S_OK;
}

void Draw()
{
	// get a random color
	double red = rand()/(double)RAND_MAX;
	double green = rand()/(double)RAND_MAX;
	double blue = rand()/(double)RAND_MAX;

	// set the color
	glColor3d(red, green, blue);
	// we are going to do a translate here of the current position, so
	// push the current matrix onto the stack so when we leave this function
	// the current position will be restored after we call pop matrix
	glPushMatrix();
	// make side random from 1 to maxSide
	long side = (rand()/(double)RAND_MAX * (maxSide - 1)) + 1;
	// keeping random
	int diameter = 5*side;
	long x = rand()/(double)RAND_MAX * ((Height/20.0) - diameter) + side ;
	long y = rand()/(double)RAND_MAX * ((Width/20.0) - diameter) + side;
    // Drawing finally
	glTranslatef(x,y, zoom);
	glBegin(GL_QUADS);									        // Draw A Quad
		glVertex3f(-side, side, 0.0f);					// Top Left
		glVertex3f( side, side, 0.0f);					// Top Right
		glVertex3f( side,-side, 0.0f);					// Bottom Right
		glVertex3f(-side,-side, 0.0f);					// Bottom Left
	glEnd();	
	glPopMatrix();
}