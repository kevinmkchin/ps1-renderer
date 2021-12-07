INTERNAL Shader sceneShader;

INTERNAL void gameInit()
{
    Shader::glLoadShaderProgramFromFile(sceneShader, "data/shaders/scene.vert", "data/shaders/scene.frag");
}

INTERNAL void gameUpdate()
{

}

INTERNAL void gameRender()
{
    glViewport(0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT);

    glClearColor(0.39f, 0.582f, 0.926f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader::glUseShader(sceneShader);

        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, deferred_composition_output_texture);
        LOCAL_PERSIST Mesh quad;
        LOCAL_PERSIST bool meshmade = false;
        if (!meshmade) {
            meshmade = true;
            u32 quadindices[6] = {
                    0, 1, 3,
                    0, 3, 2
            };
            GLfloat quadvertices[16] = {
                    //  x   y    u    v
                    -1.f, -1.f, 0.f, 0.f,
                    1.f, -1.f, 1.f, 0.f,
                    -1.f, 1.f, 0.f, 1.f,
                    1.f, 1.f, 1.f, 1.f
            };
            Mesh::glCreateMesh(quad, quadvertices, quadindices, 16, 6, 2, 2, 0);
        }
        quad.glRenderMesh();

    glUseProgram(0);   
}
