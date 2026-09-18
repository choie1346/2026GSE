#pragma once
class Renderer;
namespace FirstLevel {
    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Draw(Renderer& renderer,int width,int height);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void MouseButton(bool pressed,int x,int y,int width,int height);
    void ClearInput();
}
