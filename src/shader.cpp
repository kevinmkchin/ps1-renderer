struct Shader
{
    static void glUseShader(Shader& shader);
    static void glDeleteShader(Shader& shader);
    static void glLoadShaderProgramFromFile(Shader& shader, const char* vertex_path, const char* fragment_path);
    static void glLoadShaderProgramFromFile(Shader& shader, const char* vertex_path, const char* geometry_path, const char* fragment_path);
    static void glCreateShaderProgram(Shader& shader, const char* vertex_shader_str, const char* fragment_shader_str);
    static void glCreateShaderProgram(Shader& shader, const char* vertex_shader_str, const char* geometry_shader_str, const char* fragment_shader_str);
    static void glLoadComputeShaderProgramFromFile(Shader& shader, const char* compute_path);
    static void glCreateComputeShaderProgram(Shader& shader, const char* compute_shader_str);

    void glBind1i(const char* uniform_name, GLint v0) const;
    void glBind2i(const char* uniform_name, GLint v0, GLint v1) const;
    void glBind3i(const char* uniform_name, GLint v0, GLint v1, GLint v2) const;
    void glBind4i(const char* uniform_name, GLint v0, GLint v1, GLint v2, GLint v3) const;
    void glBind1f(const char* uniform_name, GLfloat v0) const;
    void glBind2f(const char* uniform_name, GLfloat v0, GLfloat v1) const;
    void glBind3f(const char* uniform_name, GLfloat v0, GLfloat v1, GLfloat v2) const;
    void glBind4f(const char* uniform_name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const;
    void glBindMatrix3fv(const char* uniform_name, GLsizei count, const GLfloat* value) const;
    void glBindMatrix4fv(const char* uniform_name, GLsizei count, const GLfloat* value) const;

    i32 getCachedUniformLocation(const char* uniform_name) const;

private:
    GLuint id_shader_program = 0; // id of this shader program in GPU memory

    std::unordered_map<std::string, i32> uniform_locations;

    static void cache_uniform_locations(Shader& shader);

    static void cache_uniform_location(Shader& shader, const char* uniform_name);

    void warning_uniform_not_found(const char* uniform_name) const;

    // Create shader on GPU and compile shader
    static void gl_compile_shader(u32 program_id, const char* shader_code, GLenum Shaderype);

    // Return true if there was a compile error
    static bool gl_check_error_and_validate(GLuint program_id);
};

/** Telling opengl to start using this shader program */
void Shader::glUseShader(Shader& shader)
{
    if (shader.id_shader_program == 0)
    {
        printf("WARNING: Passed an unloaded shader program to glUseShader! Aborting.\n");
        return;
    }
    glUseProgram(shader.id_shader_program);
}

/** Delete the shader program off GPU memory */
void Shader::glDeleteShader(Shader& shader)
{
    if (shader.id_shader_program == 0)
    {
        printf("WARNING: Passed an unloaded shader program to glDeleteShader! Aborting.\n");
        return;
    }
    glDeleteProgram(shader.id_shader_program);
}

void Shader::glCreateShaderProgram(Shader& shader, const char* vertex_shader_str, const char* fragment_shader_str)
{
    shader.id_shader_program = glCreateProgram(); // Create an empty shader program and get the id
    if (!shader.id_shader_program)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.id_shader_program, vertex_shader_str, GL_VERTEX_SHADER); // Compile and attach the shaders
    gl_compile_shader(shader.id_shader_program, fragment_shader_str, GL_FRAGMENT_SHADER);
    glLinkProgram(shader.id_shader_program); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.id_shader_program))
    {
        return;
    }
#endif

    Shader::cache_uniform_locations(shader);
}

void Shader::glCreateShaderProgram(Shader& shader, const char* vertex_shader_str, const char* geometry_shader_str, const char* fragment_shader_str)
{

    shader.id_shader_program = glCreateProgram(); // Create an empty shader program and get the id
    if (!shader.id_shader_program)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.id_shader_program, vertex_shader_str, GL_VERTEX_SHADER); // Compile and attach the shaders
    gl_compile_shader(shader.id_shader_program, geometry_shader_str, GL_GEOMETRY_SHADER);
    gl_compile_shader(shader.id_shader_program, fragment_shader_str, GL_FRAGMENT_SHADER);
    glLinkProgram(shader.id_shader_program); // Actually create the exectuable shader program on the graphics card

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.id_shader_program))
    {
        return;
    }
#endif

    Shader::cache_uniform_locations(shader);
}


