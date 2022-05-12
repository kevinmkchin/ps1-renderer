/** Level geometry and Editor code */

enum EditorMode {
    EDITOR_MODE_SELECT,
    EDITOR_MODE_CREATE,
    EDITOR_MODE_EDIT
};

INTERNAL EditorMode editorMode = EDITOR_MODE_SELECT;
INTERNAL Billboard vertexClickableBillboard;

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


struct VolumeBrush : GameClass
{
    // Up to the map maker to make convex shapes only.
    // Counter clockwise winding order

    struct Vertex
    {
        vec3 pos;
    };
    struct Triangle
    {
        // indices of array of vertices
        u8 in0;
        u8 in1;
        u8 in2;
    };
    struct Face
    {
        std::vector<Triangle> tris;

        Texture texture;
        vec2 textureShift = vec2(); //(TODO(Kevin) initial offset can be related to world position)
        vec2 textureScale = vec2(WORLD_UNIT_TO_UV_UNIT_RATIO, WORLD_UNIT_TO_UV_UNIT_RATIO);
        float textureRotation = 0.f;
        // texture alignment?

        i32 NumberOfIndices()
        {
            return (u32) (tris.size() * 3);
        }
    };

    std::vector<Vertex> vertices; // independently mutable
    std::vector<Face> faces;

    u32 idVAO;
    u32 idVBO;

    VolumeBrush()
    {
        gameClassCode = BRUSH_VOLUME;

        vertices = {
            { { 0.f, 0.f, 0.f } },
            { { 0.f, 0.f, 32.f } },
            { { 0.f, 32.f, 0.f } },
            { { 32.f, 0.f, 0.f } },
            { { 32.f, 32.f, 0.f } },
            { { 0.f, 32.f, 32.f } },
            { { 32.f, 0.f, 32.f } },
            { { 32.f, 32.f, 32.f } },
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
    }

    virtual ~VolumeBrush()
    {
        if(idVAO != 0 && idVBO != 0)
        {
            DeleteVolumeMeshOnGPU();
        }
    }

    void MakeVolumeMeshOnGPU()
    {
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

    void DeleteVolumeMeshOnGPU()
    {
        glDeleteBuffers(1, &idVBO);
        glDeleteVertexArrays(1, &idVAO);
    }

    void Render(const Shader& shader) override
    {
        ASSERT(0); // should never be called. crash.
    }

    void EditorModeRender(const Shader& shader, bool mousePicking) override;

    std::vector<float> GenerateBufferObject()
    {
        std::vector<float> vbo;

        std::vector<Vertex> verticesTransformed;
        for(int i = 0; i < vertices.size(); ++i)
        {
            Vertex v;
            v.pos = (GetTiedEntity()->GetModelMatrix() * vec4(vertices[i].pos, 1.f)).xyz;
            verticesTransformed.push_back(v);
        }

        for(int f = 0; f < faces.size(); ++f)
        {
            Face& face = faces[f];
            vec3 vtx0 = verticesTransformed[face.tris[0].in0].pos;
            vec3 vtx1 = verticesTransformed[face.tris[0].in1].pos;
            vec3 vtx2 = verticesTransformed[face.tris[0].in2].pos;
            vec3 uvNormal = Normalize(Cross(vtx1 - vtx0, vtx2 - vtx0)); // we could average instead?
            float upDownCheck = Dot(GM_UP_VECTOR, uvNormal);
            vec3 uAxis;
            if(upDownCheck > 0.98f) {
                uAxis = GM_RIGHT_VECTOR;
            } else if(upDownCheck < -0.98f) {
                uAxis = -GM_RIGHT_VECTOR;
            } else {
                uAxis = Normalize(Cross(GM_UP_VECTOR, uvNormal));
            }
            vec3 vAxis = -Normalize(Cross(uvNormal, uAxis));
            
            //     WORLD ALIGNMENT:
            //     // actually i dont think it matters that we are using up forward right but rather that 
            //     all the triangles are using the same
            //     uAxis = GM_FORWARD_VECTOR or GM_RIGHT_VECTOR; 
            //     vec3 vAxis = GM_UP_VECTOR;

            for(int t = 0; t < face.tris.size(); ++t)
            {
                u8 in0 = face.tris[t].in0;
                u8 in1 = face.tris[t].in1;
                u8 in2 = face.tris[t].in2;
                vec3 normal = Normalize(Cross(verticesTransformed[in1].pos - verticesTransformed[in0].pos, verticesTransformed[in2].pos - verticesTransformed[in0].pos));

                mat3 _uvRotMat = RotationMatrix2D(face.textureRotation);

                vbo.push_back(verticesTransformed[in0].pos.x);
                vbo.push_back(verticesTransformed[in0].pos.y);
                vbo.push_back(verticesTransformed[in0].pos.z);
                float _u = 0.f + face.textureShift.x;
                float _v = 0.f + face.textureShift.y;
                _u /= face.textureScale.x ? face.textureScale.x : 1.f;
                _v /= face.textureScale.y ? face.textureScale.y : 1.f;
                vec3 _uvRotResult = _uvRotMat * vec3(_u, _v, 1.f);
                vbo.push_back(_uvRotResult.x);
                vbo.push_back(_uvRotResult.y);
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);

                vbo.push_back(verticesTransformed[in1].pos.x);
                vbo.push_back(verticesTransformed[in1].pos.y);
                vbo.push_back(verticesTransformed[in1].pos.z);
                _u = Dot(verticesTransformed[in1].pos-verticesTransformed[in0].pos, uAxis) + face.textureShift.x;
                _v = Dot(verticesTransformed[in1].pos-verticesTransformed[in0].pos, vAxis) + face.textureShift.y;
                _u /= face.textureScale.x ? face.textureScale.x : 1.f;
                _v /= face.textureScale.y ? face.textureScale.y : 1.f;
                _uvRotResult = _uvRotMat * vec3(_u, _v, 1.f);
                vbo.push_back(_uvRotResult.x);
                vbo.push_back(_uvRotResult.y);
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);

                vbo.push_back(verticesTransformed[in2].pos.x);
                vbo.push_back(verticesTransformed[in2].pos.y);
                vbo.push_back(verticesTransformed[in2].pos.z);
                _u = Dot(verticesTransformed[in2].pos-verticesTransformed[in0].pos, uAxis) + face.textureShift.x;
                _v = Dot(verticesTransformed[in2].pos-verticesTransformed[in0].pos, vAxis) + face.textureShift.y;
                _u /= face.textureScale.x ? face.textureScale.x : 1.f;
                _v /= face.textureScale.y ? face.textureScale.y : 1.f;
                _uvRotResult = _uvRotMat * vec3(_u, _v, 1.f);
                vbo.push_back(_uvRotResult.x);
                vbo.push_back(_uvRotResult.y);
                vbo.push_back(normal.x);
                vbo.push_back(normal.y);
                vbo.push_back(normal.z);
            }
        }

        return vbo;
    }

