
sdk_compile_definitions(-D_FDB_CFG_H_)
sdk_inc(FlashDB/inc)
sdk_src(FlashDB/src/fdb.c)
sdk_src(FlashDB/src/fdb_kvdb.c)
sdk_src(FlashDB/src/fdb_file.c)
sdk_src(FlashDB/src/fdb_tsdb.c)
sdk_src(FlashDB/src/fdb_utils.c)