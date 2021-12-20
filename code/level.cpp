/** Level geometry and Editor code */

struct EditorObject : GameObject
{
    // PARENT
    // CHILDREN
    // WE CAN HAVE EMPTY EDITOR OBJECT WHICH HAS NON-EMPTY EDITOR OBJECTS AS CHILDREN

    bool isVisible = true;
    bool hasCollision = true;
    Mesh mesh;

    void Update() override {}
    void OnCollision(CollisionEventArgs collisionInfo) override {}

    void Render(const Shader& shader) override
    {
        auto bo = GetBufferObjects();
        mesh.RebindBufferObjects(bo.first.data(), bo.second.data(), (u32)bo.first.size(), (u32)bo.second.size());
        shader.GLBindMatrix4fv("modelMatrix", 1, GetModelMatrix().ptr());
        mesh.RenderMesh();
    }

    virtual void Initialize() = 0;
    virtual void CleanUp() = 0;
    virtual std::pair<std::vector<float>, std::vector<u32>> GetBufferObjects() = 0;
    virtual std::vector<vec3> GetColliders() = 0;
};

// map from GameObject to EditorObject

struct EditorCubeObject : EditorObject
{
// - cube with arbitrary xyz lengths
//      - need to set vertex position XYZ based on size
//      - need to set texture coordinates UVs based on size
//      - normal vectors are constant
//      - indices into the above ^ can stay constant
//      - we need to set collider values appropriately based on above vao

    vec3 dimensions; // width, height, length
    vec3 originAsOffsetFromMinCorner = vec3(2.f, 0.2f, 1.f);
    vec2 textureScale = vec2(4.f, 2.f); // i.e. how many world units is one UV unit
    bool textureTileFlag = true;
    bool textureOrientationFlag = true;

    void Initialize() override
    {
        MeshCreate(mesh, nullptr, nullptr, 0, 0);
    }

    void CleanUp() override
    {
        MeshDelete(mesh);
    }

    std::pair<std::vector<float>, std::vector<u32>> GetBufferObjects() override
    {
        // Get Mesh VBO for vertex position, texture coords, normals, indices
        // Use Mesh VBOs to rebind a mesh on a Mesh+GameObject in the scene.
        // So there is a 1 to 1 mapping between the EditorObjects and GameObjects in the scene.
#pragma region MakingBufferObjects
        vec3 posArray[24] = {
                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {0.f, dimensions.y, dimensions.z},
                {0.f, dimensions.y, 0.f},

                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z },
                {dimensions.x, dimensions.y, dimensions.z},
                {0.f, dimensions.y, dimensions.z},

                {dimensions.x, 0.f, 0.f},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {dimensions.x, 0.f, 0.f},
                {dimensions.x, dimensions.y, 0.f},
                {0.f, dimensions.y, 0.f},

                {0.f, dimensions.y, 0.f},
                {0.f, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, 0.f, 0.f}
        };
        vec2 uvArray[24] = {
                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.x / textureScale.x, 0.f},
                {dimensions.x / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.x / textureScale.x, 0.f},
                {dimensions.x / textureScale.x, dimensions.y / textureScale.y},
                {0.f, dimensions.y / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.x / textureScale.y},
                {0.f, dimensions.x / textureScale.y},

