project "Editor"
    kind "ConsoleApp"

    -- Output directories
    targetdir ("%{wks.location}/.Out/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/.Out/Obj/" .. output_dir .. "%{prj.name}")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
    }

    includedirs {
        "%{wks.location}/Engine/Include",
        "%{wks.location}/Engine/Source",
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
        "Lion",
        "%{dependencies.imgui.lib}",
    }

    filter "configurations:Shipping"
        kind "WindowedApp"

    -- The editor runs from its output folder, next to the engine DLL and its resources.
    debugdir "%{cfg.targetdir}"

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

        postbuildcommands {
            -- Engine DLL next to the editor executable.
            '{COPY} "%{wks.location}/.Out/Bin/' .. output_dir .. 'Lion/Lion.dll" "%{cfg.targetdir}"',
            -- Shared resources (shader + sprites) flattened next to the executable.
            'xcopy /E /I /Y /Q "%{wks.location}/Game/Resource" "%{cfg.targetdir}"',
        }
