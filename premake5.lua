workspace "Lion"
    configurations { "Debug", "Release", "Shipping" }

    language "C++"
    cppdialect "C++20"
    architecture "x64"

    filter "configurations:Debug"
        defines "LN_DEBUG"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines "LN_RELEASE"
        symbols "Off"
        optimize "Speed"

    filter "configurations:Shipping"
        defines "LN_SHIPPING"
        symbols "Off"
        optimize "Speed"
        linkoptions { "/SUBSYSTEM:WINDOWS" }

    output_dir = "%{cfg.buildcfg}/"

    dependencies = {}

    dependencies["glad"] = {
        include = "%{wks.location}/Vendor/glad/include",
        lib = "glad",
    }

    dependencies["glfw"] = {
        include = "%{wks.location}/Vendor/glfw/include",
        lib = "glfw",
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
    }

    includedirs {
        "%{prj.location}",
    }

    externalincludedirs {
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "%{dependencies.glad.lib}",
        "%{dependencies.glfw.lib}",
        "%{dependencies.stb.lib}",
    }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../.Output/Bin/" .. output_dir .. "/Sandbox"
    }

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
    }

    includedirs {
        "Engine/Include",
    }

    externalincludedirs {
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
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

group "External Dependencies"
    include "Vendor/glad"
    include "Vendor/glfw"
    include "Vendor/spdlog"
    include "Vendor/stb"
group ""
