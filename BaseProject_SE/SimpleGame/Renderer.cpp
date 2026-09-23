#include "stdafx.h"
#include "Renderer.h"
#include "Profiler.h"
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace
{
    const char* GeometryVS = R"GLSL(#version 330 core
layout(location = 0) in vec4 positions01;
layout(location = 1) in vec4 positions23;
layout(location = 2) in vec4 uvs01;
layout(location = 3) in vec4 uvs23;
layout(location = 4) in vec4 color;
layout(location = 5) in float material;
uniform vec2 canvas;
out vec4 tint;
out vec2 uv;
flat out int kind;

void main()
{
    // One shared quad topology, with all primitive data advancing per instance.
    const int corners[6] = int[6](0, 1, 2, 0, 2, 3);
    int corner = corners[gl_VertexID];
    vec2 positions[4] = vec2[4](positions01.xy, positions01.zw, positions23.xy, positions23.zw);
    vec2 texcoords[4] = vec2[4](uvs01.xy, uvs01.zw, uvs23.xy, uvs23.zw);
    vec2 position = positions[corner];
    gl_Position = vec4(position.x / canvas.x * 2. - 1., 1. - position.y / canvas.y * 2., 0, 1);
    tint = color;
    uv = texcoords[corner];
    kind = int(material + .5);
}
)GLSL";

    const char* GeometryFS = R"GLSL(#version 330 core
in vec4 tint;
in vec2 uv;
flat in int kind;
uniform float clock;
uniform sampler2D spriteAtlas;
uniform sampler2D fontAtlas;
uniform sampler2D modelAtlas;
out vec4 pixel;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3. - 2. * f);

    return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x), mix(hash(i + vec2(0, 1)), hash(i + 1.), f.x), f.y);
}

