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

project "Lua"
    kind "StaticLib"
    language "C++"
    location "vendor/lua"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"    


    files { "vendor/lua/**.c" }    
    
project "ImGui"
    kind "StaticLib"
    language "C++"
    location "vendor/imgui"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    
    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }

    includedirs { "vendor/imgui", "vendor/glad/include", "vendor/glfw/include" }
    files { "vendor/imgui/**.h", "vendor/imgui/**.cpp" }

project "TinyXML2"
    kind "StaticLib"
    language "C++"
    location "vendor/tinyxml2"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    
    files { "vendor/tinyxml2/**.cpp" }

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
    cppdialect "C++17"

    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"

    files { "%{prj.name}/**.h", "%{prj.name}/**.cpp" }

    libdirs { "vendor/glfw/lib"  }
    includedirs { 
        "Raytracer", 
        "vendor/imgui", 
        "vendor/glfw/include", 
        "vendor/glad/include",
        "vendor/lua/src",
        "vendor/luastate/include",
        "vendor/tinyxml2"
    }


    links { "Raytracer", "Glad", "glfw3", "ImGui", "opengl32", "Lua", "TinyXML2" }

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }

    postbuildcommands {
        "{COPY} res/ ../bin/%{cfg.buildcfg}/%{prj.name}/res"  
    }            