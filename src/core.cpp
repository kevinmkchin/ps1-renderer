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

void Texture::UseTexture() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
}

INTERNAL std::unordered_map<std::string, Texture> gpuLoadedTextures;

/** Deletes texture object from GPU memory; resets texture_id, width, height, bit_depth to 0. */
INTERNAL void TextureDelete(Texture& texture)
{
    if(texture.textureId == 0)
    {
        return;
    }
    glDeleteTextures(1, &texture.textureId);

    auto gpuTextureIter = gpuLoadedTextures.find(texture.filePath);
    if(gpuTextureIter != gpuLoadedTextures.end())
    {
        gpuLoadedTextures.erase(gpuTextureIter);
    }

    texture.textureId = 0;
    texture.width = 0;
    texture.height = 0;
    texture.format = GL_NONE;
}

/** Loads texture from bitmap; generates a new texture object in GPU mem; store the id
    of the new texture object into texture.texture_id; sets texture parameters; copies texture data
    into the texture object in GPU mem; and generates mip maps automatically. */
INTERNAL void TextureCreateFromBitmap(Texture&        texture,
                                      unsigned char*  bitmap,
                                      u32             bitmapWidth,
                                      u32             bitmapHeight,
                                      GLenum          targetFormat,
                                      GLenum          sourceFormat)
{
    if(texture.textureId != 0)
    {
        printf("WARNING: Trying to load a Texture when there is already a texture loaded! Clearing texture first...\n");
        TextureDelete(texture);
    }

    texture.width = bitmapWidth;
    texture.height = bitmapHeight;
    texture.format = sourceFormat;

    glGenTextures(1, &texture.textureId);
    glBindTexture(GL_TEXTURE_2D, texture.textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
            GL_TEXTURE_2D,              // texture target type
            0,                          // level-of-detail number n = n-th mipmap reduction image
            targetFormat,               // format of data to store (target): num of color components
            bitmapWidth,                // texture width
            bitmapHeight,               // texture height
            0,                          // must be 0 (legacy)
            sourceFormat,               // format of data being loaded (source)
            GL_UNSIGNED_BYTE,           // data type of the texture data
            bitmap);                    // data
    glGenerateMipmap(GL_TEXTURE_2D);    // generate mip maps automatically
    glBindTexture(GL_TEXTURE_2D, 0);
}

/** Loads texture at file_path; generates a new texture object in GPU mem; stores the id
    of the new texture object into texture.texture_id; sets texture parameters; copies
    texture data into the texture object in GPU mem; and generates mip maps automatically. */
INTERNAL void TextureCreateFromFile(Texture& texture, const char* textureFilePath)
{
    auto textureAlreadyLoaded = gpuLoadedTextures.find(std::string(textureFilePath));
    if(textureAlreadyLoaded != gpuLoadedTextures.end())
    {
        texture = textureAlreadyLoaded->second;
        return;
    }

    BitmapHandle textureHandle;
    ReadImage(textureHandle, textureFilePath);
    TextureCreateFromBitmap(texture, (unsigned char *) textureHandle.memory, textureHandle.width,
                            textureHandle.height, GL_RGBA, (textureHandle.bitDepth == 3 ? GL_RGB : GL_RGBA));
    FreeImage(textureHandle); // texture data has been copied to GPU memory, so we can free image from memory

    texture.filePath = textureFilePath;
    gpuLoadedTextures[textureFilePath] = texture;
}




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

