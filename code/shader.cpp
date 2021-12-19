/** Telling opengl to start using this shader program */
void Shader::UseShader()
{
    if (idShaderProgram == 0)
    {
        printf("WARNING: Passed an unloaded shader program to UseShader! Aborting.\n");
        return;
    }
    glUseProgram(idShaderProgram);
}

/** Delete the shader program off GPU memory */
void Shader::GLDeleteShader(Shader& shader)
{
    if (shader.idShaderProgram == 0)
    {
        printf("WARNING: Passed an unloaded shader program to GLDeleteShader! Aborting.\n");
        return;
    }
    glDeleteProgram(shader.idShaderProgram);
}

void Shader::GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* fragmentShaderStr)
{
    shader.idShaderProgram = glCreateProgram(); // Create an empty shader program and get the id
    if (!shader.idShaderProgram)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.idShaderProgram, vertexShaderStr, GL_VERTEX_SHADER); // Compile and attach the shaders
    gl_compile_shader(shader.idShaderProgram, fragmentShaderStr, GL_FRAGMENT_SHADER);
    glLinkProgram(shader.idShaderProgram); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.idShaderProgram))
    {
        return;
    }
#endif

    Shader::cache_uniform_locations(shader);
}

void Shader::GLCreateShaderProgram(Shader& shader, const char* vertexShaderStr, const char* geometryShaderStr, const char* fragmentShaderStr)
{

    shader.idShaderProgram = glCreateProgram(); // Create an empty shader program and get the id
    if (!shader.idShaderProgram)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.idShaderProgram, vertexShaderStr, GL_VERTEX_SHADER); // Compile and attach the shaders
    gl_compile_shader(shader.idShaderProgram, geometryShaderStr, GL_GEOMETRY_SHADER);
    gl_compile_shader(shader.idShaderProgram, fragmentShaderStr, GL_FRAGMENT_SHADER);
    glLinkProgram(shader.idShaderProgram); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.idShaderProgram))
    {
        return;
    }
#endif

    Shader::cache_uniform_locations(shader);
}


void Shader::GLCreateComputeShaderProgram(Shader& shader, const char* computeShaderStr)
{
    shader.idShaderProgram = glCreateProgram();
    if (!shader.idShaderProgram)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.idShaderProgram, computeShaderStr, GL_COMPUTE_SHADER);
    glLinkProgram(shader.idShaderProgram);

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.idShaderProgram))
    {
        return;
    }
#endif

    Shader::cache_uniform_locations(shader);
}

bool Shader::gl_check_error_and_validate(GLuint program_id)
{
#if INTERNAL_BUILD
    GLint result = 0;
    GLchar eLog[1024] = {};
    glGetProgramiv(program_id, GL_LINK_STATUS, &result); // Make sure the program was created
    if (!result)
    {
        glGetProgramInfoLog(program_id, sizeof(eLog), nullptr, eLog);
        printf("Error linking program: '%s'! Aborting.\n", eLog);
        return true;
    }

    // Validate the program will work
    glValidateProgram(program_id);
    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &result);
    if (!result)
    {
        glGetProgramInfoLog(program_id, sizeof(eLog), nullptr, eLog);
        printf("Error validating program %u: %s", program_id, eLog);
    }
#endif
    return false;
}

void Shader::GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* fragmentPath)
{
    std::string v = ReadFileString(vertexPath);
    std::string f = ReadFileString(fragmentPath);
    GLCreateShaderProgram(shader, v.c_str(), f.c_str());
}

void Shader::GLLoadShaderProgramFromFile(Shader& shader, const char* vertexPath, const char* geometryPath, const char* fragmentPath)
{
    std::string v = ReadFileString(vertexPath);
    std::string g = ReadFileString(geometryPath);
    std::string f = ReadFileString(fragmentPath);
    GLCreateShaderProgram(shader, v.c_str(), g.c_str(), f.c_str());
}

void Shader::GLLoadComputeShaderProgramFromFile(Shader& shader, const char* computePath)
{
    std::string c = ReadFileString(computePath);
    GLCreateComputeShaderProgram(shader, c.c_str());
}

