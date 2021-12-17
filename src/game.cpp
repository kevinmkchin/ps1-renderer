struct DefaultGameObject : GameObject
{
    void Update() override
    {

    }

    void OnCollision(CollisionEventArgs collisionInfo) override
    {

    }
};

INTERNAL GameCamera camera;
INTERNAL Shader sceneShader;
INTERNAL std::vector<GameObject*> gameObjectsA;

INTERNAL DefaultGameObject objA;
INTERNAL DefaultGameObject objB;
INTERNAL SphereCollider colA;
INTERNAL SphereCollider colB;
INTERNAL vec3 modelColor;

INTERNAL void GameInit()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    camera.CameraCalculatePerspectiveMatrix();
    camera.position = { -3.048528f, 1.879444f, 2.618031f };
    camera.rotation = { 0.0f, 31.56f, -21.f };

    objA.SetModel(MeshGroupAssimpLoad("data/primitives/sphere.obj"));
    objA.SetScale(vec3(1.4f,1.4f,1.4f));
    gameObjectsA.push_back(&objA);
    objB.SetModel(MeshGroupAssimpLoad("data/primitives/sphere.obj"));
    objB.SetOrientation(quat());
    objB.SetPos({ 4.f, 0.f, 0.f });
    gameObjectsA.push_back(&objB);

    colA.owningGameObject = &objA;
    colA.radius = 1.f;
//    colA.verticesModelSpace = {
//            {-0.5f, 0.5f, -0.5f},
//            {-0.5f, 0.5f, 0.5f},
//            {0.5f, 0.5f, 0.5f},
//            {0.5f, 0.5f, -0.5f},
//            {-0.5f, -0.5f, -0.5f},
//            {-0.5f, -0.5f, 0.5f},
//            {0.5f, -0.5f, 0.5f},
//            {0.5f, -0.5f, -0.5f}
//    };

    colB.owningGameObject = &objB;
    colB.radius = 1.f;

    DynamicColliders.push_back(&colA);
    DynamicColliders.push_back(&colB);
}

INTERNAL void GameUpdate()
{
    camera.CameraUpdate();
    for(auto& gameObj : gameObjectsA)
    {
        gameObj->Update();
    }

    gameObjectsA[0]->Move(
            {(gKeyState[SDL_SCANCODE_RIGHT] * 1.f + -gKeyState[SDL_SCANCODE_LEFT] * 1.f) * gDeltaTime,
             (gKeyState[SDL_SCANCODE_UP] * 1.f + -gKeyState[SDL_SCANCODE_DOWN] * 1.f) * gDeltaTime,
              0.f });
//    quat rot = EulerToQuat(vec3(0.f, 30.f, -47.f) * SML_DEG2RAD * gDeltaTime);
//    gameObjectsA[0].Rotate(rot);

    CollisionWorldUpdate();
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
            gameObj->Render(sceneShader);
        }
    glUseProgram(0);
}