void Mesh::RenderMesh(GLenum renderMode) const
{
    if (indicesCount == 0) // Early out if index_count == 0, nothing to draw
    {
        printf("WARNING: Attempting to render a mesh with 0 index count!\n");
        return;
    }

    // Bind VAO, bind VBO, draw elements(indexed draw)
    glBindVertexArray(idVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
            glDrawElements(renderMode, indicesCount, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::RebindBufferObjects(float* vertices,
                               u32* indices,
                               u32 verticesArrayCount,
                               u32 indicesArrayCount,
                               GLenum drawUsage)
{
    if(idVBO == 0 || idIBO == 0)
    {
        return;
    }

    indicesCount = indicesArrayCount;
    glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 * verticesArrayCount, vertices, drawUsage);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * indicesArrayCount, indices, drawUsage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/** Create a Mesh with the given vertices and indices.
vertex_attrib_size: vertex coords size (e.g. 3 if x y z)
texture_attrib_size: texture coords size (e.g. 2 if u v)
draw_usage: affects optimization; GL_STATIC_DRAW buffer data
only set once, GL_DYNAMIC_DRAW if buffer modified repeatedly */
INTERNAL void MeshCreate(Mesh& mesh,
                         float* vertices,
                         u32* indices,
                         u32 verticesArrayCount,
                         u32 indicesArrayCount,
                         u8 positionAttribSize = 3,  // x y z
                         u8 textureAttribSize = 2,   // u v
                         u8 normalAttribSize = 3,    // x y z
                         GLenum drawUsage = GL_STATIC_DRAW)
{
    u8 stride = 0;
    if(textureAttribSize)
    {
        stride += positionAttribSize + textureAttribSize;
        if(normalAttribSize)
        {
            stride += normalAttribSize;
        }
    }

    mesh.indicesCount = indicesArrayCount;

    glGenVertexArrays(1, &mesh.idVAO);
    glBindVertexArray(mesh.idVAO);
        glGenBuffers(1, &mesh.idVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.idVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 /*bytes cuz float*/ * verticesArrayCount, vertices, drawUsage);
            glVertexAttribPointer(0, positionAttribSize, GL_FLOAT, GL_FALSE,
                                  sizeof(float) * stride,
                                  nullptr);
            glEnableVertexAttribArray(0);
            if(textureAttribSize > 0)
            {
                glVertexAttribPointer(1, textureAttribSize, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * stride,
                                      (void*)(sizeof(float) * positionAttribSize));
                glEnableVertexAttribArray(1);
                if(normalAttribSize > 0)
                {
                    glVertexAttribPointer(2, normalAttribSize, GL_FLOAT, GL_FALSE,
                                          sizeof(float) * stride,
                                          (void*)(sizeof(float) * (positionAttribSize + textureAttribSize)));
                    glEnableVertexAttribArray(2);
                }
            }

        glGenBuffers(1, &mesh.idIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 /*bytes cuz uint32*/ * indicesArrayCount, indices, drawUsage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // Unbind the VAO;
}

/** Clearing GPU memory: glDeleteBuffers and glDeleteVertexArrays deletes the buffer
    object and vertex array object off the GPU memory. */
INTERNAL void MeshDelete(Mesh& mesh)
{
    if (mesh.idIBO != 0)
    {
        glDeleteBuffers(1, &mesh.idIBO);
        mesh.idIBO = 0;
    }
    if (mesh.idVBO != 0)
    {
        glDeleteBuffers(1, &mesh.idVBO);
        mesh.idVBO = 0;
    }
    if (mesh.idVAO != 0)
    {
        glDeleteVertexArrays(1, &mesh.idVAO);
        mesh.idVAO = 0;
    }

    mesh.indicesCount = 0;
}





struct MeshGroup
{
    std::vector<Mesh>     meshes;
    std::vector<Texture>  textures;
    std::vector<u16>      meshToTextureTable; // stores index of textures element corresponding to mesh
    std::string           filePath = "";

    void render();
};

void MeshGroup::render()
{
    LOCAL_PERSIST Texture missingTexture;
    if(missingTexture.textureId == 0)
    {
        TextureCreateFromFile(missingTexture, "data/primitives/missing_texture.png");
    }

    for(size_t i = 0; i < meshes.size(); ++i)
    {
        u16 materialIndex = meshToTextureTable[i];
        if(materialIndex < textures.size() && textures[materialIndex].textureId != 0)
        {
            textures[materialIndex].UseTexture();
        }
        else if(textures[materialIndex].textureId == 0)
        {
            missingTexture.UseTexture();
        }

        meshes[i].RenderMesh();
    }
}

INTERNAL Mesh __assimpLoadMeshHelper(aiMesh* meshNode)
{
    const u8 vb_entries_per_vertex = 8;
    std::vector<float> vb(meshNode->mNumVertices * vb_entries_per_vertex);
    std::vector<u32> ib(meshNode->mNumFaces * meshNode->mFaces[0].mNumIndices);
    if(meshNode->mTextureCoords[0])
    {
        for(size_t i = 0; i < meshNode->mNumVertices; ++i)
        {
            // mNormals and mVertices are both mNumVertices in size
            size_t v_start_index = i * vb_entries_per_vertex;
            vb[v_start_index] = meshNode->mVertices[i].x;
            vb[v_start_index + 1] = meshNode->mVertices[i].y;
            vb[v_start_index + 2] = meshNode->mVertices[i].z;
            vb[v_start_index + 3] = meshNode->mTextureCoords[0][i].x;
            vb[v_start_index + 4] = meshNode->mTextureCoords[0][i].y;
            vb[v_start_index + 5] = meshNode->mNormals[i].x;
            vb[v_start_index + 6] = meshNode->mNormals[i].y;
            vb[v_start_index + 7] = meshNode->mNormals[i].z;
        }
    }
    else
    {
        for(size_t i = 0; i < meshNode->mNumVertices; ++i)
        {
            size_t v_start_index = i * vb_entries_per_vertex;
            vb[v_start_index] = meshNode->mVertices[i].x;
            vb[v_start_index + 1] = meshNode->mVertices[i].y;
            vb[v_start_index + 2] = meshNode->mVertices[i].z;
            vb[v_start_index + 3] = 0.f;
            vb[v_start_index + 4] = 0.f;
            vb[v_start_index + 5] = meshNode->mNormals[i].x;
            vb[v_start_index + 6] = meshNode->mNormals[i].y;
            vb[v_start_index + 7] = meshNode->mNormals[i].z;
        }
    }

    for(size_t i = 0; i < meshNode->mNumFaces; ++i)
    {
        aiFace face = meshNode->mFaces[i];
        for(size_t j = 0; j < face.mNumIndices; ++j)
        {
            ib[i * face.mNumIndices + j] = face.mIndices[j]; // prob sometimes not correct to index ib this way
        }
    }

    Mesh mesh;
    MeshCreate(mesh, &vb[0], &ib[0], (u32) vb.size(), (u32) ib.size());
    return mesh;
}

INTERNAL std::unordered_map<std::string, MeshGroup> gpuLoadedModels;

/** Load model from disk through Assimp
 * TODO(Kevin): probably add option to create a basic MeshCollider from the obj vertices info
 * */
INTERNAL MeshGroup* MeshGroupAssimpLoad(const char* fileName)
{
    auto modelAlreadyLoaded = gpuLoadedModels.find(fileName);
    if(modelAlreadyLoaded != gpuLoadedModels.end())
    {
        return &modelAlreadyLoaded->second;
    }

    Assimp::Importer importer;
    /*  NOTE: To create smooth normals respecting edges sharper than a given angle,
        use importer.SetPropertyFloat("PP_GSN_MAX_SMOOTHING_ANGLE", 90) along with
        aiProcess_GenSmoothNormals flag. https://github.com/assimp/assimp/issues/1713

        aiProcess_GenSmoothNormals
        This flag may not be specified together with #aiProcess_GenNormals. There's
        a importer property, #AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE which allows you to
        specify an angle maximum for the normal smoothing algorithm. Normals exceeding
        this limit are not smoothed, resulting in a 'hard' seam between two faces.
        Using a decent angle here (e.g. 80 degrees) results in very good visual
        appearance. To create smooth normals respecting edges sharper than a given angle,
        use importer.SetPropertyFloat("PP_GSN_MAX_SMOOTHING_ANGLE", 90) along with
        aiProcess_GenSmoothNormals flag. https://github.com/assimp/assimp/issues/1713

        aiProcess_JoinIdenticalVertices
        is compulsory for indexed drawing. This still works with flat shaded geometry
        because it only joins vertices that are identical in both position and normal.
        e.g. a flat shaded cube will have 24 vertices after joining because each side
        of the cube will have 4 unique vertices and the vertices at the corners will
        not be shared by multiple faces of the cube because they will have different
        normals even though their positions are the same.
    */
    const aiScene* scene = importer.ReadFile(fileName,
                                             aiProcess_Triangulate
                                             |aiProcess_GenNormals
                                             |aiProcess_JoinIdenticalVertices
    );
    if(!scene)
    {
        printf("Model '%s' failed to load: %s\n", fileName, importer.GetErrorString());
        return nullptr;
    }

    MeshGroup meshGroupToLoadTo;

    meshGroupToLoadTo.meshes = std::vector<Mesh>(scene->mNumMeshes);
    meshGroupToLoadTo.textures = std::vector<Texture>(scene->mNumMaterials);
    meshGroupToLoadTo.meshToTextureTable = std::vector<u16>(scene->mNumMeshes);

    // Unpack meshes
    for(size_t i = 0; i < scene->mNumMeshes; ++i)
    {
        aiMesh* mesh_node = scene->mMeshes[i];
        meshGroupToLoadTo.meshes[i] = __assimpLoadMeshHelper(mesh_node);
        meshGroupToLoadTo.meshToTextureTable[i] = mesh_node->mMaterialIndex;
    }

    // Load diffuse textures
    for(size_t i = 0; i < scene->mNumMaterials; ++i)
    {
        aiMaterial* mat = scene->mMaterials[i];
        if(mat->GetTextureCount(aiTextureType_DIFFUSE))
        {
            aiString path;
            if(mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
            {
                int idx = (int)std::string(path.data).find_last_of("\\");
                std::string texture_file_name = std::string(path.data).substr(idx+1);
                std::string model_file_directory = std::string(fileName);
                idx = SML_max((int)model_file_directory.find_last_of("/"), (int)model_file_directory.find_last_of("\\"));
                model_file_directory = model_file_directory.substr(0, idx + 1);

                std::string tex_path = model_file_directory + texture_file_name;
                TextureCreateFromFile(meshGroupToLoadTo.textures[i], tex_path.c_str());
            }
        }
    }

    meshGroupToLoadTo.filePath = fileName;
    gpuLoadedModels[fileName] = meshGroupToLoadTo;
    return &gpuLoadedModels[fileName];
}

INTERNAL void MeshGroupDelete(MeshGroup* const modelToDelete)
{
    for(auto& mesh : modelToDelete->meshes)
    {
        MeshDelete(mesh);
    }
    for(auto& texture : modelToDelete->textures)
    {
        TextureDelete(texture);
    }

    auto gpuModelIter = gpuLoadedModels.find(modelToDelete->filePath);
    if(gpuModelIter != gpuLoadedModels.end())
    {
        gpuLoadedModels.erase(gpuModelIter);
    }
}




struct GameCamera
{
    vec3  position;             // camera x y z pos in world space
    vec3  rotation;             // roll yaw pitch - in that order
    vec3  calculatedDirection;  // Intuitive direction - direction forward
    vec3  calculatedUp;
    vec3  calculatedRight;
    mat4  perspectiveMatrix;    // Perspective projection matrix
    mat4  viewMatrix;           // Last calculated view matrix
    float nearClip              = 0.1f;
    float farClip               = 2000.f;
    float moveSpeed             = 6.f;
    float turnSpeed             = 0.15f;

    void CameraUpdate();
    void CameraCalculatePerspectiveMatrix();
    void CameraCalculateViewMatrix();
};

void GameCamera::CameraUpdate()
{
    if(SML_abs(gMouseDeltaX) < 50.f && SML_abs(gMouseDeltaY) < 50.f) // don't move if mouse delta is too big to be normal
    {
        rotation.y /*yaw*/ -= ((float)gMouseDeltaX) * turnSpeed * 0.8f;
        rotation.z /*pitch*/ -= ((float)gMouseDeltaY) * turnSpeed * 0.8f;
    }
    if(gGamepadStates[0].isConnected)
    {
        rotation.y -= gGamepadStates[0].rightThumbStickDir.x * turnSpeed * gDeltaTime * 260.f;
        rotation.z += gGamepadStates[0].rightThumbStickDir.y * turnSpeed * gDeltaTime * 260.f;
    }

    if(rotation.z > 89.f)
    {
        rotation.z = 89.f;
    }
    if(rotation.z < -89.f)
    {
        rotation.z = -89.f;
    }

    // Calcuate direction, right, and up vectors
    calculatedDirection = OrientationToDirection(EulerToQuat(rotation * SML_DEG2RAD));
    calculatedDirection = Normalize(calculatedDirection);
    calculatedRight = Normalize(Cross(calculatedDirection, SML_UP_VECTOR));
    calculatedUp = Normalize(Cross(calculatedRight, calculatedDirection));

    if(gGamepadStates[0].isConnected)
    {

        position += calculatedDirection * gGamepadStates[0].leftThumbStickDir.y * gDeltaTime;
        position += calculatedRight * gGamepadStates[0].leftThumbStickDir.x * gDeltaTime;

        if (gGamepadStates[0].IsPressed(GAMEPAD_RB)) {
            position.y += moveSpeed * gDeltaTime;
        }
        if (gGamepadStates[0].IsPressed(GAMEPAD_LB)) {
            position.y += -moveSpeed * gDeltaTime;
        }
    }

    if (gKeyState[SDL_SCANCODE_W])
    {
        position += calculatedDirection * moveSpeed * gDeltaTime;
    }
    if (gKeyState[SDL_SCANCODE_A])
    {
        position += -calculatedRight * moveSpeed * gDeltaTime;
    }
    if (gKeyState[SDL_SCANCODE_S])
    {
        position += -calculatedDirection * moveSpeed * gDeltaTime;
    }
    if (gKeyState[SDL_SCANCODE_D])
    {
        position += calculatedRight * moveSpeed * gDeltaTime;
    }
    if (gKeyState[SDL_SCANCODE_Q])
    {
        position.y += -moveSpeed * gDeltaTime;
    }
    if (gKeyState[SDL_SCANCODE_E])
    {
        position.y += moveSpeed * gDeltaTime;
    }

    CameraCalculateViewMatrix();
}

void GameCamera::CameraCalculatePerspectiveMatrix()
{
    float fov = 90.f;
    float aspect_ratio = (float)INTERNAL_RESOLUTION_WIDTH / (float)INTERNAL_RESOLUTION_HEIGHT;
    perspectiveMatrix = ProjectionMatrixPerspective((fov / 2.f) * SML_DEG2RAD, aspect_ratio, nearClip, farClip);
}

void GameCamera::CameraCalculateViewMatrix()
{
    viewMatrix = ViewMatrixLookAt(position, position + calculatedDirection, calculatedUp);
}





struct GameObject
{
    GameObject()
        : pos({ 0.f, 0.f, 0.f })
        , orientation(quat())
        , scale({ 1.f, 1.f, 1.f })
        , modelMatrix(mat4())
        , isModelMatrixOutdated(false)
        , model(nullptr)
    {}

    virtual void Update()
    {

    }

    virtual void Render(const Shader& shader)
    {
        MeshGroup* modelToRender = GetModel();
        if(modelToRender)
        {
            shader.GLBindMatrix4fv("modelMatrix", 1, GetModelMatrix().ptr());
            modelToRender->render();
        }
    }

    const mat4& GetModelMatrix()
    {
        if(isModelMatrixOutdated)
        {
            modelMatrix = mat4();
            modelMatrix *= TranslationMatrix(pos);
            modelMatrix *= RotationMatrix(orientation);
            modelMatrix *= ScaleMatrix(scale);
            isModelMatrixOutdated = false;
        }

        return modelMatrix;
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
    void Move(const vec3& moveAmount)
    {
        pos += moveAmount;
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

    MeshGroup* GetModel() const
    {
        return model;
    }
    void SetModel(MeshGroup* newModelPtr)
    {
        model = newModelPtr;
    }

private:
    vec3 pos; // World space
    quat orientation;
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





// GJK + EPA

/** A collider contains two basic pieces of information - a Shape and a Transform */
struct Collider
{
    /* A preparation function which transforms the collider to World Space from Model Space */
    virtual void TransformCollider() = 0;

    /* Returns the furthest point on the collider along a given direction */
    virtual vec3 FindFurthestPoint(vec3 direction) const = 0;

    GameObject* owningGameObject = nullptr;
};

struct MeshCollider : Collider
{
    void TransformCollider() override
    {
        mat4 modelMatrix = owningGameObject->GetModelMatrix();
        verticesWorldSpace.resize(verticesModelSpace.size());
        // TODO(Kevin): can probably multi-thread vertex position calculation
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

    std::vector<vec3> verticesModelSpace; // shouldn't change after first initialization
private:
    std::vector<vec3> verticesWorldSpace; // changes based on transform of owning game object
};

struct SphereCollider : Collider
{
    void TransformCollider() override
    {
        // TODO probably need to scale the radius too
        position = owningGameObject->GetPos();
    }

    vec3 FindFurthestPoint(vec3 direction) const override
    {
        vec3 offset = Normalize(direction) * radius;
        return position + offset;
    }

    vec3 position;
    float radius;
};

struct GJKSimplex
{
private:
    std::array<vec3, 4> points = {};
    u8 count = 0;

public:
    GJKSimplex& operator=(std::initializer_list<vec3> list)
    {
        count = 0;
        for(auto v : list)
        {
            points[count] = v;
            ++count;
        }
        return *this;
    }

    vec3& operator[](int i)
    {
        return points[i];
    }

    void pushFront(vec3 point)
    {
        points = { point, points[0], points[1], points[2] };
        count = SML_min(count + 1, 4u);
    }

    u8 size() const
    {
        return count;
    }
};

INTERNAL vec3 GJKSupport(const Collider* colA, const Collider* colB, const vec3& direction)
{
    // https://en.wikipedia.org/wiki/Support_function
    return colA->FindFurthestPoint(direction) - colB->FindFurthestPoint(-direction);
}

#define GJKSameDirection(dir, ao) (Dot(dir, ao) > 0.f)

INTERNAL bool GJKLine(GJKSimplex& capture, vec3& direction)
{
    vec3 a = capture[0];
    vec3 b = capture[1];
    vec3 ab = b - a;
    // Note that vec3 ao (a to origin) is just -a

    if(GJKSameDirection(ab, -a))
    {
        direction = Cross(Cross(ab, -a), ab);
    }
    else
    {
        capture = { a };
        direction = -a;
        // THIS PATH WILL SIGNAL THAT THERE IS NO COLLISION, but it might waste a support call
        // Is there a way to stop the algorithm here?
        // if origin is past point A, then it's impossible to be included in the simplex.
        // I need to figure out how to stop the algorithm here and return that they are not intersecting.
    }

    return false;
}

INTERNAL bool GJKTriangle(GJKSimplex& capture, vec3& direction)
{
    vec3 a = capture[0];
    vec3 b = capture[1];
    vec3 c = capture[2];
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 triangleNormal = Cross(ab, ac);

    if(GJKSameDirection(Cross(triangleNormal, ac), -a))
    {
        if(GJKSameDirection(ac, -a))
        {
            capture = { a, c };
            direction = Cross(Cross(ac, -a), ac);
        }
        else
        {
            capture = { a };
            direction = -a;
        }
    }
    else
    {
        if(GJKSameDirection(Cross(ab, triangleNormal), -a))
        {
            if(GJKSameDirection(ab, -a))
            {
                capture = { a, b };
                direction = Cross(Cross(ab, -a), ab);
            }
            else
            {
                capture = { a };
                direction = -a;
            }
        }
        else
        {
            // The origin must be above or below this triangle
            if(GJKSameDirection(triangleNormal, -a))
            {
                capture = { a, b, c };
                direction = triangleNormal;
            }
            else
            {
                capture = { a, c, b };
                direction = -triangleNormal;
            }
        }
    }

    return false;
}

INTERNAL bool GJKTetrahedron(GJKSimplex& capture, vec3& direction)
{
    vec3 a = capture[0];
    vec3 b = capture[1];
    vec3 c = capture[2];
    vec3 d = capture[3];
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 ad = d - a;
    vec3 abcNormal = Cross(ab, ac);
    vec3 acdNormal = Cross(ac, ad);
    vec3 adbNormal = Cross(ad, ab);

    /*
     * ORDER NEEDS TO BE ABC first, ACD second, ADB third OR IT MIGHT INFINITELY
     * LOOP THE TETRAHEDRON AND TRIANGLE CASES BUT I DON'T UNDERSTAND WHY
     * */

    if(GJKSameDirection(abcNormal, -a))
    {
        capture = { a, b, c };
        return GJKTriangle(capture, direction);
    }
    if(GJKSameDirection(acdNormal, -a))
    {
        capture = { a, c, d };
        return GJKTriangle(capture, direction);
    }
    if(GJKSameDirection(adbNormal, -a))
    {
        capture = { a, d, b };
        return GJKTriangle(capture, direction);
    }

    // IN THIS CASE: The origin is not in the direction of the normals of the three triangles formed with a.
    // AND we know that the origin is not "below" the triangle (if bcd forms the base) because we just added
    // the point a i.e. the origin is in direction of direction given to this function.
    // THEREFORE, the origin MUST be inside the tetrahedron
    return true;
}

INTERNAL bool GJKNextSimplex(GJKSimplex& capture, vec3& direction)
{
    /* We need a series of different checks for each shape of simplex
     * to see what the new simplex should be and what direction we’ll
     * search in next */
    switch (capture.size()) {
        case 2: return GJKLine(capture, direction);
        case 3: return GJKTriangle(capture, direction);
        case 4: return GJKTetrahedron(capture, direction);
    }

    ASSERT(false) // should never happen
    return false;
}

struct GJKEPACollisionInfo
{
    vec3 normal;
    float penetrationDepth;
    bool hasCollision;
};

INTERNAL std::pair<std::vector<vec4>, int> EPAGetFaceNormals(
        const std::vector<vec3>& polytope,
        const std::vector<int>& faces)
{
    /* Returns the normal of every face in the polytope, their distances
     * from the origin, and the index of the face closest to the origin. */

    std::vector<vec4> normals;
    int minTriangle = 0;
    float minDistance = FLT_MAX;

    for(int i = 0; i < faces.size(); i += 3)
    {
        vec3 a = polytope[faces[i]];
        vec3 b = polytope[faces[i + 1]];
        vec3 c = polytope[faces[i + 2]];
        vec3 normal = Normalize(Cross((b - a), (c - a)));
        float distance = Dot(normal, a); // the distance is the component of vector a projected onto the normal

        if(distance < 0.f)
        {
            normal *= -1.f;
            distance *= -1.f;
        }

        normals.emplace_back(normal, distance);

        if(distance < minDistance)
        {
            minDistance = distance;
            minTriangle = i / 3;
        }
    }

    return { normals, minTriangle };
}

INTERNAL void EPAAddIfUniqueEdge(
        std::vector<std::pair<int, int>>& edges,
        const std::vector<int>& faces,
        int a,
        int b)
{
    auto reverse = std::find(edges.begin(), edges.end(),
                             std::make_pair(faces[b], faces[a]));
    if(reverse != edges.end())
    {
        edges.erase(reverse);
    }
    else
    {
        edges.emplace_back(faces[a], faces[b]);
    }
}

#ifndef EPA_MAX_ITERATIONS
#define EPA_MAX_ITERATIONS 10
#endif
INTERNAL GJKEPACollisionInfo EPA(GJKSimplex& simplex, const Collider* colA, const Collider* colB)
{
    std::vector<vec3> polytope;
    ASSERT(simplex.size() == 4)
    for(int i = 0; i < simplex.size(); ++i)
    {
        polytope.push_back(simplex[i]);
    }
    std::vector<int> faces = {
            0, 1, 2,
            0, 3, 1,
            0, 2, 3,
            1, 3, 2
    };

    auto normalsResult = EPAGetFaceNormals(polytope, faces);
    auto normals = normalsResult.first;
    auto minFace = normalsResult.second;

    vec3 minNormal;
    float minDistance = FLT_MAX;

    u8 epaIterations = 0; // the EPA loop can be infinite if we have curved edges - just return whatever it has after X iterations.
    while(minDistance == FLT_MAX)
    {
        minNormal = normals[minFace].xyz;
        minDistance = normals[minFace].w;

        if(epaIterations++ > EPA_MAX_ITERATIONS)
        {
            break;
        }

        vec3 support = GJKSupport(colA, colB, minNormal); // i.e. is there a point to be added in this direction?
        float sDistance = Dot(minNormal, support);

        if(SML_abs(sDistance - minDistance) > 0.001f)
        {
            // then we must expand the polytope
            std::vector<std::pair<int, int>> uniqueEdges;
            for(int i = 0; i < normals.size(); ++i)
            {
                if(GJKSameDirection(normals[i].xyz, support))
                {
                    // for each face, if the face is in the same direction as support, remove
                    int f = i * 3;
                    EPAAddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
                    EPAAddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
                    EPAAddIfUniqueEdge(uniqueEdges, faces, f + 2, f);
                    // reconstruction
                    faces[f+2] = faces.back();
                    faces.pop_back();
                    faces[f+1] = faces.back();
                    faces.pop_back();
                    faces[f] = faces.back();
                    faces.pop_back();
                    normals[i] = normals.back();
                    normals.pop_back();
                    --i;
                }
            }

            std::vector<int> newFaces;
            for(auto edge : uniqueEdges)
            {
                newFaces.push_back(edge.first);
                newFaces.push_back(edge.second);
                newFaces.push_back(polytope.size());
            }
            polytope.push_back(support);

            normalsResult = EPAGetFaceNormals(polytope, newFaces);
            auto newNormals = normalsResult.first;
            auto newMinFace = normalsResult.second;

            float oldMinDistance = FLT_MAX;
            for(int i = 0; i < normals.size(); ++i)
            {
                if(normals[i].w < oldMinDistance)
                {
                    oldMinDistance = normals[i].w;
                    minFace = i;
                }
            }

            if(newNormals[newMinFace].w < oldMinDistance)
            {
                minFace = newMinFace + normals.size();
            }

            faces.insert(faces.end(), newFaces.begin(), newFaces.end());
            normals.insert(normals.end(), newNormals.begin(), newNormals.end());

            minDistance = FLT_MAX;
        }
    }

    GJKEPACollisionInfo points;
    points.normal = minNormal;
    points.penetrationDepth = minDistance + 0.001f;
    points.hasCollision = true;
    return points;
}

INTERNAL GJKEPACollisionInfo GJK(Collider* colA, Collider* colB)
{
    colA->TransformCollider();
    colB->TransformCollider();

    //NOTE(Kevin): start direction could be origin - (colA owning object position - colB owning object position)
    vec3 support = GJKSupport(colA, colB, {1.f,0.f,0.f});
    GJKSimplex capture;
    capture.pushFront(support);
    vec3 direction = -support;

    for(;;)
    {
        support = GJKSupport(colA, colB, direction);
        if(Dot(support, direction) <= 0)
        {
            // previous support point was already the furthest along direction
            GJKEPACollisionInfo collisionInfo;
            collisionInfo.hasCollision = false;
            return collisionInfo;
        }

        capture.pushFront(support);
        if(GJKNextSimplex(capture, direction))
        {
            GJKEPACollisionInfo collisionInfo = EPA(capture, colA, colB);
            collisionInfo.hasCollision = true;
            return collisionInfo;
        }
    }
}




// Collision World




