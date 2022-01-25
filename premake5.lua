--[[

Win64:

- enable exceptions in assimp (/EHsc)

Win32:

- uncomment "constexpr=const" in this file

]]

--COMPILE_FOR_CPP11_AND_LATER = true


--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==
-- CONFIG: PLATFORM
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

--local PLATFORM_WINDOWS = "windows"


--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==
-- CONFIG: LIBRARIES
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

WITH_RUNTIME_COMPILED_CPLUSPLUS = true

WITH_GLFW = true

WITH_GAINPUT = true
WITH_BULLET_PHYSICS = true -- Bullet
WITH_OZZ_ANIMATION = true

WITH_BGFX = false

WITH_SOUND = false

WITH_BROFILER = false
WITH_REMOTERY = false

--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==
-- CONFIG: SCRIPTING
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

WITH_LUA = false

-- https://quirrel.io/
WITH_QUIRREL = false

-- https://wren.io
--WITH_WREN = false

-- https://marcobambini.github.io/gravity/
--WITH_GRAVITY = false




-- https://github.com/mlabbe/nativefiledialog
WITH_NATIVE_FILE_DIALOG = true

-- a C++ 11 compiler is required for building Google::flatbuffers
WITH_FLATBUFFERS = false

local BOOST_DIR = "D:/sdk/boost_1_57_0/"
D3D_DIR = "D:/sdk/dx_sdk/Include/"
D3D_LIB_DIR = "D:/sdk/dx_sdk/Lib/"

-- disabled, because debug exe cannot link with release libs
FORCE_OPTIMIZATION = false

--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

ROOT_DIR = _MAIN_SCRIPT_DIR
BUILD_DIR = path.join( ROOT_DIR, ".Build" )

SOURCE_DIR = path.join( ROOT_DIR, "Source" )
ENGINE_DIR = path.join( SOURCE_DIR, "Engine" )
TOOLS_DIR = path.join( SOURCE_DIR, "Tools" )

PROJECT_DIR = path.join( BUILD_DIR, _ACTION )
BINARIES_DIR = path.join( ROOT_DIR, "Binaries" )
THIRD_PARTY_DIR = path.join( ROOT_DIR, "External" )

--

local BX_DIR = path.join(THIRD_PARTY_DIR, "bx/include")
local BGFX_DIR = path.join(THIRD_PARTY_DIR, "bgfx/include")

local GAINPUT_INCLUDE_DIR = path.join( THIRD_PARTY_DIR, "gainput/lib/include/" )


function getOzzAnimationIncludeDirs()
	local path_to_ozz_animation_source_code = path.join( THIRD_PARTY_DIR, "ozz-animation" )
	local path_to_ozz_animation_include_dir = path.join( path_to_ozz_animation_source_code, "include" )

	return {
		path.join( path_to_ozz_animation_source_code, "include" ),
		path.join( path_to_ozz_animation_source_code, "src" ),	-- Allows to include private headers.
	}
end

--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

-- global scope, all workspaces will receive these values

defines {
	-- "stdint.h" / "inttypes.h"
	"__STDC_LIMIT_MACROS=1",	-- UINT16_C, etc.
	"__STDC_FORMAT_MACROS=1",	-- PRIi64, etc.
	"__STDC_CONSTANT_MACROS=1",	-- UINT16_MAX, etc.

	"_CRT_SECURE_NO_WARNINGS",
	
	
	--"constexpr=const",	-- for VC++ 2008
}


includedirs {
	BOOST_DIR,
	D3D_DIR,
	THIRD_PARTY_DIR,
	BX_DIR,
	ENGINE_DIR,
	path.join( SOURCE_DIR, "Engine/Runtime" ),
	path.join( SOURCE_DIR, "Research" ),
}

if WITH_GAINPUT then
	includedirs {
		GAINPUT_INCLUDE_DIR
	}
end

if WITH_BGFX then
	includedirs {
		BGFX_DIR
	}
end

--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

-- links {}
function GetExternalLibraries()
	local external_libraries_table = {}

	if WITH_GLFW then
		table.insert( external_libraries_table, "GLFW" )
	end

	if WITH_REMOTERY then
		table.insert( external_libraries_table, "Remotery" )
	end

	if WITH_GAINPUT then
		table.insert( external_libraries_table, "gainput" )
	end
	
	return external_libraries_table
end

if WITH_BROFILER then
	filter "platforms:Win32"
		links ( "ProfilerCore32" )
	filter "platforms:Win64"
		links ( "ProfilerCore64" )
