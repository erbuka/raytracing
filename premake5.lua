workspace "Raytracer"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "Sandbox0"
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"    

project "Raytracer"
    kind "StaticLib"
    language "C++"
    location "Raytracer"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

project "Sandbox0"
    kind "ConsoleApp"
    language "C++"
    location "Sandbox0"
    
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

    includedirs { "Raytracer", "vendor/SFML/include" }

    defines { "SFML_STATIC" }

    libdirs { "vendor/SFML/lib" }

    links { "Raytracer", "freetype", "opengl32", "winmm", "gdi32" }


    filter "configurations:Debug"
        links { "sfml-system-s-d", "sfml-window-s-d", "sfml-graphics-s-d" }

    filter "configurations:Release"
        links { "sfml-system-s", "sfml-window-s", "sfml-graphics-s" }  
        postbuildcommands {
            "{COPY} res/ ../bin/%{cfg.buildcfg}/%{prj.name}/res"
        }        