
INTERNAL EditorCamera gameCamera;
INTERNAL std::vector<GameObject*> gameObjectsA;
INTERNAL MeshCollider colA;

INTERNAL PreAllocArray<GameObject> staticMapObjects;
INTERNAL PreAllocArray<MeshCollider> staticMapColliders;

struct PlayerController : GameObject
{
    float yVel = 0.f;

    void Update() override
    {
        float moveSpeed = 4.f;
        float forwardMov = (float)(gCurrentKeyState[SDL_SCANCODE_UP] - gCurrentKeyState[SDL_SCANCODE_DOWN]) * moveSpeed * gDeltaTime;
        vec3 forwardDir = SML_FORWARD_VECTOR;//vec3(gameCamera.calculatedDirection.x, 0.f, gameCamera.calculatedDirection.z);
        Move(forwardDir * forwardMov);
        float rightMov = (float)(gCurrentKeyState[SDL_SCANCODE_RIGHT] - gCurrentKeyState[SDL_SCANCODE_LEFT]) * moveSpeed * gDeltaTime;
        vec3 rightDir = SML_RIGHT_VECTOR;//vec3(gameCamera.calculatedRight.x, 0.f, gameCamera.calculatedRight.z);
        Move(rightDir * rightMov);

        yVel -= 0.098f * gDeltaTime;
        Move(vec3(0.f, yVel, 0.f));


        //gameCamera.position = position;
        gameCamera.CameraUpdate();
    }

    void OnCollision(CollisionEventArgs collisionInfo) override
    {

        if(Dot(Normalize(-collisionInfo.normal), SML_UP_VECTOR) > 0.5f)
        {
            Move(SML_UP_VECTOR * collisionInfo.depth);
            yVel = 0.f;
        }
        else
        {
            Move(-collisionInfo.normal * collisionInfo.depth);
        }
    }
};

INTERNAL void GameInitOnStartUp()
{
    staticMapObjects.Allocate(MAX_EDITOR_ENTITIES_COUNT);
    staticMapColliders.Allocate(MAX_EDITOR_ENTITIES_COUNT);
    GameImportMap("slopetest.map", staticMapObjects, staticMapColliders);

    gameCamera.CameraCalculatePerspectiveMatrix();
    gameCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    gameCamera.rotation = {0.0f, 31.56f, -21.f };
    gameCamera.bMouseControl = true;

    auto objA = new PlayerController();
    gameObjectsA.push_back(objA);
    auto psA = new PropStatic();
    objA->SetClass(psA);
    psA->SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));

    colA.owningGameObject = objA;
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
    for(u32 i = 0; i < staticMapColliders.Count(); ++i)
    {
        StaticColliders.push_back(&staticMapColliders[i]);
    }

    // objA.SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));
    // objA.SetScale(vec3(1.4f,1.4f,1.4f));
    // gameObjectsA.push_back(&objA);
    // objB.SetModel(MeshGroupAssimpLoad("data/primitives/sphere.obj"));
    // objB.SetOrientation(quat());
    // objB.SetPos({ 4.f, 0.f, 0.f });
    // gameObjectsA.push_back(&objB);
    // colA.owningGameObject = &objA;
    // colA.verticesModelSpace = {
    //         {-0.5f, 0.5f, -0.5f},
    //         {-0.5f, 0.5f, 0.5f},
    //         {0.5f, 0.5f, 0.5f},
    //         {0.5f, 0.5f, -0.5f},
    //         {-0.5f, -0.5f, -0.5f},
    //         {-0.5f, -0.5f, 0.5f},
    //         {0.5f, -0.5f, 0.5f},
    //         {0.5f, -0.5f, -0.5f}
    // };
    // colB.owningGameObject = &objB;
    // colB.radius = 1.f;
    // DynamicColliders.push_back(&colA);
    // DynamicColliders.push_back(&colB);
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

    // gameObjectsA[0]->Move(
    //         {(gCurrentKeyState[SDL_SCANCODE_RIGHT] * 1.f + -gCurrentKeyState[SDL_SCANCODE_LEFT] * 1.f) * gDeltaTime,
    //          (gCurrentKeyState[SDL_SCANCODE_UP] * 1.f + -gCurrentKeyState[SDL_SCANCODE_DOWN] * 1.f) * gDeltaTime,
    //           0.f });
    // quat rot = EulerToQuat(vec3(0.f, 30.f, -47.f) * SML_DEG2RAD * gDeltaTime);
    // gameObjectsA[0]->Rotate(rot);

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
