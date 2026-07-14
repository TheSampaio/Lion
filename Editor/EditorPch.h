#pragma once

// What the editor includes and never changes.
//
// A precompiled header is only worth having if what is in it holds still: the standard library, the engine's
// public headers, and the GUI the editor is written against. Put a header of the editor's own in here and
// every edit to it recompiles every translation unit that exists — which is the opposite of the point.
//
// It carries no entry point. The engine writes one in Lion/Launcher.h, which is a header that defines a
// function, so exactly one translation unit may include it (Editor.cpp does).

// The standard library, which is most of the cost.
#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// The engine, through its own umbrella.
#include <Lion/Lion.h>

// The GUI the editor draws with, and the icons it draws.
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <ImGuizmo.h>
#include <IconsMaterialDesignIcons.h>
