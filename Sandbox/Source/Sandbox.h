#ifndef SANDBOX_GAME_H
#define SANDBOX_GAME_H

// Main game's class
class Sandbox : public Application
{
public:
    Sandbox();

    // Main methods
    void Start();
    void Update(float DeltaTime);
    void Draw();
    void Finalize();

private:
    // Attributes
    Mesh* Quads;

	// Static attributes
	static bool s_bWireframe;
};

#endif

// === Vertices (Temporary) =====
static const std::vector<Vertex> Vertices
{
	// Red quad
	Vertex{ -Vector::Right,               Colour::Red  },                                         // 0
	Vertex{  Vector::Zero,                Colour::Red  },                                         // 1
	Vertex{ -Vector::Right + Vector::Up,  Colour::Red + Colour::Green * 0.8f  },                  // 2
	Vertex{  Vector::Up,                  Colour::Red + Colour::Green * 0.8f  },                  // 3

	// Green quad
	Vertex{  Vector::Zero,                Colour::Green  },                                       // 4
	Vertex{  Vector::Right,               Colour::Green  },                                       // 5
	Vertex{  Vector::Up,                  Colour::Green + (Colour::Red + Colour::Blue) * 0.6f  }, // 6
	Vertex{  Vector::Right + Vector::Up,  Colour::Green + (Colour::Red + Colour::Blue) * 0.6f  }, // 7

	// Blue quad
	Vertex{ -Vector::Right + -Vector::Up, Colour::Blue  },                                        // 8
	Vertex{ -Vector::Up,                  Colour::Blue  },                                        // 9
	Vertex{ -Vector::Right,               Colour::Blue + Colour::Green * 0.8f  },                 // 10
	Vertex{  Vector::Zero,                Colour::Blue + Colour::Green * 0.8f  },                 // 11

	// Yellow quad
	Vertex{ -Vector::Up,                  Colour::Red + Colour::Green  },                         // 12
	Vertex{  Vector::Right + -Vector::Up, Colour::Red + Colour::Green  },                         // 13
	Vertex{  Vector::Zero,                Colour::Red + Colour::Green + Colour::Blue * 0.6f  },   // 14
	Vertex{  Vector::Right,               Colour::Red + Colour::Green + Colour::Blue * 0.6f  },   // 15
};

// === Indices (Temporary) =====
static const std::vector<Index> Indices
{
	// Red quad
	Index{ 0, 1, 2 },
	Index{ 2, 1, 3 },

	// Green quad
	Index{ 4, 5, 6 },
	Index{ 6, 5, 7 },

	// Blue quad
	Index{  8, 9, 10 },
	Index{ 10, 9, 11 },

	// Yellow quad
	Index{ 12, 13, 14 },
	Index{ 14, 13, 15 },
};
