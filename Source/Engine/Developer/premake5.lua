if MakeStaticLibrary( "Developer" ) then
	files {
		"**.h", "**.cpp",
		"**.md",
	}
	links {
		"efsw",	-- File watcher
	}
end
