
if MakeStaticLibrary( "Utility" ) then
	files {
		"**.h", "**.cpp",
		"**.md",
	}
	links {
		"ImGuizmo",
	}
end

--[[
if MakeStaticLibrary( "Animation", "Animation" ) then
	links {
		"Base",
		"Renderer",
	}
end

if MakeStaticLibrary( "Compression", "Compression" ) then
	links {
		"Base",
	}
end

if MakeStaticLibrary( "DemoFramework", "DemoFramework" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Renderer",
	}
end

if MakeStaticLibrary( "GameFramework", "GameFramework" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Renderer",
	}
end


if MakeStaticLibrary( "glTF", "glTF" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Renderer",
	}
end


if MakeStaticLibrary( "GUI", "GUI" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Renderer",
	}
end


if MakeStaticLibrary( "Image", "Image" ) then
	links {
		"Base",
		"Core",
		"Graphics",
		"Renderer",
	}
end

if MakeStaticLibrary( "Implicit", "Implicit" ) then
	links {
		"Base",
		"Core",
	}
end

if MakeStaticLibrary( "MeshLib", "MeshLib" ) then
	links {
		"Base",
		"Core",
	}
end

if MakeStaticLibrary( "Meshok", "Meshok" ) then
	links {
		"Base",
		"Core",
	}
end


if MakeStaticLibrary( "VoxelEngine", "VoxelEngine" ) then
	links {
		"Base",
		"Core",
	}
end

]]
