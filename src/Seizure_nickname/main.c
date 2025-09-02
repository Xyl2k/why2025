// sdk_apps/Seizure_nickname/main.c
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "SDL3/SDL.h"

// ========= RNG =========
static uint32_t s = 123456789u;
static inline uint32_t rnd(void){ s = (1103515245u * s + 12345u) & 0x00FFFFFFu; return s; }

// ========= Colors (no black) =========
static void pick_rgb(uint8_t *r, uint8_t *g, uint8_t *b){
    const int gs = 160;
    int v = (int)(rnd()%7) + 1; // 1..7
    *r = ((v>>2)&1)? gs:0; *g = ((v>>1)&1)? gs:0; *b = (v&1)? gs:0;
}

// ========= 5x7 bitmap font (ASCII 32..95) =========
static const uint8_t F[64][5] = {
  {0,0,0,0,0},{0,0,0x5F,0,0},{0,7,0,7,0},{0x14,0x7F,0x14,0x7F,0x14},
  {0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,8,0x64,0x62},{0x36,0x49,0x55,0x22,0x50},
  {0,5,3,0,0},{0,0x1C,0x22,0x41,0},{0,0x41,0x22,0x1C,0},{0x14,8,0x3E,8,0x14},
  {8,8,0x3E,8,8},{0,0x50,0x30,0,0},{8,8,8,8,8},{0,0x60,0x60,0,0},{0x20,0x10,8,4,2},
  {0x3E,0x51,0x49,0x45,0x3E},{0,0x42,0x7F,0x40,0},{0x62,0x51,0x49,0x49,0x46},
  {0x22,0x41,0x49,0x49,0x36},{0x18,0x14,0x12,0x7F,0x10},{0x2F,0x49,0x49,0x49,0x31},
  {0x3E,0x49,0x49,0x49,0x32},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},
  {0x26,0x49,0x49,0x49,0x3E},{0,0x36,0x36,0,0},{0,0x56,0x36,0,0},
  {8,0x14,0x22,0x41,0},{0x14,0x14,0x14,0x14,0x14},{0,0x41,0x22,0x14,8},
  {2,1,0x59,9,6},{0x3E,0x41,0x5D,0x55,0x1E},{0x7E,0x11,0x11,0x11,0x7E},
  {0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},
  {0x7F,0x49,0x49,0x49,0x41},{0x7F,9,9,9,1},{0x3E,0x41,0x49,0x49,0x7A},
  {0x7F,8,8,8,0x7F},{0,0x41,0x7F,0x41,0},{0x20,0x40,0x41,0x3F,1},
  {0x7F,8,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,2,0x0C,2,0x7F},
  {0x7F,4,8,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},{0x7F,9,9,9,6},
  {0x3E,0x41,0x51,0x21,0x5E},{0x7F,9,0x19,0x29,0x46},{0x46,0x49,0x49,0x49,0x31},
  {1,1,0x7F,1,1},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
  {0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,8,0x14,0x63},{7,8,0x70,8,7},
  {0x61,0x51,0x49,0x45,0x43},{0,0x7F,0x41,0x41,0},{2,4,8,0x10,0x20},
  {0,0x41,0x41,0x7F,0},{4,2,1,2,4},{0x40,0x40,0x40,0x40,0x40},
};
static float text_wf(const char*s,float sc){ int n=(int)strlen(s); return n? (float)(n*5+(n-1))*sc:0.f; }

// ========= small math helpers =========
static inline float clampf(float v, float lo, float hi){ return v<lo?lo : (v>hi?hi : v); }

// Fit scale so text fits width even at pulse peak and extra multiplier (for DECT).
static float fit_scale_to_width(const char* txt, int screen_w,
                                float pulse_amp, float extra_mul, float margin_px)
{
    float unit_w = text_wf(txt, 1.0f);
    if (unit_w <= 0.0f) return 6.0f;
    float max_pulse = 1.0f + pulse_amp;
    float avail = (float)screen_w - margin_px*2.0f;
    float sc = avail / (unit_w * max_pulse * extra_mul);
    return clampf(sc, 3.0f, 18.0f);
}

