workspace "Lion"
    configurations { "Debug", "Release", "Shipping" }

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

    output_dir = "%{cfg.buildcfg}/"

    dependencies = {}

    dependencies["box2d"] = {
        include = "%{wks.location}/Vendor/box2d/include",
    }

    dependencies["glad"] = {
        include = "%{wks.location}/Vendor/glad/include",
        lib = "glad",
        source = "Vendor/glad/source/glad.c",
    }

    dependencies["glfw"] = {
        include = "%{wks.location}/Vendor/glfw/include",
        lib = "glfw",
    }

    dependencies["glm"] = {
        include = "%{wks.location}/Vendor/glm/include",
    }

    dependencies["spdlog"] = {
        include = "%{wks.location}/Vendor/spdlog/include",
    }

    dependencies["stb"] = {
        include = "%{wks.location}/Vendor/stb/include",
        lib = "stb",
    }

-- ========== Lion ========== --
project "Lion"
    location "Engine"
    kind "SharedLib"

    defines "LN_DLL"

    -- Output directories
    targetdir (".Output/Bin/" .. output_dir .. "%{prj.name}")
    objdir    (".Output/Obj/" .. output_dir .. "%{prj.name}")

    pchheader("Engine.h")
    pchsource("%{prj.location}/Engine.cpp")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.hint",

        "%{dependencies.glad.source}",
    }

    includedirs {
        "%{prj.location}",
    }

    externalincludedirs {
        "%{dependencies.box2d.include}",
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.glm.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "%{dependencies.glad.lib}",
        "%{dependencies.glfw.lib}",
        "%{dependencies.stb.lib}",
    }

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../.Output/Bin/" .. output_dir .. "/Sandbox"
    }

    filter { "files:vendor/glad/source/glad.c" }
        flags { "NoPCH" }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

-- ========== Sandbox ========== --
project "Sandbox"
    location "Game"
    kind "ConsoleApp"

    -- Output directories
    targetdir (".Output/Bin/" .. output_dir .. "%{prj.name}")
    objdir    (".Output/Obj/" .. output_dir .. "%{prj.name}")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",

        "%{dependencies.glad.source}",
    }

    includedirs {
        "Engine/Include",
    }

    externalincludedirs {
        "%{dependencies.box2d.include}",
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.glm.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion"
    }

    filter "configurations:Shipping"
        kind "WindowedApp"

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

-- ========== Dependencies ========== --
group "External Dependencies"
    include "Vendor/box2d"
    include "Vendor/glad"
    include "Vendor/glfw"
    include "Vendor/glm"
    include "Vendor/spdlog"
    include "Vendor/stb"
group ""
