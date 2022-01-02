/** Level geometry and Editor code */

INTERNAL vec3 EntityIdToRGB(EntityId id)
{
    float r = (float) ((id & 0x000000FF) >> 0);
    float g = (float) ((id & 0x0000FF00) >> 8);
    float b = (float) ((id & 0x00FF0000) >> 16);

    return vec3(r,g,b)/255.f;
}

INTERNAL EntityId RGBToEntityId(u8 r, u8 g, u8 b)
{
    EntityId pickedEntityId = 
    r + 
    g * 256 +
    b * 256 * 256;
    return pickedEntityId;
}

struct PropStatic : GameClass
{
    PropStatic()
        : model(nullptr) 
    {
        gameClassCode = CLASS_PROPSTATIC;
    }

    void Render(const Shader& shader) override
    {
        // PropStatic is always visible regardless of isVisible

        MeshGroup* modelToRender = GetModel();
        if(modelToRender)
        {
            shader.GLBindMatrix4fv("modelMatrix", 1, GetTiedEntity()->GetModelMatrix().ptr());
            modelToRender->Render();
        }
    }

    void EditorModeRender(const Shader& shader, bool mousePicking) override
    {
        Render(shader);
    }

    MeshGroup* GetModel() const
    {
        return model;
    }

    void SetModel(MeshGroup* newModelPtr)
    {
        model = newModelPtr;
    }

private:
    MeshGroup* model;
};

// void EditorModeRender(const Shader& shader, bool mousePicking) override
// {
//     auto bo = GetBufferObjects();
//     mesh.RebindBufferObjects(bo.first.data(), bo.second.data(), (u32)bo.first.size(), (u32)bo.second.size());
//     shader.GLBind1i("dontShade", 0);
//     if(!isVisible && !mousePicking)
//     {
//         shader.GLBind1i("dontShade", 1);
//         hasCollision ? shader.GLBind3f("modelColor", _RGB(0.f, 255.f, 0.f)) : shader.GLBind3f("modelColor", _RGB(255.f, 255.f, 255.f));
//         glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
//     }
//     shader.GLBindMatrix4fv("modelMatrix", 1, GetTiedEntity()->GetModelMatrix().ptr());
//     //shader.GLBindMatrix4fv("modelMatrix", 1, mat4().ptr());
//     mesh.RenderMesh();
//     if(!isVisible && !mousePicking)
//     {
//         shader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));
//         glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
//     }
// }

struct LevelGeometry : GameClass  // we could have subclasses of level geometry which do respond to collision or update
{
    /* Stripped down version of LevelGeometryBrush that can be used in game */

    u32 idVAO;
    u32 idVBO;

    struct {
        u32 idTexture;
        i32 numberOfIndices;
    } faces[8];
    u32 facesCount;

