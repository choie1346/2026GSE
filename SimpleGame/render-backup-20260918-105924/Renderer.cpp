#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include <climits>
#include <cmath>
#include <cstring>
#include <vector>
#include "Renderer.h"
#include "ShaderSources.generated.h"

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
	Initialize(windowSizeX, windowSizeY);
}

Renderer::~Renderer()
{
	for (const auto& glyph : m_TextGlyphs) if (glyph.second.texture) glDeleteTextures(1, &glyph.second.texture);
	m_TextGlyphs.clear();
	if (m_TextShader) glDeleteProgram(m_TextShader);

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
	if (m_LakeShader) glDeleteProgram(m_LakeShader);
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
	SetWindowSize(windowSizeX, windowSizeY);

	m_SolidRectShader = CompileShaders("./Shaders/SolidRect.vs", "./Shaders/SolidRect.fs");
	m_LakeShader = CompileShaders("./Shaders/SolidRect.vs", "./Shaders/Lake.fs");
	m_DrawShader = m_SolidRectShader;
	m_TextShader = CompileShaders("./Shaders/SolidRect.vs", "./Shaders/Text.fs");
	glVertexAttrib1f(1, 1.f);
	m_PostProcessShader = CompileShaders("./Shaders/PostProcess.vs", "./Shaders/PostProcess.fs");
	CreateVertexBufferObjects();
	CreateSceneTarget();
	const char* required[] = { "u_Color","u_Origin","u_Time","u_Material","u_RenderScale" };
	if (m_SolidRectShader) for (const char* name : required) {
		if (glGetUniformLocation(m_SolidRectShader, name) < 0) {
			std::cerr << "Missing material uniform: " << name << std::endl;
			glDeleteProgram(m_SolidRectShader); m_SolidRectShader = 0; m_DrawShader = 0;
			break;
		}
	}

	if (m_SolidRectShader > 0 && m_LakeShader > 0 && m_TextShader > 0 && m_PostProcessShader > 0 && m_SceneFramebuffer > 0 && m_VBORect > 0 && m_VBOTriangle > 0 && m_VBOFullscreen > 0)
	{
		m_Initialized = true;
	}
	else {
		MessageBoxW(nullptr, L"렌더링 초기화에 실패했습니다. 콘솔의 셰이더 오류를 확인해 주세요.", L"렌더링 오류", MB_OK | MB_ICONERROR);
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

	glViewport(0, 0, m_WindowSizeX * m_RenderScale, m_WindowSizeY * m_RenderScale);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetMaterial(int material)
{
	m_DrawShader = (material == 3 || material == 8) && m_LakeShader ? m_LakeShader : m_SolidRectShader;
	glUseProgram(m_DrawShader);
	glUniform1i(glGetUniformLocation(m_DrawShader, "u_Material"), material);
	if (m_DrawShader == m_LakeShader)glUniform1i(glGetUniformLocation(m_DrawShader, "u_GenericWater"), material == 8);
}

void Renderer::SetWorld(float cameraX, float cameraY, float time)
{
	GLuint programs[] = { m_SolidRectShader, m_LakeShader };
	for (GLuint program : programs) if (program) {
		glUseProgram(program);
		glUniform2f(glGetUniformLocation(program, "u_Origin"), cameraX - m_WindowSizeX * .5f, cameraY - m_WindowSizeY * .5f);
		glUniform1f(glGetUniformLocation(program, "u_Time"), time);
		glUniform1f(glGetUniformLocation(program, "u_RenderScale"), (float)m_RenderScale);
	}
}

void Renderer::DrawCachedModel(GLuint buffer, int vertexCount, float x, float y, float scale, float flash)
{
	SetMaterial(0);
	glUseProgram(m_DrawShader);
	glUniform1i(glGetUniformLocation(m_DrawShader, "u_Model"), 1);
	glUniform2f(glGetUniformLocation(m_DrawShader, "u_ModelScale"), 2.f * scale / m_WindowSizeX, 2.f * scale / m_WindowSizeY);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Trans"), 2.f * x / m_WindowSizeX, 2.f * y / m_WindowSizeY, 0, 1);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Color"), 1 + flash, 1 + flash, 1 + flash, 1);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glEnableVertexAttribArray(0); glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), 0);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)(2 * sizeof(float)));
	glVertexAttrib1f(1, 1);
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	glDisableVertexAttribArray(0); glDisableVertexAttribArray(2);
	glUniform1i(glGetUniformLocation(m_DrawShader, "u_Model"), 0);
}

