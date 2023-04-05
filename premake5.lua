-- // === WORKSPACE === // --
workspace "Owl"
    configurations {
        "Debug",
        "Release",
    }    

    language "C++"
    cppdialect "C++20"
    architecture "x64"

    filter "configurations:Debug"
        defines "OWL_DEBUG"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines "_RELEASE"
        symbols "Off"
        optimize "Speed"

    OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}/"

-- // ==================== PROJECT OWL ==================== // --
project "Owl"
    kind "SharedLib"
    location "Engine"

    defines "_DLL"

    -- Output directories
    targetdir ("_Output/Bin/" .. OutputDir .. "%{prj.name}")
    objdir    ("_Output/Obj/" .. OutputDir .. "%{prj.name}")

    -- Files & Includes
    files {
        "_Dependencies/Source/glad.c",

        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.glsl",
    }

    includedirs {
        "_Dependencies/Include",
        "%{prj.location}",
    }    

    -- Filters
    filter "files:_Dependencies/Source/glad.c"
        flags "NoPCH"

    -- Windows
    filter "system:windows"
        defines "_WIN"
        systemversion "latest"
        staticruntime "Off"
        runtime "Release"

        -- Precompiled Headers
        pchheader("Core.h")
        pchsource("%{prj.location}/Core.cpp")

        -- Libraries
        links {
            "_Dependencies/Lib/Windows/glfw3",
            "opengl32",
        }

        postbuildcommands
        {
            "{COPY} %{cfg.buildtarget.relpath} ../_Output/Bin/" .. OutputDir .. "/Sandbox"
        }

    -- Linux
    filter "system:linux"
        defines "_TUX"
        systemversion "latest"

        staticruntime "Off"
        runtime "Release"

        -- Precompiled Headers
        pchheader("Core.h")

        -- Libraries
        links {
            "glfw",
            "GL",
        }

-- // ==================== PROJECT SANDBOX ==================== // --
project "Sandbox"
    kind "ConsoleApp"
    location "%{prj.name}"

    -- Output directories
    targetdir ("_Output/Bin/" .. OutputDir .. "%{prj.name}")
    objdir    ("_Output/Obj/" .. OutputDir .. "%{prj.name}")    

    -- Includes
    files {
        "_Dependencies/Source/glad.c",

        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp",
    }

    includedirs {
        "_Dependencies/Include",
        "Engine/Include",
        "%{prj.location}",
    }

    -- Filters
    filter "files:_Dependencies/Source/glad.c"
        flags "NoPCH"

     -- Windows
    filter "system:windows"
        defines "_WIN"
        systemversion "latest"

        staticruntime "On"
        runtime "Release"

        -- Precompiled Headers
        pchheader("PCH.h")
        pchsource("%{prj.location}/PCH.cpp")

        -- Libraries
        links {
            "Owl",
        }

    filter "system:linux"
        defines "_TUX"
        systemversion "latest"
        staticruntime "Off"
        runtime "Release"

        pchheader("Owl.h")
        pchsource("%{prj.location}/Owl.cpp")

        links {
            "Owl"
        }
