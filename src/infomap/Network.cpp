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


#include "Network.h"
#include "../utils/FileURI.h"
#include "../io/convert.h"
#include "../io/SafeFile.h"
#include <cmath>
#include "../utils/Logger.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

using std::make_pair;

void Network::readFromFile(std::string filename)
{
	FileURI networkFilename(filename, false);
	std::string format = m_config.inputFormat;

	if (format == "")
	{
		std::string type = networkFilename.getExtension();
		if (type == "net")
			format = "pajek";
		else if (type == "txt")
			format = "link-list";
	}
	if (format == "")
		throw UnknownFileTypeError("No known input format specified or implied by file extension.");

	if (format == "pajek")
		parsePajekNetwork(filename);
	else if (format == "link-list")
		parseLinkList(filename);
	else
		throw UnknownFileTypeError("No known input format specified.");

	zoom();
}

void Network::parsePajekNetwork(std::string filename)
{
	if (m_config.parseWithoutIOStreams)
	{
		parsePajekNetworkCStyle(filename);
		return;
	}

	RELEASE_OUT("Parsing " << (m_config.isUndirected() ? "undirected" : "directed") << " network from file '" <<
			filename << "'... " << std::flush);
	string line;
	string buf;
	SafeInFile input(filename.c_str());

	std::getline(input,line);
	if (!input.good())
		throw FileFormatError("Can't read first line of pajek file.");

	unsigned int numNodes = 0;
	std::istringstream ss;
	ss.str(line);
	ss >> buf;
	if(buf == "*Vertices" || buf == "*vertices" || buf == "*VERTICES") {
		if (!(ss >> numNodes))
			throw BadConversionError(io::Str() << "Can't parse an integer after \"" << buf <<
					"\" as the number of nodes.");
	}
	else {
		throw FileFormatError("The first line (to lower cases) doesn't match *vertices.");
	}

	if (numNodes == 0)
		throw FileFormatError("The number of vertices cannot be zero.");

	unsigned int specifiedNumNodes = numNodes;
	bool checkNodeLimit = m_config.nodeLimit > 0;
	if (checkNodeLimit)
		numNodes = m_config.nodeLimit;

	m_numNodes = numNodes;
	m_nodeNames.resize(numNodes);
	m_nodeWeights.assign(numNodes, 1.0);
	m_sumNodeWeights = 0.0;

	char next = input.peek();
	if (next == '*') // Short pajek version (no nodes defined), set node number as name
	{
		for (unsigned int i = 0; i < numNodes; ++i)
		{
			m_nodeWeights[i] = 1.0;
			m_nodeNames[i] = io::stringify(i+1);
		}
		m_sumNodeWeights = numNodes * 1.0;
	}
	else
	{
		// Read node names, assuming order 1, 2, 3, ...
		for (unsigned int i = 0; i < numNodes; ++i)
		{
			unsigned int id = 0;
			if (!(input >> id) || id != static_cast<unsigned int>(i+1))
			{
				throw BadConversionError(io::Str() << "Couldn't parse node number " << (i+1) << " from line " << (i+2) << ".");
			}
			std::getline(input,line);
			unsigned int nameStart = line.find_first_of("\"");
			unsigned int nameEnd = line.find_last_of("\"");
			string name;
			double nodeWeight = 1.0;
			if(nameStart < nameEnd) {
				name = string(line.begin() + nameStart + 1, line.begin() + nameEnd);
				line = line.substr(nameEnd + 1);
				ss.clear();
				ss.str(line);
			}
			else {
				ss.clear();
				ss.str(line);
				ss >> buf; // Take away the index from the stream
				ss >> name; // Extract the next token as the name assuming no spaces
			}
			ss >> nodeWeight; // Extract the next token as node weight. If failed, the old value (1.0) is kept.
			m_sumNodeWeights += nodeWeight;
			m_nodeWeights[i] = nodeWeight;
	//		m_nodeMap.insert(make_pair(name, i));
			m_nodeNames[i] = name;
		}

		if (m_config.nodeLimit > 0 && numNodes < specifiedNumNodes)
		{
			unsigned int surplus = specifiedNumNodes - numNodes;
			for (unsigned int i = 0; i < surplus; ++i)
				std::getline(input, line);
		}
	}

	// Read the number of links in the network
	getline(input, line);
	ss.clear();
	ss.str(line);
	ss >> buf;
	if(buf != "*Edges" && buf != "*edges" && buf != "*Arcs" && buf != "*arcs") {
		throw FileFormatError("The first line (to lower cases) after the nodes doesn't match *edges or *arcs.");
	}

	unsigned int numEdgeLines = 0;
	unsigned int numSkippedEdges = 0;
	unsigned int maxLinkEnd = 0;
	m_totalLinkWeight = 0.0;

	// Read links in format "from to weight", for example "1 3 2" (all integers) and each undirected link only ones (weight is optional).
	while(!std::getline(input, line).fail())
	{
		if (line.length() == 0)
			continue;
		++numEdgeLines;
		ss.clear();
		ss.str(line);
		unsigned int linkEnd1;
		if (!(ss >> linkEnd1))
		{
			throw BadConversionError(io::Str() << "Can't parse first integer of link number " <<
					numEdgeLines << " from line '" << line << "'");
		}
		unsigned int linkEnd2;
		if (!(ss >> linkEnd2))
			throw BadConversionError(io::Str() << "Can't parse second integer of link number " <<
					numEdgeLines << " from line '" << line << "'");

		if (checkNodeLimit && (linkEnd2 > numNodes || linkEnd1 > numNodes))
		{
			++numSkippedEdges;
			continue;
		}

		double linkWeight;
		if (!(ss >> linkWeight))
			linkWeight = 1.0;

		if (linkEnd2 == linkEnd1)
		{
			++m_numSelfLinks;
			if (!m_config.includeSelfLinks)
				continue;
			m_totalSelfLinkWeight += linkWeight;
		}
		else if (m_config.isUndirected() && linkEnd2 < linkEnd1) // minimize number of links
			std::swap(linkEnd1, linkEnd2);

		--linkEnd1; // Node numbering starts from 1 in the .net format
		--linkEnd2;

		maxLinkEnd = std::max(maxLinkEnd, std::max(linkEnd1, linkEnd2));

		++m_numLinks;
		m_totalLinkWeight += linkWeight;

		if (!addLink(linkEnd1, linkEnd2, linkWeight) && linkEnd1 == linkEnd2)
			--m_numSelfLinks;
	}

	if (m_links.empty())
		throw InputDomainError(io::Str() << "No links could be found!");

	unsigned int zeroMinusOne = 0;
	--zeroMinusOne;
	if (maxLinkEnd == zeroMinusOne)
		throw InputDomainError(io::Str() << "Integer overflow, be sure to use zero-based node numbering if the node numbers starts from zero.");
	if (maxLinkEnd >= numNodes)
		throw InputDomainError(io::Str() << "At least one link is defined with node numbers that exceeds the number of nodes.");

	m_numLinks -= m_numAggregatedLinks;
	//	unsigned int sumEdgesFound = m_links.size() + m_numSelfLinks + numDoubleLinks + numSkippedEdges;
	std::cout << "done! Found " << specifiedNumNodes << " nodes and " << (m_numLinks + numSkippedEdges) << " links. ";
//	std::cout << "Average node weight: " << (m_sumNodeWeights / numNodes) << ". ";
	if (m_config.nodeLimit > 0)
		std::cout << "Limiting network to " << numNodes << " nodes and " << m_numLinks << " links. ";
	if(m_numAggregatedLinks > 0)
		std::cout << m_numAggregatedLinks << " links was aggregated to existing links. ";
	if (m_numSelfLinks > 0 && !m_config.includeSelfLinks)
		std::cout << m_numSelfLinks << " self-links was ignored. ";
	std::cout << std::endl;

}

