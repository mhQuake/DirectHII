
// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

#define MAX_DISC 18

extern	qpic_t	*draw_disc[MAX_DISC]; // also used on sbar

void Draw_Init (void);
void Draw_Character (int x, int y, unsigned int num);
void Draw_DebugChar (char num);
void Draw_Pic (int x, int y, qpic_t *pic);
void Draw_PicCropped (int x, int y, qpic_t *pic);
void Draw_TransPic (int x, int y, qpic_t *pic);
void Draw_TransPicCropped (int x, int y, qpic_t *pic);
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, int classnum, int color);
void Draw_ConsoleBackground (int lines);
void Draw_BeginDisc (void);
void Draw_EndDisc (void);
void Draw_TileClear (int x, int y, int w, int h);
void Draw_Fill (int x, int y, int w, int h, int c);
void Draw_FadeScreen (void);
void Draw_String (int x, int y, char *str);
void Draw_SmallCharacter (int x, int y, int num);
void Draw_SmallString (int x, int y, char *str);
qpic_t *Draw_PicFromWad (char *name);
qpic_t *Draw_CachePic (char *path);