    void Serialize(ByteBuffer* writeBuffer) override
    {
        u64 _verticesCount = vertices.size();
        ByteBufferWrite(writeBuffer, u64, _verticesCount);
        for(int i = 0; i < _verticesCount; ++i)
        {
            ByteBufferWrite(writeBuffer, vec3, vertices[i].pos);
        }
        u64 _facesCount = faces.size();
        ByteBufferWrite(writeBuffer, u64, _facesCount);
        for(int i = 0; i < _facesCount; ++i)
        {
            const Face& _face = faces[i];
            u64 _trisCount = _face.tris.size();
            ByteBufferWrite(writeBuffer, u64, _trisCount);
            for(int j = 0; j < _trisCount; ++j)
            {
                ByteBufferWrite(writeBuffer, u8, _face.tris[j].in0);
                ByteBufferWrite(writeBuffer, u8, _face.tris[j].in1);
                ByteBufferWrite(writeBuffer, u8, _face.tris[j].in2);
            }
            const std::string& _faceTexturePath = _face.texture.filePath;
            u64 _faceTexturePathSize = _faceTexturePath.size() + 1;
            ByteBufferWrite(writeBuffer, u64, _faceTexturePathSize);
            ByteBufferWriteBulk(writeBuffer, (void*)_faceTexturePath.c_str(), _faceTexturePathSize); 
            ByteBufferWrite(writeBuffer, vec2, _face.textureShift);
            ByteBufferWrite(writeBuffer, vec2, _face.textureScale);
            ByteBufferWrite(writeBuffer, float, _face.textureRotation);
        }
    }

