require("vstudio")

-- Visual Studio keeps "Show All Files" in a project's .user file, which premake already writes (it is
-- where the debug working directory goes). Adding the flag there means a fresh clone opens with the
-- whole folder in the tree — premake5.lua, the assets — rather than only the files the glob picked up.
premake.override(premake.vstudio.vc2010.elements, "user", function(base, cfg)
    local items = base(cfg)
    table.insert(items, function(cfg)
        premake.w('<ShowAllFiles>true</ShowAllFiles>')
    end)
    return items
end)

workspace "Lion"
    configurations { "Debug", "Release", "Shipping" }
    startproject "Mane"  -- The project in Editor/; F5 in Visual Studio opens the editor.

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

    -- Every project builds into a folder named after itself, so a project that ships something into
    -- another one's folder needs that project's name — not a second spelling of it. Renaming a tool is
    -- then a change here, and nowhere else.
    editor_project = "Mane"
    launcher_project = "Launcher"

    -- This project's own artefacts. Each vendored library keeps a Build/ of its own, inside its
    -- folder (see the override below), so a library's output never mixes with the engine's.
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
        dll = "%{wks.location}/Vendor/glfw/Build/Bin/" .. output_dir .. "glfw/glfw.dll",
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

    -- Each library builds inside its own folder, so its artefacts never mix with the engine's. Set
    -- here rather than in the vendored scripts, which are submodules: they keep the name this project
    -- uses for an output folder without being edited.
    for _, vendor in ipairs { "box2d", "glad", "glfw", "glm", "imgui", "spdlog", "stb" } do
        project(vendor)
            filter {}
            targetdir ("%{wks.location}/Vendor/" .. vendor .. "/Build/Bin/" .. output_dir .. "%{prj.name}")
            objdir    ("%{wks.location}/Vendor/" .. vendor .. "/Build/Obj/" .. output_dir .. "%{prj.name}")
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
-- Editor/ holds Mane, Sandbox/ holds the Game.
group "Tools"
    include "Editor"
    include "Launcher"
    include "Packer"
group ""

-- The gameplay layer: what the tools load and run, as opposed to the engine underneath them.
group "Runtime"
    include "Sandbox"
group ""
