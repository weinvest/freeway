#pragma once

#include <iostream>
using WorkflowID_t = int64_t;
using WorkerID_t = int32_t;

// \033[显示方式;前景色;背景色m
//#define ESC "\033["
//#define HIGHLIGHT "1;"
//#define BLACK "30m"
//#define RED "31m"
//#define GREEN "32m"
//#define YELLOW "33m"
//#define PURPLE "35m"
//#define WHITE "37m"
//
//
//#define ERROR ESC HIGHLIGHT RED
//#define DEBUG ESC WHITE
//#define INFO ESC HIGHLIGHT GREEN
//#define WARN ESC HIGHLIGHT YELLOW
//
//#define DEFAULT ESC "0m"
//
//#if 0
//#define LOG_DEBUG(x, msg) do {\
//    std::cerr<<DEBUG<<msg<<DEFAULT<<"\n";\
//}while(0)
//
//
//#define LOG_INFO(x, msg) std::cout<<INFO<<msg<<DEFAULT<<"\n";
//#define LOG_ERROR(x, msg)  std::cerr<<ERROR<<msg<<DEFAULT<<"\n";
//#else
//#define LOG_DEBUG(f, msg)
//#define LOG_INFO(f, msg)
//#define LOG_ERROR(f, msg)
//#endif
//#if 1
//#define LOG_DEBUG(msg, args...) printf(DEBUG msg DEFAULT"\n", args)
//#define LOG_INFO(msg, args...) printf(INFO msg DEFAULT"\n", args)
//#define LOG_WARN(msg, args...) printf(WARN msg DEFAULT"\n", args)
//#define LOG_ERROR(msg, args...)  printf(ERROR msg DEFAULT"\n", args)
//#else
//#define LOG_DEBUG(msg, args...)  do {} while(0)
//#define LOG_INFO(msg, args...)  do {} while(0)
//#define LOG_WARN(msg, args...) do {} while(0)
//#define LOG_ERROR(msg, args...)  do {} while(0)
//#endif