void Shader::cache_uniform_locations(Shader &shader)
{
    shader.uniformLocationsMap.clear();

    GLint longest_uniform_name_length;
    GLint number_of_uniforms;
    glGetProgramiv(shader.idShaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longest_uniform_name_length);
    glGetProgramiv(shader.idShaderProgram, GL_ACTIVE_UNIFORMS, &number_of_uniforms);
    //printf("number of active uniforms for shader %d:  %d\n", shader.id_shader_program, number_of_uniforms);

    GLint readlength;
    GLint size;
    GLenum type;
    GLchar uniformName[128]; ASSERT(longest_uniform_name_length <= 128)

    /**If one or more elements of an array are active, the name of the array is returned in name, the type is returned
     * in type, and the size parameter returns the highest array element index used, plus one, as determined by the compiler
     * and/or linker. Only one active uniform variable will be reported for a uniform array.
    */
    for (GLint i = 0; i < number_of_uniforms; ++i)
    {
        glGetActiveUniform(shader.idShaderProgram, i, longest_uniform_name_length, &readlength, &size, &type, uniformName);
        cache_uniform_location(shader, uniformName);
    }
}

void Shader::cache_uniform_location(Shader& shader, const char *uniformName)
{
    i32 location = glGetUniformLocation(shader.idShaderProgram, uniformName);
    if (location != 0xffffffff)
    {
        shader.uniformLocationsMap[std::string(uniformName)] = location;
    }
    else
    {
        printf("Warning! Unable to get the location of uniform '%s' for shader id %d...\n", uniformName, shader.idShaderProgram);
    }
}

void Shader::gl_compile_shader(u32 program_id, const char* shader_code, GLenum shaderType)
{
    GLuint id_shader = glCreateShader(shaderType);             // Create an empty shader of given type and get id
    GLint code_length = (GLint) strlen(shader_code);
    glShaderSource(id_shader, 1, &shader_code, &code_length);   // Fill the empty shader with the shader code
    glCompileShader(id_shader);                                 // Compile the shader source

    GLint result = 0;
    GLchar eLog[1024] = {};
    glGetShaderiv(id_shader, GL_COMPILE_STATUS, &result);       // Make sure the shader compiled correctly
    if (!result)
    {
        glGetProgramInfoLog(id_shader, sizeof(eLog), nullptr, eLog);
        printf("Error compiling the %d shader: '%s' \n", shaderType, eLog);
        return;
    }

    glAttachShader(program_id, id_shader);
}

void Shader::GLBind1i(const char* uniformName, GLint v0) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform1i(location, v0);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind2i(const char* uniformName, GLint v0, GLint v1) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform2i(location, v0, v1);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind3i(const char* uniformName, GLint v0, GLint v1, GLint v2) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform3i(location, v0, v1, v2);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind4i(const char* uniformName, GLint v0, GLint v1, GLint v2, GLint v3) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform4i(location, v0, v1, v2, v3);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind1f(const char* uniformName, GLfloat v0) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform1f(location, v0);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind2f(const char* uniformName, GLfloat v0, GLfloat v1) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform2f(location, v0, v1);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind3f(const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform3f(location, v0, v1, v2);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBind4f(const char* uniformName, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniform4f(location, v0, v1, v2, v3);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBindMatrix3fv(const char* uniformName, GLsizei count, const GLfloat* value) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniformMatrix3fv(location, count, GL_FALSE, value);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

void Shader::GLBindMatrix4fv(const char* uniformName, GLsizei count, const GLfloat* value) const
{
    i32 location = GetCachedUniformLocation(uniformName);
    if(location >= 0)
    {
        glUniformMatrix4fv(location, count, GL_FALSE, value);
    }
    else
    {
        warning_uniform_not_found(uniformName);
    }
}

i32 Shader::GetCachedUniformLocation(const char* uniformName) const
{
    auto location_iter = uniformLocationsMap.find(uniformName);
    if(location_iter != uniformLocationsMap.end())
    {
        return location_iter->second;
    }
    return -1;
}

void Shader::warning_uniform_not_found(const char* uniformName) const
{
    printf("Warning: Uniform '%s' doesn't exist or isn't active on shader %d.\n", uniformName, idShaderProgram);
}