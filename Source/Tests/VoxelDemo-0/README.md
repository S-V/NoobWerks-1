# FPS Game

For russian gamedev contest:
https://gamedev.ru/projects/forum/?id=260833


## Checklist for release:

- Disable MX_DEBUG and MX_DEVELOPER
- Don't use text format for release assets


## TODO:
- Find only used assets

- particle rendering is very taxing (e.g. smoke puffs);
add "Particle Quality" to graphics settings tab
(highest = lit particles).


- Editor: additive vs subtractive: set background voxel material



## BUGS:

- alt-tab and back - weapon orientation changes => block UI when not in focus


## References:

Gibs:
https://developer.valvesoftware.com/wiki/Gibs
https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/gib.cpp

AI programming:
https://developer.valvesoftware.com/wiki/Category:AI

https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/server/ai_senses.h


Renderware AI **How it works** [Posted May 12, 2019]
https://gtaforums.com/topic/928687-renderware-ai-how-it-works/

AI Middleware: Getting Into Character, Part 3: RenderWare AI [2003]
https://www.gamasutra.com/view/feature/131232/ai_middleware_getting_into_.php










| Half-Assed: game settings.
|

window =
{
	fullscreen = 0

	width = 1024 height = 768	; aspect ratio ~ 1.3333
	;width = 1600 height = 1200	; aspect ratio ~ 1.3333
	;width = 1600 height = 900	; aspect ratio ~ 1.7777
	;width = 1365 height = 768	; aspect ratio ~ 1.7777
	;width = 1820 height = 1024	; aspect ratio ~ 1.7777
	;width = 1920 height = 1080	; aspect ratio ~ 1.7777
	
	offset_x = 0
	offset_y = 0
	resizable = 0
}
