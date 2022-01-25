-- Useful functions for creating library and executable projects.
-- references: https://github.com/premake/premake-core/wiki/flags

-- NOTE: disabled whole program optimization / POGO / LTCG

local BX_DIR = path.join( THIRD_PARTY_DIR, "bx/include" )


function AddDefaultFilters()
	filter "configurations:Debug"
		defines {
			"DEBUG", "_DEBUG", "___DEBUG",
		}
		flags { "Symbols" }
		optimize "Off"
		--targetsuffix "-D"

	filter "configurations:Release"
		defines {
			"NDEBUG",
			"_SECURE_SCL=0",
		}
		flags {
			
			
			-- recommended to DISABLE in RELEASE
			--"Symbols",	-- Generate debugging information for better debugging experience.

			
			-- recommended to ENABLE in RELEASE
			--"Optimize",		-- Perform a balanced set of optimizations.
			--"LinkTimeOptimization",	-- Enable link-time (i.e. whole program) optimizations. Link-time Code Generation: https://msdn.microsoft.com/en-us/library/xbf3tbeh.aspx
			

			
			
			"NoBufferSecurityCheck",	-- Turn off stack protection checks.
			
			--FatalCompileWarnings,	Treat compiler warnings as errors.
			--"FatalWarnings",	-- Treat all warnings as errors; equivalent to FatalCompileWarnings, FatalLinkWarnings
		}

		-- Perform a balanced set of optimizations.
		--optimize "On"
		optimize "Full"

		-- Enable Intrinsic Functions
		--intrinsics "On"
		
		-- Any suitable
		inlining "Auto"

		floatingpoint "Fast"

-- NOTE: Assimp and boost rely on C++ exceptions
		--Disable C++ exception support.
		exceptionhandling "Off"

		-- Disable C++ runtime type information.
		rtti "Off"


	filter "platforms:Win32"
		system "Windows"
		architecture "x86"
		defines { "WIN32", "PLATFORM_WIN32" }
		linkoptions { }
		objdir (path.join( BUILD_DIR, "obj" )) -- premake will append "\Win32\Debug"
		targetdir (path.join( BINARIES_DIR, "x86" ))
		debugdir (targetdir())
		libdirs {
			path.join( THIRD_PARTY_DIR, "_libs32" ),
			path.join( BINARIES_DIR, "x86" ),
			path.join( D3D_LIB_DIR, "x86" ),
		}

	filter "platforms:Win64"
		system "Windows"
		architecture "x86_64"
		defines { "WIN64", "PLATFORM_WIN64" }
		--linkoptions { "/machine:x64" }
		objdir (path.join( BUILD_DIR, "obj" ))
		targetdir (path.join( BINARIES_DIR, "x64" ))
		debugdir (targetdir())
		libdirs {
			path.join( THIRD_PARTY_DIR, "_libs64" ),
			path.join( BINARIES_DIR, "x64" ),
			path.join( D3D_LIB_DIR, "x64" ),
		}
		
	filter { "action:vs*" }
		includedirs {
			path.join(BX_DIR, "compat/msvc")
		}
		
	filter {}
end


function GetEngineLibraries()
    return {
		--"Runtime.lib",
		"Base",
		"Core",
		"Graphics",
		"Rendering",
		"Physics",

		"Scripting",
		"Sound",

		"Engine",

		"Voxels",
		"VoxelsSupport",
		"ProcGen",
		"Planets",
		
		-- Runtime Utilities
		"Utility.lib",

		--"Offline.lib",
		"Developer.lib",
	}
end


function getUtilityLibraries()
    return {
		"Animation",
		"Compression",
		"DemoFramework",
		"GameFramework",
		"glTF",
		"GUI",
		"Image",
		"Implicit",
		"MeshLib",
		"Meshok",
		"TxTSupport",
		"VisualDebugger",

		"VoxelEngine",
		
		"ThirdPartyGames",
	}
end


function Compose_Filter_For_Files_In_Folder( _folder )
    return {
		path.join( _folder, "**.h" ),
		path.join( _folder, "**.c" ),
		path.join( _folder, "**.cc" ),
		path.join( _folder, "**.hpp" ),
		path.join( _folder, "**.cpp" ),		
		path.join( _folder, "**.hxx" ),
		path.join( _folder, "**.cxx" ),
		path.join( _folder, "**.inl" ),
		path.join( _folder, "**.lua" ),
		
		path.join( _folder, "**.config" ),
		path.join( _folder, "**.script" ),
		path.join( _folder, "**.shader" ),

		path.join( _folder, "**.vs" ), path.join( _folder, "**.ps" ), path.join( _folder, "**.gs" ),
		path.join( _folder, "**.glsl" ),
		path.join( _folder, "**.hlsl" ),
		path.join( _folder, "**.hlsli" ),
		path.join( _folder, "**.fx" ),
		--path.join( _folder, "**.shd" ),	-- Humus's framework and old ATI SDK samples (circa 2006)

		path.join( _folder, "**.md" ),
		path.join( _folder, "**.txt" ),
	}
end

function MakeStaticLibrary( _name, _folder, _defines )

	project ( _name )
		--uuid (os.uuid(_name)) --dunno what this is for
		kind "StaticLib"
		language "C++"
		location ( PROJECT_DIR )

		includedirs {
			".",
			_folder,
		}

		--
		if WITH_OZZ_ANIMATION then
			includedirs {
				getOzzAnimationIncludeDirs()
			}
			if _name ~= "ozz-animation" then
				links ( "ozz-animation" )
			end
		end

		--
		if _folder ~= nil then
			files {
				Compose_Filter_For_Files_In_Folder( _folder )
			}
		end

		AddDefaultFilters()

	return true;
end

function MakeExecutable( _name, _folder, _defines )

	project ( _name )
		--uuid (os.uuid(_name)) --dunno what this is for
		kind "ConsoleApp"
		language "C++"
		location ( PROJECT_DIR )
		
		includedirs {
			".",
			_folder,
		}
		
		--
		if WITH_OZZ_ANIMATION then
			includedirs {
				getOzzAnimationIncludeDirs()
			}
			links ( "ozz-animation" )
		end

		--
		local BGFX_DIR = path.join(THIRD_PARTY_DIR, "bgfx")
		local BGFX_THIRD_PARTY_DIR = path.join(BGFX_DIR, "3rdparty")

		if WITH_BGFX then
			includedirs {
				BGFX_THIRD_PARTY_DIR,
				path.join(BGFX_DIR, "examples/common"),
			}
		end

		if _folder ~= nil then
			files {
				Compose_Filter_For_Files_In_Folder( _folder )
			}
		end

		AddDefaultFilters()

	return true;
end

-- NOTE: for some unknown reason, Premake won't add my static lib projects
function HACK_AddDependencies( _libraries )
	dependson ( _libraries )
	links ( _libraries )
end
