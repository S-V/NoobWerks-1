--[[
if MakeStaticLibrary( "Runtime" ) then
	filter "system:windows"
		links {
			"kernel32",
			"gdi32",
			"winmm",	-- timeGetTime()
			"Dbghelp",	-- MiniDumpWriteDump()
			"Imm32",	-- Input Method Manager
			"comctl32",	-- Common Controls
		}

	links {
		"SDL2.lib" -- for Windows Driver, Input Handling
	}
end
]]

if MakeStaticLibrary( "Base", "Base" ) then
	filter "system:windows"
		links {
			"kernel32",
			"gdi32",
			"winmm",	-- timeGetTime()
			"Dbghelp",	-- MiniDumpWriteDump()
			"Imm32",	-- Input Method Manager
			"comctl32",	-- Common Controls
		}
	HACK_AddDependencies {
		"grisu",
	}
end

if MakeStaticLibrary( "Core", "Core" ) then
	links {
		"Base",
		"dlmalloc",	-- for Lua
	}
end

if MakeStaticLibrary( "Input", "Input" ) then
	links {
		"Base",
		"Core",

		"SDL2.lib" -- for Windows Driver, Input Handling
	}
end

if MakeStaticLibrary( "Scripting", "Scripting" ) then
	links {
		"Base",
		"Core",

		"Lua",
		--"Lua.lib",
	}
end

include "GPU"
include "Graphics"

if MakeStaticLibrary( "Rendering", "Rendering" ) then
	links {
		"Base",
		"Core",
		"Graphics",
	}
	local path_to_rendering = path.join( ENGINE_DIR, "Runtime/Rendering" )
	local path_to_shaders = path.join( path_to_rendering, "Private/Shaders" )
	includedirs {
		path_to_shaders
	}
end

if MakeStaticLibrary( "Physics", "Physics" ) then
	links {
		"Base",
		"Core",
	}
	if WITH_BULLET_PHYSICS then
		-- VoxelTerrainCollisions includes "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"
		local path_to_bullet = path.join( THIRD_PARTY_DIR, "Bullet" )
		includedirs {
			path_to_bullet
		}
	end
end

if MakeStaticLibrary( "Sound", "Sound" ) then
	links {
		"Base",
		"Core",
	}
end

if MakeStaticLibrary( "ProcGen", "ProcGen" ) then
	links {
		"Base",
	}
end

if MakeStaticLibrary( "Voxels", "Voxels" ) then
	links {
		"Base",
	}
end

--
if MakeStaticLibrary( "Planets", "Planets" ) then
	links {
		"Voxels",
	}
end


--
if MakeStaticLibrary( "VoxelsSupport", "VoxelsSupport" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Rendering",
		
		"Voxels",
	}
end

--
if MakeStaticLibrary( "Engine", "Engine" ) then
	links {
		"Base",
		"Core",
		"Input",
		"Graphics",
		"Rendering",
		"Scripting",
		
		"SDL2.lib" -- for Windows Driver, Input Handling
	}
	
	if WITH_GAINPUT then
		dependson { "gainput" }
	end
end

--[[
if MakeStaticLibrary( "Driver", "Driver" ) then
	links {
		"Base",
		"Core",
		"SDL2.lib" -- for Windows Driver, Input Handling
	}

	if WITH_GAINPUT then
		dependson { "gainput" }
	end
end
]]