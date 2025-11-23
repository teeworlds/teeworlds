if(NOT PREFER_BUNDLED_LIBS)
  if(NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig QUIET)
    pkg_check_modules(ZSTD libzstd)
  endif()
endif()

if(NOT ZSTD_FOUND)
  set(ZSTD_BUNDLED ON)
  set(ZSTD_SRC_DIR src/engine/external/zstd)
  set_src(ZSTD_SRC GLOB_RECURSE ${ZSTD_SRC_DIR}
    common/allocations.h
    common/bits.h
    common/bitstream.h
    common/compiler.h
    common/cpu.h
    common/debug.c
    common/debug.h
    common/entropy_common.c
    common/error_private.c
    common/error_private.h
    common/fse.h
    common/fse_decompress.c
    common/huf.h
    common/mem.h
    common/pool.c
    common/pool.h
    common/portability_macros.h
    common/threading.c
    common/threading.h
    common/xxhash.c
    common/xxhash.h
    common/zstd_common.c
    common/zstd_deps.h
    common/zstd_internal.h
    common/zstd_trace.h
    compress/clevels.h
    compress/fse_compress.c
    compress/hist.c
    compress/hist.h
    compress/huf_compress.c
    compress/zstd_compress.c
    compress/zstd_compress_internal.h
    compress/zstd_compress_literals.c
    compress/zstd_compress_literals.h
    compress/zstd_compress_sequences.c
    compress/zstd_compress_sequences.h
    compress/zstd_compress_superblock.c
    compress/zstd_compress_superblock.h
    compress/zstd_cwksp.h
    compress/zstd_double_fast.c
    compress/zstd_double_fast.h
    compress/zstd_fast.c
    compress/zstd_fast.h
    compress/zstd_lazy.c
    compress/zstd_lazy.h
    compress/zstd_ldm.c
    compress/zstd_ldm.h
    compress/zstd_ldm_geartab.h
    compress/zstd_opt.c
    compress/zstd_opt.h
    compress/zstd_preSplit.c
    compress/zstd_preSplit.h
    compress/zstdmt_compress.c
    compress/zstdmt_compress.h
    decompress/huf_decompress.c
    decompress/zstd_ddict.c
    decompress/zstd_ddict.h
    decompress/zstd_decompress.c
    decompress/zstd_decompress_block.c
    decompress/zstd_decompress_block.h
    decompress/zstd_decompress_internal.h
    dictBuilder/cover.c
    dictBuilder/cover.h
    dictBuilder/divsufsort.c
    dictBuilder/divsufsort.h
    dictBuilder/fastcover.c
    dictBuilder/zdict.c
    zdict.h
    zstd.h
    zstd_errors.h
)

  set(ZSTD_DISABLE_ASM OFF)
  if(MSVC)
    set(ZSTD_DISABLE_ASM ON)
  else ()
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
      enable_language(ASM)
      set(ZSTD_SRC ${ZSTD_SRC} ${ZSTD_SRC_DIR}/decompress/huf_decompress_amd64.S)
    else()
      set(ZSTD_DISABLE_ASM ON)
    endif()
  endif()

  add_library(zstd EXCLUDE_FROM_ALL OBJECT ${ZSTD_SRC})
  set(ZSTD_INCLUDEDIR ${ZSTD_SRC_DIR})
  target_include_directories(zstd PRIVATE ${ZSTD_INCLUDEDIR})
  if(ZSTD_DISABLE_ASM)
    target_compile_definitions(zstd PRIVATE ZSTD_DISABLE_ASM)
  endif()

  set(ZSTD_DEP $<TARGET_OBJECTS:zstd>)
  set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDEDIR})
  set(ZSTD_LIBRARIES)

  list(APPEND TARGETS_DEP zstd)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZSTD DEFAULT_MSG ZSTD_INCLUDEDIR)
endif()
