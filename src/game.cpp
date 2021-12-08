INTERNAL GameCamera camera;
INTERNAL Shader sceneShader;
INTERNAL MeshGroup cubeModel;

INTERNAL std::string bgColor = "blue";

INTERNAL void GameInit()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    camera.CameraCalculatePerspectiveMatrix();

    MeshGroupAssimpLoad(cubeModel, "data/cube/cube.obj");

    console.bind_cvar("color", &bgColor);
}

INTERNAL void GameUpdate()
{
    camera.CameraUpdate();
}

INTERNAL void GameRender()
{
    glViewport(0, 0, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
    // glClearColor(RGB(46,88,120), 1.f);
    if(bgColor == "red")
    {
        glClearColor(RGB(243,52,57), 1.f);
    }
    else if(bgColor == "blue")
    {
        glClearColor(RGB(46,88,120), 1.f);
    }
    else if(bgColor == "green")
    {
        glClearColor(RGB(103,192,115), 1.f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();
    sceneShader.GLBindMatrix4fv("modelMatrix", 1, identity_mat4().ptr());
    sceneShader.GLBindMatrix4fv("viewMatrix", 1, camera.viewMatrix.ptr());
    sceneShader.GLBindMatrix4fv("projMatrix", 1, camera.perspectiveMatrix.ptr());
        cubeModel.render();
    glUseProgram(0);
}