void Network::parseLinkList(std::string filename)
{
	if (m_config.parseWithoutIOStreams)
	{
		parseLinkListCStyle(filename);
		return;
	}

	string line;
	string buf;
	SafeInFile input(filename.c_str());
	std::cout << "Parsing " << (m_config.directed ? "directed" : "undirected") << " link list from file '" <<
			filename << "'... " << std::flush;

	std::istringstream ss;

	unsigned int numEdgeLines = 0;
	unsigned int numSkippedEdges = 0;
	m_totalLinkWeight = 0.0;
	unsigned int maxLinkEnd = 0;
	bool minLinkIsZero = false;
	bool checkNodeLimit = m_config.nodeLimit > 0;
	unsigned int nodeLimit = m_config.nodeLimit;
	unsigned int lowestNodeNumber = m_config.zeroBasedNodeNumbers ? 0 : 1;

	// Read links in format "from to weight", for example "1 3 2" (all integers) and each undirected link only ones (weight is optional).
	while(!std::getline(input, line).fail())
	{
		if (line.length() == 0 || line[0] == '#')
			continue;
		++numEdgeLines;
		ss.clear();
		ss.str(line);
		unsigned int linkEnd1;
		if (!(ss >> linkEnd1))
			throw BadConversionError(io::Str() << "Can't parse first integer of link number " <<
					numEdgeLines << " from line '" << line << "'");
		unsigned int linkEnd2;
		if (!(ss >> linkEnd2))
			throw BadConversionError(io::Str() << "Can't parse second integer of link number " <<
					numEdgeLines << " from line '" << line << "'");

		linkEnd1 -= lowestNodeNumber;
		linkEnd2 -= lowestNodeNumber;

		if (checkNodeLimit && (linkEnd2 >= nodeLimit || linkEnd1 >= nodeLimit))
		{
			++numSkippedEdges;
			continue;
		}

		double linkWeight = 1.0;
		ss >> linkWeight;

		if (linkEnd2 == linkEnd1)
		{
			++m_numSelfLinks;
			if (!m_config.includeSelfLinks)
				continue;
			m_totalSelfLinkWeight += linkWeight;
		}
		else if (m_config.isUndirected() && linkEnd2 < linkEnd1) // minimize number of links
			std::swap(linkEnd1, linkEnd2);

		maxLinkEnd = std::max(maxLinkEnd, std::max(linkEnd1, linkEnd2));
		if (!minLinkIsZero && (linkEnd1 == 0 || linkEnd2 == 0))
			minLinkIsZero = true;

		++m_numLinks;
		m_totalLinkWeight += linkWeight;

		if (!addLink(linkEnd1, linkEnd2, linkWeight) && linkEnd1 == linkEnd2)
			--m_numSelfLinks;


	}

	if (m_links.empty())
		throw InputDomainError(io::Str() << "No links could be found!");

	unsigned int zeroMinusOne = 0;
	--zeroMinusOne;
	if (maxLinkEnd == zeroMinusOne)
		throw InputDomainError(io::Str() << "Integer overflow, be sure to use zero-based node numbering if the node numbers starts from zero.");


	m_numLinks -= m_numAggregatedLinks;
	m_numNodes = maxLinkEnd + 1;

	m_nodeNames.resize(m_numNodes);
	m_nodeWeights.assign(m_numNodes, 1.0);
	m_sumNodeWeights = 1.0 * m_numNodes;

	for (unsigned int i = 0; i < m_numNodes; ++i)
	{
//		m_nodeMap.insert(make_pair(io::stringify(i + 1), i));
		m_nodeNames[i] = io::stringify(i+1);
	}

	std::cout << "done! Found " << m_numNodes << " nodes and " << m_links.size() << " links." << std::endl;
	if (!minLinkIsZero)
		std::cout << "(Warning: minimum link index is not zero, check that you don't use zero based numbering if it's not true.)\n";
}


