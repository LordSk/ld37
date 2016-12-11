--
local SDL2_lib_gcc = "C:/Prog/Libraries/SDL2-2.0.4/install_gcc/lib/libSDL2.dll.a"
local SDL2_include_gcc = "C:/Prog/Libraries/SDL2-2.0.4/install_gcc/include"
local SDL2_lib_msvc = "C:/Prog/Libraries/SDL2-2.0.4/install_msvc/lib/SDL2.lib"
local SDL2_include_msvc = "C:/Prog/Libraries/SDL2-2.0.4/install_msvc/include"
local soloud_include = "C:/Prog/Libraries/soloud/include"
local soloud_lib_msvc = "C:/Prog/Libraries/soloud/lib/soloud_static.lib"
--local soloud_lib_msvc = "C:/Prog/Libraries/soloud/lib/debug/soloud_static.lib"
local lz4_include = "C:/Prog/Libraries/lz4_msvc/include"
local lz4_lib_msvc = "C:/Prog/Libraries/lz4_msvc/dll/liblz4.lib"
--local lz4_lib_msvc = "C:/Prog/Libraries/lz4/visual/VS2010/bin/x64_Debug/liblz4.lib"

solution "ld37_sol"
	location "build"
	
	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"x64"
	}

	language "C++"

PROJ_DIR = path.getabsolute("..")

project "LD37"
	kind "WindowedApp"
	
	configuration {"Debug"}
		targetsuffix "_debug"
		flags {
			"Symbols"
		}
		defines {
			"DEBUG",
			"CONF_DEBUG"
		}
	
	configuration {"Release"}
		targetsuffix "_release"
		flags {
			"Optimize"
		}
		defines {
			"NDEBUG",
			"CONF_RELEASE"
		}
	
	configuration {}
	
	includedirs {
		SDL2_include_msvc,
		soloud_include,
		lz4_include
	}
	
	links {
		"user32",
		"shell32",
		"winmm",
		"ole32",
		"oleaut32",
		"imm32",
		"version",
		"ws2_32",
		"advapi32",
		SDL2_lib_msvc,
		soloud_lib_msvc,
		lz4_lib_msvc
	}
	linkoptions { "/subsystem:windows" }
	
	flags {
		"NoExceptions",
		"NoRTTI",
		"EnableSSE",
		"EnableSSE2",
		"EnableAVX",
		"EnableAVX2"
	}
	
	targetdir(path.join(PROJ_DIR, "build"))
	
	includedirs {
		"src",
		"src/common",
	}
	
	files {
		"src/common/**.h",
		"src/common/**.cpp",
		
		"src/engine/**.h",
		"src/engine/**.cpp",
		
		"src/external/external.cpp",
		"src/external/gl3w.h",
		"src/external/glcorearb.h",
		"src/external/stb_image.h",
		"src/external/parson.h",
		"src/external/parson.c",
		
		"src/ld37/**.h",
		"src/ld37/**.cpp",
	}
	
	links {
		"gdi32",
		"glu32",
		"opengl32"
	}
	
	defines {
		"LSK_MATH_OPERATORS",
		--"LSK_MATH_SIMD_IMPL"
	}