    LevelGeometry()
    {
        // set gameClassCode to whatever level geometry is

        // maybe we can generate VAOs and VBOs in the create level geometry function instead of the constructor
        // i.e. make the vaos vbos when importing map
        glGenVertexArrays(1, &idVAO);
        glBindVertexArray(idVAO);
            glGenBuffers(1, &idVBO);
            glBindBuffer(GL_ARRAY_BUFFER, idVBO);
                glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * (3 + 2)));
                glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    virtual ~LevelGeometry()
    {
        glDeleteBuffers(1, &idVBO);
        glDeleteVertexArrays(1, &idVAO);
    }

    void Render(const Shader& shader)
    {
        shader.GLBind1i("dontShade", 0);
        shader.GLBindMatrix4fv("modelMatrix", 1, GetTiedEntity()->GetModelMatrix().ptr());

        glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
        i32 indexStart = 0;
        for(u32 i = 0; i < facesCount; ++i)
        {
            // GL USE faces[i].idTexture use whatever texture defined in that face
            glDrawArrays(GL_TRIANGLES, indexStart, faces[i].numberOfIndices);
            indexStart += faces[i].numberOfIndices;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

struct LevelGeometryBrush : GameClass
{
    // Up to the map maker to make convex shapes only.
    // Counter clockwise winding order

    std::vector<vec3> vertices; // independently mutable

    struct Triangle
    {
        u8 in0;
        u8 in1;
        u8 in2;
    };
    struct Face
    {
        std::vector<Triangle> tris;
        // stuff like texture properties
        // what texture to show
        // texture offset (NOTE initial offset can be related to world position)
        // alignment, etc.

        i32 NumberOfIndices()
        {
            return (u32) (tris.size() * 3);
        }
    };
    std::vector<Face> faces;

    u32 idVAO;
    u32 idVBO;

    LevelGeometryBrush()
    {
        gameClassCode = BRUSH_LEVELGEOM;

        vertices = {
            { 0.f, 0.f, 0.f },
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { 1.f, 0.f, 0.f },
            { 1.f, 1.f, 0.f },
            { 0.f, 1.f, 1.f },
            { 1.f, 0.f, 1.f },
            { 1.f, 1.f, 1.f },
        };

        faces = {
            {{
                {0, 1, 5},
                {0, 5, 2},
            }},
            {{
                {1, 6, 7},
                {1, 7, 5},
            }},
            {{ 
                {6, 3, 4},
                {6, 4, 7},
            }},
            {{
                {3, 0, 2},
                {3, 2, 4},
            }},
            {{
                {2, 5, 7},
                {2, 7, 4},
            }},
            {{
                {1, 0, 3},
                {1, 3, 6}
            }},
        };

        glGenVertexArrays(1, &idVAO);
        glBindVertexArray(idVAO);
            glGenBuffers(1, &idVBO);
            glBindBuffer(GL_ARRAY_BUFFER, idVBO);
                glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, nullptr);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * 3));
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (void*)(sizeof(float) * (3 + 2)));
                glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }

    virtual ~LevelGeometryBrush()
    {
        glDeleteBuffers(1, &idVBO);
        glDeleteVertexArrays(1, &idVAO);
    }

    void Render(const Shader& shader) override
    {
        shader.GLBind1i("dontShade", 0);
        shader.GLBindMatrix4fv("modelMatrix", 1, GetTiedEntity()->GetModelMatrix().ptr());
        glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
        i32 indexStart = 0;
        for(int i = 0; i < faces.size(); ++i)
        {
            Face& face = faces[i];
            // use whatever texture defined in that face
            glDrawArrays(GL_TRIANGLES, indexStart, face.NumberOfIndices());
            indexStart += face.NumberOfIndices();
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void EditorModeRender(const Shader& shader, bool mousePicking) override
    {
        std::vector<float> vbo = GenerateBufferObject();
        glBindVertexArray(idVAO);
            glBindBuffer(GL_ARRAY_BUFFER, idVBO);
                glBufferData(GL_ARRAY_BUFFER, 4 * vbo.size(), vbo.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        Render(shader);
    }

    std::vector<float> GenerateBufferObject()
    {
        std::vector<float> vbo;
        // TODO resize vbo before pushback calls

        for(int f = 0; f < faces.size(); ++f)
        {
            Face& face = faces[f];
            vec3 vtx0 = vertices[face.tris[0].in0];
            vec3 vtx1 = vertices[face.tris[0].in1];
            vec3 vtx2 = vertices[face.tris[0].in2];
            vec3 uvNormal = Normalize(Cross(vtx1 - vtx0, vtx2 - vtx0)); // we could average instead?
            float upDownCheck = Dot(SML_UP_VECTOR, uvNormal);
            vec3 uAxis;
            if(upDownCheck > 0.98f) {
                uAxis = SML_RIGHT_VECTOR;
            } else if(upDownCheck < -0.98f) {
                uAxis = -SML_RIGHT_VECTOR;
            } else {
                uAxis = Normalize(Cross(SML_UP_VECTOR, uvNormal));
            }
            vec3 vAxis = -Normalize(Cross(uvNormal, uAxis));
            
            //     WORLD ALIGNMENT:
            //     // actually i dont think it matters that we are using up forward right but rather that 
            //     all the triangles are using the same
            //     uAxis = SML_FORWARD_VECTOR or SML_RIGHT_VECTOR; 
            //     vec3 vAxis = SML_UP_VECTOR;

            for(int t = 0; t < face.tris.size(); ++t)
            {
                u8 in0 = face.tris[t].in0;
                u8 in1 = face.tris[t].in1;
                u8 in2 = face.tris[t].in2;
                vec3 normal = Normalize(Cross(vertices[in1] - vertices[in0], vertices[in2] - vertices[in0]));

                // NOTE(Kevin): we could have texture id (which texture to use) as a vertex attribute
                vbo.push_back(vertices[in0].x);
                vbo.push_back(vertices[in0].y);
                vbo.push_back(vertices[in0].z);
                vbo.push_back(0.f);
                vbo.push_back(0.f);
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);

                vbo.push_back(vertices[in1].x);
                vbo.push_back(vertices[in1].y);
                vbo.push_back(vertices[in1].z);
                vbo.push_back(Dot(vertices[in1]-vertices[in0], uAxis));
                vbo.push_back(Dot(vertices[in1]-vertices[in0], vAxis));
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);

                vbo.push_back(vertices[in2].x);
                vbo.push_back(vertices[in2].y);
                vbo.push_back(vertices[in2].z);
                vbo.push_back(Dot(vertices[in2]-vertices[in0], uAxis));
                vbo.push_back(Dot(vertices[in2]-vertices[in0], vAxis));
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);
            }
        }

        return vbo;
    }
};

