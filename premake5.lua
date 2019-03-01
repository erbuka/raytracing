workspace "Raytracer"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "Sandbox"
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"    

    
project "ImGui"
    kind "StaticLib"
    language "C++"
    location "vendor/imgui"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    
    includedirs { "vendor/imgui" }
    files { "vendor/imgui/**.h", "vendor/imgui/**.cpp" }

project "Glad"
    kind "StaticLib"
    language "C"
    location "vendor/glad"  

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { "vendor/glad/include" }
    files { "vendor/glad/src/*.c" }

project "Raytracer"
    kind "StaticLib"
    language "C++"
    location "Raytracer"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    location "Sandbox"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

    libdirs { "vendor/glfw/lib"  }
    includedirs { "Raytracer", "vendor/imgui", "vendor/glfw/include", "vendor/glad/include" }

    links { "Raytracer", "Glad", "glfw3dll", "ImGui", "opengl32" }

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }

    postbuildcommands {
        "{COPY} ../vendor/glfw/lib/glfw3.dll ../bin/%{cfg.buildcfg}/%{prj.name}" 
    }            


project "Sandbox0"
    kind "ConsoleApp"
    language "C++"
    location "Sandbox0"
    
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

    includedirs { "Raytracer", "vendor/SFML/include" }

    defines { "SFML_STATIC" }

    libdirs { "vendor/SFML/lib" }

    links { "Raytracer", "freetype", "opengl32", "winmm", "gdi32" }

    postbuildcommands {
        "{COPY} res/ ../bin/%{cfg.buildcfg}/%{prj.name}/res"
    }            

    filter "configurations:Debug"
        links { "sfml-system-s-d", "sfml-window-s-d", "sfml-graphics-s-d" }

    filter "configurations:Release"
        links { "sfml-system-s", "sfml-window-s", "sfml-graphics-s" }  