void Network::parsePajekNetworkCStyle(std::string filename)
{
	RELEASE_OUT("Parsing " << (m_config.isUndirected() ? "undirected" : "directed") << " network from file '" <<
				filename << "' (without iostreams)... " << std::flush);
	FILE* file;
	file = fopen(filename.c_str(), "r");
	if (file == NULL)
		throw FileOpenError(io::Str() << "Error opening file '" << filename << "'");

	const int LINELENGTH = 511;
	char line[LINELENGTH];
	char *cptr;

	unsigned int numNodes = 0;
	while (numNodes == 0)
	{
		if (fgets(line , LINELENGTH , file) == NULL)
			throw FileFormatError("Can't find a correct line that defines the beginning of the node section.");
		if (strncmp(line, "*", 1) != 0)
			continue;

		cptr = strchr(line, ' ');
		if (cptr == NULL)
			throw FileFormatError("Can't find a correct line that defines the beginning of the node section.");
		numNodes = atoi(++cptr);
	}

	unsigned int specifiedNumNodes = numNodes;
	bool checkNodeLimit = m_config.nodeLimit > 0;
	if (checkNodeLimit)
		numNodes = m_config.nodeLimit;

	m_numNodes = numNodes;
	m_nodeNames.resize(numNodes);
	m_nodeWeights.assign(numNodes, 1.0);
	m_sumNodeWeights = 0.0;


	int next = fgetc(file);
	ungetc(next, file);
	if (next == '*') // Short pajek version (no nodes defined), set node number as name
	{
		char nameBuff[16];
		for (unsigned int i = 0; i < numNodes; ++i)
		{
			m_nodeWeights[i] = 1.0;
			snprintf(nameBuff, 16, "%d", i+1);
			m_nodeNames[i] = std::string(nameBuff);
		}
		m_sumNodeWeights = numNodes * 1.0;
	}
	else
	{
		char *first;
		char *last;
		// Read node names, assuming order 1, 2, 3, ...
		for(unsigned int i = 0; i < numNodes; ++i)
		{
			if (fgets (line , LINELENGTH , file) == NULL)
				throw FileFormatError("Can't read enough nodes.");

			first = strchr(line, '\"')+1;
			last = strrchr(line, '\"');
			if(last > first)
			{
				size_t len = (size_t)(last - first);
				m_nodeNames[i] = std::string(first, len);
			}
			else
			{
				throw FileFormatError(io::Str() << "Can't read \"name\" of node " << (i+1) << ".");
			}

			double nodeWeight = strtod(++last, NULL);
			if (nodeWeight < 1e-10)
				nodeWeight = 1.0;

			m_sumNodeWeights += nodeWeight;
			m_nodeWeights[i] = nodeWeight;
		}

		if (m_config.nodeLimit > 0 && numNodes < specifiedNumNodes)
		{
			unsigned int surplus = specifiedNumNodes - numNodes;
			for (unsigned int i = 0; i < surplus; ++i)
			{
				if (fgets (line , LINELENGTH , file) == NULL)
					throw FileFormatError("The specified number of nodes is more than the number of lines that can be read.");
			}
		}
	}


	// Read the number of links in the network
	unsigned int numEdgeLines = 0;
	unsigned int numSkippedEdges = 0;
	unsigned int maxLinkEnd = 0;
	m_totalLinkWeight = 0.0;

	if (fgets (line , LINELENGTH , file) == NULL)
		throw FileFormatError("Can't find a correct line that defines the beginning of the edge section.");
	if (strncmp(line, "*", 1) != 0)
		throw FileFormatError("Can't find a correct line that defines the beginning of the edge section.");

	// Read links in format "from to weight", for example "1 3 2" (all integers) and each undirected link only ones (weight is optional).
	while(fgets (line , LINELENGTH , file) != NULL)
	{
		++numEdgeLines;
		unsigned int linkEnd1, linkEnd2;
		double linkWeight;
		// Parse the line
		if (!parseEdgeCStyle(line, linkEnd1, linkEnd2, linkWeight))
			throw FileFormatError(io::Str() << "Can't parse correct edge data from edge number " << numEdgeLines);



		if (checkNodeLimit && (linkEnd2 > numNodes || linkEnd1 > numNodes))
		{
			++numSkippedEdges;
			continue;
		}

		--linkEnd1; // Node numbering starts from 1 in the .net format
		--linkEnd2;

		maxLinkEnd = std::max(maxLinkEnd, std::max(linkEnd1, linkEnd2));

		if (linkEnd2 == linkEnd1)
		{
			++m_numSelfLinks;
			if (!m_config.includeSelfLinks)
				continue;
			m_totalSelfLinkWeight += linkWeight;
		}
		else if (m_config.isUndirected() && linkEnd2 < linkEnd1) // minimize number of links
			std::swap(linkEnd1, linkEnd2);

		++m_numLinks;
		m_totalLinkWeight += linkWeight;

		if (!addLink(linkEnd1, linkEnd2, linkWeight) && linkEnd1 == linkEnd2)
			--m_numSelfLinks;
	}

	if (m_links.empty())
		throw InputDomainError(io::Str() << "No links could be found!");

	unsigned int zeroMinusOne = 0;
	--zeroMinusOne;
	if (maxLinkEnd == zeroMinusOne)
		throw InputDomainError(io::Str() << "Integer overflow, be sure to use zero-based node numbering if the node numbers starts from zero.");
	if (maxLinkEnd >= numNodes)
		throw InputDomainError(io::Str() << "At least one link is defined with node numbers that exceeds the number of nodes.");

	m_numLinks -= m_numAggregatedLinks;
	//	unsigned int sumEdgesFound = m_links.size() + m_numSelfLinks + numDoubleLinks + numSkippedEdges;
	std::cout << "done! Found " << specifiedNumNodes << " nodes and " << (m_numLinks + numSkippedEdges) << " links. ";
//	std::cout << "Average node weight: " << (m_sumNodeWeights / numNodes) << ". ";
	if (m_config.nodeLimit > 0)
		std::cout << "Limiting network to " << numNodes << " nodes and " << m_numLinks << " links. ";
	if(m_numAggregatedLinks > 0)
		std::cout << m_numAggregatedLinks << " links was aggregated to existing links. ";
	if (m_numSelfLinks > 0 && !m_config.includeSelfLinks)
		std::cout << m_numSelfLinks << " self-links was ignored. ";
	std::cout << std::endl;
}

