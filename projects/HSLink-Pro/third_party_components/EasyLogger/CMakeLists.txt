sdk_compile_definitions(_ELOG_CFG_H_) # avoid use config in lib

sdk_inc(EasyLogger/easylogger/inc)
sdk_src(EasyLogger/easylogger/src/elog.c)
sdk_src(EasyLogger/easylogger/src/elog_buf.c)
sdk_src(EasyLogger/easylogger/src/elog_utils.c)
sdk_src(EasyLogger/easylogger/src/elog_async.c)