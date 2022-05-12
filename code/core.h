struct GameObject;
struct GameClass;




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

/** Loads texture from bitmap; generates a new texture object in GPU mem; store the id
    of the new texture object into texture.texture_id; sets texture parameters; copies texture data
    into the texture object in GPU mem; and generates mip maps automatically. 
    filteringMode
    GL_NEAREST : nearest neighbor
    GL_LINEAR : bilinear filtering
    GL_LINEAR_MIPMAP_LINEAR : trilinear filtering */
INTERNAL void TextureCreateFromBitmap(Texture&        texture,
                                      unsigned char*  bitmap,
                                      u32             bitmapWidth,
                                      u32             bitmapHeight,
                                      GLenum          targetFormat,
                                      GLenum          sourceFormat,
                                      GLenum          filteringMode = GL_NEAREST);

/** Loads texture at file_path; generates a new texture object in GPU mem; stores the id
    of the new texture object into texture.texture_id; sets texture parameters; copies
    texture data into the texture object in GPU mem; and generates mip maps automatically.
    Returns false if something failed. */
INTERNAL bool TextureCreateFromFile(Texture& texture, const char* textureFilePath, GLenum filteringMode = GL_NEAREST);

/** Deletes texture object from GPU memory; resets texture_id, width, height, bit_depth to 0. */
INTERNAL void TextureDelete(Texture& texture); // TODO(Kevin): rename to "delete from GPU" or something





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

/** Create a Mesh with the given vertices and indices.
vertex_attrib_size: vertex coords size (e.g. 3 if x y z)
texture_attrib_size: texture coords size (e.g. 2 if u v)
draw_usage: affects optimization; GL_STATIC_DRAW buffer data
only set once, GL_DYNAMIC_DRAW if buffer modified repeatedly */
INTERNAL Mesh MeshCreate(Mesh& mesh,
                         float* vertices,
                         u32* indices,
                         u32 verticesArrayCount,
                         u32 indicesArrayCount,
                         u8 positionAttribSize = 3,  // x y z
                         u8 textureAttribSize = 2,   // u v
                         u8 normalAttribSize = 3,    // x y z
                         GLenum drawUsage = GL_STATIC_DRAW);

/** Clearing GPU memory: glDeleteBuffers and glDeleteVertexArrays deletes the buffer
    object and vertex array object off the GPU memory. */
INTERNAL void MeshDelete(Mesh& mesh);





struct MeshGroup
{
    std::vector<Mesh>     meshes;
    std::vector<Texture>  textures;
    std::vector<u16>      meshToTextureTable; // stores index of textures element corresponding to mesh
    std::string           filePath = "";

    void Render();
};

/** Load model from disk through Assimp */
INTERNAL MeshGroup* MeshGroupAssimpLoad(const char* fileName);





struct CollisionEventArgs
{
    vec3 normal;
    float depth;
    GameObject* other;
};

enum GAME_CLASS_CODE : u32
{
    CLASS_INVALID,

    BRUSH_VOLUME, // brushes are editor only classes

    CLASS_LEVELGEOM,
    CLASS_PROPSTATIC,
    CLASS_PROPDYNAMIC
};

INTERNAL EntityId GetUniqueEntityId()
{
    // 0 is invalid
    LOCAL_PERSIST EntityId id = 1;
    return id++;
}

struct GameObject
{
    GameObject()
        : position({0.f, 0.f, 0.f })
        , orientation(quat())
        , scale({ 1.f, 1.f, 1.f })
        , modelMatrix(mat4())
        , isModelMatrixOutdated(false)
        , entityId(GetUniqueEntityId())
    {}

    virtual void    Update();
    virtual void    Render(const Shader& shader);
    virtual void    OnCollision(CollisionEventArgs collisionInfo);

    EntityId        GetId() const { return entityId; }
    GAME_CLASS_CODE GetGameClassCode() const;
    GameClass*      GetClass() const;
    void            SetClass(GameClass* childClass);

