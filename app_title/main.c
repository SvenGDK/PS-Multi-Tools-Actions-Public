#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>

// Paths
#define FONT_FILE "/preinst/common/font/SST-Medium.otf"
#define INPUT_ROOT "/user/appmeta/"
#define TARGET_IMAGE_NAME "icon0.png"

// PS5 Notification Struct
typedef struct notify_request {
    char useless1[45];
    char message[3075];
} notify_request_t;

int sceKernelSendNotificationRequest(int device, notify_request_t* req, size_t size, int flags);

// PS5 Notification Function
static void send_notification_fmt(const char *fmt, ...) {
    notify_request_t req = {0};
    va_list ap; va_start(ap, fmt);
    vsnprintf(req.message, sizeof(req.message), fmt, ap);
    va_end(ap);
    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

static int is_directory(const char *p) {
    struct stat st; return stat(p, &st)==0 && S_ISDIR(st.st_mode);
}

// Draws text on top of the image
void draw_text(unsigned char *img, int w, int h, const char *text) {
    // 1) Download the full font.
    FILE *f = fopen(FONT_FILE, "rb"); if(!f){ send_notification_fmt("Error: Font %s missing", FONT_FILE); return; }
    fseek(f,0,SEEK_END); size_t sz=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char *font = malloc(sz); fread(font,1,sz,f); fclose(f);

    // 2) Prepare the font
    stbtt_fontinfo fi;
    stbtt_InitFont(&fi, font, 0);

    // 3) Choose size
    float scale = stbtt_ScaleForPixelHeight(&fi, 60);
    int ascent,descent,lineskip;
    stbtt_GetFontVMetrics(&fi, &ascent, &descent, &lineskip);
    ascent = (int)(ascent * scale);

    // 4) Horizontal alignment
    int total_w = 0;
    for(const char*c=text;*c;c++){
        int ax,ay; stbtt_GetCodepointHMetrics(&fi, *c, &ax, &ay);
        total_w += (int)(ax*scale);
    }
    int x = (w - total_w)/2, y = ascent + 10;

    // 5) Draw each letter.
    for(const char*c=text;*c;c++){
        int ax,ay; stbtt_GetCodepointHMetrics(&fi, *c, &ax, &ay);
        int c_x0,c_y0,c_x1,c_y1;
        stbtt_GetCodepointBitmapBox(&fi, *c, scale, scale,&c_x0,&c_y0,&c_x1,&c_y1);
        int byte_w = c_x1 - c_x0;
        int byte_h = c_y1 - c_y0;
        unsigned char *bmp = stbtt_GetCodepointBitmap(&fi,0,scale,*c,&byte_w,&byte_h,0,0);
        // Copy pixels
        for(int by=0;by<byte_h;by++){
          for(int bx=0;bx<byte_w;bx++){
            int px = x + c_x0 + bx;
            int py = y + c_y0 + by;
            if(px>=0 && px<w && py>=0 && py<h){
              unsigned char v = bmp[by*byte_w+bx];
              int idx = (py*w + px)*4;
              img[idx+0] = img[idx+1] = img[idx+2] = v; 
              img[idx+3] = 255;
            }
          }
        }
        stbtt_FreeBitmap(bmp, NULL);
        x += (int)(ax * scale);
    }
    free(font);
}

// Draw ID vertically on the left side of the image
void draw_text_left(unsigned char *img, int w, int h, const char *text) {

    FILE *f = fopen(FONT_FILE, "rb");
    if (!f) {
        send_notification_fmt("Error: Font %s missing", FONT_FILE);
        return;
    }
	
    fseek(f, 0, SEEK_END); 
    size_t sz = ftell(f); 
    fseek(f, 0, SEEK_SET);
    unsigned char *font = malloc(sz); 
    fread(font, 1, sz, f); 
    fclose(f);

    stbtt_fontinfo fi;
    stbtt_InitFont(&fi, font, 0);

    float scale = stbtt_ScaleForPixelHeight(&fi, 60);

    int total_h = 0;
    int max_w = 0;
    for (const char *c = text; *c; c++) {
        int c_x0, c_y0, c_x1, c_y1;
        stbtt_GetCodepointBitmapBox(&fi, *c, scale, scale, &c_x0, &c_y0, &c_x1, &c_y1);
        int glyph_h = c_y1 - c_y0;
        int glyph_w = c_x1 - c_x0;
        total_h += glyph_h;
        if (glyph_w > max_w)
            max_w = glyph_w;
    }
    
    int start_y = (h - total_h) / 2;
    int pos_x = 10;
    int pos_y = start_y;
    
    for (const char *c = text; *c; c++) {
        int c_x0, c_y0, c_x1, c_y1;
        stbtt_GetCodepointBitmapBox(&fi, *c, scale, scale, &c_x0, &c_y0, &c_x1, &c_y1);
        int glyph_w = c_x1 - c_x0;
        int glyph_h = c_y1 - c_y0;
        unsigned char *bmp = stbtt_GetCodepointBitmap(&fi, 0, scale, *c, &glyph_w, &glyph_h, 0, 0);
        
        for (int by = 0; by < glyph_h; by++){
            for(int bx = 0; bx < glyph_w; bx++){
                int px = pos_x + c_x0 + bx;
                int py = pos_y + c_y0 + by;
                if (px >= 0 && px < w && py >= 0 && py < h) {
                    unsigned char v = bmp[by * glyph_w + bx];
                    int idx = (py * w + px) * 4;
                    img[idx + 0] = img[idx + 1] = img[idx + 2] = v;
                    img[idx + 3] = 255;
                }
            }
        }
        stbtt_FreeBitmap(bmp, NULL);
        pos_y += glyph_h;
    }
    free(font);
}

// Draw ID on the right side of the image
void draw_text_right(unsigned char *img, int w, int h, const char *text) {

    FILE *f = fopen(FONT_FILE, "rb");
    if (!f) {
        send_notification_fmt("Erro: Font %s missing", FONT_FILE);
        return;
    }
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *font = malloc(sz);
    fread(font, 1, sz, f);
    fclose(f);

    stbtt_fontinfo fi;
    stbtt_InitFont(&fi, font, 0);

    float scale = stbtt_ScaleForPixelHeight(&fi, 60);

    int total_h = 0;
    int max_w = 0;
    for (const char *c = text; *c; c++) {
        int c_x0, c_y0, c_x1, c_y1;
        stbtt_GetCodepointBitmapBox(&fi, *c, scale, scale, &c_x0, &c_y0, &c_x1, &c_y1);
        int glyph_h = c_y1 - c_y0;
        int glyph_w = c_x1 - c_x0;
        total_h += glyph_h;
        if (glyph_w > max_w)
            max_w = glyph_w;
    }
    
    int start_y = (h - total_h) / 2;
    int margin = 10;
    int pos_x = w - max_w - margin;
    int pos_y = start_y;
    
    for (const char *c = text; *c; c++) {
        int c_x0, c_y0, c_x1, c_y1;
        stbtt_GetCodepointBitmapBox(&fi, *c, scale, scale, &c_x0, &c_y0, &c_x1, &c_y1);
        int glyph_w = c_x1 - c_x0;
        int glyph_h = c_y1 - c_y0;
        unsigned char *bmp = stbtt_GetCodepointBitmap(&fi, 0, scale, *c, &glyph_w, &glyph_h, 0, 0);
        
        for (int by = 0; by < glyph_h; by++){
            for (int bx = 0; bx < glyph_w; bx++){
                int px = pos_x + c_x0 + bx;
                int py = pos_y + c_y0 + by;
                if (px >= 0 && px < w && py >= 0 && py < h) {
                    unsigned char v = bmp[by * glyph_w + bx];
                    int idx = (py * w + px) * 4;
                    img[idx + 0] = img[idx + 1] = img[idx + 2] = v;
                    img[idx + 3] = 255;
                }
            }
        }
        stbtt_FreeBitmap(bmp, NULL);
        pos_y += glyph_h;  // add spacing if needed: pos_y += glyph_h + 2;
    }
    free(font);
}

int main(){
    send_notification_fmt("Loading App IDs...");
    DIR *d = opendir(INPUT_ROOT); struct dirent *e;
    if(!d){ send_notification_fmt("Error: Cannot open %s", INPUT_ROOT); return 1; }
    while((e=readdir(d))){
        if(e->d_type==DT_DIR && e->d_name[0]!='.'){
            char p[512],imgpath[512];
            snprintf(p,sizeof(p),"%s/%s",INPUT_ROOT,e->d_name);
            if(is_directory(p)){
                snprintf(imgpath,sizeof(imgpath),"%s/%s/%s",INPUT_ROOT,e->d_name,TARGET_IMAGE_NAME);
                int w,h,ch; unsigned char *img = stbi_load(imgpath,&w,&h,&ch,4);
                if(img){
                    draw_text(img,w,h,e->d_name);
                    stbi_write_png(imgpath,w,h,4,img,w*4);
                    stbi_image_free(img);
                    send_notification_fmt("Found: %s", e->d_name);
                } else {
                    send_notification_fmt("Warning: Loading %s failed", e->d_name);
                }
            }
        }
    }
    closedir(d);
    send_notification_fmt("All Done!");
    return 0;
}
