#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_image.h>

#include "vlak.h"

#define RIGHT	0
#define UP	1
#define LEFT	2
#define DOWN	3
#define STOP	4

#define IMGSIZE	32

#define NUM_SOUNDS 5

/* PATH configured in Makefile */
//#define RESPATH "res"

static struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

static void mixaudio(void *unused, Uint8 *stream, int len) {
	int i;
	Uint32 amount;

	for (i=0; i<NUM_SOUNDS; ++i) {
		amount = (sounds[i].dlen-sounds[i].dpos);
		if ( amount > len ) {
			amount = len;
		}
		SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos], amount, SDL_MIX_MAXVOLUME);
		sounds[i].dpos += amount;
	}
}

static void audioInit(void) {
	SDL_AudioSpec fmt;

	/* Set 16-bit stereo audio at 22Khz */
	fmt.freq = 44100;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 512;        /* A good value for games */
	fmt.callback = mixaudio;
	fmt.userdata = NULL;
	
	/* Open the audio device and start playing sound! */
	if (SDL_OpenAudio(&fmt, NULL) < 0) {
		fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
	}
	SDL_PauseAudio(0);
}

static void LoadSound(int index, char *file) {
	SDL_AudioSpec wave;
	Uint8 *data;
	Uint32 dlen;
	SDL_AudioCVT cvt;
	/* Load the sound file and convert it to 16-bit stereo at 22kHz */
	if (SDL_LoadWAV(file, &wave, &data, &dlen) == NULL) {
		fprintf(stderr, "Couldn't load %s: %s\n", file, SDL_GetError());
		return;
	}
	SDL_BuildAudioCVT(&cvt, wave.format, wave.channels, wave.freq, AUDIO_S16, 2, 44100);
	cvt.buf = malloc(dlen*cvt.len_mult);
	memcpy(cvt.buf, data, dlen);
	cvt.len = dlen;
	SDL_ConvertAudio(&cvt);
	SDL_FreeWAV(data);
	
	if (sounds[index].data) {
		free(sounds[index].data);
	}
	SDL_LockAudio();
	sounds[index].data = cvt.buf;
	sounds[index].dlen = cvt.len_cvt;
	sounds[index].dpos = cvt.len_cvt;
	SDL_UnlockAudio();
}

static void PlaySound(int index) {
	SDL_LockAudio();
	sounds[index].dpos = 0;
	SDL_UnlockAudio();
}

static int done;
static int direction;
static int lokodir;
static int lokox, lokoy;
static int kolook;
static int anim, vrataanim, lokobumanim;
static int aktualnaScena;
static SDL_Surface *screen;
static SDL_Surface *loko, *lokobum, *stena, *vecivagony, *vrata;
static SDL_Event event;
static unsigned char heslo[6];
static int hesloindex;

static int mapa[12][20];

typedef struct vagon {
	int naklad;
	int smer;
	int x;
	int y;
} vagon_t;

static vagon_t vagony[200];
static int pocet_vagonov;

static void drawImg(SDL_Surface *screen, SDL_Surface *img, int ix, int iy, int jx, int jy) {
	SDL_Rect drect, srect;
	drect.x = IMGSIZE * ix;
	drect.y = IMGSIZE * iy;
	srect.x = IMGSIZE * jx;
	srect.y = IMGSIZE * jy;
	srect.w = IMGSIZE;
	srect.h = IMGSIZE;
	SDL_BlitSurface(img, &srect, screen, &drect);
	//boxColor(screen, drect.x, drect.y, IMGSIZE, IMGSIZE, random());
}

