#include "render_backend/rb.h"
#include "window_manager/wm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glext.h>

#define M_DEG_TO_RAD(x) ((x) * 3.14159 / 180.f)
#ifndef NDEBUG
  #define CALL(f) assert(f == 0)
#else
  #define CALL(f) f
#endif

struct render_mesh {
  struct rb_buffer* vertex_buffer;
  struct rb_buffer* index_buffer;
  struct rb_vertex_array* vertex_array;
};

struct render_shader {
  struct rb_shader* vertex_shader;
  struct rb_shader* fragment_shader;
  struct rb_program* program;
  struct rb_uniform* view_uniform;
  struct rb_uniform* proj_uniform;
};

static void
compute_proj(float fov_y, float ratio, float znear, float zfar, float* proj)
{
  const float f = cosf(fov_y*0.5f) / sinf(fov_y*0.5f);
  proj[0] = f / ratio;
  proj[1] = 0.f;
  proj[2] = 0.f;
  proj[3] = 0.f;

  proj[4] = 0.f;
  proj[5] = f;
  proj[6] = 0.f;
  proj[7] = 0.f;

  proj[8] = 0.f;
  proj[9] = 0.f;
  proj[10] = (zfar + znear) / (znear - zfar);
  proj[11] = -1.f;

  proj[12] = 0.f;
  proj[13] = 0.f;
  proj[14] = (2.f * zfar *znear) / (znear - zfar);
  proj[15] = 0.f;
}

static void
compute_view(float angle, float x, float y, float z, float* view)
{
  const float cosa = cosf(angle);
  const float sina = sinf(angle);

  view[0] = cosa;
  view[1] = 0;
  view[2] = -sina;
  view[3] = 0.f;

  view[4] = 0.f;
  view[5] = 1.f;
  view[6] = 0.f;
  view[7] = 0.f;

  view[8] = sina;
  view[9] = 0.f;
  view[10] = cosa;
  view[11] = 0.f;

  view[12] = x;
  view[13] = y;
  view[14] = z;
  view[15] = 1.f;
}

static int
free_mesh(struct rb_context* ctxt, struct render_mesh* mesh)
{
  if(!ctxt || !mesh)
    return -1;

  if(mesh->vertex_array)
    rb_free_vertex_array(ctxt, mesh->vertex_array);
  if(mesh->vertex_buffer)
    rb_free_buffer(ctxt, mesh->vertex_buffer);
  if(mesh->index_buffer)
    rb_free_buffer(ctxt, mesh->index_buffer);

  free(mesh);
  return 0;
}

