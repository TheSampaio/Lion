workspace "GLF3D"
    configurations {
        "Debug",
        "Release",
    }

    language "C++"
    cppdialect "C++14"

    filter "configurations:Debug"
        defines "GLF_DEBUG"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines "GLF_RELEASE"
        symbols "Off"
        optimize "Speed"

    OutPutDir = "%{cfg.system}/%{cfg.buildcfg}/%{cfg.architecture}/"

project "GLF3D"
    kind "ConsoleApp"
    location "%{prj.name}"

    pchheader("%{prj.name}/PCH.h")
    pchsource("${prj.name}/PCH.cpp")

    files {
        "%{prj.name}/**.h",
        "%{prj.name}/**.cpp",
    }

    targetdir ("_Bin/" .. OutPutDir .. "%{prj.name}")
    objdir    ("_Obj/" .. OutPutDir .. "%{prj.name}")

    filter "system:windows"
        defines "GLF_WIN32"
        systemversion "latest"

    filter "system:linux"
        defines "GLF_TUX"
        systemversion "latest"

        links {
            "glfw",
            "GLEW",
            "GL",
        }