static void draw(SDL_Surface *screen) {
	int i, j;
	unsigned char tmps[255];
	if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0) return;
	
	boxColor(screen, 0, 0, screen->w, screen->h, 0x000000ff);
	/* scena */
	for (i = 0; i < 20; i++) {
		for (j = 0; j < 12; j++) {
			if (mapa[j][i] < 100) drawImg(screen, stena, i, j, mapa[j][i], 0);
			else if (mapa[j][i] < 1000) drawImg(screen, vecivagony, i, j, anim, mapa[j][i] - 100);
			else if (mapa[j][i] < 10000) drawImg(screen, vrata, i, j, vrataanim, 0);
			else if (mapa[j][i] == 10000) {
				mapa[j][i] = 0;
				lokox = i;
				lokoy = j;
			};
		}
	}
	
	/* naklad */
	for (i = 0; i < pocet_vagonov; i++) {
		drawImg(screen, vecivagony, vagony[i].x, vagony[i].y, vagony[i].smer + 3, vagony[i].naklad);
	}
	
	/* loko */
	if (lokobumanim == 100) {
		drawImg(screen, loko, lokox, lokoy, anim, lokodir);
	} else {
		drawImg(screen, lokobum, lokox, lokoy, lokobumanim, 0);
	}

	if (kolook) {
		sprintf(tmps, "HESLO: %s", hesla[aktualnaScena]);
		stringColor(screen, 33, 9, tmps, 0xffffffff);
	}
	sprintf(tmps, "%s", heslo);
	stringColor(screen, 33, 21, tmps, 0xffffffff);

	SDL_Flip(screen);
	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	printf("onTime\n");
}

static void control(void) {
	int lx = lokox;
	int ly = lokoy;
	int i, j, k, m;
	switch (direction) {
		case RIGHT :
			lokox++;
			break;
		case UP :
			lokoy--;
			break;
		case LEFT :
			lokox--;
			break;
		case DOWN :
			lokoy++;
			break;
	}
	if (mapa[lokoy][lokox] == VRA) {
		if (vrataanim) {
			kolook = 1;
			PlaySound(2);
		} else {
			lokobumanim = 0;
			lokox = lx;
			lokoy = ly;
			PlaySound(3);
			return;
		}
	}
	if (mapa[lokoy][lokox] == 1) {
		lokobumanim = 0;
		lokox = lx;
		lokoy = ly;
		PlaySound(3);
		return;
	}
	if (pocet_vagonov && (lokox == vagony[0].x) && (lokoy == vagony[0].y)) {
		lokobumanim = 0;
		lokox = lx;
		lokoy = ly;
		PlaySound(3);
		return;
	}
	for (i = 2; i < pocet_vagonov - 1; i++) {
		if ((lokox == vagony[i].x) && (lokoy == vagony[i].y)) {
			lokobumanim = 0;
			lokox = lx;
			lokoy = ly;
			PlaySound(3);
			return;
		}
	}
	m = pocet_vagonov;
	if ((mapa[lokoy][lokox] >= 100) && (mapa[lokoy][lokox] < 1000)) {
		PlaySound(4);
		if (pocet_vagonov) {
			vagony[pocet_vagonov].x = vagony[pocet_vagonov - 1].x;
			vagony[pocet_vagonov].y = vagony[pocet_vagonov - 1].y;
		} else {
			vagony[0].x = lx;
			vagony[0].y = ly;
		}
		vagony[pocet_vagonov].smer = 0;
		vagony[pocet_vagonov].naklad = mapa[lokoy][lokox] - 100;
		pocet_vagonov++;
		mapa[lokoy][lokox] = 0;
		k = 0;
		for (i = 0; i < 20; i++) {
			for (j = 0; j < 12; j++) {
				if ((mapa[j][i] >= 100) && (mapa[j][i] < 1000)) k++;
			}
		}
		if (k == 0) {
			vrataanim = 1;
			PlaySound(1);
		}
	}
	while (m) {
		vagony[m].x = vagony[m - 1].x;
		vagony[m].y = vagony[m - 1].y;
		vagony[m].smer = vagony[m - 1].smer;
		m--;
	}
	vagony[0].x = lx;
	vagony[0].y = ly;
	vagony[0].smer = lokodir;
	if (direction < 4) {
		lokodir = direction;
		PlaySound(0);
	}
}

