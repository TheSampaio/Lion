project "Editor"
    kind "ConsoleApp"

    -- Output directories
    targetdir ("%{wks.location}/.Out/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/.Out/Obj/" .. output_dir .. "%{prj.name}")

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
            '{COPY} "%{wks.location}/.Out/Bin/' .. output_dir .. 'Lion/Lion.dll" "%{cfg.targetdir}"',
            -- GLFW shared library (the editor's ImGui backend links against it directly).
            '{COPYFILE} "%{dependencies.glfw.dll}" "%{cfg.targetdir}"',
            -- Shared resources (shader + sprites) flattened next to the executable.
            'xcopy /E /I /Y /Q "%{wks.location}/Game/Resource" "%{cfg.targetdir}"',
        }
