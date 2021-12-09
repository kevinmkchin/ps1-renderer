INTERNAL GameCamera camera;
INTERNAL Shader sceneShader;
INTERNAL MeshGroup cubeModel;
INTERNAL MeshGroup sphereModel;

INTERNAL void GameInit()
{
    Shader::GLLoadShaderProgramFromFile(sceneShader, "shaders/scene.vert", "shaders/scene.frag");
    camera.CameraCalculatePerspectiveMatrix();

    TextureCreateFromFile(missingTexture, "data/primitives/missing_texture.png");

    MeshGroupAssimpLoad(cubeModel, "data/primitives/cube.obj");
    MeshGroupAssimpLoad(sphereModel, "data/primitives/sphere.obj");
}

INTERNAL void GameUpdate()
{
    camera.CameraUpdate();
}

INTERNAL void GameRender()
{
    glViewport(0, 0, INTERNAL_RESOLUTION_WIDTH, INTERNAL_RESOLUTION_HEIGHT);
    glClearColor(RGB(46,88,120), 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    sceneShader.UseShader();
        sceneShader.GLBindMatrix4fv("viewMatrix", 1, camera.viewMatrix.ptr());
        sceneShader.GLBindMatrix4fv("projMatrix", 1, camera.perspectiveMatrix.ptr());

        sceneShader.GLBindMatrix4fv("modelMatrix", 1, identity_mat4().ptr());
        sceneShader.GLBind3f("modelColor", RGB(0,0,0));
        cubeModel.render();

        sceneShader.GLBindMatrix4fv("modelMatrix", 1, translation_matrix(5.f,0.f,0.f).ptr());
        sceneShader.GLBind3f("modelColor", RGB(255,255,255));
        sphereModel.render();
    glUseProgram(0);
}
