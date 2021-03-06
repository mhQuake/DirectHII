
#ifndef QUAKEDEF_H_DEFINED
#define QUAKEDEF_H_DEFINED

// quakedef.h -- primary header for client

//#define	GLTEST			// experimental stuff

#define stringify(x)  #x

#define	QUAKE_GAME			// as opposed to utilities

#define HEXEN2_VERSION		1.12

//define	PARANOID			// speed sapping error checking

#ifdef QUAKE2
#define	GAMENAME	"data1"		// directory to look in by default
#else
#define	GAMENAME	"data1"
#endif

#include "matrix.h"

#if defined(_WIN32) && !defined(WINDED)
#if defined(_M_IX86)
#define __i386__	1
#endif
#endif

#ifdef __i386__
#define id386	1
#else
#define id386	0
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


// Timing macros
#define HX_FRAME_TIME		0.05
#define HX_FPS				20


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

//#define	MAX_MSGLEN		8000		// max length of a reliable message
//#define	MAX_MSGLEN		16000		// max length of a reliable message
#define	MAX_MSGLEN		20000		// for mission pack tibet2

#define	MAX_DATAGRAM	1024		// max length of unreliable message
//#define	MAX_DATAGRAM	2048		// max length of unreliable message  TEMP: This only for E3

//
// per-level limits
//
#define	MAX_EDICTS		2048		// FIXME: ouch! ouch! ouch!

#define	MAX_MODELS		512			// Sent over the net as a word
#define	MAX_SOUNDS		512			// Sent over the net as a byte

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
//#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
//#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
//#define	STAT_SHELLS			6
//#define	STAT_NAILS			7
//#define	STAT_ROCKETS		8
//#define	STAT_CELLS			9
//#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
//#define	STAT_BLUEMANA			15
//#define	STAT_GREENMANA			16
//#define	STAT_EXPERIENCE		17


#define	MAX_INVENTORY			15		// Max inventory array size

// stock defines

#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8
#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define	IT_INVISIBILITY			524288
#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

#define ART_HASTE					1
#define ART_INVINCIBILITY			2
#define ART_TOMEOFPOWER				4
#define ART_INVISIBILITY			8
#define ARTFLAG_FROZEN				128
#define ARTFLAG_STONED				256
#define ARTFLAG_DIVINE_INTERVENTION 512

//===========================================

#define NUM_CLASSES					4
#define ABILITIES_STR_INDEX			400

#ifdef DEMOBUILD
#define	MAX_SCOREBOARD		8
#else
#define	MAX_SCOREBOARD		16
#endif

#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"

//#define BASE_ENT_ON		1
//#define BASE_ENT_SENT	2

typedef struct entity_state_s {
	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;

#if RJNET
	byte	ClearCount[32];
#endif
} entity_state_t;

typedef struct entity_state2_s {
	byte	flags;
	short	index;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state2_t;

typedef struct entity_state3_s {
	byte	flags;

	vec3_t	origin;
	vec3_t	angles;
	short	modelindex;
	byte	frame;
	byte	colormap;
	byte	skin;
	byte	effects;
	byte	scale;
	byte	drawflags;
	byte	abslight;
} entity_state3_t;

#define MAX_CLIENT_STATES 150
#define MAX_FRAMES 5
#define MAX_CLIENTS 8
#define CLEAR_LIMIT 2

#define ENT_STATE_ON		1
#define ENT_CLEARED			2

typedef struct client_frames_s {
	entity_state2_t states[MAX_CLIENT_STATES];
	//	unsigned long frame;
	//	unsigned long flags;
	int count;
} client_frames_t;

typedef struct client_frames2_s {
	entity_state2_t states[MAX_CLIENT_STATES * 2];
	int count;
} client_frames2_t;

typedef struct client_state2_s {
	client_frames_t frames[MAX_FRAMES + 2]; // 0 = base, 1-max = proposed, max+1 = too late
} client_state2_t;


#include "wad.h"
#include "draw.h"
#include "cvar.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "cl_effect.h"
#include "progs.h"
#include "client.h"
#include "server.h"
#include "model.h"
#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"
#include "glquake.h"


//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct quakeparms_s {
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int		argc;
	char	**argv;
} quakeparms_t;


//=============================================================================



extern qboolean noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

extern	qboolean	host_initialized;		// true if into command execution
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at start of every frame, never reset

void Host_ClearMemory (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown (void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (float time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
//  an fullscreen DIB focus gain/loss
extern int			current_skill;		// skill level for currently loaded level (in case
//  the user changes the cvar while the level is
//  running, this reflects the level actually in use)

extern qboolean		isDedicated;

extern int			minimum_memory;

extern int			sv_kingofhill;
extern qboolean		intro_playing;
extern qboolean		skip_start;
extern int			num_intro_msg;
extern qboolean		check_bottom;


// Midi Stuff
// Put it here cuz I didn't want to make all the files
// include the window components
qboolean MIDI_Init (void);
void MIDI_Cleanup (void);
void MIDI_Play (char *Name);
void MIDI_Stop (void);
void MIDI_Pause (void);
void MIDI_Resume (void);
void MIDI_Loop (int NewValue);

#endif