void Renderer::DrawFlame(float x, float y, float size)
{
	SetMaterial(7);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Flame"),
		(x + m_WindowSizeX * .5f) * m_RenderScale, (y + m_WindowSizeY * .5f) * m_RenderScale, size * m_RenderScale, size * 2 * m_RenderScale);
	DrawSolidQuad(x - size, y, x + size, y, x + size, y + size * 2, x - size, y + size * 2, 1, 1, 1, 1);
	SetMaterial(0);
}

void Renderer::DrawShadow(float x, float y, float radius, float length)
{
	// Actual elliptical geometry: no rectangular mask can leak onto the terrain.
	SetMaterial(0);
	const int segments = 64;
	float vertices[(segments + 2) * 4] = {};
	const float cx = x + length * .35f, cy = y - length * .18f;
	GetGLPosition(cx, cy, &vertices[0], &vertices[1]);
	vertices[3] = 1.f;
	for (int i = 0; i <= segments; ++i) {
		float angle = i * 6.2831853f / segments;
		float along = std::cos(angle) * (radius + length * .45f);
		float across = std::sin(angle) * (radius * .38f + length * .06f);
		GetGLPosition(cx + along * .88f + across * .47f, cy - along * .47f + across * .88f,
			&vertices[(i + 1) * 4], &vertices[(i + 1) * 4 + 1]);
	}
	glUseProgram(m_DrawShader);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Trans"), 0, 0, 0, 1);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Color"), .035f, .05f, .065f, .38f);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOTriangle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glVertexAttrib1f(1, 1.f);
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
	glUniform2f(glGetUniformLocation(m_PostProcessShader, "u_Resolution"), (float)m_WindowSizeX * m_RenderScale, (float)m_WindowSizeY * m_RenderScale);

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
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	m_RenderScale = m_WindowSizeX * 2u <= (unsigned)maxTextureSize && m_WindowSizeY * 2u <= (unsigned)maxTextureSize ? 2 : 1;
	glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_WindowSizeX * m_RenderScale, m_WindowSizeY * m_RenderScale, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenFramebuffers(1, &m_SceneFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneTexture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "장면 프레임버퍼를 만들 수 없습니다. 기본 화면으로 계속합니다.\n";
		DestroySceneTarget();
		m_RenderScale = 1;
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

bool Renderer::AddShader(GLuint program, const char* source, GLenum type)
{
	GLuint shader = glCreateShader(type);
	if (!shader) return false;
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[8192] = {};
		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
		std::cerr << "Shader compile failed: " << log << std::endl;
		glDeleteShader(shader);
		return false;
	}
	glAttachShader(program, shader);
	glDeleteShader(shader);
	return true;
}

bool Renderer::ReadFile(const char* filename, std::string* target)
{
	// Linked with Renderer.cpp itself; no resource compiler or runtime path lookup.
	for (const auto& shader : ShaderSources::entries) {
		if (std::strcmp(filename, shader.path) == 0) {
			target->assign(shader.source);
			std::cout << "Compiled-in shader: " << filename << " (" << target->size() << " bytes)" << std::endl;
			return true;
		}
	}
	std::cerr << "Unknown compiled-in shader: " << filename << std::endl;
	return false;
}

GLuint Renderer::CompileShaders(const char* vertexFile, const char* fragmentFile)
{
	std::string vertex, fragment;
	if (!ReadFile(vertexFile, &vertex) || !ReadFile(fragmentFile, &fragment)) return 0;
	GLuint program = glCreateProgram();
	if (!program) return 0;
	if (!AddShader(program, vertex.c_str(), GL_VERTEX_SHADER) || !AddShader(program, fragment.c_str(), GL_FRAGMENT_SHADER)) {
		glDeleteProgram(program);
		return 0;
	}
	glLinkProgram(program);
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char log[8192] = {};
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		std::cerr << fragmentFile << ": " << log << std::endl;
		glDeleteProgram(program);
		return 0;
	}
	return program;
}

void Renderer::DrawSolidRect(float x, float y, float z, float size, float r, float g, float b, float a)
{
	float newX, newY;

	GetGLPosition(x, y, &newX, &newY);

	glUseProgram(m_DrawShader);

	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Trans"), newX, newY, 0, size);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Color"), r, g, b, a);

	int attribPosition = glGetAttribLocation(m_DrawShader, "a_Position");
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

	glUseProgram(m_DrawShader);

	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Trans"), 0.f, 0.f, 0.f, 1.f);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Color"), r, g, b, a);

	int attribPosition = glGetAttribLocation(m_DrawShader, "a_Position");
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
	glUseProgram(m_DrawShader);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Trans"), 0, 0, 0, 1);
	glUniform4f(glGetUniformLocation(m_DrawShader, "u_Color"), r, g, b, a);
	GLint position = glGetAttribLocation(m_DrawShader, "a_Position");
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
		-50,
		0,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		HANGEUL_CHARSET,
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_ROMAN,
		L"Noto Serif KR");

	if (font == NULL)
	{
		return false;
	}

	m_TextFont = font;
	return true;
}