                {0.f, 0.f},
                {dimensions.z / textureScale.x, 0.f},
                {dimensions.z / textureScale.x, dimensions.x / textureScale.y},
                {0.f, dimensions.x / textureScale.y},
        };

        if(!textureTileFlag)
        {
            uvArray[0] = {0.f, 0.f};
            uvArray[1] = {1.f, 0.f};
            uvArray[2] = {1.f, 1.f};
            uvArray[3] = {0.f, 1.f};

            uvArray[4] = {0.f, 0.f};
            uvArray[5] = {1.f, 0.f};
            uvArray[6] = {1.f, 1.f};
            uvArray[7] = {0.f, 1.f};

            uvArray[8] = {0.f, 0.f};
            uvArray[9] = {1.f, 0.f};
            uvArray[10] = {1.f, 1.f};
            uvArray[11] = {0.f, 1.f};

            uvArray[12] = {0.f, 0.f};
            uvArray[13] = {1.f, 0.f};
            uvArray[14] = {1.f, 1.f};
            uvArray[15] = {0.f, 1.f};

            uvArray[16] = {0.f, 0.f};
            uvArray[17] = {1.f, 0.f};
            uvArray[18] = {1.f, 1.f};
            uvArray[19] = {0.f, 1.f};

            uvArray[20] = {0.f, 0.f};
            uvArray[21] = {1.f, 0.f};
            uvArray[22] = {1.f, 1.f};
            uvArray[23] = {0.f, 1.f};
        }

        const vec3 normalArray[24] = {
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},
                {-1.f, 0.f, 0.f},

                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},
                {0.f, 0.f, 1.f},

                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},
                {1.f, 0.f, 0.f},

                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},
                {0.f, 0.f, -1.f},

                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},
                {0.f, 1.f, 0.f},

                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f},
                {0.f, -1.f, 0.f}
        };

        std::vector<float> vbo;
        std::vector<u32> indices = {
                0, 1, 2,
                3, 0, 2,

                4, 5, 6,
                7, 4, 6,

                8, 10, 9,
                11, 10, 8,

                12, 14, 13,
                15, 14, 12,

                16, 17, 18,
                19, 16, 18,

                20, 22, 21,
                23, 22, 20
        };

        for(int i = 0; i < 24; ++i)
        {
            // and THEN we use the origin offset to go and offset the vertex positions
            posArray[i].x -= originAsOffsetFromMinCorner.x;
            posArray[i].y -= originAsOffsetFromMinCorner.y;
            posArray[i].z -= originAsOffsetFromMinCorner.z;
        }

        if(textureOrientationFlag)
        {
            if(textureTileFlag)
            {
                uvArray[16] = {0.f, dimensions.z / textureScale.y};
                uvArray[17] = {0.f, 0.f};
                uvArray[18] = {dimensions.x / textureScale.x, 0.f};
                uvArray[19] = {dimensions.x / textureScale.x, dimensions.z / textureScale.y};

                uvArray[20] = {0.f, dimensions.z / textureScale.y};
                uvArray[21] = {0.f, 0.f};
                uvArray[22] = {dimensions.x / textureScale.x, 0.f};
                uvArray[23] = {dimensions.x / textureScale.x, dimensions.z / textureScale.y};
            }
            else
            {
                uvArray[16] = {0.f, 1.f};
                uvArray[17] = {0.f, 0.f};
                uvArray[18] = {1.f, 0.f};
                uvArray[19] = {1.f, 1.f};

                uvArray[20] = {0.f, 1.f};
                uvArray[21] = {0.f, 0.f};
                uvArray[22] = {1.f, 0.f};
                uvArray[23] = {1.f, 1.f};
            }
        }

        // populate vbo
        for(int i = 0; i < 24; ++i)
        {
            vbo.push_back(posArray[i].x);
            vbo.push_back(posArray[i].y);
            vbo.push_back(posArray[i].z);
            vbo.push_back(uvArray[i].x);
            vbo.push_back(uvArray[i].y);
            vbo.push_back(normalArray[i].x);
            vbo.push_back(normalArray[i].y);
            vbo.push_back(normalArray[i].z);
        }
#pragma endregion
        return { vbo, indices };
    }
    std::vector<vec3> GetColliders() override
    {
#pragma region MakeColliderVertices
        // Returns a vector of vertex positions for use with MeshCollider
        std::vector<vec3> colliderVertices = {
                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {0.f, dimensions.y, dimensions.z},
                {0.f, dimensions.y, 0.f},

                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z },
                {dimensions.x, dimensions.y, dimensions.z},
                {0.f, dimensions.y, dimensions.z},

                {dimensions.x, 0.f, 0.f},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {dimensions.x, 0.f, 0.f},
                {dimensions.x, dimensions.y, 0.f},
                {0.f, dimensions.y, 0.f},

                {0.f, dimensions.y, 0.f},
                {0.f, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, dimensions.z},
                {dimensions.x, dimensions.y, 0.f},

                {0.f, 0.f, 0.f},
                {0.f, 0.f, dimensions.z},
                {dimensions.x, 0.f, dimensions.z},
                {dimensions.x, 0.f, 0.f}
        };

        for(int i = 0; i < 24; ++i)
        {
            // and THEN we use the origin offset to go and offset the vertex positions
            colliderVertices[i].x -= originAsOffsetFromMinCorner.x;
            colliderVertices[i].y -= originAsOffsetFromMinCorner.y;
            colliderVertices[i].z -= originAsOffsetFromMinCorner.z;
        }
#pragma endregion
        return colliderVertices;
    }
};

// - planes and one-sided surfaces.
// - sphere with arbitrary radius
// - curves?
// - pyramids

