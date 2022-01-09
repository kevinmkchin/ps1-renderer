INTERNAL GameCamera gameCamera;
INTERNAL std::vector<GameObject*> gameObjectsA;

INTERNAL PreAllocArray<GameObject> staticMapObjects;
INTERNAL PreAllocArray<MeshCollider> staticMapColliders;

struct PlayerController : GameObject
{
    float yVel = 0.f;

    void Update() override
    {
        float moveSpeed = 4.f;
        float forwardMov = (float)(gCurrentKeyState[SDL_SCANCODE_W] - gCurrentKeyState[SDL_SCANCODE_S]) * moveSpeed * gDeltaTime;
        vec3 forwardDir = vec3(gameCamera.calculatedDirection.x, 0.f, gameCamera.calculatedDirection.z);
        Move(forwardDir * forwardMov);
        float rightMov = (float)(gCurrentKeyState[SDL_SCANCODE_D] - gCurrentKeyState[SDL_SCANCODE_A]) * moveSpeed * gDeltaTime;
        vec3 rightDir = vec3(gameCamera.calculatedRight.x, 0.f, gameCamera.calculatedRight.z);
        Move(rightDir * rightMov);

        if(gJustPressedKeyState[SDL_SCANCODE_SPACE])
        {
            yVel += 0.1f;
        }

        yVel -= 0.325f * gDeltaTime;

        Move(vec3(0.f, yVel, 0.f));

        gameCamera.position = position;
        gameCamera.CameraUpdate();
    }

    void OnCollision(CollisionEventArgs collisionInfo) override
    {

        if(Dot(Normalize(-collisionInfo.normal), SML_UP_VECTOR) > 0.5f)
        {
            Move(SML_UP_VECTOR * collisionInfo.depth);
            yVel = 0.f;
        }
        else if(Dot(Normalize(-collisionInfo.normal), SML_UP_VECTOR) < -0.5f)
        {
            yVel = 0.f;
            Move(-collisionInfo.normal * collisionInfo.depth);
        }
        else
        {
            Move(-collisionInfo.normal * collisionInfo.depth);
        }
    }
};

INTERNAL PlayerController playerController;
INTERNAL MeshCollider colA;

INTERNAL void GameInitOnStartUp()
{
    staticMapObjects.Allocate(MAX_EDITOR_ENTITIES_COUNT);
    staticMapColliders.Allocate(MAX_EDITOR_ENTITIES_COUNT);
    GameImportMap("slopetest.map", staticMapObjects, staticMapColliders);
    for(u32 i = 0; i < staticMapColliders.Count(); ++i)
    {
        StaticColliders.push_back(&staticMapColliders[i]);
    }

    gameCamera.CameraCalculatePerspectiveMatrix();
    gameCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    gameCamera.rotation = {0.0f, 31.56f, -21.f };
    gameCamera.bMouseControl = true;

    playerController = PlayerController();
    auto psA = new PropStatic();
    playerController.SetClass(psA);
    psA->SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));

    colA.owningGameObject = &playerController;
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
    DynamicColliders.push_back(&colA);
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
    for(auto& gameObj : gameObjectsA)
    {
        gameObj->Update();
    }

    playerController.Update();

    CollisionWorldUpdate();
}

INTERNAL void GameRender()
{
    glViewport(0, 0, internalResolutionWidth, internalResolutionHeight);
    glClearColor(_RGB(46, 88, 120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();
        sceneShader.GLBindMatrix4fv("viewMatrix", 1, gameCamera.viewMatrix.ptr());
        sceneShader.GLBindMatrix4fv("projMatrix", 1, gameCamera.perspectiveMatrix.ptr());
        sceneShader.GLBind1i("internalResolutionWidth", internalResolutionWidth);
        sceneShader.GLBind1i("internalResolutionHeight", internalResolutionHeight);
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