#define MAX_EDITOR_ENTITIES_COUNT 4096
#define MAX_EDITOR_BRUSH_COUNT 1024
INTERNAL PreAllocArray<GameObject> editorEntities;
INTERNAL GameObject* GetEditorObjectWithId(EntityId eid)
{
    if(editorEntities.Count() > 0)
    {
        EntityId firstId = editorEntities[0].GetId();
        EntityId indexForId = eid - firstId;
        if(editorEntities.Count() > indexForId)
        {
            return &editorEntities[indexForId];
        }
    }
    return nullptr;
}
INTERNAL GameCamera editorCamera;

enum EditorMode {
    EDITOR_MODE_SELECT,
    EDITOR_MODE_CREATE,
    EDITOR_MODE_EDIT
};

struct TranslationGizmo
{
    EntityId attachedObjId = 0;

    MeshGroup* arrowModel;
    EntityId XArrowId;
    EntityId YArrowId;
    EntityId ZArrowId;
    // EntityId XZPlaneId;
    // EntityId XYPlaneId;
    // EntityId YZPlaneId;

    enum ActiveTranslationAxis
    {
        GIZMO_TRANSLATION_INACTIVE,
        GIZMO_TRANSLATION_ACTIVE_X,
        GIZMO_TRANSLATION_ACTIVE_Y,
        GIZMO_TRANSLATION_ACTIVE_Z,
    };
    ActiveTranslationAxis activeAxis = GIZMO_TRANSLATION_INACTIVE;

    /* 
    For the translation and scaling gizmos, which are represented as 3 axes, 
    I do the following: I make a vector of the difference between the previous 
    and current mouse position in screen space. Then multiply this vector by 
    the inverse of the view and projection matrices, which gives me a vector 
    of the mouse movement in world space. Then I take the dot product of the 
    vector in world space with the selected axis. This allows me determine 
    if the mouse movement is in line with the axis or away from the axis.
    basically getting the projection of the mouse world vector onto the movement axis

    Rotation gizmo
    Project the center of the gizmo to screen space.
    Get the angle between the vectors GizmoToLastMouse and GizmoToCurrentMouse using the dot product 
    formula: acos(Dot(GizmoToLastMouse, GizmoToCurrentMouse) / (GizmoToLastMouse.Length() * GizmoToCurrentMouse.Length()))
    Get the sign of the angle by taking the cross of the GizmoToLastMouse and GizmoToCurrentMouse 
    and dotting it with the screen normal. Get another sign (times 1.0f or -1.0f) based on the 
    orientation of the gizmo, by taking the dot product between the rotated axis and a vector 
    from the center of the gizmo to the camera. Apply the rotation based on the angle and the
    two signs to the appropriate axis.

    */

    TranslationGizmo()
    {
        XArrowId = GetUniqueEntityId();
        YArrowId = GetUniqueEntityId();
        ZArrowId = GetUniqueEntityId();
    }

    void InitGizmo()
    {
        arrowModel = MeshGroupAssimpLoad("data/primitives/gizmo_arrow.obj");
    }

