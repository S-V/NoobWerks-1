if MakeExecutable( "AssetCompiler", "AssetCompiler" ) then
	includedirs {
		TOOLS_DIR,
	}
	HACK_AddDependencies {
		GetEngineLibraries(),
		"Developer",	-- shader compiler
		-- 3rd party
		"efsw",	-- File watcher
		"meshoptimizer",
		"SDFGen",
	}
	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end

--[[
----------------------------------------------------------
if MakeExecutable( "BundleUsedAssets", "BundleUsedAssets" ) then
	--
	includedirs {
		TOOLS_DIR
	}
	HACK_AddDependencies {
		GetEngineLibraries(),
	}
end

----------------------------------------------------------
if MakeExecutable( "HeightmapCompiler", "PlanetTools/HeightmapCompiler" ) then
	-- CImg
	local path_to_JPEG_lib_source = path.join( THIRD_PARTY_DIR, "jpeglib" );
	--
	includedirs {
		TOOLS_DIR,
		path_to_JPEG_lib_source,
	}
	HACK_AddDependencies {
		GetEngineLibraries(),
		"Developer",	-- shader compiler
		-- 3rd party
		"efsw",	-- File watcher		
	}
	if WITH_BROFILER then
		filter "platforms:Win32"
			links ( "ProfilerCore32" )
		filter "platforms:Win64"
			links ( "ProfilerCore64" )
	end
end
]]
