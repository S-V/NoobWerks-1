// hardcoded set of all possible actions that can be executed in the game

//DECLARE_GAME_ACTION( name, description )
#ifdef DECLARE_GAME_ACTION

DECLARE_GAME_ACTION(NONE, "")

DECLARE_GAME_ACTION(move_forward, "")
DECLARE_GAME_ACTION(move_back, "")
DECLARE_GAME_ACTION(move_left, "")
DECLARE_GAME_ACTION(move_right, "")
DECLARE_GAME_ACTION(move_up, "")
DECLARE_GAME_ACTION(move_down, "")

DECLARE_GAME_ACTION(rotate_yaw, "")
DECLARE_GAME_ACTION(rotate_pitch, "")

DECLARE_GAME_ACTION(test, "for testing/debugging")

/*
DECLARE_GAME_ACTION(moveleft, "")
DECLARE_GAME_ACTION(moveright, "")
DECLARE_GAME_ACTION(moveforward, "")
DECLARE_GAME_ACTION(moveback, "")
DECLARE_GAME_ACTION(jump, "")
DECLARE_GAME_ACTION(crouch, "")
DECLARE_GAME_ACTION(prone, "")
DECLARE_GAME_ACTION(togglestance, "")
DECLARE_GAME_ACTION(sprint, "")
DECLARE_GAME_ACTION(special, "")
DECLARE_GAME_ACTION(leanleft, "")
DECLARE_GAME_ACTION(leanright, "")

DECLARE_GAME_ACTION(nextspawnpoint, "")
DECLARE_GAME_ACTION(flymode, "")
DECLARE_GAME_ACTION(godmode, "")
DECLARE_GAME_ACTION(toggleaidebugdraw, "")
DECLARE_GAME_ACTION(togglepdrawhelpers, "")
DECLARE_GAME_ACTION(toggle_airstrike, "")

DECLARE_GAME_ACTION(cycle_spectator_mode, "")
DECLARE_GAME_ACTION(prev_spectator_target, "")
DECLARE_GAME_ACTION(next_spectator_target, "")

DECLARE_GAME_ACTION(ulammo, "")
DECLARE_GAME_ACTION(giveitems, "")
*/
DECLARE_GAME_ACTION(attack1, "")	// primary fire
DECLARE_GAME_ACTION(attack2, "")	// secondary fire
DECLARE_GAME_ACTION(attack3, "")	// alternative fire
DECLARE_GAME_ACTION(reload, "")

