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
        -- The library is GLFW; the file is named for what it is *here* — the platform layer's shared
        -- library — so what ships beside the executables reads as the engine's, not as a grab-bag of
        -- third-party names. Renaming the binary is fine under GLFW's zlib licence (it asks that altered
        -- versions not misrepresent their origin, not that the file keep its name); the licence ships
        -- beside the DLL, and the README's dependency table says what it really is.
        dll = "%{wks.location}/Vendor/glfw/Build/Bin/" .. output_dir .. "glfw/lion-platform.dll",
        license = "%{wks.location}/Vendor/glfw/LICENSE.md",
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
        -- Upstream keeps its sources in src/, so that is what a translation unit including "ImGuizmo.h" is
        -- given. A fork that rearranged its own folders would have to rearrange them again at every merge.
        include = "%{wks.location}/Vendor/imguizmo/src",
        lib = "imguizmo",
    }

    -- The icons the editor draws with: a webfont, and the header that names its glyphs. The font is data,
    -- like GLFW's DLL — nothing links it, so what a dependency on it means is a file that has to arrive
    -- beside the executable. Its licence travels with it, which is what the Apache 2.0 licence asks for.
    dependencies["mdi"] = {
        font = "%{wks.location}/Vendor/mdi/fonts/materialdesignicons-webfont.ttf",
        license = "%{wks.location}/Vendor/mdi/LICENSE",
    }

    dependencies["iconfont"] = {
        include = "%{wks.location}/Vendor/iconfont",
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
    include "Vendor/iconfont"
    include "Vendor/imgui"
    include "Vendor/imguizmo"
    include "Vendor/json"
    include "Vendor/mdi"
    include "Vendor/spdlog"
    include "Vendor/stb"

    -- Each library builds inside its own folder, so its artefacts never mix with the engine's. Set
    -- here rather than in the vendored scripts, which are submodules: they keep the name this project
    -- uses for an output folder without being edited.
    for _, vendor in ipairs { "box2d", "glad", "glfw", "glm", "iconfont", "imgui", "imguizmo", "json", "mdi", "spdlog", "stb" } do
        project(vendor)
            filter {}
            targetdir ("%{wks.location}/Vendor/" .. vendor .. "/Build/Bin/" .. output_dir .. "%{prj.name}")
            objdir    ("%{wks.location}/Vendor/" .. vendor .. "/Build/Obj/" .. output_dir .. "%{prj.name}")

            -- A vendored library is optimised even in Debug. Debug is where *this* code is stepped through;
            -- nobody steps into ImGui to find a bug in the editor, and an unoptimised ImGui builds the whole
            -- UI's geometry every frame at debug speed — which is most of what the editor was paying for.
            -- The runtime is left alone: it is the one thing that has to match across every module.
            -- Optimising means giving up the runtime checks that come with an unoptimised Debug build:
            -- /O2 and /RTC1 are mutually exclusive, and the checks are there to catch bugs in the code you
            -- are writing, which this is not.
            filter "configurations:Debug"
                optimize "Speed"
                runtimechecks "Off"

            filter {}
    end

    -- Override GLFW (a submodule) to build as a shared library without editing the vendored file.
    -- A single shared GLFW keeps one copy of its global state, shared by the engine DLL (which owns
    -- the window) and the editor executable (whose ImGui GLFW backend must act on that same window).
    --
    -- The target is named lion-platform: it is the platform layer's shared library, and the name beside
    -- the executables should say so (see dependencies["glfw"].dll above for the licence note).
    project "glfw"
        filter {}
        kind "SharedLib"
        targetname "lion-platform"
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
group ""

-- The gameplay layer: what the tools load and run, as opposed to the engine underneath them.
group "Runtime"
    include "Sandbox"
group ""