const Renderer::Glyph& Renderer::GetTextGlyph(wchar_t character)
{
	auto found = m_TextGlyphs.find(character);
	if (found != m_TextGlyphs.end()) return found->second;
	Glyph glyph;
	if (!EnsureTextFont()) return m_TextGlyphs.emplace(character, glyph).first->second;
	HDC dc = CreateCompatibleDC(nullptr);
	if (!dc) return m_TextGlyphs.emplace(character, glyph).first->second;
	HGDIOBJ previous = SelectObject(dc, (HFONT)m_TextFont);
	MAT2 transform = {};
	transform.eM11.value = transform.eM22.value = 1;
	GLYPHMETRICS metrics = {};
	DWORD size = GetGlyphOutlineW(dc, character, GGO_GRAY8_BITMAP, &metrics, 0, nullptr, &transform);
	if (size != GDI_ERROR) {
		glyph.width = metrics.gmBlackBoxX; glyph.height = metrics.gmBlackBoxY;
		glyph.left = metrics.gmptGlyphOrigin.x; glyph.top = metrics.gmptGlyphOrigin.y;
		glyph.advance = metrics.gmCellIncX;
		if (size && glyph.width && glyph.height) {
			std::vector<unsigned char> bitmap(size);
			if (GetGlyphOutlineW(dc, character, GGO_GRAY8_BITMAP, &metrics, size, bitmap.data(), &transform) != GDI_ERROR) {
				const int pitch = (glyph.width + 3) & ~3;
				std::vector<unsigned char> coverage(glyph.width * glyph.height);
				for (int y = 0; y < glyph.height; ++y) for (int x = 0; x < glyph.width; ++x)
					coverage[y * glyph.width + x] = (unsigned char)(bitmap[y * pitch + x] * 255 / 64);
				glGenTextures(1, &glyph.texture);
				glBindTexture(GL_TEXTURE_2D, glyph.texture);
				GLint alignment = 4;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, glyph.width, glyph.height, 0, GL_RED, GL_UNSIGNED_BYTE, coverage.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
			}
		}
	}
	else {
		SIZE extent = {};
		GetTextExtentPoint32W(dc, &character, 1, &extent);
		glyph.advance = extent.cx;
		std::cerr << "Glyph rasterization failed: " << (unsigned)character << std::endl;
	}
	SelectObject(dc, previous);
	DeleteDC(dc);
	return m_TextGlyphs.emplace(character, glyph).first->second;
}

float Renderer::TextAdvance(wchar_t character)
{
	return GetTextGlyph(character).advance * .5f;
}

void Renderer::DrawText(float x, float y, const wchar_t* text, float r, float g, float b, float a)
{
	if (!text || !EnsureTextFont()) return;
	const float start = x;
	GLuint previous = m_DrawShader;
	m_DrawShader = m_TextShader;
	glActiveTexture(GL_TEXTURE0);
	for (const wchar_t* c = text; *c; ++c) {
		if (*c == L'\n') { x = start; y -= 32; continue; }
		const Glyph& glyph = GetTextGlyph(*c);
		if (glyph.texture) {
			float left = x + glyph.left * .5f, bottom = y + (glyph.top - glyph.height) * .5f;
			float w = glyph.width * .5f, h = glyph.height * .5f;
			glUseProgram(m_TextShader);
			glBindTexture(GL_TEXTURE_2D, glyph.texture);
			glUniform1i(glGetUniformLocation(m_TextShader, "u_Glyph"), 0);
			glUniform4f(glGetUniformLocation(m_TextShader, "u_TextRect"), left + m_WindowSizeX * .5f, bottom + m_WindowSizeY * .5f, w, h);
			DrawSolidQuad(left, bottom, left + w, bottom, left + w, bottom + h, left, bottom + h, r, g, b, a);
		}
		x += glyph.advance * .5f;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	m_DrawShader = previous;
}

void Renderer::GetGLPosition(float x, float y, float* newX, float* newY)
{
	*newX = x * 2.f / m_WindowSizeX;
	*newY = y * 2.f / m_WindowSizeY;
}