void main()
{
    vec4 c = tint;

    if (kind == 1)
    {
        float n = noise(uv * 2.) * .45 + noise(uv * 13.) * .25 + noise(uv * 55.) * .30;
        c.rgb *= .73 + n * .55;
        float blade = step(.91, hash(floor(uv * 31.)));
        c.rgb += vec3(.055, .072, .015) * blade;
    }
    else if (kind == 2)
    {
        float n = noise(uv * 6.) * .65 + noise(uv * 49.) * .35;
        c.rgb *= .75 + n * .48;
        float pebble = (1. - smoothstep(.0, .10, length(fract(uv * 9.) - .5)));
        c.rgb += pebble * .08;
        float rut = pow(abs(sin(uv.x * 4. + noise(uv * .4) * 2.)), 24.);
        c.rgb *= 1. - rut * .09;
    }
    else if (kind == 3)
    {
        vec2 p = uv * 2.8;
        p.x += mod(floor(p.y), 2.) * .5;
        vec2 f = fract(p);
        float seam = 1. - smoothstep(.025, .095, min(min(f.x, 1. - f.x), min(f.y, 1. - f.y)));
        float variation = hash(floor(p));
        c.rgb *= .74 + variation * .40;
        c.rgb = mix(c.rgb, vec3(.16, .19, .13), seam * .72);
        c.rgb += noise(uv * 68.) * .035;
    }
    else if (kind == 4)
    {
        float wave = sin(uv.x * 3. + uv.y * 1.6 + clock * 1.1) * sin(uv.y * 4. - clock * .7);
        float detail = sin(uv.x * 14. + clock * 1.7 + sin(uv.y * 8.)) * sin(uv.y * 11. - clock);
        c.rgb += vec3(.018, .043, .047) * wave;
        float glint = pow(max(0., wave * .66 + detail * .34), 12.);
        c.rgb += vec3(.8, .95, .82) * glint * .7;
        c.rgb += vec3(.02, .045, .027) * pow(noise(uv * 7. + clock * .16), 4.);
    }
    else if (kind == 5)
    {
        float d = dot(uv, uv);
        c.a *= exp(-d * 3.8) * (1. - smoothstep(.7, 1., d));
    }
    else if (kind == 6)
    {
        float grain = sin(uv.x * 29. + noise(uv * 3.) * 5.);
        c.rgb *= .87 + .10 * grain + noise(uv * 54.) * .10;
    }
    else if (kind == 7)
    {
        float straw = hash(floor(uv * vec2(80, 7)));
        c.rgb *= .77 + straw * .40;
    }
    else if (kind == 8)
    {
        vec4 s = texture(spriteAtlas, uv);
        c *= s;
    }
    else if (kind == 9)
    {
        c.a *= texture(fontAtlas, uv).r;
    }

    if (kind == 10)
    {
        // The quad is static; flame shape, rising smoke and embers live in the shader.
        float height = 1. - uv.y;
        float wobble = sin(height * 10. - clock * 5.) * .055;
        float turbulence = noise(vec2(uv.x * 6., height * 8. - clock * 2.8));
        float width = max(.025, (.84 - height) * .49);
        float flame = 1. - smoothstep(width * .35, width, abs(uv.x + wobble) + turbulence * .095);
        flame *= smoothstep(.035, .16, height) * (1. - smoothstep(.65, .90, height));
        float core = flame * (1. - smoothstep(.14, .52, height));
        vec3 fire = mix(vec3(2.5, .48, .035), vec3(3., 1.9, .32), core);
        float smoke = noise(vec2(uv.x * 5. - height, height * 8. - clock * .6));
        smoke *= (1. - smoothstep(.1, .65, abs(uv.x - height * .16))) * smoothstep(.50, .75, height) *
                 (1. - smoothstep(.82, 1., height)) * .16;
        float ember = 0.;

        for (int i = 0; i < 8; ++i)
        {
            float id = float(i);
            float rise = fract(clock * (.22 + id * .011) + id * .137);
            vec2 position = vec2(sin(rise * 8. + id) * .20 + rise * .15, .18 + rise * .80);
            vec2 delta = (vec2(uv.x, height) - position) * vec2(1., .6);
            ember += (1. - smoothstep(.004, .025, length(delta))) * (1. - rise);
        }

        float alpha = clamp(flame + smoke + ember, 0., 1.);
        vec3 light = fire * flame + vec3(.45, .47, .40) * smoke + vec3(2.8, 1.2, .12) * ember;
        c = vec4(light / max(alpha, .001), alpha) * tint;
    }

    if (kind == 11)
    {
        vec4 model = texture(modelAtlas, uv);
        vec3 rgb = model.a > .001 ? model.rgb / model.a : vec3(0);
        c = vec4(mix(rgb, vec3(1), tint.r), model.a * tint.a);
    }

    pixel = c;
}
)GLSL";

    const char* ScreenVS = R"GLSL(#version 330 core
out vec2 uv;

void main()
{
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    uv = p;
    gl_Position = vec4(p * 2. - 1., 0, 1);
}
)GLSL";

    const char* ScreenFS = R"GLSL(#version 330 core
in vec2 uv;
out vec4 pixel;
uniform sampler2D source;
uniform sampler2D extra;
uniform int pass;
uniform bool effects;

vec3 blur(vec2 axis)
{
    vec2 d = axis / vec2(textureSize(source, 0));
    vec3 c = texture(source, uv).rgb * .227027;
    c += (texture(source, uv + d * 1.384615).rgb + texture(source, uv - d * 1.384615).rgb) * .316216;
    c += (texture(source, uv + d * 3.230769).rgb + texture(source, uv - d * 3.230769).rgb) * .070270;

    return c;
}

