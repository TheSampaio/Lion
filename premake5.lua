workspace "Lion"
    configurations { "Debug", "Release", "Shipping" }
    startproject "Sandbox"

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
        dll = "%{wks.location}/Vendor/glfw/.Out/Bin/" .. output_dir .. "glfw/glfw.dll",
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

group "Tools"
    include "Editor"
group ""

group "Misc"
    include "Game"
group ""
