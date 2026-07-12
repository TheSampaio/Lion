project "Sealer"
    kind "ConsoleApp"
    targetname "lion-seal"

    -- Output directories
    targetdir ("%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
    }

    includedirs {
        "%{wks.location}/Engine/Include",
        "%{wks.location}/Engine/Source",
    }

    -- It includes the engine's umbrella header, which reaches every library the engine's own headers
    -- reach. The tool uses none of them; it just has to be able to see them.
    externalincludedirs {
        "%{dependencies.box2d.include}",
        "%{dependencies.glad.include}",
        "%{dependencies.glfw.include}",
        "%{dependencies.glm.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion",
    }

    -- The workspace hands every project /SUBSYSTEM:WINDOWS in Shipping, because what ships is a game. This
    -- is a build tool: it has a console, it prints to it, and it is never shipped at all.
    filter "configurations:Shipping"
        linkoptions { "/SUBSYSTEM:CONSOLE" }
        entrypoint "mainCRTStartup"

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

        -- It links the engine, so it needs the engine beside it to run.
        postbuildcommands {
            '{COPY} "%{wks.location}/Build/Bin/' .. output_dir .. 'Lion/lion-core.dll" "%{cfg.targetdir}"',
            '{COPYFILE} "%{dependencies.glfw.dll}" "%{cfg.targetdir}"',
        }
