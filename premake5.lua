-- // === WORKSPACE === // --
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

-- // === PROJECT GLF3D === // --
project "GLF3D"
    kind "SharedLib"
    location "Engine"

    defines "GLF_DLL"

    -- Output directories
    targetdir ("_Bin/" .. OutPutDir .. "%{prj.name}")
    objdir    ("_Obj/" .. OutPutDir .. "%{prj.name}")

    -- Files & Includes
    files {
        "Dependencies/Source/glad/glad.c",

        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
    }

    includedirs {
        "Dependencies/Include",
        "%{prj.location}",
    }    

    -- Filters
    filter "files:Dependencies/Source/glad/glad.c"
        flags "NoPCH"

    -- Windows
    filter "system:windows"
        defines "GLF_WIN"
        systemversion "latest"
        staticruntime "Off"
        runtime "Release"

        -- Precompiled Headers
        pchheader("Core.h")
        pchsource("%{prj.location}/Core.cpp")

        -- Libraries
        links {
            "Dependencies/Lib/glfw/lib-vc2022/glfw3_mt",
            "opengl32",
        }

        postbuildcommands
        {
            "{COPY} %{cfg.buildtarget.relpath} ../_Bin/" .. OutPutDir .. "/Sandbox"
        }

    -- Linux
    filter "system:linux"
        defines "GLF_TUX"
        systemversion "latest"

        -- Precompiled Headers
        pchheader("Core.h")

        -- Libraries
        links {
            "glfw",
            "GL",
        }

-- // === PROJECT SANDBOX === // --
project "Sandbox"
    kind "ConsoleApp"
    location "%{prj.name}"

    -- Output directories
    targetdir ("_Bin/" .. OutPutDir .. "%{prj.name}")
    objdir    ("_Obj/" .. OutPutDir .. "%{prj.name}")    

    -- Includes
    files {
        "Dependencies/Source/glad/glad.c",

        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp",
    }

    includedirs {
        "Dependencies/Include",
        "Engine/Source",
        "%{prj.location}",
    }

    -- Filters
    filter "files:Dependencies/Source/glad/glad.c"
        flags "NoPCH"

     -- Windows
     filter "system:windows"
        defines "GLF_WIN"
        systemversion "latest"
        staticruntime "Off"
        runtime "Release"

        -- Precompiled Headers
        pchheader("GLF3D.h")
        pchsource("%{prj.location}/GLF3D.cpp")

        -- Libraries
        links {
            "GLF3D",
        }
