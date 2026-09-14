#pragma once

#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

#include "Dependencies\glew.h"

class Renderer
{
public:
	Renderer(int windowSizeX, int windowSizeY);
	~Renderer();

	bool IsInitialized();
	void BeginFrame();
	void EndFrame(float timeSeconds);
	void SetMaterial(int material);
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
	void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType);
	GLuint CompileShaders(const char* filenameVS, const char* filenameFS);
	void CreateVertexBufferObjects();
	void CreateSceneTarget();
	void DestroySceneTarget();
	void GetGLPosition(float x, float y, float* newX, float* newY);
	bool EnsureTextFont();
	GLuint GetTextGlyphList(wchar_t character);

	bool m_Initialized = false;

	unsigned int m_WindowSizeX = 0;
	unsigned int m_WindowSizeY = 0;

	GLuint m_VBORect = 0;
	GLuint m_VBOTriangle = 0;
	GLuint m_VBOFullscreen = 0;
	GLuint m_SolidRectShader = 0;
	GLuint m_PostProcessShader = 0;
	GLuint m_SceneFramebuffer = 0;
	GLuint m_SceneTexture = 0;
	void* m_TextFont = NULL;
	std::map<wchar_t, GLuint> m_TextGlyphs;
};