    void UpdateGizmo()
    {
        if(!gIsLeftMouseDown)
        {
            activeAxis = GIZMO_TRANSLATION_INACTIVE;
        }

        GameObject* attachedObj = GetEditorObjectWithId(attachedObjId);
        if(attachedObj && activeAxis != GIZMO_TRANSLATION_INACTIVE)
        {
            float scaledDistToObjFromCam = Magnitude(attachedObj->GetPos() - editorCamera.position);
            mat4 inverseViewMatrix = editorCamera.viewMatrix.GetInverse();
            mat4 inverseProjMatrix = editorCamera.perspectiveMatrix.GetInverse();
            float xNDC = gMouseDelta.x / ((float)backbufferWidth/2.f) * scaledDistToObjFromCam;
            float yNDC = -gMouseDelta.y / ((float)backbufferHeight/2.f) * scaledDistToObjFromCam;
            vec4 rayCCS = vec4(xNDC, yNDC, 0.f, 0.f);
            vec4 rayVCS = vec4((inverseProjMatrix * rayCCS).xyz, 0.f);
            vec3 rayWCS = (inverseViewMatrix * rayVCS).xyz;
            switch(activeAxis)
            {
                case GIZMO_TRANSLATION_ACTIVE_X:{
                    attachedObj->Move(vec3(Dot(SML_FORWARD_VECTOR, rayWCS), 0.f, 0.f));
                }break;
                case GIZMO_TRANSLATION_ACTIVE_Y:{
                    attachedObj->Move(vec3(0.f, Dot(SML_UP_VECTOR, rayWCS), 0.f));
                }break;
                case GIZMO_TRANSLATION_ACTIVE_Z:{
                    attachedObj->Move(vec3(0.f, 0.f, Dot(SML_RIGHT_VECTOR, rayWCS)));
                }break;
                default:{}
            }
        }
    }

    void RenderGizmo(const Shader& shader, bool mousePicking)
    {
        GameObject* attachedObj = GetEditorObjectWithId(attachedObjId);
        if(attachedObj)
        {
            float scaledDistToObjFromCam = Magnitude(attachedObj->GetPos() - editorCamera.position) * 0.13f;

            shader.GLBind1i("dontShade", 1);
            
            mat4 modelMatrix = TranslationMatrix(attachedObj->GetPos());
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            vec3 eidColor = EntityIdToRGB(XArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 1.f, 0.f, 0.f);
            arrowModel->Render();

            modelMatrix = TranslationMatrix(attachedObj->GetPos());
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            modelMatrix *= RotationMatrix(EulerToQuat(0.f,0.f,90.f*SML_DEG2RAD));
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            eidColor = EntityIdToRGB(YArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 0.f, 1.f, 0.f);
            arrowModel->Render();

            modelMatrix = TranslationMatrix(attachedObj->GetPos());
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            modelMatrix *= RotationMatrix(EulerToQuat(0.f,-90.f*SML_DEG2RAD,0.f));
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            eidColor = EntityIdToRGB(ZArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 0.f, 0.f, 1.f);
            arrowModel->Render();

            // TODO Render quads for moving along XY YZ XZ planes

            shader.GLBind1i("dontShade", 0);
        }
    }
};

INTERNAL TranslationGizmo transGizmo;
INTERNAL EditorMode editorMode = EDITOR_MODE_SELECT;
INTERNAL std::string loadedMapPath = "no map loaded...";
INTERNAL MeshGroup* defaultEntityModel = nullptr;
INTERNAL EntityId selectedEditObjId = 0;
INTERNAL Texture devUnitTexture;

INTERNAL void SetSelectedEditObjId(EntityId eid)
{
    selectedEditObjId = eid;
    transGizmo.attachedObjId = eid;
}

INTERNAL void EditorInitOnStartUp()
{
    editorEntities.Allocate(MAX_EDITOR_ENTITIES_COUNT);

    editorCamera.CameraCalculatePerspectiveMatrix();
    editorCamera.position = {-3.048528f, 1.879444f, 2.618031f };
    editorCamera.rotation = {0.0f, 31.56f, -21.f };

    defaultEntityModel = MeshGroupAssimpLoad("data/primitives/sphere.obj");

    transGizmo.InitGizmo();
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

    GameObject* test = &editorEntities.PushBackDefault();
    test->SetClass(new LevelGeometryBrush());

    // Load textures
    TextureCreateFromFile(devUnitTexture, "data/primitives/dev_unitwall11.png", GL_LINEAR_MIPMAP_LINEAR);
}

INTERNAL void EditorUnload()
{
    printf("editor unload\n");

// TODO(Kevin): make these editor brush objects not dynamically allocated. Or is it okay because its only for editor?
    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        delete(editorEntities[i].GetClass());
    }
    // Reset editor object arrays
    editorEntities.Clear();

    loadedMapPath = "no map loaded...";

    // Unload all textures
    TextureDelete(devUnitTexture);
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

void MoveCameraToSelectedEditorObject()
{
    auto selectedObj = GetEditorObjectWithId(selectedEditObjId);
    if(selectedObj)
    {
        editorCamera.position = selectedObj->GetPos() - editorCamera.calculatedDirection * 13.f;
    }
}

#pragma region EditorImGuiImGuiStuff

#if (defined _WIN32)
    #include <shobjidl.h>
    #include <codecvt>
#endif