bool Network::parseEdgeCStyle(char line[], unsigned int& sourceIndex, unsigned int& targetIndex, double& weight)
{
	char *cptr;
	cptr = strtok(line, " \t"); // Get first non-whitespace character position
	if (cptr == NULL)
		return false;
	sourceIndex = atoi(cptr); // get first connected node
	cptr = strtok(NULL, " \t"); // Get second non-whitespace character position
	if (cptr == NULL)
		return false;
	targetIndex = atoi(cptr); // get the second connected node
	cptr = strtok(NULL, " \t"); // Get third non-whitespace character position
	if (cptr != NULL)
		weight = atof(cptr); // get the link weight
	else
		weight = 1.0;

	return true;
}

void Network::parseLinkListCStyle(std::string filename)
{
	RELEASE_OUT("Parsing " << (m_config.isUndirected() ? "undirected" : "directed") << " link list from file '" <<
					filename << "' (without iostreams)... " << std::flush);
	FILE* file;
	file = fopen(filename.c_str(), "r");
	if (file == NULL)
		throw FileOpenError(io::Str() << "Error opening file '" << filename << "'");

	const int LINELENGTH = 63;
	char line[LINELENGTH];


	unsigned int numEdgeLines = 0;
	unsigned int numSkippedEdges = 0;
	m_totalLinkWeight = 0.0;
	unsigned int maxLinkEnd = 0;
	bool minLinkIsZero = false;
	bool checkNodeLimit = m_config.nodeLimit > 0;
	unsigned int nodeLimit = m_config.nodeLimit;
	unsigned int lowestNodeNumber = m_config.zeroBasedNodeNumbers ? 0 : 1;

	// Read links in format "from to weight", for example "1 3 2" (all integers) and each undirected link only ones (weight is optional).
	while(fgets (line , LINELENGTH , file) != NULL)
	{
		if (*line == '#')
			continue;
		++numEdgeLines;
		unsigned int linkEnd1, linkEnd2;
		double linkWeight;
		if (!parseEdgeCStyle(line, linkEnd1, linkEnd2, linkWeight))
			throw FileFormatError("Can't parse edge data.");


		linkEnd1 -= lowestNodeNumber;
		linkEnd2 -= lowestNodeNumber;

		if (checkNodeLimit && (linkEnd2 >= nodeLimit || linkEnd1 >= nodeLimit))
		{
			++numSkippedEdges;
			continue;
		}

		maxLinkEnd = std::max(maxLinkEnd, std::max(linkEnd1, linkEnd2));
		if (!minLinkIsZero && (linkEnd1 == 0 || linkEnd2 == 0))
			minLinkIsZero = true;

		if (linkEnd2 == linkEnd1)
		{
			++m_numSelfLinks;
			if (!m_config.includeSelfLinks)
				continue;
			m_totalSelfLinkWeight += linkWeight;
		}
		else if (m_config.isUndirected() && linkEnd2 < linkEnd1) // minimize number of links
			std::swap(linkEnd1, linkEnd2);

		++m_numLinks;
		m_totalLinkWeight += linkWeight;

		if (!addLink(linkEnd1, linkEnd2, linkWeight) && linkEnd1 == linkEnd2)
			--m_numSelfLinks;
	}

	if (m_links.size() == 0)
		throw InputDomainError(io::Str() << "No links could be found!");

	unsigned int zeroMinusOne = 0;
	--zeroMinusOne;
	if (maxLinkEnd == zeroMinusOne)
		throw InputDomainError(io::Str() << "Integer overflow, be sure to use zero-based node numbering if the node numbers starts from zero.");


	m_numLinks -= m_numAggregatedLinks;
	m_numNodes = maxLinkEnd + 1;

	m_nodeNames.resize(m_numNodes);
	m_nodeWeights.assign(m_numNodes, 1.0);
	m_sumNodeWeights = 1.0 * m_numNodes;

	for (unsigned int i = 0; i < m_numNodes; ++i)
	{
		int length = snprintf(line, LINELENGTH, "%d", i+1);
		m_nodeNames[i] = std::string(line, length);
	}

	std::cout << "done! Found " << m_numNodes << " nodes and " << m_numLinks << " links." << std::endl;
	if (!minLinkIsZero)
		std::cout << "(Warning: minimum link index is not zero, check that you don't use zero based numbering if it's not true.)\n";
}

