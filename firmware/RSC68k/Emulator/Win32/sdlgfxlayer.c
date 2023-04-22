
/* ************************************************************************* *\
** ************************************************************************* **
** Host Display stuffs
** ************************************************************************* **
\* ************************************************************************* */
#include <errno.h>
#include <stdlib.h>
#include "include/types.h"
#include "startup/app.h"
#include "win32/host.h"

static SDL_Surface *sg_psVideoSurface = NULL;

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/gl.h"

#pragma comment(lib, "opengl32.lib")

#include "Win32/SDL/sdl.h"
#include "Win32/SDL/sdl_syswm.h"
static BOOL sg_bLeftShiftHeld = FALSE;
static BOOL sg_bRightShiftHeld = FALSE;

#ifndef GLAPI
	#define GLAPI
#endif // GLAPI

#ifndef GL_APIENTRY
	#define GL_APIENTRY
#endif // GL_APIENTRY

//#define USE_SDL_GRAB
static BOOL s_bCapture = FALSE;

/*
 * Main switch about whether the app is 3d aware or not.
 * For 3D operation - if it is 3d aware it will manage
 * all drawing via th OpenGL Api, swaps should be done via GCWaitForVSync
 * For 2D operation -- If the app is not 3d aware ie it is a 2d app.
 */

CRITICAL_SECTION g_sVideoCriticalSection;
static void (*sg_pGameExitCallbackProc)(void) = NULL;

GfxLocals g_sGfxLocals = { 0 };

GLuint s_TempTexId = 0;

typedef struct
{
    GLuint X,Y;
    GLuint Width, Height;
} Rect;


typedef struct
{
    GLint Right, Left;
    GLint Top, Bottom;
} RectOffset;

typedef struct
{
    float FrameTimeS;

    // Viewport Offsets
    RectOffset ViewportOffsets;

    // Viewport
    Rect Viewport;

    // Projection Matrix
    GLfloat Projection[16];

    //
    GLuint	sHWTextureName;
    GLdouble	fTextureClip[2];

    Vertex TexRect[4];

} GL2dState;
static GL2dState s_sGl2dState = { 0 };

static Surface *sg_psDefaultBackbufferSurface = NULL;
static Surface *sg_psCurrentMemorySurface = NULL;

static void (*s_pFrameCallback)(void) = NULL;
static void (*s_pModeChangeCallback)(void) = NULL;


/* ************************************************************************* *\
** FUNCTION: DrawLine
\* ************************************************************************* */
static void DrawLine(int x1, int y1, int x2, int y2, 
					 float r, float g, float b)
{
	Vertex Line[2];

	Line[0].x = (float)x1;
	Line[0].y = (float)y1;
	Line[0].z = 0.0f;

	Line[1].x = (float)x2;
	Line[1].y = (float)y2;
	Line[1].z = 0.0f;

    // Draw a model with the texture on it
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &(Line[0].x));

	glColor3f(r, g, b);

	glDrawArrays(GL_LINES, 0, 2);
    
	glColor3f(1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_VERTEX_ARRAY);
}

/* ************************************************************************* *\
** FUNCTION: DrawBox
\* ************************************************************************* */
static void DrawBox(int x1, int y1, int x2, int y2, 
					 float r, float g, float b)
{
	DrawLine(x1, y1, x2, y1, r, g, b);
	DrawLine(x2, y1, x2, y2, r, g, b);
	DrawLine(x2, y2, x1, y2, r, g, b);
	DrawLine(x1, y2, x1, y1, r, g, b);
}


