project "Lion"
    kind "SharedLib"

    defines { "LN_DLL", "GLFW_DLL" }  -- GLFW_DLL: consume GLFW as a shared library (see Vendor/glfw).

    -- Output directories
    targetdir ("%{wks.location}/.Out/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/.Out/Obj/" .. output_dir .. "%{prj.name}")

    pchheader("Engine.h")
    pchsource("Engine.cpp")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{prj.location}/**.hint",
    }

    includedirs {
        "%{wks.location}/Engine",
        "%{wks.location}/Engine/Source",
    }

    externalincludedirs {
        "%{dependencies.box2d.include}",
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.glm.include}",
        "%{dependencies.json.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "%{dependencies.box2d.lib}",
        "%{dependencies.glad.lib}",
        "%{dependencies.glfw.lib}",
        "%{dependencies.stb.lib}",
    }

    postbuildcommands {
        "{COPY} %{cfg.buildtarget.relpath} ../.Out/Bin/" .. output_dir .. "/Sandbox",
        -- GLFW is a shared library; ship it next to the game executable that loads the engine.
        '{COPYFILE} "%{dependencies.glfw.dll}" "%{wks.location}/.Out/Bin/' .. output_dir .. 'Sandbox/"',
    }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"