end
	
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==

workspace "NoobWerks"
	startproject "SpaceGame"
	location (PROJECT_DIR)
	language "C++" -- default
	configurations { "Debug", "Release" }
	platforms { "Win32", "Win64" }
	
	include "Make/util.lua"

	group "_0_Engine_"
		include "Source/Engine"

	group "_1_External"
	
		--
		if WITH_GLFW then
			-- https://www.glfw.org/docs/latest/compile.html
			local path_to_glfw = path.join( THIRD_PARTY_DIR, "GLFW" )
			if MakeStaticLibrary( "GLFW", path_to_glfw ) then
				--
				configuration { "vs20* or mingw*" }
					defines {
						"_GLFW_WIN32",
						-- MVC++2008 doesn't have _WIN32_WINNT_VISTA, _WIN32_WINNT_WIN7, etc.
						"_WIN32_WINNT_VISTA=0x0600",
						"_WIN32_WINNT_WIN7=0x0601",
					}
				removefiles {
					path.join( path_to_glfw, "src/glx_context.*" ),
					path.join( path_to_glfw, "src/linux_*.*" ),
					path.join( path_to_glfw, "src/posix_*.*" ),
					path.join( path_to_glfw, "src/x11_*.*" ),
					path.join( path_to_glfw, "src/cocoa_*.*" ),
					path.join( path_to_glfw, "src/wl_*.*" ),
					path.join( path_to_glfw, "src/null_*.*" ),
				}

				--
				configuration { "xcode* or osx**" }
					defines {
						"_GLFW_X11",
					}

				--
				configuration { "ios*" }
					defines {
						"_GLFW_COCOA",
					}
			end
		end

	
		--
		local path_to_bx = path.join( THIRD_PARTY_DIR, "bx" )
		if MakeStaticLibrary( "bx",	path_to_bx ) then
			removefiles {
				path.join( path_to_bx, "src/amalgamated.cpp" ),
				
				-- <bx/math.h> has brace initializers that are not supported by VC++ 2008
				path.join( path_to_bx, "src/math.cpp" ),
				--path.join( path_to_bx, "src/dtoa.cpp" ),	-- cannot exclude: contains various toString() conversion functions
				path.join( path_to_bx, "src/crtnone.cpp" ),
				path.join( path_to_bx, "src/settings.cpp" ),
				
				path.join( path_to_bx, "tests/*.*" ),
				
				path.join( path_to_bx, "tools/*.*" ),
				path.join( path_to_bx, "tools/bin2c/*.*" ),
			}
		end
	
--		if MakeStaticLibrary( "rampantpixels",	path.join( THIRD_PARTY_DIR, "rampantpixels" ) ) then
--		end

--		if MakeStaticLibrary( "turf",	path.join( THIRD_PARTY_DIR, "turf" ) ) then		
--		end

--		if MakeStaticLibrary( "enkiTS",	path.join( THIRD_PARTY_DIR, "enkiTS" ) ) then		
--		end

		-- low-level math libraries
--[[
		if MakeStaticLibrary( "XNAMath",		path.join( THIRD_PARTY_DIR, "XNAMath" ) ) then
--			optimize "On" -- always build with optimization enabled
		end
		if MakeStaticLibrary( "XNACollision",	path.join( THIRD_PARTY_DIR, "XNACollision" ) ) then
--			optimize "On" -- always build with optimization enabled
		end
]]

		local pathToDirectXMath = path.join( THIRD_PARTY_DIR, "DirectXMath" );

		if MakeStaticLibrary( "DirectXMath",	pathToDirectXMath ) then
--			optimize "On" -- always build with optimization enabled
		end

		
--[[	
		-- 
		if MakeStaticLibrary( "DirectXTex",	path.join( THIRD_PARTY_DIR, "DirectXTex" ) ) then
			includedirs {
				pathToDirectXMath,
			}
		end

		-- NVIDIA Texture Tools
		local pathToNvidiaTextureTools = path.join( THIRD_PARTY_DIR, "nvidia-texture-tools" );
		if MakeStaticLibrary( "nvidia-texture-tools", pathToNvidiaTextureTools ) then
			includedirs {
				path.join( pathToNvidiaTextureTools, "src" ),
			}
		end
		
		-- JPEG
		local path_to_JPEG_lib_source = path.join( THIRD_PARTY_DIR, "jpeglib" );
		if MakeStaticLibrary( "jpeglib", path_to_JPEG_lib_source ) then
		end
]]

		
		-- memory debugging
		if MakeStaticLibrary( "ig-memtrace",	path.join( THIRD_PARTY_DIR, "ig-memtrace" ) ) then
			--optimize "On"	-- always build with optimizations enabled
		end
		if MakeStaticLibrary( "tracey",			path.join( THIRD_PARTY_DIR, "tracey" ) ) then
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end		

		-- -- string formatting library (note: use only for tools code)
		-- if MakeStaticLibrary( "cppformat",		path.join( THIRD_PARTY_DIR, "cppformat" ) ) then
			-- if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			-- end
		-- end
		-- double to string conversion:
		if MakeStaticLibrary( "grisu",	path.join( THIRD_PARTY_DIR, "grisu" ) ) then
			--optimize "On"	-- always build with optimizations enabled
		end

		-- compression and decompression
