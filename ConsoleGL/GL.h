#pragma once

#pragma region Begin Modes
#define GL_POINTS				(0x0000)
#define GL_LINES				(0x0001)
#define GL_TRIANGLES			(0x0004)
#define GL_QUADS				(0x0007)
#pragma endregion

#pragma region Errors
#define GL_NO_ERROR				(0x0000)
#define GL_INVALID_ENUM			(0x0500)
#define GL_INVALID_VALUE		(0x0501)
#define GL_INVALID_OPERATION	(0x0502)
#pragma endregion

#pragma region Features
#define GL_DEPTH_TEST			(0x0B71)
#define GL_CULL_FACE			(0x0B44)
#define GL_TEXTURE_2D			(0x0DE1)
#pragma endregion

#pragma region Types
#define GL_BYTE					(0x1400)
#define GL_FLOAT				(0x1406)
#pragma endregion

#pragma region Matrix Modes
#define GL_MODELVIEW			(0x1700)
#define GL_PROJECTION			(0x1707)
#pragma endregion

#pragma region Pixel Formats
#define GL_DEPTH_COMPONENT		(0x1902)
#define GL_RGB					(0x1907)
#define GL_RGBA					(0x1908)
#pragma endregion

#pragma region OLC extensions
// pixel format
#define EXT_OLC_PIXEL_FORMAT	(0x2000)
// type
#define EXT_OLC_PIXEL			(0x1500)
// features
#define EXT_OLC_SLOW_COLOR		(0x0C00)
#pragma endregion


#pragma region Buffers
#define GL_DEPTH_BUFFER_BIT		(0x0100)
#define GL_COLOR_BUFFER_BIT		(0x0400)
#pragma endregion

#pragma region Strings
#define GL_VENDOR				(0x0000)
#define GL_RENDERER				(0x0001)
#define GL_VERSION				(0x0002)
#define GL_EXTENSIONS			(0x0003)
#pragma endregion

void glInit(int w, int h);

const char* glGetString(int string);
void glEnable(int capability);
void glDisable(int capability);
int glGetError();
void glClearColor(float r, float g, float b, float a);
void glClearDepth(float depth);
void glClear(int mask);
void glBegin(int mode);
void glEnd();

void glColor4f(float r, float g, float b, float a);
void glColor3f(float r, float g, float b);
void glTexCoord2f(float u, float v);
void glVertex3f(float x, float y, float z);
void glVertex2f(float x, float y);

/*
Specify the matrix to apply operations on
*/
void glMatrixMode(int mode);
void glLoadIdentity();
void glTranslate(float x, float y, float z);
void glScale(float x, float y, float z);
void glRotatef(float angle, float x, float y, float z);
void glPerspective(float fovy, float aspect, float near, float far);
void glLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);

/*
Generate texture object
*/
void glGenTextures(int count, int* buf);
/*
Make a texture current
*/
void glBindTexture(int target, int id);
/*
Specify the pixels for a texture image
*/
void glTexImage2D(int target, int width, int height, int type, void* data);
void glReadPixels(int x, int y, int w, int h, int format, int type, void* data);

struct OlcPixel {
	wchar_t c;
	unsigned short col;
};