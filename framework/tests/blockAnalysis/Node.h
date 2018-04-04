//
// Created by shugan.li on 18-4-4.
//

#ifndef FREEWAY_NODE_H
#define FREEWAY_NODE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>

struct Waiter
{
    enum WaitState
    {
        Waitting,
        Doing,
        Done
    };
    Waiter(const std::string& n, int32_t w, int32_t wf)
            :name(n), worker(w), workflowId(wf){
    }

    std::string name;
    int32_t worker;
    int32_t workflowId;
    WaitState state;

    friend std::ostream& operator<< (std::ostream& out, const Waiter& n);
};

struct Node {
    Node(const std::string& n): name(n) {}

    std::string name;
    std::vector<Waiter> waiters;
    std::vector<Node*> precessors;
    std::unordered_map<std::string, Waiter*> waiterMapping;

    friend std::ostream& operator<< (std::ostream& out, const Node& n);
};

struct Graph{
    std::vector<Node> nodes;
    std::unordered_map<std::string, Node*> nodeMapping;
    Node* GetNode(const std::string& name) {
        auto itNode = nodeMapping.find(name);
        if(itNode == nodeMapping.end()){
            return nullptr;
        }

        return itNode->second;
    }

    Node* AddNode(const std::string& name);
    void AddEdge(const std::string& from, const std::string& to);
    void Dispatch(const std::string& name, int32_t worker, int32_t workflowId);

    friend std::ostream& operator<< (std::ostream& out, const Graph& g);
};
#endif //FREEWAY_NODE_H