    void Deserialize(ByteBuffer* readBuffer) override
    {
        u64 _verticesCount;
        ByteBufferRead(readBuffer, u64, &_verticesCount);
        vertices.resize(_verticesCount);
        for(int i = 0; i < _verticesCount; ++i)
        {
            ByteBufferRead(readBuffer, vec3, &vertices[i].pos);
        }
        u64 _facesCount;
        ByteBufferRead(readBuffer, u64, &_facesCount);
        faces.resize(_facesCount);
        for(int i = 0; i < _facesCount; ++i)
        {
            Face& _face = faces[i];
            u64 _trisCount;
            ByteBufferRead(readBuffer, u64, &_trisCount);
            _face.tris.resize(_trisCount);
            for(int j = 0; j < _trisCount; ++j)
            {
                ByteBufferWrite(readBuffer, u8, _face.tris[j].in0);
                ByteBufferWrite(readBuffer, u8, _face.tris[j].in1);
                ByteBufferWrite(readBuffer, u8, _face.tris[j].in2);
            }
            u64 _faceTexturePathSize;
            char _tempTexturePathBuffer[128];
            ByteBufferRead(readBuffer, u64, &_faceTexturePathSize);
            ByteBufferReadBulk(readBuffer, _tempTexturePathBuffer, _faceTexturePathSize);
            if(_faceTexturePathSize > 1)
            {
                TextureCreateFromFile(_face.texture, _tempTexturePathBuffer);
            }
            ByteBufferRead(readBuffer, vec2, &_face.textureShift);
            ByteBufferRead(readBuffer, vec2, &_face.textureScale);
            ByteBufferRead(readBuffer, float, &_face.textureRotation);
        }
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
INTERNAL EditorCamera editorCamera;
#define NumberOfReservedClickableVertexIds 32
INTERNAL EntityId reservedClickableVertexIds[NumberOfReservedClickableVertexIds];
INTERNAL std::vector<VolumeBrush::Vertex*> selectedClickableVertices;
#define NumberOfReservedClickableFaceIds 32
INTERNAL EntityId reservedClickableFaceIds[NumberOfReservedClickableFaceIds];
INTERNAL std::vector<VolumeBrush::Face*> selectedClickableFaces;

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

            if(editorMode == EDITOR_MODE_EDIT)
            {
                vec3 vertexTranslationDelta;
                switch(activeAxis)
                {
                    case GIZMO_TRANSLATION_ACTIVE_X:{
                        vertexTranslationDelta = vec3(Dot(GM_FORWARD_VECTOR, rayWCS), 0.f, 0.f);
                    }break;
                    case GIZMO_TRANSLATION_ACTIVE_Y:{
                        vertexTranslationDelta = vec3(0.f, Dot(GM_UP_VECTOR, rayWCS), 0.f);
                    }break;
                    case GIZMO_TRANSLATION_ACTIVE_Z:{
                        vertexTranslationDelta = vec3(0.f, 0.f, Dot(GM_RIGHT_VECTOR, rayWCS));
                    }break;
                    default:{}
                }
                mat4 attachedObjModelMatrix = attachedObj->GetModelMatrix();

                std::vector<VolumeBrush::Vertex*> _verticesToMove;
                if(!selectedClickableVertices.empty())
                {
                    _verticesToMove = selectedClickableVertices;
                }
                else if(!selectedClickableFaces.empty() && attachedObj->GetGameClassCode() == BRUSH_VOLUME)
                {
                    _verticesToMove = GetVerticesOfSelectedFaces(attachedObj);
                }
                for(auto vptr : _verticesToMove)
                {
                    vptr->pos += (attachedObjModelMatrix.GetInverse() * vec4(vertexTranslationDelta, 0.f)).xyz;
                }

                return;
            }
            else
            {
                switch(activeAxis)
                {
                    case GIZMO_TRANSLATION_ACTIVE_X:{
                        attachedObj->Move(vec3(Dot(GM_FORWARD_VECTOR, rayWCS), 0.f, 0.f));
                    }break;
                    case GIZMO_TRANSLATION_ACTIVE_Y:{
                        attachedObj->Move(vec3(0.f, Dot(GM_UP_VECTOR, rayWCS), 0.f));
                    }break;
                    case GIZMO_TRANSLATION_ACTIVE_Z:{
                        attachedObj->Move(vec3(0.f, 0.f, Dot(GM_RIGHT_VECTOR, rayWCS)));
                    }break;
                    default:{}
                }
            }
        }
    }

    void RenderGizmo(const Shader& shader, bool mousePicking)
    {
        GameObject* attachedObj = GetEditorObjectWithId(attachedObjId);
        if(attachedObj)
        {
            vec3 attachedPos = attachedObj->GetPos();
            if(editorMode == EDITOR_MODE_EDIT)
            {
                if(!selectedClickableVertices.empty() || !selectedClickableFaces.empty())
                {
                    std::vector<VolumeBrush::Vertex*> _verticesToRenderGizmoFor;
                    if(!selectedClickableVertices.empty())
                    {
                        _verticesToRenderGizmoFor = selectedClickableVertices;
                    }
                    else
                    {
                        _verticesToRenderGizmoFor = GetVerticesOfSelectedFaces(attachedObj);
                    }
                    vec3 avgSelectedVerticesPos;
                    for(auto vptr : _verticesToRenderGizmoFor)
                    {
                        avgSelectedVerticesPos += (attachedObj->GetModelMatrix() * vec4(vptr->pos, 1.f)).xyz;
                    }
                    avgSelectedVerticesPos /= (float) _verticesToRenderGizmoFor.size();
                    attachedPos = avgSelectedVerticesPos;
                }
                else
                {
                    return;
                }
            }

            float scaledDistToObjFromCam = Magnitude(attachedPos - editorCamera.position) * 0.13f;

            shader.GLBind1i("dontShade", 1);
            
            mat4 modelMatrix = TranslationMatrix(attachedPos);
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            vec3 eidColor = EntityIdToRGB(XArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 1.f, 0.f, 0.f);
            arrowModel->Render();

            modelMatrix = TranslationMatrix(attachedPos);
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            modelMatrix *= RotationMatrix(EulerToQuat(0.f,0.f,90.f*GM_DEG2RAD));
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            eidColor = EntityIdToRGB(YArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 0.f, 1.f, 0.f);
            arrowModel->Render();

            modelMatrix = TranslationMatrix(attachedPos);
            modelMatrix *= ScaleMatrix(scaledDistToObjFromCam, scaledDistToObjFromCam, scaledDistToObjFromCam);
            modelMatrix *= RotationMatrix(EulerToQuat(0.f,-90.f*GM_DEG2RAD,0.f));
            shader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());
            eidColor = EntityIdToRGB(ZArrowId);
            mousePicking ? shader.GLBind3f("entityIdColor", eidColor.x, eidColor.y, eidColor.z) 
                : shader.GLBind3f("modelColor", 0.f, 0.f, 1.f);
            arrowModel->Render();

            // TODO Render quads for moving along XY YZ XZ planes

            shader.GLBind1i("dontShade", 0);
        }
    }

