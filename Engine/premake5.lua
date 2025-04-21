project "Lion"
    kind "SharedLib"

    defines "LN_DLL"

    -- Output directories
    targetdir ("%{wks.location}/.Out/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/.Out/Obj/" .. output_dir .. "%{prj.name}")

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
        "%{dependencies.box2d.include}",
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.glm.include}",
        "%{dependencies.imgui.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "%{dependencies.glad.lib}",
        "%{dependencies.glfw.lib}",
        "%{dependencies.imgui.lib}",
        "%{dependencies.stb.lib}",
    }

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../.Out/Bin/" .. output_dir .. "/Sandbox"
    }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"