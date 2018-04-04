//
// Created by shugan.li on 18-4-4.
//

#include <iostream>
#include "Node.h"

std::ostream& operator<< (std::ostream& out, const Waiter& n)
{
    switch (n.state)
    {
        case Waiter::Waitting:
            out << n.name << "[label={<n>" << n.name << "[" << n.workflowId << "]|" << n.worker << "|<p>},"
                << "style=filled, color=grey\"]";
            break;
        case Waiter::Doing:
            out << n.name << "[label={<n>" << n.name << "[" << n.workflowId << "]|" << n.worker << "|<p>},"
                << "style=filled, color=lightblue]";
            break;
        case Waiter::Done:
            out << n.name << "[label={<n>" << n.name << "[" << n.workflowId << "]|" << n.worker << "|<p>},"
                << "style=filled, color=green]";
            break;
        default:
            break;
    }

    return out;
}

std::ostream& operator<< (std::ostream& out, const Node& n)
{
    std::string prevNode("node0:"+n.name);
    for(int32_t iWaiter = 0; iWaiter < n.waiters.size(); ++iWaiter)
    {
        auto& waiter = n.waiters[iWaiter];
        if(Waiter::Done != waiter.state || ((iWaiter != (n.waiters.size()-1)) && (Waiter::Done != n.waiters[iWaiter].state))) {
            out << waiter << "\n";
            out << prevNode << "->" << waiter.name << ":n\n";
            prevNode = waiter.name + ":p";
        }
    }

    return out;
}

std::ostream& operator<< (std::ostream& out, const Graph& g)
{
    auto& nodes = g.nodes;
    out << "digraph g{\n";
    out << " nodesep = 0.05;\n";
    out << " rankdir = LR;\n";
    out << " node[shape=record, width= .1, height = .1];\n";
    out << " node0[label=\"";
    for(auto& n : nodes)
    {
        out << "<" << n.name << ">|";
    }
    out << "\", height = 2.5];";

    for(auto& n : nodes)
    {
        out << n << "\n";
    }

    out << "}";

    return out;
}

Node* Graph::AddNode(const std::string& name)
{
    auto itNode = nodeMapping.find(name);
    if(nodeMapping.end() == itNode) {
        nodes.emplace_back(name);
        auto pNode = &nodes.back();
        nodeMapping[name] = pNode;
        return pNode;
    }

    return itNode->second;
}

void Graph::AddEdge(const std::string& from, const std::string& to)
{
    auto pFromNode = AddNode(from);
    auto pToNode = AddNode(to);
    pToNode->precessors.push_back(pFromNode);
}

void Graph::Dispatch(const std::string& name, int32_t worker, int32_t workflowId)
{
    auto pNode = GetNode(name);
    if(nullptr == pNode)
    {
        std::cerr << __FUNCTION__ << ":" << __LINE__ << " can't find node:" << name
                  << ",worker:" << worker << "workflowId:" << workflowId << "\n";
    }
    else
    {
        pNode->waiters.emplace_back(name, worker, workflowId);
        for(auto pPreNode : pNode->precessors)
        {
            pPreNode->waiters.emplace_back(name, worker, workflowId);
        }
    }
}