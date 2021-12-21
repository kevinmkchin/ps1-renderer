
INTERNAL GameCamera gameCamera;
INTERNAL std::vector<GameObject*> gameObjectsA;

INTERNAL PreAllocArray<DefaultGameObject> staticMapObjects;
INTERNAL PreAllocArray<MeshCollider> staticMapColliders;

INTERNAL DefaultGameObject objA;
INTERNAL DefaultGameObject objB;
INTERNAL MeshCollider colA;
INTERNAL SphereCollider colB;

INTERNAL void GameInitOnStartUp()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");

    staticMapObjects.Allocate(MAX_EDITOR_OBJECT_COUNT);
    staticMapColliders.Allocate(MAX_EDITOR_OBJECT_COUNT);
    GameImportMap("peepee", staticMapObjects, staticMapColliders);

    gameCamera.CameraCalculatePerspectiveMatrix();
    gameCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    gameCamera.rotation = {0.0f, 31.56f, -21.f };

    objA.SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));
    objA.SetScale(vec3(1.4f,1.4f,1.4f));
    gameObjectsA.push_back(&objA);
    objB.SetModel(MeshGroupAssimpLoad("data/primitives/sphere.obj"));
    objB.SetOrientation(quat());
    objB.SetPos({ 4.f, 0.f, 0.f });
    gameObjectsA.push_back(&objB);

    colA.owningGameObject = &objA;
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

    colB.owningGameObject = &objB;
    colB.radius = 1.f;

    DynamicColliders.push_back(&colA);
    DynamicColliders.push_back(&colB);
}

INTERNAL void GameLoad()
{
    printf("game load\n");
}

INTERNAL void GameUnload()
{
    printf("game unload\n");
}

INTERNAL void GameUpdate()
{
    gameCamera.CameraUpdate();

    for(auto& gameObj : gameObjectsA)
    {
        gameObj->Update();
    }

    gameObjectsA[0]->Move(
            {(gKeyState[SDL_SCANCODE_RIGHT] * 1.f + -gKeyState[SDL_SCANCODE_LEFT] * 1.f) * gDeltaTime,
             (gKeyState[SDL_SCANCODE_UP] * 1.f + -gKeyState[SDL_SCANCODE_DOWN] * 1.f) * gDeltaTime,
              0.f });
    quat rot = EulerToQuat(vec3(0.f, 30.f, -47.f) * SML_DEG2RAD * gDeltaTime);
    gameObjectsA[0]->Rotate(rot);

    CollisionWorldUpdate();
}

INTERNAL void GameRender()
{
    glViewport(0, 0, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
    glClearColor(_RGB(46, 88, 120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();
        sceneShader.GLBindMatrix4fv("viewMatrix", 1, gameCamera.viewMatrix.ptr());
        sceneShader.GLBindMatrix4fv("projMatrix", 1, gameCamera.perspectiveMatrix.ptr());
        sceneShader.GLBind3f("modelColor", _RGB(0, 0, 0));
        for(auto& gameObj : gameObjectsA)
        {
            gameObj->Render(sceneShader);
        }
        for(u32 i = 0; i < staticMapObjects.Count(); ++i)
        {
            staticMapObjects[i].Render(sceneShader);
        }
    glUseProgram(0);
}