void main()
{
    if (pass == 0)
    {
        vec2 d = 1. / vec2(textureSize(source, 0));
        vec3 c = vec3(0);

        for (int x = 0; x < 2; ++x)
            for (int y = 0; y < 2; ++y)
                c += texture(source, uv + vec2(x, y) * d).rgb * .25;
        float light = max(max(c.r, c.g), c.b);
        pixel = vec4(c * smoothstep(.72, 1.4, light), 1);
    }
    else if (pass == 1 || pass == 2)
        pixel = vec4(blur(pass == 1 ? vec2(1, 0) : vec2(0, 1)), 1);
    else if (pass == 4)
    {
        vec2 d = 1. / vec2(textureSize(source, 0));
        float mask = 0.;

        // Bilinear paired taps approximate the same five-texel box footprint with nine fetches.
        const float offsets[3] = float[3](-1.5, .5, 2.);
        const float weights[3] = float[3](.4, .4, .2);
        for (int x = 0; x < 3; ++x)
            for (int y = 0; y < 3; ++y)
                mask += texture(source, uv + vec2(offsets[x], offsets[y]) * d).a * weights[x] * weights[y];
        pixel = vec4(.055, .085, .085, mask * .78);
    }
    else
    {
        vec3 c = texture(source, uv).rgb;

        if (effects)
        {
            vec2 d = 1. / vec2(textureSize(source, 0));
            vec3 n = texture(source, uv + vec2(0, d.y)).rgb;
            vec3 s = texture(source, uv - vec2(0, d.y)).rgb, e = texture(source, uv + vec2(d.x, 0)).rgb,
                 w = texture(source, uv - vec2(d.x, 0)).rgb;
            vec3 lum = vec3(.299, .587, .114);
            float edge = abs(dot(n - s, lum)) + abs(dot(e - w, lum));
            c = mix(c, (n + s + e + w) * .25, clamp(edge * .28, 0., .25));
            c += texture(extra, uv).rgb * .23;
            c *= vec3(1.035, 1.005, .965);
            c = (c * (2.51 * c + .03)) / (c * (2.43 * c + .59) + .14);
            vec2 v = (uv - .5) * vec2(1., .85);
            c *= 1. - dot(v, v) * .30;
        }

        pixel = vec4(clamp(c, 0., 1.), 1);
    }
}
)GLSL";

}

GLuint Renderer::Program(const char* vs, const char* fs)
{
    Profile::Scope scope("shader_compile_link");
    GLuint shaders[2] = {};
    const char* src[2] = {vs, fs};
    GLenum types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};

    for (int i = 0; i < 2; ++i)
    {
        shaders[i] = glCreateShader(types[i]);

        if (!shaders[i])
        {
            for (GLuint s : shaders)
                if (s)
                    glDeleteShader(s);

            return 0;
        }

        glShaderSource(shaders[i], 1, &src[i], nullptr);
        glCompileShader(shaders[i]);
        GLint ok = 0;
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);

        if (!ok)
        {
            char log[4096] = {};
            glGetShaderInfoLog(shaders[i], sizeof(log), nullptr, log);
            std::cerr << "셰이더 컴파일 실패: " << log << '\n';

            for (GLuint s : shaders)
                if (s)
                    glDeleteShader(s);

            return 0;
        }
    }

    GLuint p = glCreateProgram();

    if (p)
    {
        for (GLuint s : shaders)
            glAttachShader(p, s);
        glLinkProgram(p);
    }

    for (GLuint s : shaders)
        glDeleteShader(s);

    if (!p)
        return 0;

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);

    if (!ok)
    {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::cerr << "셰이더 연결 실패: " << log << '\n';
        glDeleteProgram(p);

        return 0;
    }

    return p;
}

bool Renderer::Target(GLuint& fbo, GLuint& texture, int w, int h, GLint format)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

    if (!ok)
        std::cerr << "렌더링 버퍼 생성 실패\n";

    return ok;
}