private:
    std::vector<VolumeBrush::Vertex*> GetVerticesOfSelectedFaces(GameObject* attachedObj)
    {
        std::vector<u32> vertIdxOfFaces;
        for(auto fptr : selectedClickableFaces)
        {
            for(const auto& _tri : fptr->tris)
            {
                if(std::find(vertIdxOfFaces.begin(), vertIdxOfFaces.end(), _tri.in0) == vertIdxOfFaces.end()) {
                    vertIdxOfFaces.push_back(_tri.in0);
                }
                if(std::find(vertIdxOfFaces.begin(), vertIdxOfFaces.end(), _tri.in1) == vertIdxOfFaces.end()) {
                    vertIdxOfFaces.push_back(_tri.in1);
                }
                if(std::find(vertIdxOfFaces.begin(), vertIdxOfFaces.end(), _tri.in2) == vertIdxOfFaces.end()) {
                    vertIdxOfFaces.push_back(_tri.in2);
                }
            }
        }
        std::vector<VolumeBrush::Vertex*> verticesOfSelectedFaces;
        VolumeBrush* _vb_ = (VolumeBrush*)attachedObj->GetClass();
        if(_vb_)
        {
            for(auto _i : vertIdxOfFaces)
            {
                verticesOfSelectedFaces.push_back(&_vb_->vertices[_i]);
            }
        }
        return verticesOfSelectedFaces;
    }
};

