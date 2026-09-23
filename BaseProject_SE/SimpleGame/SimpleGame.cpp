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
#include "LevelOne.h"
#include "Profiler.h"
#include "GameStorage.h"
#include <string>

namespace
{
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Tutorial> tutorial;
    std::unique_ptr<LevelOne> level;
    int previousTime = 0;
    HWND gameWindow = nullptr;

    void Display()
    {
        if (!renderer)
        {
            return;
        }

        Profile::Context(level ? "level_one" : "tutorial",
            level ? level->EffectsEnabled() : tutorial->EffectsEnabled(),
            GetForegroundWindow() == gameWindow);
        {
            Profile::Scope render("frame_render_cpu");
            if (level)
            {
                level->Render(*renderer);
            }
            else if (tutorial)
            {
                tutorial->Render(*renderer);
            }
        }
        {
            Profile::Scope present("swap_buffers_wait");
            glutSwapBuffers();
        }
        renderer->LogFrame();
    }

    void Resize(int width, int height)
    {
        if (renderer)
            renderer->Resize(width, height);
        glutPostRedisplay();
    }

    void Down(unsigned char key, int, int)
    {
        if (level)
        {
            level->KeyDown(key);
        }
        else if (tutorial)
        {
            tutorial->KeyDown(key);
        }
    }

    void Up(unsigned char key, int, int)
    {
        if (level)
        {
            level->KeyUp(key);
        }
        else if (tutorial)
        {
            tutorial->KeyUp(key);
        }
    }

    void Mouse(int button, int state, int x, int y)
    {
        Point canvas;

        if (level && renderer && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN &&
            renderer->ToCanvas(x, y, canvas))
        {
            level->AttackAt(canvas);
        }
    }

    void Close()
    {
        // Free GPU objects while freeglut still has the current context.
        level.reset();
        tutorial.reset();
        GameStorage::FlushPending();
        renderer.reset();
    }

    void Tick(int)
    {
        if (!tutorial && !level)
        {
            return;
        }

        int now = glutGet(GLUT_ELAPSED_TIME);
        float dt = (now - previousTime) * .001f;
        previousTime = now;

        bool focused = GetForegroundWindow() == gameWindow;

        Profile::Scope updateScope("game_update");
        if (level)
        {
            if (focused)
            {
                level->Update(dt);
            }
            else
            {
                level->ClearInput();
            }
        }
        else if (tutorial)
        {
            if (focused)
            {
                tutorial->Update(dt);
            }
            else
            {
                tutorial->ClearInput();
            }
        }

        if ((level && level->WantsQuit()) || (tutorial && tutorial->WantsQuit()))
        {
            glutLeaveMainLoop();
            return;
        }

        glutPostRedisplay();
        if (!Profile::Settings().uncapped)
        {
            glutTimerFunc(16, Tick, 0);
        }
    }

    void Idle()
    {
        Tick(0);
    }
}

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    bool tutorialMode = false;
    Profile::Options profileOptions;

    for (int i = 1; i < argc;)
    {
        std::string argument(argv[i]);
        bool consumed = true;
        if (argument == "--tutorial")
        {
            tutorialMode = true;
        }
        else if (argument == "--profile-per-frame")
        {
            profileOptions.perFrame = true;
        }
        else if (argument == "--profile-no-gpu")
        {
            profileOptions.gpu = false;
        }
        else if (argument == "--profile-gpu-every-frame")
        {
            profileOptions.gpuEvery = 1;
        }
        else if (argument == "--uncapped")
        {
            profileOptions.uncapped = true;
        }
        else
        {
            consumed = false;
        }
        if (consumed)
        {
            for (int j = i; j < argc; ++j)
            {
                argv[j] = argv[j + 1];
            }
            --argc;
        }
        else
        {
            ++i;
        }
    }

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
        SetWindowTextW(gameWindow, tutorialMode ? L"재와 갈대 - 마지막 모닥불" : L"재와 갈대 - 첫 사냥터");
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

    Profile::Start(profileOptions);
    Profile::Metadata(reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
        reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    renderer.reset(new Renderer(1280, 800));

    if (!renderer->IsInitialized())
    {
        std::cerr << "렌더러 초기화에 실패했습니다.\n";
        renderer.reset();
        GameStorage::Shutdown();
        Profile::Stop();

        return 1;
    }

    if (tutorialMode)
    {
        tutorial.reset(new Tutorial());
    }
    else
    {
        level.reset(new LevelOne());
    }
    Profile::Checkpoint("startup_complete");
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
    glutIgnoreKeyRepeat(1);
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(Down);
    glutKeyboardUpFunc(Up);
    glutMouseFunc(Mouse);
    glutCloseFunc(Close);
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    if (profileOptions.uncapped)
    {
        glutIdleFunc(Idle);
    }
    else
    {
        glutTimerFunc(16, Tick, 0);
    }
    glutMainLoop();
    level.reset();
    tutorial.reset();
    GameStorage::Shutdown();
    renderer.reset();
    Profile::Stop();

    return 0;
}
