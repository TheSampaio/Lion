project "Game"
    -- The game is a shared library, loaded at runtime by the standalone launcher and by the editor.
    -- Its file name is fixed (see Lion/Core/GameModule.h), so those loaders do not care what the
    -- project is called.
    kind "SharedLib"
    targetname "lion-game"

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

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

        -- The module ships itself to both of its consumers: the standalone launcher and the editor,
        -- which each load it from their own directory. Doing it here (rather than in their postbuilds)
        -- means rebuilding just the game refreshes both — which is what the editor's hot reload needs.
        -- The symbols travel with the module. They are what lets a debugger step into game code loaded
        -- by either tool, and the editor needs them beside the library to give its own copy a copy of
        -- them (see Mane/Source/ModuleSymbols.h). An optimised build produces none, hence "if exist".
        postbuildcommands {
            '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. 'Launcher"',
            '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. 'Editor"',
            '{COPYFILE} "%{cfg.buildtarget.relpath}" "%{wks.location}/Build/Bin/' .. output_dir .. 'Launcher/"',
            '{COPYFILE} "%{cfg.buildtarget.relpath}" "%{wks.location}/Build/Bin/' .. output_dir .. 'Editor/"',
            'if exist "$(TargetDir)lion-game.pdb" copy /Y "$(TargetDir)lion-game.pdb" "%{wks.location}Build\\Bin\\' .. output_dir:gsub("/", "\\") .. 'Launcher\\"',
            'if exist "$(TargetDir)lion-game.pdb" copy /Y "$(TargetDir)lion-game.pdb" "%{wks.location}Build\\Bin\\' .. output_dir:gsub("/", "\\") .. 'Editor\\"',
            -- The assets, minus the scripts: those are the game's code, already compiled into the module,
            -- and shipping the source of a game alongside it is not a thing anyone means to do.
            --
            -- The exclude list is named relatively because xcopy will not take a quoted path for it, so
            -- an absolute one would break on the first clone that lives under a folder with a space. A
            -- post-build runs in the project's folder, one level under the workspace.
            'xcopy /E /I /Y /Q /EXCLUDE:..\\Scripts\\AssetCopyExclude.txt "%{prj.location}Assets" "%{wks.location}/Build/Bin/' .. output_dir .. 'Launcher/"',
        }

    filter { "system:windows", "configurations:Shipping" }
        -- Make shipped shaders unreadable so they cannot be edited in a text editor.
        postbuildcommands {
            'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}Scripts/ObfuscateShaders.ps1" "%{wks.location}/Build/Bin/' .. output_dir .. 'Launcher/"',
        }
