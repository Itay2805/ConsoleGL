#include "GL.h"

#include <vector>
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>
#include <glm\gtc\matrix_transform.hpp>

#define GL_BEGIN_CHECK if (context->beginMode != -1) { context->err = GL_INVALID_OPERATION; return; }
#define GL_CLAMP(val) (val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val))

struct Pixel {
	union {
		struct {
			float a;
			float b;
			float g;
			float r;
		};
	};

	Pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		:	r(r / 255.0f),
			g(g / 255.0f),
			b(b / 255.0f),
			a(a / 255.0f)
	{

	}

	Pixel(int rgba) {
		r = rgba >> 24 & 0xFF;
		g = rgba >> 16 & 0xFF;
		b = rgba >> 8 & 0xFF;
		a = rgba & 0xFF;
	}

	Pixel(float r, float g, float b, float a)
		:	r(GL_CLAMP(r)),
			g(GL_CLAMP(g)),
			b(GL_CLAMP(b)),
			a(GL_CLAMP(a))
	{

	}

	Pixel()
		:	r(0.0f),
			g(0.0f),
			b(0.0f),
			a(1.0f)
	{

	}
};

struct Texture {
	int w, h;
	Pixel* pixels;

	Texture() 
		: w(0), h(0), pixels(nullptr)
	{

	}

	~Texture() {
		if (pixels != nullptr) {
			delete pixels;
		}
	}
};

struct Vertex {
	glm::vec3 coord;
	glm::vec2 texCoord;
	Pixel color;
	bool cull;

	Vertex(glm::vec3 coord, glm::vec2 texCoord, Pixel color) 
		:	coord(coord),
			texCoord(texCoord),
			color(color)
	{

	}
};

struct GLContext {
	int w, h;

	Pixel* bufColor;
	float* bufDepth;

	glm::mat4 matModelView;
	glm::mat4 matProj;
	glm::mat4* curMatrix;

	int err;

	Pixel bufColorClear;
	float bufDepthClear;

	int beginMode;
	Pixel beginColor;
	glm::vec2 beginTexCoord;

	std::vector<Texture> textures;
	int curTexture;

	bool depthEnabled;
	bool cullingEnabled;
	bool textureEnabled;
	bool extOlcSlowColor;

	std::vector<Vertex> beginVertices;
	
	GLContext(int w, int h) 
		:	w(w),
			h(h),
			bufColor(new Pixel[w * h]),
			bufDepth(new float[w * h]),
			matModelView(glm::mat4(1.0f)),
			matProj(glm::mat4(1.0f)),
			curMatrix(&matModelView),
			err(GL_NO_ERROR),
			bufColorClear(Pixel(0.0f, 0.0f, 0.0f, 1.0f)),
			bufDepthClear(-1.0f),
			beginMode(-1),
			beginColor(Pixel(1.0f, 1.0f, 1.0f, 1.0f)),
			beginTexCoord(glm::vec2(0.0f, 0.0f)),
			curTexture(0),
			depthEnabled(false),
			cullingEnabled(false),
			textureEnabled(false),
			extOlcSlowColor(false)
	{
		for (int i = 0; i < w * h; i++) {
			bufColor[i] = bufColorClear;
		}
	}

	~GLContext() {
		delete bufColor;
		delete bufDepth;
	}

};

thread_local GLContext* context;

void glInit(int w, int h)
{
	context = new GLContext(w, h);
}

const char* glGetString(int string) {
	switch (string) {
		case GL_VENDOR: return "Itay Almog";
		case GL_RENDERER: return "Software Based (C++)";
		case GL_VERSION: return "OpenGL 1.1 WIN32";
		case GL_EXTENSIONS: return "EXT_OLC";
	default:
		context->err = GL_INVALID_ENUM;
		return nullptr;
	}
}

void glEnable(int capability) {
	GL_BEGIN_CHECK;

	switch (capability) {
	case GL_DEPTH_TEST: context->depthEnabled = true; break;
	case GL_CULL_FACE: context->cullingEnabled = true; break;
	case GL_TEXTURE_2D: context->textureEnabled = true; break;
	default:
		context->err = GL_INVALID_ENUM;
	}
}