/* ************************************************************************* *\
** FUNCTION: DrawBounds
\* ************************************************************************* */
static void DrawBounds( void )
{
    glViewport(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);
	glScissor(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, g_sGfxLocals.ScreenWidth, 0, g_sGfxLocals.ScreenHeight, 0, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	DrawBox(
		0 + s_sGl2dState.ViewportOffsets.Left,
		0 + s_sGl2dState.ViewportOffsets.Bottom,
		(int)g_sGfxLocals.ScreenWidth - s_sGl2dState.ViewportOffsets.Right, 
		(int)g_sGfxLocals.ScreenHeight - s_sGl2dState.ViewportOffsets.Top, 
		1.0f, 1.0f, 0.0f);

	DrawBox(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
           s_sGl2dState.Viewport.X + s_sGl2dState.Viewport.Width, 
		   s_sGl2dState.Viewport.Y + s_sGl2dState.Viewport.Height, 
		   1.0f, 0.0f, 0.0f);
    
	glViewport(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
           s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
    glScissor(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
           s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );

}

/* ************************************************************************* *\
** ************************************************************************* **
** Surface Management
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: FreeSurface
\* ************************************************************************* */
static void FreeSurface(Surface* psSurface)
{
	if( NULL == psSurface )
	{
		return;
	}

	if( NULL != psSurface->pixels )
	{
		free(psSurface->pixels);
		psSurface->pixels = NULL;
	}

	free(psSurface);
}

/* ************************************************************************* *\
** FUNCTION: CreateRGBSurface
\* ************************************************************************* */
static Surface* CreateRGBSurface(UINT32 u32XSize, UINT32 u32YSize,
                                UINT8 u8ColorDepth)
{
	Surface* pNewSurface = NULL;

	// Allocate a new surface structure
	pNewSurface = (Surface*) malloc(sizeof(*pNewSurface));
	GCASSERT(pNewSurface);

	// Allocate space for the image data
	pNewSurface->pixels = (void*)malloc(u32XSize * u32YSize * (u8ColorDepth / 8));
	GCASSERT(pNewSurface->pixels);

	// pitch in this context is the number of bytes in image width
	pNewSurface->pitch = (UINT16)(u32XSize * (u8ColorDepth / 8));

	pNewSurface->w = u32XSize;
	pNewSurface->h = u32YSize;
	pNewSurface->u8ColorDepth = u8ColorDepth;

	return(pNewSurface);
}

/* ************************************************************************* *\
** ************************************************************************* **
** GLMangement Utils
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: Utility to print GL Error state if it exists
\* ************************************************************************* */

BOOL cglCheckErrorInternal( const char* in_pStr, int in_Line)
{
    GLenum err = GL_NO_ERROR; 
	BOOL hadErr = FALSE;
	if(wglGetCurrentContext() == NULL)
	{
//		DebugOut("cglCheckErrorInternal without context, %s:%d\n", in_pStr, in_Line);
		return FALSE;
	}

	do
	{
		err = glGetError();
		if(err != GL_NO_ERROR)
		{
			DebugOut("GLERROR(%X) @ %s:%d\n", err, in_pStr, in_Line);
			hadErr = TRUE;
		}
	} while (err != GL_NO_ERROR);

	return hadErr;
}

/* ************************************************************************* *\
** FUNCTION: GLDestroyOffScreenTexture
\* ************************************************************************* */
static BOOL GLDestroyOffScreenTexture( void )
{
    if(s_sGl2dState.sHWTextureName)
    {
		cglCheckErrorEx("GLDestroyOffScreenTexture-setCurrent");
        glDeleteTextures( 1, &s_sGl2dState.sHWTextureName );
		cglCheckErrorEx("GLDestroyOffScreenTexture-glDeleteTextures");
        s_sGl2dState.sHWTextureName = 0;
        cglCheckErrorEx("GLDestroyOffScreenTexture-setCurrent-End");
    }
	return TRUE;
}

/* ************************************************************************* *\
** FUNCTION: GLDestroyContext
\* ************************************************************************* */
static BOOL GLDestroyContext( void )
{
    if(g_sGfxLocals.hGlContext)
    {
        GLDestroyOffScreenTexture();
		cglMakeCurrentThread( FALSE );
		wglDeleteContext( g_sGfxLocals.hGlContext );
        g_sGfxLocals.hGlContext = 0;
		cglCheckError();
    }
	return TRUE;
}

/* ************************************************************************* *\
** FUNCTION: GLCreateContext
\* ************************************************************************* */
static BOOL GLCreateContext( void )
{
    //GLDestroyContext();

//FIXME
	cglCheckErrorEx("GLCreateContext-setCurrent");
	glEnable(GL_SCISSOR_TEST);
	glPushAttrib (GL_ALL_ATTRIB_BITS);
	g_sGfxLocals.hGlContext = wglGetCurrentContext();
	g_sGfxLocals.hDisplay = wglGetCurrentDC();
	g_sGfxLocals.hWnd = GetForegroundWindow();
	cglCheckErrorEx("GLCreateContext-BeforeGlew");
	glewInit();
	cglCheckErrorEx("GLCreateContext-AfterGlew");

	return TRUE;
}

/* ************************************************************************* *\
** FUNCTION: SetVertex
\* ************************************************************************* */
void SetVertex(Vertex* in_pVtx,
                float in_x, float in_y, float in_z,
                float in_u, float in_v)
{
	in_pVtx->x = in_x;
	in_pVtx->y = in_y;
	in_pVtx->z = in_z;

	in_pVtx->u = in_u;
	in_pVtx->v = in_v;
}

/* ************************************************************************* *\
** FUNCTION: GLSetProjection
\* ************************************************************************* */
static BOOL GLSetProjection( void )
{
#ifdef PRESERVE_ASPECT_RATIO
    GLdouble HalfWidth = 0.0, HalfHeigth = 0.0;

    GLdouble fBufferRatio = 0.0f;
    GLdouble fScreenRatio = 1.0f;

    UINT32 u32ImageWTemp = g_sGfxLocals.BackBufferWidth;
    UINT32 u32ImageHTemp = g_sGfxLocals.BackBufferHeight;

    GLdouble u32ScreenWTemp = (g_sGfxLocals.ScreenWidth -  s_sGl2dState.ViewportOffsets.Left) - (0+s_sGl2dState.ViewportOffsets.Right);
    GLdouble u32ScreenHTemp = (g_sGfxLocals.ScreenHeight -  s_sGl2dState.ViewportOffsets.Bottom) - (0+s_sGl2dState.ViewportOffsets.Top);

	// Modify the ratio for a doubled dimension
    if(g_sGfxLocals.BackBufferWidth == 640 && g_sGfxLocals.BackBufferHeight == 240)
    {
        u32ImageHTemp *= 2;
    }

    fBufferRatio = (GLdouble)u32ImageWTemp / (GLdouble)u32ImageHTemp;
    fScreenRatio = u32ScreenWTemp / u32ScreenHTemp;

    // If it's wider than expected, fit to the height
    if( fScreenRatio > fBufferRatio )
    {
            HalfWidth = u32ScreenHTemp * fBufferRatio;
            HalfHeigth = u32ScreenHTemp;
    }
    // Otherwise fit to the width
    else
    {
            HalfWidth = u32ScreenWTemp;
            HalfHeigth = u32ScreenWTemp / fBufferRatio;
    }

    //printf("Half %f, %f\n", HalfWidth, HalfHeigth);
    //printf("Screen %f, BackBuffer %f\n", fWindowRatio, fImageRatio);

    // Compute the Viewport
    s_sGl2dState.Viewport.X = ((GLuint)(u32ScreenWTemp- HalfWidth)/2) + s_sGl2dState.ViewportOffsets.Left;
    s_sGl2dState.Viewport.Y = ((GLuint)(u32ScreenHTemp - HalfHeigth)/2) + s_sGl2dState.ViewportOffsets.Bottom;
    s_sGl2dState.Viewport.Width = (GLuint)HalfWidth;
    s_sGl2dState.Viewport.Height = (GLuint)HalfHeigth;
#else // !PRESERVE_ASPECT_RATIO
    GLdouble HalfWidth = 0.0, HalfHeigth = 0.0;
    GLdouble fBufferRatio = 0.0f;
    GLdouble fScreenRatio = 1.0f;

    UINT32 u32ImageWTemp = g_sGfxLocals.BackBufferWidth;
    UINT32 u32ImageHTemp = g_sGfxLocals.BackBufferHeight;

    GLdouble u32ScreenWTemp = (g_sGfxLocals.ScreenWidth);
    GLdouble u32ScreenHTemp = (g_sGfxLocals.ScreenHeight);

	GLdouble u32DrawableScreenWTemp = (g_sGfxLocals.ScreenWidth -  s_sGl2dState.ViewportOffsets.Left) - (0+s_sGl2dState.ViewportOffsets.Right);
    GLdouble u32DrawableScreenHTemp = (g_sGfxLocals.ScreenHeight -  s_sGl2dState.ViewportOffsets.Bottom) - (0+s_sGl2dState.ViewportOffsets.Top);

	// Modify the ratio for a doubled dimension
    if(g_sGfxLocals.BackBufferWidth == 640 && g_sGfxLocals.BackBufferHeight == 240)
    {
        u32ImageHTemp *= 2;
    }

    fBufferRatio = (GLdouble)u32ImageWTemp / (GLdouble)u32ImageHTemp;
    fScreenRatio = u32ScreenWTemp / u32ScreenHTemp;

    // If it's wider than expected, fit to the height
    if( fScreenRatio > fBufferRatio )
    {
            HalfWidth = u32ScreenHTemp * fBufferRatio;
            HalfHeigth = u32ScreenHTemp;
    }
    // Otherwise fit to the width
    else
    {
            HalfWidth = u32ScreenWTemp;
            HalfHeigth = u32ScreenWTemp / fBufferRatio;
    }

    //printf("Half %f, %f\n", HalfWidth, HalfHeigth);
    //printf("Screen %f, BackBuffer %f\n", fWindowRatio, fImageRatio);

    // Compute the Viewport
	{
		GLdouble u32ClampedHalfWidth =  min(u32DrawableScreenWTemp, HalfWidth);
		GLdouble u32ClampedHalfHeigth = min(u32DrawableScreenHTemp, HalfHeigth);

		s_sGl2dState.Viewport.X = ((GLuint)(u32DrawableScreenWTemp - u32ClampedHalfWidth)/2) + s_sGl2dState.ViewportOffsets.Left;
		s_sGl2dState.Viewport.Y = ((GLuint)(u32DrawableScreenHTemp - u32ClampedHalfHeigth)/2) +s_sGl2dState.ViewportOffsets.Bottom;
		s_sGl2dState.Viewport.Width = (GLuint)u32ClampedHalfWidth;
		s_sGl2dState.Viewport.Height = (GLuint)u32ClampedHalfHeigth;
	}
#endif // PRESERVE_ASPECT_RATIO

    glViewport(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
    glScissor(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );

/*
	DebugOut("Drawable screen size (%f, %f)\n", u32ScreenWTemp, u32ScreenHTemp);
    DebugOut("GL Viewport (%d, %d, %d, %d)\n",
            s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
            s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height);

    DebugOut("GL HalfWidth (%f, %f)\n", HalfWidth, HalfHeigth);
    DebugOut("RATIO Screen %f, BackBuffer %f, Viewport %f\n", fScreenRatio, fBufferRatio, HalfWidth/HalfHeigth);
/**/


    // Half each because these values are centered around coord 0,0
    HalfWidth = HalfWidth/ 2;
    HalfHeigth = HalfHeigth / 2;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glOrtho(-HalfWidth, HalfWidth, -HalfHeigth, HalfHeigth, 1, -1);
    glOrtho(0.0f, 1.0f, 1.0f, 0.0f, 0, 1);
    glGetFloatv (GL_PROJECTION_MATRIX, s_sGl2dState.Projection);
    glLoadIdentity ();

    // Create the Quad vertices
    //	{ 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, 1
	//	{ 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, 2
	//	{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, 3
	//	{ 1.0f, 0.0f, 0.0f, 1.0f, 0.0f}, 4
	SetVertex(&(s_sGl2dState.TexRect[0]), 0.0f, 1.0f, 0.0f, 0.0f, 1.0f); //1
	SetVertex(&(s_sGl2dState.TexRect[1]), 1.0f, 1.0f, 0.0f, 1.0f, 1.0f); //2
	SetVertex(&(s_sGl2dState.TexRect[2]), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); //3
	SetVertex(&(s_sGl2dState.TexRect[3]), 1.0f, 0.0f, 0.0f, 1.0f, 0.0f); //4
	cglCheckError();

	return TRUE;
}

/* ************************************************************************* *\
** FUNCTION: GLCreateOffScreenTexture
\* ************************************************************************* */
static BOOL GLCreateOffScreenTexture( void )
{
    UINT32 u32TextureX = 0;
    UINT32 u32TextureY = 0;
    GLDestroyOffScreenTexture();

    // Calculate a 2^n dimensional texture size for this window
    u32TextureX = g_sGfxLocals.BackBufferWidth;
    u32TextureY = g_sGfxLocals.BackBufferHeight;
    s_sGl2dState.fTextureClip[0] = 1.0f;
    s_sGl2dState.fTextureClip[1] = 1.0f;

    // Calculate the correct projection
    GLSetProjection();
	cglCheckErrorEx("GLCreateOffScreenTexture-After SetProjection");

	glMatrixMode(GL_MODELVIEW);

    // Setup model info
    glClearColor(0,0,0,0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

    // Create a texture object
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &s_sGl2dState.sHWTextureName);
    DebugOut("GLCreateOffScreenTexture %d(%d, %d)\n", s_sGl2dState.sHWTextureName, u32TextureX, u32TextureY);
	cglCheckErrorEx("GLCreateOffScreenTexture-glGenTextures");

    // Setup the options
    glBindTexture(GL_TEXTURE_2D, s_sGl2dState.sHWTextureName);
	cglCheckErrorEx("GLCreateOffScreenTexture-Bind Texture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // For non-rotating ortho images, Use lineear since we exect to be scaling
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Force the texture allocation and set it to black
    {
        UINT32 TexSz = u32TextureX * u32TextureY *2;
        void* pData = GCAllocateMemory(TexSz);
        memset(pData, 0, TexSz);
        glTexImage2D(GL_TEXTURE_2D, 	// GLenum  	target,
                     0,   // GLint  	level,
                     GL_RGB,   // GLint  	internalFormat,
                     u32TextureX, // GLsizei  	width,
                     u32TextureY,   // GLsizei  	height,
                     0,   // GLint  	border,
                     GL_RGB,   // GLenum  	format,
                     GL_UNSIGNED_SHORT_5_6_5,   // GLenum  	type,
                      pData  // const GLvoid *  	data);)
                );
        GCFreeMemory(pData);
		cglCheckErrorEx("GLCreateOffScreenTexture-SetData");
    }

    return TRUE;
}

/* ************************************************************************* *\
** FUNCTION: GLGetTempTexture
\* ************************************************************************* */
UINT32 GLGetTempTexture( void )
{
	if(s_TempTexId == 0)
	{
		// Create a texture object
		glGenTextures(1, &s_TempTexId);
		// Setup the options
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Setup the options
		glBindTexture(GL_TEXTURE_2D, s_TempTexId);
//		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// For non-rotating ortho images, Use lineear since we exect to be scaling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	cglCheckError();

	return s_TempTexId;
}

BOOL GLBlitRect( float l, float t, float r, float b )
{
	Vertex Rect[4];
	l /= g_sGfxLocals.BackBufferWidth;
	r /= g_sGfxLocals.BackBufferWidth;
	t /= g_sGfxLocals.BackBufferHeight;
	b /= g_sGfxLocals.BackBufferHeight;
	SetVertex(&(Rect[0]), l, b, 0.0f, 0.0f, 1.0f); //1
	SetVertex(&(Rect[1]), r, b, 0.0f, 1.0f, 1.0f); //2
	SetVertex(&(Rect[2]), l, t, 0.0f, 0.0f, 0.0f); //3
	SetVertex(&(Rect[3]), r, t, 0.0f, 1.0f, 0.0f); //4


    glViewport(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
    glScissor(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf (s_sGl2dState.Projection);
	cglCheckErrorEx("GLBlitRect-glMatrixMode(GL_PROJECTION)");

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	cglCheckErrorEx("GLBlitRect-glMatrixMode(GL_MODELVIEW)");
    

	// Draw a model with the texture on it
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &(Rect[0].x));
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &(Rect[0].u));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	cglCheckError();

	return GL_TRUE;
}

volatile static BOOL sg_bAppMinimized = FALSE;
volatile static BOOL sg_bNowMinimizeApp = FALSE;

/* ************************************************************************* *\
** FUNCTION: GLBlit2dBuffer
\* ************************************************************************* */
static BOOL GLBlit2dBuffer( void )
{
	// Clear the full back buffer
	glViewport(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);
	glScissor(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);
	glClearColor(0, 0, 0, 0);
	glClear (GL_COLOR_BUFFER_BIT);

	glViewport(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
			   s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
	glScissor(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
			   s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf (s_sGl2dState.Projection);
	cglCheckErrorEx("GLBlit2dBuffer-glMatrixMode(GL_PROJECTION)");

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	cglCheckErrorEx("GLBlit2dBuffer-glMatrixMode(GL_MODELVIEW)");

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// This is where we update the texture from a pre-generated buffer
	glBindTexture(GL_TEXTURE_2D, s_sGl2dState.sHWTextureName);
	if( 16 == sg_psCurrentMemorySurface->u8ColorDepth )
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
					 sg_psCurrentMemorySurface->w,
					 sg_psCurrentMemorySurface->h,
					 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
					 sg_psCurrentMemorySurface->pixels);
		cglCheckErrorEx("GLBlit2dBuffer-glTexImage2D");
	}
	else
	{
		// Undefined color depth
		GCASSERT(0);
	}

	// Draw a model with the texture on it
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), &(s_sGl2dState.TexRect[0].x));

	//glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &(s_sGl2dState.TexRect[0].u));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);

	//DrawBounds();

	// Display it
	glFlush();

	// Only blit if the application is not minimized
	if( FALSE == sg_bAppMinimized )
	{
		(void) SDL_GL_SwapBuffers();
	}

	cglCheckError();

	return TRUE;
}

static void* GetWindowHandle()
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);

	if( ! SDL_GetWMInfo(&wmi) )
	{
		return NULL;
	}

	return wmi.window;
}


/* ************************************************************************* *\
** FUNCTION: SetVideoMode
\* ************************************************************************* */
static void SetVideoMode(UINT32 u32WindowXSize, UINT32 u32WindowYSize, BOOL bFullScreen)
{
	UINT32 sdlFlags = SDL_HWSURFACE;

    DebugOut("SetVideoMode %d %d -- FullScreen = %d\n", u32WindowXSize, u32WindowYSize, bFullScreen);

    g_sGfxLocals.bModeSet = FALSE;
    GLDestroyContext ();
	if (sg_psVideoSurface)
	{
		SDL_Surface *psFree = NULL;

		psFree = sg_psVideoSurface;
		sg_psVideoSurface = NULL;
		SDL_FreeSurface(psFree);
	}

   
	if(bFullScreen)
	{
		sdlFlags |= SDL_FULLSCREEN;
		g_sGfxLocals.ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		g_sGfxLocals.ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		g_sGfxLocals.ScreenWidth = u32WindowXSize;
		g_sGfxLocals.ScreenHeight = u32WindowYSize;
	}

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	sdlFlags |= SDL_OPENGL;

	sg_psVideoSurface = SDL_SetVideoMode(g_sGfxLocals.ScreenWidth,
										 g_sGfxLocals.ScreenHeight,
										 16,
										 sdlFlags);
	GCASSERT(sg_psVideoSurface);
	
	GLCreateContext();
	HostSetFrameRate(GCGetRefreshRate() >> 24);
	cglCheckError();

}


/* ************************************************************************* *\
** ************************************************************************* **
** GFX Sub system Control
** ************************************************************************* **
\* ************************************************************************* */
BOOL HostGfxInit(void)
{
	InitializeCriticalSection(&g_sVideoCriticalSection);

	memset(&g_sGfxLocals, 0, sizeof(g_sGfxLocals));
    memset(&s_sGl2dState, 0, sizeof(s_sGl2dState));

	if (SDL_Init(SDL_INIT_VIDEO |
				 SDL_INIT_TIMER |
				 SDL_INIT_JOYSTICK |
				 SDL_INIT_AUDIO) < 0)
	{
		return(FALSE);
	}

#ifdef _RELEASE
    SetVideoMode(1920, 1080, TRUE);
#else
	SetVideoMode(1920, 1080, FALSE);
#endif

	(void) SetWindowText(GetWindowHandle(), "RSC68K-Emu");

	cglMakeCurrentThread(FALSE);
	
	return(TRUE);
}


void HostInputInit(void)
{
	// Enable unicode translation of keyboard input events
	(void) SDL_EnableUNICODE(1);

	// Enable the key repeat with defaults
	(void) SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}


/* ************************************************************************* *\
** FUNCTION: HostGfxShutdown
\* ************************************************************************* */
void HostGfxShutdown(void)
{
	EnterCriticalSection(&g_sVideoCriticalSection);

	if (sg_psDefaultBackbufferSurface)
	{
		FreeSurface(sg_psDefaultBackbufferSurface);
		sg_psDefaultBackbufferSurface = NULL;
	}

	if (sg_psVideoSurface)
	{
		SDL_Surface *psFree = NULL;

		psFree = sg_psVideoSurface;
		sg_psVideoSurface = NULL;
		SDL_FreeSurface(psFree);
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	// Leave the video critical section locked...
	LeaveCriticalSection(&g_sVideoCriticalSection);
}


/* ************************************************************************* *\
** 
\* ************************************************************************* */
GLAPI void GL_APIENTRY cglSetViewport ( GLint  	x, GLint  	y,
            GLsizei  	width,  GLsizei  	height )
{

    glViewport(s_sGl2dState.Viewport.X + (GLuint)x, s_sGl2dState.Viewport.Y + (GLuint)y,
               min((GLuint)width, s_sGl2dState.Viewport.Width),
                min((GLuint)height, s_sGl2dState.Viewport.Height) );
    glScissor(s_sGl2dState.Viewport.X + (GLuint)x, s_sGl2dState.Viewport.Y + (GLuint)y,
               min((GLuint)width, s_sGl2dState.Viewport.Width),
                min((GLuint)height, s_sGl2dState.Viewport.Height) );
	cglCheckError();


/*
     printf("cglSetViewport(%d, %d, %d, %d)\n",
                s_sGl2dState.Viewport.X + x, s_sGl2dState.Viewport.Y + y,
               min(width, s_sGl2dState.Viewport.Width),
                min(height, s_sGl2dState.Viewport.Height));
*/
}

/* ************************************************************************* *\
** 
\* ************************************************************************* */
GLAPI void GL_APIENTRY cglReadPixels(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    GLvoid *pixels
)
{
    glReadPixels(s_sGl2dState.Viewport.X + x, s_sGl2dState.Viewport.Y + y,
               min((GLuint)width, s_sGl2dState.Viewport.Width),
                min((GLuint)height, s_sGl2dState.Viewport.Height),
                 format, type, pixels);
	cglCheckError();

}


/* ************************************************************************* *\
** 
\* ************************************************************************* */
GLAPI GLuint GL_APIENTRY cglScreenHeight ( void )
{
	return s_sGl2dState.Viewport.Height;
}

/* ************************************************************************* *\
** 
\* ************************************************************************* */
GLAPI GLuint GL_APIENTRY cglScreenWidth ( void )
{
	return s_sGl2dState.Viewport.Width;
}

/* ************************************************************************* *\
** 
\* ************************************************************************* */
BOOL cglMakeCurrentThread( BOOL in_MakeCurrent )
{
	BOOL ret = TRUE;
	cglCheckError();
    if(in_MakeCurrent)
    {
        ret = wglMakeCurrent( g_sGfxLocals.hDisplay, g_sGfxLocals.hGlContext);
    }
    else
    {
		ret = wglMakeCurrent( NULL, NULL );
    }

	if(!ret)
	{
		cglCheckError();
	}
	return GL_TRUE;
}

/* ************************************************************************* *\
** 
\* ************************************************************************* */
void cglSetGLAware( BOOL in_Aware )
{
	g_sGfxLocals.b2DGraphics = in_Aware; // since menu system resets the gfx mode, we need to get this lib back to 3d mode...
}

/* ************************************************************************* *\
** FUNCTION: SetDefaultSurface
\* ************************************************************************* */
void SetDefaultSurface(Surface *psSurface)
{
    // CLM -- WTF
	if ((NULL == psSurface) &&
		(sg_psDefaultBackbufferSurface == sg_psCurrentMemorySurface))
	{
		sg_psCurrentMemorySurface = NULL;
	}

	sg_psDefaultBackbufferSurface = psSurface;
	if (NULL == sg_psCurrentMemorySurface)
	{
		sg_psCurrentMemorySurface = sg_psDefaultBackbufferSurface;
	}
}

/* ************************************************************************* *\
** FUNCTION: GetActiveSurface
\* ************************************************************************* */
Surface* GetActiveSurface(void)
{
	return(sg_psCurrentMemorySurface);
}
/* ************************************************************************* *\
** FUNCTION: GetDefaultSurface
\* ************************************************************************* */
Surface* GetDefaultSurface(void)
{
	return(sg_psDefaultBackbufferSurface);
}

/* ************************************************************************* *\
** FUNCTION: HostSetDisplaySurface
\* ************************************************************************* */
void HostSetDisplaySurface(UINT32 u32XSize,
						  UINT32 u32YSize,
						  UINT8 u8ColorDepth,
						  BOOL bWindowed,
						  BOOL bOpenGL)
{
	static Surface * psSurface;
    GCASSERT(u8ColorDepth == 16);

	cglMakeCurrentThread(TRUE);
  
    glPopAttrib ();
    glPushAttrib (GL_ALL_ATTRIB_BITS);
  
	psSurface = GetDefaultSurface();
	if (psSurface)
	{
		FreeSurface((void *)psSurface);
		SetDefaultSurface(NULL);
	}

	psSurface = CreateRGBSurface(u32XSize, u32YSize, u8ColorDepth);
	GCASSERT(psSurface);

	SetDefaultSurface(psSurface);

	g_sGfxLocals.BackBufferWidth = u32XSize;
	g_sGfxLocals.BackBufferHeight = u32YSize;
    g_sGfxLocals.BackBufferColorDepth = u8ColorDepth;

	// Buffer will be scaled on final output to best fit the screen
	{
		UINT32 u32NewX;
		UINT32 u32NewY;

		// First try fitting to the screen width
		if( (g_sGfxLocals.ScreenWidth != u32XSize) ||
			(g_sGfxLocals.ScreenHeight != u32YSize) )
		{
			u32NewX = g_sGfxLocals.ScreenWidth;
			u32NewY = u32YSize * g_sGfxLocals.ScreenWidth / u32XSize;

			// If that didn't work, fit to the height
			if( u32NewY > (UINT32)g_sGfxLocals.ScreenHeight )
			{
				u32NewX = u32XSize * g_sGfxLocals.ScreenHeight / u32YSize;
				u32NewY = g_sGfxLocals.ScreenHeight;
			}
		}
		else
		{
			u32NewX = u32XSize;
			u32NewY = u32YSize;
		}

		g_sGfxLocals.OutputWidth = u32NewX;
		g_sGfxLocals.OutputHeight = u32NewY;

		DebugOut("Output resized to %d %d\n", u32NewX, u32NewY);
	}


    g_sGfxLocals.b2DGraphics = !bOpenGL;

	// If 2D mode, setup opengl for a single quad and texture
	if(g_sGfxLocals.b2DGraphics)
	{
        GLCreateOffScreenTexture ();
    }
	else
	{
		GLSetProjection();
	}

	HostSetFrameRate(GCGetRefreshRate() >> 24);

	cglMakeCurrentThread(FALSE);
	
	g_sGfxLocals.bModeSet = TRUE;

	//Hack to work around OGL context issue on win32.
	if(s_pModeChangeCallback)
	{
		s_pModeChangeCallback();
	}
}

/* ************************************************************************* *\
** FUNCTION: HostCreateBackBuffer
\* ************************************************************************* */
void *HostCreateBackBuffer(UINT32 u32XSize, UINT32 u32YSize, UINT8 u8ColorDepth)
{
	Surface *pvFrameBuffer = CreateRGBSurface (u32XSize, u32YSize, u8ColorDepth);
	GCASSERT(pvFrameBuffer);
	return((void *) pvFrameBuffer);
}

/* ************************************************************************* *\
** FUNCTION: HostGetBackBufferPitch
\* ************************************************************************* */
UINT32 HostGetBackBufferPitch(void)
{
	GCASSERT(sg_psDefaultBackbufferSurface);
	return((UINT32) sg_psDefaultBackbufferSurface->pitch);
}

/* ************************************************************************* *\
** FUNCTION: HostDeleteBackBuffer
\* ************************************************************************* */
void HostDeleteBackBuffer(void *pvBackBuffer)
{
	FreeSurface((Surface *) pvBackBuffer);
}

/* ************************************************************************* *\
** FUNCTION: GetTimeStamp
\* ************************************************************************* */
static  UINT64 GetTimeStamp()
{
	return timeGetTime() * 1000;
}

/* ************************************************************************* *\
** FUNCTION: TimeStampToSec
\* ************************************************************************* */
static  float TimeStampToSec(UINT64 in_Time)
{
    const float factor = 1.0f/1000000.0f;
    return in_Time * factor;
}

/* ************************************************************************* *\
** FUNCTION: SecToMS
\* ************************************************************************* */
static  UINT32 SecToMS(float in_Time)
{
    return (UINT32)(1000*in_Time);
}

/* ************************************************************************* *\
** FUNCTION: SecToUS
\* ************************************************************************* */
static UINT32 SecToUS(float in_Time)
{
    return (UINT32)(1000000*in_Time);
}

/* ************************************************************************* *\
** FUNCTION: GCReadPixels
\* ************************************************************************* */
void GCReadPixels(
    UINT32 x,
    UINT32 y,
    UINT32 width,
    UINT32 height,
    UINT16  *pixels
)
{
    glReadPixels(s_sGl2dState.Viewport.X + x, s_sGl2dState.Viewport.Y + y,
               min(width, s_sGl2dState.Viewport.Width),
                min(height, s_sGl2dState.Viewport.Height),
                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, (void*)pixels);
    cglCheckError();

}

UINT32 GCGetScreenPixelWidth(void)
{
    return s_sGl2dState.Viewport.Width;
}

UINT32 GCGetScreenPixelHeight(void)
{
    return s_sGl2dState.Viewport.Height;
}


EGCResultCode HostMinimizeApplicationInternal( void )
{
    int s32SDLResult;

	// Take the video section so that the minimize doesn't occur during a buffer swap
	EnterCriticalSection(&g_sVideoCriticalSection);

    s32SDLResult = SDL_WM_IconifyWindow();
	sg_bAppMinimized = TRUE;

	LeaveCriticalSection(&g_sVideoCriticalSection);

	GCASSERT( s32SDLResult != 0 );

    return( GC_OK );
}


/* ************************************************************************* *\
** FUNCTION: HostThrottleAndBlit
\* ************************************************************************* */
void HostThrottleAndBlit(void)
{
    static UINT64 LastFrameTime = 0;
    static UINT64 LastTitleUpdateTime = 0;
    static UINT32 Frames = 0;
    static INT64 FrameTimeError = 0;

    UINT64 CurTime = 0;

	// If the application is minimized, don't try to blit it right now.
	if( sg_bAppMinimized )
	{
		return;
	}

	// If app should be minimized, do it now
	if( sg_bNowMinimizeApp )
	{
		sg_bNowMinimizeApp = FALSE;
		(void) HostMinimizeApplicationInternal();
		return;
	}

    if(s_sGl2dState.FrameTimeS == 0.0f)
    {
        return;
    }

    if(LastFrameTime == 0)
    {
        LastFrameTime = GetTimeStamp();
        LastTitleUpdateTime = LastFrameTime;
    }
    
    CurTime = GetTimeStamp();
    Frames++;

	EnterCriticalSection(&g_sVideoCriticalSection);

	cglMakeCurrentThread(TRUE);
	
    if(g_sGfxLocals.b2DGraphics)
    {
		UINT16 *pu16Surface;
		UINT32 u32XSize;
		UINT32 u32YSize;
		UINT32 u32Pitch;

		(void) GCDisplayGetDisplayBuffer(&pu16Surface);
		(void) GCDisplayGetXSize(&u32XSize);
		(void) GCDisplayGetYSize(&u32YSize);
		(void) GCDisplayGetDisplayPitch(&u32Pitch);

		GLBlit2dBuffer();
    }
    else
    {
		cglCheckErrorEx("HostThrottleAndBlit-PreSwap");

 		// DrawBounds();
		(void) SDL_GL_SwapBuffers();

		glViewport(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);
		glScissor(0, 0, g_sGfxLocals.ScreenWidth, g_sGfxLocals.ScreenHeight);
        glClearColor(0, 0, 0, 0);
        glClear (GL_COLOR_BUFFER_BIT);
        glViewport(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
        glScissor(s_sGl2dState.Viewport.X, s_sGl2dState.Viewport.Y,
               s_sGl2dState.Viewport.Width, s_sGl2dState.Viewport.Height );
		cglCheckErrorEx("HostThrottleAndBlit-GL mode swap");
   }

    // Update the Frame title for the
    if( 1.0f < TimeStampToSec(CurTime - LastTitleUpdateTime))
    {
        char string[150] = "";
        sprintf(string, "Frames=%i FPS=%f Elapsed=%f", Frames,
                Frames /TimeStampToSec(CurTime - LastTitleUpdateTime),
                TimeStampToSec(CurTime - LastTitleUpdateTime));
		//SetWindowText(g_sGfxLocals.hWnd, string);
        Frames = 0;
        LastTitleUpdateTime = CurTime;
    }
	
	cglMakeCurrentThread(FALSE);
	
	LeaveCriticalSection(&g_sVideoCriticalSection);

    if(s_pFrameCallback)
    {
        s_pFrameCallback();
    }

//  // Wait for the next frame time.
//  {
//
//      float ElapsedTime = 0;
//      INT32 WaitTime = 0;
//      UINT64
//
//      CurTime = GetTimeStamp();
//      ElapsedTime = TimeStampToSec(CurTime - LastFrameTime);
//      WaitTime = SecToMS(s_sGl2dState.FrameTimeS - ElapsedTime);
//      FrameTimeError += SecToUS(s_sGl2dState.FrameTimeS - ElapsedTime) - (WaitTime * 1000);  // Accumulate the usec error in calculations
//      if(FrameTimeError > 1000) // if we have more then 1 MS err handle it.
//      {
//          WaitTime++; // Move the MS to wait time.
//          FrameTimeError -= 1000;
//      }
//
//      if(WaitTime > 0)
//      {
//          Sleep(WaitTime);
//      }
//      LastFrameTime = GetTimeStamp();
//      //printf("CT %llu ElapsedTime %f WaitTime %i  Slept %d, Error %lld\n", CurTime, ElapsedTime, WaitTime,
//      //   SecToMS(TimeStampToSec(GetTimeStamp() - CurTime)), FrameTimeError);
//
//
//  }
}

/* ************************************************************************* *\
** FUNCTION: GCSetFrameCallback
\* ************************************************************************* */
// Assumption: Caller must be holding video critical section
void HostSetFrameRate(UINT32 u32Frames)
{
    if(u32Frames == 0)
    {
        s_sGl2dState.FrameTimeS = 0;
    }
    else
    {
        s_sGl2dState.FrameTimeS = 1.0f/u32Frames;
    }
}

/* ************************************************************************* *\
** FUNCTION: GCSetFrameCallback
\* ************************************************************************* */
void GCSetFrameCallback(void (*pFrameProc)(void))
{
	s_pFrameCallback = pFrameProc;
}


/* ************************************************************************* *\
** FUNCTION: GCSetModeChangeCallback
\* ************************************************************************* */
void GCSetModeChangeCallback(void (*pModeChangeProc)(void))
{
	s_pModeChangeCallback = pModeChangeProc;
}

/* ************************************************************************* *\
** FUNCTION: GCSetScreenOffsets
\* ************************************************************************* */
void GCSetScreenOffsets(UINT32 in_Left, UINT32 in_Right, UINT32 in_Top, UINT32 in_Bottom )
{
    s_sGl2dState.ViewportOffsets.Left = in_Left;
    s_sGl2dState.ViewportOffsets.Right = in_Right;
    s_sGl2dState.ViewportOffsets.Top = in_Top;
    s_sGl2dState.ViewportOffsets.Bottom = in_Bottom;

    // Apply the changes
    GLSetProjection();

}

/* ************************************************************************* *\
** FUNCTION: GCGetModeChangeCallback
\* ************************************************************************* */
void GCGetScreenOffsets(UINT32* in_pLeft, UINT32* in_pRight, UINT32* in_pTop, UINT32* in_pBottom )
{
    *in_pLeft = s_sGl2dState.ViewportOffsets.Left;
    *in_pRight = s_sGl2dState.ViewportOffsets.Right;
    *in_pTop = s_sGl2dState.ViewportOffsets.Top;
    *in_pBottom = s_sGl2dState.ViewportOffsets.Bottom;
}

/* ************************************************************************* *\
** FUNCTION: HostProcessMessages
\* ************************************************************************* */
void HostProcessMessages(void)
{
	static int i = 0;
	SDL_Event sEvents;
#ifdef USE_SDL_GRAB
	static BOOL bFirst = TRUE;

	if(bFirst)
	{
		SDL_WM_GrabInput(SDL_GRAB_ON);
		SDL_WarpMouse((UINT16)(g_sGfxLocals.ScreenWidth /2), (UINT16)(g_sGfxLocals.ScreenHeight/2));
		SDL_ShowCursor(FALSE);
		s_bCapture = !s_bCapture;	
		bFirst = FALSE;
		
		//SetFocus(g_sGfxLocals.hWnd);
		SetForegroundWindow(g_sGfxLocals.hWnd);
	}
#endif

	while (SDL_PollEvent(&sEvents))
	{
		if (SDL_QUIT == sEvents.type)
		{
			BlockReport("heap.txt");
			GCASSERT(sg_pGameExitCallbackProc);
			sg_pGameExitCallbackProc();
		}
		else
		if (SDL_VIDEOEXPOSE == sEvents.type )
		{
			sg_bAppMinimized = FALSE;
			HostThrottleAndBlit();
			DebugOutFunc("Application Exposed\r\n");
		}
		else
		if (SDL_ACTIVEEVENT == sEvents.type )
		{
			if( (sEvents.active.state & SDL_APPACTIVE) && (sEvents.active.gain == 0) )
			{
				HostMinimizeApplicationInternal();
				DebugOutFunc("Application Minimized/Disabled\r\n");
			}
		}
		else
		if (SDL_KEYDOWN == sEvents.type || SDL_KEYUP == sEvents.type)
		{
			if (SDL_KEYDOWN == sEvents.type)
			{
				SetKeyState(sEvents.key.keysym, TRUE);

				if (SDLK_LSHIFT == sEvents.key.keysym.sym)
				{
					sg_bLeftShiftHeld = TRUE;
				}
				else
				if (SDLK_RSHIFT == sEvents.key.keysym.sym)
				{
					sg_bRightShiftHeld = TRUE;
				}
			}
			else
			{
				SetKeyState(sEvents.key.keysym, FALSE);
				if (SDLK_LSHIFT == sEvents.key.keysym.sym)
				{
					sg_bLeftShiftHeld = FALSE;
				}
				else
				if (SDLK_RSHIFT == sEvents.key.keysym.sym)
				{
					sg_bRightShiftHeld = FALSE;
				}
			}

#ifdef USE_SDL_GRAB
			if(sEvents.key.keysym.sym == SDLK_RCTRL
				|| sEvents.key.keysym.sym == SDLK_LCTRL)
			{
				if(s_bCapture)
				{
					SDL_WM_GrabInput(SDL_GRAB_OFF);
					s_bCapture = !s_bCapture;
					SDL_ShowCursor(TRUE);
				}				
			}
#endif
		}
#ifdef USE_SDL_GRAB
		else if(SDL_MOUSEBUTTONDOWN == sEvents.type)
		{
			if(!s_bCapture)
			{
				SDL_WM_GrabInput(SDL_GRAB_ON);
				SDL_WarpMouse((UINT16)(g_sGfxLocals.ScreenWidth /2), (UINT16)(g_sGfxLocals.ScreenHeight/2));
				SDL_ShowCursor(FALSE);
				s_bCapture = !s_bCapture;				
			}
		}
#endif
		else
		if (SDL_JOYAXISMOTION == sEvents.type)
		{
			SetJoyAxisState(sEvents.jaxis.which, sEvents.jaxis.axis, sEvents.jaxis.value );
		}
		else if (SDL_JOYBUTTONDOWN == sEvents.type)
		{
			SetJoyButtonState(sEvents.jbutton.which, sEvents.jbutton.button, TRUE);
		}
		else if (SDL_JOYBUTTONUP == sEvents.type)
		{
			SetJoyButtonState(sEvents.jbutton.which, sEvents.jbutton.button, FALSE);
		}
		else if (SDL_JOYHATMOTION == sEvents.type)
		{
			SetJoyHatState(sEvents.jhat.which, sEvents.jhat.hat, sEvents.jhat.value ); 
		}
		else
		if (SDL_MOUSEBUTTONDOWN == sEvents.type)
		{
			if (SDL_BUTTON_WHEELUP == sEvents.button.button)
			{
				// Scroll wheel up
				HostSetMousewheel(0);
			}
			if (SDL_BUTTON_WHEELDOWN == sEvents.button.button)
			{
				// Scroll wheel down
				HostSetMousewheel(1);
			}
		}
	}
}

/* ************************************************************************* *\
** HostGetTrackState
\* ************************************************************************* */
void HostGetTrackState(INT8* ps8X, INT8* ps8Y)
{
#ifdef USE_SDL_GRAB
	if(s_bCapture)
	{
		INT32 mx, my;
		SDL_GetMouseState(&mx, &my);

		if(ps8X)
		{
			*ps8X = -(INT8)((g_sGfxLocals.ScreenWidth/2) - mx);
		}

		if(ps8Y)
		{
			*ps8Y = (INT8)((g_sGfxLocals.ScreenHeight/2) - my);
		}

		SDL_WarpMouse((UINT16)(g_sGfxLocals.ScreenWidth/2), (UINT16)(g_sGfxLocals.ScreenHeight/2));
	}
	else
#endif
	{
		if(ps8X)
		{
			*ps8X = 0;
		}

		if(ps8Y)
		{
			*ps8Y = 0;
		}
	}
}

/* ************************************************************************* *\
** FUNCTION: HostSetGameExitCallbackProcedure
\* ************************************************************************* */
void HostSetGameExitCallbackProcedure(void (*pGameExitCallbackProc)(void))
{
	sg_pGameExitCallbackProc = pGameExitCallbackProc;
}


EGCResultCode HostConvertScreenPosition(UINT32* pu32X, UINT32* pu32Y)
{
	UINT32 u32X;
	UINT32 u32Y;

	GCASSERT(pu32X && pu32Y);

	u32X = *pu32X;
	u32Y = *pu32Y;

	// First scale the coordinate
	u32X = u32X * g_sGfxLocals.BackBufferWidth / g_sGfxLocals.OutputWidth;
	u32Y = u32Y * g_sGfxLocals.BackBufferHeight / g_sGfxLocals.OutputHeight;
	
	// Then offset the coordinate (with a scaled offset)
	u32X -= ((g_sGfxLocals.ScreenWidth - g_sGfxLocals.OutputWidth) / 2) * g_sGfxLocals.BackBufferWidth / g_sGfxLocals.OutputWidth;
	u32Y -= ((g_sGfxLocals.ScreenHeight - g_sGfxLocals.OutputHeight) / 2) * g_sGfxLocals.BackBufferHeight / g_sGfxLocals.OutputHeight;

	if( (u32X > (UINT32)g_sGfxLocals.BackBufferWidth) ||
		(u32Y > (UINT32)g_sGfxLocals.BackBufferHeight) )
	{
		return(GC_OUT_OF_RANGE);
	}

	*pu32X = u32X;
	*pu32Y = u32Y;

	return(GC_OK);
}


EGCResultCode HostMinimizeApplication( void )
{
	return(HostMinimizeApplicationInternal());
}

/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */
