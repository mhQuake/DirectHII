// cl.input.c  -- builds an intended movement command to send to the server

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook, in_klook;
kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed, in_use, in_jump, in_attack;
kbutton_t	in_up, in_down, in_crouch;
kbutton_t	in_infoplaque;

int					in_impulse;
extern qboolean		info_up;

void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;

	c = Cmd_Argv (1);

	if (c[0])
		k = atoi (c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Con_Printf (PRINT_NORMAL, "Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;		// still down

	b->state |= 1 + 2;	// down + impulse down
}

void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;

	c = Cmd_Argv (1);

	if (c[0])
		k = atoi (c);
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)

	if (b->down[0] || b->down[1])
		return;		// some other key is still holding it down

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

void IN_KLookDown (void) { KeyDown (&in_klook); }
void IN_KLookUp (void) { KeyUp (&in_klook); }
void IN_MLookDown (void) { KeyDown (&in_mlook); }
void IN_MLookUp (void) { KeyUp (&in_mlook); }
void IN_UpDown (void) { KeyDown (&in_up); }
void IN_UpUp (void) { KeyUp (&in_up); }
void IN_DownDown (void) { KeyDown (&in_down); }
void IN_DownUp (void) { KeyUp (&in_down); }
void IN_LeftDown (void) { KeyDown (&in_left); }
void IN_LeftUp (void) { KeyUp (&in_left); }
void IN_RightDown (void) { KeyDown (&in_right); }
void IN_RightUp (void) { KeyUp (&in_right); }
void IN_ForwardDown (void) { KeyDown (&in_forward); }
void IN_ForwardUp (void) { KeyUp (&in_forward); }
void IN_BackDown (void) { KeyDown (&in_back); }
void IN_BackUp (void) { KeyUp (&in_back); }
void IN_LookupDown (void) { KeyDown (&in_lookup); }
void IN_LookupUp (void) { KeyUp (&in_lookup); }
void IN_LookdownDown (void) { KeyDown (&in_lookdown); }
void IN_LookdownUp (void) { KeyUp (&in_lookdown); }
void IN_MoveleftDown (void) { KeyDown (&in_moveleft); }
void IN_MoveleftUp (void) { KeyUp (&in_moveleft); }
void IN_MoverightDown (void) { KeyDown (&in_moveright); }
void IN_MoverightUp (void) { KeyUp (&in_moveright); }

void IN_SpeedDown (void) { KeyDown (&in_speed); }
void IN_SpeedUp (void) { KeyUp (&in_speed); }
void IN_StrafeDown (void) { KeyDown (&in_strafe); }
void IN_StrafeUp (void) { KeyUp (&in_strafe); }

void IN_AttackDown (void) { KeyDown (&in_attack); }
void IN_AttackUp (void) { KeyUp (&in_attack); }

void IN_UseDown (void) { KeyDown (&in_use); }
void IN_UseUp (void) { KeyUp (&in_use); }
void IN_JumpDown (void) { KeyDown (&in_jump); }
void IN_JumpUp (void) { KeyUp (&in_jump); }

void IN_Impulse (void) { in_impulse = atoi (Cmd_Argv (1)); }

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val;
	qboolean	impulsedown, impulseup, down;

	impulsedown = key->state & 2;
	impulseup = key->state & 4;
	down = key->state & 1;
	val = 0;

	if (impulsedown && !impulseup)
		if (down)
			val = 0.5;	// pressed and held this frame
		else
			val = 0;	//	I_Error ();

	if (impulseup && !impulsedown)
		if (down)
			val = 0;	//	I_Error ();
		else
			val = 0;	// released this frame

	if (!impulsedown && !impulseup)
		if (down)
			val = 1.0;	// held the entire frame
		else
			val = 0;	// up the entire frame

	if (impulsedown && impulseup)
		if (down)
			val = 0.75;	// released and re-pressed this frame
		else
			val = 0.25;	// pressed and released this frame

	key->state &= 1;		// clear impulses

	return val;
}




//==========================================================================

cvar_t	cl_upspeed = {"cl_upspeed", "200"};
cvar_t	cl_forwardspeed = {"cl_forwardspeed", "200", true};
cvar_t	cl_backspeed = {"cl_backspeed", "200", true};
//cvar_t	cl_sidespeed = {"cl_sidespeed","350"};
cvar_t	cl_sidespeed = {"cl_sidespeed", "225"};

cvar_t	cl_movespeedkey = {"cl_movespeedkey", "2.0"};

cvar_t	cl_yawspeed = {"cl_yawspeed", "140"};
cvar_t	cl_pitchspeed = {"cl_pitchspeed", "150"};

