
#include "precompiled.h"

#include "quakedef.h"
#include "winquake.h"

void WriteCommands (FILE *FH);
void SV_UpdateEffects (sizebuf_t *sb);
int LoadGamestate (char *level, char *startspot, int ClientsMode);
qboolean CL_CopyFiles (char *source, char *pat, char *dest);
void CL_RemoveGIPFiles (char *path);
void R_ClearParticles (void);

extern cvar_t	pausable;
extern	cvar_t	sv_flypitch;
extern	cvar_t	sv_walkpitch;

int	current_skill;
static double		old_time;

void SaveGamestate (qboolean ClientsOnly);
void RestoreClients (void);

UINT	info_mask, info_mask2;

#define TESTSAVE

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f (void);

void Host_Quit_f (void)
{
	if (key_dest != key_console)
	{
		M_Menu_Quit_f ();
		return;
	}

	CL_Disconnect ();
	Host_ShutdownServer (false);

	Sys_Quit ();
}


/*
==================
Host_Status_f
==================
*/
void Host_Printf (char *msg)
{
	Con_Printf (PRINT_NORMAL, msg);
}


void Host_Status_f (void)
{
	client_t	*client;
	int			seconds;
	int			minutes;
	int			hours = 0;
	int			j;
	void (*print) (char *fmt);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer ();
			return;
		}

		print = Host_Printf;
	}
	else
		print = SV_ClientPrintf;

	print (va ("host:    %s\n", Cvar_VariableString ("hostname")));
	print (va ("version: %4.2f\n", HEXEN2_VERSION));

	if (tcpipAvailable)
		print (va ("tcp/ip:  %s\n", my_tcpip_address));

	if (ipxAvailable)
		print (va ("ipx:     %s\n", my_ipx_address));

	print (va ("map:     %s\n", sv.name));
	print (va ("players: %i active (%i max)\n\n", net_activeconnections, svs.maxclients));

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active)
			continue;

		seconds = (int) (net_time - client->netconnection->connecttime);
		minutes = seconds / 60;

		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;

			if (hours)
				minutes -= (hours * 60);
		}
		else
			hours = 0;

		print (va ("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int) client->edict->v.frags, hours, minutes, seconds));
		print (va ("   %s\n", client->netconnection->address));
	}
}


/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if ((pr_global_struct->deathmatch ||
		pr_global_struct->coop || skill.value > 2) && !host_client->privileged)
		return;

	sv_player->v.flags = (int) sv_player->v.flags ^ FL_GODMODE;

	if (!((int) sv_player->v.flags & FL_GODMODE))
		SV_ClientPrintf ("godmode OFF\n");
	else
		SV_ClientPrintf ("godmode ON\n");
}

void Host_Notarget_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if ((pr_global_struct->deathmatch || skill.value > 2) && !host_client->privileged)
		return;

	sv_player->v.flags = (int) sv_player->v.flags ^ FL_NOTARGET;

	if (!((int) sv_player->v.flags & FL_NOTARGET))
		SV_ClientPrintf ("notarget OFF\n");
	else
		SV_ClientPrintf ("notarget ON\n");
}

qboolean noclip_anglehack;

void Host_Noclip_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if ((pr_global_struct->deathmatch ||
		pr_global_struct->coop || skill.value > 2) && !host_client->privileged)
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		noclip_anglehack = true;
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf ("noclip ON\n");
	}
	else
	{
		noclip_anglehack = false;
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf ("noclip OFF\n");
	}
}


/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f (void)
{
	int		i, j;
	float	total;
	client_t	*client;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	SV_ClientPrintf ("Client ping times:\n");

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		if (!client->active)
			continue;

		total = 0;

		for (j = 0; j < NUM_PING_TIMES; j++)
			total += client->ping_times[j];

		total /= NUM_PING_TIMES;
		SV_ClientPrintf (va ("%4i %s\n", (int) (total * 1000), client->name));
	}
}

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/


/*
======================
Host_Map_f

handle a
map <servername>
command from the console.  Active clients are kicked off.
======================
*/
void Host_Map_f (void)
{
	int		i;
	char	name[MAX_QPATH];

	if (Cmd_Argc () < 2)	//no map name given
	{
		Con_Printf (PRINT_NORMAL, va ("map <levelname>: start a new server\nCurrently on: %s\n", cl.levelname));
		Con_Printf (PRINT_NORMAL, va ("%s\n", cls.mapstring));
		return;
	}

	if (cmd_source != src_command)
		return;

	cls.demonum = -1;		// stop demo loop in case this fails

	CL_Disconnect ();
	Host_ShutdownServer (false);

	key_dest = key_game;			// remove console or menu
	SCR_BeginLoadingPlaque ();

	info_mask = 0;

	if (!coop.value && deathmatch.value)
		info_mask2 = 0x80000000;
	else
		info_mask2 = 0;

	cls.mapstring[0] = 0;

	for (i = 0; i < Cmd_Argc (); i++)
	{
		strcat (cls.mapstring, Cmd_Argv (i));
		strcat (cls.mapstring, " ");
	}

	strcat (cls.mapstring, "\n");

	svs.serverflags = 0;			// haven't completed an episode yet
	strcpy (name, Cmd_Argv (1));
#ifdef QUAKE2RJ
	SV_SpawnServer (name, NULL);
#else
	SV_SpawnServer (name);
#endif

	if (!sv.active)
		return;

	if (cls.state != ca_dedicated)
	{
		loading_stage = 2;

		strcpy (cls.spawnparms, "");

		for (i = 2; i < Cmd_Argc (); i++)
		{
			strcat (cls.spawnparms, Cmd_Argv (i));
			strcat (cls.spawnparms, " ");
		}

		Cmd_ExecuteString ("connect local", src_command);
	}
}