INTERNAL GameCamera editorCamera;
INTERNAL EditorObject* selectedEditObj = nullptr;
INTERNAL std::vector<EditorObject*> editorObjects;
INTERNAL EditorCubeObject testEditorObj1;
INTERNAL EditorCubeObject testEditorObj2;
INTERNAL EditorCubeObject testEditorObj3;
INTERNAL EditorCubeObject testEditorObj4;
INTERNAL Texture textureA;

INTERNAL void EditorInitOnStartUp()
{
    editorCamera.CameraCalculatePerspectiveMatrix();
    editorCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    editorCamera.rotation = {0.0f, 31.56f, -21.f };
}

INTERNAL void EditorLoad()
{
    printf("editor load\n");

    testEditorObj1.dimensions = vec3(4.f, 0.4f, 2.f);
    testEditorObj1.SetPos(vec3(23,-3,2));
    testEditorObj1.Initialize();

    testEditorObj2.dimensions = vec3(2.f, 2.f, 1.f);
    testEditorObj2.SetPos(vec3(5,-3,2));
    testEditorObj2.Initialize();

    testEditorObj3.dimensions = vec3(1,3,20);
    testEditorObj3.SetPos(vec3(10,-5,2));
    testEditorObj3.Initialize();

    testEditorObj4.dimensions = vec3(5, 2, 3);
    testEditorObj4.SetPos(vec3(0,0,0));
    testEditorObj4.Initialize();

    editorObjects.push_back(&testEditorObj1);
    editorObjects.push_back(&testEditorObj2);
    editorObjects.push_back(&testEditorObj3);
    editorObjects.push_back(&testEditorObj4);

    TextureCreateFromFile(textureA, "data/primitives/TilesPlain0304.png");
}

INTERNAL void EditorUnload()
{
    printf("editor unload\n");

    testEditorObj1.CleanUp();
    testEditorObj2.CleanUp();
    testEditorObj3.CleanUp();
    testEditorObj4.CleanUp();
}

INTERNAL void EditorUpdate()
{
    editorCamera.CameraUpdate();
}

INTERNAL void EditorRender()
{
    glViewport(0, 0, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
    glClearColor(RGB(46,88,120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();

    sceneShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    sceneShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    sceneShader.GLBind3f("modelColor", RGB(0.f,0.f,0.f));

    textureA.UseTexture();
    for(auto editObj : editorObjects)
    {
        editObj->Render(sceneShader);
    }

    glUseProgram(0);
}

INTERNAL void EditorMousePick(int x, int y, float rayLength = 100.f)
{
    float xNDC = ((float) x - ((float)INTERNAL_RESOLUTION_WIDTH/2.f)) / ((float)INTERNAL_RESOLUTION_WIDTH/2.f);
    float yNDC = ((float) y - ((float)INTERNAL_RESOLUTION_HEIGHT/2.f)) / ((float)-INTERNAL_RESOLUTION_HEIGHT/2.f);
    mat4 inverseViewMatrix = editorCamera.viewMatrix.GetInverse();
    mat4 inverseProjMatrix = editorCamera.perspectiveMatrix.GetInverse();
    vec4 rayCCS = vec4(xNDC, yNDC, -1.f, 1.f);
    vec4 rayVCS = vec4((inverseProjMatrix * rayCCS).xyz, 0.f);
    vec3 rayWCS = Normalize((inverseViewMatrix * rayVCS).xyz);

    LineCollider lc;
    lc.a = editorCamera.position;
    lc.b = editorCamera.position + rayWCS * rayLength;

    // iterate editor/game objects to see which object was clicked on.
    std::vector<Collider*> collided;
    std::vector<MeshCollider> collidersToCheck;
    for(auto editObj : editorObjects)
    {
        MeshCollider col;
        col.owningGameObject = editObj;
        col.verticesModelSpace = editObj->GetColliders();
        collidersToCheck.push_back(col);
    }

    for(auto& col : collidersToCheck)
    {
        if(GJK(&col, &lc).hasCollision)
        {
            collided.push_back(&col);
        }
    }

    EditorObject* first = nullptr;
    float minDistanceSoFar = FLT_MAX;
    for(auto c : collided)
    {
        if(c->owningGameObject)
        {
            float distance = Magnitude(editorCamera.position - c->owningGameObject->GetPos());
            if(distance < minDistanceSoFar)
            {
                first = (EditorObject*) c->owningGameObject;
                minDistanceSoFar = distance;
            }
        }
    }

    if(first)
    {
        selectedEditObj = first;
    }
}

INTERNAL void EditorExportLevel()
{

}

INTERNAL void EditorImportLevel()
{

}

INTERNAL void GameImportLevel()
{

}