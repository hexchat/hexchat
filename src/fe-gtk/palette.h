extern GdkColor colors[];

#define COL_MARK_FG 32
#define COL_MARK_BG 33
#define COL_FG 34
#define COL_BG 35
#define COL_MARKER 36
#define COL_NEW_DATA 37
#define COL_HILIGHT 38
#define COL_NEW_MSG 39
#define COL_AWAY 40

void palette_alloc (GtkWidget * widget);
void palette_load (void);
void palette_save (void);