INTERNAL std::string OpenFilePrompt()
{
    std::string filePathString;
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
                    filePathString = converterX.to_bytes(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    CoUninitialize();
#endif
    return filePathString;
}

INTERNAL std::string SaveFilePrompt()
{
    std::string filePathString;
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
                    filePathString = converterX.to_bytes(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }
    CoUninitialize();
#endif
    return filePathString;
}

INTERNAL void UpdateEditorImGui()
{
    ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_MenuBar);
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O"))
            {
                EditorSwitchMap(OpenFilePrompt());
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                EditorExportMap(SaveFilePrompt());
            }
            if (ImGui::MenuItem("Help"))
            {
                // TODO(Kevin): show some info about hotkeys controls etc
            }
            if (ImGui::MenuItem("Shut down"))  { gIsRunning = false; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::TextUnformatted(loadedMapPath.c_str());

    ImGui::Separator();
    ImGui::BeginGroup();
    if(ImGui::Button("Select"))
    {
        editorMode = EDITOR_MODE_SELECT;
    }
    ImGui::SameLine();
    if(ImGui::Button("Create"))
    {
        editorMode = EDITOR_MODE_CREATE;
    }
    ImGui::SameLine();
    if(ImGui::Button("Edit"))
    {
        editorMode = EDITOR_MODE_EDIT;
    }
    ImGui::EndGroup();
    ImGui::Separator();

    GameObject* selectedEditObj = GetEditorObjectWithId(selectedEditObjId);

    switch(editorMode)
    {
        case EDITOR_MODE_SELECT:{
            ImGui::Text("Selected object: %p", selectedEditObj);
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
                ImGui::Separator();

                /* Class specific IMGUI */
                switch(selectedEditObj->GetGameClassCode())
                {
                    case BRUSH_BLOCK:{
                        // BlockBrush* box = (BlockBrush*) selectedEditObj->GetClass();
                        // ImGui::Checkbox("Visible", &box->isVisible);
                        // ImGui::Checkbox("Collidable", &box->hasCollision);
                        // ImGui::DragFloat3("Dimensions", &box->dimensions.x, 0.01f);
                        // ImGui::DragFloat3("Origin offset", &box->originAsOffsetFromMinCorner.x, 0.01f);
                        // ImGui::DragFloat2("Texture scale", &box->textureScale.x, 0.01f);
                        // ImGui::Checkbox("Tile texture", &box->textureTileFlag);
                        // ImGui::Checkbox("Texture orientation", &box->textureOrientationFlag);
                    }break;
                    case CLASS_PROPSTATIC:{
                        PropStatic* ps = (PropStatic*) selectedEditObj->GetClass();
                        if(ImGui::Button("Load Model"))
                        {
                            std::string modelPath = OpenFilePrompt();
                            char cwd[PATH_MAX];
                            if (_getcwd(cwd, sizeof(cwd)) != NULL) 
                            {
                                modelPath = modelPath.substr(std::string(cwd).length()+1);
                            }
                            printf("PropStatic loaded model: %s\n", modelPath.c_str());
                            ps->SetModel(MeshGroupAssimpLoad(modelPath.c_str()));
                        }
                    }break;
                    case BRUSH_LEVELGEOM:{
                        LevelGeometryBrush* lgb = (LevelGeometryBrush*) selectedEditObj->GetClass();
                        for(int i = 0; i < lgb->vertices.size(); ++i)
                        {
                            vec3& v = lgb->vertices[i];
                            ImGui::PushID(i);
                            ImGui::DragFloat3("v", &v.x, 0.01f);
                            ImGui::PopID();
                        }
                    }break;
                    default:{
                        // use code reflection to list class properties
                    }break;
                }

                if(ImGui::Button("Go to Selected"))
                {
                    MoveCameraToSelectedEditorObject();
                }
            }
        }break;
        case EDITOR_MODE_CREATE:{
            enum EditorObjectCreateType {
                EDITOR_CREATE_TYPE_BRUSH,
                EDITOR_CREATE_TYPE_ENTITY
            };
            LOCAL_PERSIST EditorObjectCreateType editorObjectCreateType = EDITOR_CREATE_TYPE_BRUSH;
            ImGui::Text("Select editor object type");
            if(ImGui::Button("Brush"))
            {
                editorObjectCreateType = EDITOR_CREATE_TYPE_BRUSH;
            }
            ImGui::SameLine();
            if(ImGui::Button("Entity"))
            {
                editorObjectCreateType = EDITOR_CREATE_TYPE_ENTITY;
            }
            switch(editorObjectCreateType)
            {
                case EDITOR_CREATE_TYPE_BRUSH:{
                    ImGui::Text("Brush creation mode");

                    LOCAL_PERSIST vec3 createPosition;
                    LOCAL_PERSIST quat createOrientation;
                    LOCAL_PERSIST vec3 createScale = vec3(1.f, 1.f, 1.f);
                    LOCAL_PERSIST bool createIsVisible = true;
                    LOCAL_PERSIST bool createHasCollision = true;

                    // TODO(Kevin): Add dropdown to choose brush type

                    ImGui::DragFloat3("Pos", &createPosition.x, 0.01f);
                    vec3 selectedRot = QuatToEuler(createOrientation) * SML_RAD2DEG;
                    ImGui::DragFloat3("Rot", &selectedRot.x, 0.2f);
                    createOrientation = EulerToQuat(selectedRot * SML_DEG2RAD);
                    ImGui::DragFloat3("Scale", &createScale.x, 0.01f);
                    ImGui::Checkbox("Visible", &createIsVisible);
                    ImGui::Checkbox("Collidable", &createHasCollision);

                    LOCAL_PERSIST vec3 createDimensions;
                    ImGui::DragFloat3("Dimensions", &createDimensions.x, 0.01f);

                    if(ImGui::Button("Create Brush"))
                    {
                        GameObject* newEntity = &editorEntities.PushBackDefault();
                        newEntity->SetPos(createPosition);
                        newEntity->SetOrientation(createOrientation);
                        newEntity->SetScale(createScale);

                        // BlockBrush* box = new BlockBrush();
                        // newEntity->SetClass(box);
                        // box->isVisible = createIsVisible;
                        // box->hasCollision = createHasCollision;
                        // box->dimensions = createDimensions;

                        SetSelectedEditObjId(newEntity->GetId());
                    }
                }break;
                case EDITOR_CREATE_TYPE_ENTITY:{
                    ImGui::Text("Entity creation mode");

                    LOCAL_PERSIST vec3 createPosition;
                    LOCAL_PERSIST quat createOrientation;
                    LOCAL_PERSIST vec3 createScale = vec3(1.f, 1.f, 1.f);
                    LOCAL_PERSIST bool createIsVisible = true;
                    LOCAL_PERSIST bool createHasCollision = true;

                    ImGui::DragFloat3("Pos", &createPosition.x, 0.01f);
                    vec3 selectedRot = QuatToEuler(createOrientation) * SML_RAD2DEG;
                    ImGui::DragFloat3("Rot", &selectedRot.x, 0.2f);
                    createOrientation = EulerToQuat(selectedRot * SML_DEG2RAD);
                    ImGui::DragFloat3("Scale", &createScale.x, 0.01f);
                    ImGui::Checkbox("Visible", &createIsVisible);
                    ImGui::Checkbox("Collidable", &createHasCollision);

                    if(ImGui::Button("Create Entity"))
                    {
                        GameObject* newEntity = &editorEntities.PushBackDefault();
                        newEntity->SetPos(createPosition);
                        newEntity->SetOrientation(createOrientation);
                        newEntity->SetScale(createScale);

                        PropStatic* propStatic = new PropStatic();
                        newEntity->SetClass(propStatic);
                        propStatic->isVisible = createIsVisible;
                        propStatic->hasCollision = createHasCollision;
                        propStatic->SetModel(defaultEntityModel);

                        SetSelectedEditObjId(newEntity->GetId());
                    }
                }break;
            }
        }break;
        case EDITOR_MODE_EDIT:{

        }break;
    }

    ImGui::End();
}
#pragma endregion

