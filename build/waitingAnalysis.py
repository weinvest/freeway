#!/bin/python
import os
import sys

waittingList = {}

for worker in range(1, 128):
    node = None
    nodeList = []
    workerFileName = 'Worker%d.waiting' % worker
    if not os.path.exists(workerFileName):
        break

    workerFile = open(workerFileName, 'r')
    for l in workerFile:
        l = l.strip()
        if 0 == len(l):
            continue

        if '[' == l[0]:
            if node is not None:
                if waittingList.has_key(node):
                    waittingList[node].extend(nodeList)
                else:
                    waittingList[node] = nodeList
            node = l[1:-1].replace(':', '_')
            #print 'new node:' + node
            nodeList = []
        else:
            waitNode = l[l.index(':')+1 : l.index('|')]
            nodeList.append(waitNode.split(':'))
            #print nodeList

    if node is not None:
        if waittingList.has_key(node):
            waittingList[node].extend(nodeList)
            #print  waittingList[node]
        else:
            waittingList[node] = nodeList
            #print  waittingList[node]

node = ''
def node2Key(s):
    global node
    index = 0 if node == s[0] else ord(s[0])
    return int(s[1]) * 1000 + index

sortedWaitting = {}
for k, v in waittingList.items():
    node = k
    sortedWaitting[k] = sorted(v, key=node2Key)

w = open('waiting.dot', 'w')
w.write('''digraph g{
    nodesep = 0.05;
    rankdir = LR;
    node[shape=record, width= .2, height = .1];
    node0[label="''')

w.write('|'.join([ '<'+n+'>'+n for n in sortedWaitting.keys()]))
w.write('", height = 2.5];\n')

for k, v in sortedWaitting.items():
    dnode = 'node0:' + k
    nodeCount = 0
    for n in v:
        cnode = '_'.join(n)
        w.write('%s[label="{%s}",style=filled, color=white]\n' % (cnode, cnode))
        w.write(dnode + '->' + cnode + ':w\n')
        dnode = cnode + ':e'
        nodeCount += 1
        if nodeCount > 5:
            break

w.write('}\n')


