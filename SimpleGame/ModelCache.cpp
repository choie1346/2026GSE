#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include "ModelCache.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace {
    using Vertex = Renderer::ModelVertex;
    static_assert(sizeof(Vertex) == 6 * sizeof(float), "Cache vertex layout changed; increment the version.");
    using Vertices = std::vector<Vertex>;
    struct Color { float r, g, b, a; };
    void triangle(Vertices& v, float ax, float ay, float bx, float by, float cx, float cy, Color c) {
        v.push_back({ ax,ay,c.r,c.g,c.b,c.a }); v.push_back({ bx,by,c.r,c.g,c.b,c.a }); v.push_back({ cx,cy,c.r,c.g,c.b,c.a });
    }
    void rectangle(Vertices& v, float x, float y, float w, float h, Color c) {
        triangle(v, x - w / 2, y - h / 2, x + w / 2, y - h / 2, x + w / 2, y + h / 2, c);
        triangle(v, x - w / 2, y - h / 2, x + w / 2, y + h / 2, x - w / 2, y + h / 2, c);
    }
    void oval(Vertices& v, float x, float y, float rx, float ry, Color c) {
        for (int i = 0; i < 40; ++i) {
            float a = i * 6.2831853f / 40, b = (i + 1) * 6.2831853f / 40;
            triangle(v, x, y, x + std::cos(a) * rx, y + std::sin(a) * ry, x + std::cos(b) * rx, y + std::sin(b) * ry, c);
        }
    }
    uint32_t checksum(const Vertices& v) {
        uint32_t result = 2166136261u;
        const auto* bytes = reinterpret_cast<const unsigned char*>(v.data());
        for (size_t i = 0; i < v.size() * sizeof(Vertex); ++i) result = (result ^ bytes[i]) * 16777619u;
        return result;
    }
}
ModelCache::~ModelCache() { for (auto& m : models)if (m.buffer)glDeleteBuffers(1, &m.buffer); }
void ModelCache::Build() {
    models.clear(); models.resize(22);
    for (int type = 0; type < 2; ++type)for (int frame = 0; frame < 8; ++frame) {
        auto& v = models[type * 8 + frame].vertices;
        // Match the tutorial's tapered coat, joint positions and head proportions.
        float cycle = std::sin(frame * 6.2831853f / 8), breath = std::abs(cycle) * 1.1f;
        Color coat = type ? Color{ .47f,.37f,.39f,1 } : Color{ .30f,.46f,.67f,1 };
        Color skin = type ? Color{ .74f,.595f,.47f,1 } : Color{ .70f,.57f,.45f,1 };
        Color leather = { .27f,.25f,.22f,1 }, hair = type ? Color{ .295f,.25f,.19f,1 } : Color{ .25f,.28f,.19f,1 };
        auto shade = [](Color c, float light)->Color {return { c.r * light,c.g * light,c.b * light,c.a }; };
        auto limb = [&](float ax, float ay, float bx, float by, float radius, Color color) {
            float length = std::hypot(bx - ax, by - ay);
            float nx = -(by - ay) * radius / length, ny = (bx - ax) * radius / length;
            triangle(v, ax + nx, ay + ny, bx + nx, by + ny, bx - nx, by - ny, color);
            triangle(v, ax + nx, ay + ny, bx - nx, by - ny, ax - nx, ay - ny, color);
            oval(v, ax, ay, radius, radius, color); oval(v, bx, by, radius, radius, color);
            };
        for (int side = -1; side <= 1; side += 2) {
            float step = side * cycle * 3.8f;
            limb(side * 4.f, 23, side * 4 + step * .25f, 13 + step, 2.9f, { .32f,.34f,.34f,1 });
            limb(side * 4 + step * .25f, 13 + step, side * 4 - step * .35f, 4 + step * .3f, 2.5f, leather);
            oval(v, side * 4 - step * .35f + 1, 3 + step * .3f, 4.2f, 2.5f, leather);
        }
        triangle(v, -7, 42 + breath, 7, 42 + breath, 10, 20, coat);
        triangle(v, -7, 42 + breath, 10, 20, -9, 20, coat);
        triangle(v, -7, 41 + breath, -9, 20, -2, 20, shade(coat, .72f));
        triangle(v, 1, 40 + breath, 3, 21, 7, 22, shade(coat, 1.14f));
        for (int side = -1; side <= 1; side += 2) {
            float swing = -side * cycle * 3;
            limb(side * 8.f, 40 + breath, side * 11.f, 32 + swing, 3.2f, shade(coat, side < 0 ? .78f : 1.05f));
            limb(side * 11.f, 32 + swing, side * 10.f, 25 + swing, 2.5f, coat);
            oval(v, side * 10.f, 24 + swing, 2.5f, 3.2f, skin);
        }
        limb(-8, 27, 8, 27, 1.3f, leather);
        oval(v, 2, 27, 1.8f, 1.8f, { .76f,.64f,.39f,1 });
        oval(v, 0, 44 + breath, 2.8f, 4, skin);
        oval(v, 0, 53 + breath, 7.5f, 9, hair);
        oval(v, .7f, 51 + breath, 6.3f, 7.5f, skin);
        oval(v, -2, 57 + breath, 6.5f, 4.2f, hair);
        oval(v, -6, 53 + breath, 2, 4.5f, hair);
        for (int side = -1; side <= 1; side += 2)
            oval(v, side * 2.1f, 52 + breath, .8f, 1, { .16f,.18f,.18f,1 });
        oval(v, 1.2f, 49.5f + breath, 1, 1, { .84f,.64f,.48f,1 });
        if (!type) {
            oval(v, 0, 46 + breath, 4.3f, 2.8f, shade(hair, 1.6f));
            oval(v, 0, 60 + breath, 10, 2.5f, shade(coat, .8f));
            oval(v, 0, 63 + breath, 6.5f, 4, coat);
            triangle(v, -7, 42, 1, 35, 7, 42, { .74f,.68f,.49f,1 });
            limb(-6, 41, 7, 26, 1.2f, { .53f,.41f,.27f,1 });
            oval(v, 8, 24, 4.5f, 5.5f, leather);
        }
        else {
            oval(v, 0, 60 + breath, 8, 3, { .36f,.39f,.38f,1 });
            oval(v, -8, 39 + breath, 4, 3, { .47f,.49f,.46f,1 });
        }
        // The weapon is the only additional player equipment in the combat level.
        float handY = 24 - cycle * 3;
        limb(11, handY - 3, 11, handY + 21, 1.2f, { .66f,.70f,.70f,1 });
        limb(7, handY, 15, handY, .9f, { .72f,.61f,.36f,1 });
        if (!type)for (auto& vertex : v) { vertex.x *= 1.04f; vertex.y *= 1.04f; }
    }
    auto& tree = models[16].vertices; rectangle(tree, 0, 27, 12, 54, { .30f,.26f,.20f,1 });
    for (int i = 0; i < 7; ++i) { float a = i * 2.399f; oval(tree, std::cos(a) * 18, 70 + std::sin(a) * 19, 26, 27, { .16f + i * .014f,.35f + i * .018f,.24f + i * .008f,1 }); }
    auto& rock = models[17].vertices; oval(rock, 0, 9, 28, 18, { .36f,.42f,.43f,1 }); oval(rock, -7, 16, 19, 12, { .53f,.58f,.56f,1 });
    auto& loot = models[18].vertices; oval(loot, 0, 9, 8, 10, { .46f,.63f,.60f,1 }); rectangle(loot, 0, 20, 5, 6, { .83f,.71f,.44f,1 }); oval(loot, -2, 12, 3, 5, { .76f,.91f,.85f,1 });
    auto& camp = models[19].vertices;
    for (int i = 0; i < 8; ++i) { float a = i * 6.2831853f / 8; oval(camp, std::cos(a) * 21, std::sin(a) * 9, 8, 6, { .46f,.49f,.46f,1 }); }
    rectangle(camp, 0, 5, 32, 7, { .33f,.24f,.16f,1 });
    auto& shard = models[20].vertices;
    triangle(shard, 0, 25, -9, 10, 0, 0, { .58f,.82f,.85f,1 }); triangle(shard, 0, 25, 0, 0, 9, 10, { .35f,.58f,.71f,1 });
    auto& sword = models[21].vertices;
    rectangle(sword, 0, 20, 5, 29, { .75f,.83f,.87f,1 }); triangle(sword, -2.5f, 34.5f, 2.5f, 34.5f, 0, 41, { .87f,.92f,.94f,1 });
    rectangle(sword, 0, 8, 19, 4, { .83f,.65f,.31f,1 }); rectangle(sword, 0, 2, 4, 12, { .41f,.28f,.17f,1 });
}
bool ModelCache::Read(const std::wstring& path) {
    FILE* file = nullptr; if (_wfopen_s(&file, path.c_str(), L"rb") != 0)return false;
    uint32_t header[3] = {}; bool ok = fread(header, sizeof(header), 1, file) == 1 && header[0] == 0x4D434831 && header[1] == 3 && header[2] == 22;
    std::vector<Mesh> loaded(22);
    for (auto& m : loaded) {
        if (!ok)break; uint32_t count = 0, hash = 0;
        ok = fread(&count, 4, 1, file) == 1 && fread(&hash, 4, 1, file) == 1 && count > 0 && count <= 30000 && count % 3 == 0;
        if (!ok)break; m.vertices.resize(count);
        ok = fread(m.vertices.data(), sizeof(Vertex), count, file) == count && checksum(m.vertices) == hash;
        for (const auto& v : m.vertices)if (!std::isfinite(v.x) || !std::isfinite(v.y) || std::abs(v.x) > 512 || std::abs(v.y) > 512 ||
            !std::isfinite(v.r) || !std::isfinite(v.g) || !std::isfinite(v.b) || !std::isfinite(v.a) || v.a < 0 || v.a>1)ok = false;
    }
    if (ok && fgetc(file) != EOF)ok = false; fclose(file);
    if (ok)models = std::move(loaded); return ok;
}
bool ModelCache::Write(const std::wstring& path) const {
    std::wstring temp = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    FILE* file = nullptr; if (_wfopen_s(&file, temp.c_str(), L"wb") != 0)return false;
    uint32_t header[] = { 0x4D434831,3,22 }; bool ok = fwrite(header, sizeof(header), 1, file) == 1;
    for (const auto& m : models) {
        uint32_t count = (uint32_t)m.vertices.size(), hash = checksum(m.vertices);
        ok = ok && fwrite(&count, 4, 1, file) == 1 && fwrite(&hash, 4, 1, file) == 1 && fwrite(m.vertices.data(), sizeof(Vertex), count, file) == count;
    }
    if (fclose(file) != 0)ok = false;
    if (ok)ok = MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok)DeleteFileW(temp.c_str()); return ok;
}
void ModelCache::Load() {
    if (!models.empty())return;
    wchar_t folder[32768] = {}; DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", folder, 32768);
    std::wstring path;
    if (length > 0 && length < 32768) { path = std::wstring(folder) + L"\\MoonlitVillage"; CreateDirectoryW(path.c_str(), nullptr); path += L"\\models-v3.bin"; }
    if (!path.empty() && Read(path))status = L"모델 캐시 불러옴";
    else { Build(); status = !path.empty() && Write(path) ? L"모델 캐시 저장 완료" : L"모델 캐시 저장 실패: 이번 실행은 메모리 모델 사용"; }
    for (auto& m : models) {
        glGenBuffers(1, &m.buffer); glBindBuffer(GL_ARRAY_BUFFER, m.buffer);
        glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(Vertex), m.vertices.data(), GL_STATIC_DRAW);
    }
}
void ModelCache::Draw(Renderer& r, int model, float x, float y, float scale, float flash) const {
    if (model < 0 || model >= (int)models.size())return;
    const auto& m = models[model]; r.DrawCachedModel(m.buffer, (int)m.vertices.size(), x, y, scale, flash);
}