INTERNAL void EditorUpdate()
{
    UpdateEditorImGui();

    editorCamera.CameraUpdate();

    transGizmo.UpdateGizmo();
}

INTERNAL void EditorRender()
{
    glViewport(0, 0, backbufferWidth, backbufferHeight);
    glClearColor(_RGB(46, 88, 120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    wireframeShader.UseShader();
    wireframeShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    wireframeShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    editorShader.UseShader();
    editorShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    editorShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    editorShader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));

    devUnitTexture.UseTexture();
    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        if(selectedEditObjId == editorEntities[i].GetId())
        {
            wireframeShader.UseShader();
            LOCAL_PERSIST float _breathef = 0.f;
            LOCAL_PERSIST vec4 selectColor;
            _breathef += gDeltaTime;
            selectColor.x = SML_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.y = SML_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.z = SML_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.w = SML_abs(sinf(_breathef))*0.7f + 0.3f;
            wireframeShader.GLBind4f("wireframeColor", selectColor.x, selectColor.y, selectColor.z, selectColor.w);
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            wireframeShader.SuppressWarningsPush();
            editorEntities[i].GetClass()->EditorModeRender(wireframeShader, false);
            wireframeShader.SuppressWarningsPop();
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
            editorShader.UseShader();
        }

        editorEntities[i].GetClass()->EditorModeRender(editorShader, false);
    }

    
    glDisable(GL_DEPTH_TEST);
    transGizmo.RenderGizmo(editorShader, false);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(0);
}