// ========= font rendering =========
static void draw_glyphf(SDL_Renderer*r,char ch,float x,float y,float sc,SDL_Color col){
    if(ch<32||ch>95) ch='_';
    const uint8_t *c=F[ch-32];
    SDL_SetRenderDrawColor(r,col.r,col.g,col.b,255);
    for(int cx=0;cx<5;++cx){ uint8_t bits=c[cx];
        for(int cy=0;cy<7;++cy) if(bits&(1u<<cy)){
            SDL_FRect fr={ x + cx*sc, y + cy*sc, sc, sc };
            SDL_RenderFillRect(r,&fr);
        }
    }
}

static void draw_text_plain(SDL_Renderer*r,const char*s,float x,float y,float sc,SDL_Color fg,SDL_Color outline){
    float w=text_wf(s,sc), h=7.f*sc;
    // crisp outline box
    SDL_SetRenderDrawColor(r,outline.r,outline.g,outline.b,255);
    SDL_FRect o={ x-2.f, y-2.f, w+4.f, h+4.f }; SDL_RenderFillRect(r,&o);
    o.x=x-1.f; o.y=y-1.f; o.w=w+2.f; o.h=h+2.f; SDL_RenderFillRect(r,&o);
    float pen=x; for(const char*p=s;*p;++p){ draw_glyphf(r,*p,pen,y,sc,fg); pen+=(5.f+1.f)*sc; }
}

// DECT "flag" wave: background + glyphs share identical wave per column
static void draw_text_flag(SDL_Renderer*r,const char*s,float x,float y,float sc,
                           SDL_Color fg,SDL_Color outline,float t){
    float tw=text_wf(s,sc), th=7.f*sc;
    // smoother, longer wavelength, a bit slower, slightly smaller amplitude
    const float WAVE_L = 20.0f;   // wavelength divisor (bigger => longer waves)
    const float WAVE_W = 2.2f;    // angular speed
    const float WAVE_A = 8.0f;    // amplitude in px

    // Waving background "cartouche"
    for(int i=0;i<(int)tw;i++){
        float colx = x + (float)i;
        float waveY = SDL_sinf((colx/WAVE_L) + t*WAVE_W) * WAVE_A;
        SDL_SetRenderDrawColor(r,outline.r,outline.g,outline.b,255);
        SDL_FRect strip = { colx-1.f, y-3.f+waveY, 2.f, th+6.f };
        SDL_RenderFillRect(r, &strip);
    }

    // Waving
    float pen=x;
    for(const char*p=s;*p;++p){
        if(*p<32||*p>95){ pen+=(5.f+1.f)*sc; continue; }
        const uint8_t *c=F[*p-32];
        for(int cx=0;cx<5;++cx){
            float colx = pen + cx*sc;
            float waveY = SDL_sinf((colx/WAVE_L) + t*WAVE_W) * WAVE_A;
            for(int cy=0;cy<7;++cy){
                if(c[cx]&(1<<cy)){
                    SDL_FRect fr={ colx, y + cy*sc + waveY, sc, sc };
                    SDL_SetRenderDrawColor(r,fg.r,fg.g,fg.b,255);
                    SDL_RenderFillRect(r,&fr);
                }
            }
        }
        pen+=(5.f+1.f)*sc;
    }
}

// ========= tiny file loader (BadgeVMS-safe), uppercase =========
static bool try_read(const char *path, char *out, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t n = fread(out, 1, cap - 1, f);
    fclose(f);
    out[n] = 0;
    while (n && (out[n-1]=='\n'||out[n-1]=='\r')) out[--n]=0;
    for (size_t i=0; out[i]; ++i) out[i]=(char)toupper((unsigned char)out[i]);
    return true;
}

static void load_text_into(char *dst,size_t cap,const char*app_id,const char*fname,const char*fallback){
    char buf[256];
    // same folder as ELF
    snprintf(buf,sizeof(buf),"APPS:[%s]%s",app_id,fname);
    if(try_read(buf,dst,cap)) return;
    // absolute + relative fallbacks
    snprintf(buf,sizeof(buf),"/BADGEVMS/APPS/%s/%s",app_id,fname);
    if(try_read(buf,dst,cap)) return;
    snprintf(buf,sizeof(buf),"BADGEVMS/APPS/%s/%s",app_id,fname);
    if(try_read(buf,dst,cap)) return;
    if(try_read(fname,dst,cap)) return;
    SDL_strlcpy(dst,fallback,cap);
}