--[[
		-- doesn't compile on MVC++ 2008-2013: "only GCC support in this version"
		if MakeStaticLibrary( "TurboRLE",	path.join( THIRD_PARTY_DIR, "TurboRLE" ) ) then
			removefiles {
				path.join(pathToLMDB, "trle.c"),	-- main()
			}
--			optimize "On" -- always build with optimization enabled
		end
		-- doesn't compile on MVC++ 2008-2013: "only GCC support in this version"
		if MakeStaticLibrary( "TurboPFor",	path.join( THIRD_PARTY_DIR, "TurboPFor" ) ) then
			optimize "On" -- always build with optimization enabled
		end
]]

--[[
		if MakeStaticLibrary( "libfor",		path.join( THIRD_PARTY_DIR, "libfor" ) ) then
			removefiles {
				path.join(pathToLMDB, "for-gen.c"),
				path.join(pathToLMDB, "benchmark.c"),
				path.join(pathToLMDB, "test.c"),
			}
		end
		if MakeStaticLibrary( "libvbyte",	path.join( THIRD_PARTY_DIR, "libvbyte" ) ) then
			removefiles {
				path.join(pathToLMDB, "test.cc"),
				path.join(pathToLMDB, "timer.h"),
				path.join(pathToLMDB, "varintdecode.c"),
				path.join(pathToLMDB, "varintdecode.h"),
			}
		end
]]

		if MakeStaticLibrary( "LZ4", path.join( THIRD_PARTY_DIR, "LZ4" ) ) then
			--optimize "On"	-- always build with optimizations enabled
		end
		
		if MakeStaticLibrary( "zstd", path.join( THIRD_PARTY_DIR, "zstd" ) ) then
			--optimize "On"	-- always build with optimizations enabled
		end


		-- procedural content generation
--		if MakeStaticLibrary( "libnoise",	path.join( THIRD_PARTY_DIR, "libnoise" ) ) then
--			optimize "On" -- always build with optimization enabled
--		end

		-- serialization
		-- if WITH_FLATBUFFERS then
			-- MakeStaticLibrary( "flatbuffers",	path.join( THIRD_PARTY_DIR, "flatbuffers" ) )
		-- end

		-- memory-mapped database (key-value store)
		local pathToLMDB = path.join( THIRD_PARTY_DIR, "liblmdb" )
		if MakeStaticLibrary( "LMDB", pathToLMDB ) then
			removefiles {
				path.join(pathToLMDB, "mdb_copy.c"),
				path.join(pathToLMDB, "mdb_dump.c"),
				path.join(pathToLMDB, "mdb_load.c"),
				path.join(pathToLMDB, "mdb_stat.c"),
				path.join(pathToLMDB, "mtest**.c"),
			}
--			optimize "On" -- always build with optimization enabled
		end

		
		
		
		
		
		
		-- embeddable scripting languages
		if MakeStaticLibrary( "Lua",		path.join( THIRD_PARTY_DIR, "Lua" ) ) then
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
		-- if MakeStaticLibrary( "LuaBridge",	path.join( THIRD_PARTY_DIR, "LuaBridge" ) ) then
			-- if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			-- end
		-- end
		
		if WITH_QUIRREL then
			local path_to_Quirrel = path.join( THIRD_PARTY_DIR, "quirrel" )
			if MakeStaticLibrary( "Quirrel", path_to_Quirrel ) then
				includedirs {
					path.join( path_to_Quirrel, "include" ),
				}
				removefiles {
					path.join( path_to_Quirrel, "sqmodules/*.*" )
				}
			end	
		end