static int loadScena(int scena) {
	int i, j;
	direction = STOP;
	lokodir = RIGHT;
	anim = 0; vrataanim = 0; lokobumanim = 100;
	pocet_vagonov = 0;
	lokox = 0; lokoy = 0;
	kolook = 0;
	for (i = 0; i < 20; i++) {
		for (j = 0; j < 12; j++) {
			if (sceny[scena][j][i] == 10000) {
				mapa[j][i] = 0;
				lokox = i;
				lokoy = j;
			} else {
				mapa[j][i] = sceny[scena][j][i];
			}
		}
	}
}

static int onTime = 0;

Uint32 timCallBack(Uint32 interval, void *param) {
	onTime = 1;
	return interval;
}

int main(int argc, char *argv[]) {
	SDL_TimerID tim;
	hesloindex = 0;
	heslo[0] = 0;
	aktualnaScena = 0;
	done = 1;
	loadScena(aktualnaScena);
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) { printf("init error: %s\n", SDL_GetError()); exit(1); }
	//screen = SDL_SetVideoMode(640, 384, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	screen = SDL_SetVideoMode(640, 384, 24, SDL_HWSURFACE);
	loko = IMG_Load(RESPATH"img/loko.png");
	lokobum = IMG_Load(RESPATH"img/loko-bum.png");
	stena = IMG_Load(RESPATH"img/stena.png");
	vrata = IMG_Load(RESPATH"img/vrata.png");
	vecivagony = IMG_Load(RESPATH"img/veci-vagony.png");
	printf("loko: %d %d\n", (int)loko->w, (int)loko->h);
	audioInit();
	LoadSound(0, RESPATH"sounds/loko.wav");
	LoadSound(1, RESPATH"sounds/bell.wav");
	LoadSound(2, RESPATH"sounds/horn.wav");
	LoadSound(3, RESPATH"sounds/crash.wav");
	LoadSound(4, RESPATH"sounds/vagon.wav");
	SDL_ShowCursor(SDL_DISABLE);
	tim = SDL_AddTimer(150, timCallBack, NULL);
	int keyantirepeat = 1;
	while(done){
		SDL_PollEvent(&event);
		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
				case SDLK_ESCAPE :
					done = 0;
					break;
				case SDLK_RIGHT :
					direction = RIGHT;
					break;
				case SDLK_UP :
					direction = UP;
					break;
				case SDLK_LEFT :
					direction = LEFT;
					break;
				case SDLK_DOWN :
					direction = DOWN;
					break;
				case SDLK_RETURN :
				case SDLK_SPACE :
					if (hesloindex) {
						int i;
						for (i = 0; i < 50; i++) {
							if(strncmp(heslo, hesla[i], 5) == 0) {
								aktualnaScena = i + 1;
								loadScena(aktualnaScena);
							}
						}
						hesloindex = 0;
						heslo[hesloindex] = 0;
					} else if (kolook && (aktualnaScena < 50)) {
						loadScena(++aktualnaScena);
					} else {
						loadScena(aktualnaScena);
					}
					
					break;
				case SDLK_BACKSPACE :
					heslo[--hesloindex] = 0;
					break;
				default :
					if ((event.key.keysym.sym >= 'a') && (event.key.keysym.sym <= 'z') && keyantirepeat) {
						keyantirepeat = 0;
						if (hesloindex >= 5) {
							//over heslo
							hesloindex = 0;
							heslo[hesloindex] = 0;
						}
						heslo[hesloindex++] = (unsigned char)event.key.keysym.sym - 'a' + 'A';
						heslo[hesloindex] = 0;
					}
					break;
			}
		} else if (event.type == SDL_KEYUP) {
			keyantirepeat = 1;
		} else if (event.type == SDL_QUIT) {
			done = 0;
		}
		if (onTime) {
			onTime = 0;
			if (++anim >= 3) anim = 0;
			if (vrataanim && vrataanim < 5) vrataanim++;
			if (lokobumanim < 100) {
				lokobumanim++;
				if (lokobumanim == 10) lokobumanim = 7;
			} else {
				if (!anim) if (!kolook) control();
			}
			draw(screen);
		} else {
			SDL_Delay(50);
		}
	}
	SDL_RemoveTimer(tim);
	SDL_CloseAudio();
	SDL_Quit();
	return 0;
}