// ========= main loop =========
int main(void){
    if(SDL_Init(0)<0) if(SDL_Init(SDL_INIT_VIDEO)<0) return 1;

    SDL_DisplayID did=SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *m=SDL_GetCurrentDisplayMode(did);
    int W=m?m->w:720, H=m?m->h:720;

    SDL_Window*win=SDL_CreateWindow("Seizure_nickname",W,H,SDL_WINDOW_FULLSCREEN);
    if(!win) return 2;
    SDL_Renderer*ren=SDL_CreateRenderer(win,NULL);
    if(!ren){ SDL_DestroyWindow(win); return 3; }

    // Load texts
    char nick[128], dect[128];
    load_text_into(nick,sizeof(nick),"Seizure_nickname","nickname.txt","SAMPLE TEXT");
    load_text_into(dect,sizeof(dect),"Seizure_nickname","dect.txt","DECT: XXXX");

    // Grid covers entire screen
    int cell=40, cols=(W+cell-1)/cell, rows=(H+cell-1)/cell;
    int cw=(W+cols-1)/cols, ch=(H+rows-1)/rows;

    uint32_t t0=SDL_GetTicks();
    const float pulse_amp = 0.12f;   // ±12% pulse
    const float dect_mul  = 1.25f;   // DECT larger than nickname but still fitted
    const float side_margin = 12.0f; // side padding in px

    bool run=true;
    while(run){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_EVENT_QUIT) run=false;
            if(e.type==SDL_EVENT_KEY_DOWN)
                if(e.key.key==SDLK_ESCAPE||e.key.key==SDLK_Q) run=false;
        }

        uint32_t ms=SDL_GetTicks()-t0;
        float t = ms * 0.001f;
        bool use_dect=((ms/10000)%2)==1;
        const char*txt=use_dect?dect:nick;

        // Background mosaic
        for(int r=0;r<rows;++r){
            for(int c=0;c<cols;++c){
                int x=c*cw, y=r*ch;
                int w=cw-(int)(rnd()%(cw/6+1));
                int h=ch-(int)(rnd()%(ch/6+1));
                if(w<cw*3/4) w=cw*3/4;
                if(h<ch*3/4) h=ch*3/4;
                int ox=x+(cw-w)/2, oy=y+(ch-h)/2;
                uint8_t R,G,B; pick_rgb(&R,&G,&B);
                SDL_SetRenderDrawColor(ren,R,G,B,255);
                SDL_FRect fr={(float)ox,(float)oy,(float)w,(float)h};
                SDL_RenderFillRect(ren,&fr);
            }
        }
        // Big pops
        for(int i=0;i<10;++i){
            int w=W/3+(int)(rnd()%(W/3));
            int h=H/3+(int)(rnd()%(H/3));
            int x=(int)(rnd()%(W-w)), y=(int)(rnd()%(H-h));
            uint8_t R,G,B; pick_rgb(&R,&G,&B);
            SDL_SetRenderDrawColor(ren,R,G,B,255);
            SDL_FRect fr={(float)x,(float)y,(float)w,(float)h};
            SDL_RenderFillRect(ren,&fr);
        }

        // Choose a base that FITS even at pulse peak (+ DECT multiplier)
        float base = fit_scale_to_width(txt, W, pulse_amp, use_dect ? dect_mul : 1.0f, side_margin);
        // Apply pulse; for DECT, apply multiplier
        float sc = base * (1.0f + pulse_amp * SDL_sinf(t*2.0f*3.14159f*0.8f));
        if (use_dect) sc *= dect_mul;

        float tw=text_wf(txt,sc), th=7.f*sc;
        float nx=(W-tw)*0.5f;

        // center Y but clamp so waving cartouche never leaves the screen
        float wave_amp = use_dect ? 8.0f : 0.0f;   // must match draw_text_flag amplitude
        float outline_pad = 3.0f + 2.0f;           // outline + tiny safety
        float ny_center = (H - th) * 0.5f;
        float ny_min = outline_pad + wave_amp;
        float ny_max = (float)H - (th + outline_pad + wave_amp);
        float ny = clampf(ny_center, ny_min, ny_max);

        // Colors + contrast outline
        uint8_t r8,g8,b8; pick_rgb(&r8,&g8,&b8);
        SDL_Color fg={r8,g8,b8,255};
        int lum=r8*3+g8*6+b8;
        SDL_Color outline=(lum>700)?(SDL_Color){0,0,0,255}:(SDL_Color){255,255,255,255};

        if(use_dect) draw_text_flag (ren,txt,nx,ny,sc,fg,outline,t);
        else         draw_text_plain(ren,txt,nx,ny,sc,fg,outline);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