--[[
		if WITH_WREN then
			local path_to_Wren_sources = path.join( THIRD_PARTY_DIR, "Wren" )
			if MakeStaticLibrary( "Wren", path_to_Wren_sources ) then
				includedirs {
					path.join( path_to_Wren_sources, "include" ),
					
					-- wren_common.h
					path.join( path_to_Wren_sources, "vm" ),
				}
				removefiles {
					path.join( path_to_Wren_sources, "optional/*.*" )
				}
			end
		end
			
		if WITH_GRAVITY then
			local path_to_Gravity_sources = path.join( THIRD_PARTY_DIR, "Gravity" )
			if MakeStaticLibrary( "Gravity", path_to_Gravity_sources ) then
				includedirs {
					path.join( path_to_Gravity_sources, "shared" ),	-- e.g. gravity_memory.h
					path.join( path_to_Gravity_sources, "runtime" ),	-- e.g. gravity_core.h
					path.join( path_to_Gravity_sources, "utils" ),	-- e.g. gravity_utils.h
					path.join( path_to_Gravity_sources, "compiler" ),	-- e.g. debug_macros.h
					path.join( path_to_Gravity_sources, "optionals" ),	-- e.g. gravity_optionals.h
				}
--				removefiles {
--					path.join( path_to_Gravity_sources, "optionals/*.*" )
--				}
			end
		end
]]
		
		
		
		
		
		-- procedural generation
		if MakeStaticLibrary( "FastNoise",	path.join( THIRD_PARTY_DIR, "FastNoise" ) ) then
			--optimize "On"	-- always build with optimizations enabled
		end
		-- if MakeStaticLibrary( "FastNoiseSIMD",	path.join( THIRD_PARTY_DIR, "FastNoiseSIMD" ) ) then
			-- --optimize "On"	-- always build with optimizations enabled
		-- end

		-- mesh importer
		local pathToAssimp = path.join( THIRD_PARTY_DIR, "assimp" );
		if MakeStaticLibrary( "assimp", pathToAssimp ) then
			defines {
				"ASSIMP_BUILD_NO_C4D_IMPORTER=1",	-- "c4d_file.h" not found
				"ASSIMP_BUILD_NO_IFC_IMPORTER=1",	-- MVC++9.0(2008): C2039: 'data' : is not a member of 'std::vector<_Ty>' ("ifcboolean.cpp", line 423)
				"ASSIMP_BUILD_NO_OPENGEX_IMPORTER=1",

				"OPENDDL_NO_USE_CPP11=1",	-- for MVC++9.0(2008)
				"OPENDDLPARSER_BUILD=1",	-- DLL_ODDLPARSER_EXPORT := TAG_DLL_EXPORT
			}
			local pathToContrib = path.join( pathToAssimp, "contrib" );
			includedirs {
				path.join( pathToAssimp, "include" ),	-- #include <assimp/cimport.h>
				pathToContrib,
				path.join( pathToContrib, "openddlparser/include" ),
				path.join( pathToContrib, "rapidjson/include" ),
			}
			local pathToZLib = path.join( pathToContrib, "zlib" );
			removefiles {
--				path.join( pathToZLib, "*.*" )
			}
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
		
		-- mesh optimizer
		local path_to_meshoptimizer = path.join( THIRD_PARTY_DIR, "meshoptimizer" );
		if MakeStaticLibrary( "meshoptimizer", path_to_meshoptimizer ) then
		-- vertexfilter.cpp(293): error C3861: '_mm_set1_epi64x': identifier not found
			removefiles {
				path.join( path_to_meshoptimizer, "src/vertexfilter.cpp" ),
			}
		end

