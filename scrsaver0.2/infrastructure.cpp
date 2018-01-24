//windows
#include <windows.h>
#include <scrnsave.h>
#include <stdio.h>
#include <string>
//opengl
#include <gl\gl.h>
#include <gl\glu.h>			
#include <gl\glaux.h>
//custom
#include "resource.h"
#include "infrastructure.h"
#include "3ds.h"

// window variables
HWND myHwnd;
HGLRC myRC;
HDC myDC;

// windowGL variables
long Height, Width;
double zDist;

//MACROS AND GLOBAL DATA		
#define unsignet int UINT
#define FILE_NAME  "face.3ds"							
UINT g_Texture[MAX_TEXTURES] = {0};						
CLoad3DS g_Load3ds;										
t3DModel g_3DModel;	
bool doubleBuffer = true;
int   g_ViewMode	  = GL_TRIANGLES;					
bool  g_bLighting     = true;						
float g_RotateX		  = 0.0f;						
float g_RotationSpeed = 0.1f;						
DWORD g_LastTime = 0;
DWORD g_FrameRate = 100;
bool g_RedrawAll = FALSE;
POINT InitCursorPos;

//functions
void Draw(void);
void CreateTexture(UINT textureArray[], LPSTR strFileName, int textureID);
HRESULT Initialize(HWND hWnd);
HRESULT Finalize();
HRESULT Main();

//functions implementation and body of the probram

//register sub class dialogs, which we don't have.
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	return TRUE;
}

//configure
BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch ( message ) 
	{
		
	case WM_INITDIALOG:
		return TRUE;
		
	case WM_COMMAND:
		switch( LOWORD( wParam ) ) 
		{ 
		case IDOK:
			EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
			return TRUE; 
			
		case IDCANCEL: 
			EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
			return TRUE;   
		}
		
	}
	
	return FALSE; 
}

//message handler. main function
LONG WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL canClose = TRUE;
	DWORD totalTime = ::GetTickCount();
	switch (msg)
	{
	case WM_CREATE:
		OutputDebugString("WM_Create called\n");
		RECT screenSize;
		GetClientRect(hWnd, &screenSize);
		g_LastTime = GetTickCount();
		//preview
		if (screenSize.bottom < 300 && screenSize.right < 300) 
		{
		}		
		Initialize(hWnd);
		return 0;
	case WM_DESTROY:
		Finalize();
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		OutputDebugString("WM_CLOSE called\n");
		SendMessage(hWnd,WM_SETCURSOR,0,0);
		SendMessage(hWnd,WM_SETCURSOR,0,0); 
		GetCursorPos(&InitCursorPos);
		if (canClose) 
		{
			OutputDebugString("WM_CLOSE: doing a DestroyWindow"); 
			DestroyWindow(hWnd);
		}
		else 
		{
			OutputDebugString("WM_CLOSE: but failed password, so doing nothing");
		}
		return 0;
	case WM_PAINT:
		//get timming for the animation
		totalTime = GetTickCount() - g_LastTime;
		if (totalTime < g_FrameRate) 
		{
			return 0;
		}
		//our main function
		Main();
		g_LastTime = GetTickCount();
		return 0;
	case WM_ERASEBKGND:
		g_RedrawAll = TRUE;
		return 0;
	}
	//unprocessed messages go there. dealt in default way
	return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}


HRESULT Initialize(HWND hWnd)
{
	//preparing
	HRESULT result = S_OK;
	myHwnd = hWnd;
	myDC = GetDC(myHwnd);
	
	//set pixel format
	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
			1,											// Version Number
			PFD_DRAW_TO_WINDOW |						// Format Must Support Window
			PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
			PFD_DOUBLEBUFFER,							// Must Support Double Buffering // remove this tag for simple buffering
			PFD_TYPE_RGBA,								// Request An RGBA Format
			24,									     	// Select Our Color Depth
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
	int iFormat = ChoosePixelFormat(myDC, &pfd);
	if (!SetPixelFormat(myDC, iFormat, &pfd))
	{
		MessageBox(NULL,"Set Pixel Format Failed.","SETUP ERROR",MB_OK | MB_ICONINFORMATION);
		return FALSE;	
	}
	
	//get rendering context
	if (!(myRC=wglCreateContext(myDC)))		
	{
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							
	}  
	if(!wglMakeCurrent(myDC,myRC))				
	{
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							
	} 
	//get screen size
	RECT screenSize;
	GetClientRect(myHwnd, &screenSize);
	Height = screenSize.bottom;
	Width = screenSize.right;
	
	//reset The Current Viewport
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
	
    //configure light
	GLfloat posl[]={-Width,Height,0.0, 1.0};
	GLfloat amb[]={0.1f, 0.1f, 0.1f ,1.0f};             //global ambient
	GLfloat amb2[]={0.3f, 0.3f, 0.3f ,1.0f};            //ambient of lightsource
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0,GL_POSITION,posl);
	glLightfv(GL_LIGHT0,GL_AMBIENT,amb2);
	glEnable(GL_LIGHT0);
	
	//enable ambient and material properties
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
	
	if (!doubleBuffer) 
	{
		glEnable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		glLoadIdentity();							
	}
	else 
	{
	}
	
	//time to load data
	g_Load3ds.Import3DS(&g_3DModel, FILE_NAME);			// Load our .3DS file into our model structure
	
	//preparing materials
	for(int i = 0; i < g_3DModel.numOfMaterials; i++)
	{
		if(strlen(g_3DModel.pMaterials[i].strFile) > 0)
		{
			CreateTexture(g_Texture, g_3DModel.pMaterials[i].strFile, i);			
		}
		
		g_3DModel.pMaterials[i].texureId = i;
	}
	
	//preparing animation
	g_FrameRate = 100;
	return result;
}

