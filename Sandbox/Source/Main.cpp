#include "MainGame.h"

int main()
{
    Application Application;

    Application.GetWindow()->SetSize(1280, 720);
    Application.GetWindow()->SetTitle("Sandbox");
    Application.GetWindow()->SetBackgroundColor(0.80f, 0.85f, 0.95f);

    return Application.Start(new MainGame);
}
