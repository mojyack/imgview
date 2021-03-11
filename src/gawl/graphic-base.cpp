#include <IL/il.h>

#include "global.hpp"
#include "graphic-base.hpp"
#include "misc.hpp"
#include "type.hpp"
#include "gawl-window.hpp"

namespace gawl {
extern GlobalVar* global;
namespace {
GLuint  ebo;
GLuint  vbo;
GLfloat vertices[4][4];
void    move_vertices(const GawlWindow* window, Area area) {
    gawl::convert_screen_to_viewport(window, area);
    vertices[0][0] = area[0];
    vertices[0][1] = area[1];
    vertices[1][0] = area[2];
    vertices[1][1] = area[1];
    vertices[2][0] = area[2];
    vertices[2][1] = area[3];
    vertices[3][0] = area[0];
    vertices[3][1] = area[3];
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
}
} // namespace
void init_graphics() {
    ilInit();
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

    const static GLuint elements[] = {
        0, 1, 2,
        2, 3, 0};
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    vertices[0][2] = 0.0;
    vertices[0][3] = 0.0;
    vertices[1][2] = 1.0;
    vertices[1][3] = 0.0;
    vertices[2][2] = 1.0;
    vertices[2][3] = 1.0;
    vertices[3][2] = 0.0;
    vertices[3][3] = 1.0;
}
void finish_graphics() {
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
}
void Shader::bind_vao() {
    glBindVertexArray(vao);
}
GLuint Shader::get_shader() {
    return shader_program;
}
Shader::Shader(const char* vertex_shader_source, const char* fragment_shader_source) {
    GLint status;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    if(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &status); status != GL_TRUE) {
        throw std::runtime_error("vertex shader");
    }
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    if(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &status); status != GL_TRUE) {
        throw std::runtime_error("fragment shader");
    }
    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    if(glGetProgramiv(shader_program, GL_LINK_STATUS, &status); status != GL_TRUE) {
        throw std::runtime_error("link shader");
    }
    glBindFragDataLocation(shader_program, 0, "outColor");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    auto pos_attrib = glGetAttribLocation(shader_program, "position");
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
    glEnableVertexAttribArray(pos_attrib);

    auto tex_attrib = glGetAttribLocation(shader_program, "texcoord");
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (void*)(sizeof(GLfloat) * 2));
    glEnableVertexAttribArray(tex_attrib);
    glUseProgram(shader_program);

    glBindVertexArray(0);
}
Shader::~Shader() {
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shader_program);
    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);
}
void GraphicBase::prepare_shader() {
}
int GraphicBase::get_width(const GawlWindow* window) const {
    return width / window->get_scale();
}
int GraphicBase::get_height(const GawlWindow* window) const {
    return height / window->get_scale();
}
void GraphicBase::draw(const GawlWindow* window, double x, double y) {
    draw_rect(window, {x, y, x + width, y + height});
}
void GraphicBase::draw_rect(const GawlWindow* window, Area area) {
    area.magnify(window->get_scale());
    move_vertices(window, area);
    type_specific.bind_vao();
    glUseProgram(type_specific.get_shader());
    prepare_shader();
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void GraphicBase::draw_fit_rect(const GawlWindow* window, Area area) {
    draw_rect(window, calc_fit_rect(area, width, height));
}
GraphicBase::GraphicBase(Shader& type_specific) : type_specific(type_specific) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
GraphicBase::~GraphicBase() {
    glDeleteTextures(1, &texture);
}
} // namespace gawl