void Shader::glCreateComputeShaderProgram(Shader& shader, const char* compute_shader_str)
{
    shader.id_shader_program = glCreateProgram();
    if (!shader.id_shader_program)
    {
        printf("Failed to create shader program! Aborting.\n");
        return;
    }
    gl_compile_shader(shader.id_shader_program, compute_shader_str, GL_COMPUTE_SHADER);
    glLinkProgram(shader.id_shader_program);

#if INTERNAL_BUILD
    if (gl_check_error_and_validate(shader.id_shader_program))
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

void Shader::glLoadShaderProgramFromFile(Shader& shader, const char* vertex_path, const char* fragment_path)
{
    std::string v = readFileString(vertex_path);
    std::string f = readFileString(fragment_path);
    glCreateShaderProgram(shader, v.c_str(), f.c_str());
}

void Shader::glLoadShaderProgramFromFile(Shader& shader, const char* vertex_path, const char* geometry_path, const char* fragment_path)
{
    std::string v = readFileString(vertex_path);
    std::string g = readFileString(geometry_path);
    std::string f = readFileString(fragment_path);
    glCreateShaderProgram(shader, v.c_str(), g.c_str(), f.c_str());
}

void Shader::glLoadComputeShaderProgramFromFile(Shader& shader, const char* compute_path)
{
    std::string c = readFileString(compute_path);
    glCreateComputeShaderProgram(shader, c.c_str());
}

void Shader::cache_uniform_locations(Shader &shader)
{
    shader.uniform_locations.clear();

    GLint longest_uniform_name_length;
    GLint number_of_uniforms;
    glGetProgramiv(shader.id_shader_program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &longest_uniform_name_length);
    glGetProgramiv(shader.id_shader_program, GL_ACTIVE_UNIFORMS, &number_of_uniforms);
    //printf("number of active uniforms for shader %d:  %d\n", shader.id_shader_program, number_of_uniforms);

    GLint readlength;
    GLint size;
    GLenum type;
    GLchar uniform_name[128]; ASSERT(longest_uniform_name_length <= 128)

    /**If one or more elements of an array are active, the name of the array is returned in name, the type is returned
     * in type, and the size parameter returns the highest array element index used, plus one, as determined by the compiler
     * and/or linker. Only one active uniform variable will be reported for a uniform array.
    */
    for (GLint i = 0; i < number_of_uniforms; ++i)
    {
        glGetActiveUniform(shader.id_shader_program, i, longest_uniform_name_length, &readlength, &size, &type, uniform_name);
        cache_uniform_location(shader, uniform_name);
    }
}

void Shader::cache_uniform_location(Shader& shader, const char *uniform_name)
{
    i32 location = glGetUniformLocation(shader.id_shader_program, uniform_name);
    if (location != 0xffffffff)
    {
        shader.uniform_locations[std::string(uniform_name)] = location;
    }
    else
    {
        printf("Warning! Unable to get the location of uniform '%s' for shader id %d...\n", uniform_name, shader.id_shader_program);
    }
}

void Shader::gl_compile_shader(u32 program_id, const char* shader_code, GLenum Shaderype)
{
    GLuint id_shader = glCreateShader(Shaderype);             // Create an empty shader of given type and get id
    GLint code_length = (GLint) strlen(shader_code);
    glShaderSource(id_shader, 1, &shader_code, &code_length);   // Fill the empty shader with the shader code
    glCompileShader(id_shader);                                 // Compile the shader source

    GLint result = 0;
    GLchar eLog[1024] = {};
    glGetShaderiv(id_shader, GL_COMPILE_STATUS, &result);       // Make sure the shader compiled correctly
    if (!result)
    {
        glGetProgramInfoLog(id_shader, sizeof(eLog), nullptr, eLog);
        printf("Error compiling the %d shader: '%s' \n", Shaderype, eLog);
        return;
    }

    glAttachShader(program_id, id_shader);
}

void Shader::glBind1i(const char* uniform_name, GLint v0) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform1i(location, v0);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind2i(const char* uniform_name, GLint v0, GLint v1) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform2i(location, v0, v1);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind3i(const char* uniform_name, GLint v0, GLint v1, GLint v2) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform3i(location, v0, v1, v2);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind4i(const char* uniform_name, GLint v0, GLint v1, GLint v2, GLint v3) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform4i(location, v0, v1, v2, v3);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind1f(const char* uniform_name, GLfloat v0) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform1f(location, v0);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind2f(const char* uniform_name, GLfloat v0, GLfloat v1) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform2f(location, v0, v1);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind3f(const char* uniform_name, GLfloat v0, GLfloat v1, GLfloat v2) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform3f(location, v0, v1, v2);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBind4f(const char* uniform_name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniform4f(location, v0, v1, v2, v3);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBindMatrix3fv(const char* uniform_name, const GLsizei count, const GLfloat* value) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniformMatrix3fv(location, count, GL_FALSE, value);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

void Shader::glBindMatrix4fv(const char* uniform_name, const GLsizei count, const GLfloat* value) const
{
    i32 location = getCachedUniformLocation(uniform_name);
    if(location >= 0)
    {
        glUniformMatrix4fv(location, count, GL_FALSE, value);
    }
    else
    {
        warning_uniform_not_found(uniform_name);
    }
}

i32 Shader::getCachedUniformLocation(const char* uniform_name) const
{
    auto location_iter = uniform_locations.find(uniform_name);
    if(location_iter != uniform_locations.end())
    {
        return location_iter->second;
    }
    return -1;
}

void Shader::warning_uniform_not_found(const char* uniform_name) const
{
    printf("Warning: Uniform '%s' doesn't exist or isn't active on shader %d.\n", uniform_name, id_shader_program);
}