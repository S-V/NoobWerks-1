-- Useful functions for creating library and executable projects.

function MakeStaticLibrary( _name, _folder, _defines )

	project ( _name )
		--uuid (os.uuid(_name)) --dunno what this is for
		kind "StaticLib"
		language "C++"
		location ( PROJECT_DIR )

		-- files {
			-- "**.h", "**.cpp",
		-- }

		includedirs {
			".",
			_folder,
		}

		if _folder ~= nil then
			files {
				path.join( _folder, "**.h" ),
				path.join( _folder, "**.cpp" ),
			}
		end

		-- defines {
			-- _defines,
		-- }

		-- configuration { "Debug" }
			-- defines {
				-- "BGFX_CONFIG_DEBUG=1",
			-- }
			-- links {
				-- "sdldebug",
			-- }

		-- configuration { "Release" }
			-- links {
				-- "sdlrelease",
			-- }

	return true;
end

function MakeExecutable( _name, _folder, _defines )

	project ( _name )
		--uuid (os.uuid(_name)) --dunno what this is for
		kind "ConsoleApp"
		language "C++"
		location ( PROJECT_DIR )

		libdirs {
			targetdir(),
		}

		if _folder ~= nil then
			files {
				path.join( _folder, "**.h" ),
				path.join( _folder, "**.cpp" ),
			}
		end
		
		-- files {
			-- "**.h", "**.cpp",
		-- }

		includedirs {
			".",
			_folder,
		}
	
		-- defines {
			-- _defines,
		-- }

		-- configuration { "Debug" }
			-- defines {
				-- "BGFX_CONFIG_DEBUG=1",
			-- }
			-- links {
				-- "sdldebug",
			-- }

		-- configuration { "Release" }
			-- links {
				-- "sdlrelease",
			-- }

	return true;
end
