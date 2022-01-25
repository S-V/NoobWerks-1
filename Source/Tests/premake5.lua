-- 2021.08.25
if MakeExecutable( "VoxelDemo-0", "VoxelDemo-0" ) then
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"

	HACK_AddDependencies {
		GetEngineLibraries(),	-- Engine
		"Utility",
		"Developer",
		-- 3rd party
		"LMDB",	-- voxel database
		"LZ4",	-- voxel compressor
		"ImGui.lib",
		"efsw.lib",
		-- precompiled libraries
		"SDL2.lib",
		GetExternalLibraries(),

		--
		--"BlobTree",
		
		-- for creating custom spaceships from .obj
		"nativefiledialog",
	}

	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end



-- 2016.07.03. deleted z-demo-terrain2, created z-demo-terrain3.
-- 2016.12.29. commented out z-demo-terrain3, created z-demo-terrain4.
--  2017.05.19 - created z-voxel-terrain-5.
-- 2019.11.15 - created TowerDefenseGame from z-voxel-terrain-5.


-- 2020.09.11 ..
if MakeExecutable( "Half-Assed", "HalfAssed" ) then
	--pchheader "stdafx.h"
	--pchsource "stdafx.cpp"

	HACK_AddDependencies {
		GetEngineLibraries(),	-- Engine
		"Utility",
		"Developer",
		-- 3rd party
		"LMDB",	-- voxel database
		"LZ4",	-- voxel compressor
		"ImGui.lib",
		"efsw.lib",
		-- precompiled libraries
		"SDL2.lib",
		GetExternalLibraries(),

		--
		"BlobTree",
		
		-- for creating custom spaceships from .obj
		"nativefiledialog",
	}

	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end




-- 2020.08.30 ..
if MakeExecutable( "Demo-Red-Faction-Remake", "Demo-Red-Faction-Remake" ) then
	HACK_AddDependencies {
		GetEngineLibraries(),	-- Engine
		"Utility",
		"Developer",
		-- 3rd party
		"LMDB",	-- voxel database
		"LZ4",	-- voxel compressor
		"ImGui.lib",
		"efsw.lib",
		-- precompiled libraries
		"SDL2.lib",
		GetExternalLibraries(),
	}
	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end



-- 2019.11.15 ..
if MakeExecutable( "TowerDefenseGame", "TowerDefenseGame" ) then
	HACK_AddDependencies {
		GetEngineLibraries(),	-- Engine
		"Utility",
		"Developer",
		-- 3rd party
		"LMDB",	-- voxel database
		"LZ4",	-- voxel compressor
		"ImGui.lib",
		"efsw.lib",
		-- precompiled libraries
		"SDL2.lib",
		GetExternalLibraries(),
	}
	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end
--[[

]]--



--[[



if MakeExecutable( "proc-tex-gen", "proc-tex-gen" ) then
	dependson {
		GetEngineLibraries(),
		"Utility",
		"Developer",
	}

	-- NOTE: for some unknown reason, Premake won't add static lib projects
	links {
		GetEngineLibraries(),	-- Engine

		-- 3rd party
		--"bgfx",
		"ImGui.lib",
		"efsw.lib",

		-- precompiled libraries
		"SDL2.lib",
	}
end


if MakeExecutable( "1-demo-shaders", "1-demo-shaders" ) then
	dependson {
		GetEngineLibraries(),
		"Utility",
		"Developer",
	}

	-- NOTE: for some unknown reason, Premake won't add static lib projects
	links {
		GetEngineLibraries(),	-- Engine

		-- 3rd party
		--"bgfx",
		"ImGui.lib",
		"efsw.lib",

		-- precompiled libraries
		"SDL2.lib",
	}
end




if MakeExecutable( "Read-RES", "Read-RES" ) then
	dependson {
		GetEngineLibraries(),	-- Engine
	}
	-- NOTE: for some unknown reason, Premake won't add static lib projects
	links {
		GetEngineLibraries(),	-- Engine
	}
end

]]
