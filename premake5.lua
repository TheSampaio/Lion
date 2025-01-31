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

    OutputDir = "%{cfg.buildcfg}/"

-- ========== Lion ========== --
project "Lion"
    location "Engine"
    kind "SharedLib"

    defines "LN_DLL"

    -- Output directories
    targetdir ("_Output/Bin/" .. OutputDir .. "%{prj.name}")
    objdir    ("_Output/Obj/" .. OutputDir .. "%{prj.name}")

    pchheader("Engine.h")
    pchsource("%{prj.location}/Engine.cpp")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.hint",
    }

    includedirs {
        "%{prj.location}",

        "Vendor/glfw/include",
        "Vendor/spdlog/include",
    }

    libdirs {
        "Vendor/glfw/src/Release",
    }

    links {
        "glfw3"
    }

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../_Output/Bin/" .. OutputDir .. "/Sandbox"
    }

    filter "system:windows"
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

-- ========== Sandbox ========== --
project "Sandbox"
    location "Game"
    kind "ConsoleApp"

    -- Output directories
    targetdir ("_Output/Bin/" .. OutputDir .. "%{prj.name}")
    objdir    ("_Output/Obj/" .. OutputDir .. "%{prj.name}")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
    }

    includedirs {
        "Engine/Include",
        
        "Vendor/glfw/include",
        "Vendor/spdlog/include",
    }

    libdirs {
    }

    links {
        "Lion"
    }

    filter "system:windows"
        defines "LN_PLATFORM_WIN"
        systemversion "latest"
