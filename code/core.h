struct GameObject;
struct GameClass;
struct ByteBuffer;
struct MeshGroup;
INTERNAL MeshGroup* MeshGroupAssimpLoad(const char* fileName);

struct Texture
{
    /** Handle for texture stored in GPU memory */

    GLuint      textureId   = 0;        // ID for the texture in GPU memory
    i32         width       = 0;        // Width of the texture
    i32         height      = 0;        // Height of the texture
    GLenum      format      = GL_NONE;  // format / bitdepth of texture (GL_RGB would be 3 byte bit depth)
    std::string filePath    = "";

    void UseTexture() const;      // Binds this texture to Texture Unit 1
};


struct Mesh
{
    /** Stores mesh { VAO, VBO, IBO } info. Handle for VAO on GPU memory
        Holds the ID for the VAO, VBO, IBO in the GPU memory */

    u32  idVAO          = 0;
    u32  idVBO          = 0;
    u32  idIBO          = 0;
    u32  indicesCount   = 0;

    /** Binds VAO and draws elements. Bind a shader program and texture
        before calling RenderMesh */
    void RenderMesh(GLenum renderMode = GL_TRIANGLES) const;

    /** Overwrite existing buffer data */
    void RebindBufferObjects(float* vertices,
                             u32* indices,
                             u32 verticesArrayCount,
                             u32 indicesArrayCount,
                             GLenum drawUsage = GL_DYNAMIC_DRAW);
};


struct MeshGroup
{
    std::vector<Mesh>     meshes;
    std::vector<Texture>  textures;
    std::vector<u16>      meshToTextureTable; // stores index of textures element corresponding to mesh
    std::string           filePath = "";

    void Render();
};

struct CollisionEventArgs
{
    vec3 normal;
    float depth;
    GameObject* other;
};

enum GAME_CLASS_CODE : u32
{
    CLASS_INVALID = 0x0000,

    OBJECT_VISIBLE = 0x0001,
    OBJECT_COLLIDABLE = 0x0002,

    BRUSH_VOLUME = 0x0010, // brushes are editor only classes

    CLASS_LEVELGEOM = 0x0020,
    CLASS_PROPSTATIC = 0x0040,
    CLASS_PROPDYNAMIC = 0x0080
};

INTERNAL EntityId GetUniqueEntityId()
{
    // 0 is invalid
    LOCAL_PERSIST EntityId id = 1;
    return id++;
}

struct GameObject final
{
    GameObject()
        : position({0.f, 0.f, 0.f })
        , orientation(quat())
        , scale({ 1.f, 1.f, 1.f })
        , modelMatrix(mat4())
        , isModelMatrixOutdated(false)
        , entityId(GetUniqueEntityId())
    {}

    virtual void Update();

    virtual void Render(const Shader& shader);

    virtual void OnCollision(CollisionEventArgs collisionInfo);

    GAME_CLASS_CODE GetGameClassCode() const;

    EntityId GetId() const { return entityId; }

    GameClass* GetClass() const;

    void SetClass(GameClass* childClass);


    const mat4& GetModelMatrix()
    {
        if(isModelMatrixOutdated)
        {
            modelMatrix = mat4();
            modelMatrix *= TranslationMatrix(position);
            modelMatrix *= RotationMatrix(orientation);
            modelMatrix *= ScaleMatrix(scale);
            isModelMatrixOutdated = false;
        }

        return modelMatrix;
    }
    vec3 GetPos() const
    {
        return position;
    }
    void SetPos(const vec3& newPos)
    {
        position = newPos;
        isModelMatrixOutdated = true;
    }
    void Move(const vec3& moveAmount)
    {
        position += moveAmount;
        isModelMatrixOutdated = true;
    }
    quat GetOrientation() const
    {
        return orientation;
    }
    void SetOrientation(const quat& newOrientation)
    {
        orientation = newOrientation;
        isModelMatrixOutdated = true;
    }
    void Rotate(const vec3& eulerInDegrees)
    {
        quat q = EulerToQuat(eulerInDegrees * SML_DEG2RAD);
        Rotate(q);
    }
    void Rotate(const quat& rotationAsQuat)
    {
        SetOrientation(rotationAsQuat * GetOrientation());
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

protected:
    vec3 position; // World space
    quat orientation;
    vec3 scale;

private:
    GameClass* tiedClass;
    mat4 modelMatrix;
    bool32 isModelMatrixOutdated;
    EntityId entityId;
};

struct GameClass
{
    GameClass()
    {
        /** Must specify correct gameClassCode in derived classes. */
        gameClassCode = CLASS_INVALID;
    }

    virtual ~GameClass() = default;

    /** Update logic. */
    virtual void Update()
    {
        // do nothing by default
    }
    /** Rendering if this class should Render anything. */
    virtual void Render(const Shader& shader)
    {
        // do nothing by default
    }
    /** Response to collision */
    virtual void OnCollision(CollisionEventArgs collisionInfo)
    {
        // do nothing by default
    }

    /** What to Render in Editor mode. */
    virtual void EditorModeRender(const Shader& shader, bool mousePicking)
    {
        // do nothing by default
    }
    /** Decompose */
    virtual void Serialize(ByteBuffer* writeBuffer)
    {
        printf("WARNING: Attemping to serialize a GameClass that doesn't support it.\n");
    }
    /** Reconstruct */
    virtual void Deserialize(ByteBuffer* readBuffer)
    {
        printf("WARNING: Attemping to deserialize to a GameClass that doesn't support it.\n");
    }

    GAME_CLASS_CODE GetGameClassCode() const { return gameClassCode; }

