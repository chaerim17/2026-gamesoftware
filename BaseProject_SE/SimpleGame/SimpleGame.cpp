/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)
This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include <windows.h>
#include <memory>
#include <iostream>
#include "Renderer.h"
#include "Tutorial.h"

namespace
{
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Tutorial> tutorial;
    int previousTime = 0;
    HWND gameWindow = nullptr;

    void Display()
    {
        if (!renderer || !tutorial)
            return;

        tutorial->Render(*renderer);
        glutSwapBuffers();
    }

    void Resize(int width, int height)
    {
        if (renderer)
            renderer->Resize(width, height);
        glutPostRedisplay();
    }

    void Down(unsigned char key, int, int)
    {
        if (tutorial)
            tutorial->KeyDown(key);
    }

    void Up(unsigned char key, int, int)
    {
        if (tutorial)
            tutorial->KeyUp(key);
    }

    void Close()
    {
        // Free GPU objects while freeglut still has the current context.
        renderer.reset();
        tutorial.reset();
    }

    void Tick(int)
    {
        if (!tutorial)
            return;

        int now = glutGet(GLUT_ELAPSED_TIME);
        float dt = (now - previousTime) * .001f;
        previousTime = now;

        if (GetForegroundWindow() == gameWindow)
            tutorial->Update(dt);
        else
            tutorial->ClearInput();

        if (tutorial->WantsQuit())
        {
            glutLeaveMainLoop();
            return;
        }

        glutPostRedisplay();
        glutTimerFunc(16, Tick, 0);
    }
}

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 800);
    glutCreateWindow(" ");
    gameWindow = WindowFromDC(wglGetCurrentDC());

    if (!gameWindow)
        gameWindow = GetActiveWindow();

    if (gameWindow)
        SetWindowTextW(gameWindow, L"재와 갈대 - 마지막 모닥불");
    glewExperimental = GL_TRUE;
    GLenum status = glewInit();

    if (status != GLEW_OK || !GLEW_VERSION_3_3)
    {
        std::cerr << "OpenGL 3.3 지원과 GLEW 초기화가 필요합니다.\n";

        if (status != GLEW_OK)
            std::cerr << glewGetErrorString(status) << '\n';

        return 1;
    }

    // GLEW can probe a legacy extension enum during core-context initialization.
    while (glGetError() != GL_NO_ERROR)
    {
    }

    renderer.reset(new Renderer(1280, 800));

    if (!renderer->IsInitialized())
    {
        std::cerr << "렌더러 초기화에 실패했습니다.\n";
        renderer.reset();

        return 1;
    }

    tutorial.reset(new Tutorial());
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(Down);
    glutKeyboardUpFunc(Up);
    glutCloseFunc(Close);
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, Tick, 0);
    glutMainLoop();
    tutorial.reset();
    renderer.reset();

    return 0;
}
