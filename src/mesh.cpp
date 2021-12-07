struct Mesh
{
    /** Stores mesh { VAO, VBO, IBO } info. Handle for VAO on GPU memory
     *  Holds the ID for the VAO, VBO, IBO in the GPU memory
    */

    u32  idVAO          = 0;
    u32  idVBO          = 0;
    u32  idIBO          = 0;
    u32  indicesCount   = 0;

    /** Create a Mesh with the given vertices and indices.
    vertex_attrib_size: vertex coords size (e.g. 3 if x y z)
    texture_attrib_size: texture coords size (e.g. 2 if u v)
    draw_usage: affects optimization; GL_STATIC_DRAW buffer data
    only set once, GL_DYNAMIC_DRAW if buffer modified repeatedly */
    static void glCreateMesh(Mesh& mesh,
                               float* vertices,
                               u32* indices,
                               u32 vertices_array_count,
                               u32 indices_array_count,
                               u8 vertex_attrib_size = 3,
                               u8 texture_attrib_size = 2,
                               u8 normal_attrib_size = 3,
                               GLenum draw_usage = GL_STATIC_DRAW);

    /** Clearing GPU memory: glDeleteBuffers and glDeleteVertexArrays deletes the buffer
        object and vertex array object off the GPU memory. */
    static void glDeleteMesh(Mesh& mesh);

    /** Binds VAO and draws elements. Bind a shader program and texture
        before calling glRenderMesh */
    void glRenderMesh(GLenum render_mode = GL_TRIANGLES) const;

    /** Overwrite existing buffer data */
    void glRebindBufferObjects(float* vertices,
                                  u32* indices,
                                  u32 vertices_array_count,
                                  u32 indices_array_count,
                                  GLenum draw_usage = GL_DYNAMIC_DRAW);
};

void Mesh::glCreateMesh(Mesh& mesh,
                            float* vertices,
                            u32* indices,
                            u32 vertices_array_count,
                            u32 indices_array_count,
                            u8 vertex_attrib_size,
                            u8 texture_attrib_size,
                            u8 normal_attrib_size,
                            GLenum draw_usage)
{
    u8 stride = 0;
    if(texture_attrib_size)
    {
        stride += vertex_attrib_size + texture_attrib_size;
        if(normal_attrib_size)
        {
            stride += normal_attrib_size;
        }
    }

    // Need to store to index_count because we need the count of indices when we are drawing in Mesh::render_mesh
    mesh.indicesCount = indices_array_count;

    glGenVertexArrays(1, &mesh.idVAO); // Defining some space in the GPU for a vertex array and giving you the vao ID
    glBindVertexArray(mesh.idVAO); // Binding a VAO means we are currently operating on that VAO
    // Indentation is to indicate that we are now working within the bound VAO
    glGenBuffers(1, &mesh.idVBO); // Creating a buffer object inside the bound VAO and returning the ID
    glBindBuffer(GL_ARRAY_BUFFER, mesh.idVBO); // Bind VBO to operate on that VBO
    /* Connect the vertices data to the actual gl array buffer for this VBO. We need to pass in the size of the data we are passing as well.
    GL_STATIC_DRAW (as opposed to GL_DYNAMIC_DRAW) means we won't be changing these data values in the array.
    The vertices array does not need to exist anymore after this call because that data will now be stored in the VAO on the GPU. */
    glBufferData(GL_ARRAY_BUFFER, 4 /*bytes cuz float*/ * vertices_array_count, vertices, draw_usage);
    /* Index is location in VAO of the attribute we are creating this pointer for.
    Size is number of values we are passing in (e.g. size is 3 if x y z).
    Normalized is normalizing the values.
    Stride is the number of values to skip after getting the values we need.
        for example, you could have vertices and colors in the same array
        [ Ax, Ay, Az,  Ar, Ag, Ab,  Bx, By, Bz,  Br, Bg, Bb ]
            use          stride        use          stride
        In this case, the stride would be 3 because we need to skip 3 values (the color values) to reach the next vertex data.
    Apparently the last parameter is the offset? */
    glVertexAttribPointer(0, vertex_attrib_size, GL_FLOAT, GL_FALSE, sizeof(float) * stride, 0); // vertex pointer
    glEnableVertexAttribArray(0); // Enabling location in VAO for the attribute
    if(texture_attrib_size > 0)
    {
        glVertexAttribPointer(1, texture_attrib_size, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * vertex_attrib_size)); // uv coord pointer
        glEnableVertexAttribArray(1);
        if(normal_attrib_size > 0)
        {
            glVertexAttribPointer(2, normal_attrib_size, GL_FLOAT, GL_FALSE, sizeof(float) * stride, (void*)(sizeof(float) * (vertex_attrib_size + texture_attrib_size))); // normal pointer
            glEnableVertexAttribArray(2);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO

    // Index Buffer Object
    glGenBuffers(1, &mesh.idIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 /*bytes cuz uint32*/ * indices_array_count, indices, draw_usage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); // Unbind the VAO;
}

void Mesh::glDeleteMesh(Mesh& mesh)
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

void Mesh::glRenderMesh(GLenum render_mode) const
{
    if (indicesCount == 0) // Early out if index_count == 0, nothing to draw
    {
        printf("WARNING: Attempting to render a mesh with 0 index count!\n");
        return;
    }

    // Bind VAO, bind VBO, draw elements(indexed draw)
    glBindVertexArray(idVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
            glDrawElements(render_mode, indicesCount, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Mesh::glRebindBufferObjects(float* vertices,
                                      u32* indices,
                                      u32 vertices_array_count,
                                      u32 indices_array_count,
                                      GLenum draw_usage)
{
    if(idVBO == 0 || idIBO == 0)
    {
        return;
    }

    indicesCount = indices_array_count;
    glBindVertexArray(idVAO);
        glBindBuffer(GL_ARRAY_BUFFER, idVBO);
            glBufferData(GL_ARRAY_BUFFER, 4 * vertices_array_count, vertices, draw_usage);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * indices_array_count, indices, draw_usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}