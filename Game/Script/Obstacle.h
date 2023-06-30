#pragma once

#include <Owl.h>

class Obstacle : public owl::Entity
{
public:
    Obstacle(owl::Texture* Texture, float Speed, float Z = owl::Layer::Lower);
    ~Obstacle();

    void OnUpdate();
    void OnDraw() { m_Sprite->Draw(X, Y, Z); }

private:
    owl::Sprite* m_Sprite;
    float m_Speed;
};

