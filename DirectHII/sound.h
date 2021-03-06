// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct portable_samplepair_s {
	int left;
	int right;
} portable_samplepair_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct sfxcache_s {
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	width;
	int 	stereo;
	byte	data[1];		// variable sized
} sfxcache_t;

typedef struct sfx_s {
	char 	name[MAX_QPATH];
	int registration_sequence;
	sfxcache_t		*sndcache;
} sfx_t;

typedef struct dma_s {
	qboolean		gamealive;
	qboolean		soundalive;
	qboolean		splitbuffer;
	int				channels;
	int				samples;				// mono samples in buffer
	int				submission_chunk;		// don't mix less than this #
	int				samplepos;				// in mono samples
	int				samplebits;
	int				speed;
	unsigned char	*buffer;
} dma_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct channel_s {
	sfx_t	*sfx;			// sfx number
	int		leftvol;		// 0-255 volume
	int		rightvol;		// 0-255 volume
	int		end;			// end time in global paintsamples
	int 	pos;			// sample position in sfx
	//	int		looping;		// where to loop, -1 = no looping
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;
	vec3_t	origin;			// origin of sound effect
	float	dist_mult;		// distance multiplier (attenuation/clipK)
	float	master_vol;		// 0-255 master volume
} channel_t;

typedef struct wavinfo_s {
	int		rate;
	int		width;
	int		channels;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

void S_Init (void);
void S_Startup (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_UpdateSoundPos (int entnum, int entchannel, vec3_t origin);
void S_StopAllSounds (qboolean clear);
void S_ClearBuffer (void);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);

sfx_t *S_PrecacheSound (char *sample);
void S_TouchSound (char *sample);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);
void S_PaintChannels (int endtime);
void S_InitPaintChannels (void);
void S_PurgeAllSounds (void);
void S_FreeUnusedSounds (void);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel (int entnum, int entchannel);

// spatializes a channel
void SND_Spatialize (channel_t *ch);

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init (void);

// gets the current DMA position
int SNDDMA_GetDMAPos (void);

// shutdown the DMA xfer.
void SNDDMA_Shutdown (void);

// ====================================================================
// User-setable variables
// ====================================================================

#define	MAX_CHANNELS			128
#define	MAX_DYNAMIC_CHANNELS	8


extern	channel_t   channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds

extern	int			total_channels;

extern int		paintedtime;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern dma_t dma;
extern vec_t sound_nominal_clip_dist;

extern	cvar_t bgmvolume;
extern	cvar_t bgmtype;
extern	cvar_t volume;

extern qboolean	snd_initialized;

extern int		snd_blocked;

void S_LocalSound (char *s);
sfxcache_t *S_LoadSound (sfx_t *s);

wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength);

void SND_InitScaletable (void);
void SNDDMA_BeginPainting (void);
void SNDDMA_Submit (void);

void S_AmbientOff (void);
void S_AmbientOn (void);

void S_BlockSound (void);
void S_UnblockSound (void);

#endif