/*
==================
Host_Changelevel_f

Goes to a new map, taking all clients along
==================
*/
void Host_Changelevel_f (void)
{
#ifdef QUAKE2RJ
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc () < 2)
	{
		Con_Printf (PRINT_NORMAL, "changelevel <levelname> : continue game on a new level\n");
		return;
	}

	if (!sv.active || cls.demoplayback)
	{
		Con_Printf (PRINT_NORMAL, "Only the server may changelevel\n");
		return;
	}

	strcpy (level, Cmd_Argv (1));

	if (Cmd_Argc () == 2)
		startspot = NULL;
	else
	{
		strcpy (_startspot, Cmd_Argv (2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms ();
	SV_SpawnServer (level, startspot);

	//updatePlaqueMessag ();
#else
#endif
}

/*
==================
Host_Restart_f

Restarts the current server for a dead player
==================
*/
void Host_Restart_f (void)
{
	char	mapname[MAX_QPATH];
	char	startspot[MAX_QPATH];

	if (cls.demoplayback || !sv.active)
		return;

	if (cmd_source != src_command)
		return;

	strcpy (mapname, sv.name);	// must copy out, because it gets cleared
	strcpy (startspot, sv.startspot);

	if (Cmd_Argc () == 2 && strcmpi (Cmd_Argv (1), "restore") == 0)
	{
		if (LoadGamestate (mapname, startspot, 3))
		{
			SV_SpawnServer (mapname, startspot);
			RestoreClients ();
		}
	}
	else
	{
		// in sv_spawnserver
		SV_SpawnServer (mapname, startspot);
	}

	//	updatePlaqueMessag ();
}

/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f (void)
{
	R_ClearParticles ();	//jfm: for restarts which didn't use to clear parts.

	//updatePlaqueMessag ();

	SCR_BeginLoadingPlaque ();
	cls.signon = 0;		// need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f (void)
{
	char	name[MAX_QPATH];

	cls.demonum = -1;		// stop demo loop in case this fails

	if (cls.demoplayback)
	{
		CL_StopPlayback ();
		CL_Disconnect ();
	}

	strcpy (name, Cmd_Argv (1));
	CL_EstablishConnection (name);
	Host_Reconnect_f ();
}


/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define	SAVEGAME_VERSION	5

#define ShortTime "%m/%d/%Y %H:%M"


char	name[MAX_OSPATH], dest[MAX_OSPATH], tempdir[MAX_OSPATH];

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment (char *text)
{
	int		i;
	char	kills[20];
	struct tm *tblock;
	time_t TempTime;

	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		text[i] = ' ';

	memcpy (text, cl.levelname, strlen (cl.levelname));
	//	sprintf (kills,"kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);

	TempTime = time (NULL);
	tblock = localtime (&TempTime);
	strftime (kills, sizeof (kills), ShortTime, tblock);

	memcpy (text + 21, kills, strlen (kills));

	// convert space to _ to make stdio happy
	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		if (text[i] == ' ')
			text[i] = '_';

	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

/*
===============
Host_Savegame_f
===============
*/
void Host_Savegame_f (void)
{
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH + 1];
	//	char	name[MAX_OSPATH],dest[MAX_OSPATH],tempdir[MAX_OSPATH];
	qboolean error_state = false;
	int attempts = 0;
	char *message;

	if (cmd_source != src_command)
		return;

	if (!sv.active)
	{
		Con_Printf (PRINT_NORMAL, "Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf (PRINT_NORMAL, "Can't save in intermission.\n");
		return;
	}

#ifndef TESTSAVE

	if (svs.maxclients != 1)
	{
		Con_Printf (PRINT_NORMAL, "Can't save multiplayer games.\n");
		return;
	}

#endif

	if (Cmd_Argc () != 2)
	{
		Con_Printf (PRINT_NORMAL, "save <savename> : save a game\n");
		return;
	}

	if (strstr (Cmd_Argv (1), ".."))
	{
		Con_Printf (PRINT_NORMAL, "Relative pathnames are not allowed.\n");
		return;
	}

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0))
		{
			Con_Printf (PRINT_NORMAL, "Can't savegame with a dead player\n");
			return;
		}
	}



	SaveGamestate (false);

retry:
	attempts++;

	sprintf (name, "%s/%s", com_savedir, Cmd_Argv (1));
	Sys_mkdir (name);

	CL_RemoveGIPFiles (name);

	i = GetTempPath (sizeof (tempdir), tempdir);

	if (!i)
	{
		sprintf (tempdir, "%s\\", com_savedir);
	}

	sprintf (name, "%sclients.gip", tempdir);
	DeleteFile (name);

	sprintf (name, "%s*.gip", tempdir);
	sprintf (dest, "%s/%s/", com_savedir, Cmd_Argv (1));
	Con_Printf (PRINT_NORMAL, va ("Saving game to %s...\n", dest));

	error_state = CL_CopyFiles (tempdir, name, dest);

	sprintf (dest, "%s/%s/info.dat", com_savedir, Cmd_Argv (1));
	f = fopen (dest, "w");

	if (!f)
	{
		Con_Printf (PRINT_NORMAL, "ERROR: couldn't open.\n");
		return;
	}

	fprintf (f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment (comment);
	fprintf (f, "%s\n", comment);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);

	fprintf (f, "%d\n", current_skill);
	fprintf (f, "%s\n", sv.name);
	fprintf (f, "%f\n", sv.time);
	fprintf (f, "%d\n", svs.maxclients);
	fprintf (f, "%f\n", deathmatch.value);
	fprintf (f, "%f\n", coop.value);
	fprintf (f, "%f\n", teamplay.value);
	fprintf (f, "%f\n", randomclass.value);
	fprintf (f, "%f\n", cl_playerclass.value);
	fprintf (f, "%d\n", info_mask);
	fprintf (f, "%d\n", info_mask2);

	if (ferror (f))
		error_state = true;

	fclose (f);

	if (error_state)
	{
		if (attempts == 1)
			message = "The game could not be saved properly.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-save the game, otherwise 'N' to ignore.";
		else
			message = "The game could not be saved properly on the previous attempt.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-save the game, otherwise 'N' to ignore.";

		key_lastpress = 0;

		if (SCR_ModalMessage (message))
		{
			goto retry;
		}
	}
}


/*
===============
Host_Loadgame_f
===============
*/
void Host_Loadgame_f (void)
{
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time, tfloat;
	char	str[32768];
	int		i;
	edict_t	*ent;
	int		version;
	float	tempf;
	int		tempi;
	float			spawn_parms[NUM_SPAWN_PARMS];
	//	char	name[MAX_OSPATH],dest[MAX_OSPATH],tempdir[MAX_OSPATH];
	qboolean error_state = false;
	int attempts = 0;
	char *message;

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc () != 2)
	{
		Con_Printf (PRINT_NORMAL, "load <savename> : load a game\n");
		return;
	}

	cls.demonum = -1;		// stop demo loop in case this fails
	CL_Disconnect ();
	CL_RemoveGIPFiles (NULL);

	sprintf (name, "%s/%s", com_savedir, Cmd_Argv (1));

	Con_Printf (PRINT_NORMAL, va ("Loading game from %s...\n", name));

	i = GetTempPath (sizeof (tempdir), tempdir);

	if (!i)
	{
		sprintf (tempdir, "%s\\", com_savedir);
	}

	sprintf (dest, "%s/info.dat", name);

	f = fopen (dest, "r");

	if (!f)
	{
		Con_Printf (PRINT_NORMAL, "ERROR: couldn't open.\n");
		return;
	}

	fscanf (f, "%i\n", &version);

	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf (PRINT_NORMAL, va ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION));
		return;
	}

	fscanf (f, "%s\n", str);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		fscanf (f, "%f\n", &spawn_parms[i]);

	// this silliness is so we can load 1.06 save files, which have float skill values
	fscanf (f, "%f\n", &tfloat);
	current_skill = (int) (tfloat + 0.1);
	Cvar_SetValue ("skill", (float) current_skill);