//our main program, where everything happends
HRESULT Main()
{
	HRESULT result = S_OK;
	if (doubleBuffer) 
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();								
		Draw();
	}
	else 
	{
		Draw();
	}
	glFlush();
	if (doubleBuffer) 
	{
		SwapBuffers(myDC);
	}
	return result;
}


HRESULT Finalize()
{
	if (myRC)										
	{
		if (!wglMakeCurrent(NULL,NULL))				
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		
		if (!wglDeleteContext(myRC))					
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		myRC=NULL;										
	}
	
	if (myDC && !ReleaseDC(myHwnd,myDC))					
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		myDC=NULL;									
	}
	
	if (myHwnd && !DestroyWindow(myHwnd))					
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		myHwnd=NULL;										
	}
	return S_OK;
}

// everything is drawn using this function
void Draw()
{
	gluLookAt(0, 1.5f, 8,	0, 0.5f, 0,	  0, 1, 0);
	
	glRotatef(g_RotateX, 0, 1.0f, 0);						// Rotate the object around the Y-Axis
	g_RotateX += g_RotationSpeed;							// Increase the speed of rotation
	
	//if many objects our model has, go through each of them.
	for(int i = 0; i < g_3DModel.numOfObjects; i++)
	{
		if(g_3DModel.pObject.size() <= 0)
			break;
		
		t3DObject *pObject = &g_3DModel.pObject[i];
		
		if(pObject->bHasTexture) 
		{
			glEnable(GL_TEXTURE_2D);
			glColor3ub(255, 255, 255);
			glBindTexture(GL_TEXTURE_2D, g_Texture[pObject->materialID]);
		} 
		else 
		{
			glDisable(GL_TEXTURE_2D);
			glColor3ub(255, 255, 255);
		}
		glBegin(g_ViewMode);				
		
		for(int j = 0; j < pObject->numOfFaces; j++)
		{
			for(int whichVertex = 0; whichVertex < 3; whichVertex++)
			{
				int index = pObject->pFaces[j].vertIndex[whichVertex];
				glNormal3f(pObject->pNormals[ index ].x, pObject->pNormals[ index ].y, pObject->pNormals[ index ].z);
				
				if(pObject->bHasTexture) 
				{
					if(pObject->pTexVerts) 
					{
						glTexCoord2f(pObject->pTexVerts[ index ].x, pObject->pTexVerts[ index ].y);
					}
				} 
				else 
				{
					
					if(g_3DModel.pMaterials.size() && pObject->materialID >= 0) 
					{
						BYTE *pColor = g_3DModel.pMaterials[pObject->materialID].color;
						glColor3ub(pColor[0], pColor[1], pColor[2]);
					}
				}
				
				glVertex3f(pObject->pVerts[ index ].x, pObject->pVerts[ index ].y, pObject->pVerts[ index ].z);
			}
		}
		
		glEnd();							
	}
}

void CreateTexture(UINT textureArray[], LPSTR strFileName, int textureID)
{
	AUX_RGBImageRec *pBitmap = NULL;
	
	if(!strFileName)								
		return;
	pBitmap = auxDIBImageLoad(strFileName);			
	if(pBitmap == NULL)									
		exit(0);
	
	glGenTextures(1, &textureArray[textureID]);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, textureArray[textureID]);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, pBitmap->sizeX, pBitmap->sizeY, GL_RGB, GL_UNSIGNED_BYTE, pBitmap->data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	
	if (pBitmap)									
	{
		if (pBitmap->data)							
		{
			free(pBitmap->data);						
		}
		
		free(pBitmap);								
	}
}