#pragma once
#include <functional>
#include <stdexcept>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "type.hpp"

namespace gawl {
void init_graphics();
void finish_graphics();

class Shader {
  private:
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint shader_program;
    GLuint vao;

  public:
    void   bind_vao();
    GLuint get_shader();
    Shader(const char* vertex_shader_source, const char* fragment_shader_source);
    ~Shader();
};

class GawlWindow;
class GraphicBase {
  private:
    GLuint texture;

  protected:
    Shader&      type_specific;
    int          width, height;
    virtual void prepare_shader();

  public:
    virtual int get_width(const GawlWindow* window) const;
    virtual int get_height(const GawlWindow* window) const;
    void        draw(const GawlWindow* window, double x, double y);
    void        draw_rect(const GawlWindow* window, Area area);
    void        draw_fit_rect(const GawlWindow* window, Area area);
    GraphicBase(Shader& type_specific);
    virtual ~GraphicBase();
};
} // namespace gawl