cvar_t	cl_anglespeedkey = {"cl_anglespeedkey", "1.5"};

cvar_t	cl_prettylights = {"cl_prettylights", "1"};

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (in_speed.state & 1)
		speed = (cl.time - cl.oldtime) * cl_anglespeedkey.value;
	else
		speed = (cl.time - cl.oldtime);

	if (!(in_strafe.state & 1))
	{
		cl.viewangles[1] -= speed * cl_yawspeed.value * CL_KeyState (&in_right);
		cl.viewangles[1] += speed * cl_yawspeed.value * CL_KeyState (&in_left);
		cl.viewangles[1] = anglemod (cl.viewangles[1]);
	}

	if (in_klook.state & 1)
	{
		cl.viewangles[0] -= speed * cl_pitchspeed.value * CL_KeyState (&in_forward);
		cl.viewangles[0] += speed * cl_pitchspeed.value * CL_KeyState (&in_back);
	}

	up = CL_KeyState (&in_lookup);
	down = CL_KeyState (&in_lookdown);

	cl.viewangles[0] -= speed * cl_pitchspeed.value * up;
	cl.viewangles[0] += speed * cl_pitchspeed.value * down;

	if (cl.viewangles[0] > 80)
		cl.viewangles[0] = 80;

	if (cl.viewangles[0] < -70)
		cl.viewangles[0] = -70;

	if (cl.viewangles[2] > 50)
		cl.viewangles[2] = 50;

	if (cl.viewangles[2] < -50)
		cl.viewangles[2] = -50;
}


/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
void CL_BaseMove (usercmd_t *cmd)
{
	if (cls.signon != SIGNONS)
		return;

	if (cl.v.cameramode)	// Stuck in a different camera so don't move
	{
		memset (cmd, 0, sizeof (*cmd));
		return;
	}

	CL_AdjustAngles ();

	memset (cmd, 0, sizeof (*cmd));

	if (in_strafe.state & 1)
	{
		//		cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_right);
		//		cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_left);
		cmd->sidemove += 225 * CL_KeyState (&in_right);
		cmd->sidemove -= 225 * CL_KeyState (&in_left);
	}

	//	cmd->sidemove += cl_sidespeed.value * CL_KeyState (&in_moveright);
	//	cmd->sidemove -= cl_sidespeed.value * CL_KeyState (&in_moveleft);
	cmd->sidemove += 225 * CL_KeyState (&in_moveright);
	cmd->sidemove -= 225 * CL_KeyState (&in_moveleft);

	cmd->upmove += cl_upspeed.value * CL_KeyState (&in_up);
	cmd->upmove -= cl_upspeed.value * CL_KeyState (&in_down);

	if (!(in_klook.state & 1))
	{
		//		cmd->forwardmove += cl_forwardspeed.value * CL_KeyState (&in_forward);
		cmd->forwardmove += 200 * CL_KeyState (&in_forward);
		//		cmd->forwardmove -= cl_backspeed.value * CL_KeyState (&in_back);
		cmd->forwardmove -= 200 * CL_KeyState (&in_back);
	}

	// adjust for speed key (but not if always runs has been chosen)
	if ((cl_forwardspeed.value > 200 || in_speed.state & 1) && cl.v.hasted <= 1)
	{
		cmd->forwardmove *= cl_movespeedkey.value;
		cmd->sidemove *= cl_movespeedkey.value;
		cmd->upmove *= cl_movespeedkey.value;
	}

	// Hasted player?
	if (cl.v.hasted)
	{
		cmd->forwardmove = cmd->forwardmove * cl.v.hasted;
		cmd->sidemove = cmd->sidemove * cl.v.hasted;
		cmd->upmove = cmd->upmove * cl.v.hasted;
	}

#ifdef QUAKE2RJ
	cmd->lightlevel = cl.light_level;
#endif
}



