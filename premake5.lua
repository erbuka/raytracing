workspace "Raytracer"
    configurations { "Debug", "Release" }
    architecture "x86_64"
    startproject "Sandbox0"
    system "Windows"

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"    

project "Raytracer"
    kind "StaticLib"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    

    files { "Raytracer/**.h", "Raytracer/**.cpp" }

project "Sandbox0"
    kind "ConsoleApp"
    language "C++"
    targetdir "build/%{cfg.buildcfg}"
    debugdir "build/%{cfg.buildcfg}"

    files { "Sandbox0/**.h", "Sandbox0/**.cpp" }

    includedirs { "Raytracer", "vendor/SFML/include" }

    libdirs { "vendor/SFML/lib" }

    links { "Raytracer", "freetype", "opengl32", "winmm", "gdi32" }

    defines { "SFML_STATIC" }

    postbuildcommands {
        "{COPY} \"Sandbox0/res\" \"%{cfg.buildtarget.directory}res\""
    }

    filter "configurations:Debug"
        links { "sfml-system-s-d", "sfml-window-s-d", "sfml-graphics-s-d" }

    filter "configurations:Release"
        links { "sfml-system-s", "sfml-window-s", "sfml-graphics-s" }  