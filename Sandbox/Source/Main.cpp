#include "GLF3D.h"
#include "Sandbox.h"

int main()
{
    Application Engine;

    Engine.GetWindow()->SetSize(1280, 720);
    Engine.GetWindow()->SetTitle("Sandbox");
    Engine.GetWindow()->SetBackgroundColor(0.25f, 0.25f, 0.25f);

    return Engine.Start(new Sandbox);
}
