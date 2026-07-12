project "Launcher"
    -- Thin standalone launcher: it links only the engine and loads the game module at runtime. It owns
    -- no game code — that lives in the game module, which the editor loads too.
    kind "ConsoleApp"
    targetname "lion-launcher"

    -- Output directories
    targetdir ("%{wks.location}/Build/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/Build/Obj/" .. output_dir .. "%{prj.name}")

    -- Run from the output folder, where the game module and the assets are.
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
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion",
    }

    -- The game module is loaded at runtime, not linked, so name it as a build-order dependency to
    -- ensure Game.dll exists (and has been copied next to this executable) before a run.
    dependson { "Game" }

    filter "configurations:Shipping"
        kind "WindowedApp"

        -- A shipped game's assets are sealed; a project's are not. The format lives in the engine
        -- (Lion/Core/Vault.h) and the editor is what applies it, because building a game is a thing an
        -- editor does — it used to be a whole executable of its own, which is one project too many.
        --
        -- It runs here, and not in the game module's build, because this is the folder the assets are
        -- copied into: seal them where they land, once they have all landed.
        dependson { editor_project }

        postbuildcommands {
            '"%{wks.location}/Build/Bin/' .. output_dir .. editor_project .. '/Lion.exe" --seal "%{cfg.targetdir}" .glsl .lscene',
        }

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"
