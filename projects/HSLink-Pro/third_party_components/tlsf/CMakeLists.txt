sdk_compile_definitions(-D_DEBUG=0)
sdk_app_inc(tlsf)
sdk_app_src(tlsf/tlsf.c)


sdk_ld_options(-Wl,--wrap=_malloc_r,--wrap=_free_r,--wrap=_realloc_r)