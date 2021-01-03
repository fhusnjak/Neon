workspace "Neon"
	architecture "x64"
	targetdir "build"
	
	configurations 
	{ 
		"Debug", 
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	startproject "Sandbox"
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Neon/vendor/GLFW/include"
IncludeDir["ImGui"] = "Neon/vendor/ImGui"
IncludeDir["glm"] = "Neon/vendor/glm"
IncludeDir["entt"] = "Neon/vendor/entt/include"
IncludeDir["Vulkan"] = "Neon/vendor/Vulkan/1.2.148.1/Include"

LibraryDir = {}
LibraryDir["Vulkan"] = "vendor/Vulkan/1.2.148.1/Lib/vulkan-1.lib"

group "Dependencies"
include "Neon/vendor/GLFW"
include "Neon/vendor/ImGui"
group ""

group "Core"
project "Neon"
	location "Neon"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "neopch.h"
	pchsource "Neon/src/neopch.cpp"

	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/src/Neon",
		"%{prj.name}/vendor",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Vulkan}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.entt}",
		"%{prj.name}/vendor/assimp/include",
		"%{prj.name}/vendor/stb/include"
	}
	
	links 
	{ 
		"GLFW",
		"ImGui",
		"opengl32.lib",
		"%{LibraryDir.Vulkan}",
	}

	filter "system:windows"
		systemversion "latest"
		
		defines 
		{ 
			"NEO_PLATFORM_WINDOWS",
		}

	filter "configurations:Debug"
		defines "NEO_DEBUG"
		symbols "On"
				
	filter "configurations:Release"
		defines "NEO_RELEASE"
		optimize "On"

group "Tools"
project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	links 
	{ 
		"Neon"
	}
	
	files 
	{ 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.c", 
		"%{prj.name}/src/**.hpp", 
		"%{prj.name}/src/**.cpp" 
	}
	
	includedirs 
	{
		"%{prj.name}/src",
		"Neon/src",
		"Neon/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.glm}"
	}
	
	filter "system:windows"
		systemversion "latest"
				
		defines 
		{ 
			"NEO_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug"
		defines "NEO_DEBUG"
		symbols "on"

		links
		{
			"Neon/vendor/assimp/bin/Debug/assimp-vc141-mtd.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Neon/vendor/assimp/bin/Debug/assimp-vc141-mtd.dll" "%{cfg.targetdir}"'
		}
				
	filter "configurations:Release"
		defines "NEO_RELEASE"
		optimize "on"

		links
		{
			"Neon/vendor/assimp/bin/Release/assimp-vc141-mt.lib"
		}

		postbuildcommands 
		{
			'{COPY} "../Neon/vendor/assimp/bin/Release/assimp-vc141-mt.dll" "%{cfg.targetdir}"'
		}
group ""