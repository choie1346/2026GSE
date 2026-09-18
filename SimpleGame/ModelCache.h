#pragma once
#include "Renderer.h"
#include <vector>

// 0-7: player, 8-15: sentinel, 16: tree, 17: rock, 18: potion, 19: camp, 20: shard, 21: sword.
class ModelCache {
public:
    ~ModelCache();
    void Load();
    void Draw(Renderer& renderer,int model,float x,float y,float scale=1.f,float flash=0.f) const;
    const std::wstring& Status() const { return status; }
private:
    struct Mesh { std::vector<Renderer::ModelVertex> vertices; GLuint buffer=0; };
    std::vector<Mesh> models;
    std::wstring status;
    void Build();
    bool Read(const std::wstring& path);
    bool Write(const std::wstring& path) const;
};