INTERNAL TranslationGizmo transGizmo;
INTERNAL std::string loadedMapPath = "no map loaded...";
INTERNAL MeshGroup* defaultEntityModel = nullptr;
INTERNAL EntityId selectedEditObjId = 0;

INTERNAL void SetSelectedEditObjId(EntityId eid)
{
    selectedEditObjId = eid;
    transGizmo.attachedObjId = eid;

    selectedClickableVertices.clear();
    selectedClickableFaces.clear();

    GameObject* eobj = GetEditorObjectWithId(eid);
    if(eobj && eobj->GetGameClassCode() == BRUSH_VOLUME)
    {
        VolumeBrush* _vb = (VolumeBrush*) eobj->GetClass();
        if(_vb)
        {
            for(auto& _f : _vb->faces)
            {
                selectedClickableFaces.push_back(&_f);
            }
        }
    }
}

INTERNAL void EditorInitOnStartUp()
{
    editorEntities.Allocate(MAX_EDITOR_ENTITIES_COUNT);

    editorCamera.CameraCalculatePerspectiveMatrix();
    editorCamera.position = {-96.f, 64.f, 80.f };
    editorCamera.rotation = {0.0f, 31.56f, -21.f };

    defaultEntityModel = MeshGroupAssimpLoad("data/primitives/sphere.obj");

    transGizmo.InitGizmo();

    for(int i = 0; i < NumberOfReservedClickableVertexIds; ++i)
    {
        reservedClickableVertexIds[i] = GetUniqueEntityId();
    }
    for(int i = 0; i < NumberOfReservedClickableFaceIds; ++i)
    {
        reservedClickableFaceIds[i] = GetUniqueEntityId();
    }
}

INTERNAL void EditorExportMap(const std::string& mapName);
INTERNAL void EditorImportMap(const std::string& mapName);
INTERNAL void EditorLoad(const std::string& mapName)
{
    SDL_SetRelativeMouseMode(SDL_FALSE);

    // Load map
    if(!mapName.empty())
    {
        EditorImportMap(mapName);
    }

    vertexClickableBillboard.target = &editorCamera;
    vertexClickableBillboard.InitQuadMesh(0.1f, 0.1f, vec2(0.05f, 0.05f));

    printf("editor load\n");
}

INTERNAL void EditorUnload()
{
// TODO(Kevin): make these editor brush objects not dynamically allocated. Or is it okay because its only for editor?
    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        delete(editorEntities[i].GetClass());
    }
    // Reset editor object arrays
    editorEntities.Clear();

    loadedMapPath = "no map loaded...";

    vertexClickableBillboard.DeleteQuadMesh();

    printf("editor unload\n");
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
        editorCamera.position = selectedObj->GetPos() - editorCamera.calculatedDirection * 416.f;
    }
}

#include "editor_imgui.cpp"

INTERNAL void EditorUpdate()
{
    UpdateEditorImGui();

    editorCamera.CameraUpdate();

    transGizmo.UpdateGizmo();
}

INTERNAL void EditorRender()
{
    glViewport(0, 0, backbufferWidth, backbufferHeight);
    glClearColor(_RGB(18, 23, 17), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    billboardShader.UseShader();
    billboardShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    billboardShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    wireframeShader.UseShader();
    wireframeShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    wireframeShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    editorShader.UseShader();
    editorShader.GLBindMatrix4fv("viewMatrix", 1, editorCamera.viewMatrix.ptr());
    editorShader.GLBindMatrix4fv("projMatrix", 1, editorCamera.perspectiveMatrix.ptr());
    editorShader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));
    editorShader.GLBind3f("tint", 1.f, 1.f, 1.f);

    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        if(selectedEditObjId == editorEntities[i].GetId())
        {
            wireframeShader.UseShader();
            LOCAL_PERSIST float _breathef = 0.f;
            LOCAL_PERSIST vec4 selectColor;
            _breathef += gDeltaTime;
            selectColor.x = GM_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.y = GM_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.z = GM_abs(sinf(_breathef))*0.7f + 0.3f;
            selectColor.w = GM_abs(sinf(_breathef))*0.7f + 0.3f;
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

    auto selectedObj = GetEditorObjectWithId(selectedEditObjId);
    if(editorMode == EDITOR_MODE_EDIT 
        && selectedObj
        && selectedObj->GetGameClassCode() == BRUSH_VOLUME)
    {
        VolumeBrush* geom = (VolumeBrush*) selectedObj->GetClass();

        billboardShader.UseShader();
        for(auto& v : geom->vertices)
        {
            billboardShader.GLBind3f("quadColor", _RGB(0.f,255.f,0.f));
            for(auto vptr : selectedClickableVertices)
            {
                if(vptr == &v)
                {
                    billboardShader.GLBind3f("quadColor", _RGB(255.f,0.f,0.f));
                    break;
                }
            }
            vertexClickableBillboard.position = (selectedObj->GetModelMatrix() * vec4(v.pos, 1.f)).xyz;
            float scaledDistToObjFromCam = Magnitude(vertexClickableBillboard.position - editorCamera.position) * 0.13f;
            vertexClickableBillboard.scale = vec3(scaledDistToObjFromCam,scaledDistToObjFromCam,scaledDistToObjFromCam);
            vertexClickableBillboard.RotateAndRender(billboardShader);
        }
    }

    editorShader.UseShader();
    glDisable(GL_DEPTH_TEST);
    transGizmo.RenderGizmo(editorShader, false);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(0);
}

