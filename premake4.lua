-- A solution contains projects, and defines the available configurations
solution "SDL img"
	configurations { "Debug", "Release" }
	--location "build"
	-- includedirs {  }
	links { "SDL2", "m" }

	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	configuration "Release"
		defines { "NDEBUG" }
		defines { "Optimize" }
	

	project "sdl_img"
		location "build"
		kind "ConsoleApp"
		language "C"
		files {
			"src/sdl_img.c",
			"src/cvector.h",
			"src/stb_image.h"
		}
		targetdir "build"
		
		configuration { "linux", "gmake" }
			buildoptions { "-std=gnu99", "-fno-strict-aliasing", "-Wall" }