#ifdef QUAKE2RJ
	Cvar_SetValue ("deathmatch", 0);
	Cvar_SetValue ("coop", 0);
	Cvar_SetValue ("teamplay", 0);
	Cvar_SetValue ("randomclass", 0);
#endif

	fscanf (f, "%s\n", mapname);
	fscanf (f, "%f\n", &time);

	tempi = -1;
	fscanf (f, "%d\n", &tempi);

	if (tempi >= 1)
		svs.maxclients = tempi;

	tempf = -1;
	fscanf (f, "%f\n", &tempf);

	if (tempf >= 0)
		Cvar_SetValue ("deathmatch", tempf);

	tempf = -1;
	fscanf (f, "%f\n", &tempf);

	if (tempf >= 0)
		Cvar_SetValue ("coop", tempf);

	tempf = -1;
	fscanf (f, "%f\n", &tempf);

	if (tempf >= 0)
		Cvar_SetValue ("teamplay", tempf);

	tempf = -1;
	fscanf (f, "%f\n", &tempf);

	if (tempf >= 0)
		Cvar_SetValue ("randomclass", tempf);

	tempf = -1;
	fscanf (f, "%f\n", &tempf);

	if (tempf >= 0)
		Cvar_SetValue ("_cl_playerclass", tempf);

	fscanf (f, "%d\n", &info_mask);
	fscanf (f, "%d\n", &info_mask2);

	fclose (f);

	CL_RemoveGIPFiles (tempdir);

retry:
	attempts++;

	sprintf (name, "%s/%s/*.gip", com_savedir, Cmd_Argv (1));
	sprintf (dest, "%s/%s/", com_savedir, Cmd_Argv (1));
	strcat (tempdir, "/");

	error_state = CL_CopyFiles (dest, name, tempdir);

	if (error_state)
	{
		if (attempts == 1)
			message = "The game could not be loaded properly.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-load the game, otherwise 'N' to abort.";
		else
			message = "The game could not be loaded properly on the previous attempt.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-load the game, otherwise 'N' to abort.";

		key_lastpress = 0;

		if (SCR_ModalMessage (message))
		{
			goto retry;
		}
		else
			return;
	}

	LoadGamestate (mapname, NULL, 2);

	SV_SaveSpawnparms ();

	ent = EDICT_NUM (1);

	Cvar_SetValue ("_cl_playerclass", ent->v.playerclass);//this better be the same as above...

	svs.clients->playerclass = ent->v.playerclass;

	sv.paused = true;		// pause until all clients connect
	sv.loadgame = true;

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection ("local");
		Host_Reconnect_f ();
	}
}

#ifdef QUAKE2RJ
void SaveGamestate (qboolean ClientsOnly)
{
	//	char	name[MAX_OSPATH],tempdir[MAX_OSPATH];
	FILE	*f;
	int		i;
	char	comment[SAVEGAME_COMMENT_LENGTH + 1];
	edict_t	*ent;
	int start, end;
	qboolean error_state = false;
	int attempts = 0;
	char *message;

retry:

	attempts++;

	i = GetTempPath (sizeof (tempdir), tempdir);

	if (!i)
	{
		sprintf (tempdir, "%s\\", com_savedir);
	}

	if (ClientsOnly)
	{
		start = 1;
		end = svs.maxclients + 1;

		sprintf (name, "%sclients.gip", tempdir);
	}
	else
	{
		start = 1;
		end = sv.num_edicts;

		sprintf (name, "%s%s.gip", tempdir, sv.name);
	}

	f = fopen (name, "w");

	if (!f)
	{
		Con_Printf (PRINT_NORMAL, "ERROR: couldn't open.\n");
		return;
	}

	fprintf (f, "%i\n", SAVEGAME_VERSION);

	if (!ClientsOnly)
	{
		Host_SavegameComment (comment);
		fprintf (f, "%s\n", comment);
		//	for (i=0; i<NUM_SPAWN_PARMS; i++)
		//		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
		fprintf (f, "%f\n", skill.value);
		fprintf (f, "%s\n", sv.name);
		fprintf (f, "%f\n", sv.time);

		//		fprintf (f, "%d\n", info_mask);
		//		fprintf (f, "%d\n", info_mask2);

		// write the light styles

		for (i = 0; i < MAX_LIGHTSTYLES; i++)
		{
			if (sv.lightstyles[i])
				fprintf (f, "%s\n", sv.lightstyles[i]);
			else
				fprintf (f, "m\n");
		}

		SV_SaveEffects (f);
		fprintf (f, "-1\n");
		ED_WriteGlobals (f);
	}
	else
	{
		/*fprintf(f, "%d\n", info_mask);
		fprintf(f, "%d\n", info_mask2);*/
	}

	host_client = svs.clients;

	//	for (i=svs.maxclients+1; i<sv.num_edicts; i++)
	//  to save the client states
	for (i = start; i < end; i++)
	{
		ent = EDICT_NUM (i);

		if ((int) ent->v.flags & FL_ARCHIVE_OVERRIDE)
			continue;

		if (ClientsOnly)
		{
			if (host_client->active)
			{
				fprintf (f, "%i\n", i);
				ED_Write (f, ent);
				fflush (f);
			}

			host_client++;
		}
		else
		{
			fprintf (f, "%i\n", i);
			ED_Write (f, ent);
			fflush (f);
		}
	}

	if (ferror (f))
	{
		error_state = true;
	}

	fclose (f);

	if (error_state)
	{
		if (attempts == 1)
			message = "The level could not be saved properly.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-save the level, otherwise 'N' to ignore.";
		else
			message = "The level could not be saved properly on the previous attempt.  You may be out of hard drive space!  You can ALT-TAB out to try and free up some space.  Type 'Y' if you want to try and re-save the level, otherwise 'N' to ignore.";

		key_lastpress = 0;

		if (SCR_ModalMessage (message))
		{
			goto retry;
		}
	}
}

void RestoreClients (void)
{
	int i, j;
	edict_t	*ent;
	double time_diff;

	if (LoadGamestate (NULL, NULL, 1))
		return;

	time_diff = sv.time - old_time;

	for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		if (host_client->active)
		{
			ent = host_client->edict;

			//ent->v.colormap = NUM_FOR_EDICT(ent);
			ent->v.team = (host_client->colors & 15) + 1;
			ent->v.netname = host_client->name - pr_strings;
			ent->v.playerclass = host_client->playerclass;

			// copy spawn parms out of the client_t

			for (j = 0; j < NUM_SPAWN_PARMS; j++)
				(&pr_global_struct->parm1)[j] = host_client->spawn_parms[j];

			// call the spawn function

			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG (ent);
			G_FLOAT (OFS_PARM0) = time_diff;
			PR_ExecuteProgram (pr_global_struct->ClientReEnter);
		}

	SaveGamestate (true);
}

