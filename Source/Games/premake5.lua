-- 2021.08.03
if MakeExecutable( "SpaceGame", "SpaceGame" ) then
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"

	---
	local path_to_bullet = path.join( THIRD_PARTY_DIR, "Bullet" )

	includedirs {
		path_to_bullet
	}

	---
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



-- 2021.05.16
if MakeExecutable( "SpaceGateDefense", "SpaceGateDefense" ) then
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