void glDisable(int capability) {
	GL_BEGIN_CHECK;

	switch (capability) {
	case GL_DEPTH_TEST: context->depthEnabled = false; break;
	case GL_CULL_FACE: context->cullingEnabled = false; break;
	case GL_TEXTURE_2D: context->textureEnabled = false; break;
	default:
		context->err = GL_INVALID_ENUM;
	}
}

int glGetError() {
	if (context->beginMode != -1) return GL_NO_ERROR;
	return context->err;
}

void glClearColor(float r, float g, float b, float a) {
	GL_BEGIN_CHECK;
	
	context->bufColorClear = Pixel(r, g, b, a);
}

void glClearDepth(float depth) {
	GL_BEGIN_CHECK;

	context->bufDepthClear = depth;
}

void glClear(int mask) {
	if (mask - GL_COLOR_BUFFER_BIT - GL_DEPTH_BUFFER_BIT > 0) {
		context->err = GL_INVALID_VALUE;
		return;
	}

	int size = context->w * context->h;

	if ((mask & GL_COLOR_BUFFER_BIT) != 0) {
		for (int i = 0; i < size; i++) {
			context->bufColor[i] = context->bufColorClear;
		}
	}

	if ((mask & GL_DEPTH_BUFFER_BIT) != 0) {
		for (int i = 0; i < size; i++) {
			context->bufDepth[i] = context->bufDepthClear;
		}
	}
}

void glBegin(int mode) {
	GL_BEGIN_CHECK;

	switch (mode) {
	case GL_POINTS:
	case GL_LINES:
	case GL_TRIANGLES:
	case GL_QUADS:
		context->beginMode = mode;
		context->beginVertices.clear();
		break;
	default:
		context->err = GL_INVALID_ENUM;
		return;
	}
}

void drawPoint(Vertex p) {
	int o = (int)(glm::floor(p.coord.x) + glm::floor(p.coord.y) * context->w);

	if (context->depthEnabled) {
		float d = p.coord.z;
		if (d > context->bufDepth[o]) return;
		else context->bufDepth[o] = d;
	}

	// Vertex Color
	Pixel fragColor = p.color;

	// Texture samples
	if (context->textureEnabled && context->curTexture != -1) {
		const Texture& tex = context->textures[context->curTexture];
		float u = p.texCoord.x;
		float v = p.texCoord.y;
		u = (float)(((int)glm::floor(u * tex.w)) % tex.w);
		v = (float)(((int)glm::floor(v * tex.h)) % tex.h);

		int to = (int)(u + v * tex.w);
		fragColor.r *= tex.pixels[to].r;
		fragColor.g *= tex.pixels[to].g;
		fragColor.b *= tex.pixels[to].b;
		fragColor.a *= tex.pixels[to].a;
	}

	context->bufColor[o] = fragColor;
}

