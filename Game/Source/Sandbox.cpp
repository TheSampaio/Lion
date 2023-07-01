#include "Sandbox.h"
#include "Resources.h"

#include "Frogger.h"
#include "Obstacle.h"

using namespace owl;

Sandbox::Sandbox()
{
	{ // Game's initial's setup

		// Set-up the window
		Window::SetIcon(IDI_ICON);
		Window::SetSize(800, 600);
		Window::SetTitle("Frogger Demo");
		Window::SetDisplayMode(Windowed);

		// Set-up the window's cursor
		Cursor::SetCursor(IDC_CURSOR);
	}

	// Initializes all pointers
	m_Truck = nullptr;
	m_Car01 = nullptr;
	m_Car02 = nullptr;
	m_Car03 = nullptr;
	m_Car04 = nullptr;
	m_Turtle01 = nullptr;
	m_Turtle02 = nullptr;
	m_Wood01 = nullptr;
	m_Wood02 = nullptr;

	m_Background = nullptr;
	m_Player = nullptr;
	m_Obstacle = nullptr;
}

void Sandbox::OnStart()
{
	Debug::Console(Information, "The game was initialized.");

	// Loads all textures
	m_Truck = new Texture("Data/Frogger/truck.png");
	m_Car01 = new Texture("Data/Frogger/car-02.png");
	m_Car02 = new Texture("Data/Frogger/car-03.png");
	m_Car03 = new Texture("Data/Frogger/car-04.png");
	m_Car04 = new Texture("Data/Frogger/car-04.png");
	m_Turtle01 = new Texture("Data/Frogger/turtles-small.png");
	m_Turtle02 = new Texture("Data/Frogger/turtles-big.png");
	m_Wood01 = new Texture("Data/Frogger/wood-small.png");
	m_Wood02 = new Texture("Data/Frogger/wood-big.png");

	// Loads background and player
	m_Background = new Sprite("Data/Frogger/background.jpg");
	m_Player = new Frogger();

	// Add an entity to the "scene"
	m_Scene.push_back(m_Player);

	// Water's obstacles
	m_Obstacle = new Obstacle(m_Wood01, 80);
	m_Obstacle->SetPosition(150, 109, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Wood01, 80);
	m_Obstacle->SetPosition(550, 109, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Turtle01, 70);
	m_Obstacle->SetPosition(480, 142, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Turtle01, 70);
	m_Obstacle->SetPosition(680, 142, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Wood02, 50);
	m_Obstacle->SetPosition(200, 190, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Wood02, 50);
	m_Obstacle->SetPosition(700, 190, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Wood01, 60);
	m_Obstacle->SetPosition(350, 229, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Turtle02, 40);
	m_Obstacle->SetPosition(150, 262, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Turtle02, 40);
	m_Obstacle->SetPosition(450, 262, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Turtle02, 40);
	m_Obstacle->SetPosition(750, 262, Layer::Lower);
	m_Scene.push_back(m_Obstacle);

	// Road's obstacles
	m_Obstacle = new Obstacle(m_Truck, 50);
	m_Obstacle->SetPosition(300, 344, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Truck, 50);
	m_Obstacle->SetPosition(700, 344, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Car01, 90);
	m_Obstacle->SetPosition(350, 384, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Car04, 90);
	m_Obstacle->SetPosition(600, 387, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Car02, 110);
	m_Obstacle->SetPosition(500, 427, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Car03, 100);
	m_Obstacle->SetPosition(750, 467, Layer::Upper);
	m_Scene.push_back(m_Obstacle);

	m_Obstacle = new Obstacle(m_Car04, 80);
	m_Obstacle->SetPosition(450, 507, Layer::Upper);
	m_Scene.push_back(m_Obstacle);
}

void Sandbox::OnUpdate()
{
	// Updates entities
	for (auto Entity : m_Scene)
		Entity->OnUpdate();
}

void Sandbox::OnDraw()
{
	// Draws the background
	m_Background->Draw(0.0f, 0.0f, Layer::Back);

	// Draws entities
	for (auto Entity : m_Scene)
		Entity->OnDraw();
}

void Sandbox::OnFinish()
{
	// Deletes entities
	for (auto Entity : m_Scene)
		delete Entity;

	// Deletes backround
	delete m_Background;

	// Deletes all textures
	delete m_Truck;
	delete m_Car01;
	delete m_Car02;
	delete m_Car03;
	delete m_Car04;
	delete m_Turtle01;
	delete m_Turtle02;
	delete m_Wood01;
	delete m_Wood02;

	Debug::Console(Information, "The game was finalized.");
}
