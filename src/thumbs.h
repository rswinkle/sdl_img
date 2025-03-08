
typedef struct thumb_state
{
	int w;
	int h;
	SDL_Texture* tex;
} thumb_state;

#define RESIZE(x) ((x+1)*2)

CVEC_NEW_DECLS2(thumb_state)