int LoadGamestate (char *level, char *startspot, int ClientsMode)
{
	//	char	name[MAX_OSPATH],tempdir[MAX_OSPATH];
	FILE	*f;
	char	mapname[MAX_QPATH];
	float	time = 0, sk;
	char	str[32768], *start;
	int		i, r;
	edict_t	*ent;
	int		entnum;
	int		version;
	//	float	spawn_parms[NUM_SPAWN_PARMS];
	qboolean auto_correct = false;

	i = GetTempPath (sizeof (tempdir), tempdir);

	if (!i)
	{
		sprintf (tempdir, "%s\\", com_savedir);
	}

	if (ClientsMode == 1)
	{
		sprintf (name, "%sclients.gip", tempdir);
	}
	else
	{
		sprintf (name, "%s%s.gip", tempdir, level);

		if (ClientsMode != 2 && ClientsMode != 3)
			Con_Printf (PRINT_NORMAL, va ("Loading game from %s...\n", name));
	}

	f = fopen (name, "r");

	if (!f)
	{
		if (ClientsMode == 2)
			Con_Printf (PRINT_NORMAL, "ERROR: couldn't open.\n");

		return -1;
	}

	fscanf (f, "%i\n", &version);

	if (version != SAVEGAME_VERSION)
	{
		fclose (f);
		Con_Printf (PRINT_NORMAL, va ("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION));
		return -1;
	}

	if (ClientsMode != 1)
	{
		fscanf (f, "%s\n", str);
		//	for (i=0; i<NUM_SPAWN_PARMS; i++)
		//		fscanf (f, "%f\n", &spawn_parms[i]);
		fscanf (f, "%f\n", &sk);
		Cvar_SetValue ("skill", sk);

		fscanf (f, "%s\n", mapname);
		fscanf (f, "%f\n", &time);

		SV_SpawnServer (mapname, startspot);

		if (!sv.active)
		{
			Con_Printf (PRINT_NORMAL, "Couldn't load map\n");
			return -1;
		}

		//		fscanf (f, "%d\n",&info_mask);
		//		fscanf (f, "%d\n",&info_mask2);

		// load the light styles
		for (i = 0; i < MAX_LIGHTSTYLES; i++)
		{
			fscanf (f, "%s\n", str);
			sv.lightstyles[i] = Hunk_Alloc (MAP_HUNK, strlen (str) + 1);
			strcpy (sv.lightstyles[i], str);
		}

		SV_LoadEffects (f);
	}

	// load the edicts out of the savegame file
	while (!feof (f))
	{
		fscanf (f, "%i\n", &entnum);

		for (i = 0; i < sizeof (str) - 1; i++)
		{
			r = fgetc (f);

			if (r == EOF || !r)
				break;

			str[i] = r;

			if (r == '}')
			{
				i++;
				break;
			}
		}

		if (i == sizeof (str) - 1)
			Sys_Error ("Loadgame buffer overflow");

		str[i] = 0;
		start = str;
		start = COM_Parse (str);

		if (!com_token[0])
			break;		// end of file

		if (strcmp (com_token, "{"))
			Sys_Error ("First token isn't a brace");

		// parse an edict

		if (entnum == -1)
		{
			ED_ParseGlobals (start);
			// Need to restore this
			pr_global_struct->startspot = sv.startspot - pr_strings;
		}
		else
		{
			ent = EDICT_NUM (entnum);
			memset (&ent->v, 0, progs->entityfields * 4);
			//ent->free = false;
			ED_ParseEdict (start, ent);

			if (ClientsMode == 1 || ClientsMode == 2 || ClientsMode == 3)
				ent->v.stats_restored = true;

			// link it into the bsp tree
			if (!ent->free)
			{
				SV_LinkEdict (ent, false);

				if (ent->v.modelindex && ent->v.model)
				{
					i = SV_ModelIndex (ent->v.model + pr_strings);

					if (i != ent->v.modelindex)
					{
						ent->v.modelindex = i;
						auto_correct = true;
					}
				}
			}
		}
	}

	fclose (f);

	//	sv.num_edicts = entnum;
	if (ClientsMode == 0)
	{
		sv.time = time;
		sv.paused = true;

		pr_global_struct->serverflags = svs.serverflags;

		RestoreClients ();
	}
	else if (ClientsMode == 2)
	{
		sv.time = time;
	}
	else if (ClientsMode == 3)
	{
		sv.time = time;

		pr_global_struct->serverflags = svs.serverflags;

		RestoreClients ();
	}

	if (ClientsMode != 1 && auto_correct)
	{
		Con_Printf (PRINT_DEVELOPER, "*** Auto-corrected model indexes!\n");
	}

	//	for (i=0; i<NUM_SPAWN_PARMS; i++)
	//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void Host_Changelevel2_f (void)
{
	char	level[MAX_QPATH];
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc () < 2)
	{
		Con_Printf (PRINT_NORMAL, "changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}

	if (!sv.active || cls.demoplayback)
	{
		Con_Printf (PRINT_NORMAL, "Only the server may changelevel\n");
		return;
	}

	strcpy (level, Cmd_Argv (1));

	if (Cmd_Argc () == 2)
		startspot = NULL;
	else
	{
		strcpy (_startspot, Cmd_Argv (2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms ();

	// save the current level's state
	old_time = sv.time;
	SaveGamestate (false);

	// try to restore the new level
	if (LoadGamestate (level, startspot, 0))
	{
		SV_SpawnServer (level, startspot);
		RestoreClients ();
	}
}
#endif


//============================================================================

/*
======================
Host_Name_f
======================
*/
void Host_Name_f (void)
{
	char	*newName;
	char	*pdest;

	if (Cmd_Argc () == 1)
	{
		Con_Printf (PRINT_NORMAL, va ("\"name\" is \"%s\"\n", cl_name.string));
		return;
	}

	if (Cmd_Argc () == 2)
		newName = Cmd_Argv (1);
	else newName = Cmd_Args ();

	newName[15] = 0;

	//this is for the fuckers who put braces in the name causing loadgame to crash.
	pdest = strchr (newName, '{');

	if (pdest)
	{
		*pdest = 0;	//zap the brace
		Con_Printf (PRINT_NORMAL, "Illegal char in name removed!\n");
	}

	if (cmd_source == src_command)
	{
		if (strcmp (cl_name.string, newName) == 0)
			return;

		Cvar_Set ("_cl_name", newName);

		if (cls.state == ca_connected)
			Cmd_ForwardToServer ();

		return;
	}

	if (host_client->name[0] && strcmp (host_client->name, "unconnected"))
		if (strcmp (host_client->name, newName) != 0)
			Con_Printf (PRINT_NORMAL, va ("%s renamed to %s\n", host_client->name, newName));

	strcpy (host_client->name, newName);
	host_client->edict->v.netname = host_client->name - pr_strings;

	// send notification to all clients

	MSG_WriteByte (&sv.reliable_datagram, svc_updatename);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteString (&sv.reliable_datagram, host_client->name);
}

extern char *ClassNames[NUM_CLASSES];	//from menu.c
void Host_Class_f (void)
{
	float	newClass;

	if (Cmd_Argc () == 1)
	{
		if (!(int) cl_playerclass.value)
			Con_Printf (PRINT_NORMAL, va ("\"playerclass\" is %d (\"unknown\")\n", (int) cl_playerclass.value));
		else
			Con_Printf (PRINT_NORMAL, va ("\"playerclass\" is %d (\"%s\")\n", (int) cl_playerclass.value, ClassNames[(int) cl_playerclass.value - 1]));

		return;
	}

	if (Cmd_Argc () == 2)
		newClass = atof (Cmd_Argv (1));
	else newClass = atof (Cmd_Args ());

	if (cmd_source == src_command)
	{
		Cvar_SetValue ("_cl_playerclass", newClass);

		if (cls.state == ca_connected)
			Cmd_ForwardToServer ();

		return;
	}

	if (sv.loadgame || host_client->playerclass)
	{
		if (host_client->edict->v.playerclass)
			newClass = host_client->edict->v.playerclass;
		else if (host_client->playerclass)
			newClass = host_client->playerclass;
	}

	host_client->playerclass = newClass;
	host_client->edict->v.playerclass = newClass;

	// Change the weapon model used
	pr_global_struct->self = EDICT_TO_PROG (host_client->edict);
	PR_ExecuteProgram (pr_global_struct->ClassChangeWeapon);

	// send notification to all clients

	MSG_WriteByte (&sv.reliable_datagram, svc_updateclass);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte (&sv.reliable_datagram, (byte) newClass);
}

//just an easy place to do some profile testing
#if 0
void Host_Version_f (void)
{
	int i;
	int repcount = 10000;
	float time1, time2, r1, r2;


	if (Cmd_Argc () == 2)
	{
		repcount = atof (Cmd_Argv (1));

		if (repcount < 0)
			repcount = 0;
	}

	Con_Printf (PRINT_NORMAL, va ("looping %d times.\n", repcount));

	time1 = Sys_DoubleTime ();

	for (i = repcount; i; i--)
	{
		char buf[2048];
		Q_memset (buf, i, 2048);
	}

	time2 = Sys_DoubleTime ();
	r1 = time2 - time1;
	Con_Printf (PRINT_NORMAL, va ("loop 1 = %f\n", r1));

	time1 = Sys_DoubleTime ();

	for (i = repcount; i; i--)
	{
		char buf[2048];
		memset (buf, i, 2048);
	}

	time2 = Sys_DoubleTime ();
	r2 = time2 - time1;
	Con_Printf (PRINT_NORMAL, va ("loop 2 = %f\n", r2));

	if (r2 < r1)
	{
		Con_Printf (PRINT_NORMAL, va ("loop 2 is faster by %f\n", r1 - r2));
	}
	else
	{
		Con_Printf (PRINT_NORMAL, va ("loop 1 is faster by %f\n", r2 - r1));
	}

	Con_Printf (PRINT_NORMAL, va ("Version %4.2f\n", HEXEN2_VERSION));
	Con_Printf (PRINT_NORMAL, va ("Exe: "__TIME__" "__DATE__"\n"));
}
#else
void Host_Version_f (void)
{
	Con_Printf (PRINT_NORMAL, va ("Version %4.2f\n", HEXEN2_VERSION));
	Con_Printf (PRINT_NORMAL, va ("Exe: "__TIME__" "__DATE__"\n"));
}
#endif

#ifdef IDGODS
void Host_Please_f (void)
{
	client_t *cl;
	int			j;

	if (cmd_source != src_command)
		return;

	if ((Cmd_Argc () == 3) && strcmp (Cmd_Argv (1), "#") == 0)
	{
		j = atof (Cmd_Argv (2)) - 1;

		if (j < 0 || j >= svs.maxclients)
			return;

		if (!svs.clients[j].active)
			return;

		cl = &svs.clients[j];

		if (cl->privileged)
		{
			cl->privileged = false;
			cl->edict->v.flags = (int) cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
			cl->edict->v.movetype = MOVETYPE_WALK;
			noclip_anglehack = false;
		}
		else
			cl->privileged = true;
	}

	if (Cmd_Argc () != 2)
		return;

	for (j = 0, cl = svs.clients; j < svs.maxclients; j++, cl++)
	{
		if (!cl->active)
			continue;

		if (Q_strcasecmp (cl->name, Cmd_Argv (1)) == 0)
		{
			if (cl->privileged)
			{
				cl->privileged = false;
				cl->edict->v.flags = (int) cl->edict->v.flags & ~(FL_GODMODE | FL_NOTARGET);
				cl->edict->v.movetype = MOVETYPE_WALK;
				noclip_anglehack = false;
			}
			else
				cl->privileged = true;

			break;
		}
	}
}
#endif


void Host_Say (qboolean teamonly)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	char	text[64];
	qboolean	fromServer = false;

	if (cmd_source == src_command)
	{
		if (cls.state == ca_dedicated)
		{
			fromServer = true;
			teamonly = false;
		}
		else
		{
			Cmd_ForwardToServer ();
			return;
		}
	}

	if (Cmd_Argc () < 2)
		return;

	save = host_client;

	p = Cmd_Args ();

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen (p) - 1] = 0;
	}

	// turn on color set 1
	if (!fromServer)
		sprintf (text, "%c%s: ", 1, save->name);
	else
		sprintf (text, "%c<%s> ", 1, hostname.string);

	j = sizeof (text) - 2 - strlen (text); // -2 for /n and null terminator

	if (strlen (p) > j)
		p[j] = 0;

	strcat (text, p);
	strcat (text, "\n");

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client || !client->active || !client->spawned)
			continue;

		if (teamplay.value && teamonly && client->edict->v.team != save->edict->v.team)
			continue;

		host_client = client;
		SV_ClientPrintf (va ("%s", text));
	}

	host_client = save;

	Sys_Printf ("%s", &text[1]);
}


void Host_Say_f (void)
{
	Host_Say (false);
}


void Host_Say_Team_f (void)
{
	Host_Say (true);
}


void Host_Tell_f (void)
{
	client_t *client;
	client_t *save;
	int		j;
	char	*p;
	char	text[64];

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if (Cmd_Argc () < 3)
		return;

	strcpy (text, host_client->name);
	strcat (text, ": ");

	p = Cmd_Args ();

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen (p) - 1] = 0;
	}

	// check length & truncate if necessary
	j = sizeof (text) - 2 - strlen (text); // -2 for /n and null terminator

	if (strlen (p) > j)
		p[j] = 0;

	strcat (text, p);
	strcat (text, "\n");

	save = host_client;

	for (j = 0, client = svs.clients; j < svs.maxclients; j++, client++)
	{
		if (!client->active || !client->spawned)
			continue;

		if (Q_strcasecmp (client->name, Cmd_Argv (1)))
			continue;

		host_client = client;
		SV_ClientPrintf (va ("%s", text));
		break;
	}

	host_client = save;
}