Renderer::Renderer(int w, int h)
{
    Profile::Scope startup("renderer_initialize");
    program = Program(GeometryVS, GeometryFS);
    post = Program(ScreenVS, ScreenFS);

    if (!program || !post)
        return;

    canvasLocation = glGetUniformLocation(program, "canvas");
    timeLocation = glGetUniformLocation(program, "clock");
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "spriteAtlas"), 1);
    glUniform1i(glGetUniformLocation(program, "fontAtlas"), 2);
    glUniform1i(glGetUniformLocation(program, "modelAtlas"), 3);
    glUseProgram(post);
    glUniform1i(glGetUniformLocation(post, "source"), 0);
    glUniform1i(glGetUniformLocation(post, "extra"), 1);
    passLocation = glGetUniformLocation(post, "pass");
    effectsLocation = glGetUniformLocation(post, "effects");
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    static_assert(sizeof(Instance) == 21 * sizeof(float), "Instance layout changed.");
    const int widths[6] = {4, 4, 4, 4, 4, 1};
    const size_t offsets[6] = {0, 4, 8, 12, 16, 20};
    for (GLuint i = 0; i < 6; ++i)
    {
        glVertexAttribPointer(i,
            widths[i],
            GL_FLOAT,
            GL_FALSE,
            sizeof(Instance),
            reinterpret_cast<void*>(offsets[i] * sizeof(float)));
        glEnableVertexAttribArray(i);
        glVertexAttribDivisor(i, 1);
    }

    if (!Target(sceneFbo, sceneTex, 1280, 800, GL_RGBA16F) ||
        !Target(shadowFbo, shadowTex, 640, 400, GL_RGBA8))
        return;

    for (int i = 0; i < 2; ++i)
        if (!Target(bloomFbo[i], bloomTex[i], 640, 400, GL_RGBA16F))
            return;

    vertices.reserve(10000);
    instances.reserve(16000);

    if (!CreateAssets())
        return;

    instanceBufferBytes = 0;
    instances.clear();
    instances.shrink_to_fit();
    instances.reserve(16000);
    initialized = vao && vbo;
    gpu.Initialize();
    Profile::Gauge("estimated_texture_storage_bytes", 39002112);
    Profile::Gauge("shadow_filter_texture_fetches_per_pixel", 9);
    Profile::Gauge("render_width_px", 1280);
    Profile::Gauge("render_height_px", 800);
    Profile::Gauge("shadow_width_px", 640);
    Profile::Gauge("shadow_height_px", 400);
    Resize(w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Renderer::~Renderer()
{
    gpu.Release();
    if (modelAtlas)
    {
        glDeleteTextures(1, &modelAtlas);
    }
    ReleaseAssets();
    GLuint textures[] = {sceneTex, shadowTex, bloomTex[0], bloomTex[1]};
    glDeleteTextures(4, textures);
    GLuint fbos[] = {sceneFbo, shadowFbo, bloomFbo[0], bloomFbo[1]};
    glDeleteFramebuffers(4, fbos);

    if (vbo)
        glDeleteBuffers(1, &vbo);

    if (vao)
        glDeleteVertexArrays(1, &vao);

    if (program)
        glDeleteProgram(program);

    if (post)
        glDeleteProgram(post);
}

void Renderer::Resize(int w, int h)
{
    if (w < 1)
        w = 1;

    if (h < 1)
        h = 1;
    windowHeight = h;
    float sx = w / Width, sy = h / Height, s = sx < sy ? sx : sy;
    viewportW = static_cast<int>(Width * s);
    viewportH = static_cast<int>(Height * s);
    viewportX = (w - viewportW) / 2;
    viewportY = (h - viewportH) / 2;
}

void Renderer::Begin(float time, bool effects)
{
    instances.clear();
    modelInstances = 0;
    currentPass = "draw_ground";
    gpu.BeginFrame();
    clock = time;
    postEnabled = effects;
    worldFinished = false;
    canvasW = Width;
    canvasH = Height;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glViewport(0, 0, 1280, 800);
    glClearColor(.08f, .12f, .12f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::Flush()
{
    if (instances.empty())
    {
        return;
    }
    Profile::Scope scope("render_flush_cpu");
    glUseProgram(program);
    glUniform2f(canvasLocation, canvasW, canvasH);
    glUniform1f(timeLocation, clock);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sprites);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, modelAtlas);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    size_t bytes = instances.size() * sizeof(Instance);
    {
        Profile::Scope upload("render_buffer_upload");
        if (bytes > instanceBufferBytes)
        {
            instanceBufferBytes = std::max(bytes, instanceBufferBytes * 2);
            Profile::Count("instance_buffer_capacity_growths");
        }
        // Orphan old storage instead of waiting for the previous GPU reader.
        glBufferData(GL_ARRAY_BUFFER, instanceBufferBytes, nullptr, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, instances.data());
    }
    {
        Profile::Scope submit("render_draw_submit");
        GpuPass pass = std::string(currentPass) == "draw_ground"        ? GpuPass::Ground
                       : std::string(currentPass) == "draw_shadow_mask" ? GpuPass::Shadows
                       : std::string(currentPass) == "draw_ui"          ? GpuPass::UI
                                                                        : GpuPass::World;
        gpu.Begin(pass);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
        gpu.End(pass);
    }
    Profile::Count("draw_calls");
    Profile::Count(currentPass);
    Profile::Count("instanced_draw_calls");
    Profile::Count("primitive_instances", static_cast<double>(instances.size()));
    Profile::Count("submitted_vertices", static_cast<double>(instances.size() * 6));
    Profile::Count("vertex_upload_bytes", static_cast<double>(bytes));
    Profile::Gauge("instance_buffer_capacity_bytes", static_cast<double>(instanceBufferBytes));
    instances.clear();
}

void Renderer::ScreenPass(GLuint target, int w, int h, int pass, GLuint texture, GLuint extra)
{
    Profile::Scope scope("render_screen_pass_cpu");
    glBindFramebuffer(GL_FRAMEBUFFER, target);

    if (target)
        glViewport(0, 0, w, h);
    else
        glViewport(viewportX, viewportY, viewportW, viewportH);
    glUseProgram(post);
    glUniform1i(passLocation, pass);
    glUniform1i(effectsLocation, postEnabled);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, extra);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao);
    GpuPass gpuPass = pass == 0   ? GpuPass::BloomExtract
                      : pass == 1 ? GpuPass::BloomHorizontal
                      : pass == 2 ? GpuPass::BloomVertical
                      : pass == 4 ? GpuPass::ShadowComposite
                                  : GpuPass::Composite;
    const char* name = pass == 0   ? "draw_bloom_extract"
                       : pass == 1 ? "draw_bloom_horizontal"
                       : pass == 2 ? "draw_bloom_vertical"
                       : pass == 4 ? "draw_shadow_composite"
                                   : "draw_composite";
    gpu.Begin(gpuPass);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    gpu.End(gpuPass);
    Profile::Count("draw_calls");
    Profile::Count(name);
    Profile::Count("fullscreen_passes");
    Profile::Count("submitted_vertices", 3);
}

