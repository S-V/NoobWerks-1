local BGFX_DIR = path.join(THIRD_PARTY_DIR, "bgfx")
local BX_DIR = path.join(THIRD_PARTY_DIR, "bx/include")
local BGFX_THIRD_PARTY_DIR = path.join(BGFX_DIR, "3rdparty")

local useDirect3D = false
local bWithTools = true

project "bgfx"
	kind "StaticLib"
	language "C++"
	location (PROJECT_DIR)

	includedirs {
		BGFX_THIRD_PARTY_DIR,
		-- bgfx distribution contains more up-to-date DirectX SDK headers
		path.join(BGFX_THIRD_PARTY_DIR, "dxsdk/include"),
		path.join(BGFX_DIR, "include"),
	}

	defines {
		-- use built-in allocator
		"BGFX_CONFIG_USE_TINYSTL=0",

		"BGFX_CONFIG_RENDERER_DIRECT3D11=1",
		--"BGFX_CONFIG_RENDERER_OPENGL=1",

		-- temporary hack!
		--"BGFX_CONFIG_MULTITHREADED=0",
	}

	files {
		path.join(BGFX_DIR, "include/**.h"),
		path.join(BGFX_DIR, "src/**.cpp"),
		path.join(BGFX_DIR, "src/**.h"),
		-- include bx headers
		path.join(BX_DIR, "**.h"),
	}
	removefiles {
		path.join(BGFX_DIR, "src/**.bin.h"),
		path.join(BGFX_DIR, "src/amalgamated.cpp"),
	}
	-- -- these break mvs2008 compatibility (std::vector<T>::data())
	-- if _ACTION == "vs2008" then
		-- removefiles {
			-- path.join(BGFX_DIR, "src/shader_dx9bc.cpp"),
			-- path.join(BGFX_DIR, "src/shader_dxbc.cpp"),
			-- path.join(BGFX_DIR, "src/shader_spirv.cpp"),
		-- }
	-- end

	filter { "configurations:Debug" }
		defines {
			"BGFX_CONFIG_DEBUG=1",
		}
	filter { "configurations:Release" }
		defines {
		}

	filter { "action:vs20* or action:mingw*" }
		links {
			"gdi32",
			"psapi",
		}
	filter { "action:vs2008" }
		includedirs {
			"$(DXSDK_DIR)/include",
		}
	AddDefaultFilters()


if bWithTools then

	project "shaderc"
		kind "ConsoleApp"
		language "C++"
		location (PROJECT_DIR)

		local GLSL_OPTIMIZER = path.join(BGFX_THIRD_PARTY_DIR, "glsl-optimizer")
		local FCPP_DIR = path.join(BGFX_THIRD_PARTY_DIR, "fcpp")
		
		defines {
			-- use built-in allocator
			"BGFX_CONFIG_USE_TINYSTL=0",

			"BGFX_CONFIG_RENDERER_DIRECT3D11=1",
			--"BGFX_CONFIG_RENDERER_OPENGL=1",

			-- temporary hack!
			--"BGFX_CONFIG_MULTITHREADED=0",
		}

		files {
			path.join(BGFX_DIR, "tools/shaderc/**.*"),
			path.join(BGFX_DIR, "src/vertexdecl.**"),

			path.join(FCPP_DIR, "**.h"),
			path.join(FCPP_DIR, "cpp1.c"),
			path.join(FCPP_DIR, "cpp2.c"),
			path.join(FCPP_DIR, "cpp3.c"),
			path.join(FCPP_DIR, "cpp4.c"),
			path.join(FCPP_DIR, "cpp5.c"),
			path.join(FCPP_DIR, "cpp6.c"),
			path.join(FCPP_DIR, "cpp6.c"),

			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/*.*"),
			path.join(GLSL_OPTIMIZER, "src/mesa/**.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/**.cpp"),
			path.join(GLSL_OPTIMIZER, "src/mesa/**.h"),
			path.join(GLSL_OPTIMIZER, "src/glsl/**.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/**.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/**.h"),
			path.join(GLSL_OPTIMIZER, "src/util/**.c"),
			path.join(GLSL_OPTIMIZER, "src/util/**.h"),
		}

		removefiles {
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/glcpp.c"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/tests/**"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.l"),
			path.join(GLSL_OPTIMIZER, "src/glsl/glcpp/**.y"),
			path.join(GLSL_OPTIMIZER, "src/glsl/ir_set_program_inouts.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/main.cpp"),
			path.join(GLSL_OPTIMIZER, "src/glsl/builtin_stubs.cpp"),
		}

		includedirs {
			FCPP_DIR,
			path.join(GLSL_OPTIMIZER, "src"),		
			path.join(GLSL_OPTIMIZER, "include"),
			path.join(GLSL_OPTIMIZER, "src/mesa"),
			path.join(GLSL_OPTIMIZER, "src/mapi"),
			path.join(GLSL_OPTIMIZER, "src/glsl"),
			-- bgfx distribution contains more up-to-date DirectX SDK headers
			path.join(BGFX_THIRD_PARTY_DIR, "dxsdk/include"),
			path.join(BGFX_DIR, "include"),
			path.join(BGFX_DIR, "tools/shaderc/"),
		}
		
		defines { -- fcpp
			"NINCLUDE=64",
			"NWORK=65536",
			"NBUFF=65536",
			"OLD_PREPROCESSOR=0",
		}

		filter { "action:vs20*" }
			includedirs {
				path.join(GLSL_OPTIMIZER, "src/glsl/msvc"),
			}
			defines { -- glsl-optimizer
				"__STDC__",
				"__STDC_VERSION__=199901L",
				"strdup=_strdup",
				"alloca=_alloca",
				"isascii=__isascii",
			}
			buildoptions {
				"/wd4996" -- warning C4996: 'strdup': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strdup.
			}
			links {
				"d3dcompiler",
			}
		
		filter { "configurations:Debug" }
			defines {
				"BGFX_CONFIG_DEBUG=1",
			}
		filter { "configurations:Release" }
			defines {
			}

		filter { "action:vs20* or action:mingw*" }
			links {
				"gdi32",
				"psapi",
			}
		filter { "action:vs2008" }
			includedirs {
				"$(DXSDK_DIR)/include",
			}

		AddDefaultFilters()
end

-------------------------------------------------------------------------------
-- SAMPLE FRAMEWORK
-------------------------------------------------------------------------------
project "bgfx-examples-common"
	kind "StaticLib"
	language "C++"
	location (PROJECT_DIR)

	includedirs {
		THIRD_PARTY_DIR,	-- e.g. <ib-compress/indexbufferdecompression.h>
		BGFX_THIRD_PARTY_DIR,	-- e.g. <ocornut-imgui/imgui.h>
		path.join(BGFX_DIR, "include"),
	}
	files {
		path.join(BGFX_DIR, "examples/common/**.h"),
		path.join(BGFX_DIR, "examples/common/**.cpp"),
		path.join(BGFX_THIRD_PARTY_DIR, "ib-compress/*.*"),
		path.join(BGFX_THIRD_PARTY_DIR, "ocornut-imgui/*.*"),
	}

	HACK_AddDependencies {
		"bgfx.lib",
		"Remotery.lib",
	}
	
	AddDefaultFilters()
