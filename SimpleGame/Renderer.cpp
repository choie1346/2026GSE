#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <climits>
#include <cmath>
#include <cstring>
#include "Renderer.h"

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
	Initialize(windowSizeX, windowSizeY);
}

Renderer::~Renderer()
{
	for (std::map<wchar_t, GLuint>::iterator it = m_TextGlyphs.begin(); it != m_TextGlyphs.end(); ++it)
	{
		if (it->second > 0)
		{
			glDeleteLists(it->second, 1);
		}
	}
	m_TextGlyphs.clear();

	if (m_TextFont != NULL)
	{
		DeleteObject((HFONT)m_TextFont);
		m_TextFont = NULL;
	}

	if (m_VBORect > 0)
	{
		glDeleteBuffers(1, &m_VBORect);
	}
	if (m_VBOTriangle > 0)
	{
		glDeleteBuffers(1, &m_VBOTriangle);
	}
	if (m_VBOFullscreen > 0)
	{
		glDeleteBuffers(1, &m_VBOFullscreen);
	}
	if (m_SolidRectShader > 0)
	{
		glDeleteProgram(m_SolidRectShader);
	}
	if (m_PostProcessShader > 0)
	{
		glDeleteProgram(m_PostProcessShader);
	}
	DestroySceneTarget();
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
	SetWindowSize(windowSizeX, windowSizeY);

	m_SolidRectShader = CompileShaders("./Shaders/SolidRect.vs", "./Shaders/SolidRect.fs");
	m_PostProcessShader = CompileShaders("./Shaders/PostProcess.vs", "./Shaders/PostProcess.fs");
	CreateVertexBufferObjects();
	CreateSceneTarget();

	if (m_SolidRectShader > 0 && m_VBORect > 0 && m_VBOTriangle > 0 && m_VBOFullscreen > 0)
	{
		m_Initialized = true;
	}
}

bool Renderer::IsInitialized()
{
	return m_Initialized;
}

void Renderer::BeginFrame()
{
	if (m_SceneFramebuffer > 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetMaterial(int material)
{
	glUseProgram(m_SolidRectShader);
	glUniform1i(glGetUniformLocation(m_SolidRectShader, "u_Material"), material);
}

void Renderer::SetWorld(float cameraX, float cameraY, float time)
{
	glUseProgram(m_SolidRectShader);
	glUniform2f(glGetUniformLocation(m_SolidRectShader, "u_Origin"), cameraX - m_WindowSizeX * .5f, cameraY - m_WindowSizeY * .5f);
	glUniform1f(glGetUniformLocation(m_SolidRectShader, "u_Time"), time);
}

void Renderer::EndFrame(float timeSeconds)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_WindowSizeX, m_WindowSizeY);

	if (m_SceneTexture == 0 || m_PostProcessShader == 0 || m_VBOFullscreen == 0)
	{
		return;
	}

	glDisable(GL_BLEND);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(m_PostProcessShader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
	glUniform1i(glGetUniformLocation(m_PostProcessShader, "u_Scene"), 0);
	glUniform1f(glGetUniformLocation(m_PostProcessShader, "u_Time"), timeSeconds);
	glUniform2f(glGetUniformLocation(m_PostProcessShader, "u_Resolution"), (float)m_WindowSizeX, (float)m_WindowSizeY);

	int attribPosition = glGetAttribLocation(m_PostProcessShader, "a_Position");
	glEnableVertexAttribArray(attribPosition);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOFullscreen);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(attribPosition);

	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glEnable(GL_BLEND);
}

void Renderer::SetWindowSize(int windowSizeX, int windowSizeY)
{
	if (windowSizeX <= 0)
	{
		windowSizeX = 1;
	}
	if (windowSizeY <= 0)
	{
		windowSizeY = 1;
	}

	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;
	glViewport(0, 0, windowSizeX, windowSizeY);
	if (m_SceneFramebuffer > 0)
	{
		CreateSceneTarget();
	}
}

