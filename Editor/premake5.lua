project "Editor"
    kind "ConsoleApp"

    -- Output directories
    targetdir ("%{wks.location}/.Out/Bin/" .. output_dir .. "%{prj.name}")
    objdir    ("%{wks.location}/.Out/Obj/" .. output_dir .. "%{prj.name}")

    files {
        "%{prj.location}/**.h",
        "%{prj.location}/**.cpp",
        "%{dependencies.imguizmo.source}",  -- compiled with the editor (editor-only dependency)
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
        "%{wks.location}/Vendor/imgui/include/imgui",  -- so ImGuizmo can include "imgui.h" directly
        "%{dependencies.spdlog.include}",
        "%{dependencies.stb.include}",
    }

    links {
        "Lion",
        "%{dependencies.imgui.lib}",
        "%{dependencies.glfw.lib}",  -- ImGui's GLFW backend links against GLFW directly (editor-only).
    }

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
