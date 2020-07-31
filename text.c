#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "util/stb_truetype.h"

#include "shader.h"
#include "text.h"

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

static GLuint text_program;
static GLuint ftex;

static GLuint vao;
static GLuint vbo;

void text_init()
{
    shader_build_program(&text_program,
        "shaders/text.vert.glsl",
        "shaders/text.frag.glsl"
    );

    fread(ttf_buffer, 1, 1<<20, fopen("fonts/Roboto-Regular.ttf", "rb"));
    stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!

    glGenTextures(1, &ftex);
    glBindTexture(GL_TEXTURE_2D, ftex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA , GL_UNSIGNED_BYTE, temp_bitmap);

    glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void text_print(float x, float y, char *text)
{
    // assume orthographic projection with units = screen pixels, origin at top left
    glUseProgram(text_program);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, ftex);
    glActiveTexture(GL_TEXTURE0);
    glBegin(GL_QUADS);

    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
            glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y0);
            glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y0);
            glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y1);
            glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y1);
          }
          ++text;
    }
    glEnd();
}