/*
==============
CL_SendMove
==============
*/
void CL_SendMove (usercmd_t *cmd)
{
	int		i;
	int		bits;
	sizebuf_t	buf;
	byte	data[1024];

	buf.maxsize = 128;
	buf.cursize = 0;
	buf.data = data;

	cl.cmd = *cmd;

	// send the movement message
	MSG_WriteByte (&buf, clc_frame);
	MSG_WriteByte (&buf, cl.reference_frame);
	MSG_WriteByte (&buf, cl.current_sequence);

	MSG_WriteByte (&buf, clc_move);

	MSG_WriteFloat (&buf, cl.mtime[0]);	// so server can get ping times

	if (cl.Protocol == PROTOCOL_VERSION2)
	{
		for (i = 0; i < 3; i++)
			MSG_WriteFloat (&buf, cl.viewangles[i]);

		MSG_WriteFloat (&buf, cmd->forwardmove);
		MSG_WriteFloat (&buf, cmd->sidemove);
		MSG_WriteFloat (&buf, cmd->upmove);
	}
	else
	{
		for (i = 0; i < 3; i++)
			MSG_WriteAngle (&buf, cl.viewangles[i]);

		MSG_WriteShort (&buf, cmd->forwardmove);
		MSG_WriteShort (&buf, cmd->sidemove);
		MSG_WriteShort (&buf, cmd->upmove);
	}

	// send button bits
	bits = 0;

	if (in_attack.state & 3)
		bits |= 1;

	in_attack.state &= ~2;

	if (in_jump.state & 3)
		bits |= 2;

	in_jump.state &= ~2;

	if (in_crouch.state & 1)
		bits |= 4;

	MSG_WriteByte (&buf, bits);

	MSG_WriteByte (&buf, in_impulse);
	in_impulse = 0;

#ifdef QUAKE2RJ
	// light level
	MSG_WriteByte (&buf, cmd->lightlevel);
#endif

	// deliver the message
	if (cls.demoplayback)
		return;

	// allways dump the first two message, because it may contain leftover inputs
	// from the last level
	if (++cl.movemessages <= 2)
		return;

	if (NET_SendUnreliableMessage (cls.netcon, &buf) == -1)
	{
		Con_Printf (PRINT_NORMAL, "CL_SendMove: lost server connection\n");
		CL_Disconnect ();
	}
}

void IN_CrouchDown (void)
{
	int state;

	if (key_dest == key_game)
	{
		state = in_crouch.state;
		KeyDown (&in_crouch);

		//		if (!(state & 1) && (in_crouch.state & 1))
		//			in_impulse = 22;
	}
}

void IN_CrouchUp (void)
{
	int state;

	if (key_dest == key_game)
	{
		state = in_crouch.state;

		KeyUp (&in_crouch);
		//		if ((state & 1) && !(in_crouch.state & 1))
		//			in_impulse = 22;
	}
}

void IN_infoPlaqueUp (void)
{
	if (key_dest == key_game)
	{
		//They want to lower the plaque
		info_up = 0;
		KeyUp (&in_infoplaque);
	}
}

void IN_infoPlaqueDown (void)
{
	if (key_dest == key_game)
	{
		//They want to see the plaque
		info_up = 1;
		KeyDown (&in_infoplaque);
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	Cmd_AddCommand ("+moveup", IN_UpDown);
	Cmd_AddCommand ("-moveup", IN_UpUp);
	Cmd_AddCommand ("+movedown", IN_DownDown);
	Cmd_AddCommand ("-movedown", IN_DownUp);
	Cmd_AddCommand ("+left", IN_LeftDown);
	Cmd_AddCommand ("-left", IN_LeftUp);
	Cmd_AddCommand ("+right", IN_RightDown);
	Cmd_AddCommand ("-right", IN_RightUp);
	Cmd_AddCommand ("+forward", IN_ForwardDown);
	Cmd_AddCommand ("-forward", IN_ForwardUp);
	Cmd_AddCommand ("+back", IN_BackDown);
	Cmd_AddCommand ("-back", IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_AttackDown);
	Cmd_AddCommand ("-attack", IN_AttackUp);
	Cmd_AddCommand ("+use", IN_UseDown);
	Cmd_AddCommand ("-use", IN_UseUp);
	Cmd_AddCommand ("+jump", IN_JumpDown);
	Cmd_AddCommand ("-jump", IN_JumpUp);
	Cmd_AddCommand ("impulse", IN_Impulse);
	Cmd_AddCommand ("+klook", IN_KLookDown);
	Cmd_AddCommand ("-klook", IN_KLookUp);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	Cmd_AddCommand ("+crouch", IN_CrouchDown);
	Cmd_AddCommand ("-crouch", IN_CrouchUp);

	Cmd_AddCommand ("+infoplaque", IN_infoPlaqueDown);
	Cmd_AddCommand ("-infoplaque", IN_infoPlaqueUp);
}


/*
============
CL_ClearStates
============
*/
void CL_ClearStates (void)
{
	in_mlook.state = 0;
	in_klook.state = 0;
	in_left.state = 0;
	in_right.state = 0;
	in_forward.state = 0;
	in_back.state = 0;
	in_lookup.state = 0;
	in_lookdown.state = 0;
	in_moveleft.state = 0;
	in_moveright.state = 0;
	in_strafe.state = 0;
	in_speed.state = 0;
	in_use.state = 0;
	in_jump.state = 0;
	in_attack.state = 0;
	in_up.state = 0;
	in_down.state = 0;
	in_crouch.state = 0;
}