INTERNAL void EditorMousePick(i32 screenPixelX, i32 screenPixelY)
{
    glViewport(0, 0, backbufferWidth, backbufferHeight);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    mousepickShader.UseShader();
    mousepickShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    mousepickShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    mousepickShader.SuppressWarningsPush();

    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        vec3 rgbId = EntityIdToRGB(editorEntities[i].GetId());
        mousepickShader.GLBind3f("entityIdColor", rgbId.x, rgbId.y, rgbId.z);
        editorEntities[i].GetClass()->EditorModeRender(mousepickShader,
            gCurrentKeyState[SDL_SCANCODE_LSHIFT] ? false : true);
    }
    glDisable(GL_DEPTH_TEST);
    transGizmo.RenderGizmo(mousepickShader, true);
    glEnable(GL_DEPTH_TEST);

    mousepickShader.SuppressWarningsPop();
    glFlush();
    glFinish(); 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    u8 pickedRGBData[4];
    glReadPixels(screenPixelX, screenPixelY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pickedRGBData);
    glUseProgram(0);

    EntityId pickedId = RGBToEntityId(pickedRGBData[0], pickedRGBData[1], pickedRGBData[2]);
    if(pickedId != 0x00FFFFFF)
    {
        if(pickedId == transGizmo.XArrowId)
        {
            transGizmo.activeAxis = TranslationGizmo::GIZMO_TRANSLATION_ACTIVE_X;
            return;
        }
        if(pickedId == transGizmo.YArrowId)
        {
            transGizmo.activeAxis = TranslationGizmo::GIZMO_TRANSLATION_ACTIVE_Y;
            return;
        }
        if(pickedId == transGizmo.ZArrowId)
        {
            transGizmo.activeAxis = TranslationGizmo::GIZMO_TRANSLATION_ACTIVE_Z;
            return;
        }

        if(editorMode == EDITOR_MODE_SELECT)
        {
            SetSelectedEditObjId(pickedId);
        }
        else if(editorMode == EDITOR_MODE_CREATE)
        {
            
        }
        else
        {
            // we should be able to mousepick things while in create or edit mode
        }
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
    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        u32 code = editorEntities[i].GetGameClassCode()
                | (editorEntities[i].GetClass()->isVisible ? OBJECT_VISIBLE : CLASS_INVALID)
                | (editorEntities[i].GetClass()->hasCollision ? OBJECT_COLLIDABLE : CLASS_INVALID);
        ByteBufferWrite(&exportBuffer, u32, code);

        vec3 pos = editorEntities[i].GetPos();
        quat rot = editorEntities[i].GetOrientation();
        vec3 sca = editorEntities[i].GetScale();
        ByteBufferWriteBulk(&exportBuffer, &pos, 3*sizeof(float));
        ByteBufferWriteBulk(&exportBuffer, &rot, 4*sizeof(float));
        ByteBufferWriteBulk(&exportBuffer, &sca, 3*sizeof(float));

        switch (editorEntities[i].GetGameClassCode())
        {
            case BRUSH_BLOCK:{
                // auto* boxObj = (BlockBrush*) editorEntities[i].GetClass();
                // vec3 dimensions = boxObj->dimensions;
                // vec3 origin = boxObj->originAsOffsetFromMinCorner;
                // vec2 textureScale = boxObj->textureScale;
                // ByteBufferWriteBulk(&exportBuffer, &dimensions, 3*sizeof(float));
                // ByteBufferWriteBulk(&exportBuffer, &origin, 3*sizeof(float));
                // ByteBufferWriteBulk(&exportBuffer, &textureScale, 2*sizeof(float));
                // ByteBufferWrite(&exportBuffer, bool, boxObj->textureTileFlag);
                // ByteBufferWrite(&exportBuffer, bool, boxObj->textureOrientationFlag);
            }break;

            default:{
                printf("WARNING: An EditorBrush doesn't have its editObjCode set or it is an unknown code.\n");
            }
        }
    }
    std::string mapPath = mapName.at(1) == ':' ? mapName : GAME_MAPS_DIRECTORY + mapName;
    ByteBufferWriteToFile(&exportBuffer, mapPath.c_str());
    ByteBufferFree(&exportBuffer);
}

