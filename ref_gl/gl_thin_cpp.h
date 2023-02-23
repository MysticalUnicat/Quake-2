#ifndef __GL_THIN_CPP_H__
#define __GL_THIN_CPP_H__

#include "alias/cpp.h"
#include <stdint.h>

#include "gl_thin.h"

#define THIN_GL_ALIGNED(X) __attribute__((aligned(X)))

#define THIN_GL_STRUCT_TO_C_require(NAME)
#define THIN_GL_STRUCT_TO_C_unorm8(NAME) uint8_t NAME[4] THIN_GL_ALIGNED(4);     // uint (4, 4) -> float
#define THIN_GL_STRUCT_TO_C_unorm8x2(NAME) uint8_t NAME[4] THIN_GL_ALIGNED(4);   // uint (4, 4) -> vec2
#define THIN_GL_STRUCT_TO_C_unorm8x3(NAME) uint8_t NAME[4] THIN_GL_ALIGNED(4);   // uint (4, 4) -> vec3
#define THIN_GL_STRUCT_TO_C_unorm8x4(NAME) uint8_t NAME[4] THIN_GL_ALIGNED(4);   // uint (4, 4) -> vec4
#define THIN_GL_STRUCT_TO_C_unorm16(NAME) uint16_t NAME[2] THIN_GL_ALIGNED(4);   // uint (4, 4) -> float
#define THIN_GL_STRUCT_TO_C_unorm16x2(NAME) uint16_t NAME[2] THIN_GL_ALIGNED(4); // uint (4, 4) -> vec2
#define THIN_GL_STRUCT_TO_C_unorm16x3(NAME) uint16_t NAME[4] THIN_GL_ALIGNED(8); // uvec2 (8, 8) -> vec3
#define THIN_GL_STRUCT_TO_C_unorm16x4(NAME) uint16_t NAME[4] THIN_GL_ALIGNED(8); // uvec2 (8, 8) -> vec4
#define THIN_GL_STRUCT_TO_C_snorm16(NAME) int16_t NAME[2] THIN_GL_ALIGNED(4);    // uint (4, 4) -> float
#define THIN_GL_STRUCT_TO_C_snorm16x2(NAME) int16_t NAME[2] THIN_GL_ALIGNED(4);  // uint (4, 4) -> vec2
#define THIN_GL_STRUCT_TO_C_snorm16x3(NAME) int16_t NAME[4] THIN_GL_ALIGNED(8);  // uvec2 (8, 8) -> vec3
#define THIN_GL_STRUCT_TO_C_snorm16x4(NAME) int16_t NAME[4] THIN_GL_ALIGNED(8);  // uvec2 (8, 8) -> vec4
#define THIN_GL_STRUCT_TO_C_uint32(NAME) uint32_t NAME THIN_GL_ALIGNED(4);       // uint (4, 4) -> uint
#define THIN_GL_STRUCT_TO_C_int32(NAME) int32_t NAME THIN_GL_ALIGNED(4);         // int (4, 4) -> int
#define THIN_GL_STRUCT_TO_C_float32(NAME) float NAME THIN_GL_ALIGNED(4);         // float (4, 4) -> float
#define THIN_GL_STRUCT_TO_C_float32x2(NAME) float NAME[2] THIN_GL_ALIGNED(8);    // vec2 (8, 8) -> vec2
#define THIN_GL_STRUCT_TO_C_float32x3(NAME) float NAME[3] THIN_GL_ALIGNED(16);   // vec3 (16, 16) -> vec3
#define THIN_GL_STRUCT_TO_C_float32x4(NAME) float NAME[4] THIN_GL_ALIGNED(16);   // vec4 (16, 16) -> vec4
#define THIN_GL_STRUCT_TO_C_struct(TYPE, NAME) struct GL_##TYPE NAME;
#define THIN_GL_STRUCT_TO_C_unsized_array(TYPE)
#define THIN_GL_STRUCT_TO_C_(ITEM) ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_C_, ITEM)
#define THIN_GL_STRUCT_TO_C(NAME, ...)                                                                                 \
  struct GL_##NAME {                                                                                                   \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_C_, __VA_ARGS__))                                                   \
  };