/*
==================
Host_Color_f
==================
*/
void Host_Color_f (void)
{
	int		top, bottom;
	int		playercolor;

	if (Cmd_Argc () == 1)
	{
		Con_Printf (PRINT_NORMAL, va ("\"color\" is \"%i %i\"\n", ((int) cl_color.value) >> 4, ((int) cl_color.value) & 0x0f));
		Con_Printf (PRINT_NORMAL, "color <0-10> [0-10]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		top = bottom = atoi (Cmd_Argv (1));
	else
	{
		top = atoi (Cmd_Argv (1));
		bottom = atoi (Cmd_Argv (2));
	}

	top &= 15;

	if (top > 13)
		top = 13;

	bottom &= 15;

	if (bottom > 13)
		bottom = 13;

	playercolor = (top << 4) | bottom;

	if (cmd_source == src_command)
	{
		Cvar_SetValue ("_cl_color", playercolor);

		if (cls.state == ca_connected)
			Cmd_ForwardToServer ();

		return;
	}

	host_client->colors = playercolor;
	host_client->edict->v.team = bottom + 1;

	// send notification to all clients
	MSG_WriteByte (&sv.reliable_datagram, svc_updatecolors);
	MSG_WriteByte (&sv.reliable_datagram, host_client - svs.clients);
	MSG_WriteByte (&sv.reliable_datagram, host_client->colors);
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f (void)
{
	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if (sv_player->v.health <= 0)
	{
		SV_ClientPrintf ("Can't suicide -- allready dead!\n");
		return;
	}

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG (sv_player);
	PR_ExecuteProgram (pr_global_struct->ClientKill);
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f (void)
{

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if (!pausable.value)
		SV_ClientPrintf ("Pause not allowed.\n");
	else
	{
		sv.paused ^= 1;

		if (sv.paused)
		{
			SV_BroadcastPrintf (va ("%s paused the game\n", pr_strings + sv_player->v.netname));
		}
		else
		{
			SV_BroadcastPrintf (va ("%s unpaused the game\n", pr_strings + sv_player->v.netname));
		}

		// send notification to all clients
		MSG_WriteByte (&sv.reliable_datagram, svc_setpause);
		MSG_WriteByte (&sv.reliable_datagram, sv.paused);
	}
}

//===========================================================================


/*
==================
Host_PreSpawn_f
==================
*/
void Host_PreSpawn_f (void)
{
	if (cmd_source == src_command)
	{
		Con_Printf (PRINT_NORMAL, "prespawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf (PRINT_NORMAL, "prespawn not valid -- allready spawned\n");
		return;
	}

	SZ_Write (&host_client->message, sv.signon.data, sv.signon.cursize);
	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 2);
	host_client->sendsignon = true;
}

/*
==================
Host_Spawn_f
==================
*/
void Host_Spawn_f (void)
{
	int		i;
	client_t	*client;
	edict_t	*ent;
	float *sendangle;

	if (cmd_source == src_command)
	{
		Con_Printf (PRINT_NORMAL, "spawn is not valid from the console\n");
		return;
	}

	if (host_client->spawned)
	{
		Con_Printf (PRINT_NORMAL, "Spawn not valid -- allready spawned\n");
		return;
	}

	// send all current names, colors, and frag counts
	SZ_Clear (&host_client->message);

	// run the entrance script
	if (sv.loadgame)
	{
		// loaded games are fully inited allready
		// if this is the last client to be connected, unpause
		sv.paused = false;
	}
	else
	{
		// set up the edict
		ent = host_client->edict;
		sv.paused = false;

		if (!ent->v.stats_restored || deathmatch.value)
		{
			memset (&ent->v, 0, progs->entityfields * 4);

			//ent->v.colormap = NUM_FOR_EDICT(ent);
			ent->v.team = (host_client->colors & 15) + 1;
			ent->v.netname = host_client->name - pr_strings;
			ent->v.playerclass = host_client->playerclass;

			// copy spawn parms out of the client_t

			for (i = 0; i < NUM_SPAWN_PARMS; i++)
				(&pr_global_struct->parm1)[i] = host_client->spawn_parms[i];

			// call the spawn function

			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG (sv_player);
			PR_ExecuteProgram (pr_global_struct->ClientConnect);

			if ((Sys_DoubleTime () - host_client->netconnection->connecttime) <= sv.time)
				Sys_Printf ("%s entered the game\n", host_client->name);

			PR_ExecuteProgram (pr_global_struct->PutClientInServer);
		}
	}


	// send time of update
	MSG_WriteByte (&host_client->message, svc_time);
	MSG_WriteFloat (&host_client->message, sv.time);

	for (i = 0, client = svs.clients; i < svs.maxclients; i++, client++)
	{
		MSG_WriteByte (&host_client->message, svc_updatename);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteString (&host_client->message, client->name);

		MSG_WriteByte (&host_client->message, svc_updateclass);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteByte (&host_client->message, client->playerclass);

		MSG_WriteByte (&host_client->message, svc_updatefrags);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteShort (&host_client->message, client->old_frags);

		MSG_WriteByte (&host_client->message, svc_updatecolors);
		MSG_WriteByte (&host_client->message, i);
		MSG_WriteByte (&host_client->message, client->colors);
	}

	// send all current light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		MSG_WriteByte (&host_client->message, svc_lightstyle);
		MSG_WriteByte (&host_client->message, (char) i);
		MSG_WriteString (&host_client->message, sv.lightstyles[i]);
	}

	// send some stats
	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALSECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_TOTALMONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->total_monsters);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_SECRETS);
	MSG_WriteLong (&host_client->message, pr_global_struct->found_secrets);

	MSG_WriteByte (&host_client->message, svc_updatestat);
	MSG_WriteByte (&host_client->message, STAT_MONSTERS);
	MSG_WriteLong (&host_client->message, pr_global_struct->killed_monsters);


	SV_UpdateEffects (&host_client->message);

	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	ent = EDICT_NUM (1 + (host_client - svs.clients));
	MSG_WriteByte (&host_client->message, svc_setangle);

	sendangle = sv.loadgame ? ent->v.v_angle : ent->v.angles;

	for (i = 0; i < 2; i++)
		MSG_WriteAngle (&host_client->message, sendangle[i]);

	MSG_WriteAngle (&host_client->message, 0);

	SV_WriteClientdataToMessage (host_client, sv_player, &host_client->message);

	MSG_WriteByte (&host_client->message, svc_signonnum);
	MSG_WriteByte (&host_client->message, 3);
	host_client->sendsignon = true;
}

dfunction_t *ED_FindFunctioni (char *name);

#define		MAXCMDLINE	256
extern  char	key_lines[32][MAXCMDLINE];
extern int		key_linepos;
extern int		edit_line;

int strdiff (char *s1, char *s2)
{
	int L1, L2, i;

	L1 = strlen (s1);
	L2 = strlen (s2);

	for (i = 0; (i < L1 && i < L2); i++)
	{
		if (tolower (s1[i]) != tolower (s2[i]))
			break;
	}

	return i;
}

void Host_Commands_f (void)
{
	FILE *FH;
	cvar_t	*var;

	FH = fopen ("commands.txt", "w");

	fprintf (FH, "\n\nConsole Commands:\n");
	WriteCommands (FH);

	fprintf (FH, "\n\nConsole Variables:\n");

	for (var = cvar_vars; var; var = var->next)
		fprintf (FH, "   %s\n", var->name);

	fclose (FH);
}

void Host_Create_f (void)
{
	char *FindName;
	dfunction_t	*Search, *func;
	edict_t		*ent;
	int			i, Length, NumFound, Diff, NewDiff;

	if (!sv.active)
	{
		Con_Printf (PRINT_NORMAL, "server is not active!\n");
		return;
	}

	if ((svs.maxclients != 1) || (skill.value > 2))
	{
		Con_Printf (PRINT_NORMAL, "can't cheat anymore!\n");
		return;
	}

	if (Cmd_Argc () == 1)
	{
		Con_Printf (PRINT_NORMAL, "create <quake-ed spawn function>\n");
		return;
	}

	FindName = Cmd_Argv (1);

	func = ED_FindFunctioni (FindName);

	if (!func)
	{
		Length = strlen (FindName);
		NumFound = 0;

		Diff = 999;

		for (i = 0; i < progs->numfunctions; i++)
		{
			Search = &pr_functions[i];

			if (!_strnicmp (pr_strings + Search->s_name, FindName, Length))
			{
				if (NumFound == 1)
				{
					Con_Printf (PRINT_NORMAL, va ("   %s\n", pr_strings + func->s_name));
				}

				if (NumFound)
				{
					Con_Printf (PRINT_NORMAL, va ("   %s\n", pr_strings + Search->s_name));
					NewDiff = strdiff (pr_strings + Search->s_name, pr_strings + func->s_name);

					if (NewDiff < Diff) Diff = NewDiff;
				}

				func = Search;
				NumFound++;
			}
		}

		if (!NumFound)
		{
			Con_Printf (PRINT_NORMAL, "Could not find spawn function\n");
			return;
		}

		if (NumFound != 1)
		{
			sprintf (key_lines[edit_line], ">create %s", func->s_name + pr_strings);
			key_lines[edit_line][Diff + 8] = 0;
			key_linepos = strlen (key_lines[edit_line]);
			return;
		}
	}

	Con_Printf (PRINT_NORMAL, va ("Executing %s...\n", pr_strings + func->s_name));

	ent = ED_Alloc ();

	ent->v.classname = func->s_name;
	VectorCopy (r_origin, ent->v.origin);
	ent->v.origin[0] += vpn[0] * 80;
	ent->v.origin[1] += vpn[1] * 80;
	ent->v.origin[2] += vpn[2] * 80;
	VectorCopy (ent->v.origin, ent->v.absmin);
	VectorCopy (ent->v.origin, ent->v.absmax);
	ent->v.absmin[0] -= 16;
	ent->v.absmin[1] -= 16;
	ent->v.absmin[2] -= 16;
	ent->v.absmax[0] += 16;
	ent->v.absmax[1] += 16;
	ent->v.absmax[2] += 16;

	pr_global_struct->self = EDICT_TO_PROG (ent);
	ignore_precache = true;
	PR_ExecuteProgram (func - pr_functions);
	ignore_precache = false;
}

/*
==================
Host_Begin_f
==================
*/
void Host_Begin_f (void)
{
	if (cmd_source == src_command)
	{
		Con_Printf (PRINT_NORMAL, "begin is not valid from the console\n");
		return;
	}

	host_client->spawned = true;
}

//===========================================================================


/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f (void)
{
	char		*who;
	char		*message = NULL;
	client_t	*save;
	int			i;
	qboolean	byNumber = false;

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer ();
			return;
		}
	}
	else if (pr_global_struct->deathmatch && !host_client->privileged)
		return;

	save = host_client;

	if (Cmd_Argc () > 2 && strcmp (Cmd_Argv (1), "#") == 0)
	{
		i = atof (Cmd_Argv (2)) - 1;

		if (i < 0 || i >= svs.maxclients)
			return;

		if (!svs.clients[i].active)
			return;

		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++)
		{
			if (!host_client->active)
				continue;

			if (Q_strcasecmp (host_client->name, Cmd_Argv (1)) == 0)
				break;
		}
	}

	if (i < svs.maxclients)
	{
		if (cmd_source == src_command)
			if (cls.state == ca_dedicated)
				who = "Console";
			else
				who = cl_name.string;
		else
			who = save->name;

		// can't kick yourself!
		if (host_client == save)
			return;

		if (Cmd_Argc () > 2)
		{
			message = COM_Parse (Cmd_Args ());

			if (byNumber)
			{
				message++;							// skip the #

				while (*message == ' ')				// skip white space
					message++;

				message += strlen (Cmd_Argv (2));	// skip the number
			}

			while (*message && *message == ' ')
				message++;
		}

		if (message)
			SV_ClientPrintf (va ("Kicked by %s: %s\n", who, message));
		else
			SV_ClientPrintf (va ("Kicked by %s\n", who));

		SV_DropClient (false);
	}

	host_client = save;
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

