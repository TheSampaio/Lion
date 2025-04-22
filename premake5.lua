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
    }

    dependencies["glad"] = {
        include = "%{wks.location}/Vendor/glad/include",
        lib = "glad",
    }

    dependencies["glfw"] = {
        include = "%{wks.location}/Vendor/glfw/include",
        lib = "glfw",
    }

    dependencies["glm"] = {
        include = "%{wks.location}/Vendor/glm/include",
    }

    dependencies["imgui"] = {
        include = "%{wks.location}/Vendor/imgui/include",
        lib = "imgui",
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
group ""

group "Core"
    include "Engine"
group ""

group "Misc"
    include "Game"
group ""
