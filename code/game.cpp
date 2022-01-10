struct PlayerController : GameObject
{
    float yVel = 0.f;
    GameCamera playerCamera;
    MeshCollider playerCollider;

    PlayerController()
    {
        playerCamera.CameraCalculatePerspectiveMatrix();
        playerCamera.bMouseControl = true;
    }

    void InitializePlayerController()
    {
        playerCollider.owningGameObject = this;
        playerCollider.verticesModelSpace = {
                {-16.f, 16.f, -16.f},
                {-16.f, 16.f, 16.f},
                {16.f, 16.f, 16.f},
                {16.f, 16.f, -16.f},
                {-16.f, -16.f, -16.f},
                {-16.f, -16.f, 16.f},
                {16.f, -16.f, 16.f},
                {16.f, -16.f, -16.f}
        };
        DynamicColliders.push_back(&playerCollider);
    }

    void CleanUpPlayerController()
    {

    }

    void Update() override
    {
        float moveSpeed = 128.f;
        float forwardMov = (float)(gCurrentKeyState[SDL_SCANCODE_W] - gCurrentKeyState[SDL_SCANCODE_S]) * moveSpeed * gDeltaTime;
        vec3 forwardDir = vec3(playerCamera.calculatedDirection.x, 0.f, playerCamera.calculatedDirection.z);
        Move(forwardDir * forwardMov);
        float rightMov = (float)(gCurrentKeyState[SDL_SCANCODE_D] - gCurrentKeyState[SDL_SCANCODE_A]) * moveSpeed * gDeltaTime;
        vec3 rightDir = vec3(playerCamera.calculatedRight.x, 0.f, playerCamera.calculatedRight.z);
        Move(rightDir * rightMov);

        if(gJustPressedKeyState[SDL_SCANCODE_SPACE])
        {
            yVel += 3.f;
        }

        yVel -= 9.81f * gDeltaTime;

        Move(vec3(0.f, yVel, 0.f));

        playerCamera.position = position;
        playerCamera.CameraUpdate();
    }

    void OnCollision(CollisionEventArgs collisionInfo) override
    {

        if(Dot(Normalize(collisionInfo.normal), SML_UP_VECTOR) > 0.5f)
        {
            Move(SML_UP_VECTOR * collisionInfo.depth);
            yVel = 0.f;
        }
        else if(Dot(Normalize(collisionInfo.normal), SML_UP_VECTOR) < -0.5f)
        {
            yVel = 0.f;
            Move(collisionInfo.normal * collisionInfo.depth);
        }
        else
        {
            Move(collisionInfo.normal * collisionInfo.depth);
        }
    }
};

INTERNAL PlayerController playerController;
INTERNAL std::vector<GameObject*> gameObjectsA;
INTERNAL PreAllocArray<GameObject> staticMapObjects;
INTERNAL PreAllocArray<MeshCollider> staticMapColliders;

INTERNAL void GameInitOnStartUp()
{
    staticMapObjects.Allocate(MAX_EDITOR_ENTITIES_COUNT);
    staticMapColliders.Allocate(MAX_EDITOR_ENTITIES_COUNT);
}

INTERNAL void GameLoad()
{
    SDL_SetRelativeMouseMode(SDL_TRUE);

    staticMapObjects.Clear();
    staticMapColliders.Clear();
    StaticColliders.clear();
    DynamicColliders.clear();

    GameImportMap("slopetest.map", staticMapObjects, staticMapColliders);
    for(u32 i = 0; i < staticMapColliders.Count(); ++i)
    {
        StaticColliders.push_back(&staticMapColliders[i]);
    }

    playerController = PlayerController();
    playerController.SetPos(vec3(0.f,32.f,0.f));
    playerController.InitializePlayerController();

    printf("game load\n");
}

INTERNAL void GameUnload()
{
    playerController.CleanUpPlayerController();

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
        sceneShader.GLBindMatrix4fv("viewMatrix", 1, playerController.playerCamera.viewMatrix.ptr());
        sceneShader.GLBindMatrix4fv("projMatrix", 1, playerController.playerCamera.perspectiveMatrix.ptr());
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
