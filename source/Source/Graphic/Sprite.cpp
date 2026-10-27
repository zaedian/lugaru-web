/*
Copyright (C) 2003, 2010 - Wolfire Games
Copyright (C) 2010-2017 - Lugaru contributors (see AUTHORS file)

This file is part of Lugaru, maintained as part of the Loupgarenne fork.
See README and AUTHORS for project details.

Lugaru is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Lugaru is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Lugaru.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Graphic/Sprite.hpp"

#include "Game.hpp"
#include "Objects/Person.hpp"

extern XYZ viewer;
extern float viewdistance;
extern float fadestart;
extern int environment;
extern float texscale;
extern Light light;
extern float multiplier;
extern float frameDeltaTime;
extern float gravity;
extern Terrain terrain;
extern int detail;
extern XYZ viewerfacing;
extern int bloodtoggle;
extern XYZ windvector;

// Web sprites use CPU billboards and client arrays; native keeps the historical path.
#ifdef __EMSCRIPTEN__
#include <cmath>

namespace {
struct SpriteBillboard {
    float vertex[4][3];
};

// Equivalent to M*T(position)*T(offset), then the historical eye-space T*Rz*S.
// Snow's rotating local offset is supplied separately, just as its old T(1,0,0).
SpriteBillboard spriteBillboard(const float* matrix, const XYZ& position, const XYZ& offset,
                               float rotation, float size, float growth)
{
    float center[3];
    for (int row = 0; row < 3; ++row) {
        center[row] = matrix[row] * position.x + matrix[4 + row] * position.y
                    + matrix[8 + row] * position.z + matrix[12 + row];
        center[row] += matrix[row] * offset.x + matrix[4 + row] * offset.y
                     + matrix[8 + row] * offset.z;
    }
    const float angle = rotation * (3.1415926535f / 180.f);
    const float cosine = std::cos(angle) * growth;
    const float sine = std::sin(angle) * growth;
    const float half = .5f * size;
    const float corners[4][2] = {{half, half}, {-half, half}, {half, -half}, {-half, -half}};
    SpriteBillboard result;
    for (int corner = 0; corner < 4; ++corner) {
        const float x = corners[corner][0];
        const float y = corners[corner][1];
        result.vertex[corner][0] = center[0] + cosine * x - sine * y;
        result.vertex[corner][1] = center[1] + sine * x + cosine * y;
        result.vertex[corner][2] = center[2];
    }
    return result;
}
}
#endif

// Temporary characterization only: no GL synchronization or per-particle timers.
#if defined(__EMSCRIPTEN__) && defined(LUGARU_WEB_EFFECT_DIAGNOSTICS) && LUGARU_WEB_EFFECT_DIAGNOSTICS
#include <emscripten/emscripten.h>
#include <cstdio>

namespace {
struct EffectDiagnostics {
    double started = emscripten_get_now();
    double drawMs = 0;
    double updateMs = 0;
    double geometryMs = 0;
    unsigned batches = 0;
    unsigned arrayVertices = 0;
    unsigned calls = 0;
    unsigned submitted = 0;
    unsigned updated = 0;
    unsigned binds = 0;
    unsigned blendCalls = 0;
    unsigned active[spritenumber] = {};
    unsigned spawned[spritenumber] = {};

    void report(double now)
    {
        const double elapsed = (now - started) / 1000;
        if (elapsed < 1 || !calls) {
            return;
        }
        // Calls are Sprite::Draw invocations, not necessarily rendered frames (stereo).
        std::printf("EFFECT PERF: seconds=%.2f calls_s=%.1f submit_call=%.1f update_call=%.1f "
                    "draw_cpu_ms=%.3f update_cpu_ms=%.3f binds_call=%.1f blend_calls=%.1f\n",
                    elapsed, calls / elapsed, double(submitted) / calls, double(updated) / calls,
                    drawMs / calls, updateMs / calls, double(binds) / calls, double(blendCalls) / calls);
        std::printf("EFFECT SECTIONS: geometry_cpu_ms=%.3f submit_cpu_ms=%.3f batches_call=%.1f array_vertices_call=%.1f\n",
                    geometryMs / calls, (drawMs - geometryMs) / calls, double(batches) / calls,
                    double(arrayVertices) / calls);
        static const char* names[spritenumber] = {
            "cloud", "blood", "flame", "smoke", "weaponflame", "impact", "snow",
            "shine", "bloodflame", "breath", "splinter"
        };
        std::printf("EFFECT TYPES: mean_active/spawn_s");
        for (int i = 0; i < spritenumber; ++i) {
            std::printf(" %s=%.1f/%.1f", names[i], double(active[i]) / calls, spawned[i] / elapsed);
        }
        std::printf("\n");
        *this = EffectDiagnostics();
    }
};
EffectDiagnostics effectDiagnostics;
}
#define EFFECT_DIAGNOSTIC(statement) do { statement; } while (0)
#else
#define EFFECT_DIAGNOSTIC(statement) do {} while (0)
#endif

// init statics
Texture Sprite::cloudtexture;
Texture Sprite::cloudimpacttexture;
Texture Sprite::bloodtexture;
Texture Sprite::flametexture;
Texture Sprite::bloodflametexture;
Texture Sprite::smoketexture;
Texture Sprite::snowflaketexture;
Texture Sprite::shinetexture;
Texture Sprite::splintertexture;
Texture Sprite::leaftexture;
Texture Sprite::toothtexture;

float Sprite::checkdelay = 0;

vector<std::unique_ptr<Sprite> > Sprite::sprites = vector<std::unique_ptr<Sprite> >();

//Functions
void Sprite::Draw()
{
#if defined(__EMSCRIPTEN__) && defined(LUGARU_WEB_EFFECT_DIAGNOSTICS) && LUGARU_WEB_EFFECT_DIAGNOSTICS
    const double drawStarted = emscripten_get_now();
#endif
    int k;
    static float M[16];
    static XYZ point;
    static float distancemult;
    static int lasttype;
    static int lastspecial;
    static int whichpatchx, whichpatchz;
    static XYZ start, end, colpoint;
    static bool check;
    static bool blend;
    static float tempmult;
    static XYZ difference;
    static float lightcolor[3];
    static float viewdistsquared = viewdistance * viewdistance;
    static XYZ tempviewer;

    tempviewer = viewer + viewerfacing * 6;
    check = 0;

    lightcolor[0] = light.color[0] * .5 + light.ambient[0];
    lightcolor[1] = light.color[1] * .5 + light.ambient[1];
    lightcolor[2] = light.color[2] * .5 + light.ambient[2];

    checkdelay -= frameDeltaTime * 10;

    if (checkdelay <= 0) {
        check = 1;
        checkdelay = 1;
    }

    lasttype = -1;
    lastspecial = -1;
    glEnable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    blend = 1;
    EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(0);
    glAlphaFunc(GL_GREATER, 0.0001);
#ifdef __EMSCRIPTEN__
    static std::vector<SpriteBillboard> billboards;
    unsigned batchSprites = 0;
    // Tightly packed attributes avoid FULL_ES2 uploading count*interleaved-stride
    // separately for each attribute (including bytes beyond the last field).
    static GLfloat arrayPositions[1024 * 6 * 3];
    static GLfloat arrayUVs[1024 * 6 * 2];
    static GLfloat arrayColors[1024 * 6 * 4];
    unsigned arrayVertexCount = 0;
    float arrayColor[4] = {};
    GLint savedArrayBuffer = 0;
    if (!sprites.empty()) {
        // GL4ES saves array pointers/enables and client texture selection here, but
        // not GL_ARRAY_BUFFER_BINDING. Restore that binding explicitly below.
        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
        glDisableClientState(GL_FOG_COORDINATE_ARRAY);
        GLint textureUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &textureUnits);
        for (GLint unit = 0; unit < textureUnits; ++unit) {
            glClientActiveTexture(GL_TEXTURE0 + unit);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    const auto flushArray = [&]() {
        if (!arrayVertexCount) {
            return;
        }
        glVertexPointer(3, GL_FLOAT, 0, arrayPositions);
        glTexCoordPointer(2, GL_FLOAT, 0, arrayUVs);
        glColorPointer(4, GL_FLOAT, 0, arrayColors);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(arrayVertexCount));
        EFFECT_DIAGNOSTIC(++effectDiagnostics.batches);
        EFFECT_DIAGNOSTIC(effectDiagnostics.arrayVertices += arrayVertexCount);
        arrayVertexCount = 0;
    };
    if (!sprites.empty()) {
#if defined(LUGARU_WEB_EFFECT_DIAGNOSTICS) && LUGARU_WEB_EFFECT_DIAGNOSTICS
        const double geometryStarted = emscripten_get_now();
#endif
        // Capture separately for each draw/eye. Restore this matrix after all submissions.
        glMatrixMode(GL_MODELVIEW);
        glGetFloatv(GL_MODELVIEW_MATRIX, M);
        glPushMatrix();
        glLoadIdentity();
        billboards.resize(sprites.size());
        for (size_t i = 0; i < sprites.size(); ++i) {
            const Sprite& sprite = *sprites[i];
            XYZ offset;
            offset = 0;
            if (sprite.type == flamesprite || sprite.type == weaponflamesprite || sprite.type == weaponshinesprite) {
                offset = viewer - sprite.position;
                Normalise(&offset);
                offset.x = offset.x * sprite.size / 4;
                offset.y = offset.y * sprite.size / 4;
                offset.z = offset.z * sprite.size / 4;
            }
            if (sprite.type == snowsprite) {
                // First column of R(rotation*.2, 0,.3,1), with the axis normalized.
                const float angle = static_cast<float>(sprite.rotation * .2) * (3.1415926535f / 180.f);
                const float inverseLength = 1.f / std::sqrt(.3f * .3f + 1.f);
                offset.x = std::cos(angle);
                offset.y = inverseLength * std::sin(angle);
                offset.z = -.3f * inverseLength * std::sin(angle);
            }
            float growth = 1;
            if ((sprite.type == flamesprite || sprite.type == weaponflamesprite ||
                 sprite.type == weaponshinesprite || sprite.type == bloodflamesprite) && sprite.alivetime < .14) {
                growth = sprite.alivetime / .14;
            }
            billboards[i] = spriteBillboard(M, sprite.position, offset, sprite.rotation, sprite.size, growth);
        }
        EFFECT_DIAGNOSTIC(effectDiagnostics.geometryMs += emscripten_get_now() - geometryStarted);
    }
#endif
#ifdef __EMSCRIPTEN__
    const auto setSpriteColor = [&](float red, float green, float blue, float alpha) {
        arrayColor[0] = red;
        arrayColor[1] = green;
        arrayColor[2] = blue;
        arrayColor[3] = alpha;
    };
#else
#define setSpriteColor glColor4f
#endif
    for (unsigned i = 0; i < sprites.size(); i++) {
        EFFECT_DIAGNOSTIC(++effectDiagnostics.active[sprites[i]->type]);
        EFFECT_DIAGNOSTIC(++effectDiagnostics.submitted);
#ifdef __EMSCRIPTEN__
        // The historical dispatcher changes render state only at type boundaries.
        if (arrayVertexCount && lasttype != sprites[i]->type) {
            flushArray();
            batchSprites = 0;
        }
#endif
        if (lasttype != sprites[i]->type) {
            switch (sprites[i]->type) {
                case cloudsprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    cloudtexture.bind();
                    if (!blend) {
                        blend = 1;
                        glAlphaFunc(GL_GREATER, 0.0001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                    break;
                case breathsprite:
                case cloudimpactsprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    cloudimpacttexture.bind();
                    if (!blend) {
                        blend = 1;
                        glAlphaFunc(GL_GREATER, 0.0001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                    break;
                case smoketype:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    smoketexture.bind();
                    if (!blend) {
                        blend = 1;
                        glAlphaFunc(GL_GREATER, 0.0001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                    break;
                case bloodsprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    bloodtexture.bind();
                    if (!blend) {
                        blend = 1;
                        glAlphaFunc(GL_GREATER, 0.0001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                    break;
                case splintersprite:
                    if (lastspecial != sprites[i]->special) {
                        if (sprites[i]->special == 0) {
                            EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                            splintertexture.bind();
                        }
                        if (sprites[i]->special == 1) {
                            EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                            leaftexture.bind();
                        }
                        if (sprites[i]->special == 2) {
                            EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                            snowflaketexture.bind();
                        }
                        if (sprites[i]->special == 3) {
                            EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                            toothtexture.bind();
                        }
                        if (!blend) {
                            blend = 1;
                            glAlphaFunc(GL_GREATER, 0.0001);
                            EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        }
                    }
                    break;
                case snowsprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    snowflaketexture.bind();
                    if (!blend) {
                        blend = 1;
                        glAlphaFunc(GL_GREATER, 0.0001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    }
                    break;
                case weaponshinesprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    shinetexture.bind();
                    if (blend) {
                        blend = 0;
                        glAlphaFunc(GL_GREATER, 0.001);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    }
                    break;
                case flamesprite:
                case weaponflamesprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    flametexture.bind();
                    if (blend || lasttype == bloodflamesprite) {
                        blend = 0;
                        glAlphaFunc(GL_GREATER, 0.3);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    }
                    break;
                case bloodflamesprite:
                    EFFECT_DIAGNOSTIC(++effectDiagnostics.binds);
                    bloodflametexture.bind();
                    if (blend) {
                        blend = 0;
                        glAlphaFunc(GL_GREATER, 0.3);
                        EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
                        glBlendFunc(GL_ONE, GL_ZERO);
                    }
                    break;
            }
        }
        if (sprites[i]->type == snowsprite) {
            distancemult = (144 - (distsq(&tempviewer, &sprites[i]->position) - (144 * fadestart)) * (1 / (1 - fadestart))) / 144;
        } else {
            distancemult = (viewdistsquared - (distsq(&viewer, &sprites[i]->position) - (viewdistsquared * fadestart)) * (1 / (1 - fadestart))) / viewdistsquared;
        }
        if (sprites[i]->type == flamesprite) {
            if (distancemult >= 1) {
                setSpriteColor(sprites[i]->color[0], sprites[i]->color[1], sprites[i]->color[2], sprites[i]->opacity);
            } else {
                setSpriteColor(sprites[i]->color[0], sprites[i]->color[1], sprites[i]->color[2], sprites[i]->opacity * distancemult);
            }
        } else {
            if (distancemult >= 1) {
                setSpriteColor(sprites[i]->color[0] * lightcolor[0], sprites[i]->color[1] * lightcolor[1], sprites[i]->color[2] * lightcolor[2], sprites[i]->opacity);
            } else {
                setSpriteColor(sprites[i]->color[0] * lightcolor[0], sprites[i]->color[1] * lightcolor[1], sprites[i]->color[2] * lightcolor[2], sprites[i]->opacity * distancemult);
            }
        }
        lasttype = sprites[i]->type;
        lastspecial = sprites[i]->special;
#ifndef __EMSCRIPTEN__
        {
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glTranslatef(sprites[i]->position.x, sprites[i]->position.y, sprites[i]->position.z);
            if ((sprites[i]->type == flamesprite || sprites[i]->type == weaponflamesprite || sprites[i]->type == weaponshinesprite)) {
                difference = viewer - sprites[i]->position;
                Normalise(&difference);
                glTranslatef(difference.x * sprites[i]->size / 4, difference.y * sprites[i]->size / 4, difference.z * sprites[i]->size / 4);
            }
            if (sprites[i]->type == snowsprite) {
                glRotatef(sprites[i]->rotation * .2, 0, .3, 1);
                glTranslatef(1, 0, 0);
            }
            glGetFloatv(GL_MODELVIEW_MATRIX, M);
            point.x = M[12];
            point.y = M[13];
            point.z = M[14];
            glLoadIdentity();
            glTranslatef(point.x, point.y, point.z);

            glRotatef(sprites[i]->rotation, 0, 0, 1);

            if ((sprites[i]->type == flamesprite || sprites[i]->type == weaponflamesprite || sprites[i]->type == weaponshinesprite || sprites[i]->type == bloodflamesprite)) {
                if (sprites[i]->alivetime < .14) {
                    glScalef(sprites[i]->alivetime / .14, sprites[i]->alivetime / .14, sprites[i]->alivetime / .14);
                }
            }
        }
#endif
        if (sprites[i]->type == smoketype || sprites[i]->type == snowsprite || sprites[i]->type == weaponshinesprite || sprites[i]->type == breathsprite) {
            if (sprites[i]->alivetime < .3) {
                if (distancemult >= 1) {
                    setSpriteColor(sprites[i]->color[0] * lightcolor[0], sprites[i]->color[1] * lightcolor[1], sprites[i]->color[2] * lightcolor[2], sprites[i]->opacity * sprites[i]->alivetime / .3);
                }
                if (distancemult < 1) {
                    setSpriteColor(sprites[i]->color[0] * lightcolor[0], sprites[i]->color[1] * lightcolor[1], sprites[i]->color[2] * lightcolor[2], sprites[i]->opacity * distancemult * sprites[i]->alivetime / .3);
                }
            }
        }
        if (sprites[i]->type == splintersprite && sprites[i]->special > 0 && sprites[i]->special != 3) {
            // Leaf particles (special == 1) must modulate with white so the
            // texture shows instead of being tinted black by the sprite color.
            const bool isLeaf = (sprites[i]->special == 1);
            const float cr = isLeaf ? 1.f : sprites[i]->color[0];
            const float cg = isLeaf ? 1.f : sprites[i]->color[1];
            const float cb = isLeaf ? 1.f : sprites[i]->color[2];
            if (sprites[i]->alivetime < .2) {
                if (distancemult >= 1) {
                    setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], sprites[i]->alivetime / .2);
                } else {
                    setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], distancemult * sprites[i]->alivetime / .2);
                }
            } else {
                if (distancemult >= 1) {
                    setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], 1);
                } else {
                    setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], distancemult);
                }
            }
        }
        if (sprites[i]->type == splintersprite && (sprites[i]->special == 0 || sprites[i]->special == 3)) {
            // Prevent entirely black player/character sprites from disappearing
            float cr = (sprites[i]->color[0] < 0.01f && sprites[i]->color[1] < 0.01f && sprites[i]->color[2] < 0.01f) ? 1.f : sprites[i]->color[0];
            float cg = (sprites[i]->color[0] < 0.01f && sprites[i]->color[1] < 0.01f && sprites[i]->color[2] < 0.01f) ? 1.f : sprites[i]->color[1];
            float cb = (sprites[i]->color[0] < 0.01f && sprites[i]->color[1] < 0.01f && sprites[i]->color[2] < 0.01f) ? 1.f : sprites[i]->color[2];
            if (distancemult >= 1) {
                setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], 1);
            } else {
                setSpriteColor(cr * lightcolor[0], cg * lightcolor[1], cb * lightcolor[2], distancemult);
            }
        }

#ifdef __EMSCRIPTEN__
        {
            const int indices[6] = {0, 1, 2, 3, 2, 1};
            const float uv[4][2] = {{1, 1}, {0, 1}, {1, 0}, {0, 0}};
            for (int corner : indices) {
                std::memcpy(arrayPositions + arrayVertexCount * 3, billboards[i].vertex[corner], 3 * sizeof(GLfloat));
                std::memcpy(arrayUVs + arrayVertexCount * 2, uv[corner], 2 * sizeof(GLfloat));
                std::memcpy(arrayColors + arrayVertexCount * 4, arrayColor, 4 * sizeof(GLfloat));
                ++arrayVertexCount;
            }
            // Bound each run without dropping, sorting, or delaying any triangles.
            if (++batchSprites == 1024) {
                flushArray();
                batchSprites = 0;
            }
        }
#else
        {
            glBegin(GL_TRIANGLES);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(.5 * sprites[i]->size, .5 * sprites[i]->size, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-.5 * sprites[i]->size, .5 * sprites[i]->size, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(.5 * sprites[i]->size, -.5 * sprites[i]->size, 0.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-.5 * sprites[i]->size, -.5 * sprites[i]->size, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(.5 * sprites[i]->size, -.5 * sprites[i]->size, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-.5 * sprites[i]->size, .5 * sprites[i]->size, 0.0f);
            glEnd();
            glPopMatrix();
            EFFECT_DIAGNOSTIC(++effectDiagnostics.batches);
        }
#endif
    }
#ifdef __EMSCRIPTEN__
    flushArray();
    if (!sprites.empty()) {
        glPopClientAttrib();
        glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
        // DrawArrays need not retain current color/UV; match the immediate path's
        // last particle color and final texture coordinate for subsequent draws.
        glColor4fv(arrayColor);
        glTexCoord2f(0, 1);
        glPopMatrix();
    }
#else
#undef setSpriteColor
#endif
#if defined(__EMSCRIPTEN__) && defined(LUGARU_WEB_EFFECT_DIAGNOSTICS) && LUGARU_WEB_EFFECT_DIAGNOSTICS
    const double updateStarted = emscripten_get_now();
    effectDiagnostics.drawMs += updateStarted - drawStarted;
#endif
    tempmult = frameDeltaTime;
    for (int i = sprites.size() - 1; i >= 0; i--) {
        EFFECT_DIAGNOSTIC(++effectDiagnostics.updated);
        float spriteDt = tempmult;
        if (sprites[i]->type != snowsprite) {
            sprites[i]->position += sprites[i]->velocity * spriteDt;
            sprites[i]->velocity += windvector * spriteDt;
        }
        if (sprites[i]->type == flamesprite || sprites[i]->type == smoketype) {
            sprites[i]->position += windvector * spriteDt / 2;
        }
        if ((sprites[i]->type == flamesprite || sprites[i]->type == weaponflamesprite || sprites[i]->type == weaponshinesprite || sprites[i]->type == bloodflamesprite)) {
            spriteDt *= sprites[i]->speed * .7;
        }
        sprites[i]->alivetime += spriteDt;

        if (sprites[i]->type == cloudsprite || sprites[i]->type == cloudimpactsprite) {
            sprites[i]->opacity -= spriteDt / 2;
            sprites[i]->size += spriteDt / 2;
            sprites[i]->velocity.y += gravity * spriteDt * .25;
        }
        if (sprites[i]->type == breathsprite) {
            sprites[i]->opacity -= spriteDt / 2;
            sprites[i]->size += spriteDt / 2;
            if (findLength(&sprites[i]->velocity) <= spriteDt) {
                sprites[i]->velocity = 0;
            } else {
                XYZ slowdown;
                slowdown = sprites[i]->velocity * -1;
                Normalise(&slowdown);
                slowdown *= spriteDt;
                sprites[i]->velocity += slowdown;
            }
        }
        if (sprites[i]->type == snowsprite) {
            sprites[i]->size -= spriteDt / 120;
            sprites[i]->rotation += spriteDt * 360;
            sprites[i]->position.y -= spriteDt;
            sprites[i]->position += windvector * spriteDt;
            if (sprites[i]->position.y < tempviewer.y - 6) {
                sprites[i]->position.y += 12;
            }
            if (sprites[i]->position.y > tempviewer.y + 6) {
                sprites[i]->position.y -= 12;
            }
            if (sprites[i]->position.z < tempviewer.z - 6) {
                sprites[i]->position.z += 12;
            }
            if (sprites[i]->position.z > tempviewer.z + 6) {
                sprites[i]->position.z -= 12;
            }
            if (sprites[i]->position.x < tempviewer.x - 6) {
                sprites[i]->position.x += 12;
            }
            if (sprites[i]->position.x > tempviewer.x + 6) {
                sprites[i]->position.x -= 12;
            }
        }
        if (sprites[i]->type == bloodsprite) {
            bool spritehit = 0;
            sprites[i]->rotation += spriteDt * 100;
            sprites[i]->velocity.y += gravity * spriteDt;
            if (check) {
                XYZ where, startpoint, endpoint, movepoint, footpoint;
                float rotationpoint;
                int whichtri;

                for (unsigned j = 0; j < Person::players.size(); j++) {
                    if (!spritehit && Person::players[j]->dead && sprites[i]->alivetime > .1) {
                        where = sprites[i]->oldposition;
                        where -= Person::players[j]->coords;
                        if (!Person::players[j]->skeleton.free) {
                            where = DoRotation(where, 0, -Person::players[j]->yaw, 0);
                        }
                        startpoint = where;
                        where = sprites[i]->position;
                        where -= Person::players[j]->coords;
                        if (!Person::players[j]->skeleton.free) {
                            where = DoRotation(where, 0, -Person::players[j]->yaw, 0);
                        }
                        endpoint = where;

                        movepoint = 0;
                        rotationpoint = 0;
                        whichtri = Person::players[j]->skeleton.drawmodel.LineCheck(&startpoint, &endpoint, &footpoint, &movepoint, &rotationpoint);
                        if (whichtri != -1) {
                            spritehit = 1;
                            Person::players[j]->DoBloodBigWhere(0, 160, sprites[i]->oldposition);
                        }
                    }
                }

                whichpatchx = sprites[i]->position.x / (terrain.size / subdivision * terrain.scale);
                whichpatchz = sprites[i]->position.z / (terrain.size / subdivision * terrain.scale);
                if (whichpatchx > 0 && whichpatchz > 0 && whichpatchx < subdivision && whichpatchz < subdivision) {
                    if (!spritehit) {
                        for (unsigned int j = 0; j < terrain.patchobjects[whichpatchx][whichpatchz].size(); j++) {
                            k = terrain.patchobjects[whichpatchx][whichpatchz][j];
                            start = sprites[i]->oldposition;
                            end = sprites[i]->position;
                            if (!spritehit) {
                                if (Object::objects[k]->model.LineCheck(&start, &end, &colpoint, &Object::objects[k]->position, &Object::objects[k]->yaw) != -1) {
                                    if (detail == 2 || (detail == 1 && abs(Random() % 4) == 0) || (detail == 0 && abs(Random() % 8) == 0)) {
                                        Object::objects[k]->model.MakeDecal(blooddecalfast, DoRotation(colpoint - Object::objects[k]->position, 0, -Object::objects[k]->yaw, 0), sprites[i]->size * 1.6, .5, Random() % 360);
                                    }
                                    spritehit = 1;
                                }
                            }
                        }
                    }
                }
                if (!spritehit) {
                    if (sprites[i]->position.y < terrain.getHeight(sprites[i]->position.x, sprites[i]->position.z)) {
                        terrain.MakeDecal(blooddecalfast, sprites[i]->position, sprites[i]->size * 1.6, .6, Random() % 360);
                        spritehit = 1;
                    }
                }
            }

            if (spritehit) {
                DeleteSprite(i);
                continue;
            }
        }

        if (sprites[i]->type == splintersprite) {
            sprites[i]->rotation += sprites[i]->rotatespeed * spriteDt;
            sprites[i]->opacity -= spriteDt / 2;
            if (sprites[i]->special == 0 || sprites[i]->special == 2 || sprites[i]->special == 3) {
                sprites[i]->velocity.y += gravity * spriteDt;
            }
            if (sprites[i]->special == 1) {
                sprites[i]->velocity.y += gravity * spriteDt * .5;
            }
        }
        if (sprites[i]->type == flamesprite || sprites[i]->type == weaponflamesprite || sprites[i]->type == weaponshinesprite || sprites[i]->type == bloodflamesprite) {
            sprites[i]->rotation += spriteDt * sprites[i]->rotatespeed;
            sprites[i]->opacity -= spriteDt * 5 / 4;
            if (sprites[i]->type != weaponshinesprite && sprites[i]->type != bloodflamesprite) {
                if (sprites[i]->opacity < .5 && sprites[i]->opacity + spriteDt * 5 / 4 >= .5 && (abs(Random() % 4) == 0 || (sprites[i]->initialsize > 2 && Random() % 2 == 0))) {
                    MakeSprite(smoketype, sprites[i]->position, sprites[i]->velocity, .9, .9, .6, sprites[i]->size * 1.2, .4);
                }
            }
            if (sprites[i]->alivetime > .14 && (sprites[i]->type == flamesprite)) {
                sprites[i]->velocity = 0;
                sprites[i]->velocity.y = 1.5;
            }
        }
        if (sprites[i]->type == smoketype) {
            sprites[i]->opacity -= spriteDt / 3 / sprites[i]->initialsize;
            sprites[i]->color[0] -= spriteDt;
            sprites[i]->color[1] -= spriteDt;
            sprites[i]->color[2] -= spriteDt;
            if (sprites[i]->color[0] < .6) {
                sprites[i]->color[0] = .6;
            }
            if (sprites[i]->color[1] < .6) {
                sprites[i]->color[1] = .6;
            }
            if (sprites[i]->color[2] < .6) {
                sprites[i]->color[2] = .6;
            }
            sprites[i]->size += spriteDt;
            sprites[i]->velocity = 0;
            sprites[i]->velocity.y = 1.5;
            sprites[i]->rotation += spriteDt * sprites[i]->rotatespeed / 5;
        }
        if (sprites[i]->opacity <= 0 || sprites[i]->size <= 0) {
            DeleteSprite(i);
        }
    }

    if (check) {
        for (int i = sprites.size() - 1; i >= 0; i--) {
            sprites[i]->oldposition = sprites[i]->position;
        }
    }
    glAlphaFunc(GL_GREATER, 0.0001);
    EFFECT_DIAGNOSTIC(++effectDiagnostics.blendCalls);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    EFFECT_DIAGNOSTIC(effectDiagnostics.updateMs += emscripten_get_now() - updateStarted);
    EFFECT_DIAGNOSTIC(++effectDiagnostics.calls);
    EFFECT_DIAGNOSTIC(effectDiagnostics.report(emscripten_get_now()));
}

void Sprite::DeleteSprite(int i)
{
    sprites.erase(sprites.begin() + i);
}

void Sprite::MakeSprite(int atype, XYZ where, XYZ avelocity, float red, float green, float blue, float asize, float aopacity)
{
    if (sprites.size() < max_sprites - 1) {
        sprites.push_back(std::unique_ptr<Sprite>(new Sprite()));
        if ((atype != bloodsprite && atype != bloodflamesprite) || bloodtoggle) {
            EFFECT_DIAGNOSTIC(++effectDiagnostics.spawned[atype]);
            sprites.back()->special = 0;
            sprites.back()->type = atype;
            sprites.back()->position = where;
            sprites.back()->oldposition = where;
            sprites.back()->velocity = avelocity;
            sprites.back()->alivetime = 0;
            sprites.back()->opacity = aopacity;
            sprites.back()->size = asize;
            sprites.back()->initialsize = asize;
            sprites.back()->color[0] = red;
            sprites.back()->color[1] = green;
            sprites.back()->color[2] = blue;
            sprites.back()->rotatespeed = abs(Random() % 720) - 360;
            sprites.back()->speed = float(abs(Random() % 100)) / 200 + 1.5;
        }
    }
}

Sprite::Sprite()
{
    oldposition = 0;
    position = 0;
    velocity = 0;
    size = 0;
    initialsize = 0;
    type = 0;
    special = 0;
    memset(color, 0, sizeof(color));
    opacity = 0;
    rotation = 0;
    alivetime = 0;
    speed = 0;
    rotatespeed = 0;
}
