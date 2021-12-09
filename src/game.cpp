struct GameObject
{
    GameObject()
    {
        pos = { 0.f, 0.f, 0.f };
        orientation = identity_quaternion();
        scale = { 1.f, 1.f, 1.f };
        modelMatrix = identity_mat4();
        isModelMatrixOutdated = false;
        model = nullptr;
    }

    vec3 GetPos() const
    {
        return pos;
    }

    void SetPos(const vec3& newPos)
    {
        pos = newPos;
        isModelMatrixOutdated = true;
    }

    quaternion GetOrientation() const
    {
        return orientation;
    }

    void SetOrientation(const quaternion& newOrientation)
    {
        orientation = newOrientation;
        isModelMatrixOutdated = true;
    }

    vec3 GetScale() const
    {
        return scale;
    }

    void SetScale(const vec3& newScale)
    {
        scale = newScale;
        isModelMatrixOutdated = true;
    }

    const mat4& GetModelMatrix()
    {
        if(isModelMatrixOutdated)
        {
            modelMatrix = identity_mat4();
            modelMatrix *= translation_matrix(pos);
            modelMatrix *= rotation_matrix(orientation);
            modelMatrix *= scale_matrix(scale);
            isModelMatrixOutdated = false;
        }

        return modelMatrix;
    }

    MeshGroup* GetModel() const
    {
        return model;
    }

    void SetModel(MeshGroup* newModelPtr)
    {
        model = newModelPtr;
    }

    void Render(const Shader& shader)
    {
        MeshGroup* modelToRender = GetModel();
        if(modelToRender)
        {
            shader.GLBindMatrix4fv("modelMatrix", 1, GetModelMatrix().ptr());
            modelToRender->render();
        }
    }

private:
    vec3 pos; // World space
    quaternion orientation;
    vec3 scale;

    mat4 modelMatrix;
    bool32 isModelMatrixOutdated;

    MeshGroup* model;

    // POINTER to a collider which is described in MODEL SPACE
    // In order to do collision checks, we first move the collider by modelMatrix
    // Colliders are SHARED between objects e.g. cars in a racing game may all have same collider
    // Colliders have a hierarchy from BROAD to NEAR phase
    //   SPHERE A          SPHERE B
    //     |-- A1            |-- B1
    //     |-- A2            |-- B2
    //                       |-- B3
    //                       |-- B4
    // For a PlayStation 1 style game, a single Sphere for BROAD phase and child polygons for NEAR phase may be enough
};

INTERNAL GameCamera camera;
INTERNAL Shader sceneShader;
INTERNAL std::vector<GameObject> gameObjectsA;
INTERNAL std::vector<GameObject> gameObjectsB;

INTERNAL void GameInit()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    camera.CameraCalculatePerspectiveMatrix();
    camera.position = { -3.048528f, 1.879444f, 2.618031f };
    camera.rotation = { 0.0f, 31.56f, -21.f };

    GameObject newObj;
    newObj.SetModel(MeshGroupAssimpLoad("data/primitives/cube.obj"));
    gameObjectsA.push_back(newObj);
    newObj.SetModel(MeshGroupAssimpLoad("data/primitives/sphere.obj"));
    newObj.SetPos({ 3.f, 0.f, 0.f });
    gameObjectsA.push_back(newObj);
}

INTERNAL void GameUpdate()
{
    camera.CameraUpdate();
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
        for(auto& gameObj : gameObjectsA)
        {
            gameObj.Render(sceneShader);
        }
    glUseProgram(0);
}
