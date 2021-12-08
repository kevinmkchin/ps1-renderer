struct Texture
{
    /** Handle for texture stored in GPU memory */

    GLuint  textureId   = 0;        // ID for the texture in GPU memory
    i32     width       = 0;        // Width of the texture
    i32     height      = 0;        // Height of the texture
    GLenum  format      = GL_NONE;  // format / bitdepth of texture (GL_RGB would be 3 byte bit depth)

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
        printf("WARNING: Attempting to clear a texture with id: 0. This means this texture hasn't been loaded!\n");
        return;
    }
    glDeleteTextures(1, &texture.textureId);

    for(auto it = gpuLoadedTextures.begin(); it != gpuLoadedTextures.end(); ++it)
    {
        if(it->second.textureId == texture.textureId)
        {
            gpuLoadedTextures.erase(it);
            break;
        }
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

    gpuLoadedTextures[std::string(textureFilePath)] = texture;
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

    void render();

    void clear();
};

void MeshGroup::render()
{
    for(size_t i = 0; i < meshes.size(); ++i)
    {
        u16 materialIndex = meshToTextureTable[i];
        if(materialIndex < textures.size() && textures[materialIndex].textureId != 0)
        {
            textures[materialIndex].UseTexture();
        }

        meshes[i].RenderMesh();
    }
}

void MeshGroup::clear()
{
    for(auto& mesh : meshes)
    {
        MeshDelete(mesh);
    }
    for(auto& texture : textures)
    {
        TextureDelete(texture);
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

/** Load model from disk through Assimp */
INTERNAL void MeshGroupAssimpLoad(MeshGroup& meshGroupToLoadTo, const char* fileName)
{
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
        return;
    }

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
                idx = kc_max((int)model_file_directory.find_last_of("/"), (int)model_file_directory.find_last_of("\\"));
                model_file_directory = model_file_directory.substr(0, idx + 1);

                std::string tex_path = model_file_directory + texture_file_name;
                TextureCreateFromFile(meshGroupToLoadTo.textures[i], tex_path.c_str());
            }
        }
    }
}




struct GameCamera
{
    vec3  position              = { 0.f }; // camera x y z pos in world space
    vec3  rotation              = { 0.f }; // roll yaw pitch - in that order
    vec3  calculatedDirection   = {0.f }; // Intuitive direction - direction forward
    vec3  calculatedUp          = {0.f };
    vec3  calculatedRight       = {0.f };
    mat4  perspectiveMatrix     = { 0.f }; // Perspective projection matrix
    mat4  viewMatrix            = { 0.f }; // Last calculated view matrix
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
    if(kc_abs(gMouseDeltaX) < 50.f && kc_abs(gMouseDeltaY) < 50.f) // don't move if mouse delta is too big to be normal
    {
        rotation.y /*yaw*/ -= ((float)gMouseDeltaX) * turnSpeed * 0.8f;
        rotation.z /*pitch*/ -= ((float)gMouseDeltaY) * turnSpeed * 0.8f;
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
    calculatedDirection = orientation_to_direction(euler_to_quat(rotation * KC_DEG2RAD));
    calculatedDirection = normalize(calculatedDirection);
    calculatedRight = normalize(cross(calculatedDirection, WORLD_UP_VECTOR));
    calculatedUp = normalize(cross(calculatedRight, calculatedDirection));

    if (gKeystate[SDL_SCANCODE_W])
    {
        position += calculatedDirection * moveSpeed * gDeltaTime;
    }
    if (gKeystate[SDL_SCANCODE_A])
    {
        position += -calculatedRight * moveSpeed * gDeltaTime;
    }
    if (gKeystate[SDL_SCANCODE_S])
    {
        position += -calculatedDirection * moveSpeed * gDeltaTime;
    }
    if (gKeystate[SDL_SCANCODE_D])
    {
        position += calculatedRight * moveSpeed * gDeltaTime;
    }
    if (gKeystate[SDL_SCANCODE_Q])
    {
        position.y += -moveSpeed * gDeltaTime;
    }
    if (gKeystate[SDL_SCANCODE_E])
    {
        position.y += moveSpeed * gDeltaTime;
    }

    viewMatrix = view_matrix_look_at(position, position + calculatedDirection, calculatedUp);
}

void GameCamera::CameraCalculatePerspectiveMatrix()
{
    float fov = 90.f;
    float aspect_ratio = (float)INTERNAL_RESOLUTION_WIDTH / (float)INTERNAL_RESOLUTION_HEIGHT;
    perspectiveMatrix = projection_matrix_perspective((fov/2.f)*KC_DEG2RAD, aspect_ratio, nearClip, farClip);
}