    GameObject* GetTiedEntity() const { return tiedEntity; }
    void SetTiedEntity(GameObject* parentEntity)
    {
        // TODO(Kevin): Eventually allow tied entities and tied classes to be switched
        tiedEntity = parentEntity;
        if(tiedEntity->GetClass() != this)
        {
            tiedEntity->SetClass(this);
        }
    }

public:
    bool isVisible = true;
    bool hasCollision = false;
protected:
    GAME_CLASS_CODE gameClassCode;
private:
    GameObject* tiedEntity = nullptr;
};

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

    void Serialize(ByteBuffer* writeBuffer) override
    {
        const std::string& propfilepath = GetModel()->filePath;
        ByteBufferWrite(writeBuffer, u64, (u64)propfilepath.size()+1);
        ByteBufferWriteBulk(writeBuffer, (void*)propfilepath.c_str(), propfilepath.size()+1); 
    }

    void Deserialize(ByteBuffer* readBuffer) override
    {
        u64 propfilepathSize;
        char tempPropPathBuffer[128];
        ByteBufferRead(readBuffer, u64, &propfilepathSize);
        ByteBufferReadBulk(readBuffer, tempPropPathBuffer, propfilepathSize);
        auto loadedPropModel = MeshGroupAssimpLoad(tempPropPathBuffer);
        if(loadedPropModel)
        {
            SetModel(loadedPropModel);
        }
    }

private:
    MeshGroup* model;
};

struct LevelGeometry : GameClass  // we could have subclasses of level geometry which do respond to collision or update
{
    /* Stripped down version of VolumeBrush that can be used in game */

    u32 idVAO;
    u32 idVBO;

    struct {
        u32 idTexture;
        i32 numberOfIndices;
    } faces[8];
    u32 facesCount;

    LevelGeometry()
    {
        // TODO(Kevin) set gameClassCode to whatever level geometry is

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

struct GameCamera
{
    vec3  position;             // camera x y z position in world space
    vec3  rotation;             // roll yaw pitch - in that order
    vec3  calculatedDirection;  // Intuitive direction - direction forward
    vec3  calculatedUp;
    vec3  calculatedRight;
    mat4  perspectiveMatrix;    // Perspective projection matrix
    mat4  viewMatrix;           // Last calculated view matrix
    float nearClip              = 0.1f;
    float farClip               = 2000.f;
    float moveSpeed             = 6.f;
    float turnSpeed             = 0.04f;
    bool  bMouseControl          = false;

    void CameraUpdate();
    void CameraCalculatePerspectiveMatrix();
    void CameraCalculatePerspectiveMatrix(float aspectRatio);
    void CameraCalculateViewMatrix();
};




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
enum ColliderTypes
{
    INVALID_TYPE,
    MESH_TYPE,
    SPHERE_TYPE,
    LINE_TYPE
};

/** A collider contains two basic pieces of information - a Shape and a Transform */
struct Collider
{
    /* A preparation function which transforms the collider to World Space from Model Space */
    virtual void TransformCollider() = 0;

    /* Returns the furthest point on the collider along a given direction */
    virtual vec3 FindFurthestPoint(vec3 direction) const = 0;

    GameObject* owningGameObject = nullptr;
    ColliderTypes colliderType = INVALID_TYPE;
};

struct MeshCollider : Collider
{
    std::vector<vec3> verticesModelSpace; // shouldn't change after first initialization

    MeshCollider()
    {
        colliderType = MESH_TYPE;
    }

    void TransformCollider() override
    {
        mat4 modelMatrix = owningGameObject->GetModelMatrix();
        verticesWorldSpace.resize(verticesModelSpace.size());
        // NOTE(Kevin): can probably multi-thread vertex position calculation
        for(int i = 0; i < verticesModelSpace.size(); ++i)
        {
            auto vertexMCS = verticesModelSpace[i];
            verticesWorldSpace[i] = (modelMatrix * vec4(vertexMCS, 1.0f)).xyz;
        }
    }

    vec3 FindFurthestPoint(vec3 direction) const override
    {
        vec3 maxVertexSoFar;
        float maxDistanceSoFar = -FLT_MAX;
        for(auto vertexWCS : verticesWorldSpace)
        {
            float distance = Dot(vertexWCS, direction);
            if(distance > maxDistanceSoFar)
            {
                maxVertexSoFar = vertexWCS;
                maxDistanceSoFar = distance;
            }
        }
        return maxVertexSoFar;
    }

private:
    std::vector<vec3> verticesWorldSpace; // changes based on transform of owning game object
};

struct SphereCollider : Collider
{
    float radius; // radius in model frame. before being affected by scale.

    SphereCollider()
    {
        colliderType = SPHERE_TYPE;
    }

    void TransformCollider() override
    {
        vec3 scale = owningGameObject->GetScale();
        float largestScale = SML_max(scale.x, SML_max(scale.y, scale.z));
        position = owningGameObject->GetPos();
        scaledRadius = largestScale * radius;
    }

    vec3 FindFurthestPoint(vec3 direction) const override
    {
        vec3 offset = Normalize(direction) * scaledRadius;
        return position + offset;
    }

    vec3 GetPosition() const
    {
        return position;
    }

    float GetScaledRadius() const
    {
        return scaledRadius;
    }

private:
    vec3 position; // position in world frame. should be position of owningGameObject
    float scaledRadius; // radius actually used for FindFurthestPoint
};

struct LineCollider : Collider
{
    vec3 a;
    vec3 b;

    LineCollider()
    {
        colliderType = LINE_TYPE;
    }

    void TransformCollider() override
    {

    }

    vec3 FindFurthestPoint(vec3 direction) const override
    {
        float distanceA = Dot(a, direction);
        float distanceB = Dot(b, direction);
        return (distanceA > distanceB ? a : b);
    }
};