INTERNAL void EditorMousePickRender()
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

    auto selectedObj = GetEditorObjectWithId(selectedEditObjId);
    if(editorMode == EDITOR_MODE_EDIT 
        && selectedObj
        && selectedObj->GetGameClassCode() == BRUSH_VOLUME)
    {
        VolumeBrush* geom = (VolumeBrush*) selectedObj->GetClass();
        for(int i = 0; i < geom->vertices.size(); ++i)
        {
            auto v = geom->vertices[i];
            auto vid = reservedClickableVertexIds[i];
            vec3 rgbId = EntityIdToRGB(vid);
            mousepickShader.GLBind3f("entityIdColor", rgbId.x, rgbId.y, rgbId.z);
            vertexClickableBillboard.position = (selectedObj->GetModelMatrix() * vec4(v.pos, 1.f)).xyz;
            float scaledDistToObjFromCam = Magnitude(vertexClickableBillboard.position - editorCamera.position) * 0.13f;
            vertexClickableBillboard.scale = vec3(scaledDistToObjFromCam,scaledDistToObjFromCam,scaledDistToObjFromCam);
            vertexClickableBillboard.RotateAndRender(mousepickShader);
        }
    }

    glDisable(GL_DEPTH_TEST);
    transGizmo.RenderGizmo(mousepickShader, true);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(0);
}

INTERNAL void EditorMousePick(i32 screenPixelX, i32 screenPixelY)
{
    EditorMousePickRender();

    mousepickShader.SuppressWarningsPop();
    glFlush();
    glFinish(); 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    u8 pickedRGBData[4];
    glReadPixels(screenPixelX, screenPixelY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pickedRGBData);

    EntityId pickedId = RGBToEntityId(pickedRGBData[0], pickedRGBData[1], pickedRGBData[2]);
    if(pickedId != 0x00FFFFFF)
    {
        auto selectedObj = GetEditorObjectWithId(selectedEditObjId);
        if(editorMode == EDITOR_MODE_EDIT 
            && selectedObj
            && selectedObj->GetGameClassCode() == BRUSH_VOLUME
            && pickedId >= reservedClickableVertexIds[0]
            && pickedId <= reservedClickableVertexIds[NumberOfReservedClickableVertexIds-1])
        {
            selectedClickableFaces.clear();
            if(!gCurrentKeyState[SDL_SCANCODE_LCTRL])
            {
                selectedClickableVertices.clear();
            }
            auto selectedLevelGeom = (VolumeBrush*) selectedObj->GetClass();
            int levelGeomVerticesLocalIndex = pickedId - reservedClickableVertexIds[0];
            for(auto selectedVertexPtr : selectedClickableVertices)
            {
                if(selectedVertexPtr == &selectedLevelGeom->vertices[levelGeomVerticesLocalIndex])
                {
                    return;
                }
            }
            selectedClickableVertices.push_back(&selectedLevelGeom->vertices[levelGeomVerticesLocalIndex]);
            return;
        }

        if(editorMode == EDITOR_MODE_EDIT
            && selectedObj
            && selectedObj->GetGameClassCode() == BRUSH_VOLUME
            && pickedId >= reservedClickableFaceIds[0]
            && pickedId <= reservedClickableFaceIds[NumberOfReservedClickableFaceIds-1])
        {
            selectedClickableVertices.clear();
            if(!gCurrentKeyState[SDL_SCANCODE_LCTRL])
            {
                selectedClickableFaces.clear();
            }
            auto selectedLevelGeom = (VolumeBrush*) selectedObj->GetClass();
            int levelGeomFacesLocalIndex = pickedId - reservedClickableFaceIds[0];
            for(auto fptr : selectedClickableFaces)
            {
                if(fptr == &selectedLevelGeom->faces[levelGeomFacesLocalIndex])
                {
                    return;
                }
            }
            selectedClickableFaces.push_back(&selectedLevelGeom->faces[levelGeomFacesLocalIndex]);
            return;
        }

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

        if(editorMode == EDITOR_MODE_SELECT || editorMode == EDITOR_MODE_EDIT)
        {
            SetSelectedEditObjId(pickedId);
        }
    }
    else
    {
        SetSelectedEditObjId(0);
    }
}

