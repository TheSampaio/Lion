project "Game"
    -- The game is a shared library, loaded at runtime by the standalone launcher and by the editor.
    -- Its file name is fixed (see Lion/Core/GameModule.h), so those loaders do not care what the
    -- project is called.
    kind "SharedLib"
    targetname "lion-game"

    -- Output directories
    targetdir ("%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}")

    -- The folder this module is built from: the project the editor is pointed at (LION_PROJECT_DIR, set by
    -- its Compile), or the built-in Sandbox when nothing overrides it — a plain Build.bat, or the standalone
    -- game. Pointing it elsewhere is what lets opening a project and rebuilding load that project's own
    -- components into the one module the editor and the launcher share.
    local project_dir = os.getenv("LION_PROJECT_DIR")
    local project_override = project_dir ~= nil and project_dir ~= ""

    if not project_override then
        project_dir = "%{prj.location}"
    end

    files {
        project_dir .. "/**.h",
        project_dir .. "/**.cpp",
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
        -- them (see Editor/Source/ModuleSymbols.h). An optimised build produces none, hence "if exist".
        -- The module ships itself to both consumers whatever project it was built from: the editor loads
        -- it from its own folder to hot-reload, so this copy is what a rebuild-on-open depends on.
        postbuildcommands {
            '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '"',
            '{MKDIR} "%{wks.location}/Build/Bin/' .. output_dir .. editor_project .. '"',
            '{COPYFILE} "%{cfg.buildtarget.relpath}" "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '/"',
            '{COPYFILE} "%{cfg.buildtarget.relpath}" "%{wks.location}/Build/Bin/' .. output_dir .. editor_project .. '/"',
            'if exist "$(TargetDir)lion-game.pdb" copy /Y "$(TargetDir)lion-game.pdb" "%{wks.location}Build\\Bin\\' .. output_dir:gsub("/", "\\") .. launcher_project .. '\\"',
            'if exist "$(TargetDir)lion-game.pdb" copy /Y "$(TargetDir)lion-game.pdb" "%{wks.location}Build\\Bin\\' .. output_dir:gsub("/", "\\") .. editor_project .. '\\"',
        }

    -- The assets travel beside the standalone launcher only for the built-in Sandbox, whose layout the
    -- exclude list is written against. A project opened from elsewhere is browsed in place by the editor and
    -- does not need its assets copied for the editor's sake — that is the standalone story, and its own step.
    -- Copying them with a Sandbox-relative exclude would fail on a folder that is not Sandbox, and a failed
    -- post-build is a failed build.
    if not project_override then
        filter "system:windows"
            -- The assets, minus the scripts: those are the game's code, already compiled into the module,
            -- and shipping the source of a game alongside it is not a thing anyone means to do.
            --
            -- The exclude list is named relatively because xcopy will not take a quoted path for it, so
            -- an absolute one would break on the first clone that lives under a folder with a space. A
            -- post-build runs in the project's folder, one level under the workspace.
            postbuildcommands {
                'xcopy /E /I /Y /Q /EXCLUDE:..\\Scripts\\AssetCopyExclude.txt "%{prj.location}Assets" "%{wks.location}/Build/Bin/' .. output_dir .. launcher_project .. '/"',
            }
    end
