/****************************************************************************
 infrastructure.cpp
 
 Author   :   Javier Palmer
 Date     :   17/2/2005

*****************************************************************************/

#include "explosion.h"
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
// This the total mount of time
float elapsedTime[5];
// The default frame rate for the screensaver.  Really 30 is the minimum 
// number of milliseconds to lock the loop execution to, but we use 60 
// because this screensaver looks better slow.
DWORD g_FrameRate = 60;
// global used for when we need to redraw the entire window next time.
bool g_RedrawAll = FALSE;

// window variables
HWND myHwnd;
HGLRC myRC;
HDC myDC;

// windowGL variables
float Height, Width;
double zDist;

// graphic variables
bool doubleBuffer = true;                       // set to false to turn off double buffering
GLuint	filter;									// Which Filter To Use
GLuint	texture[3];								// Storage for 3 textures
CExplosion* explosion[5];                       // explosion object
CVector position(0.0,0.0,-66.0);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

HRESULT YourInitialize(HWND hWnd);
HRESULT YourFinalize();
HRESULT YourMain();
void CreateParticles(int i);

// temporarily store the cursor position, not real important
POINT InitCursorPos;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
double random()
{
	return ((double) rand()) / (double) RAND_MAX;
}

AUX_RGBImageRec *LoadBMP(char *Filename)			    // Loads A Bitmap Image
{
	FILE *File=NULL;							        // File Handle

	if (!Filename)								        // Make Sure A Filename Was Given
	{
		return NULL;							        // If Not Return NULL
	}

	File=fopen(Filename,"r");					      	// Check To See If The File Exists

	if (File)								            // Does The File Exist?
	{
		fclose(File);						        	// Close The Handle
		return auxDIBImageLoad(Filename);				// Load The Bitmap And Return A Pointer
	}
	return NULL;							        	// If Load Failed Return NULL
}

int LoadGLTextures()									// Load Bitmaps And Convert To Textures
{
	int Status=FALSE;									// Status Indicator

	AUX_RGBImageRec *TextureImage[1];					// Create Storage Space For The Texture

	memset(TextureImage,0,sizeof(void *)*1);           	// Set The Pointer To NULL

	// Load The Bitmap, Check For Errors, If Bitmap's Not Found Quit
	if (TextureImage[0]=LoadBMP("Data/EXPLOSION.bmp"))
	{
		Status=TRUE;									// Set The Status To TRUE

		glGenTextures(3, &texture[0]);					// Create Three Textures

		// Create Nearest Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// Create Linear Filtered Texture
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// Create MipMapped Texture
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
	}

	if (TextureImage[0])								// If Texture Exists
	{
		if (TextureImage[0]->data)						// If Texture Image Exists
		{
			free(TextureImage[0]->data);				// Free The Texture Image Memory
		}

		free(TextureImage[0]);							// Free The Image Structure
	}

	return Status;										// Return The Status
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

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
	DWORD totalTime = GetTickCount();
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
		// if we are in that little preview screen mode, we want to detect that
		if (screenSize.bottom < 300 && screenSize.right < 300) {
			// This screensaver can fail in preview windows !!. To avoid that just create a variable
			// to disable drawing during preview.
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
		24,									    	// Select Our Color Depth
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
	if (!LoadGLTextures())								// Jump To Texture Loading Routine
	{
		return FALSE;									// If Texture Didn't Load Return FALSE
	}

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping	
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	//	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	//	glDepthFunc(GL_LEQUAL);							// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);			        // Set The Blending Function For Translucency
	glEnable(GL_BLEND);		                            // Turn Blending On
	//sets specular highlight
	GLfloat spec[]={1.0f, 1.0f ,1.0f ,1.0f};      
	glMaterialfv(GL_FRONT,GL_SPECULAR,spec);
	float df=100.0;   
	glMaterialf(GL_FRONT,GL_SHININESS,df);

	//position of light source, a full screen width to the left,
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
	glEnable(GL_COLOR_MATERIAL);  //permite cambiar el color de texturas !!
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

	if (!doubleBuffer) {
		// enable scissor testing for window updates to only a portion of the entire screen
		glEnable(GL_SCISSOR_TEST);
		// since we're not double buffering, make sure everything is clear and black to
		// start with
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
		glLoadIdentity();								// Reset The View Before We Draw Each Explosion
	}
	else {
		// for double buffering, in our YourMain function we will translate into the
		// screen each time we're called.  We must do this each time since we draw to
		// a new buffer each loop, then flip the buffer to the screen with SwapBuffers.
	}

	for (int i = 0; i < 5; i++) {
		CreateParticles(i);
	    elapsedTime[i] = 0.0;	
	}
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
		glBindTexture(GL_TEXTURE_2D, texture[2]);

		for (int i = 0; i<5 ; i++)
		{
			if (explosion[i]->IsDead())
			{
				elapsedTime[i] = 0.0;
				CreateParticles(i);
			}
		    else
			{
				if(1){//elapsedTime[i] >= 5.0){
				explosion[i]->Update(elapsedTime,i);
				explosion[i]->Render();
				elapsedTime[i]+=0.1;
				}
			}
		}
	}
	else {
		// non double buffering, draw your stuff here.
		// Note that it will not erase automatically like with double buffering.
		// You must erase it somehow.  See www.heroicvirtuecreations.com/MakingAbound.html
		// for some tips on this.
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
	for (int i=0;i<5;i++){
		explosion[i]->KillSystem();
	}

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

void CreateParticles(int e)
{
	// get a random
	position.x = random();
	position.y = random();
	position.z = -10.0;
	float red = random();
	float green = random();
	float blue = random();
	explosion[e] = new CExplosion(30, position, 8.0, texture[2], red, green, blue);  

}