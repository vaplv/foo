#include "renderer/regular/rdr_attrib_c.h"
#include <assert.h>
#include <stdbool.h>

size_t
sizeof_rdr_type(enum rdr_type type)
{
  switch(type) {
    case RDR_FLOAT: return sizeof(float);
    case RDR_FLOAT2: return 2 * sizeof(float);
    case RDR_FLOAT3: return 3 * sizeof(float);
    case RDR_FLOAT4: return 4 * sizeof(float);
    case RDR_FLOAT4x4: return 16 * sizeof(float);
    case RDR_UNKNOWN_TYPE: return 0;
    default:
      assert(false);
      return 0;
  }
}

size_t
sizeof_rb_type(enum rb_type type)
{
  switch(type) {
    case RB_FLOAT: return sizeof(float);
    case RB_FLOAT2: return 2 * sizeof(float);
    case RB_FLOAT3: return 3 * sizeof(float);
    case RB_FLOAT4: return 4 * sizeof(float);
    case RB_FLOAT4x4: return 16 * sizeof(float);
    case RB_UNKNOWN_TYPE: return 0;
    default:
      assert(false);
      return 0;
  }
}

enum rb_type
rdr_to_rb_type(enum rdr_type type)
{
  switch(type) {
    case RDR_FLOAT: return RB_FLOAT;
    case RDR_FLOAT2: return RB_FLOAT2;
    case RDR_FLOAT3: return RB_FLOAT3;
    case RDR_FLOAT4: return RB_FLOAT4;
    case RDR_FLOAT4x4: return RB_FLOAT4x4;
    case RDR_UNKNOWN_TYPE: return RB_UNKNOWN_TYPE;
    default:
      assert(false);
      return RB_UNKNOWN_TYPE;
  }
}

enum rdr_type
rb_to_rdr_type(enum rb_type type)
{
  switch(type) {
    case RB_FLOAT: return RDR_FLOAT;
    case RB_FLOAT2: return RDR_FLOAT2;
    case RB_FLOAT3: return RDR_FLOAT3;
    case RB_FLOAT4: return RDR_FLOAT4;
    case RB_FLOAT4x4: return RDR_FLOAT4x4;
    case RB_UNKNOWN_TYPE: return RDR_UNKNOWN_TYPE;
    default:
      assert(false);
      return RDR_UNKNOWN_TYPE;
  }
}