void Renderer::BeginShadows()
{
    Flush();
    currentPass = "draw_shadow_mask";
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
    glViewport(0, 0, 640, 400);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::EndShadows()
{
    Flush();
    ScreenPass(sceneFbo, 1280, 800, 4, shadowTex);
    currentPass = "draw_world";
}

void Renderer::FinishWorld()
{
    if (worldFinished)
        return;

    Flush();
    glDisable(GL_BLEND);

    if (postEnabled)
    {
        ScreenPass(bloomFbo[0], 640, 400, 0, sceneTex);
        ScreenPass(bloomFbo[1], 640, 400, 1, bloomTex[0]);
        ScreenPass(bloomFbo[0], 640, 400, 2, bloomTex[1]);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(.025f, .035f, .035f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ScreenPass(0, 1280, 800, 3, sceneTex, postEnabled ? bloomTex[0] : 0);
    glEnable(GL_BLEND);
    worldFinished = true;
    currentPass = "draw_ui";
}

void Renderer::End()
{
    if (!worldFinished)
        FinishWorld();
    Flush();
    glBindVertexArray(0);
    glUseProgram(0);
    gpu.EndFrame();
    Profile::Count("model_sprite_instances", modelInstances);
}

void Renderer::LogFrame()
{
    Profile::EndFrame();
}

void Renderer::Push(Point p, Color c, Point uv, Material m)
{
    vertices.push_back({p.x, p.y, c.r, c.g, c.b, c.a, uv.x, uv.y, static_cast<float>(m)});
}

void Renderer::Triangle(Point a, Point b, Point c, Color col)
{
    SurfaceTriangle(a, b, c, {0, 0}, {0, 0}, {0, 0}, Material::Solid, col);
}

void Renderer::Quad(Point a, Point b, Point c, Point d, Color col)
{
    SurfaceQuad(a, b, c, d, {0, 0}, {0, 0}, {0, 0}, {0, 0}, Material::Solid, col);
}

void Renderer::SurfaceTriangle(Point a, Point b, Point c, Point ua, Point ub, Point uc, Material m, Color col)
{
    if (buildingModels)
    {
        Push(a, col, ua, m);
        Push(b, col, ub, m);
        Push(c, col, uc, m);
        return;
    }
    instances.push_back({{a, b, c, c}, {ua, ub, uc, uc}, col, static_cast<float>(m)});
}

void Renderer::SurfaceQuad(
    Point a, Point b, Point c, Point d, Point ua, Point ub, Point uc, Point ud, Material m, Color col)
{
    if (buildingModels)
    {
        SurfaceTriangle(a, b, c, ua, ub, uc, m, col);
        SurfaceTriangle(a, c, d, ua, uc, ud, m, col);
        return;
    }
    instances.push_back({{a, b, c, d}, {ua, ub, uc, ud}, col, static_cast<float>(m)});
}

void Renderer::Rect(float x, float y, float w, float h, Color c)
{
    Quad({x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}, c);
}

void Renderer::Ellipse(float x, float y, float rx, float ry, Color c, int segments)
{
    for (int i = 0; i < segments; ++i)
    {
        float a = i * 6.2831853f / segments, b = (i + 1) * 6.2831853f / segments;
        Triangle({x, y},
            {x + std::cos(a) * rx, y + std::sin(a) * ry},
            {x + std::cos(b) * rx, y + std::sin(b) * ry},
            c);
    }
}

void Renderer::Line(Point a, Point b, float thickness, Color c)
{
    float dx = b.x - a.x, dy = b.y - a.y, len = std::sqrt(dx * dx + dy * dy);

    if (len < .001f)
        return;

    float nx = -dy / len * thickness * .5f, ny = dx / len * thickness * .5f;
    Quad({a.x + nx, a.y + ny}, {b.x + nx, b.y + ny}, {b.x - nx, b.y - ny}, {a.x - nx, a.y - ny}, c);
}

void Renderer::SoftShadow(Point p, float rx, float ry, float length, float opacity)
{
    // Projected afternoon light: silhouettes share a single ground-only blurred mask.
    SurfaceQuad({p.x - rx, p.y - ry},
        {p.x + rx + length, p.y - ry + length * .34f},
        {p.x + rx + length, p.y + ry + length * .34f},
        {p.x - rx, p.y + ry},
        {-1, -1},
        {1, -1},
        {1, 1},
        {-1, 1},
        Material::Shadow,
        {0, 0, 0, opacity});
}

void Renderer::DrawSolidRect(float x, float y, float, float size, float r, float g, float b, float a)
{
    Rect(Width * .5f + x - size * .5f, Height * .5f - y - size * .5f, size, size, {r, g, b, a});
}