static int
create_mesh(struct rb_context* ctxt, struct render_mesh** out_mesh)
{
  /* Vertex buffer. Interleaved positions (float3) and colors (float3). */
  const float vertices[] = {
    -1.f, -1.f, -1.f, 0.f, 0.f, 0.f,
    -1.f, 1.f, -1.f, 0.f, 1.f, 0.f,
    1.f, 1.f, -1.f, 1.f, 1.f, 0.f,
    1.f, -1.f, -1.f, 1.f, 0.f, 0.f,
    -1.f, -1.f, 1.f, 0.f, 0.f, 1.f,
    -1.f, 1.f, 1.f, 0.f, 1.f, 1.f,
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
    1.f, -1.f, 1.f, 1.f, 0.f, 1.f
  };
  const struct rb_buffer_desc vb_desc = {
    .size = sizeof(vertices),
    .target = RB_BIND_VERTEX_BUFFER,
    .usage = RB_BUFFER_USAGE_IMMUTABLE
  };

  /* Index buffer. */
  const unsigned int indices[] = {
    0, 1, 2, 0, 2, 3, /* back */
    3, 2, 6, 3, 6, 7, /* right */
    7, 6, 5, 7, 5, 4, /* front */
    4, 5, 1, 4, 1, 0, /* left */
    4, 0, 7, 0, 3, 7, /* bottom */
    5, 2, 1, 5, 6, 2  /* top */
  };
  const struct rb_buffer_desc ib_desc = {
    .size = sizeof(indices),
    .target = RB_BIND_INDEX_BUFFER,
    .usage = RB_BUFFER_USAGE_IMMUTABLE
  };

  /* Vertex attrib. */
  struct rb_buffer_attrib attribs[] = {
    /* Position. */
    {
      .index = 0,
      .stride = sizeof(float) * 6.f,
      .offset = 0,
      .format = RB_ATTRIB_FLOAT3
    },
    /* Color. */
    {
      .index = 1,
      .stride = sizeof(float) * 6.f,
      .offset = sizeof(float) * 3.f,
      .format = RB_ATTRIB_FLOAT3
    }
  };

  struct render_mesh* mesh = NULL;

  if(!ctxt || !out_mesh)
    return -1;

  mesh = malloc(sizeof(struct render_mesh));
  if(!mesh) {
    fprintf(stderr, "Error allocating the render mesh.\n");
    return -1;
  }

  CALL(rb_create_buffer(ctxt, &vb_desc, vertices, &mesh->vertex_buffer));
  CALL(rb_create_buffer(ctxt, &ib_desc, indices, &mesh->index_buffer));
  CALL(rb_create_vertex_array(ctxt, &mesh->vertex_array));
  CALL(rb_vertex_attrib_array
       (ctxt, mesh->vertex_array, mesh->vertex_buffer, 2, attribs));
  CALL(rb_vertex_index_array(ctxt, mesh->vertex_array, mesh->index_buffer));

  *out_mesh = mesh;

  return 0;
}

static int
free_shader(struct rb_context* ctxt, struct render_shader* shader)
{
  if(!ctxt || !shader)
    return -1;

  if(shader->view_uniform)
    rb_release_uniform(ctxt, shader->view_uniform);
  if(shader->proj_uniform)
    rb_release_uniform(ctxt, shader->proj_uniform);
  if(shader->program)
    rb_free_program(ctxt, shader->program);
  if(shader->vertex_shader)
    rb_free_shader(ctxt, shader->vertex_shader);
  if(shader->fragment_shader)
    rb_free_shader(ctxt, shader->fragment_shader);

  free(shader);

  return 0;
}

static int
create_shader(struct rb_context* ctxt, struct render_shader** out_shader)
{
  int err = 0;

  const char* vs_source =
    "#version 330\n"
    "uniform mat4x4 view;\n"
    "uniform mat4x4 proj;\n"
    "layout (location = 0) in vec3 in_position;\n"
    "layout (location = 1) in vec3 in_color; \n"
    "smooth out vec3 theColor;\n"
    "void main()\n"
    "{\n"
    " theColor = in_color;\n"
    " gl_Position = proj * view * vec4(in_position, 1.f); \n"
    "}\n";

  const char* fs_source =
    "#version 330\n"
    "smooth in vec3 theColor;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    " color = vec4(theColor, 1.f);\n"
    "}\n";

  struct render_shader* shader = NULL;
  size_t length = 0;

  if(!ctxt || !out_shader) {
    err = -1;
    goto error;
  }

  shader = malloc(sizeof(struct render_shader));
  memset(shader, 0, sizeof(struct render_shader));
  if(!shader) {
    err = -1;
    goto error;
  }

  length = strlen(vs_source);
  err = rb_create_shader
    (ctxt, RB_VERTEX_SHADER, vs_source, length, &shader->vertex_shader);
  if(err != 0) {
    if(shader->vertex_shader) {
      const char* log = NULL;
      CALL(rb_get_shader_log(ctxt, shader->vertex_shader, &log));
      if(log)
        fprintf(stderr, "error: vertex shader\n%s", log);
    }
    goto error;
  }

  length = strlen(fs_source);
  err = rb_create_shader
    (ctxt, RB_FRAGMENT_SHADER, fs_source, length, &shader->fragment_shader);
  if(err != 0) {
    if(shader->fragment_shader) {
      const char* log = NULL;
      CALL(rb_get_shader_log(ctxt, shader->fragment_shader, &log));
      if(log)
        fprintf(stderr, "error: pixel shader\n%s", log);
    }
    goto error;
  }

  CALL(rb_create_program(ctxt, &shader->program));
  CALL(rb_attach_shader(ctxt, shader->program, shader->vertex_shader));
  CALL(rb_attach_shader(ctxt, shader->program, shader->fragment_shader));

  err = rb_link_program(ctxt, shader->program);
  if(err != 0) {
    const char* log = NULL;
    CALL(rb_get_program_log(ctxt, shader->program, &log));
    if(log)
      fprintf(stderr, "error: program link\n%s", log);
    goto error;
  }

  CALL(rb_get_uniform(ctxt, shader->program, "view", &shader->view_uniform));
  CALL(rb_get_uniform(ctxt, shader->program, "proj", &shader->proj_uniform));

  *out_shader = shader;

exit:
  return err;

error:
  if(shader)
    free_shader(ctxt, shader);

  goto exit;
}