void Network::zoom()
{
	double selfProb = m_config.selfTeleportationProbability;
	bool addSelfLinks = selfProb > 0 && selfProb < 1;
	if (!addSelfLinks)
		return;

	unsigned int numNodes = m_numNodes;
	std::vector<unsigned int> nodeOutDegree(numNodes, 0);
	std::vector<double> sumLinkOutWeight(numNodes, 0.0);
	std::map<unsigned int, double> dummy;
	std::vector<std::map<unsigned int, double>::iterator> existingSelfLinks(numNodes, dummy.end());

	for (LinkMap::iterator linkIt(m_links.begin()); linkIt != m_links.end(); ++linkIt)
	{
		unsigned int linkEnd1 = linkIt->first;
		std::map<unsigned int, double>& subLinks = linkIt->second;
		for (std::map<unsigned int, double>::iterator subIt(subLinks.begin()); subIt != subLinks.end(); ++subIt)
		{
			unsigned int linkEnd2 = subIt->first;
			double linkWeight = subIt->second;
			++nodeOutDegree[linkEnd1];
			if (linkEnd1 == linkEnd2)
			{
				// Store existing self-link to aggregate additional weight
				existingSelfLinks[linkEnd1] = subIt;
				sumLinkOutWeight[linkEnd1] += linkWeight;
			}
			else
			{
				if (m_config.isUndirected())
				{
					sumLinkOutWeight[linkEnd1] += linkWeight * 0.5;
					sumLinkOutWeight[linkEnd2] += linkWeight * 0.5;
					++nodeOutDegree[linkEnd2];
				}
				else
				{
					sumLinkOutWeight[linkEnd1] += linkWeight;
				}
			}
		}
	}

	double sumAdditionalLinkWeight = 0.0;
	unsigned int numAdditionalLinks = 0;
	for (unsigned int i = 0; i < numNodes; ++i)
	{
		if (nodeOutDegree[i] == 0)
			continue; // Skip dangling nodes at the moment

		double selfLinkWeight = sumLinkOutWeight[i] * selfProb / (1.0 - selfProb);

		if (existingSelfLinks[i] != dummy.end()) {
			existingSelfLinks[i]->second += selfLinkWeight;
		}
		else {
			m_links[i].insert(std::make_pair(i, selfLinkWeight));
			++numAdditionalLinks;
		}
		sumAdditionalLinkWeight += selfLinkWeight;
	}

	m_numLinks += numAdditionalLinks;
	m_totalLinkWeight += sumAdditionalLinkWeight;
	m_numSelfLinks += numAdditionalLinks;
	m_totalSelfLinkWeight += sumAdditionalLinkWeight;

	std::cout << "(Added " << numAdditionalLinks << " self-links. " << (m_totalSelfLinkWeight / m_totalLinkWeight * 100) << "% of the total link weight is now from self-links.)\n";
}

