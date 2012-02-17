#ifndef RB_OGL3_H
#define RB_OGL3_H

#include "render_backend/rb_types.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <assert.h>

#ifndef NDEBUG
  #include <GL/glu.h>
  #include <assert.h>
  #include <stdio.h>
  #define OGL(func)\
    gl##func;\
    {\
      GLenum gl_error = glGetError();\
      if(gl_error != GL_NO_ERROR) {\
        fprintf(stderr, "error:opengl: %s\n", gluErrorString(gl_error));\
        assert(gl_error == GL_NO_ERROR);\
      }\
    }
#else
  #define OGL(func) gl##func
#endif

static inline enum rb_type
ogl3_to_rb_type(GLenum attrib_type)
{
  switch(attrib_type) {
    case GL_FLOAT: return RB_FLOAT;
    case GL_FLOAT_VEC2: return RB_FLOAT2;
    case GL_FLOAT_VEC3: return RB_FLOAT3;
    case GL_FLOAT_VEC4: return RB_FLOAT4;
    case GL_FLOAT_MAT4: return RB_FLOAT4x4;
    default: return RB_UNKNOWN_TYPE;
  }
}

#endif /* RB_OGL3_H */

