#!/usr/bin/env python
# import os, sys
# sys.path.append(os.path.abspath('infomap'))

from infomap import infomap

conf = infomap.init("--two-level -v -N2")
# Add output directory (and output name) to automatically write result to file
# conf = infomap.init("--two-level -v -N2 . --out-name test")

print "Creating network..."
network = infomap.Network(conf)

names = list("ABCDEF")
network.addNodes(names)

network.addLink(0, 1)
network.addLink(0, 2)
network.addLink(0, 3)
network.addLink(1, 0)
network.addLink(1, 2)
network.addLink(2, 1)
network.addLink(2, 0)
network.addLink(3, 0)
network.addLink(3, 4)
network.addLink(3, 5)
network.addLink(4, 3)
network.addLink(4, 5)
network.addLink(5, 4)
network.addLink(5, 3)

print "Num links:", network.numLinks()

network.finalizeAndCheckNetwork()

tree = infomap.HierarchicalNetwork(conf)

infomap.run(network, tree)

print "Found %d top modules with codelength: %f" % (tree.numTopModules(), tree.codelength())

communities = {}
clusterIndexLevel = 1 # 1, 2 or 3 for top, second and lowest cluster level
print "Tree:"
for node in tree.treeIter(clusterIndexLevel):
	print node.clusterIndex(), "  " * node.depth(), node.data.flow, '"%s"' % node.data.name
	if node.isLeafNode():
	    communities[node.originalLeafIndex] = node.clusterIndex()

print "Communities:", communities

print "Done!"
