/**********************************************************************************

 Infomap software package for multi-level network clustering

 Copyright (c) 2013, 2014 Daniel Edler, Martin Rosvall

 For more information, see <http://www.mapequation.org>


 This file is part of Infomap software package.

 Infomap software package is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Infomap software package is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Infomap software package.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************/

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include <Infomap.h>

using std::vector;
using std::string;

void printTreeHelper(std::ostream& out, infomap::SNode& node, std::string prefix = "")
{
	for (unsigned int i = 0; i < node.children.size(); ++i)
	{
		infomap::SNode& child = *node.children[i];
		if (child.isLeaf)
			out << prefix << (i+1) << " " << child.data.flow <<" \"" << child.data.name << "\" " << child.originalLeafIndex << "\n";
		else {
			std::ostringstream oss(prefix);
			oss << (i + 1) << ":";
			std::string subPrefix = oss.str();
			printTreeHelper(out, child, subPrefix);
		}
	}
}

void printTree(infomap::HierarchicalNetwork& hierarchicalNetwork)
{
	std::cout << "Result tree:\n";
	printTreeHelper(std::cout, hierarchicalNetwork.getRootNode());
}

int main(int argc, char** argv)
{
	infomap::Config config = infomap::init("--two-level -N2");

	infomap::Network network(config);

  	network.addLink(0, 1);
  	network.addLink(0, 2);
  	network.addLink(0, 3);
  	network.addLink(1, 0);
  	network.addLink(1, 2);
  	network.addLink(2, 1);
  	network.addLink(2, 0);
  	network.addLink(3, 0);
  	network.addLink(3, 4);
  	network.addLink(3, 5);
  	network.addLink(4, 3);
  	network.addLink(4, 5);
  	network.addLink(5, 4);
  	network.addLink(5, 3);

  	network.finalizeAndCheckNetwork();

  	infomap::HierarchicalNetwork resultNetwork(config);

  	infomap::run(network, resultNetwork);
  	
  	printTree(resultNetwork);
}
