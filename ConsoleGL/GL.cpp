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
			unsigned char a;
			unsigned char b;
			unsigned char g;
			unsigned char r;
		};
		unsigned int rgba;
	};

	Pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
		:	r(r),
			g(g),
			b(b),
			a(a)
	{

	}

	Pixel(int pixel)
		: rgba(pixel)
	{

	}

	Pixel(float r, float g, float b, float a)
		:	r((unsigned char)(GL_CLAMP(r) * 255)),
			g((unsigned char)(GL_CLAMP(g) * 255)),
			b((unsigned char)(GL_CLAMP(b) * 255)),
			a((unsigned char)(GL_CLAMP(a) * 255))
	{

	}

	Pixel()
		:	r(0),
			g(0),
			b(0),
			a((char)255)
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
		std::memset(bufColor, bufColorClear.rgba, w * h);
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
	case EXT_OLC_SLOW_COLOR: context->extOlcSlowColor = true; break;
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
	case EXT_OLC_SLOW_COLOR: context->extOlcSlowColor = false; break;
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
			context->bufColor[i] = context->bufColorClear.rgba;
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

void glTranslate(float x, float y, float z) {
	GL_BEGIN_CHECK;
	
	*context->curMatrix = glm::translate(*context->curMatrix, glm::vec3(x, y, z));
}

void glScale(float x, float y, float z) {
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

void glReadPixels(int x, int y, int w, int h, int format, int type, void* data) {
	GL_BEGIN_CHECK;

	if ((format != EXT_OLC_PIXEL_FORMAT && format != GL_RGBA && format != GL_RGB && format != GL_DEPTH_COMPONENT)) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	if ((type != GL_BYTE && type != GL_FLOAT && type != EXT_OLC_PIXEL)) {
		context->err = GL_INVALID_ENUM;
		return;
	}

	if ((type == EXT_OLC_PIXEL && format != EXT_OLC_PIXEL_FORMAT) || (type != EXT_OLC_PIXEL && format == EXT_OLC_PIXEL_FORMAT)) {
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
				((unsigned char*)data)[i * 4 + 0] = context->bufColor->r;
				((unsigned char*)data)[i * 4 + 1] = context->bufColor->g;
				((unsigned char*)data)[i * 4 + 2] = context->bufColor->b;
				((unsigned char*)data)[i * 4 + 3] = context->bufColor->a;
			}
			else if (type == GL_FLOAT) {
				((float*)data)[i * 4 + 0] = context->bufColor->r / 255.0f;
				((float*)data)[i * 4 + 1] = context->bufColor->g / 255.0f;
				((float*)data)[i * 4 + 2] = context->bufColor->b / 255.0f;
				((float*)data)[i * 4 + 3] = context->bufColor->a / 255.0f;
			}
		}
	}
	else if (format == GL_RGB) {
		int size = context->w * context->h;
		for (int i = 0; i < size; i++) {
			if (type == GL_BYTE) {
				((unsigned char*)data)[i * 3 + 0] = context->bufColor->r;
				((unsigned char*)data)[i * 3 + 1] = context->bufColor->g;
				((unsigned char*)data)[i * 3 + 2] = context->bufColor->b;
			}
			else if (type == GL_FLOAT) {
				((float*)data)[i * 3 + 0] = context->bufColor->r / 255.0f;
				((float*)data)[i * 3 + 1] = context->bufColor->g / 255.0f;
				((float*)data)[i * 3 + 2] = context->bufColor->b / 255.0f;
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
			if (context->extOlcSlowColor) {
				// transform the pixels to the currect format - too slow, will have to try something else
				static Pixel cTable[] = { Pixel(0x000000FF), Pixel(0x000080FF), Pixel(0x008000FF), Pixel(0x008080FF), Pixel(0x800000FF), Pixel(0x800080FF), Pixel(0x808000FF), Pixel(0xC0C0C0FF), Pixel(0x808080FF), Pixel(0x0000FFFF), Pixel(0x00FF00FF), Pixel(0x00FFFFFF), Pixel(0xFF0000FF), Pixel(0xFF00FFFF), Pixel(0xFFFF00FF), Pixel(0xFFFFFFFF) };
				static wchar_t rList[] = { (wchar_t)0x2591, (wchar_t)0x2592, (wchar_t)0x2593, (wchar_t)0x2588 };
				int bestHit[] = { 0, 0, 4, INT32_MAX };
				for (int rChar = sizeof(rList); rChar > 0; rChar--) {
					for (int cFore = 0; cFore < sizeof(cTable); cFore++) {
						for (int cBack = 0; cBack < sizeof(cTable); cBack++) {
							int R = (cTable[cFore].r * rChar + cTable[cBack].r * (sizeof(rList) - rChar)) / sizeof(rList);
							int G = (cTable[cFore].g * rChar + cTable[cBack].g * (sizeof(rList) - rChar)) / sizeof(rList);
							int B = (cTable[cFore].b * rChar + cTable[cBack].b * (sizeof(rList) - rChar)) / sizeof(rList);
							int iScore = (cValue.r - R) * (cValue.r - R) + (cValue.g - G) * (cValue.g - G) + (cValue.b - B) * (cValue.b - B);
							if (!(rChar > 1 && rChar < 4 && iScore > 5000)) {
								if (iScore < bestHit[3]) {
									bestHit[3] = iScore;
									bestHit[0] = cFore;
									bestHit[1] = cBack;
									bestHit[2] = rChar;
								}
							}
						}
					}
				}
				pixels[i].c = rList[bestHit[2]];
				pixels[i].col = FG[bestHit[0]] | BG[bestHit[1]];
			}
			else {
				// fast way to turn color to console color, but it is not that good...
				int index = (cValue.r > 128 || cValue.g > 128 || cValue.b > 128) ? 8 : 0;
				index |= (cValue.r > 64) ? 4 : 0;
				index |= (cValue.g > 64) ? 2 : 0;
				index |= (cValue.b > 64) ? 1 : 0;
				//pixels[i].c = CHARS[index];
				pixels[i].c = ' ';
				pixels[i].col = BG[index];

			}			
		}
	}
}

#undef GL_BEGIN_CHECK
#undef GL_COLOR_RGBA
#undef GL_CLAMP