/*
==================
Host_Give_f
==================
*/
void Host_Give_f (void)
{
	char	*t;
	int		v;

	if (cmd_source == src_command)
	{
		Cmd_ForwardToServer ();
		return;
	}

	if ((pr_global_struct->deathmatch || skill.value > 2) && !host_client->privileged)
		return;

	t = Cmd_Argv (1);
	v = atoi (Cmd_Argv (2));

	switch (t[0])
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		// MED 01/04/97 added hipnotic give stuff
		/*      if (hipnotic)
			  {
			  if (t[0] == '6')
			  {
			  if (t[1] == 'a')
			  sv_player->v.items = (int)sv_player->v.items | HIT_PROXIMITY_GUN;
			  else
			  sv_player->v.items = (int)sv_player->v.items | IT_GRENADE_LAUNCHER;
			  }
			  else if (t[0] == '9')
			  sv_player->v.items = (int)sv_player->v.items | HIT_LASER_CANNON;
			  else if (t[0] == '0')
			  sv_player->v.items = (int)sv_player->v.items | HIT_MJOLNIR;
			  else if (t[0] >= '2')
			  sv_player->v.items = (int)sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
			  }
			  else
			  */
	{
		if (t[0] >= '2')
			sv_player->v.items = (int) sv_player->v.items | (IT_SHOTGUN << (t[0] - '2'));
	}
	break;

	case 's':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_shells1");
				if (val)
				val->_float = v;
				}

				sv_player->v.ammo_shells = v;
				break;		*/
	case 'n':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_nails1");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
				sv_player->v.ammo_nails = v;
				}
				}
				else
				{
				sv_player->v.ammo_nails = v;
				}*/
		break;
	case 'l':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_lava_nails");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
				sv_player->v.ammo_nails = v;
				}
				}*/
		break;
	case 'r':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_rockets1");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
				sv_player->v.ammo_rockets = v;
				}
				}
				else
				{
				sv_player->v.ammo_rockets = v;
				}*/
		break;
	case 'm':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_multi_rockets");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
				sv_player->v.ammo_rockets = v;
				}
				}*/
		break;
	case 'h':
		sv_player->v.health = v;
		break;
	case 'c':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_cells1");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon <= IT_LIGHTNING)
				sv_player->v.ammo_cells = v;
				}
				}
				else
				{
				sv_player->v.ammo_cells = v;
				}*/
		break;
	case 'p':
		/*		if (rogue)
				{
				val = GetEdictFieldValue(sv_player, "ammo_plasma");
				if (val)
				{
				val->_float = v;
				if (sv_player->v.weapon > IT_LIGHTNING)
				sv_player->v.ammo_cells = v;
				}
				}*/
		break;
	}
}

