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
    return true;
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