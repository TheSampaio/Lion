project "Game"
    -- The game is a shared library, loaded at runtime by the standalone launcher and by the editor.
    kind "SharedLib"

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

        -- Ship the module and its resources next to the standalone launcher (Sandbox), which loads
        -- them from its own directory. The editor copies both into its own directory separately.
        postbuildcommands {
            '{COPYFILE} "%{cfg.buildtarget.relpath}" "%{wks.location}/.Out/Bin/' .. output_dir .. 'Sandbox/"',
            'xcopy /E /I /Y /Q "%{prj.location}Resource" "%{wks.location}/.Out/Bin/' .. output_dir .. 'Sandbox/"',
        }

    filter { "system:windows", "configurations:Shipping" }
        -- Make shipped shaders unreadable so they cannot be edited in a text editor.
        postbuildcommands {
            'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}Scripts/ObfuscateShaders.ps1" "%{wks.location}/.Out/Bin/' .. output_dir .. 'Sandbox/"',
        }
