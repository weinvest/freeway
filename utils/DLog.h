#ifndef _DFC_LOG_H
#define _DFC_LOG_H
#include <cstdint>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
using namespace log4cplus;

#define LoadLogConf(cfg)  PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(cfg));

//#ifndef DEBUG
#if 1
#define TRACE(logger,msg)
#define LOG_INFO(logger,msg)
#define LOG_DEBUG(logger,msg)
#define LOG_WARN(logger,msg)
#define LOG_ERROR(logger,msg)
#define LOG_FATAL(logger,msg)

#else
#define TRACE(logger,msg) LOG4CPLUS_TRACE(logger,msg)
#define LOG_INFO(logger,msg) LOG4CPLUS_INFO(logger,msg)
#define LOG_DEBUG(logger,msg) LOG4CPLUS_DEBUG(logger,msg)
#define LOG_WARN(logger,msg)  LOG4CPLUS_WARN(logger,msg)
#define LOG_ERROR(logger,msg) LOG4CPLUS_ERROR(logger,msg)
#define LOG_FATAL(logger,msg) LOG4CPLUS_FATAL(logger,msg)
#endif
#endif
 
