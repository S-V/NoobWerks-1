if MakeStaticLibrary( "Graphics", "" ) then
	links {
		"Base",
		"Core",
		"GPU",
	}
end