INTERNAL void EditorExportMap(const std::string& mapName)
{
    if(!editorModeActiveFlag)
    {
        printf("WARNING: Editor mode is not active...\n");
        return;
    }

    if(mapName.empty())
    {
        printf("WARNING: Gave an empty map path to export to...");
        return;
    }

    ByteBuffer exportBuffer = ByteBufferNew();
    for(u32 i = 0; i < editorEntities.Count(); ++i)
    {
        u32 code = editorEntities[i].GetGameClassCode();
        ByteBufferWrite(&exportBuffer, u32, code);
        ByteBufferWrite(&exportBuffer, vec3, editorEntities[i].GetPos());
        ByteBufferWrite(&exportBuffer, quat, editorEntities[i].GetOrientation());
        ByteBufferWrite(&exportBuffer, vec3, editorEntities[i].GetScale());
        editorEntities[i].GetClass()->Serialize(&exportBuffer);
    }
    std::string mapPath = mapName.at(1) == ':' ? mapName : GAME_MAPS_DIRECTORY + mapName;
    ByteBufferWriteToFile(&exportBuffer, mapPath.c_str());
    ByteBufferFree(&exportBuffer);
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
        u32 code = CLASS_INVALID;
        ByteBufferRead(&importBuffer, u32, &code);

        GameObject* newEntity = &editorEntities.PushBackDefault();
        vec3 pos;
        quat rot;
        vec3 sca;
        ByteBufferRead(&importBuffer, vec3, &pos);
        ByteBufferRead(&importBuffer, quat, &rot);
        ByteBufferRead(&importBuffer, vec3, &sca);
        newEntity->SetPos(pos);
        newEntity->SetOrientation(rot);
        newEntity->SetScale(sca);

        GameClass* gcptr = NULL;
        switch (code)
        {
            case BRUSH_VOLUME: {
                auto vbptr = new VolumeBrush();
                vbptr->MakeVolumeMeshOnGPU();
                gcptr = vbptr;
            }break;
            case CLASS_PROPSTATIC:{
                gcptr = new PropStatic();
            }break;
            default:{
                editorEntities.PopBack();
                continue;
            }break;
        }
        gcptr->Deserialize(&importBuffer);
        newEntity->SetClass(gcptr);

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
    if(editorModeActiveFlag)
    {
        printf("WARNING: Attempting to load map when game is not active...\n");
        return;
    }

    ByteBuffer importBuffer;
    ByteBufferReadFromFile(&importBuffer, (GAME_MAPS_DIRECTORY + mapName).c_str());
    while(importBuffer.position < importBuffer.size)
    {
        u32 code = CLASS_INVALID;
        ByteBufferRead(&importBuffer, u32, &code);

        switch (code)
        {
            case BRUSH_VOLUME: {
                /* if brush volume, the class we create depends on the class of the volume
                   by default we make a level geom cuz thatll be the most common use of volume */

                /* when making levelgeom, then create volumebrush first then get buffer object to make level geom 
                   then delete the volumebrush */
                GameObject _tempGameObjectForGenerateVBO = GameObject();
                vec3 pos;
                quat rot;
                vec3 sca;
                ByteBufferRead(&importBuffer, vec3, &pos);
                ByteBufferRead(&importBuffer, quat, &rot);
                ByteBufferRead(&importBuffer, vec3, &sca);
                _tempGameObjectForGenerateVBO.SetPos(pos);
                _tempGameObjectForGenerateVBO.SetOrientation(rot);
                _tempGameObjectForGenerateVBO.SetScale(sca);
                auto _tempVolumeBrushForGenerateVBO = VolumeBrush();
                _tempVolumeBrushForGenerateVBO.Deserialize(&importBuffer);
                _tempGameObjectForGenerateVBO.SetClass(&_tempVolumeBrushForGenerateVBO);
                std::vector<float> _tempVolumeMeshVBO = _tempVolumeBrushForGenerateVBO.GenerateBufferObject();

                GameObject* addGameObj = &staticMapObjects.PushBackDefault();
                auto createdLevelGeomPtr = new LevelGeometry();
                createdLevelGeomPtr->RebindBufferObject((u32)(4*_tempVolumeMeshVBO.size()), _tempVolumeMeshVBO.data());
                for(int i = 0; i < _tempVolumeBrushForGenerateVBO.faces.size(); ++i)
                {
                    auto f = _tempVolumeBrushForGenerateVBO.faces[i];
                    createdLevelGeomPtr->faces[i].texture = f.texture;
                    createdLevelGeomPtr->faces[i].numberOfIndices = f.NumberOfIndices();
                    createdLevelGeomPtr->facesCount++;
                }
                addGameObj->SetClass(createdLevelGeomPtr);

                // TODO(Kevin): check if has collision first
                MeshCollider meshCollider;
                meshCollider.owningGameObject = addGameObj;
                meshCollider.verticesModelSpace.resize(_tempVolumeBrushForGenerateVBO.vertices.size());
                for(int i = 0; i < _tempVolumeBrushForGenerateVBO.vertices.size(); ++i)
                {
                    auto& v = _tempVolumeBrushForGenerateVBO.vertices[i];
                    v.pos = (_tempGameObjectForGenerateVBO.GetModelMatrix() * vec4(v.pos, 1.f)).xyz;
                    meshCollider.verticesModelSpace[i] = v.pos;
                }
                staticMapColliders.PushBack(meshCollider);
            }break;
            default: {
                continue;
            }
        }
    }
    ByteBufferFree(&importBuffer);
}