// INTERNAL void EditorReadBoxFromByteBuffer(ByteBuffer* importBuffer, BlockBrush* box)
// {
//     ByteBufferReadBulk(importBuffer, &box->dimensions, 3*sizeof(float));
//     ByteBufferReadBulk(importBuffer, &box->originAsOffsetFromMinCorner, 3*sizeof(float));
//     ByteBufferReadBulk(importBuffer, &box->textureScale, 2*sizeof(float));
//     ByteBufferRead(importBuffer, bool, &box->textureTileFlag);
//     ByteBufferRead(importBuffer, bool, &box->textureOrientationFlag);
// }

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
        u32 code = CLASS_INVALID;
        ByteBufferRead(&importBuffer, u32, &code);

        GameObject* newEntity = &editorEntities.PushBackDefault();
        vec3 pos;
        quat rot;
        vec3 sca;
        ByteBufferReadBulk(&importBuffer, &pos, 3*sizeof(float));
        ByteBufferReadBulk(&importBuffer, &rot, 4*sizeof(float));
        ByteBufferReadBulk(&importBuffer, &sca, 3*sizeof(float));
        newEntity->SetPos(pos);
        newEntity->SetOrientation(rot);
        newEntity->SetScale(sca);

        switch (code & 0xFFFFFFFFC)
        {
            case BRUSH_BLOCK: {
                // BlockBrush* box = new BlockBrush();
                // newEntity->SetClass(box);
                // box->isVisible = code & OBJECT_VISIBLE;
                // box->hasCollision = code & OBJECT_COLLIDABLE;
                // EditorReadBoxFromByteBuffer(&importBuffer, box);
            }break;

            default: { continue; }
        }

        if(editorEntities.Count() >= editorEntities.Capacity())
        {
            printf("WARNING: Exceeding MAX_EDITOR_ENTITIES_COUNT of %d. Stopping import.\n", editorEntities.Capacity());
            break;
        }
    }
    ByteBufferFree(&importBuffer);
}

INTERNAL void GameImportMap(const std::string& mapName,
                            PreAllocArray<GameObject>& staticMapObjects,
                            PreAllocArray<MeshCollider>& staticMapColliders)
{
    // if(editorModeActiveFlag)
    // {
    //     printf("WARNING: Attempting to load map when game is not active...\n");
    //     return;
    // }

    // /** If not a brush, then make GameObject with that class
    //     If brush, then make GameObject with brush's child class */

    // ByteBuffer importBuffer;
    // ByteBufferReadFromFile(&importBuffer,  (GAME_MAPS_DIRECTORY + mapName).c_str());
    // while(importBuffer.position < importBuffer.size)
    // {
    //     u32 code = CLASS_INVALID;
    //     ByteBufferRead(&importBuffer, u32, &code);

    //     switch (code & 0xFFFFFFFFC)
    //     {
    //         case BRUSH_BLOCK: {
    //             BlockBrush box;

    //             EditorReadBoxFromByteBuffer(&importBuffer, code, &box);
    //             DefaultGameObject addGameObj;
    //             if(box.isVisible)
    //             {
    //                 auto bo = box.GetBufferObjects();
    //                 auto vbo = bo.first;
    //                 auto ibo = bo.second;
    //                 Mesh m;
    //                 MeshCreate(m, vbo.data(), ibo.data(), (u32)vbo.size(), (u32)ibo.size());
    //                 MeshGroup* mg = new MeshGroup(m, Texture());

    //                 addGameObj.SetModel(mg);
    //                 addGameObj.SetPos(box.GetPos());
    //                 addGameObj.SetOrientation(box.GetOrientation());
    //                 addGameObj.SetScale(box.GetScale());
    //             }
    //             staticMapObjects.PushBack(addGameObj);
    //             if(box.hasCollision)
    //             {
    //                 MeshCollider meshCollider;
    //                 meshCollider.owningGameObject = &staticMapObjects[staticMapObjects.Count()-1];
    //                 meshCollider.verticesModelSpace = box.GetColliders();
    //                 staticMapColliders.PushBack(meshCollider);
    //             }
    //         }break;
    //         default: {
    //             continue;
    //         }
    //     }
    // }
    // ByteBufferFree(&importBuffer);
}