edict_t	*FindViewthing (void)
{
	int		i;
	edict_t	*e;

	for (i = 0; i < sv.num_edicts; i++)
	{
		e = EDICT_NUM (i);

		if (!strcmp (pr_strings + e->v.classname, "viewthing"))
			return e;
	}

	Con_Printf (PRINT_NORMAL, "No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();

	if (!e)
		return;

	m = Mod_ForName (Cmd_Argv (1), false);

	if (!m)
	{
		Con_Printf (PRINT_NORMAL, va ("Can't load %s\n", Cmd_Argv (1)));
		return;
	}

	e->v.frame = 0;
	cl.model_precache[(int) e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f (void)
{
	edict_t	*e;
	int		f;
	model_t	*m;

	e = FindViewthing ();

	if (!e)
		return;

	m = cl.model_precache[(int) e->v.modelindex];

	f = atoi (Cmd_Argv (1));

	if (f >= m->numframes)
		f = m->numframes - 1;

	e->v.frame = f;
}


void PrintFrameName (model_t *m, int frame)
{
	aliashdr_t 			*hdr;
	maliasframedesc_t	*pframedesc;

	if (m->type != mod_alias) return;
	if ((hdr = m->aliashdr) == NULL) return;

	pframedesc = &hdr->frames[frame];

	Con_Printf (PRINT_NORMAL, va ("frame %i: %s\n", frame, pframedesc->name));
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();

	if (!e)
		return;

	m = cl.model_precache[(int) e->v.modelindex];

	e->v.frame = e->v.frame + 1;

	if (e->v.frame >= m->numframes)
		e->v.frame = m->numframes - 1;

	PrintFrameName (m, e->v.frame);
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f (void)
{
	edict_t	*e;
	model_t	*m;

	e = FindViewthing ();

	if (!e)
		return;

	m = cl.model_precache[(int) e->v.modelindex];

	e->v.frame = e->v.frame - 1;

	if (e->v.frame < 0)
		e->v.frame = 0;

	PrintFrameName (m, e->v.frame);
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f (void)
{
	int		i, c;

	if (cls.state == ca_dedicated)
	{
		if (!sv.active)
			Cbuf_AddText ("map start\n");

		return;
	}

	c = Cmd_Argc () - 1;

	if (c > MAX_DEMOS)
	{
		Con_Printf (PRINT_NORMAL, va ("Max %i demos in demoloop\n", MAX_DEMOS));
		c = MAX_DEMOS;
	}

	Con_Printf (PRINT_NORMAL, va ("%i demo(s) in loop\n", c));

	for (i = 1; i < c + 1; i++)
		strncpy (cls.demos[i - 1], Cmd_Argv (i), sizeof (cls.demos[0]) - 1);

	if (!sv.active && cls.demonum != -1 && !cls.demoplayback)
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f (void)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demonum == -1)
		cls.demonum = 1;

	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f (void)
{
	if (cls.state == ca_dedicated)
		return;

	if (!cls.demoplayback)
		return;

	CL_StopPlayback ();
	CL_Disconnect ();
}

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands (void)
{
	Cmd_AddCommand ("status", Host_Status_f);
	Cmd_AddCommand ("quit", Host_Quit_f);
	Cmd_AddCommand ("god", Host_God_f);
	Cmd_AddCommand ("notarget", Host_Notarget_f);
	Cmd_AddCommand ("map", Host_Map_f);
	Cmd_AddCommand ("restart", Host_Restart_f);
	Cmd_AddCommand ("changelevel", Host_Changelevel_f);
#ifdef QUAKE2RJ
	Cmd_AddCommand ("changelevel2", Host_Changelevel2_f);
#endif
	Cmd_AddCommand ("connect", Host_Connect_f);
	Cmd_AddCommand ("reconnect", Host_Reconnect_f);
	Cmd_AddCommand ("name", Host_Name_f);
	Cmd_AddCommand ("playerclass", Host_Class_f);
	Cmd_AddCommand ("noclip", Host_Noclip_f);
	Cmd_AddCommand ("version", Host_Version_f);
#ifdef IDGODS
	Cmd_AddCommand ("please", Host_Please_f);
#endif
	Cmd_AddCommand ("say", Host_Say_f);
	Cmd_AddCommand ("say_team", Host_Say_Team_f);
	Cmd_AddCommand ("tell", Host_Tell_f);
	Cmd_AddCommand ("color", Host_Color_f);
	Cmd_AddCommand ("colour", Host_Color_f);
	Cmd_AddCommand ("kill", Host_Kill_f);
	Cmd_AddCommand ("pause", Host_Pause_f);
	Cmd_AddCommand ("spawn", Host_Spawn_f);
	Cmd_AddCommand ("begin", Host_Begin_f);
	Cmd_AddCommand ("prespawn", Host_PreSpawn_f);
	Cmd_AddCommand ("kick", Host_Kick_f);
	Cmd_AddCommand ("ping", Host_Ping_f);
	Cmd_AddCommand ("load", Host_Loadgame_f);
	Cmd_AddCommand ("save", Host_Savegame_f);
	Cmd_AddCommand ("give", Host_Give_f);

	Cmd_AddCommand ("startdemos", Host_Startdemos_f);
	Cmd_AddCommand ("demos", Host_Demos_f);
	Cmd_AddCommand ("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand ("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand ("viewframe", Host_Viewframe_f);
	Cmd_AddCommand ("viewnext", Host_Viewnext_f);
	Cmd_AddCommand ("viewprev", Host_Viewprev_f);

	Cmd_AddCommand ("create", Host_Create_f);
	Cmd_AddCommand ("commands", Host_Commands_f);
}

