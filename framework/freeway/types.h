#pragma once
using WorkflowID_t = uint64_t;
using WorkerID_t = int32_t;

// \033[显示方式;前景色;背景色m
#define ESC "\033["
#define HIGHLIGHT "1;"
#define RED "31m"
#define GREEN "32m"
#define YELLOW "33m"
#define PURPLE "35m"


#define ERROR ESC HIGHLIGHT RED
#define DEBUG ESC HIGHLIGHT GREEN
#define INFO ESC HIGHLIGHT YELLOW
#define WARN ESC HIGHLIGHT PURPLE

#define DEFAULT ESC "0m"

/*
#define LOG_DEBUG(mLog, msg) do {\
    std::cerr<<DEBUG<<msg<<DEFAULT<<std::endl;\
}while(0)


#define LOG_INFO(mLog, msg) std::cerr<<INFO<<msg<<DEFAULT<<std::endl;
#define LOG_ERROR(mLog, msg)  std::cerr<<ERROR<<msg<<DEFAULT<<std::endl;
*/

#if 0
#define LOG_DEBUG(msg, args...) printf(DEBUG msg DEFAULT"\n", args)
#define LOG_INFO(msg, args...) printf(INFO msg DEFAULT"\n", args)
#define LOG_WARN(msg, args...) printf(WARN msg DEFAULT"\n", args)
#define LOG_ERROR(msg, args...)  printf(ERROR msg DEFAULT"\n", args)
#else
#define LOG_DEBUG(msg, args...)  do {} while(0)
#define LOG_INFO(msg, args...)  do {} while(0)
#define LOG_WARN(msg, args...) do {} while(0)
#define LOG_ERROR(msg, args...)  do {} while(0)
#endif

