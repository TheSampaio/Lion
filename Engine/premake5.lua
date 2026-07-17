project "Lion"
    kind "SharedLib"
    targetname "lion-core"  -- The engine's own binary; the project keeps its name, so links still resolve.

    defines { "LN_DLL", "GLFW_DLL" }  -- GLFW_DLL: consume GLFW as a shared library (see Vendor/glfw).

    -- Output directories
    targetdir ("%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}")

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
        -- The launcher's and the editor's output directories may not exist yet on a clean build, and a
        -- plain file copy into a missing directory fails rather than creating it.
        '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '"',
        '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. editor_project .. '"',
        "{COPY} %{cfg.buildtarget.relpath} ../Build/Bin/" .. output_dir .. "/" .. launcher_project,
        -- GLFW is a shared library, shipped as lion-platform.dll next to the launcher that loads the
        -- engine — with its licence beside it, as with every renamed or copied vendor file.
        '{COPYFILE} "%{dependencies.glfw.dll}" "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '/"',
        '{COPYFILE} "%{dependencies.glfw.license}" "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '/LICENSE-glfw.md"',
        -- The engine's own assets — its icon — go beside anything it builds: a game that picks no icon of
        -- its own still wears the engine's, and the editor always does.
        'xcopy /E /I /Y /Q "%{prj.location}Assets" "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '/"',
        'xcopy /E /I /Y /Q "%{prj.location}Assets" "%{wks.location}/Build/Bin/' .. output_dir .. editor_project .. '/"',
    }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"