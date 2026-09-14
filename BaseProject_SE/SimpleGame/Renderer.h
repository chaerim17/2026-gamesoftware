#pragma once
#include <vector>
#include <string>
#include "Dependencies/glew.h"
struct Point { float x, y; };
struct Color {
    float r,g,b,a;
    Color(float red,float green,float blue,float alpha=1.f):r(red),g(green),b(blue),a(alpha){}
};
enum class Material { Solid=0, Grass=1, Dirt=2, Stone=3, Water=4, Shadow=5, Wood=6, Thatch=7, Sprite=8, Font=9 };
class Renderer {
public:
    Renderer(int width,int height);
    ~Renderer();
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    bool IsInitialized() const {return initialized;}
    void Resize(int width,int height);
    void Begin(float time=0,bool effects=true);
    void BeginShadows();
    void EndShadows();
    void FinishWorld();
    void End();
    void Triangle(Point a,Point b,Point c,Color color);
    void Quad(Point a,Point b,Point c,Point d,Color color);
    void SurfaceTriangle(Point a,Point b,Point c,Point ua,Point ub,Point uc,Material material,Color color);
    void SurfaceQuad(Point a,Point b,Point c,Point d,Point ua,Point ub,Point uc,Point ud,Material material,Color color);
    void Rect(float x,float y,float w,float h,Color color);
    void Ellipse(float x,float y,float rx,float ry,Color color,int segments=24);
    void Line(Point a,Point b,float thickness,Color color);
    void SoftShadow(Point center,float rx,float ry,float length,float opacity);
    void Character(Point feet,int appearance,int direction,int frame);
    void Text(float x,float y,const std::string& utf8,Color color,float scale=2.f,float maxWidth=0.f);
    float TextWidth(const std::string& utf8,float scale=2.f);
    void DrawSolidRect(float x,float y,float z,float size,float r,float g,float b,float a);
    static constexpr float Width=1280.f, Height=800.f;
private:
    struct Vertex {float x,y,r,g,b,a,u,v,kind;};
    struct FontCache;
    void Push(Point p,Color c,Point uv={0,0},Material m=Material::Solid);
    void Flush();
    GLuint Program(const char* vs,const char* fs);
    bool Target(GLuint& fbo,GLuint& texture,int w,int h,GLint format);
    void ScreenPass(GLuint target,int w,int h,int pass,GLuint texture,GLuint extra=0);
    bool CreateAssets();
    void ReleaseAssets();
    void BakeCharacters();
    GLuint program=0,post=0,vao=0,vbo=0;
    GLuint sceneFbo=0,sceneTex=0,shadowFbo=0,shadowTex=0;
    GLuint bloomFbo[2]={},bloomTex[2]={};
    GLuint sprites=0,fontTex=0;
    FontCache* font=nullptr;
    GLint canvasLocation=-1,timeLocation=-1,passLocation=-1,effectsLocation=-1;
    float canvasW=1280,canvasH=800,clock=0;
    int viewportX=0,viewportY=0,viewportW=1280,viewportH=800;
    bool initialized=false,worldFinished=false,postEnabled=true;
    std::vector<Vertex> vertices;
};