void Renderer::CreateVertexBufferObjects()
{
	float rect[] =
	{
		-1.f / m_WindowSizeX, -1.f / m_WindowSizeY, 0.f,
		-1.f / m_WindowSizeX,  1.f / m_WindowSizeY, 0.f,
		 1.f / m_WindowSizeX,  1.f / m_WindowSizeY, 0.f,

		-1.f / m_WindowSizeX, -1.f / m_WindowSizeY, 0.f,
		 1.f / m_WindowSizeX,  1.f / m_WindowSizeY, 0.f,
		 1.f / m_WindowSizeX, -1.f / m_WindowSizeY, 0.f,
	};

	glGenBuffers(1, &m_VBORect);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);

	glGenBuffers(1, &m_VBOTriangle);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, NULL, GL_DYNAMIC_DRAW);

	float fullscreen[] =
	{
		-1.f, -1.f, 0.f,
		 1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		-1.f, -1.f, 0.f,
		 1.f,  1.f, 0.f,
		-1.f,  1.f, 0.f
	};
	glGenBuffers(1, &m_VBOFullscreen);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOFullscreen);
	glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreen), fullscreen, GL_STATIC_DRAW);
}

void Renderer::CreateSceneTarget()
{
	DestroySceneTarget();

	if (m_PostProcessShader == 0 || m_WindowSizeX == 0 || m_WindowSizeY == 0)
	{
		return;
	}

	glGenTextures(1, &m_SceneTexture);
	glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_WindowSizeX, m_WindowSizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenFramebuffers(1, &m_SceneFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneTexture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "장면 프레임버퍼를 만들 수 없습니다. 기본 화면으로 계속합니다.\n";
		DestroySceneTarget();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DestroySceneTarget()
{
	if (m_SceneFramebuffer > 0)
	{
		glDeleteFramebuffers(1, &m_SceneFramebuffer);
		m_SceneFramebuffer = 0;
	}
	if (m_SceneTexture > 0)
	{
		glDeleteTextures(1, &m_SceneTexture);
		m_SceneTexture = 0;
	}
}

void Renderer::AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0)
	{
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
	}

	const GLchar* p[1];
	p[0] = pShaderText;
	GLint Lengths[1];

	size_t slen = strlen(pShaderText);
	if (slen > INT_MAX)
	{
		return;
	}
	GLint len = (GLint)slen;

	Lengths[0] = len;
	glShaderSource(ShaderObj, 1, p, Lengths);

	glCompileShader(ShaderObj);

	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar InfoLog[1024];

		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		printf("%s \n", pShaderText);
	}

	glAttachShader(ShaderProgram, ShaderObj);
	glDeleteShader(ShaderObj);
}

bool Renderer::ReadFile(const char* filename, std::string* target)
{
	std::ifstream file(filename);
	if (file.fail())
	{
		std::cout << filename << " file loading failed.. \n";
		file.close();
		return false;
	}
	std::string line;
	while (getline(file, line))
	{
		target->append(line.c_str());
		target->append("\n");
	}
	return true;
}

GLuint Renderer::CompileShaders(const char* filenameVS, const char* filenameFS)
{
	GLuint ShaderProgram = glCreateProgram();

	if (ShaderProgram == 0)
	{
		fprintf(stderr, "Error creating shader program\n");
	}

	std::string vs, fs;

	if (!ReadFile(filenameVS, &vs))
	{
		printf("Error compiling vertex shader\n");
		return 0;
	}

	if (!ReadFile(filenameFS, &fs))
	{
		printf("Error compiling fragment shader\n");
		return 0;
	}

	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
	AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };

	glLinkProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);

	if (Success == 0)
	{
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		std::cout << filenameVS << ", " << filenameFS << " Error linking shader program\n" << ErrorLog;
		return 0;
	}

	glValidateProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(ShaderProgram, sizeof(ErrorLog), NULL, ErrorLog);
		std::cout << filenameVS << ", " << filenameFS << " Error validating shader program\n" << ErrorLog;
		return 0;
	}

	glUseProgram(ShaderProgram);
	std::cout << filenameVS << ", " << filenameFS << " Shader compiling is done.";

	return ShaderProgram;
}

void Renderer::DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a)
{
	float newX, newY;

	GetGLPosition(x, y, &newX, &newY);

	glUseProgram(m_SolidRectShader);

	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Trans"), newX, newY, 0, size);
	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Color"), r, g, b, a);

	int attribPosition = glGetAttribLocation(m_SolidRectShader, "a_Position");
	glEnableVertexAttribArray(attribPosition);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(attribPosition);

}