--[[
		-- Texture conversion library (for tools)
		if MakeStaticLibrary( "DirectXTex",		path.join( THIRD_PARTY_DIR, "DirectXTex" ) ) then
			includedirs {
				path.join( THIRD_PARTY_DIR, "DirectXMath" )
			}
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
]]


		-- native file open, folder select and save dialogs
		if WITH_NATIVE_FILE_DIALOG then
			local path_to_nativefiledialog = path.join( THIRD_PARTY_DIR, "nativefiledialog" );
			if MakeStaticLibrary( "nativefiledialog", path_to_nativefiledialog ) then
				includedirs {
					path.join( path_to_nativefiledialog, "include" )
				}
				removefiles {
					path.join( path_to_nativefiledialog, "src/nfd_cocoa.m" ),
					path.join( path_to_nativefiledialog, "src/nfd_gtk.c" ),
					path.join( path_to_nativefiledialog, "src/nfd_zenity.c" ),
				}
			end
		end


		-- Input handling
		if WITH_GAINPUT then
			local path_to_gainput = path.join( THIRD_PARTY_DIR, "gainput" )
			if MakeStaticLibrary( "gainput", path_to_gainput ) then
				includedirs {
					--GAINPUT_INCLUDE_DIR
					path.join( path_to_gainput, "lib/include" )
				}
				removefiles {
					path.join( THIRD_PARTY_DIR, "gainput/samples/*.*" ),
					path.join( THIRD_PARTY_DIR, "gainput/test/*.*" ),
				}
			end
		end -- if WITH_GAINPUT


		
		-- Dear ImGui - immediate-mode GUI
		local path_to_DearImGui = path.join( THIRD_PARTY_DIR, "ImGui" )
		if MakeStaticLibrary( "ImGui", path_to_DearImGui ) then
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end

		-- Guizmo for Dear ImGui
		local path_to_ImGuizmo = path.join( THIRD_PARTY_DIR, "ImGuizmo" )
		if MakeStaticLibrary( "ImGuizmo", path_to_ImGuizmo ) then
			includedirs {
				path_to_DearImGui
			}
			-- we don't need these files, and rewriting them in C++03 is a lot of work
			removefiles {
				path.join( path_to_ImGuizmo, "ImCurveEdit.*" ),
				path.join( path_to_ImGuizmo, "ImSequencer.*" ),
			}
		end
		
		-- imGuIZMO.quat is a ImGui trackball widget.
		local path_to_imGuIZMO_quat = path.join( THIRD_PARTY_DIR, "imGuIZMO.quat" )
		if MakeStaticLibrary( "imGuIZMO.quat", path_to_imGuIZMO_quat ) then
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
		
		

		--[[
		-- Nuklear - a minimal-state, immediate-mode graphical user interface toolkit:
		-- https://github.com/Immediate-Mode-UI/Nuklear
		local path_to_Nuklear = path.join( THIRD_PARTY_DIR, "Nuklear" )
		if MakeStaticLibrary( "Nuklear", path_to_Nuklear ) then
		end
		]]--
		
		
		
		-- graphics middleware
		if WITH_BGFX then
			include "Make/bgfx.lua"
--			optimize "On" -- always build with optimization enabled
		end

		--if MakeStaticLibrary( "ASSAO", path.join( THIRD_PARTY_DIR, "ASSAO" ) ) then
		--	optimize "On"
		--end
		
		-- physics engine
		if WITH_BULLET_PHYSICS then
			local path_to_bullet = path.join( THIRD_PARTY_DIR, "Bullet" )
			if MakeStaticLibrary( "Bullet",	path_to_bullet ) then
				-- defines {
					-- "BT_NO_SIMD_OPERATOR_OVERLOADS=1",
				-- }
				removefiles {
					path.join(path_to_bullet, "BulletCollision/BroadphaseCollision/btMultiSapBroadphase.*"),
					path.join(path_to_bullet, "BulletSoftBody/*.*"),

					path.join(path_to_bullet, "btLinearMathAll.cpp"),
					path.join(path_to_bullet, "btBulletCollisionAll.cpp"),
					path.join(path_to_bullet, "btBulletDynamicsAll.cpp"),
				}
			end
			
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end

		-- animation
		if WITH_OZZ_ANIMATION then
			local path_to_ozz_animation_source_code = path.join( THIRD_PARTY_DIR, "ozz-animation" )
			--local path_to_ozz_animation_include_dir = path.join( path_to_ozz_animation_source_code, "include" )

			if MakeStaticLibrary( "ozz-animation", path_to_ozz_animation_source_code ) then
				includedirs {
					getOzzAnimationIncludeDirs()
				}
				removefiles {
					path.join(path_to_ozz_animation_source_code, "src/animation/offline/fbx/*.*"),
					path.join(path_to_ozz_animation_source_code, "src/animation/offline/tools/*.*"),
				}
				
-- OZZ_SIMD_SSE4_2 is auto-defined
--				defines {
--					"OZZ_SIMD_SSE4_2=1",	-- MVC++ 2008 "supports" SSE4.2
--				}
			end
		end
		
		
		-- AI navigation / path finding
		local path_to_recast_navigation_folder = path.join( THIRD_PARTY_DIR, "recastnavigation" )
		if MakeStaticLibrary( "recastnavigation", path_to_recast_navigation_folder ) then
			includedirs {
				path.join( path_to_recast_navigation_folder, "DebugUtils/Include" ),
				path.join( path_to_recast_navigation_folder, "Detour/Include" ),
				path.join( path_to_recast_navigation_folder, "DetourCrowd/Include" ),
				path.join( path_to_recast_navigation_folder, "DetourTileCache/Include" ),
				path.join( path_to_recast_navigation_folder, "Recast/Include" ),
			}
		end

		
		-- network
