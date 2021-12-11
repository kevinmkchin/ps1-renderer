

INTERNAL GameCamera camera;
INTERNAL Shader sceneShader;
INTERNAL std::vector<GameObject> gameObjectsA;
INTERNAL std::vector<GameObject> gameObjectsB;

INTERNAL MeshCollider colA;
INTERNAL MeshCollider colB;
INTERNAL vec3 modelColor;

INTERNAL void GameInit()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    camera.CameraCalculatePerspectiveMatrix();
    camera.position = { -3.048528f, 1.879444f, 2.618031f };
    camera.rotation = { 0.0f, 31.56f, -21.f };

    GameObject newObj;
    newObj.SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));
    //newObj.SetOrientation(euler_to_quat(make_vec3(0.f, 30.f, -47.f) * KC_DEG2RAD));
    gameObjectsA.push_back(newObj);
    newObj.SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));
    newObj.SetOrientation(quat());
    newObj.SetPos({ 3.f, 0.f, 0.f });
    gameObjectsA.push_back(newObj);

    colA.owningGameObject = &gameObjectsA[0];
    colB.owningGameObject = &gameObjectsA[1];
    colA.verticesModelSpace = {
            {-0.5f, 0.5f, -0.5f},
            {-0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, -0.5f}
    };
    colB.verticesModelSpace = {
            {-0.5f, 0.5f, -0.5f},
            {-0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f, -0.5f},
            {-0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, -0.5f}
    };
}

INTERNAL void GameUpdate()
{
    camera.CameraUpdate();
    for(auto& gameObj : gameObjectsA)
    {
        gameObj.Update();
    }

    gameObjectsA[0].Move(
            {(gKeyState[SDL_SCANCODE_RIGHT] * 1.f + -gKeyState[SDL_SCANCODE_LEFT] * 1.f) * gDeltaTime,
             (gKeyState[SDL_SCANCODE_UP] * 1.f + -gKeyState[SDL_SCANCODE_DOWN] * 1.f) * gDeltaTime,
              0.f });
    gameObjectsA[0].Rotate(SML_UP_VECTOR * 30.f * gDeltaTime);
    gameObjectsA[0].Rotate(SML_RIGHT_VECTOR * -47.f * gDeltaTime);

    if(GJK(&colA, &colB))
    {
        modelColor = { RGB(255,0,0) };
    }
    else
    {
        modelColor = { RGB(255,255,255) };
    }
}

INTERNAL void GameRender()
{
    glViewport(0, 0, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
    glClearColor(RGB(46,88,120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();
        sceneShader.GLBindMatrix4fv("viewMatrix", 1, camera.viewMatrix.ptr());
        sceneShader.GLBindMatrix4fv("projMatrix", 1, camera.perspectiveMatrix.ptr());
        sceneShader.GLBind3f("modelColor", modelColor.x, modelColor.y, modelColor.z);
        for(auto& gameObj : gameObjectsA)
        {
            gameObj.Render(sceneShader);
        }
    glUseProgram(0);
}
