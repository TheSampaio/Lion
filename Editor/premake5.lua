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
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",  -- includes the vendored ImGuizmo.cpp under Source/Vendor (editor-only)
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
        "%{dependencies.imguizmo.include}",
        "%{wks.location}/Vendor/imguizmo/include/imguizmo",  -- so ImGuizmo.cpp finds its own "ImGuizmo.h"
        "%{wks.location}/Vendor/imgui/include/imgui",  -- so ImGuizmo can include "imgui.h" directly
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion",
        "%{dependencies.imgui.lib}",
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
            -- Shared resources (shaders + sprites) flattened next to the executable, minus the scripts:
            -- the editor reads those from the project, and they are compiled into the module anyway.
            -- The exclude list is named relatively — see the same copy in Sandbox/premake5.lua.
            'xcopy /E /I /Y /Q /EXCLUDE:..\\Scripts\\AssetCopyExclude.txt "%{wks.location}/Sandbox/Assets" "%{cfg.targetdir}"',
        }
