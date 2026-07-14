project "Mane"
    kind "ConsoleApp"
    targetname "Lion"  -- The editor is the face of the engine, so it is what carries its name.

    -- Output directories
    targetdir ("%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}")

    -- Run from the output folder, not the project one: assets, the game module and the editor's own
    -- state all sit next to the executable, and Visual Studio would otherwise start it from Editor/.
    debugdir "%{cfg.targetdir}"

    files {
        "%{prj.location}/**.rc",   -- The executable's icon.
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
        "%{dependencies.iconfont.include}",
        "%{dependencies.imgui.include}",
        "%{dependencies.imguizmo.include}",
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion",
        "%{dependencies.imgui.lib}",
        "%{dependencies.imguizmo.lib}",
        "%{dependencies.glfw.lib}",  -- ImGui's GLFW backend links against GLFW directly (editor-only).
    }

    -- The game module is loaded at runtime, not linked; name it so it is built (and copied below)
    -- before the editor runs, letting the editor list the game's components.
    dependson { "Game" }

    filter "configurations:Shipping"
        kind "WindowedApp"

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

        postbuildcommands {
            -- Engine DLL next to the editor executable.
            '{COPY} "%{wks.location}/Build/Bin/' .. output_dir .. 'Lion/lion-core.dll" "%{cfg.targetdir}"',
            -- GLFW shared library (the editor's ImGui backend links against it directly).
            '{COPYFILE} "%{dependencies.glfw.dll}" "%{cfg.targetdir}"',
            -- The icon font, beside the executable, with the licence it ships under. It is the editor's
            -- alone: ImGui never crosses into the engine, and a game draws no icons.
            '{MKDIR} "%{cfg.targetdir}/Fonts"',
            '{COPYFILE} "%{dependencies.mdi.font}" "%{cfg.targetdir}/Fonts/"',
            '{COPYFILE} "%{dependencies.mdi.license}" "%{cfg.targetdir}/Fonts/LICENSE-mdi.txt"',
            -- Shared resources (shaders + sprites) flattened next to the executable, minus the scripts:
            -- the editor reads those from the project, and they are compiled into the module anyway.
            -- The exclude list is named relatively — see the same copy in Sandbox/premake5.lua.
            'xcopy /E /I /Y /Q /EXCLUDE:..\\Scripts\\AssetCopyExclude.txt "%{wks.location}/Sandbox/Assets" "%{cfg.targetdir}"',
        }
