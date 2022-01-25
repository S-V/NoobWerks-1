name = 'procedural_starfield'
entry = {
	file = 'procedural_starfield.cxx'
	vertex = 'FullScreenTriangle_VS'
	pixel = 'PixelShaderMain'
}

not_reflected_uniform_buffers = [
	'cbStarfield'
]

scene_passes = [
	{
		name = 'Unknown'
	}
]
