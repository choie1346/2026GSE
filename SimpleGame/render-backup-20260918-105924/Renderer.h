#pragma once

#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

#include "Dependencies\glew.h"

#ifdef DrawText
#undef DrawText
#endif

class Renderer
{
public:
	struct ModelVertex { float x, y, r, g, b, a; };
	void DrawCachedModel(GLuint buffer, int vertexCount, float x, float y, float scale = 1.f, float flash = 0.f);
	void DrawFlame(float x, float y, float size);
	Renderer(int windowSizeX, int windowSizeY);
	~Renderer();

	bool IsInitialized();
	void BeginFrame();
	void EndFrame(float timeSeconds);
	void SetMaterial(int material);
	float TextAdvance(wchar_t character);
	void DrawShadow(float x, float y, float radius, float length);
	void SetWorld(float cameraX, float cameraY, float time);
	void SetWindowSize(int windowSizeX, int windowSizeY);
	void DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a);
	void DrawSolidTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a);
	void DrawSolidQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float r, float g, float b, float a);
	void DrawSolidEllipse(float x, float y, float radiusX, float radiusY, int segments, float r, float g, float b, float a);
	void DrawText(float x, float y, const wchar_t* text, float r, float g, float b, float a);

private:
	void Initialize(int windowSizeX, int windowSizeY);
	bool ReadFile(const char* filename, std::string* target);
	bool AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType);
	GLuint CompileShaders(const char* filenameVS, const char* filenameFS);
	void CreateVertexBufferObjects();
	void CreateSceneTarget();
	void DestroySceneTarget();
	void GetGLPosition(float x, float y, float* newX, float* newY);
	bool EnsureTextFont();
	struct Glyph {
		GLuint texture = 0;
		int width = 0, height = 0, left = 0, top = 0, advance = 0;
	};
	const Glyph& GetTextGlyph(wchar_t character);

	bool m_Initialized = false;

	unsigned int m_WindowSizeX = 0;
	unsigned int m_WindowSizeY = 0;

	GLuint m_VBORect = 0;
	GLuint m_VBOTriangle = 0;
	GLuint m_VBOFullscreen = 0;
	GLuint m_SolidRectShader = 0;
	GLuint m_DrawShader = 0;
	GLuint m_LakeShader = 0;
	GLuint m_TextShader = 0;
	GLuint m_PostProcessShader = 0;
	GLuint m_SceneFramebuffer = 0;
	GLuint m_SceneTexture = 0;
	int m_RenderScale = 1;
	void* m_TextFont = NULL;
	std::map<wchar_t, Glyph> m_TextGlyphs;
};

