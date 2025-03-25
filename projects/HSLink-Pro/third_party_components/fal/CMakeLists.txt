
sdk_compile_definitions(-DFAL_DEBUG=1)
sdk_inc(fal/inc)
sdk_src(fal/src/fal.c)
sdk_src(fal/src/fal_flash.c)
sdk_src(fal/src/fal_partition.c)