workspace "Owl"
    configurations { "Debug", "Release" }

    language "C++"
    cppdialect "C++20"
    architecture "x64"

    filter "configurations:Debug"
        defines "WL_DEBUG"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines "WL_RELEASE"
        symbols "Off"
        optimize "Speed"

    OutputDir = "%{cfg.buildcfg}/"

-- ========== Owl ========== --
project "Owl"
    location "Engine"
    kind "SharedLib"

    defines "WL_DLL"

    -- Output directories
    targetdir ("_Output/Bin/" .. OutputDir .. "%{prj.name}")
    objdir    ("_Output/Obj/" .. OutputDir .. "%{prj.name}")

    pchheader("Engine.h")
    pchsource("%{prj.location}/Engine.cpp")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.hlsl",
    }

    includedirs {
        "%{prj.location}",
    }

    links {
        "dxgi",
        "d3d11",
        "winmm",
        "dxguid.lib",
        "d3dcompiler",
    }

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../_Output/Bin/" .. OutputDir .. "/Sandbox"
    }

    filter "system:windows"
        defines "WL_PLATFORM_WIN"
        systemversion "latest"

    filter "files:Engine/Shader/DefaultVertex.hlsl"
        shadermodel "5.0"
        shadertype "Vertex"
        shaderobjectfileoutput "%{wks.location}/Game/Bin/%%(Filename).cso"

    filter "files:Engine/Shader/DefaultPixel.hlsl"
        shadermodel "5.0"
        shadertype "Pixel"
        shaderobjectfileoutput "%{wks.location}/Game/Bin/%%(Filename).cso"

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
        "%{prj.location}/**.rc",
    }

    includedirs {
        "Engine/Include",
    }

    links {
        "Owl",
    }

    filter "system:windows"
        defines "WL_PLATFORM_WIN"
        systemversion "latest"
