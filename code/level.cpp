/** Level geometry and Editor code */

enum EDIT_OBJ_CODE : u32
{
    EDIT_INVALID = 0x0000,

    EDIT_VISIBLE = 0x0001,
    EDIT_COLLIDABLE = 0x0002,

    EDIT_MESHGROUP = 0x0010,
    EDIT_BOX = 0x0020,
    EDIT_PLANE = 0x0040,
    EDIT_PYRAMID = 0x0080
};

struct EditorObject : GameObject
{
    // PARENT
    // CHILDREN
    // WE CAN HAVE EMPTY EDITOR OBJECT WHICH HAS NON-EMPTY EDITOR OBJECTS AS CHILDREN

    bool isVisible = true;
    bool hasCollision = true;

    void Update() override {}
    void OnCollision(CollisionEventArgs collisionInfo) override {}

    void Render(const Shader& shader) override
    {
        auto bo = GetBufferObjects();
        mesh.RebindBufferObjects(bo.first.data(), bo.second.data(), (u32)bo.first.size(), (u32)bo.second.size());
        shader.GLBindMatrix4fv("modelMatrix", 1, GetModelMatrix().ptr());
        shader.GLBind1i("isColliderOnly", 0);

        if(!isVisible)
        {
            shader.GLBind1i("isColliderOnly", 1);
            hasCollision ? shader.GLBind3f("modelColor", _RGB(0.f, 255.f, 0.f)) : shader.GLBind3f("modelColor", _RGB(255.f, 255.f, 255.f));
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        }
        mesh.RenderMesh();
        if(!isVisible)
        {
            shader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }
    }

    virtual void Initialize() = 0;  // Initialize for rendering purposes
    virtual void CleanUp() = 0;     // Clean up for rendering purposes
    virtual std::pair<std::vector<float>, std::vector<u32>> GetBufferObjects() = 0;
    virtual std::vector<vec3> GetColliders() = 0;
    EDIT_OBJ_CODE GetEditObjectCode() const { return editObjCode; }

    Texture texture; // TODO(Kevin): use texture
protected:
    Mesh mesh;
    EDIT_OBJ_CODE editObjCode = EDIT_INVALID;
};

struct EditorBoxObject : EditorObject
{
    vec3 dimensions; // width, height, length
    vec3 originAsOffsetFromMinCorner = vec3(0.f, 0.f, 0.f);
    vec2 textureScale = vec2(1.f, 1.f); // i.e. how many world units is one UV unit
    bool textureTileFlag = true;
    bool textureOrientationFlag = true;

