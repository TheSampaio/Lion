-- WORKSPACE
workspace "GLF3D"
    configurations {
        "Debug",
        "Release",
    }

    language "C++"
    cppdialect "C++14"
    architecture "x64"

    filter "configurations:Debug"
        defines "GLF_DEBUG"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines "GLF_RELEASE"
        symbols "Off"
        optimize "Speed"

    OutPutDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}/"

-- PROJECT GLF3D
project "GLF3D"
    kind "ConsoleApp"
    location "%{prj.name}"

    -- Output directories
    targetdir ("_Bin/" .. OutPutDir .. "%{prj.name}")
    objdir    ("_Obj/" .. OutPutDir .. "%{prj.name}")

    -- Files & Includes
    files {
        "Dependencies/glad/src/glad.c",

        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp",
    }

    includedirs {
        "Dependencies/glfw/include",
        "Dependencies/glad/include",
        "Dependencies/glm/glm",
    }

    -- Filters
    filter "files:Dependencies/glad/src/glad.c"
        flags "NoPCH"

    filter "system:windows" -- Windows
        defines "GLF_WIN"
        systemversion "latest"

        -- Precompiled Headers
        pchheader("PCH.h")
        pchsource("%{prj.name}/PCH.cpp")

        -- Libraries
        links {
            "Dependencies/glfw/lib-vc2022/glfw3",
            "opengl32",
        }

    filter "system:linux" -- Linux
        defines "GLF_TUX"
        systemversion "latest"

        -- Precompiled Headers
        pchheader("%{prj.name}/PCH.h")

        -- Libraries
        links {
            "glfw",
            "GL",
        }