void drawLine(Vertex p1, Vertex p2) {
	int x0 = (int)glm::floor(p1.coord.x);
	int y0 = (int)glm::floor(p1.coord.y);
	int x1 = (int)glm::ceil(p2.coord.x);
	int y1 = (int)glm::ceil(p2.coord.y);
	int dx = glm::abs(x1 - x0);
	int dy = glm::abs(y1 - y0);
	int sx = x0 < x1 ? 1 : -1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx - dy;

	int totDist = dx > dy ? dx : dy;
	while (true) {
		float ic0 = glm::abs(dx > dy ? (x1 - x0) : (y1 - y0)) / totDist;
		float ic1 = 1.0 - ic0;

		int o = (x0 + y0 * context->w);
		float z = 1 / (ic0 * 1 / p1.coord.z + ic1 * 1 / p2.coord.z);
		if (context->depthEnabled) {
			if (z > context->bufDepth[o]) continue;
			else context->bufDepth[o] = z;
		}

		// Vertex Color
		Pixel fragColor;
		fragColor.r = (ic0*p1.color.r / p1.coord.z + ic1 * p2.color.r / p2.coord.z) * z;
		fragColor.g = (ic0*p1.color.g / p1.coord.z + ic1 * p2.color.g / p2.coord.z) * z;
		fragColor.b = (ic0*p1.color.b / p1.coord.z + ic1 * p2.color.b / p2.coord.z) * z;
		fragColor.a = (ic0*p1.color.a / p1.coord.z + ic1 * p2.color.a / p2.coord.z) * z;

		// Texture sample
		if (context->textureEnabled && context->curTexture != -1) {
			Texture& tex = context->textures[context->curTexture];
			float u = (ic0*p1.texCoord.x / p1.coord.z + ic1 * p2.texCoord.x / p2.coord.z) * z;
			float v = (ic0*p1.texCoord.y / p1.coord.z + ic1 * p2.texCoord.y / p2.coord.z) * z;
			u = (float)((int)glm::floor(u * tex.w) % tex.w); // This behaviour should later depend on GL_TEXTURE_WRAP_S
			v = (float)((int)glm::floor(v * tex.h) % tex.h);

			int to = (int)(u + v * tex.w);
			fragColor.r *= tex.pixels[to].r;
			fragColor.g *= tex.pixels[to].g;
			fragColor.b *= tex.pixels[to].b;
			fragColor.a *= tex.pixels[to].a;
		}

		context->bufColor[o] = fragColor;

		if (x0 == x1 && y0 == y1) break;

		float e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if(e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

void drawTriangle(Vertex p1, Vertex p2, Vertex p3) {
	if (context->cullingEnabled && !p1.cull) return;

	int x1 = (int)glm::floor(p1.coord.x);
	int x2 = (int)glm::floor(p2.coord.x);
	int x3 = (int)glm::floor(p3.coord.x);
	int y1 = (int)glm::floor(p1.coord.y);
	int y2 = (int)glm::floor(p2.coord.y);
	int y3 = (int)glm::floor(p3.coord.y);

	int minX = (int)glm::min(x1, glm::min(x2, x3));
	int minY = (int)glm::min(y1, glm::min(y2, y3));
	int maxX = (int)glm::max(x1, glm::max(x2, x3));
	int maxY = (int)glm::max(y1, glm::max(y2, y3));

	float factor = 1.0f / ((y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3));

	int o = 0;
	for (int x = minX; x <= maxX; x++) {
		for (int y = minY; y <= maxY; y++) {
			float ic0 = ((y2 - y3)*(x - x3) + (x3 - x2)*(y - y3)) * factor;
			if (ic0 < 0 || ic0 > 1) continue;
			float ic1 = ((y3 - y1)*(x - x3) + (x1 - x3)*(y - y3)) * factor;
			if (ic1 < 0 || ic1 > 1) continue;
			float ic2 = 1.0f - ic0 - ic1;
			if (ic2 < 0 || ic2 > 1) continue;

			o = (x + y * context->w);
			if (o >= context->w  * context->h || o < 0) {
				continue;
			}

			float z = 1 / (ic0 * 1 / p1.coord.z + ic1 * 1 / p2.coord.z + ic2 * 1 / p3.coord.z);
			if (context->depthEnabled) {
				if (z > context->bufDepth[o]) continue;
				else context->bufDepth[o] = z;
			}

			// Vertex color
			Pixel fragColor;
			fragColor.r = (ic0*p1.color.r / p1.coord.z + ic1 * p2.color.r / p2.coord.z + ic2 * p3.color.r / p3.coord.z) * z;
			fragColor.g = (ic0*p1.color.g / p1.coord.z + ic1 * p2.color.g / p2.coord.z + ic2 * p3.color.g / p3.coord.z) * z;
			fragColor.b = (ic0*p1.color.b / p1.coord.z + ic1 * p2.color.b / p2.coord.z + ic2 * p3.color.b / p3.coord.z) * z;
			fragColor.a = (ic0*p1.color.a / p1.coord.z + ic1 * p2.color.a / p2.coord.z + ic2 * p3.color.a / p3.coord.z) * z;

			// Texture sample
			if (context->textureEnabled && context->curTexture != -1) {
				const Texture& tex = context->textures[context->curTexture];
				float u = (ic0*p1.texCoord.x / p1.coord.z + ic1 * p2.texCoord.x / p2.coord.z + ic2 * p3.texCoord.x / p3.coord.z) * z;
				float v = (ic0*p1.texCoord.y / p1.coord.z + ic1 * p2.texCoord.y / p2.coord.z + ic2 * p3.texCoord.y / p3.coord.z) * z;
				u = (float)((int)glm::floor(u * tex.w) % tex.w); // This behaviour should later depend on GL_TEXTURE_WRAP_S
				v = (float)((int)glm::floor(v * tex.h) % tex.h);

				int to = (int)(u + v * tex.w);
				fragColor.r *= tex.pixels[to].r;
				fragColor.g *= tex.pixels[to].g;
				fragColor.b *= tex.pixels[to].b;
				fragColor.a *= tex.pixels[to].a;
			}

			context->bufColor[o] = fragColor;
		}
	}
}

void drawQuad(Vertex p1, Vertex p2, Vertex p3, Vertex p4) {
	if (context->cullingEnabled) {
		if (!p1.cull) return;
		p3.cull = true; // Or drawTriangle will think the second triangle making the quad is culled
	}

	drawTriangle(p1, p2, p3);
	drawTriangle(p3, p4, p1);
}

void glEnd() {
	if (context->beginMode == -1) {
		context->err = GL_INVALID_OPERATION;
		return;
	}

	// Transform vertives and map them to window coordinates
	for (size_t i = 0; i < context->beginVertices.size(); i++) {
		Vertex& vertex = context->beginVertices[i];
		
		glm::vec4 vec(vertex.coord.x, vertex.coord.y, vertex.coord.z, 1.0f);
		vec = context->matModelView * vec;

		// Perform back culling - if enabled
		if (context->cullingEnabled && ((context->beginMode == GL_QUADS && i % 4 == 0) || (context->beginMode == GL_TRIANGLES && i % 3 == 0))) {
			glm::vec3 side1 = context->beginVertices[i].coord - context->beginVertices[i + 1].coord;
			glm::vec3 side2 = context->beginVertices[i].coord - context->beginVertices[i + 2].coord;
			glm::vec3 normal = glm::cross(side1, side2);
			float dot = glm::dot(normal, context->beginVertices[i].coord);
			vertex.cull = dot <= 0;
		}

		// Transform vertex into clip space
		vec = context->matProj * vec;

		// Calculate normalized device coordinates
		glm::vec3 norm(vec.x / vec.w, vec.y / vec.w, vec.z / vec.w);

		vertex.coord.x = (norm.x + 1) / 2 * context->w;
		vertex.coord.y = (1 - norm.y) / 2 * context->h;
		vertex.coord.z = norm.z;
	}

	// Assemble primities
	switch (context->beginMode) {
	case GL_POINTS:
		for (size_t i = 0; i < context->beginVertices.size(); i++) {
			drawPoint(context->beginVertices[i]);
		}
		break;
	case GL_LINES:
		for (size_t i = 0; i < context->beginVertices.size(); i += 2) {
			drawLine(context->beginVertices[i], context->beginVertices[i + 1]);
		}
		break;
	case GL_TRIANGLES:
		for (size_t i = 0; i < context->beginVertices.size(); i += 3) {
			drawTriangle(context->beginVertices[i], context->beginVertices[i + 1], context->beginVertices[i + 2]);
		}
		break;
	case GL_QUADS:
		for (size_t i = 0; i < context->beginVertices.size(); i += 4) {
			drawQuad(context->beginVertices[i], context->beginVertices[i + 1], context->beginVertices[i + 2], context->beginVertices[i + 3]);
		}
		break;
	}

	context->beginMode = -1;
	context->beginVertices.clear();
}

void glColor4f(float r, float g, float b, float a) {
	context->beginColor = Pixel(r, g, b, a);
}

void glColor3f(float r, float g, float b) {
	context->beginColor = Pixel(r, g, b, 1.0f);
}

void glTexCoord2f(float u, float v) {
	context->beginTexCoord.x = u;
	context->beginTexCoord.y = v;
}

void glVertex3f(float x, float y, float z) {
	if (context->beginMode == -1) {
		context->err = GL_INVALID_OPERATION;
		return;
	}

	context->beginVertices.push_back(Vertex(glm::vec3(x, y, z), context->beginTexCoord, context->beginColor));
}

void glVertex2f(float x, float y) {
	if (context->beginMode == -1) {
		context->err = GL_INVALID_OPERATION;
		return;
	}

	context->beginVertices.push_back(Vertex(glm::vec3(x, y, 1.0), context->beginTexCoord, context->beginColor));
}

void glMatrixMode(int mode) {
	GL_BEGIN_CHECK;

	switch (mode) {
	case GL_MODELVIEW:
		context->curMatrix = &context->matModelView;
		break;
	case GL_PROJECTION:
		context->curMatrix = &context->matProj;
		break;
	default:
		context->err = GL_INVALID_ENUM;
		return;
	}
}

void glLoadIdentity() {
	GL_BEGIN_CHECK;

	*context->curMatrix = glm::mat4(1.0f);
}

void glTranslatef(float x, float y, float z) {
	GL_BEGIN_CHECK;
	
	*context->curMatrix = glm::translate(*context->curMatrix, glm::vec3(x, y, z));
}

void glScalef(float x, float y, float z) {
	GL_BEGIN_CHECK;

	*context->curMatrix = glm::scale(*context->curMatrix, glm::vec3(x, y, z));
}

void glRotatef(float angle, float x, float y, float z) {
	GL_BEGIN_CHECK;

	*context->curMatrix = glm::rotate(*context->curMatrix, angle, glm::vec3(x, y, z));
}

void glPerspective(float fovy, float aspect, float near, float far) {
	*context->curMatrix = glm::perspective(fovy, aspect, near, far);
}

void glLookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {	
	*context->curMatrix = glm::lookAt(glm::vec3(eyeX, eyeY, eyeZ), glm::vec3(centerX, centerY, centerZ), glm::vec3(upX, upY, upZ));
}

void glGenTextures(int count, int* buf) {
	GL_BEGIN_CHECK;

	if (count < 0) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	for (int i = 0; i < count; ++i) {
		int id = context->textures.size() + 1;
		buf[i] = id;
		context->textures.push_back(Texture());
	}
}

void glBindTexture(int target, int id) {
	GL_BEGIN_CHECK;

	switch (target) {
	case GL_TEXTURE_2D:
		context->curTexture = id - 1;
		break;
	default:
		context->err = GL_INVALID_ENUM;
		return;
	}
}

void glTexImage2D(int target, int width, int height, int type, void* data) {
	GL_BEGIN_CHECK;

	if (target != GL_TEXTURE_2D) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	if (type != GL_BYTE && type != GL_FLOAT) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	Texture& texture = context->textures[context->curTexture];
	texture.pixels = new Pixel[width * height];
	texture.w = width;
	texture.h = height;

	int size = width * height;
	for (int i = 0; i < size; ++i) {
		if (type == GL_BYTE) {
			unsigned char* arr = (unsigned char*)data;
			texture.pixels[i] = Pixel(	arr[i * 4 + 0],
										arr[i * 4 + 1],
										arr[i * 4 + 2],
										arr[i * 4 + 3]);
		}
		else if(type == GL_FLOAT) {
			float* arr = (float*)data;
			texture.pixels[i] = Pixel(	arr[i * 4 + 0],
										arr[i * 4 + 1],
										arr[i * 4 + 2],
										arr[i * 4 + 3]);
		}
	}
}

#pragma region OLC
// this was taken from the webcam to console:
// https://github.com/OneLoneCoder/videos/blob/master/OneLoneCoder_Webcam.cpp

enum COLOUR
{
	FG_BLACK = 0x0000,
	FG_DARK_BLUE = 0x0001,
	FG_DARK_GREEN = 0x0002,
	FG_DARK_CYAN = 0x0003,
	FG_DARK_RED = 0x0004,
	FG_DARK_MAGENTA = 0x0005,
	FG_DARK_YELLOW = 0x0006,
	FG_GREY = 0x0007, // Thanks MS :-/
	FG_DARK_GREY = 0x0008,
	FG_BLUE = 0x0009,
	FG_GREEN = 0x000A,
	FG_CYAN = 0x000B,
	FG_RED = 0x000C,
	FG_MAGENTA = 0x000D,
	FG_YELLOW = 0x000E,
	FG_WHITE = 0x000F,
	BG_BLACK = 0x0000,
	BG_DARK_BLUE = 0x0010,
	BG_DARK_GREEN = 0x0020,
	BG_DARK_CYAN = 0x0030,
	BG_DARK_RED = 0x0040,
	BG_DARK_MAGENTA = 0x0050,
	BG_DARK_YELLOW = 0x0060,
	BG_GREY = 0x0070,
	BG_DARK_GREY = 0x0080,
	BG_BLUE = 0x0090,
	BG_GREEN = 0x00A0,
	BG_CYAN = 0x00B0,
	BG_RED = 0x00C0,
	BG_MAGENTA = 0x00D0,
	BG_YELLOW = 0x00E0,
	BG_WHITE = 0x00F0,
};

enum PIXEL_TYPE
{
	PIXEL_SOLID = 0x2588,
	PIXEL_THREEQUARTERS = 0x2593,
	PIXEL_HALF = 0x2592,
	PIXEL_QUARTER = 0x2591,
};

typedef struct
{
	float r;       // a fraction between 0 and 1
	float g;       // a fraction between 0 and 1
	float b;       // a fraction between 0 and 1
} rgb;

typedef struct
{
	float h;       // angle in degrees
	float s;       // a fraction between 0 and 1
	float v;       // a fraction between 0 and 1
} hsv;

hsv rgb2hsv(rgb in)
{
	hsv         out;
	float      min, max, delta;

	min = in.r < in.g ? in.r : in.g;
	min = min  < in.b ? min : in.b;

	max = in.r > in.g ? in.r : in.g;
	max = max  > in.b ? max : in.b;

	out.v = max;                                // v
	delta = max - min;
	if (delta < 0.00001f)
	{
		out.s = 0;
		out.h = 0; // undefined, maybe nan?
		return out;
	}
	if (max > 0.0) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  // s
	}
	else {
		// if max is 0, then r = g = b = 0              
		// s = 0, h is undefined
		out.s = 0.0;
		out.h = NAN;                            // its now undefined
		return out;
	}
	if (in.r >= max)                           // > is bogus, just keeps compilor happy
		out.h = (in.g - in.b) / delta;        // between yellow & magenta
	else
		if (in.g >= max)
			out.h = 2.0 + (in.b - in.r) / delta;  // between cyan & yellow
		else
			out.h = 4.0 + (in.r - in.g) / delta;  // between magenta & cyan

	out.h *= 60.0;                              // degrees

	if (out.h < 0.0)
		out.h += 360.0;

	return out;
}

void ClassifyPixel_Grey(float r, float g, float b, wchar_t &sym, short &fg_col, short &bg_col)
{
	float luminance = 0.2987f * r + 0.5870f * g + 0.1140f * b;
	int pixel_bw = (int)(luminance * 13.0f);
	switch (pixel_bw)
	{
	case 0: bg_col = BG_BLACK; fg_col = FG_BLACK; sym = PIXEL_SOLID; break;

	case 1: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_QUARTER; break;
	case 2: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_HALF; break;
	case 3: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_THREEQUARTERS; break;
	case 4: bg_col = BG_BLACK; fg_col = FG_DARK_GREY; sym = PIXEL_SOLID; break;

	case 5: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_QUARTER; break;
	case 6: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_HALF; break;
	case 7: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_THREEQUARTERS; break;
	case 8: bg_col = BG_DARK_GREY; fg_col = FG_GREY; sym = PIXEL_SOLID; break;

	case 9:  bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_QUARTER; break;
	case 10: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_HALF; break;
	case 11: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_THREEQUARTERS; break;
	case 12: bg_col = BG_GREY; fg_col = FG_WHITE; sym = PIXEL_SOLID; break;
	}
}

void ClassifyPixel_HSL(float r, float g, float b, wchar_t &sym, short &fg_col, short &bg_col)
{
	hsv col = rgb2hsv({ r, g, b });

	const struct { wchar_t c; short fg; short bg; } hues[] =
	{
		{ PIXEL_SOLID,				FG_RED | BG_RED },
	{ PIXEL_QUARTER,			FG_YELLOW | BG_RED },
	{ PIXEL_HALF,				FG_YELLOW | BG_RED },
	{ PIXEL_THREEQUARTERS,		FG_YELLOW | BG_RED },

	{ PIXEL_SOLID,				FG_GREEN | BG_YELLOW },
	{ PIXEL_QUARTER,			FG_GREEN | BG_YELLOW },
	{ PIXEL_HALF,				FG_GREEN | BG_YELLOW },
	{ PIXEL_THREEQUARTERS,		FG_GREEN | BG_YELLOW },

	{ PIXEL_SOLID,				FG_CYAN | BG_GREEN },
	{ PIXEL_QUARTER,			FG_CYAN | BG_GREEN },
	{ PIXEL_HALF,				FG_CYAN | BG_GREEN },
	{ PIXEL_THREEQUARTERS,		FG_CYAN | BG_GREEN },

	{ PIXEL_SOLID,				FG_BLUE | BG_CYAN },
	{ PIXEL_QUARTER,			FG_BLUE | BG_CYAN },
	{ PIXEL_HALF,				FG_BLUE | BG_CYAN },
	{ PIXEL_THREEQUARTERS,		FG_BLUE | BG_CYAN },

	{ PIXEL_SOLID,				FG_MAGENTA | BG_BLUE },
	{ PIXEL_QUARTER,			FG_MAGENTA | BG_BLUE },
	{ PIXEL_HALF,				FG_MAGENTA | BG_BLUE },
	{ PIXEL_THREEQUARTERS,		FG_MAGENTA | BG_BLUE },

	{ PIXEL_SOLID,				FG_RED | BG_MAGENTA },
	{ PIXEL_QUARTER,			FG_RED | BG_MAGENTA },
	{ PIXEL_HALF,				FG_RED | BG_MAGENTA },
	{ PIXEL_THREEQUARTERS,		FG_RED | BG_MAGENTA },

	};

	int index = (int)((col.h / 360.0f) * 24.0f);

	if (col.s > 0.2f)
	{
		sym = hues[index].c;
		fg_col = hues[index].fg;
		bg_col = hues[index].bg;
	}
	else
		ClassifyPixel_Grey(r, g, b, sym, fg_col, bg_col);
}
#pragma endregion

void glReadPixels(int x, int y, int w, int h, int format, int type, void* data) {
	GL_BEGIN_CHECK;

	if ((format != EXT_OLC_PIXEL_FORMAT && format != GL_RGBA && format != GL_RGB && format != GL_DEPTH_COMPONENT)) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	bool isExtOlcType = type == EXT_OLC_PIXEL;

	if ((type != GL_BYTE && type != GL_FLOAT && !isExtOlcType)) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	if ((isExtOlcType && format != EXT_OLC_PIXEL_FORMAT) || (!isExtOlcType && format == EXT_OLC_PIXEL_FORMAT)) {
		context->err = GL_INVALID_OPERATION;
		return;
	}

	if (w <= 0 || h <= 0) {
		context->err = GL_INVALID_VALUE;
		return;
	}

	if (format == GL_RGBA) {
		int size = context->w * context->h;
		for (int i = 0; i < size; i++) {
			if (type == GL_BYTE) {
				((unsigned char*)data)[i * 4 + 0] = context->bufColor->r * 255.0f;
				((unsigned char*)data)[i * 4 + 1] = context->bufColor->g * 255.0f;
				((unsigned char*)data)[i * 4 + 2] = context->bufColor->b * 255.0f;
				((unsigned char*)data)[i * 4 + 3] = context->bufColor->a * 255.0f;
			}
			else if (type == GL_FLOAT) {
				((float*)data)[i * 4 + 0] = context->bufColor->r;
				((float*)data)[i * 4 + 1] = context->bufColor->g;
				((float*)data)[i * 4 + 2] = context->bufColor->b;
				((float*)data)[i * 4 + 3] = context->bufColor->a;
			}
		}
	}
	else if (format == GL_RGB) {
		int size = context->w * context->h;
		for (int i = 0; i < size; i++) {
			if (type == GL_BYTE) {
				((unsigned char*)data)[i * 3 + 0] = context->bufColor->r * 255.0f;
				((unsigned char*)data)[i * 3 + 1] = context->bufColor->g * 255.0f;
				((unsigned char*)data)[i * 3 + 2] = context->bufColor->b * 255.0f;
			}
			else if (type == GL_FLOAT) {
				((float*)data)[i * 3 + 0] = context->bufColor->r;
				((float*)data)[i * 3 + 1] = context->bufColor->g;
				((float*)data)[i * 3 + 2] = context->bufColor->b;
			}
		}
	}
	else if (format == GL_DEPTH_COMPONENT) {
		int size = context->w * context->h;
		for (int i = 0; i < size; i++) {
			if (type == GL_BYTE) {
				((unsigned char*)data)[i] = (char)(context->bufDepth[i] * 255);
			}
			else if (type == GL_FLOAT) {
				((float*)data)[i] = context->bufDepth[i];
			}
		}
	}
	else if (format == EXT_OLC_PIXEL_FORMAT) {
		int size = context->w * context->h;
		OlcPixel* pixels = (OlcPixel*)data;
		for (int i = 0; i < size; i++) {
			Pixel cValue = context->bufColor[i];
			static short FG[] = { 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F };
			static short BG[] = { 0x0000, 0x0010, 0x0020, 0x0030, 0x0040, 0x0050, 0x0060, 0x0070, 0x0080, 0x0090, 0x00A0, 0x00B0, 0x00C0, 0x00D0, 0x00E0, 0x00F0 };
			static const char* CHARS = "0123456789abcdef";
			unsigned char r = (unsigned char)(cValue.r * 255.0f);
			unsigned char g = (unsigned char)(cValue.g * 255.0f);
			unsigned char b = (unsigned char)(cValue.b * 255.0f);

			static Pixel cTable[] = { Pixel(0x000000FF), Pixel(0x000080FF), Pixel(0x008000FF), Pixel(0x008080FF), Pixel(0x800000FF), Pixel(0x800080FF), Pixel(0x808000FF), Pixel(0xC0C0C0FF), Pixel(0x808080FF), Pixel(0x0000FFFF), Pixel(0x00FF00FF), Pixel(0x00FFFFFF), Pixel(0xFF0000FF), Pixel(0xFF00FFFF), Pixel(0xFFFF00FF), Pixel(0xFFFFFFFF) };

			//if (type == EXT_OLC_PIXEL) {
			//	// fast way to turn color to console color, but it is not that good...
			//	int index = (r > 128 || g > 128 || b > 128) ? 8 : 0;
			//	index |= (r > 64) ? 4 : 0;
			//	index |= (g > 64) ? 2 : 0;
			//	index |= (b > 64) ? 1 : 0;
			//	//pixels[i].c = CHARS[index];
			//	pixels[i].c = ' ';
			//	pixels[i].col = BG[index];
			//}
			wchar_t c;
			short bg;
			short fg;
			ClassifyPixel_HSL(cValue.r, cValue.g, cValue.b, c, fg, bg);
			pixels[i].c = c;
			pixels[i].col = fg | bg;
		}
	}
}

#undef GL_BEGIN_CHECK
#undef GL_COLOR_RGBA
#undef GL_CLAMP