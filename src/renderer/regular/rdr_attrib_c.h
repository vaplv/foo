#ifndef RDR_ATTRIB_C_H
#define RDR_ATTRIB_C_H

#include "renderer/rdr_attrib.h"
#include "render_backend/rb_types.h"
#include <stddef.h>

size_t sizeof_rdr_type(enum rdr_type type);
size_t sizeof_rb_type(enum rb_type type);
enum rb_type rdr_to_rb_type(enum rdr_type type);
enum rdr_type rb_to_rdr_type(enum rb_type type);

#endif /* RDR_ATTRIB_C_H */

