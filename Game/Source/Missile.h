#pragma once

#include <Lion.h>
#include "Spaceship.h"

class Missile : public Lion::Entity
{
public:
    Missile(Lion::Texture* Missile);
    ~Missile();

    void OnDraw();
    void OnUpdate();

private:
    Lion::Sprite* m_Sprite;
    float m_Speed;
};

