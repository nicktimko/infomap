/**********************************************************************************

 Infomap software package for multi-level network clustering

 Copyright (c) 2013 Daniel Edler, Martin Rosvall

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


#ifndef MEMNETWORK_H_
#define MEMNETWORK_H_

#include "Network.h"

#include <map>
#include <utility>

#include "../io/Config.h"
#include "../utils/types.h"
using std::map;
using std::pair;

struct M2Node
{
	unsigned int phys1;
	unsigned int phys2;
	M2Node() :
		phys1(0), phys2(0)
	{}
	M2Node(unsigned int phys1, unsigned int phys2) :
		phys1(phys1), phys2(phys2)
	{}
	M2Node(const M2Node& other) :
		phys1(other.phys1), phys2(other.phys2)
	{}

	bool operator<(M2Node other) const
	{
		return phys1 == other.phys1 ? phys2 < other.phys2 : phys1 < other.phys1;
	}
	friend std::ostream& operator<<(std::ostream& out, const M2Node& node)
	{
		return out << "(" << node.phys1 << "-" << node.phys2 << ")";
	}
};

class MemNetwork: public Network
{
public:
	typedef map<pair<M2Node, M2Node>, double> M2LinkMap;
	typedef map<M2Node, unsigned int> M2NodeMap;

	MemNetwork(const Config& config) :
		Network(config),
		m_totM2NodeWeight(0.0),
		m_totM2LinkWeight(0.0),
		m_numMemorySelfLinks(0),
		m_totalMemorySelfLinkWeight(0.0)
	{}
	virtual ~MemNetwork() {}

	virtual void readFromFile(std::string filename);

	unsigned int numM2Nodes() const { return m_m2Nodes.size(); }
	const M2LinkMap& m2LinkMap() const { return m_m2Links; }
	const M2NodeMap& m2NodeMap() const { return m_m2NodeMap; }
	const std::vector<double>& m2NodeWeights() const { return m_m2NodeWeights; }
	double totalM2NodeWeight() const { return m_totM2NodeWeight; }
	double totalM2LinkWeight() const { return m_totM2LinkWeight; }
	double totalMemorySelfLinkWeight() const { return m_totalMemorySelfLinkWeight; }

protected:

	void parseTrigram(std::string filename);
	/**
	 * Create trigrams from first order data by chaining pair of links
	 * with the same connection node, respecting the direction, for example
	 * {n1 n2} and {n2 n3}, but not {n1 n2} and {n3 n2}.
	 *
	 * Example of ordinary network:
	 * n1 n2 w12
	 * n1 n3 w13
	 * n2 n3 w23
	 * n2 n4 w24
	 * n3 n4 w34
	 *
	 * Its corresponding trigram:
	 * n1 n2 n3 w23
	 * n1 n2 n4 w24
	 * n1 n3 n4 w34
	 * n2 n3 n4 w34
	 */
	void simulateMemoryFromOrdinaryNetwork();

	map<M2Node, double> m_m2Nodes;
	M2LinkMap m_m2Links; // Raw data from file
	M2NodeMap m_m2NodeMap;
	std::vector<double> m_m2NodeWeights;
	double m_totM2NodeWeight;
	double m_totM2LinkWeight;
	unsigned int m_numMemorySelfLinks;
	double m_totalMemorySelfLinkWeight;

};

#endif /* MEMNETWORK_H_ */