int
main(int argc, char* argv[])
{
  /* Window manaher data structures. */
  struct wm_device* device = NULL;
  struct wm_window* window = NULL;
  struct wm_window_desc win_desc = {
    .width = 800, .height = 600, .fullscreen = 0
  };

  /* Render backend data structures. */
  struct rb_context* ctxt = NULL;
  struct rb_viewport_desc viewport = {
    .x = 0,
    .y = 0,
    .width = win_desc.width,
    .height = win_desc.height,
    .min_depth = 0.f,
    .max_depth = 1.f
  };
  struct rb_rasterizer_desc rasterizer = {
    .fill_mode = RB_FILL_SOLID,
    .cull_mode = RB_CULL_BACK,
    .front_facing = RB_ORIENTATION_CCW
  };
  struct rb_depth_stencil_desc depth_stencil = {
    .enable_depth_test = 1,
    .enable_stencil_test = 0,
    .depth_func = RB_COMPARISON_LESS_EQUAL
  };

  /* Miscellaneous data structures. */
  struct render_mesh* mesh = NULL;
  struct render_shader* shader = NULL;
  const float clear_color[] = { 0.02f, 0.02f, 0.02f, 0.f };
  float view_matrix[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };
  float proj_matrix[16] = {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f
  };
  const float proj_ratio = (float)win_desc.width / (float)win_desc.height;
  float angle = 0.f;

  CALL(wm_create_device(&device));
  CALL(wm_create_window(device, &window, &win_desc));
  CALL(rb_create_context(&ctxt));

  CALL(create_mesh(ctxt, &mesh));
  CALL(create_shader(ctxt, &shader));

  while(1) {
    CALL(rb_viewport(ctxt, &viewport));
    CALL(rb_rasterizer(ctxt, &rasterizer));
    CALL(rb_depth_stencil(ctxt, &depth_stencil));

    compute_proj(M_DEG_TO_RAD(70.f), proj_ratio, 1.f, 500.f, proj_matrix);
    compute_view(angle, 0.f, -2.f, -6.f, view_matrix);
    CALL(rb_uniform_data(ctxt, shader->view_uniform, 1, view_matrix));
    CALL(rb_uniform_data(ctxt, shader->proj_uniform, 1, proj_matrix));

    CALL(rb_clear
         (ctxt, RB_CLEAR_COLOR_BIT|RB_CLEAR_DEPTH_BIT, clear_color, 1.f, 0));
    CALL(rb_bind_program(ctxt, shader->program));
    CALL(rb_bind_vertex_array(ctxt, mesh->vertex_array));
    CALL(rb_draw_indexed(ctxt, RB_TRIANGLE_LIST, 36));

    CALL(rb_flush(ctxt));
    CALL(wm_swap(device, window));

    angle += M_DEG_TO_RAD(0.01f);
  }

  if(ctxt) {
    free_shader(ctxt, shader);
    free_mesh(ctxt, mesh);
    rb_free_context(ctxt);
  }

  if(device) {
    wm_free_window(device, window);
    wm_free_device(device);
  }

  return 0;
}