DECLARE_GAME_ACTION(drop_item, "")
/*
DECLARE_GAME_ACTION(modify, "")
DECLARE_GAME_ACTION(additem, "")
DECLARE_GAME_ACTION(nextitem, "")
DECLARE_GAME_ACTION(previtem, "")
DECLARE_GAME_ACTION(small, "")
DECLARE_GAME_ACTION(medium, "")
DECLARE_GAME_ACTION(heavy, "")
DECLARE_GAME_ACTION(explosive, "")
DECLARE_GAME_ACTION(handgrenade, "")
DECLARE_GAME_ACTION(xi_handgrenade, "")
DECLARE_GAME_ACTION(holsteritem, "")

DECLARE_GAME_ACTION(utility, "")
DECLARE_GAME_ACTION(debug, "")
DECLARE_GAME_ACTION(zoom, "")
DECLARE_GAME_ACTION(firemode, "")
DECLARE_GAME_ACTION(binoculars, "")
DECLARE_GAME_ACTION(objectives, "")
DECLARE_GAME_ACTION(grenade, "")
DECLARE_GAME_ACTION(xi_grenade, "")

DECLARE_GAME_ACTION(speedmode, "")
DECLARE_GAME_ACTION(strengthmode, "")
DECLARE_GAME_ACTION(defensemode, "")

DECLARE_GAME_ACTION(zoom_in, "")
DECLARE_GAME_ACTION(zoom_out, "")

DECLARE_GAME_ACTION(invert_mouse, "")

DECLARE_GAME_ACTION(thirdperson, "")
DECLARE_GAME_ACTION(use, "")
DECLARE_GAME_ACTION(xi_use, "")

DECLARE_GAME_ACTION(horn, "")
DECLARE_GAME_ACTION(gyroscope, "")
DECLARE_GAME_ACTION(gboots, "")
DECLARE_GAME_ACTION(lights, "")

DECLARE_GAME_ACTION(radio_group_0, "")
DECLARE_GAME_ACTION(radio_group_1, "")
DECLARE_GAME_ACTION(radio_group_2, "")
DECLARE_GAME_ACTION(radio_group_3, "")
DECLARE_GAME_ACTION(radio_group_4, "")

DECLARE_GAME_ACTION(voice_chat_talk, "")

DECLARE_GAME_ACTION(save, "")
DECLARE_GAME_ACTION(load, "")

DECLARE_GAME_ACTION(ai_goto, "")
DECLARE_GAME_ACTION(ai_follow, "")

	// XInput specific actions
DECLARE_GAME_ACTION(xi_zoom, "")
DECLARE_GAME_ACTION(xi_binoculars, "")
DECLARE_GAME_ACTION(xi_movex, "")
DECLARE_GAME_ACTION(xi_movey, "")
DECLARE_GAME_ACTION(xi_disconnect, "")
DECLARE_GAME_ACTION(xi_rotateyaw, "")
DECLARE_GAME_ACTION(xi_rotatepitch, "")
DECLARE_GAME_ACTION(xi_v_rotateyaw, "")
DECLARE_GAME_ACTION(xi_v_rotatepitch, "")


// Vehicle
DECLARE_GAME_ACTION(v_changeseat1, "")
DECLARE_GAME_ACTION(v_changeseat2, "")
DECLARE_GAME_ACTION(v_changeseat3, "")
DECLARE_GAME_ACTION(v_changeseat4, "")
DECLARE_GAME_ACTION(v_changeseat5, "")

DECLARE_GAME_ACTION(v_changeview, "")
DECLARE_GAME_ACTION(v_viewoption, "")
DECLARE_GAME_ACTION(v_zoom_in, "")
DECLARE_GAME_ACTION(v_zoom_out, "")

DECLARE_GAME_ACTION(v_attack1, "")
DECLARE_GAME_ACTION(v_attack2, "")
DECLARE_GAME_ACTION(v_firemode, "")
DECLARE_GAME_ACTION(v_lights, "")
DECLARE_GAME_ACTION(v_horn, "")
DECLARE_GAME_ACTION(v_exit, "")

DECLARE_GAME_ACTION(v_rotateyaw, "")
DECLARE_GAME_ACTION(v_rotatepitch, "")

DECLARE_GAME_ACTION(v_moveforward, "")
DECLARE_GAME_ACTION(v_moveback, "")
DECLARE_GAME_ACTION(v_moveup, "")
DECLARE_GAME_ACTION(v_movedown, "")
DECLARE_GAME_ACTION(v_rotatedir, "")
DECLARE_GAME_ACTION(v_turnleft, "")
DECLARE_GAME_ACTION(v_turnright, "")
DECLARE_GAME_ACTION(v_strafeleft, "")
DECLARE_GAME_ACTION(v_straferight, "")
DECLARE_GAME_ACTION(v_rollleft, "")
DECLARE_GAME_ACTION(v_rollright, "")

DECLARE_GAME_ACTION(v_pitchup, "")
DECLARE_GAME_ACTION(v_pitchdown, "")

DECLARE_GAME_ACTION(v_brake, "")
DECLARE_GAME_ACTION(v_afterburner, "")
DECLARE_GAME_ACTION(v_boost, "")
DECLARE_GAME_ACTION(v_changeseat, "")

DECLARE_GAME_ACTION(v_debug_1, "")
DECLARE_GAME_ACTION(v_debug_2, "")
DECLARE_GAME_ACTION(buyammo, "")
DECLARE_GAME_ACTION(loadLastSave, "")

DECLARE_GAME_ACTION(debug_ag_step, "")

DECLARE_GAME_ACTION(scores, "")

// UI
DECLARE_GAME_ACTION(ui_toggle_pause, "")
DECLARE_GAME_ACTION(ui_up, "")
DECLARE_GAME_ACTION(ui_down, "")
DECLARE_GAME_ACTION(ui_left, "")
DECLARE_GAME_ACTION(ui_right, "")
DECLARE_GAME_ACTION(ui_click, "")
DECLARE_GAME_ACTION(ui_back, "")
DECLARE_GAME_ACTION(ui_skip_video, "")
*/

DECLARE_GAME_ACTION(take_screenshot, "")

// Development

// execute the given callback: "void (*) (StateMan*)"
DECLARE_GAME_ACTION(_execute_script_command, "")

/*
DECLARE_GAME_ACTION(_restore_previous_state, "")	// pop off the menu stack
DECLARE_GAME_ACTION(dev_toggle_console, "")	// toggle Quake-style console
DECLARE_GAME_ACTION(dev_toggle_menu, "")// toggle debug UI menu
*/

// e.g. reset the camera position and look direction to default values
DECLARE_GAME_ACTION(dev_reset_state, "")

// e.g. save/load demo settings
DECLARE_GAME_ACTION(dev_save_state, "")
DECLARE_GAME_ACTION(dev_load_state, "")

DECLARE_GAME_ACTION(dev_cast_ray, "")

DECLARE_GAME_ACTION(dev_reload_shaders, "")

DECLARE_GAME_ACTION(dev_action0, "")
DECLARE_GAME_ACTION(dev_action1, "")
DECLARE_GAME_ACTION(dev_action2, "")
DECLARE_GAME_ACTION(dev_action3, "")
DECLARE_GAME_ACTION(dev_action4, "")
DECLARE_GAME_ACTION(dev_action5, "")
DECLARE_GAME_ACTION(dev_action6, "")
DECLARE_GAME_ACTION(dev_action7, "")
DECLARE_GAME_ACTION(dev_action8, "")
DECLARE_GAME_ACTION(dev_action9, "")

// often used to turn off debug GUI for making nice screenshots
DECLARE_GAME_ACTION(dev_toggle_gui, "Toggle GUI")

DECLARE_GAME_ACTION(dev_toggle_wireframe, "")


// In-game editor

DECLARE_GAME_ACTION(editor_change_tool, "Change Tool")

// depends on the current tool
DECLARE_GAME_ACTION(execute_action, "")

// e.g. select voxel
//DECLARE_GAME_ACTION(editor_select, "select")
//DECLARE_GAME_ACTION(editor_csg_add, "CSG add")
//DECLARE_GAME_ACTION(editor_csg_subtract, "CSG subtract")
//DECLARE_GAME_ACTION(editor_place_mesh, "Place mesh")

// Debug

// Miscellaneous
DECLARE_GAME_ACTION(exit_app, "request exit")

#endif // DECLARE_GAME_ACTION
