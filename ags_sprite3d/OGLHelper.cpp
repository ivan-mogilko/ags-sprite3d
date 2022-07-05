#include "OGLHelper.h"
#include <string>
#include <vector>
#include <glad/glad.h>
#include "Common.h"

void SetMatrix(Matrix* matrix, float tx, float ty, float sx, float sy)
{
    matrix->_11 = sx;
    matrix->_12 = 0.0;
    matrix->_13 = 0.0;
    matrix->_14 = 0.0;
    matrix->_21 = 0.0;
    matrix->_22 = sy;
    matrix->_23 = 0.0;
    matrix->_24 = 0.0;
    matrix->_31 = 0.0;
    matrix->_32 = 0.0;
    matrix->_33 = 1.0;
    matrix->_34 = 0.0;
    matrix->_41 = tx;
    matrix->_42 = ty;
    matrix->_43 = 0.0;
    matrix->_44 = 1.0;
}

void SetMatrixIdentity(Matrix* matrix)
{
    matrix->_11 = 1.0;
    matrix->_12 = 0.0;
    matrix->_13 = 0.0;
    matrix->_14 = 0.0;
    matrix->_21 = 0.0;
    matrix->_22 = 1.0;
    matrix->_23 = 0.0;
    matrix->_24 = 0.0;
    matrix->_31 = 0.0;
    matrix->_32 = 0.0;
    matrix->_33 = 1.0;
    matrix->_34 = 0.0;
    matrix->_41 = 0.0;
    matrix->_42 = 0.0;
    matrix->_43 = 0.0;
    matrix->_44 = 1.0;
}

void SetMatrixRotation(Matrix* matrix, float radians)
{
    float sin = std::sinf(radians);
    float cos = std::cosf(radians);

    matrix->_11 = cos;
    matrix->_12 = -sin;
    matrix->_13 = 0.0;
    matrix->_14 = 0.0;
    matrix->_21 = sin;
    matrix->_22 = cos;
    matrix->_23 = 0.0;
    matrix->_24 = 0.0;
    matrix->_31 = 0.0;
    matrix->_32 = 0.0;
    matrix->_33 = 1.0;
    matrix->_34 = 0.0;
    matrix->_41 = 0.0;
    matrix->_42 = 0.0;
    matrix->_43 = 0.0;
    matrix->_44 = 1.0;
}

void MatrixMultiply(Matrix* result, const Matrix* a, const Matrix* b)
{
    Matrix temp;
    for (int j = 0; j < 4; ++j)
    {
        for (int i = 0; i < 4; ++i)
        {
            temp.m[i][j] = a->m[0][j] * b->m[i][0] +
                a->m[1][j] * b->m[i][1] +
                a->m[2][j] * b->m[i][2] +
                a->m[3][j] * b->m[i][3];
        }
    }
    memcpy(result->marr, temp.marr, sizeof(Matrix::marr));
}

unsigned CreateTexture(unsigned char const* const* data, int width, int height, bool alpha)
{
    unsigned texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    SetTextureData(texture, data, width, height);
    return texture;
}

bool SetTextureData(unsigned texture, unsigned char const* const* data, int width, int height)
{
    int bpp = 4; // TODO: get from elsewhere?
    unsigned char *input = new unsigned char[width * height * bpp];
    int pitch = width * bpp;
    for (int y = 0; y < height; ++y)
    {
        memcpy(input + pitch * y, data[y], pitch);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, input);
    delete input;
    return true;
}

void OutputShaderError(GLuint obj_id, const char* obj_name, bool is_shader)
{
    GLint log_len;
    if (is_shader)
        glGetShaderiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
    else
        glGetProgramiv(obj_id, GL_INFO_LOG_LENGTH, &log_len);
    std::vector<GLchar> errorLog(log_len);
    if (log_len > 0)
    {
        if (is_shader)
            glGetShaderInfoLog(obj_id, log_len, &log_len, &errorLog[0]);
        else
            glGetProgramInfoLog(obj_id, log_len, &log_len, &errorLog[0]);
    }

    DBG("ERROR: OpenGL: %s %s:", obj_name, is_shader ? "failed to compile" : "failed to link");
    if (errorLog.size() > 0)
    {
        DBG("----------------------------------------");
        DBG("%s", &errorLog[0]);
        DBG("----------------------------------------");
    }
    else
    {
        DBG("Shader info log was empty.");
    }
}

bool CreateShaderProgram(ShaderProgram &prg, const char *name, const char *vertex_shader_src, const char *fragment_shader_src)
{
    GLint result;

    GLint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::string name_ = name + std::string(" program's vertex shader");
        OutputShaderError(vertex_shader, name_.c_str(), true);
        return false;
    }

    GLint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::string name_ = name + std::string(" program's fragment shader");
        OutputShaderError(fragment_shader, name_.c_str(), true);
        glDeleteShader(fragment_shader); //not sure yet if this goes here
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        std::string name_ = name + std::string(" program");
        OutputShaderError(program, name_.c_str(), false);
        glDeleteProgram(program); //not sure yet if this goes here
        glDeleteShader(fragment_shader); //not sure yet if this goes here
        return false;
    }

    glDetachShader(program, vertex_shader);
    glDeleteShader(vertex_shader);

    glDetachShader(program, fragment_shader);
    glDeleteShader(fragment_shader);

    prg.Program = program;
    DBG("OGL: %s shader program created successfully", name);
    return true;
}

void DeleteShaderProgram(ShaderProgram &prg)
{
    if (prg.Program)
        glDeleteProgram(prg.Program);
    prg.Program = 0;
}
