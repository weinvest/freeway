#!/bin/python
import os
import sys

waittingList = {}

for worker in range(1, 128):
    workerFileName = 'Worker%d.waiting'
    if not os.path.exists(workerFileName):
        break

    workerFile = open(workerFileName, 'r')
    for l in workerFile:
        l = l.strip()
        if 0 == len(l):
            continue

        if '[' == l[0]:
            node =
            nodeList = []

