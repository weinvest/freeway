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
	Trying,
        Doing,
        Done
    };
    Waiter(const std::string& n, const std::string& wa, int32_t w, int32_t wf)
            :name(n), waited(wa), worker(w), workflowId(wf){
    }
    Waiter(const Waiter&) = delete;
    Waiter(const Waiter&&) = delete;

    std::string GetDisplayName( void ) const ;

    std::string name;
    std::string waited;
    int32_t worker;
    int32_t workflowId;
    WaitState state{Waitting};

    friend std::ostream& operator<< (std::ostream& out, const Waiter& n);
};

struct Node {
    Node(const std::string& n): name(n) {}

    Node(const Node&) = delete;
    Node(const Node&&) = delete;
    std::string name;
    std::vector<Waiter*> waiters;
    std::vector<Node*> precessors;
    std::unordered_map<std::string, Waiter*> waiterMapping;

    Waiter* GetWaiter(const std::string& n, int32_t workflowId);
    Waiter* AddWaiter(const std::string& n, int32_t worker, int32_t workflowId);

    friend std::ostream& operator<< (std::ostream& out, const Node& n);
};

struct Graph{
    Graph( void ) = default;
    Graph(const Graph&) = delete;
    Graph(const Graph&&) = delete;

    std::vector<Node*> nodes;
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

    void TryLock(const std::string& node, int32_t workflowId);
    void Lock(const std::string& node, int32_t workflowId);
    void Unlock(const std::string& node, int32_t workflowId);

    void TryShared(const std::string& whoLock, int32_t workflowId, const std::string& waitedNode);
    void LockShared(const std::string& whoLock, int32_t workflowId, const std::string& waitedNode);
    void UnlockShared(const std::string& whoLock, int32_t workflowId, const std::string& waitedNode);

    friend std::ostream& operator<< (std::ostream& out, const Graph& g);
};
#endif //FREEWAY_NODE_H
