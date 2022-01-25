// mxDEFINE_KEYCODE( name), value )
#ifdef mxDEFINE_KEYCODE

mxDEFINE_KEYCODE( Unknown ),

mxDEFINE_KEYCODE( Escape	),
mxDEFINE_KEYCODE( Backspace	),
mxDEFINE_KEYCODE( Tab	),
mxDEFINE_KEYCODE( Enter	),
mxDEFINE_KEYCODE( Space	),

mxDEFINE_KEYCODE( LShift	),
mxDEFINE_KEYCODE( RShift	),
mxDEFINE_KEYCODE( LCtrl	),
mxDEFINE_KEYCODE( RCtrl	),
mxDEFINE_KEYCODE( LAlt	),
mxDEFINE_KEYCODE( RAlt	),

mxDEFINE_KEYCODE( LWin	),
mxDEFINE_KEYCODE( RWin	),
mxDEFINE_KEYCODE( Apps	),

mxDEFINE_KEYCODE( Pause	),
mxDEFINE_KEYCODE( Capslock	),
mxDEFINE_KEYCODE( Numlock	),
mxDEFINE_KEYCODE( Scrolllock	),
mxDEFINE_KEYCODE( PrintScreen	),

mxDEFINE_KEYCODE( PgUp	),
mxDEFINE_KEYCODE( PgDn	),
mxDEFINE_KEYCODE( Home	),
mxDEFINE_KEYCODE( End	),
mxDEFINE_KEYCODE( Insert	),
mxDEFINE_KEYCODE( Delete	),

mxDEFINE_KEYCODE( Left	),
mxDEFINE_KEYCODE( Up	),
mxDEFINE_KEYCODE( Right	),
mxDEFINE_KEYCODE( Down	),

// Alphanumerical keys
mxDEFINE_KEYCODE( 0	),
mxDEFINE_KEYCODE( 1	),
mxDEFINE_KEYCODE( 2	),
mxDEFINE_KEYCODE( 3	),
mxDEFINE_KEYCODE( 4	),
mxDEFINE_KEYCODE( 5	),
mxDEFINE_KEYCODE( 6	),
mxDEFINE_KEYCODE( 7	),
mxDEFINE_KEYCODE( 8	),
mxDEFINE_KEYCODE( 9	),

mxDEFINE_KEYCODE( A	),
mxDEFINE_KEYCODE( B	),
mxDEFINE_KEYCODE( C	),
mxDEFINE_KEYCODE( D	),
mxDEFINE_KEYCODE( E	),
mxDEFINE_KEYCODE( F	),
mxDEFINE_KEYCODE( G	),
mxDEFINE_KEYCODE( H	),
mxDEFINE_KEYCODE( I	),
mxDEFINE_KEYCODE( J	),
mxDEFINE_KEYCODE( K	),
mxDEFINE_KEYCODE( L	),
mxDEFINE_KEYCODE( M	),
mxDEFINE_KEYCODE( N	),
mxDEFINE_KEYCODE( O	),
mxDEFINE_KEYCODE( P	),
mxDEFINE_KEYCODE( Q	),
mxDEFINE_KEYCODE( R	),
mxDEFINE_KEYCODE( S	),
mxDEFINE_KEYCODE( T	),
mxDEFINE_KEYCODE( U	),
mxDEFINE_KEYCODE( V	),
mxDEFINE_KEYCODE( W	),
mxDEFINE_KEYCODE( X	),
mxDEFINE_KEYCODE( Y	),
mxDEFINE_KEYCODE( Z	),

// Special keys

// This is usually the button immediately above the TAB on most keyboards,
// also called the back-tick button, or tilde
mxDEFINE_KEYCODE( Grave	),

mxDEFINE_KEYCODE( Backslash	),
mxDEFINE_KEYCODE( LBracket	),
mxDEFINE_KEYCODE( RBracket	),
mxDEFINE_KEYCODE( Semicolon	),
mxDEFINE_KEYCODE( Apostrophe	),
mxDEFINE_KEYCODE( Comma	),
mxDEFINE_KEYCODE( Period	),
mxDEFINE_KEYCODE( Slash	),	// '/'
mxDEFINE_KEYCODE( Plus	),
mxDEFINE_KEYCODE( Minus	),
mxDEFINE_KEYCODE( Asterisk	),	// '*'
mxDEFINE_KEYCODE( Equals	),

mxDEFINE_KEYCODE( Numpad0	),
mxDEFINE_KEYCODE( Numpad1	),
mxDEFINE_KEYCODE( Numpad2	),
mxDEFINE_KEYCODE( Numpad3	),
mxDEFINE_KEYCODE( Numpad4	),
mxDEFINE_KEYCODE( Numpad5	),
mxDEFINE_KEYCODE( Numpad6	),
mxDEFINE_KEYCODE( Numpad7	),
mxDEFINE_KEYCODE( Numpad8	),
mxDEFINE_KEYCODE( Numpad9	),

/*
mxDEFINE_KEYCODE( Multiply	),
mxDEFINE_KEYCODE( Divide	),
mxDEFINE_KEYCODE( Plus	),
mxDEFINE_KEYCODE( Subtract	),
mxDEFINE_KEYCODE( Decimal	),
*/

// Function keys
mxDEFINE_KEYCODE( F1	),
mxDEFINE_KEYCODE( F2	),
mxDEFINE_KEYCODE( F3	),
mxDEFINE_KEYCODE( F4	),
mxDEFINE_KEYCODE( F5	),
mxDEFINE_KEYCODE( F6	),
mxDEFINE_KEYCODE( F7	),
mxDEFINE_KEYCODE( F8	),
mxDEFINE_KEYCODE( F9	),
mxDEFINE_KEYCODE( F10	),
mxDEFINE_KEYCODE( F11	),
mxDEFINE_KEYCODE( F12	),

//
// mouse buttons generate virtual keys
//
mxDEFINE_KEYCODE( MOUSE0	),	// Left mouse button
mxDEFINE_KEYCODE( MOUSE1	),	// Right mouse button
mxDEFINE_KEYCODE( MOUSE2	),	// Middle mouse button
mxDEFINE_KEYCODE( MOUSE3	),	// Primary mouse thumb button.
mxDEFINE_KEYCODE( MOUSE4	),	// Secondary mouse thumb button.

mxDEFINE_KEYCODE( MouseAxisX	),	// Mouse movement on the X axis.
mxDEFINE_KEYCODE( MouseAxisY	),	// Mouse movement on the Y axis.

mxDEFINE_KEYCODE( MWHEELDOWN	),	// Mouse wheel scrolling down.
mxDEFINE_KEYCODE( MWHEELUP		),	// Mouse wheel scrolling up.

//
// joystick buttons
//
mxDEFINE_KEYCODE( JOY1	),
mxDEFINE_KEYCODE( JOY2	),
mxDEFINE_KEYCODE( JOY3	),
mxDEFINE_KEYCODE( JOY4	),

//
// aux keys are for multi-buttoned joysticks to generate
// so they can use the normal binding process
//

#endif // mxDEFINE_KEYCODE
