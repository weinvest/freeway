#include "Node.h"
#include <fstream>
#include <regex>
#include <boost/algorithm/string/split.hpp>
#include <iostream>

void BuildGraph(Graph& g, std::ifstream& o)
{
    std::string line;
    while(std::getline(o, line)){
        if(line.npos != line.find("{")){
            break;
        }
    }

    while(std::getline(o, line))
    {
        if(!line.empty() && line.npos == line.find("}")){
            std::vector<std::string> r;
            boost::split(r, line, [](char c) {
                             return std::isspace(c) || c == '-' || c == '>' || c == ';';
                         }
                    , boost::token_compress_on);
            assert(2 == r.size());
            g.AddEdge(r[0], r[1]);
        }
    }
}

void ConstructWaitGraph(Graph& g, std::ifstream& d)
{
    std::regex r("push node:(\\w+) to Worker-(\\d+) @workflow:(\\d+)");
    std::string line;
    std::smatch sm;
    while(std::getline(d, line))
    {
        if(std::regex_match(line, sm, r)){
            g.Dispatch(sm[0], std::stoi(sm[1]), std::stoi(sm[2]));
        }
    }
}

bool ParseWorkLogFile(Graph& g, std::ifstream& w)
{
    const std::string GET_LOCK_TEXT("get lock for node:");
    const std::string REL_LOCK_TEXT("unlocked node:");
    const std::string GET_SHARED_LOCK_TEXT("get shared lock for node:");
    const std::string REL_SHARED_LOCK_TEXT("unlock shared lock node:");

    bool isEmpty = true;
    std::string line;
    auto getWaiterInfo = [&line](int32_t from, std::string& name, int32_t& workflowId)
    {

    };

    std::string waiterName;
    int32_t workflowId;
    std::string waitedName;
    while(std::getline(w, line))
    {
        isEmpty = false;
        auto iLockPos = line.find(GET_LOCK_TEXT);
        if(line.npos != iLockPos)
        {
            getWaiterInfo(iLockPos, waiterName, workflowId);
            g.Lock(waiterName, workflowId);
        }
        else
        {
            auto relLockPos = line.find(REL_LOCK_TEXT);
            if(line.npos != relLockPos) {
                getWaiterInfo(relLockPos, waitedName, workflowId);
                g.Unlock(waitedName, workflowId);
            }
            else
            {
                auto iSharedLockPos = line.find(GET_SHARED_LOCK_TEXT);
                if(line.npos != iSharedLockPos)
                {
                    getWaiterInfo(iSharedLockPos, waitedName, workflowId);

                    auto waitedNodePos = iSharedLockPos+GET_SHARED_LOCK_TEXT.length();
                    g.LockShared(waitedName, workflowId, line.substr(waitedNodePos, line.find(waitedNodePos, ':') - waitedNodePos));
                }
                else
                {
                    auto iRelSharedLockPos = line.find(REL_SHARED_LOCK_TEXT);
                    if(line.npos != iRelSharedLockPos)
                    {
                        getWaiterInfo(iRelSharedLockPos, waitedName, workflowId);

                        auto waitedNodePos = iRelSharedLockPos+REL_SHARED_LOCK_TEXT.length();
                        g.UnlockShared(waitedName, workflowId, line.substr(waitedNodePos, line.find(waitedNodePos, ':') - waitedNodePos));
                    }
                }
            }


        }

    }
    return !isEmpty;
}

int main(int32_t argc, char** argv){
    if(3 != argc)
    {
        std::cerr << "usage: blockAnalysis g.dot logdir\n";
        return 1;
    }

    std::ifstream graphFile(argv[1]);
    if(!graphFile)
    {
        std::cerr << "open dot graph file " << argv[1] << " failed\n";
        return 1;
    }

    Graph g;
    BuildGraph(g, graphFile);

    //解析Dispacher.log文件
    std::string dispathFileName(argv[2]);
    dispathFileName += "/Dispatcher.log";

    std::ifstream dispatchFile(dispathFileName);
    if(!dispatchFile){
        std::cerr << "open dispatcher log file " << dispathFileName << " failed\n";
    }
    ConstructWaitGraph(g, dispatchFile);

    //解析Worker\d.log文件
    std::string workerFilePrefix(argv[2]);
    workerFilePrefix += "/Worker";
    for(int32_t iWorker = 1; ; ++iWorker)
    {
        std::string workerFileName(workerFilePrefix + std::to_string(iWorker) + ".log");
        std::ifstream workerFile(workerFileName);
        if(!workerFile || !ParseWorkLogFile(g, workerFile))
        {
            break;
        }
    }

    //输出结果
    std::cout << g << std::endl;
}