void Renderer::DrawSolidTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b, float a)
{
	float vertices[9];

	GetGLPosition(x1, y1, &vertices[0], &vertices[1]);
	vertices[2] = 0.f;
	GetGLPosition(x2, y2, &vertices[3], &vertices[4]);
	vertices[5] = 0.f;
	GetGLPosition(x3, y3, &vertices[6], &vertices[7]);
	vertices[8] = 0.f;

	glUseProgram(m_SolidRectShader);

	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Trans"), 0.f, 0.f, 0.f, 1.f);
	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Color"), r, g, b, a);

	int attribPosition = glGetAttribLocation(m_SolidRectShader, "a_Position");
	glEnableVertexAttribArray(attribPosition);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(attribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(attribPosition);

}

void Renderer::DrawSolidQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float r, float g, float b, float a)
{
	DrawSolidTriangle(x1, y1, x2, y2, x3, y3, r, g, b, a);
	DrawSolidTriangle(x1, y1, x3, y3, x4, y4, r, g, b, a);
}

void Renderer::DrawSolidEllipse(float x, float y, float radiusX, float radiusY, int segments, float r, float g, float b, float a)
{
	if (segments < 8)
	{
		segments = 8;
	}
	if (segments > 96)
	{
		segments = 96;
	}

	float vertices[98 * 3] = {};
	GetGLPosition(x, y, &vertices[0], &vertices[1]);
	for (int i = 0; i <= segments; ++i) {
		float angle = i * 6.2831853f / segments;
		GetGLPosition(x + std::cos(angle) * radiusX, y + std::sin(angle) * radiusY,
			&vertices[(i + 1) * 3], &vertices[(i + 1) * 3 + 1]);
	}
	glUseProgram(m_SolidRectShader);
	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Trans"), 0, 0, 0, 1);
	glUniform4f(glGetUniformLocation(m_SolidRectShader, "u_Color"), r, g, b, a);
	GLint position = glGetAttribLocation(m_SolidRectShader, "a_Position");
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangle);
	glBufferData(GL_ARRAY_BUFFER, (segments + 2) * 3 * sizeof(float), vertices, GL_STREAM_DRAW);
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
	glDisableVertexAttribArray(position);
}

bool Renderer::EnsureTextFont()
{
	if (m_TextFont != NULL)
	{
		return true;
	}

	HDC deviceContext = wglGetCurrentDC();
	if (deviceContext == NULL)
	{
		return false;
	}

	HFONT font = CreateFontW(
		-22,
		0,
		0,
		0,
		FW_SEMIBOLD,
		FALSE,
		FALSE,
		FALSE,
		HANGEUL_CHARSET,
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"Malgun Gothic");

	if (font == NULL)
	{
		return false;
	}

	m_TextFont = font;
	return true;
}

GLuint Renderer::GetTextGlyphList(wchar_t character)
{
	std::map<wchar_t, GLuint>::iterator found = m_TextGlyphs.find(character);
	if (found != m_TextGlyphs.end())
	{
		return found->second;
	}

	if (!EnsureTextFont())
	{
		return 0;
	}

	HDC deviceContext = wglGetCurrentDC();
	if (deviceContext == NULL)
	{
		return 0;
	}

	GLuint list = glGenLists(1);
	if (list == 0)
	{
		return 0;
	}

	HGDIOBJ previousFont = SelectObject(deviceContext, (HFONT)m_TextFont);
	BOOL success = wglUseFontBitmapsW(deviceContext, (DWORD)character, 1, list);
	SelectObject(deviceContext, previousFont);
	if (!success)
	{
		glDeleteLists(list, 1);
		return 0;
	}

	m_TextGlyphs[character] = list;
	return list;
}

void Renderer::DrawText(float x, float y, const wchar_t* text, float r, float g, float b, float a)
{
	if (text == NULL || !EnsureTextFont())
	{
		return;
	}

	float startX = x + m_WindowSizeX * 0.5f;
	float lineY = y + m_WindowSizeY * 0.5f;

	glUseProgram(0);
	glColor4f(r, g, b, a);
	glWindowPos2f(startX, lineY);

	for (const wchar_t* character = text; *character != L'\0'; ++character)
	{
		if (*character == L'\n')
		{
			lineY -= 27.f;
			glWindowPos2f(startX, lineY);
			continue;
		}

		GLuint list = GetTextGlyphList(*character);
		if (list > 0)
		{
			glCallList(list);
		}
	}
}

void Renderer::GetGLPosition(float x, float y, float* newX, float* newY)
{
	*newX = x * 2.f / m_WindowSizeX;
	*newY = y * 2.f / m_WindowSizeY;
}