#define THIN_GL_STRUCT_TO_GLSL_PACKED_require(NAME) "//"
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm8(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm8x2(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm8x3(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm8x4(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm16(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm16x2(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm16x3(NAME) "uvec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_unorm16x4(NAME) "uvec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_snorm16(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_snorm16x2(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_snorm16x3(NAME) "uvec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_snorm16x4(NAME) "uvec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_uint32(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_int32(NAME) "int " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_float32(NAME) "float " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_float32x2(NAME) "vec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_float32x3(NAME) "vec3 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_float32x4(NAME) "vec4 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_struct(TYPE, NAME) #TYPE "Packed " #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACKED_(ITEM) "  " ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_GLSL_PACKED_, ITEM) ";\n"

#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_require(NAME) "//"
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm8(NAME) "float " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm8x2(NAME) "vec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm8x3(NAME) "vec3 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm8x4(NAME) "vec4 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm16(NAME) "float " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm16x2(NAME) "vec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm16x3(NAME) "vec3 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_unorm16x4(NAME) "vec4 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_snorm16(NAME) "float " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_snorm16x2(NAME) "vec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_snorm16x3(NAME) "vec3 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_snorm16x4(NAME) "vec4 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_uint32(NAME) "uint " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_int32(NAME) "int " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_float32(NAME) "float " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_float32x2(NAME) "vec2 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_float32x3(NAME) "vec3 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_float32x4(NAME) "vec4 " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_struct(TYPE, NAME) #TYPE " " #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACKED_(ITEM) "  " ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_GLSL_UNPACKED_, ITEM) ";\n"

#define THIN_GL_STRUCT_TO_GLSL_PACK_require(NAME) "//"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm8(NAME) "packUnorm4x8(vec4(unpack." #NAME ", 0, 0, 0))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm8x2(NAME) "packUnorm4x8(vec4(unpack." #NAME ", 0, 0))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm8x3(NAME) "packUnorm4x8(vec4(unpack." #NAME ", 0))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm8x4(NAME) "packUnorm4x8(unpack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm16(NAME) "packUnorm2x16(vec2(unpack." #NAME ", 0))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm16x2(NAME) "packUnorm2x16(unpack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm16x3(NAME)                                                                    \
  "uvec2(packUnorm2x16(unpack." #NAME ".xy), packUnorm2x16(vec2(unpack." #NAME ".z, 0)))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_unorm16x4(NAME)                                                                    \
  "uvec2(packUnorm2x16(unpack." #NAME ".xy), packUnorm2x16(unpack." #NAME ".zw))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_snorm16(NAME) "packSnorm2x16(vec2(unpack." #NAME ", 0))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_snorm16x2(NAME) "packSnorm2x16(unpack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_PACK_snorm16x3(NAME)                                                                    \
  "uvec2(packSnorm2x16(unpack." #NAME ".xy), packSnorm2x16(vec2(unpack." #NAME ".z, 0)))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_snorm16x4(NAME)                                                                    \
  "uvec2(packSnorm2x16(unpack." #NAME ".xy), packSnorm2x16(unpack." #NAME ".zw))"
#define THIN_GL_STRUCT_TO_GLSL_PACK_uint32(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_int32(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_float32(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_float32x2(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_float32x3(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_float32x4(NAME) "unpack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_PACK_struct(TYPE, NAME) #TYPE "_pack(unpack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_PACK_(ITEM) ",\n  " ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_GLSL_PACK_, ITEM)

#define THIN_GL_STRUCT_TO_GLSL_UNPACK_require(NAME) "//"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm8(NAME) "unpackUnorm4x8(pack." #NAME ").x"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm8x2(NAME) "unpackUnorm4x8(pack." #NAME ").xy"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm8x3(NAME) "unpackUnorm4x8(pack." #NAME ").xyz"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm8x4(NAME) "unpackUnorm4x8(pack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm16(NAME) "unpackUnorm2x16(pack." #NAME ").x"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm16x2(NAME) "unpackUnorm2x16(pack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm16x3(NAME)                                                                  \
  "vec3(unpackUnorm2x16(pack." #NAME ".x), unpackUnorm2x16(pack." #NAME ".y).x)"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_unorm16x4(NAME)                                                                  \
  "vec4(unpackUnorm2x16(pack." #NAME ".x), unpackUnorm2x16(pack." #NAME ".y))"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_snorm16(NAME) "unpackSnorm2x16(pack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_snorm16x2(NAME) "unpackSnorm2x16(pack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_snorm16x3(NAME)                                                                  \
  "vec3(unpackSnorm2x16(pack." #NAME ".x), unpackSnorm2x16(pack." #NAME ".y).x)"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_snorm16x4(NAME)                                                                  \
  "vec4(unpackSnorm2x16(pack." #NAME ".x), unpackSnorm2x16(pack." #NAME ".y))"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_uint32(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_int32(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_float32(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_float32x2(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_float32x3(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_float32x4(NAME) "pack." #NAME
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_struct(TYPE, NAME) #TYPE "_unpack(pack." #NAME ")"
#define THIN_GL_STRUCT_TO_GLSL_UNPACK_(ITEM) ",\n  " ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_GLSL_UNPACK_, ITEM)

// clang-format off
#define THIN_GL_STRUCT_TO_GLSL(NAME, ...)                                                                              \
  static const char GL_##NAME##_glsl[] =                                                                               \
    "struct " #NAME " {\n" ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_GLSL_UNPACKED_, __VA_ARGS__)) "};\n"         \
    "struct " #NAME "Packed {\n" ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_GLSL_PACKED_, __VA_ARGS__)) "};\n"     \
    #NAME "Packed " #NAME "_pack(in " #NAME " unpack) {\n"                                                             \
    "  return " #NAME "Packed(//"                                                                                      \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_GLSL_PACK_, __VA_ARGS__))                                           \
    "  );\n"                                                                                                           \
    "}\n"                                                                                                              \
    #NAME " " #NAME "_unpack(in " #NAME "Packed pack) {\n"                                                             \
    "  return " #NAME "(//"                                                                                            \
    ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_GLSL_UNPACK_, __VA_ARGS__))                                         \
    "  );\n"                                                                                                           \
    "}\n";
// clang-format onX

#define THIN_GL_STRUCT_TO_DESC_require(NAME) &GL_##NAME##_shader,
#define THIN_GL_STRUCT_TO_DESC_unorm8(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm8x2(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm8x3(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm8x4(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm16(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm16x2(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm16x3(NAME)
#define THIN_GL_STRUCT_TO_DESC_unorm16x4(NAME)
#define THIN_GL_STRUCT_TO_DESC_snorm16(NAME)
#define THIN_GL_STRUCT_TO_DESC_snorm16x2(NAME)
#define THIN_GL_STRUCT_TO_DESC_snorm16x3(NAME)
#define THIN_GL_STRUCT_TO_DESC_snorm16x4(NAME)
#define THIN_GL_STRUCT_TO_DESC_uint32(NAME)
#define THIN_GL_STRUCT_TO_DESC_int32(NAME)
#define THIN_GL_STRUCT_TO_DESC_float32(NAME)
#define THIN_GL_STRUCT_TO_DESC_float32x2(NAME)
#define THIN_GL_STRUCT_TO_DESC_float32x3(NAME)
#define THIN_GL_STRUCT_TO_DESC_float32x4(NAME)
#define THIN_GL_STRUCT_TO_DESC_struct(TYPE, NAME)
#define THIN_GL_STRUCT_TO_DESC_unsized_array(TYPE)
#define THIN_GL_STRUCT_TO_DESC_(ITEM) ALIAS_CPP_CAT(THIN_GL_STRUCT_TO_DESC_, ITEM)
#define THIN_GL_STRUCT_TO_DESC(NAME, ...)                                                                              \
  struct GL_Shader GL_##NAME##_shader = {                                                                              \
      .requires = {ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_STRUCT_TO_DESC_, __VA_ARGS__)) NULL},                          \
      .name = #NAME,                                                                                                   \
      .source = GL_##NAME##_glsl};

#define THIN_GL_DECLARE_STRUCT(NAME, ...)                                                                              \
  THIN_GL_STRUCT_TO_C(NAME, __VA_ARGS__)                                                                               \
  extern struct GL_Shader GL_##NAME##_shader;

#define THIN_GL_IMPL_STRUCT(NAME, ...)                                                                                 \
  THIN_GL_STRUCT_TO_GLSL(NAME, __VA_ARGS__)                                                                            \
  THIN_GL_STRUCT_TO_DESC(NAME, __VA_ARGS__)

#define THIN_GL_STRUCT(NAME, ...)                                                                                      \
  THIN_GL_STRUCT_TO_C(NAME, __VA_ARGS__)                                                                               \
  THIN_GL_STRUCT_TO_GLSL(NAME, __VA_ARGS__)                                                                            \
  THIN_GL_STRUCT_TO_DESC(NAME, __VA_ARGS__)

THIN_GL_DECLARE_STRUCT(DrawArraysIndirectCommand, uint32(count), uint32(instance_count), uint32(first),
                       uint32(base_instance))

THIN_GL_DECLARE_STRUCT(DrawElementsIndirectCommand, uint32(count), uint32(instance_count), uint32(first_index),
                       uint32(base_vertex), uint32(base_instance))

THIN_GL_DECLARE_STRUCT(DispatchIndirectCommand, uint32(num_groups_x), uint32(num_groups_y), uint32(num_groups_z))

#define THIN_GL_BLOCK_TO_C(NAME, ...) THIN_GL_STRUCT_TO_C(NAME, __VA_ARGS__)
#define THIN_GL_BLOCK_TO_DESC(NAME, ...) THIN_GL_STRUCT_TO_DESC(NAME, __VA_ARGS__)

#define THIN_GL_CAT(A, ...) THIN_GL_CAT_(A, ##__VA_ARGS__)
#define THIN_GL_CAT_(A, ...) A##__VA_ARGS__

#define THIN_GL_BLOCK_TO_GLSL_require(NAME) "  //"
#define THIN_GL_BLOCK_TO_GLSL_uint32(NAME) "  uint " #NAME
#define THIN_GL_BLOCK_TO_GLSL_int32(NAME) "  int " #NAME
#define THIN_GL_BLOCK_TO_GLSL_float32(NAME) "  float " #NAME
#define THIN_GL_BLOCK_TO_GLSL_float32x2(NAME) "  vec2 " #NAME
#define THIN_GL_BLOCK_TO_GLSL_float32x3(NAME) "  vec3 " #NAME
#define THIN_GL_BLOCK_TO_GLSL_float32x4(NAME) "  vec4 " #NAME
#define THIN_GL_BLOCK_TO_GLSL_struct(TYPE, NAME) "  " #TYPE " " #NAME
#define THIN_GL_BLOCK_TO_GLSL_unsized_array(TYPE) THIN_GL_CAT(THIN_GL_BLOCK_TO_GLSL_, TYPE) "[]"
#define THIN_GL_BLOCK_TO_GLSL_(ITEM) ALIAS_CPP_CAT(THIN_GL_BLOCK_TO_GLSL_, ITEM) ";\n"

#define THIN_GL_BLOCK_TO_GLSL(NAME, ...)                                                                               \
  static const char GL_##NAME##_glsl[] =                                                                               \
      "buffer " #NAME " {\n" ALIAS_CPP_EVAL(ALIAS_CPP_MAP(THIN_GL_BLOCK_TO_GLSL_, __VA_ARGS__)) "} ";

#define THIN_GL_DECLARE_BLOCK(NAME, ...)                                                                               \
  THIN_GL_BLOCK_TO_C(NAME, __VA_ARGS__)                                                                                \
  extern struct GL_Shader GL_##NAME##_shader;

#define THIN_GL_IMPL_BLOCK(NAME, ...)                                                                                  \
  THIN_GL_BLOCK_TO_GLSL(NAME, __VA_ARGS__)                                                                             \
  THIN_GL_BLOCK_TO_DESC(NAME, __VA_ARGS__)

#define THIN_GL_BLOCK(NAME, ...)                                                                                       \
  THIN_GL_BLOCK_TO_C(NAME, __VA_ARGS__)                                                                                \
  THIN_GL_BLOCK_TO_GLSL(NAME, __VA_ARGS__)                                                                             \
  THIN_GL_BLOCK_TO_DESC(NAME, __VA_ARGS__)

#endif