    const mat4& GetModelMatrix();
    vec3 GetPos() const;
    void SetPos(const vec3& newPos);
    void Move(const vec3& moveAmount);
    quat GetOrientation() const;
    void SetOrientation(const quat& newOrientation);
    void Rotate(const vec3& eulerInDegrees);
    void Rotate(const quat& rotationAsQuat);
    vec3 GetScale() const;
    void SetScale(const vec3& newScale);

protected:
    vec3 position;
    quat orientation;
    vec3 scale;

private:
    EntityId entityId;
    GameClass* tiedClass;

    mat4 modelMatrix;
    bool32 isModelMatrixOutdated;
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

    u32 idVAO = 0;
    u32 idVBO = 0;

    struct {
        Texture texture;
        i32 numberOfIndices;
    } faces[8];
    u32 facesCount = 0;

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

    void RebindBufferObject(u32 byteSize, float* data)
    {
        glBindVertexArray(idVAO);
            glBindBuffer(GL_ARRAY_BUFFER, idVBO);
                glBufferData(GL_ARRAY_BUFFER, byteSize, data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Render(const Shader& shader)
    {
        //shader.GLBind1i("dontShade", 0);
        shader.GLBindMatrix4fv("modelMatrix", 1, GetTiedEntity()->GetModelMatrix().ptr());

        glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
        i32 indexStart = 0;
        for(u32 i = 0; i < facesCount; ++i)
        {
            faces[i].texture.UseTexture();
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
    float farClip               = 64000.f;
    float turnSpeed             = 0.04f;
    bool  bMouseControl         = false;

    virtual void CameraUpdate();
    void CameraCalculatePerspectiveMatrix();
    void CameraCalculatePerspectiveMatrix(float aspectRatio);
    void CameraCalculateViewMatrix();
};

struct EditorCamera : GameCamera
{
    float moveSpeed = 200.f;

    void CameraUpdate() override;
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
        float largestScale = GM_max(scale.x, GM_max(scale.y, scale.z));
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

struct Billboard
{
    GameCamera* target = NULL;
    vec3 position;
    vec3 scale;
    bool cylindricalFlag = false; // cylindrical only rotates around Y axis

    void InitQuadMesh(float width, float height, vec2 originOffset)
    {
        MeshCreate(quad, nullptr, nullptr, 0, 0, 3, 2, 0, GL_DYNAMIC_DRAW);
        Resize(width, height, originOffset);
    }

    void DeleteQuadMesh()
    {
        MeshDelete(quad);
    }

    void Resize(float width, float height, vec2 originOffset)
    {
        quadVertices[1] = -originOffset.y;
        quadVertices[2] = -originOffset.x;
        quadVertices[6] = -originOffset.y;
        quadVertices[7] = width - originOffset.x;
        quadVertices[11] = height - originOffset.y;
        quadVertices[12] = -originOffset.x;
        quadVertices[16] = height - originOffset.y;
        quadVertices[17] = width - originOffset.x;
        u32 quadIndices[6] = {0,1,3,0,3,2};
        quad.RebindBufferObjects(quadVertices, quadIndices, 20, 6);
    }

    void RotateAndRender(const Shader& billboardShader)
    {
        mat4 modelMatrix = TranslationMatrix(position);
        if(target)
        {
            vec3 toTarget = target->position - position;
            vec3 rot;
            rot.y = atanf(-toTarget.z / toTarget.x);
            if(!cylindricalFlag)
            {
                rot = QuatToEuler(DirectionToOrientation(toTarget));
                rot.x = 0.f;
            }
            modelMatrix *= RotationMatrix(EulerToQuat(rot));
        }
        modelMatrix *= ScaleMatrix(scale);
        billboardShader.GLBindMatrix4fv("modelMatrix", 1, modelMatrix.ptr());

        // TODO(Kevin): use billboard texture

        quad.RenderMesh();
    }

private:
    Mesh quad;

    float quadVertices[20] = {
        0.f, -1.f, -1.f, 0.f, 0.f,
        0.f, -1.f, 1.f, 1.f, 0.f,
        0.f, 1.f, -1.f, 0.f, 1.f,
        0.f, 1.f, 1.f, 1.f, 1.f
    };
};

