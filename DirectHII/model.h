
#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mvertex_s {
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s {
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef struct texture_s {
	char		name[16];
	unsigned	width, height;
	int	texnum;
	struct msurface_s	*texturechain;	// for sorted drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
} texture_t;


#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_TRANSLUCENT	0x100
#define SURF_DRAWBLACK		0x200

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct medge_s {
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s {
	float		vecs[2][4];
	float		mipadjust;
	texture_t	*texture;
	int			flags;
} mtexinfo_t;

typedef struct msurface_s {
	int			order;			// order in the BSP file for sorting
	int			visframe;		// should be drawn when node is crossed

	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	int			firstvertex;
	int			lastvertex;
	int			numindexes;

	short		texturemins[2];
	short		extents[2];

	float		mins[3], maxs[3];

	int			light_s, light_t;	// gl lightmap coordinates

	struct	msurface_s	*texturechain;

	mtexinfo_t	*texinfo;

	// lighting info
	int			dlightframe;

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	byte		*samples;		// [numstyles*surfsize]
} msurface_t;


typedef struct mnode_s {
	// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	float		mins[3];		// for bounding box culling
	float		maxs[3];		// for bounding box culling

	struct mnode_s	*parent;

	// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];

	msurface_t *surfaces;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s {
	// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		mins[3];		// for bounding box culling
	float		maxs[3];		// for bounding box culling

	struct mnode_s	*parent;

	// leaf specific
	byte		*compressed_vis;
	efrag_t		*efrags;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct hull_s {
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s {
	short	width;
	short	height;
	float	up, down, left, right;
	int		firstvertex;
	int texnum;
} mspriteframe_t;

typedef struct mspritegroup_s {
	short			numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct mspriteframedesc_s {
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct msprite_s {
	short				type;
	short				maxwidth;
	short				maxheight;
	short				numframes;
	float				beamlength;		// remove?
	//void				*cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct maliasframedesc_s {
	int					firstpose;
	int					numposes;
	float				interval;
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct maliasgroupframedesc_s {
	trivertx_t			bboxmin;
	trivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct maliasgroup_s {
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

//this is only the GL version
typedef struct mtriangle_s {
	int					facesfront;
	unsigned short		vertindex[3];
	unsigned short		stindex[3];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct aliashdr_s {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

	int					numposes;
	int texnum[MAX_SKINS];
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS	2000
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048

//===================================================================

//
// Whole model
//

typedef enum { mod_brush, mod_sprite, mod_alias } modtype_t;

// EF_ changes must also be made in model.h

#define	EF_ROCKET		       1			// leave a trail
#define	EF_GRENADE		       2			// leave a trail
#define	EF_GIB			       4			// leave a trail
#define	EF_ROTATE		       8			// rotate (bonus items)
#define	EF_TRACER		      16			// green split trail
#define	EF_ZOMGIB		      32			// small blood trail
#define	EF_TRACER2			  64			// orange split trail + rotate
#define	EF_TRACER3			 128			// purple trail
#define  EF_FIREBALL		 256			// Yellow transparent trail in all directions
#define  EF_ICE				 512			// Blue-white transparent trail, with gravity
#define  EF_MIP_MAP			1024			// This model has mip-maps
#define  EF_SPIT			2048			// Black transparent trail with negative light
#define  EF_TRANSPARENT		4096			// Transparent sprite
#define  EF_SPELL           8192			// Vertical spray of particles
#define  EF_HOLEY		   16384			// Solid model with color 0
#define  EF_SPECIAL_TRANS  32768			// Translucency through the particle table
#define  EF_FACE_VIEW	   65536			// Poly Model always faces you
#define  EF_VORP_MISSILE  131072			// leave a trail at top and bottom of model
#define  EF_SET_STAFF     262144			// slowly move up and left/right
#define  EF_MAGICMISSILE  524288            // a trickle of blue/white particles with gravity
#define  EF_BONESHARD    1048576           // a trickle of brown particles with gravity
#define  EF_SCARAB       2097152           // white transparent particles with little gravity
#define  EF_ACIDBALL	 4194304			// Green drippy acid shit
#define  EF_BLOODSHOT	 8388608			// Blood rain shot trail

#define  EF_MIP_MAP_FAR	  0x1000000	// Set per frame, this model will use the far mip map

typedef struct model_s {
	char		name[MAX_QPATH];
	qboolean	needload;

	modtype_t	type;
	int			numframes;
	synctype_t	synctype;

	int			flags;

	// volume occupied by the model graphics
	vec3_t		mins, maxs;
	float		radius;

	// solid volume for clipping
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

	// brush model
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	dmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_t	**textures;

	byte		*visdata;
	byte		*lightdata;
	char		*entities;

	int			modelnum;

	int			numverts;

	// additional model data
	// to do: add brushhdr
	union {
		aliashdr_t	*aliashdr;
		msprite_t	*spritehdr;
	};
} model_t;


//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
void	Mod_TouchModel (char *name);

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);
float Mod_RadiusFromBounds (vec3_t mins, vec3_t maxs);
void Mod_CenterFromBounds (vec3_t mids, vec3_t mins, vec3_t maxs);
int Mod_BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct mplane_s *plane);

#endif	// __MODEL__

