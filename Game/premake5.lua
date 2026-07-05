project "Sandbox"
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
    }

    -- Run from the output folder so the game finds its resources the same way it ships.
    debugdir "%{cfg.targetdir}"

    filter "configurations:Shipping"
        kind "WindowedApp"

    filter "system:windows"
        buildoptions { "/utf-8" }
        defines "LN_PLATFORM_WIN"
        systemversion "latest"

        -- Copy the Resource folder contents next to the executable (Shader/, Sprite/, ... at root).
        postbuildcommands {
            'xcopy /E /I /Y /Q "%{prj.location}Resource" "%{cfg.targetdir}"',
        }

    filter { "system:windows", "configurations:Shipping" }
        -- Make shipped shaders unreadable so they cannot be edited in a text editor.
        postbuildcommands {
            'powershell -NoProfile -ExecutionPolicy Bypass -File "%{wks.location}Scripts/ObfuscateShaders.ps1" "%{cfg.targetdir}"',
        }