    EditorBoxObject()
    {
        editObjCode = EDIT_BOX;
    }

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

#define MAX_EDITOR_OBJECT_COUNT 4096
#define MAX_EDITOR_BOX_COUNT 1024
INTERNAL PreAllocArray<EditorBoxObject> editorBoxObjects;
INTERNAL PreAllocArray<EditorObject*> editorObjects;

INTERNAL std::string loadedMapPath = "no map loaded...";
INTERNAL GameCamera editorCamera;
INTERNAL EditorObject* selectedEditObj = nullptr;
INTERNAL Texture textureA;

INTERNAL void EditorInitOnStartUp()
{
    editorObjects.Allocate(MAX_EDITOR_OBJECT_COUNT);
    editorBoxObjects.Allocate(MAX_EDITOR_BOX_COUNT);

    editorCamera.CameraCalculatePerspectiveMatrix();
    editorCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    editorCamera.rotation = {0.0f, 31.56f, -21.f };
}

INTERNAL void EditorExportMap(const std::string& mapName);
INTERNAL void EditorImportMap(const std::string& mapName);
INTERNAL void EditorLoad(const std::string& mapName)
{
    printf("editor load\n");

    // Load map
    if(!mapName.empty())
    {
        EditorImportMap(mapName);
    }

    // Load textures
    TextureCreateFromFile(textureA, "data/primitives/TilesPlain0304.png");

    // InitializeAllEditorObjects
    for(u32 i = 0; i < editorObjects.Count(); ++i)
    {
        editorObjects[i]->Initialize();
    }
}

INTERNAL void EditorUnload()
{
    printf("editor unload\n");

    selectedEditObj = nullptr;

    // CleanUpAllEditorObjects
    for(u32 i = 0; i < editorObjects.Count(); ++i)
    {
        editorObjects[i]->CleanUp();
    }
    // Reset editor object arrays
    editorObjects.Clear();
    editorBoxObjects.Clear();

    loadedMapPath = "no map loaded...";

    // Unload all textures
    TextureDelete(textureA);
}

INTERNAL void EditorSwitchMap(const std::string& mapName)
{
    if(!editorModeActiveFlag)
    {
        printf("WARNING: Editor mode is not active...\n");
        return;
    }
    EditorUnload();
    EditorLoad(mapName);
}

#if (defined _WIN32)
    #include <shobjidl.h>
    #include <codecvt>
#endif

INTERNAL void EditorUpdate()
{
    editorCamera.CameraUpdate();

    // Create a window called "My First Tool", with a menu bar.
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O"))
            {
#if (defined _WIN32)
                IFileOpenDialog *pFileOpen;
                HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                      IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
                if (SUCCEEDED(hr))
                {
                    // Show the Open dialog box.
                    hr = pFileOpen->Show(NULL);
                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        IShellItem *pItem;
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {
                            PWSTR pszFilePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            if (SUCCEEDED(hr))
                            {
                                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
                                std::string filePathString = converterX.to_bytes(pszFilePath);
                                EditorSwitchMap(filePathString);
                                CoTaskMemFree(pszFilePath);
                            }
                            pItem->Release();
                        }
                    }
                    pFileOpen->Release();
                }
                CoUninitialize();
#endif
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
#if (defined _WIN32)
                IFileSaveDialog* pFileSave;
                HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                                              IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
                if (SUCCEEDED(hr))
                {
                    // Show the Open dialog box.
                    hr = pFileSave->Show(NULL);
                    // Get the file name from the dialog box.
                    if (SUCCEEDED(hr))
                    {
                        IShellItem *pItem;
                        hr = pFileSave->GetResult(&pItem);
                        if (SUCCEEDED(hr))
                        {
                            PWSTR pszFilePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                            if (SUCCEEDED(hr))
                            {
                                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
                                std::string filePathString = converterX.to_bytes(pszFilePath);
                                EditorExportMap(filePathString);
                                CoTaskMemFree(pszFilePath);
                            }
                            pItem->Release();
                        }
                    }
                    pFileSave->Release();
                }
                CoUninitialize();
#endif
            }
            if (ImGui::MenuItem("Shut down"))  { gIsRunning = false; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text(loadedMapPath.c_str());
    ImGui::Text("Selected object: 0x%x", selectedEditObj);

    if(selectedEditObj)
    {
        vec3 selectedPos = selectedEditObj->GetPos();
        ImGui::DragFloat3("Pos", &selectedPos.x,0.01f);
        selectedEditObj->SetPos(selectedPos);
        vec3 selectedRot = QuatToEuler(selectedEditObj->GetOrientation()) * SML_RAD2DEG;
        ImGui::DragFloat3("Rot", &selectedRot.x,0.2f);
        selectedEditObj->SetOrientation(EulerToQuat(selectedRot * SML_DEG2RAD));
        vec3 selectedScale = selectedEditObj->GetScale();
        ImGui::DragFloat3("Scale", &selectedScale.x, 0.01f);
        selectedEditObj->SetScale(selectedScale);
        ImGui::Checkbox("Visible", &selectedEditObj->isVisible);
        ImGui::Checkbox("Collidable", &selectedEditObj->hasCollision);
        ImGui::Separator();
        switch(selectedEditObj->GetEditObjectCode())
        {
            case EDIT_BOX:{
                EditorBoxObject* box = (EditorBoxObject*) selectedEditObj;
                ImGui::DragFloat3("Dimensions", &box->dimensions.x,0.01f);
                ImGui::DragFloat3("Origin offset", &box->originAsOffsetFromMinCorner.x,0.01f);
                ImGui::DragFloat2("Texture scale", &box->textureScale.x,0.01f);
                ImGui::Checkbox("Tile texture", &box->textureTileFlag);
                ImGui::Checkbox("Texture orientation", &box->textureOrientationFlag);
            }break;
            default:{}break;
        }
    }

    ImGui::End();
}

INTERNAL void EditorRender()
{
    glViewport(0, 0, backbufferWidth, backbufferHeight);
    glClearColor(_RGB(46, 88, 120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    
    editorShader.UseShader();

    editorShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    editorShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    editorShader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));

    textureA.UseTexture();
    for(u32 i = 0; i < editorObjects.Count(); ++i)
    {
        editorObjects[i]->Render(editorShader);
    }

    glUseProgram(0);
}

INTERNAL void EditorMousePick(float xNDC, float yNDC, float rayLength = 100.f)
{
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
    for(u32 i = 0; i < editorObjects.Count(); ++i)
    {
        MeshCollider col;
        col.owningGameObject = editorObjects[i];
        col.verticesModelSpace = editorObjects[i]->GetColliders();
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

INTERNAL void EditorExportMap(const std::string& mapName)
{
    if(!editorModeActiveFlag)
    {
        printf("WARNING: Editor mode is not active...\n");
        return;
    }

    ByteBuffer exportBuffer = ByteBufferNew();
    for(u32 i = 0; i < editorObjects.Count(); ++i)
    {
        u32 code = editorObjects[i]->GetEditObjectCode()
                | (editorObjects[i]->isVisible ? EDIT_VISIBLE : EDIT_INVALID)
                | (editorObjects[i]->hasCollision ? EDIT_COLLIDABLE : EDIT_INVALID);
        ByteBufferWrite(&exportBuffer, u32, code);

        vec3 pos = editorObjects[i]->GetPos();
        quat rot = editorObjects[i]->GetOrientation();
        vec3 sca = editorObjects[i]->GetScale();
        ByteBufferWriteBulk(&exportBuffer, &pos, 3*sizeof(float));
        ByteBufferWriteBulk(&exportBuffer, &rot, 4*sizeof(float));
        ByteBufferWriteBulk(&exportBuffer, &sca, 3*sizeof(float));

        switch (editorObjects[i]->GetEditObjectCode())
        {
            case EDIT_BOX: {
                auto* boxObj = (EditorBoxObject*)editorObjects[i];
                vec3 dimensions = boxObj->dimensions;
                vec3 origin = boxObj->originAsOffsetFromMinCorner;
                vec2 textureScale = boxObj->textureScale;
                ByteBufferWriteBulk(&exportBuffer, &dimensions, 3*sizeof(float));
                ByteBufferWriteBulk(&exportBuffer, &origin, 3*sizeof(float));
                ByteBufferWriteBulk(&exportBuffer, &textureScale, 2*sizeof(float));
                ByteBufferWrite(&exportBuffer, bool, boxObj->textureTileFlag);
                ByteBufferWrite(&exportBuffer, bool, boxObj->textureOrientationFlag);
            } break;
            default: {
                printf("WARNING: An EditorObject doesn't have its editObjCode set or it is an unknown code.\n");
                continue;
            }
        }
    }
    std::string mapPath = mapName.at(1) == ':' ? mapName : GAME_MAPS_DIRECTORY + mapName;
    ByteBufferWriteToFile(&exportBuffer, mapPath.c_str());
    ByteBufferFree(&exportBuffer);
}

INTERNAL void EditorReadBoxFromByteBuffer(ByteBuffer* importBuffer, u32 editObjCode, EditorBoxObject* box)
{
    vec3 pos;
    quat rot;
    vec3 sca;
    ByteBufferReadBulk(importBuffer, &pos, 3*sizeof(float));
    ByteBufferReadBulk(importBuffer, &rot, 4*sizeof(float));
    ByteBufferReadBulk(importBuffer, &sca, 3*sizeof(float));
    ByteBufferReadBulk(importBuffer, &box->dimensions, 3*sizeof(float));
    ByteBufferReadBulk(importBuffer, &box->originAsOffsetFromMinCorner, 3*sizeof(float));
    ByteBufferReadBulk(importBuffer, &box->textureScale, 2*sizeof(float));
    ByteBufferRead(importBuffer, bool, &box->textureTileFlag);
    ByteBufferRead(importBuffer, bool, &box->textureOrientationFlag);
    box->isVisible = editObjCode & EDIT_VISIBLE;
    box->hasCollision = editObjCode & EDIT_COLLIDABLE;
    box->SetPos(pos);
    box->SetOrientation(rot);
    box->SetScale(sca);
}

INTERNAL void EditorImportMap(const std::string& mapName)
{
    if(!editorModeActiveFlag)
    {
        printf("WARNING: Editor mode is not active...\n");
        return;
    }

    ByteBuffer importBuffer;
    std::string mapPath = mapName.at(1) == ':' ? mapName : GAME_MAPS_DIRECTORY + mapName;
    ByteBufferReadFromFile(&importBuffer, mapPath.c_str());
    loadedMapPath = mapPath;
    while(importBuffer.position < importBuffer.size)
    {
        u32 code = EDIT_INVALID;
        ByteBufferRead(&importBuffer, u32, &code);

        switch (code & 0xFFFFFFFFC)
        {
            case EDIT_BOX: {
                EditorBoxObject* box = &editorBoxObjects.PushBack(EditorBoxObject());
                EditorReadBoxFromByteBuffer(&importBuffer, code, box);
                editorObjects.PushBack(box);
            }break;
            default: {
                continue;
            }
        }

        if(editorObjects.Count() >= editorObjects.Capacity())
        {
            printf("WARNING: Exceeding MAX_EDITOR_OBJECT_COUNT of %d. Stopping import.\n", editorObjects.Capacity());
            break;
        }
    }
    ByteBufferFree(&importBuffer);
}

INTERNAL void GameImportMap(const std::string& mapName,
                            PreAllocArray<DefaultGameObject>& staticMapObjects,
                            PreAllocArray<MeshCollider>& staticMapColliders)
{
    if(editorModeActiveFlag)
    {
        printf("WARNING: Attempting to load map when game is not active...\n");
        return;
    }

    ByteBuffer importBuffer;
    ByteBufferReadFromFile(&importBuffer,  (GAME_MAPS_DIRECTORY + mapName).c_str());
    while(importBuffer.position < importBuffer.size)
    {
        u32 code = EDIT_INVALID;
        ByteBufferRead(&importBuffer, u32, &code);

        switch (code & 0xFFFFFFFFC)
        {
            case EDIT_BOX: {
                EditorBoxObject box;
                EditorReadBoxFromByteBuffer(&importBuffer, code, &box);
                DefaultGameObject addGameObj;
                if(box.isVisible)
                {
                    auto bo = box.GetBufferObjects();
                    auto vbo = bo.first;
                    auto ibo = bo.second;
                    Mesh m;
                    MeshCreate(m, vbo.data(), ibo.data(), (u32)vbo.size(), (u32)ibo.size());
                    MeshGroup* mg = new MeshGroup(m, Texture());

                    addGameObj.SetModel(mg);
                    addGameObj.SetPos(box.GetPos());
                    addGameObj.SetOrientation(box.GetOrientation());
                    addGameObj.SetScale(box.GetScale());
                }
                staticMapObjects.PushBack(addGameObj);
                if(box.hasCollision)
                {
                    MeshCollider meshCollider;
                    meshCollider.owningGameObject = &staticMapObjects[staticMapObjects.Count()-1];
                    meshCollider.verticesModelSpace = box.GetColliders();
                    staticMapColliders.PushBack(meshCollider);
                }
            }break;
            default: {
                continue;
            }
        }
    }
    ByteBufferFree(&importBuffer);
}