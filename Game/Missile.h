#pragma once

#include <Owl.h>
#include "Spaceship.h"

class Missile : public owl::Entity
{
public:
    Missile(owl::Texture* Missile);
    ~Missile();

    void OnDraw();
    void OnUpdate();

private:
    owl::Sprite* m_Sprite;
    float m_Speed;
};

