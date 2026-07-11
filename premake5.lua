workspace "Lion"
    configurations { "Debug", "Release", "Shipping" }
    startproject "Editor"  -- The project in Mane/; F5 in Visual Studio opens the editor.

    language "C++"
    cppdialect "C++20"
    architecture "x64"

    filter "configurations:Debug"
        defines "LN_DEBUG"
        symbols "On"
        optimize "Off"
        runtime "Debug"

    filter "configurations:Release"
        defines "LN_RELEASE"
        symbols "Off"
        optimize "Speed"
        runtime "Release"

    filter "configurations:Shipping"
        defines "LN_SHIPPING"
        symbols "Off"
        optimize "Speed"
        runtime "Release"
        linkoptions { "/SUBSYSTEM:WINDOWS" }

    filter "action:vs*"
        defines { "LN_DISABLE_WARNINGS=6294 26495 26498 26800" }

    output_dir = "%{cfg.buildcfg}/"

    -- Everything the build produces lands under one Build/ tree, including the vendored libraries
    -- (see the override below). Deleting it is enough for a clean build.
    binary_dir = "%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}"
    object_dir = "%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}"

    dependencies = {}

    dependencies["box2d"] = {
        include = "%{wks.location}/Vendor/box2d/include",
        lib = "box2d",
    }

    dependencies["glad"] = {
        include = "%{wks.location}/Vendor/glad/include",
        lib = "glad",
    }

    dependencies["glfw"] = {
        include = "%{wks.location}/Vendor/glfw/include",
        lib = "glfw",
        dll = "%{wks.location}/Build/Bin/" .. output_dir .. "glfw/glfw.dll",
    }

    dependencies["glm"] = {
        include = "%{wks.location}/Vendor/glm/include",
    }

    dependencies["imgui"] = {
        include = "%{wks.location}/Vendor/imgui/include",
        lib = "imgui",
    }

    dependencies["json"] = {
        include = "%{wks.location}/Vendor/json/include",
    }

    dependencies["imguizmo"] = {
        -- Header only: the editor compiles the vendored ImGuizmo.cpp from its own Source/Vendor tree.
        include = "%{wks.location}/Vendor/imguizmo/include",
    }

    dependencies["spdlog"] = {
        include = "%{wks.location}/Vendor/spdlog/include",
    }

    dependencies["stb"] = {
        include = "%{wks.location}/Vendor/stb/include",
        lib = "stb",
    }

group ". External Dependencies"
    include "Vendor/box2d"
    include "Vendor/glad"
    include "Vendor/glfw"
    include "Vendor/glm"
    include "Vendor/imgui"
    include "Vendor/spdlog"
    include "Vendor/stb"

    -- The vendored libraries are submodules, and their own scripts write into a .Out folder inside
    -- each one. Redirect them here instead of editing the submodules, so every artefact this build
    -- produces lives under one Build/ tree and the submodules stay pristine.
    for _, vendor in ipairs { "box2d", "glad", "glfw", "glm", "imgui", "spdlog", "stb" } do
        project(vendor)
            filter {}
            targetdir (binary_dir)
            objdir    (object_dir)
    end

    -- Override GLFW (a submodule) to build as a shared library without editing the vendored file.
    -- A single shared GLFW keeps one copy of its global state, shared by the engine DLL (which owns
    -- the window) and the editor executable (whose ImGui GLFW backend must act on that same window).
    project "glfw"
        filter {}
        kind "SharedLib"
        defines { "_GLFW_BUILD_DLL" }
group ""

group "Core"
    include "Engine"
group ""

-- A folder never carries its project's name, so a path always says which of the two it means:
-- Mane/ holds the Editor, Sandbox/ holds the Game.
group "Tools"
    include "Mane"
    include "Launcher"
group ""

-- The game is its own thing: the engine and the tools are built against it, never the other way round.
group "Game"
    include "Sandbox"
group ""