void VolumeBrush::EditorModeRender(const Shader& shader, bool mousePicking)
{
    std::vector<float> vbo = GenerateBufferObject();
    glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 * vbo.size(), vbo.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    shader.GLBind1i("dontShade", 0);
    /** Note(Kevin): VolumeBrush doesn't uses identity model matrix because transformations
        applied when we call GenerateBufferObject. It's already encoded into the vertex position attribute. */
    shader.GLBindMatrix4fv("modelMatrix", 1, mat4().ptr());

    if(!isVisible && !mousePicking)
    {
        shader.GLBind1i("dontShade", 1);
        hasCollision ? shader.GLBind3f("modelColor", _RGB(0.f, 255.f, 0.f)) : shader.GLBind3f("modelColor", _RGB(255.f, 255.f, 255.f));
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }

    glBindVertexArray(idVAO);
    glBindBuffer(GL_ARRAY_BUFFER, idVBO);
    i32 indexStart = 0;
    for(int i = 0; i < faces.size(); ++i)
    {
        Face& face = faces[i];
        
        if(editorMode == EDITOR_MODE_EDIT)
        {
            if(GetTiedEntity()->GetId() == selectedEditObjId)
            {
                if(mousePicking)
                {
                    vec3 rgbId = EntityIdToRGB(reservedClickableFaceIds[i]);
                    shader.GLBind3f("entityIdColor", rgbId.x, rgbId.y, rgbId.z);
                }
                else
                {
                    for(int i = 0; i < selectedClickableFaces.size(); ++i)
                    {
                        if(selectedClickableFaces[i] == &face)
                        {
                            shader.GLBind3f("tint", 1.f, 0.9f, 0.7f);
                        } 
                    }
                }
            }
        }

        face.texture.UseTexture();
        glDrawArrays(GL_TRIANGLES, indexStart, face.NumberOfIndices());
        indexStart += face.NumberOfIndices();

        shader.GLBind3f("tint", 1.f, 1.f, 1.f);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    if(!isVisible && !mousePicking)
    {
        shader.GLBind3f("modelColor", _RGB(0.f, 0.f, 0.f));
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
}