void Network::printNetworkAsPajek(std::string filename)
{
	SafeOutFile out(filename.c_str());

	out << "*Vertices " << m_numNodes << "\n";
	for (unsigned int i = 0; i < m_numNodes; ++i)
		out << (i+1) << " \"" << m_nodeNames[i] << "\"\n";

	out << (m_config.isUndirected() ? "*Edges " : "*Arcs ") << m_links.size() << "\n";
	for (LinkMap::const_iterator linkIt(m_links.begin()); linkIt != m_links.end(); ++linkIt)
	{
		unsigned int linkEnd1 = linkIt->first;
		const std::map<unsigned int, double>& subLinks = linkIt->second;
		for (std::map<unsigned int, double>::const_iterator subIt(subLinks.begin()); subIt != subLinks.end(); ++subIt)
		{
			unsigned int linkEnd2 = subIt->first;
			double linkWeight = subIt->second;
			out << (linkEnd1 + 1) << " " << (linkEnd2 + 1) << " " << linkWeight << "\n";
		}
	}
}

bool Network::addLink(unsigned int n1, unsigned int n2, double weight)
{
	// Aggregate link weights if they are definied more than once
	LinkMap::iterator firstIt = m_links.lower_bound(n1);
	if (firstIt != m_links.end() && firstIt->first == n1) // First linkEnd already exists, check second linkEnd
	{
		std::pair<std::map<unsigned int, double>::iterator, bool> ret2 = firstIt->second.insert(std::make_pair(n2, weight));
		if (!ret2.second)
		{
			ret2.first->second += weight;
			++m_numAggregatedLinks;
			return false;
		}
	}
	else
	{
		m_links.insert(firstIt, std::make_pair(n1, std::map<unsigned int, double>()))->second.insert(std::make_pair(n2, weight));
	}

	++m_numLinks;
	return true;
}




