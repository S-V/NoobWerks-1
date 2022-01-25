if MakeStaticLibrary( "GPU", "" ) then
	links {
		"Base",
		"Core",
	}
--[[
	filter "platforms:Win32"
		libdirs {
			path.join( D3D_LIB_DIR, "x86" ),
		}

	filter "platforms:Win64"
		libdirs {
			path.join( D3D_LIB_DIR, "x64" ),
		}
]]
	filter "system:windows"
		links {
			"DxErr.lib",
			"d3dcompiler.lib",
			"dxgi.lib", "dxguid.lib",

			"d3d11.lib",			
		}

	-- D3DX11 utility library
	filter( { "system:windows", "configurations:Release" } )
		links { "d3dx11d.lib" }
	filter( { "system:windows", "configurations:Release" } )
		links { "d3dx11d.lib" }
end