--		if MakeStaticLibrary( "enet",		path.join( THIRD_PARTY_DIR, "enet" ) ) then
--			optimize "On" -- always build with optimization enabled
--		end




		-- testing/profiling/performance tuning/debugging tools
		if WITH_REMOTERY then
			if MakeStaticLibrary( "Remotery",	path.join( THIRD_PARTY_DIR, "Remotery" ) ) then
				defines { 
					'RMT_ENABLED',
					'RMT_USE_D3D11=1',
				}
			end
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
		--MakeStaticLibrary( "rdestl",	path.join( THIRD_PARTY_DIR, "rdestl" ) )
		--MakeStaticLibrary( "memtracer",	path.join( THIRD_PARTY_DIR, "memtracer" ) )
		MakeStaticLibrary( "dlmalloc",	path.join( THIRD_PARTY_DIR, "dlmalloc" ) )	-- low-fragmentation allocator
		MakeStaticLibrary( "DebugHeap",	path.join( THIRD_PARTY_DIR, "ig-debugheap" ) )	-- for finding memory errors

		-- development/editor tools
		-- directory/file watcher
		if MakeStaticLibrary( "efsw",		path.join( THIRD_PARTY_DIR, "efsw" ) ) then
			if FORCE_OPTIMIZATION then optimize "On" -- always build with optimization enabled
			end
		end
		-- database engine for the asset pipeline
		if MakeStaticLibrary( "SQLite",	path.join( THIRD_PARTY_DIR, "SQLite" ) ) then
			--optimize "On" -- always build with optimization enabled
		end

		-- miscellaneous
		
		-- Signed Distance Field and Occlusion generator
		--[[
		if MakeStaticLibrary( "distance_occlusion",	path.join( THIRD_PARTY_DIR, "distance_occlusion" ) ) then
		end
		]]
		
		if MakeStaticLibrary( "SDFGen",	path.join( THIRD_PARTY_DIR, "SDFGen" ) ) then
		end
		
		--MakeStaticLibrary( "xray_re",	path.join( THIRD_PARTY_DIR, "xray_re" ) )
		--MakeStaticLibrary( "libtga",	path.join( THIRD_PARTY_DIR, "libtga" ) )
		if MakeStaticLibrary( "double-conversion",	path.join( THIRD_PARTY_DIR, "double-conversion" ) ) then
			--optimize "On" -- always build with optimization enabled
		end
--		if MakeStaticLibrary( "MathGeoLib",	path.join( THIRD_PARTY_DIR, "MathGeoLib" ) ) then
--			--defines { 'MATH_ENABLE_NAMESPACE=1' }
--		end


		if WITH_RUNTIME_COMPILED_CPLUSPLUS then
			if MakeStaticLibrary( "RuntimeCompiler",	path.join( THIRD_PARTY_DIR, "RuntimeCompiledCPlusPlus/RuntimeCompiler" ) ) then
				defines { 
					-- 'RMT_ENABLED',
					-- 'RMT_USE_D3D11=1',
				}
			end
			if MakeStaticLibrary( "RuntimeObjectSystem",	path.join( THIRD_PARTY_DIR, "RuntimeCompiledCPlusPlus/RuntimeObjectSystem" ) ) then
				defines { 
					-- 'RMT_ENABLED',
					-- 'RMT_USE_D3D11=1',
				}
			end
		end

	group "_2_Tools"
		include "Source/Tools"

	group "_3_Games"
		include "Source/Games"

	group "_4_Programs"
		include "Source/Tests"

	group "_5_Research"
		include "Source/Research"
		
--[[
--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==--==
solution "Tools"
	location (PROJECT_DIR)
	startproject "AssetCompiler"
	language "C++" -- default
	configurations { "Debug", "Release" }
	platforms { "Win32", "Win64" }

	include "Make/util.lua"

	group "Engine"
		include "Engine"

	group "External"
		-- --include "Make/bgfx.lua"
		MakeStaticLibrary( "ImGui", path.join( THIRD_PARTY_DIR, "ImGui" ) )
		MakeStaticLibrary( "efsw", path.join( THIRD_PARTY_DIR, "efsw" ) )

	include "Tools"
]]
