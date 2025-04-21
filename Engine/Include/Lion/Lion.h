#pragma once

// ----------------------------------- //
// |     DON'T CHANGE THE ORDER!     | //
// ----------------------------------- //
// | 1 |  Platform                   | //
// | 2 |  Standard                   | //
// | 3 |  External                   | //
// | 4 |  Allocator                  | //
// | 5 |  Primitive                  | //
// ----------------------------------- //
#include <Lion/Base/Platform.h>
#include <Lion/Base/Standard.h>
#include <Lion/Base/External.h>
#include <Lion/Type/Allocator.h>
#include <Lion/Type/Primitive.h>

// Core
#include <Lion/Core/Application.h>
#include <Lion/Core/Asset.h>
#include <Lion/Core/Clock.h>
#include <Lion/Core/Input.h>
#include <Lion/Core/Layer.h>
#include <Lion/Core/Log.h>
#include <Lion/Core/Window.h>

// Logic
#include <Lion/Logic/Actor.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Timer.h>

// Math
#include <Lion/Math/Sigma.h>
#include <Lion/Math/Transform.h>
#include <Lion/Math/Vector.h>

// Render
#include <Lion/Render/CameraOrthographic.h>
#include <Lion/Render/Graphics.h>
#include <Lion/Render/Renderer.h>
#include <Lion/Render/Sprite.h>
#include <Lion/Render/Texture.h>

// Signal
#include <Lion/Signal/Event.h>
#include <Lion/Signal/EventDispatcher.h>
#include <Lion/Signal/EventInput.h>
#include <Lion/Signal/EventWindow.h>

// Type
#include <Lion/Type/Depth.h>
#include <Lion/Type/Macro.h>
