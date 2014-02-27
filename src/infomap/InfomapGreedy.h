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


#ifndef INFOMAPGREEDY_H_
#define INFOMAPGREEDY_H_
#include "InfomapBase.h"
#include "Node.h"
#include "NodeFactory.h"
#include "flowData_traits.h"
#include "../utils/Logger.h"
#include "../utils/infomath.h"
#include <map>
#include <utility>
#include "../io/TreeDataWriter.h"
#include "../utils/FileURI.h"
#include "../io/SafeFile.h"
#include <sstream>
#include <iomanip>
#include "../io/convert.h"
#include "../io/HierarchicalNetwork.h"


struct DeltaFlow {
	DeltaFlow()
	:	module(0), deltaExit(0.0), deltaEnter(0.0), sumDeltaPlogpPhysFlow(0.0), sumPlogpPhysFlow(0.0) {}
	DeltaFlow(unsigned int module, double deltaExit, double deltaEnter, double sumDeltaPlogpPhysFlow, double sumPlogpPhysFlow)
	:	module(module), deltaExit(deltaExit), deltaEnter(deltaEnter), sumDeltaPlogpPhysFlow(sumDeltaPlogpPhysFlow), sumPlogpPhysFlow(sumPlogpPhysFlow) {}
	DeltaFlow(const DeltaFlow& other) // Copy constructor
	:	module(other.module), deltaExit(other.deltaExit), deltaEnter(other.deltaEnter), sumDeltaPlogpPhysFlow(other.sumDeltaPlogpPhysFlow), sumPlogpPhysFlow(other.sumPlogpPhysFlow) {}
	DeltaFlow& operator=(DeltaFlow other) // Assignment operator (copy-and-swap idiom)
	{
		swap(*this, other);
		return *this;
	}
	friend void swap(DeltaFlow& first, DeltaFlow& second)
	{
		std::swap(first.module, second.module);
		std::swap(first.deltaExit, second.deltaExit);
		std::swap(first.deltaEnter, second.deltaEnter);
		std::swap(first.sumDeltaPlogpPhysFlow, second.sumDeltaPlogpPhysFlow);
		std::swap(first.sumPlogpPhysFlow, second.sumPlogpPhysFlow);
	}
	unsigned int module;
	double deltaExit;
	double deltaEnter;
	double sumDeltaPlogpPhysFlow;
	double sumPlogpPhysFlow;
};

struct MemNodeSet
{
	MemNodeSet(unsigned int numMemNodes, double sumFlow) : numMemNodes(numMemNodes), sumFlow(sumFlow) {}
	unsigned int numMemNodes; // use counter to check for zero to avoid round-off errors in sumFlow
	double sumFlow;
};
typedef std::map<unsigned int, MemNodeSet>		ModuleToMemNodes;


template<typename InfomapImplementation>
class InfomapGreedy : public InfomapBase
{
public:
	typedef typename base_traits<InfomapImplementation>::flow_type		FlowType;
	typedef typename flowData_traits<FlowType>::detailed_balance_type 	DetailedBalanceType;
	typedef typename flowData_traits<FlowType>::directed_with_recorded_teleportation_type DirectedWithRecordedTeleportationType;
	typedef typename flowData_traits<FlowType>::teleportation_type 		TeleportationType;
	typedef MemNode<FlowType>											NodeType;
	typedef Edge<NodeBase>												EdgeType;

	InfomapGreedy(const Config& conf)
	:	InfomapBase(conf, new MemNodeFactory<FlowType>()),
	 	nodeFlow_log_nodeFlow(0.0),
		flow_log_flow(0.0),
		exit_log_exit(0.0),
		enter_log_enter(0.0),
		enterFlow(0.0),
		enterFlow_log_enterFlow(0.0),
		m_numPhysicalNodes(0),
	 	exitNetworkFlow(0.0),
	 	exitNetworkFlow_log_exitNetworkFlow(0.0)
	{
		FlowType& rootData = getNode(*root()).data;
		rootData.flow = 1.0;
		rootData.exitFlow = 0.0;
	}
	virtual ~InfomapGreedy() {}

protected:

	virtual std::auto_ptr<InfomapBase> getNewInfomapInstance();

//	virtual void initEnterExitFlow();

	virtual void initConstantInfomapTerms();

	virtual void initModuleOptimization();

	// Calculate initial codelength, specialized on detailed balance
	void calculateCodelengthFromActiveNetwork(DetailedBalance);
	void calculateCodelengthFromActiveNetwork(NoDetailedBalance);

	virtual unsigned int optimizeModules();

	// Teleportation methods - specialized implementation for InfomapDirected
	// void addTeleportationDeltaFlowOnOldModuleIfMove(NodeType& nodeToMove, DeltaFlow& oldModuleDeltaFlow) {}
	// void addTeleportationDeltaFlowOnNewModuleIfMove(NodeType& nodeToMove, DeltaFlow& newModuleDeltaFlow) {}
	// void addTeleportationDeltaFlowIfMove(NodeType& current, std::vector<DeltaFlow>& moduleDeltaExits, unsigned int numModuleLinks) {}

	// --- Helper methods ---
	// double getDeltaCodelength(NodeType& current, DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta);

	// void updateCodelength(NodeType& current, DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta);

	unsigned int tryMoveEachNodeIntoBestModule();

	virtual void moveNodesToPredefinedModules();

	virtual unsigned int consolidateModules(bool replaceExistingStructure, bool asSubModules);

	virtual void resetModuleFlowFromLeafNodes();

	virtual void resetModuleFlow(NodeBase& node);

	virtual double calcCodelengthFromFlowWithinOrExit(const NodeBase& parent);

	virtual double calcCodelengthFromEnterWithinOrExit(const NodeBase& parent);

	virtual void generateNetworkFromChildren(NodeBase& parent);

	virtual void transformNodeFlowToEnterFlow(NodeBase* parent);

	virtual void cloneFlowData(const NodeBase& source, NodeBase& target);

	virtual void printNodeRanks(std::ostream& out);

	virtual void printFlowNetwork(std::ostream& out);

	virtual void printMap(std::ostream& out);

	virtual void sortTree(NodeBase& parent);

	virtual void buildHierarchicalNetwork(HierarchicalNetwork& data, bool includeLinks);
	void buildHierarchicalNetworkHelper(HierarchicalNetwork& data, HierarchicalNetwork::node_type& parent, const TreeData& originalData, NodeBase* node = 0);
	virtual void printSubInfomapTree(std::ostream& out, const TreeData& originalData, const std::string& prefix);
	void printSubTree(std::ostream& out, NodeBase& module, const TreeData& originalData, const std::string& prefix);
	virtual void printSubInfomapTreeDebug(std::ostream& out, const TreeData& originalData, const std::string& prefix);
	void printSubTreeDebug(std::ostream& out, NodeBase& module, const TreeData& originalData, const std::string& prefix);


	NodeType& getNode(NodeBase& node);

	const NodeType& getNode(const NodeBase& node) const;

	unsigned int numActiveModules();
	virtual unsigned int numDynamicModules();

	virtual FlowDummy getNodeData(NodeBase& node);

	virtual std::vector<PhysData>& getPhysicalMembers(NodeBase& node);
	virtual M2Node& getPhysical(NodeBase& node);

	virtual void debugPrintInfomapTerms();

private:
	InfomapImplementation& getImpl();
	InfomapImplementation& getImpl(InfomapBase& infomap);

protected:
	std::vector<FlowType> m_moduleFlowData;
	std::vector<unsigned int> m_moduleMembers;
	std::vector<unsigned int> m_emptyModules;

	std::vector<ModuleToMemNodes> m_physToModuleToMemNodes; // vector[physicalNodeID] map<moduleID, {#memNodes, sumFlow}>  

	double nodeFlow_log_nodeFlow; // constant while the leaf network is the same
	double flow_log_flow; // node.(flow + exitFlow)
	double exit_log_exit;
	double enter_log_enter;
	double enterFlow;
	double enterFlow_log_enterFlow;

	unsigned int m_numPhysicalNodes;

	// For hierarchical
	double exitNetworkFlow;
	double exitNetworkFlow_log_exitNetworkFlow;

};


template<typename InfomapImplementation>
std::auto_ptr<InfomapBase> InfomapGreedy<InfomapImplementation>::getNewInfomapInstance()
{
	return std::auto_ptr<InfomapBase>(new InfomapImplementation(m_config));
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::initConstantInfomapTerms()
{
	// Not constant for memory Infomap!
	nodeFlow_log_nodeFlow = 0.0;
	// For each module
	for (activeNetwork_iterator it(m_activeNetwork.begin()), itEnd(m_activeNetwork.end());
			it != itEnd; ++it)
	{
		NodeType& node = getNode(**it);
		nodeFlow_log_nodeFlow += infomath::plogp(node.data.flow);
	}
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::initModuleOptimization()
{
	DEBUG_OUT("\n::initModuleOptimization()" << std::flush);
	unsigned int numNodes = m_activeNetwork.size();
	m_moduleFlowData.resize(numNodes);
	m_moduleMembers.assign(numNodes, 1);
	m_emptyModules.clear();
	m_emptyModules.reserve(numNodes);

	if (m_numPhysicalNodes == 0)
		m_numPhysicalNodes = numNodes;
	m_physToModuleToMemNodes.clear();
	m_physToModuleToMemNodes.resize(m_numPhysicalNodes);

	unsigned int i = 0;
	for (activeNetwork_iterator it(m_activeNetwork.begin()), itEnd(m_activeNetwork.end());
			it != itEnd; ++it, ++i)
	{
		NodeType& node = getNode(**it);
		node.index = i; // Unique module index for each node
		m_moduleFlowData[i] = node.data;

		unsigned int numPhysicalMembers = node.physicalNodes.size();
		for(unsigned int j = 0; j < numPhysicalMembers; ++j)
		{
			PhysData& physData = node.physicalNodes[j];
			m_physToModuleToMemNodes[physData.physNodeIndex].insert(m_physToModuleToMemNodes[physData.physNodeIndex].end(),
					std::make_pair(i, MemNodeSet(1, physData.sumFlowFromM2Node)));
		}
	}



	// Initiate codelength terms for the initial state of one module per node
	calculateCodelengthFromActiveNetwork(DetailedBalanceType());

//	m_numNonTrivialTopModules = numNodes;
}


/**
 * Specialized for the case when enter flow equals exit flow
 */
template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::calculateCodelengthFromActiveNetwork(DetailedBalance)
{
	exit_log_exit = 0.0;
//	enter_log_enter = 0.0;
	enterFlow = 0.0;
	flow_log_flow = 0.0;

	// For each module
	for (activeNetwork_iterator it(m_activeNetwork.begin()), itEnd(m_activeNetwork.end());
			it != itEnd; ++it)
	{
		NodeType& node = getNode(**it);
		// own node/module codebook
		flow_log_flow += infomath::plogp(node.data.flow + node.data.exitFlow);

		// use of index codebook
		enterFlow      += node.data.exitFlow;
//		enter_log_enter += infomath::plogp(node.data.enterFlow);
		exit_log_exit += infomath::plogp(node.data.exitFlow);
	}

	enterFlow += exitNetworkFlow;
	enterFlow_log_enterFlow = infomath::plogp(enterFlow);

	nodeFlow_log_nodeFlow = 0.0;
	for (unsigned int i = 0; i < m_numPhysicalNodes; ++i)
	{
		const ModuleToMemNodes& moduleToMemNodes = m_physToModuleToMemNodes[i];
		for (ModuleToMemNodes::const_iterator modToMemIt(moduleToMemNodes.begin()); modToMemIt != moduleToMemNodes.end(); ++modToMemIt)
			nodeFlow_log_nodeFlow += infomath::plogp(modToMemIt->second.sumFlow);
	}

	indexCodelength = enterFlow_log_enterFlow - exit_log_exit - exitNetworkFlow_log_exitNetworkFlow;
	moduleCodelength = -exit_log_exit + flow_log_flow - nodeFlow_log_nodeFlow;
	codelength = indexCodelength + moduleCodelength;
}

/**
 * Specialized for the case when enter and exit flow may differ
 */
template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::calculateCodelengthFromActiveNetwork(NoDetailedBalance)
{
	enter_log_enter = 0.0;
	flow_log_flow = 0.0;
	exit_log_exit = 0.0;
	enterFlow = 0.0;

	// For each module
	for (activeNetwork_iterator it(m_activeNetwork.begin()), itEnd(m_activeNetwork.end());
			it != itEnd; ++it)
	{
		NodeType& node = getNode(**it);
		// own node/module codebook
		flow_log_flow += infomath::plogp(node.data.flow + node.data.exitFlow);

		// use of index codebook
		enter_log_enter += infomath::plogp(node.data.enterFlow);
		exit_log_exit += infomath::plogp(node.data.exitFlow);
		enterFlow += node.data.enterFlow;
	}
	enterFlow += exitNetworkFlow;
	enterFlow_log_enterFlow = infomath::plogp(enterFlow);

	nodeFlow_log_nodeFlow = 0.0;
	for (unsigned int i = 0; i < m_numPhysicalNodes; ++i)
	{
		const ModuleToMemNodes& moduleToMemNodes = m_physToModuleToMemNodes[i];
		for (ModuleToMemNodes::const_iterator modToMemIt(moduleToMemNodes.begin()); modToMemIt != moduleToMemNodes.end(); ++modToMemIt)
			nodeFlow_log_nodeFlow += infomath::plogp(modToMemIt->second.sumFlow);
	}

	indexCodelength = enterFlow_log_enterFlow - enter_log_enter - exitNetworkFlow_log_exitNetworkFlow;
	moduleCodelength = -exit_log_exit + flow_log_flow - nodeFlow_log_nodeFlow;
	codelength = indexCodelength + moduleCodelength;
}

template<typename InfomapImplementation>
double InfomapGreedy<InfomapImplementation>::calcCodelengthFromFlowWithinOrExit(const NodeBase& parent)
{
	const FlowType& parentData = getNode(parent).data;
	double parentFlow = parentData.flow;
	double parentExit = parentData.exitFlow;
	double totalParentFlow = parentFlow + parentExit;
	if (totalParentFlow < 1e-16)
		return 0.0;

	double indexLength = 0.0;
	// For each child
//	for (NodeBase::const_sibling_iterator childIt(parent.begin_child()), endIt(parent.end_child());
//			childIt != endIt; ++childIt)
//	{
//		indexLength -= infomath::plogp(getNode(*childIt).data.flow / totalParentFlow);
//	}
	// For each physical node
	const std::vector<PhysData>& physNodes = getNode(parent).physicalNodes;
	for (unsigned int i = 0; i < physNodes.size(); ++i)
	{
		indexLength -= infomath::plogp(physNodes[i].sumFlowFromM2Node / totalParentFlow);
	}
	indexLength -= infomath::plogp(parentExit / totalParentFlow);

	indexLength *= totalParentFlow;

	return indexLength;
}

template<typename InfomapImplementation>
double InfomapGreedy<InfomapImplementation>::calcCodelengthFromEnterWithinOrExit(const NodeBase& parent)
{
	const FlowType& parentData = getNode(parent).data;
	double parentExit = parentData.exitFlow;
	if (parentData.flow < 1e-16)
		return 0.0;

	// H(x) = -xlog(x), T = q + SUM(p), q = exitFlow, p = flow or enterFlow
	// Normal format
	// L = q * -log(q/T) + p * SUM(-log(p/T))
	// Compact format
	// L = T * ( H(q/T) + SUM( H(p/T) ) )
	// Expanded format
	// L = q * -log(q) - q * -log(T) + SUM( p * -log(p) - p * -log(T) ) = T * log(T) - q*log(q) - SUM( p*log(p) )
	// As T is not known, use expanded format to avoid two loops
	double sumEnter = 0.0;
	double sumEnterLogEnter = 0.0;
	for (NodeBase::const_sibling_iterator childIt(parent.begin_child()), endIt(parent.end_child());
			childIt != endIt; ++childIt)
	{
		const double& enterFlow = getNode(*childIt).data.enterFlow; // rate of enter to finer level
		sumEnter += enterFlow;
		sumEnterLogEnter += infomath::plogp(enterFlow);
	}
	// The possibilities from this module: Either exit to coarser level or enter one of its children
	double totalCodewordUse = parentExit + sumEnter;

	return infomath::plogp(totalCodewordUse) - sumEnterLogEnter - infomath::plogp(parentExit);
}


// template<typename InfomapImplementation>
// inline
// double InfomapGreedySpecialized<InfomapImplementation>::getDeltaCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	double delta_enter = plogp(enterFlow + deltaEnterExitOldModule - deltaEnterExitNewModule) - enterFlow_log_enterFlow;

// 	double delta_enter_log_enter = \
// 			- plogp(m_moduleFlowData[oldModule].enterFlow) \
// 			- plogp(m_moduleFlowData[newModule].enterFlow) \
// 			+ plogp(m_moduleFlowData[oldModule].enterFlow - current.data.enterFlow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].enterFlow + current.data.enterFlow - deltaEnterExitNewModule);

// 	double delta_exit_log_exit = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow - current.data.exitFlow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + current.data.exitFlow - deltaEnterExitNewModule);

// 	double delta_flow_log_flow = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow \
// 					- current.data.exitFlow - current.data.flow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow \
// 					+ current.data.exitFlow + current.data.flow - deltaEnterExitNewModule);

// 	double delta_nodeFlow_log_nodeFlow = oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	double deltaL = delta_enter - delta_enter_log_enter - delta_exit_log_exit + delta_flow_log_flow - delta_nodeFlow_log_nodeFlow;
// 	return deltaL;
// }

// template<>
// inline
// double InfomapGreedySpecialized<InfomapUndirected>::getDeltaCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	// Double the effect as each link works in both directions
// 	deltaEnterExitOldModule *= 2;
// 	deltaEnterExitNewModule *= 2;

// 	double delta_exit = plogp(enterFlow + deltaEnterExitOldModule - deltaEnterExitNewModule) - enterFlow_log_enterFlow;

// 	double delta_exit_log_exit = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow - current.data.exitFlow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + current.data.exitFlow - deltaEnterExitNewModule);

// 	double delta_flow_log_flow = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow \
// 					- current.data.exitFlow - current.data.flow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow \
// 					+ current.data.exitFlow + current.data.flow - deltaEnterExitNewModule);

// 	double delta_nodeFlow_log_nodeFlow = oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	double deltaL = delta_exit - 2.0*delta_exit_log_exit + delta_flow_log_flow - delta_nodeFlow_log_nodeFlow;
// 	return deltaL;
// }

// template<>
// inline
// double InfomapGreedySpecialized<InfomapDirected>::getDeltaCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	double delta_exit = plogp(enterFlow + deltaEnterExitOldModule - deltaEnterExitNewModule) - enterFlow_log_enterFlow;

// 	double delta_exit_log_exit = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow - current.data.exitFlow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + current.data.exitFlow - deltaEnterExitNewModule);

// 	double delta_flow_log_flow = \
// 			- plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) \
// 			- plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow) \
// 			+ plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow \
// 					- current.data.exitFlow - current.data.flow + deltaEnterExitOldModule) \
// 			+ plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow \
// 					+ current.data.exitFlow + current.data.flow - deltaEnterExitNewModule);

// 	double delta_nodeFlow_log_nodeFlow = oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	double deltaL = delta_exit - 2.0*delta_exit_log_exit + delta_flow_log_flow - delta_nodeFlow_log_nodeFlow;
// 	return deltaL;
// }


// /**
//  * Update the codelength to reflect the move of node current
//  * in oldModuleDelta to newModuleDelta
//  * (Specialized for undirected flow and when exitFlow == enterFlow
//  */
// template<typename InfomapImplementation>
// inline
// void InfomapGreedySpecialized<InfomapImplementation>::updateCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	enterFlow -= \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	enter_log_enter -= \
// 			plogp(m_moduleFlowData[oldModule].enterFlow) + \
// 			plogp(m_moduleFlowData[newModule].enterFlow);
// 	exit_log_exit -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);


// 	m_moduleFlowData[oldModule] -= current.data;
// 	m_moduleFlowData[newModule] += current.data;

// 	m_moduleFlowData[oldModule].enterFlow += deltaEnterExitOldModule;
// 	m_moduleFlowData[oldModule].exitFlow += deltaEnterExitOldModule;
// 	m_moduleFlowData[newModule].enterFlow -= deltaEnterExitNewModule;
// 	m_moduleFlowData[newModule].exitFlow -= deltaEnterExitNewModule;

// 	enterFlow += \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	enter_log_enter += \
// 			plogp(m_moduleFlowData[oldModule].enterFlow) + \
// 			plogp(m_moduleFlowData[newModule].enterFlow);
// 	exit_log_exit += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);

// 	enterFlow_log_enterFlow = plogp(enterFlow);

// 	nodeFlow_log_nodeFlow += oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	indexCodelength = enterFlow_log_enterFlow - enter_log_enter - exitNetworkFlow_log_exitNetworkFlow;
// 	moduleCodelength = -exit_log_exit + flow_log_flow - nodeFlow_log_nodeFlow;
// 	codelength = indexCodelength + moduleCodelength;
// }

// template<>
// inline
// void InfomapGreedySpecialized<InfomapUndirected>::updateCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	// Double the effect as each link works in both directions
// 	deltaEnterExitOldModule *= 2;
// 	deltaEnterExitNewModule *= 2;

// 	enterFlow -= \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	exit_log_exit -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);


// 	m_moduleFlowData[oldModule] -= current.data;
// 	m_moduleFlowData[newModule] += current.data;

// 	m_moduleFlowData[oldModule].exitFlow += deltaEnterExitOldModule;
// 	m_moduleFlowData[newModule].exitFlow -= deltaEnterExitNewModule;

// 	enterFlow += \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	exit_log_exit += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);

// 	enterFlow_log_enterFlow = plogp(enterFlow);

// 	nodeFlow_log_nodeFlow += oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	indexCodelength = enterFlow_log_enterFlow - exit_log_exit - exitNetworkFlow_log_exitNetworkFlow;
// 	moduleCodelength = -exit_log_exit + flow_log_flow - nodeFlow_log_nodeFlow;
// 	codelength = indexCodelength + moduleCodelength;
// }

// /**
//  * Specialized when exitFlow == enterFlow
//  */
// template<>
// inline
// void InfomapGreedySpecialized<InfomapDirected>::updateCodelength(NodeType& current,
// 		DeltaFlow& oldModuleDelta, DeltaFlow& newModuleDelta)
// {
// 	using infomath::plogp;
// 	unsigned int oldModule = oldModuleDelta.module;
// 	unsigned int newModule = newModuleDelta.module;
// 	double deltaEnterExitOldModule = oldModuleDelta.deltaEnter + oldModuleDelta.deltaExit;
// 	double deltaEnterExitNewModule = newModuleDelta.deltaEnter + newModuleDelta.deltaExit;

// 	enterFlow -= \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	exit_log_exit -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow -= \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);


// 	m_moduleFlowData[oldModule] -= current.data;
// 	m_moduleFlowData[newModule] += current.data;

// 	m_moduleFlowData[oldModule].exitFlow += deltaEnterExitOldModule;
// 	m_moduleFlowData[newModule].exitFlow -= deltaEnterExitNewModule;

// 	enterFlow += \
// 			m_moduleFlowData[oldModule].enterFlow + \
// 			m_moduleFlowData[newModule].enterFlow;
// 	exit_log_exit += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow);
// 	flow_log_flow += \
// 			plogp(m_moduleFlowData[oldModule].exitFlow + m_moduleFlowData[oldModule].flow) + \
// 			plogp(m_moduleFlowData[newModule].exitFlow + m_moduleFlowData[newModule].flow);

// 	enterFlow_log_enterFlow = plogp(enterFlow);

// 	nodeFlow_log_nodeFlow += oldModuleDelta.sumDeltaPlogpPhysFlow + newModuleDelta.sumDeltaPlogpPhysFlow + oldModuleDelta.sumPlogpPhysFlow - newModuleDelta.sumPlogpPhysFlow;

// 	indexCodelength = enterFlow_log_enterFlow - exit_log_exit - exitNetworkFlow_log_exitNetworkFlow;
// 	moduleCodelength = -exit_log_exit + flow_log_flow - nodeFlow_log_nodeFlow;
// 	codelength = indexCodelength + moduleCodelength;
// }


template<typename InfomapImplementation>
inline
unsigned int InfomapGreedy<InfomapImplementation>::optimizeModules()
{
	DEBUG_OUT("\nInfomapGreedy<InfomapImplementation>::optimizeModules()");
	unsigned int numOptimizationRounds = 0;
	double oldCodelength = codelength;
	unsigned int loopLimit = m_config.coreLoopLimit;
	if (m_config.coreLoopLimit > 0 && m_config.randomizeCoreLoopLimit)
		loopLimit = static_cast<unsigned int>(m_rand() * m_config.coreLoopLimit) + 1;

	// Iterate while the optimization loop moves some nodes within the dynamic modular structure
	do
	{
		oldCodelength = codelength;
		tryMoveEachNodeIntoBestModule(); // returns numNodesMoved
		++numOptimizationRounds;
	} while (numOptimizationRounds != loopLimit &&
			codelength < oldCodelength - m_config.minimumCodelengthImprovement);

	return numOptimizationRounds;
}


/**
 * Try to minimize the codelength by trying to move nodes into the same modules as neighbouring nodes.
 * For each node:
 * 1. Calculate the change in codelength for a move to each of its neighbouring modules or to an empty module
 * 2. Move to the one that reduces the codelength the most, if any.
 *
 * The first step would require O(d^2), where d is the degree, if calculating the full change at each neighbour,
 * but a special data structure is used to accumulate the marginal effect of each link on its target, giving O(d).
 *
 * @return The number of nodes moved.
 */
template<typename InfomapImplementation>
inline
unsigned int InfomapGreedy<InfomapImplementation>::tryMoveEachNodeIntoBestModule()
{
	unsigned int numNodes = m_activeNetwork.size();
	// Get random enumeration of nodes
	std::vector<unsigned int> randomOrder(numNodes);
	infomath::getRandomizedIndexVector(randomOrder, m_rand);

	std::vector<DeltaFlow> moduleDeltaEnterExit(numNodes);
	std::vector<unsigned int> redirect(numNodes, 0);
	unsigned int offset = 1;
	unsigned int maxOffset = std::numeric_limits<unsigned int>::max() - 1 - numNodes;


	unsigned int numMoved = 0;
	for (unsigned int i = 0; i < numNodes; ++i)
	{
		// Reset offset before overflow
		if (offset > maxOffset)
		{
			redirect.assign(numNodes, 0);
			offset = 1;
		}

		// Pick nodes in random order
		unsigned int flip = randomOrder[i];
		NodeType& current = getNode(*m_activeNetwork[flip]);

		// If no links connecting this node with other nodes, it won't move into others,
		// and others won't move into this. TODO: Always best leave it alone?
//		if (current.degree() == 0)
		if (current.degree() == 0 ||
			(m_config.includeSelfLinks &&
			(current.outDegree() == 1 && current.inDegree() == 1) &&
			(**current.begin_outEdge()).target == current))
		{
			DEBUG_OUT("SKIPPING isolated node " << current << "\n");
			//TODO: If not skipping self-links, this yields different results from moveNodesToPredefinedModules!!
			ASSERT(!m_config.includeSelfLinks);
			continue;
		}

		// Create vector with module links

		unsigned int numModuleLinks = 0;
		if (current.isDangling())
		{
			redirect[current.index] = offset + numModuleLinks;
			moduleDeltaEnterExit[numModuleLinks].module = current.index;
			moduleDeltaEnterExit[numModuleLinks].deltaExit = 0.0;
			moduleDeltaEnterExit[numModuleLinks].deltaEnter = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = 0.0;
			++numModuleLinks;
		}
		else
		{
			// For all outlinks
			for (NodeBase::edge_iterator edgeIt(current.begin_outEdge()), endIt(current.end_outEdge());
					edgeIt != endIt; ++edgeIt)
			{
				EdgeType& edge = **edgeIt;
				if (edge.isSelfPointing())
					continue;
				NodeType& neighbour = getNode(edge.target);

				if (redirect[neighbour.index] >= offset)
				{
					moduleDeltaEnterExit[redirect[neighbour.index] - offset].deltaExit += edge.data.flow;
				}
				else
				{
					redirect[neighbour.index] = offset + numModuleLinks;
					moduleDeltaEnterExit[numModuleLinks].module = neighbour.index;
					moduleDeltaEnterExit[numModuleLinks].deltaExit = edge.data.flow;
					moduleDeltaEnterExit[numModuleLinks].deltaEnter = 0.0;
					moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = 0.0;
					moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = 0.0;
					++numModuleLinks;
				}
			}
		}
		// For all inlinks
		for (NodeBase::edge_iterator edgeIt(current.begin_inEdge()), endIt(current.end_inEdge());
				edgeIt != endIt; ++edgeIt)
		{
			EdgeType& edge = **edgeIt;
			if (edge.isSelfPointing())
				continue;
			NodeType& neighbour = getNode(edge.source);

			if (redirect[neighbour.index] >= offset)
			{
				moduleDeltaEnterExit[redirect[neighbour.index] - offset].deltaEnter += edge.data.flow;
			}
			else
			{
				redirect[neighbour.index] = offset + numModuleLinks;
				moduleDeltaEnterExit[numModuleLinks].module = neighbour.index;
				moduleDeltaEnterExit[numModuleLinks].deltaExit = 0.0;
				moduleDeltaEnterExit[numModuleLinks].deltaEnter = edge.data.flow;
				moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = 0.0;
				moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = 0.0;
				++numModuleLinks;
			}
		}

		// If alone in the module, add virtual link to the module (used when adding teleportation)
		if (redirect[current.index] < offset)
		{
			redirect[current.index] = offset + numModuleLinks;
			moduleDeltaEnterExit[numModuleLinks].module = current.index;
			moduleDeltaEnterExit[numModuleLinks].deltaExit = 0.0;
			moduleDeltaEnterExit[numModuleLinks].deltaEnter = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = 0.0;
			++numModuleLinks;
		}


		// Empty function if no teleportation coding model
		getImpl().addTeleportationDeltaFlowIfMove(current, moduleDeltaEnterExit, numModuleLinks);

		// Option to move to empty module (if node not already alone)
		if (m_moduleMembers[current.index] > 1 && m_emptyModules.size() > 0)
		{
			moduleDeltaEnterExit[numModuleLinks].module = m_emptyModules.back();
			moduleDeltaEnterExit[numModuleLinks].deltaExit = 0.0;
			moduleDeltaEnterExit[numModuleLinks].deltaEnter = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = 0.0;
			moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = 0.0;
			++numModuleLinks;
		}

		// Store the DeltaFlow of the current module
		DeltaFlow oldModuleDelta(moduleDeltaEnterExit[redirect[current.index] - offset]);


		// Overlapping modules
		/**
		 * delta = old.first + new.first + old.second - new.second.
		 * Two cases: (p(x) = plogp(x))
		 * Moving to a module that already have that physical node: (old: p1, p2, new p3, moving p2 -> old:p1, new p2,p3)
		 * Then old.second = new.second = plogp(physicalNodeSize) -> cancelation -> delta = p(p1) - p(p1+p2) + p(p2+p3) - p(p3)
		 * Moving to a module that not have that physical node: (old: p1, p2, new -, moving p2 -> old: p1, new: p2)
		 * Then new.first = new.second = 0 -> delta = p(p1) - p(p1+p2) + p(p2).
		 */
		unsigned int numPhysicalNodes = current.physicalNodes.size();
		for (unsigned int i = 0; i < numPhysicalNodes; ++i)
		{
			PhysData& physData = current.physicalNodes[i];
			ModuleToMemNodes& moduleToMemNodes = m_physToModuleToMemNodes[physData.physNodeIndex];
			for (ModuleToMemNodes::iterator overlapIt(moduleToMemNodes.begin()); overlapIt != moduleToMemNodes.end(); ++overlapIt)
			{
				unsigned int moduleIndex = overlapIt->first;
				MemNodeSet& memNodeSet = overlapIt->second;
				if (moduleIndex == current.index) // From where the multiple assigned node is moved
				{
					double oldPhysFlow = memNodeSet.sumFlow;
					double newPhysFlow = memNodeSet.sumFlow - physData.sumFlowFromM2Node;
					oldModuleDelta.sumDeltaPlogpPhysFlow += infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
					oldModuleDelta.sumPlogpPhysFlow += infomath::plogp(physData.sumFlowFromM2Node);
				}
				else // To where the multiple assigned node is moved
				{
					double oldPhysFlow = memNodeSet.sumFlow;
					double newPhysFlow = memNodeSet.sumFlow + physData.sumFlowFromM2Node;
					if (redirect[moduleIndex] >= offset)
					{
						moduleDeltaEnterExit[redirect[moduleIndex] - offset].sumDeltaPlogpPhysFlow += infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
						moduleDeltaEnterExit[redirect[moduleIndex] - offset].sumPlogpPhysFlow += infomath::plogp(physData.sumFlowFromM2Node);
					}
					else
					{
						redirect[moduleIndex] = offset + numModuleLinks;
						moduleDeltaEnterExit[numModuleLinks].module = moduleIndex;
						moduleDeltaEnterExit[numModuleLinks].deltaExit = 0.0;
						moduleDeltaEnterExit[numModuleLinks].deltaEnter = 0.0;
						moduleDeltaEnterExit[numModuleLinks].sumDeltaPlogpPhysFlow = infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
						moduleDeltaEnterExit[numModuleLinks].sumPlogpPhysFlow = infomath::plogp(physData.sumFlowFromM2Node);
						++numModuleLinks;
					}
				}
			}
		}



		// Randomize link order for optimized search
		for (unsigned int j = 0; j < numModuleLinks - 1; ++j)
		{
			unsigned int randPos = j + m_rand.randInt(numModuleLinks - j - 1);
			swap(moduleDeltaEnterExit[j], moduleDeltaEnterExit[randPos]);
		}

		DeltaFlow bestDeltaModule(oldModuleDelta);
		double bestDeltaCodelength = 0.0;

		// Find the move that minimizes the description length
		for (unsigned int j = 0; j < numModuleLinks; ++j)
		{
			unsigned int otherModule = moduleDeltaEnterExit[j].module;
			if(otherModule != current.index)
			{
				double deltaCodelength = getImpl().getDeltaCodelength(current, oldModuleDelta, moduleDeltaEnterExit[j]);

				if (deltaCodelength < bestDeltaCodelength)
				{
					bestDeltaModule = moduleDeltaEnterExit[j];
					bestDeltaCodelength = deltaCodelength;
				}

			}
		}

		// Make best possible move
		if(bestDeltaModule.module != current.index)
		{
			unsigned int bestModuleIndex = bestDeltaModule.module;
			//Update empty module vector
			if(m_moduleMembers[bestModuleIndex] == 0)
			{
				m_emptyModules.pop_back();
			}
			if(m_moduleMembers[current.index] == 1)
			{
				m_emptyModules.push_back(current.index);
			}

			getImpl().updateCodelength(current, oldModuleDelta, bestDeltaModule);

			m_moduleMembers[current.index] -= 1;
			m_moduleMembers[bestModuleIndex] += 1;

			unsigned int oldModuleIndex = current.index;
			current.index = bestModuleIndex;

			// For all multiple assigned nodes
			for (unsigned int i = 0; i < numPhysicalNodes; ++i)
			{
				PhysData& physData = current.physicalNodes[i];
				ModuleToMemNodes& moduleToMemNodes = m_physToModuleToMemNodes[physData.physNodeIndex];

				// Remove contribution to old module
				ModuleToMemNodes::iterator overlapIt = moduleToMemNodes.find(oldModuleIndex);
				if (overlapIt == moduleToMemNodes.end())
					throw std::length_error("Couldn't find old module among physical node assignments.");
				MemNodeSet& memNodeSet = overlapIt->second;
				memNodeSet.sumFlow -= physData.sumFlowFromM2Node;
				if (--memNodeSet.numMemNodes == 0)
					moduleToMemNodes.erase(overlapIt);

				// Add contribution to new module
				overlapIt = moduleToMemNodes.find(bestModuleIndex);
				if (overlapIt == moduleToMemNodes.end())
					moduleToMemNodes.insert(std::make_pair(bestModuleIndex, MemNodeSet(1, physData.sumFlowFromM2Node)));
				else {
					MemNodeSet& memNodeSet = overlapIt->second;
					++memNodeSet.numMemNodes;
					memNodeSet.sumFlow += physData.sumFlowFromM2Node;
				}
			}

			++numMoved;
		}

		offset += numNodes;
	}

	return numMoved;
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::moveNodesToPredefinedModules()
{
	// Size of active network and cluster array should match.
	ASSERT(m_moveTo.size() == m_activeNetwork.size());
//	getImpl().moveNodesToPredefinedModulesImpl();

	unsigned int numNodes = m_activeNetwork.size();

	DEBUG_OUT("Begin moving " << numNodes << " nodes to predefined modules, starting with codelength " <<
			codelength << "..." << std::endl);

	unsigned int numMoved = 0;
	for(unsigned int k = 0; k < numNodes; ++k)
	{
		NodeType& current = getNode(*m_activeNetwork[k]);
		unsigned int oldM = current.index; // == k
		unsigned int newM = m_moveTo[k];

		if (newM != oldM)
		{
			DeltaFlow oldModuleDelta(oldM, 0.0, 0.0, 0.0, 0.0);
			DeltaFlow newModuleDelta(newM, 0.0, 0.0, 0.0, 0.0);

			getImpl().addTeleportationDeltaFlowOnOldModuleIfMove(current, oldModuleDelta);
			getImpl().addTeleportationDeltaFlowOnNewModuleIfMove(current, newModuleDelta);

			// For all outlinks
			for (NodeBase::edge_iterator edgeIt(current.begin_outEdge()), endIt(current.end_outEdge());
					edgeIt != endIt; ++edgeIt)
			{
				EdgeType& edge = **edgeIt;
				if (edge.isSelfPointing())
					continue;
				unsigned int otherModule = edge.target.index;
				if (otherModule == oldM)
					oldModuleDelta.deltaExit += edge.data.flow;
				else if (otherModule == newM)
					newModuleDelta.deltaExit += edge.data.flow;
			}

			// For all inlinks
			for (NodeBase::edge_iterator edgeIt(current.begin_inEdge()), endIt(current.end_inEdge());
					edgeIt != endIt; ++edgeIt)
			{
				EdgeType& edge = **edgeIt;
				if (edge.isSelfPointing())
					continue;
				unsigned int otherModule = edge.source.index;
				if (otherModule == oldM)
					oldModuleDelta.deltaEnter += edge.data.flow;
				else if (otherModule == newM)
					newModuleDelta.deltaEnter += edge.data.flow;
			}


			// For all multiple assigned nodes
			for (unsigned int i = 0; i < current.physicalNodes.size(); ++i)
			{
				PhysData& physData = current.physicalNodes[i];
				ModuleToMemNodes& moduleToMemNodes = m_physToModuleToMemNodes[physData.physNodeIndex];

				// Remove contribution to old module
				ModuleToMemNodes::iterator overlapIt = moduleToMemNodes.find(oldM);
				if (overlapIt == moduleToMemNodes.end())
					throw std::length_error("Couldn't find old module among physical node assignments.");
				MemNodeSet& memNodeSet = overlapIt->second;
				double oldPhysFlow = memNodeSet.sumFlow;
				double newPhysFlow = memNodeSet.sumFlow - physData.sumFlowFromM2Node;
				oldModuleDelta.sumDeltaPlogpPhysFlow += infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
				oldModuleDelta.sumPlogpPhysFlow += infomath::plogp(physData.sumFlowFromM2Node);
				memNodeSet.sumFlow -= physData.sumFlowFromM2Node;
				if (--memNodeSet.numMemNodes == 0)
					moduleToMemNodes.erase(overlapIt);


				// Add contribution to new module
				overlapIt = moduleToMemNodes.find(newM);
				if (overlapIt == moduleToMemNodes.end())
				{
					moduleToMemNodes.insert(std::make_pair(newM, MemNodeSet(1, physData.sumFlowFromM2Node)));
					oldPhysFlow = 0.0;
					newPhysFlow = physData.sumFlowFromM2Node;
					newModuleDelta.sumDeltaPlogpPhysFlow += infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
					newModuleDelta.sumPlogpPhysFlow += infomath::plogp(physData.sumFlowFromM2Node);
				}
				else
				{
					MemNodeSet& memNodeSet = overlapIt->second;
					oldPhysFlow = memNodeSet.sumFlow;
					newPhysFlow = memNodeSet.sumFlow + physData.sumFlowFromM2Node;
					newModuleDelta.sumDeltaPlogpPhysFlow += infomath::plogp(newPhysFlow) - infomath::plogp(oldPhysFlow);
					newModuleDelta.sumPlogpPhysFlow += infomath::plogp(physData.sumFlowFromM2Node);
					++memNodeSet.numMemNodes;
					memNodeSet.sumFlow += physData.sumFlowFromM2Node;
				}

			}


			//Update empty module vector
			if(m_moduleMembers[newM] == 0)
			{
				m_emptyModules.pop_back();
			}
			if(m_moduleMembers[oldM] == 1)
			{
				m_emptyModules.push_back(oldM);
			}

			getImpl().updateCodelength(current, oldModuleDelta, newModuleDelta);

			m_moduleMembers[oldM] -= 1;
			m_moduleMembers[newM] += 1;

			current.index = newM;
			++numMoved;
		}
	}
	DEBUG_OUT("Done! Moved " << numMoved << " nodes into " << numActiveModules() << " modules to codelength: " << codelength << std::endl);
}


template<typename InfomapImplementation>
unsigned int InfomapGreedy<InfomapImplementation>::consolidateModules(bool replaceExistingStructure, bool asSubModules)
{
	unsigned int numNodes = m_activeNetwork.size();
	std::vector<NodeBase*> modules(numNodes, 0);

	bool activeNetworkAlreadyHaveModuleLevel = m_activeNetwork[0]->parent != root();
	bool activeNetworkIsLeafNetwork = m_activeNetwork[0]->isLeaf();


	if (asSubModules)
	{
		ASSERT(activeNetworkAlreadyHaveModuleLevel);
		// Release the pointers from modules to leaf nodes so that the new submodules will be inserted as its only children.
		for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), moduleEnd(root()->end_child());
				moduleIt != moduleEnd; ++moduleIt)
		{
			moduleIt->releaseChildren();
		}
	}
	else
	{
		// Happens after optimizing fine-tune and when moving leaf nodes to super clusters
		if (activeNetworkAlreadyHaveModuleLevel)
		{
			DEBUG_OUT("Replace existing " << numTopModules() << " modules with its children before consolidating the " <<
					numActiveModules() << " dynamic modules... ");
			root()->replaceChildrenWithGrandChildren();
			ASSERT(m_activeNetwork[0]->parent == root());
		}
		root()->releaseChildren();
	}

	// Create the new module nodes and re-parent the active network from its common parent to the new module level
	for (unsigned int i = 0; i < numNodes; ++i)
	{
		NodeBase* node = m_activeNetwork[i];
		unsigned int moduleIndex = node->index;
		if (modules[moduleIndex] == 0)
		{
			modules[moduleIndex] = new NodeType(m_moduleFlowData[moduleIndex]);
			node->parent->addChild(modules[moduleIndex]);
			modules[moduleIndex]->index = moduleIndex;
			// If node->parent is a module, its former children (leafnodes) has been released above, getting only submodules
//			if (node->parent->firstChild == node)
//				node->parent->firstChild = modules[moduleIndex];
		}
		modules[moduleIndex]->addChild(node);
	}

	if (asSubModules)
	{
		DEBUG_OUT("Consolidated " << numActiveModules() << " submodules under " << numTopModules() << " modules, " <<
				"store module structure before releasing it..." << std::endl);
		// Store the module structure on the submodules
		unsigned int moduleIndex = 0;
		for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), endIt(root()->end_child());
				moduleIt != endIt; ++moduleIt, ++moduleIndex)
		{
			for (NodeBase::sibling_iterator subModuleIt(moduleIt->begin_child()), endIt(moduleIt->end_child());
					subModuleIt != endIt; ++subModuleIt)
			{
				subModuleIt->index = moduleIndex;
			}
		}
		if (replaceExistingStructure)
		{
			// Remove the module level
			root()->replaceChildrenWithGrandChildren();
		}
	}


	// Aggregate links from lower level to the new modular level
	typedef std::pair<NodeBase*, NodeBase*> NodePair;
	typedef std::map<NodePair, double> EdgeMap;
	EdgeMap moduleLinks;

	for (activeNetwork_iterator nodeIt(m_activeNetwork.begin()), nodeEnd(m_activeNetwork.end());
			nodeIt != nodeEnd; ++nodeIt)
	{
		NodeBase* node = *nodeIt;

		NodeBase* parent = node->parent;
		for (NodeBase::edge_iterator edgeIt(node->begin_outEdge()), edgeEnd(node->end_outEdge());
				edgeIt != edgeEnd; ++edgeIt)
		{
			EdgeType* edge = *edgeIt;
			NodeBase* otherParent = edge->target.parent;

			if (otherParent != parent)
			{
				NodeBase *m1 = parent, *m2 = otherParent;
				// If undirected, the order may be swapped to aggregate the edge on an opposite one
				if (m_config.isUndirected() && m1->index > m2->index)
					std::swap(m1, m2);
				// Insert the node pair in the edge map. If not inserted, add the flow value to existing node pair.
				std::pair<EdgeMap::iterator, bool> ret = \
						moduleLinks.insert(std::make_pair(NodePair(m1, m2), edge->data.flow));
				if (!ret.second)
					ret.first->second += edge->data.flow;
			}
		}
	}

	// Add the aggregated edge flow structure to the new modules
	for (EdgeMap::const_iterator edgeIt(moduleLinks.begin()), edgeEnd(moduleLinks.end());
			edgeIt != edgeEnd; ++edgeIt)
	{
		const NodePair& nodePair = edgeIt->first;
		nodePair.first->addOutEdge(*nodePair.second, 0.0, edgeIt->second);
	}

	// Replace active network with its children if not at leaf level.
	if (!activeNetworkIsLeafNetwork && replaceExistingStructure)
	{
		for (activeNetwork_iterator nodeIt(m_activeNetwork.begin()), nodeEnd(m_activeNetwork.end());
				nodeIt != nodeEnd; ++nodeIt)
		{
			(*nodeIt)->replaceWithChildren();
		}
	}

	// Calculate the number of non-trivial modules
	m_numNonTrivialTopModules = 0;
	for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), endIt(root()->end_child());
			moduleIt != endIt; ++moduleIt)
	{
		if (moduleIt->childDegree() != 1)
			++m_numNonTrivialTopModules;
	}


	// Update physicalNodes
	std::map<unsigned int, std::map<unsigned int, unsigned int> > validate;

	for(unsigned int i = 0; i < m_numPhysicalNodes; ++i)
	{
		ModuleToMemNodes& modToMemNodes = m_physToModuleToMemNodes[i];
		for(ModuleToMemNodes::iterator overlapIt = modToMemNodes.begin(); overlapIt != modToMemNodes.end(); ++overlapIt)
		{
			if(++validate[overlapIt->first][i] > 1)
				throw std::domain_error("[InfomapGreedy::consolidateModules] Error updating physical nodes: duplication error");

			getNode(*modules[overlapIt->first]).physicalNodes.push_back(PhysData(i, overlapIt->second.sumFlow));
		}
	}


	return numActiveModules();
}


template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printNodeRanks(std::ostream& out)
{
	out << "#node-flow\n";
	for (TreeData::leafIterator it(m_treeData.begin_leaf()), itEnd(m_treeData.end_leaf());
			it != itEnd; ++it)
	{
		out << getNode(**it).data.flow << '\n';
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printFlowNetwork(std::ostream& out)
{
	for (TreeData::leafIterator nodeIt(m_treeData.begin_leaf());
			nodeIt != m_treeData.end_leaf(); ++nodeIt)
	{
		NodeBase& node = **nodeIt;
		out << node.originalIndex << " (" << getNode(node).data << ")\n";
		for (NodeBase::edge_iterator edgeIt(node.begin_outEdge()), endEdgeIt(node.end_outEdge());
				edgeIt != endEdgeIt; ++edgeIt)
		{
			EdgeType& edge = **edgeIt;
			out << "  --> " << edge.target.originalIndex << " (" << edge.data.flow << ")\n";
		}
		for (NodeBase::edge_iterator edgeIt(node.begin_inEdge()), endEdgeIt(node.end_inEdge());
				edgeIt != endEdgeIt; ++edgeIt)
		{
			EdgeType& edge = **edgeIt;
			out << "  <-- " << edge.source.originalIndex << " (" << edge.data.flow << ")\n";
		}
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printMap(std::ostream& out)
{
	typedef std::multimap<double, EdgeType*, std::greater<double> > LinkMap;
	typedef std::multimap<double, NodeBase*, std::greater<double> > NodeMap;
	typedef std::vector<std::pair<NodeBase*, NodeMap> > ModuleData;
	// First collect and sort leaf nodes and module links
	LinkMap moduleLinks;
	ModuleData moduleData(root()->childDegree());
	unsigned int moduleIndex = 0;
	for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), endIt(root()->end_child());
			moduleIt != endIt; ++moduleIt, ++moduleIndex)
	{
		moduleData[moduleIndex].first = moduleIt.base();
		// Collect leaf nodes
		LeafNodeIterator<NodeBase*> li(moduleIt.base());
		LeafNodeIterator<NodeBase*> liEnd(moduleIt->next);
		while (li != liEnd)
		{
			moduleData[moduleIndex].second.insert(std::make_pair(getNode(*li).data.flow, li.base()));
			++li;
		}

		// Collect module links
		for (NodeBase::edge_iterator outEdgeIt(moduleIt->begin_outEdge()), endIt(moduleIt->end_outEdge());
				outEdgeIt != endIt; ++outEdgeIt)
		{
			moduleLinks.insert(std::make_pair((*outEdgeIt)->data.flow, *outEdgeIt));
		}
	}

	out << "# modules: " << numTopModules() << "\n";
	out << "# modulelinks: " << moduleLinks.size() << "\n";
	out << "# nodes: " << numLeafNodes() << "\n";
	out << "# links: " << m_treeData.numLeafEdges() << "\n";
	out << "# codelength: " << hierarchicalCodelength << "\n";
	out << "*" << (m_config.isUndirected() ? "Undirected" : "Directed") << "\n";

	out << "*Modules " << numTopModules() << "\n";
	unsigned int moduleNumber = 1;
	for (ModuleData::iterator moduleIt(moduleData.begin()), endIt(moduleData.end());
			moduleIt != endIt; ++moduleIt, ++moduleNumber)
	{
		// Use the name of the biggest leaf node in the module to name the module
		NodeBase& module = *moduleIt->first;
		FlowType flowData = getNode(module).data;
		out << (module.index + 1) << " \"" << moduleIt->second.begin()->second->name << ",...\" " <<
				flowData.flow << " " << flowData.exitFlow << "\n";
	}

	// Collect the leaf nodes under each top module, sort them on flow, and write grouped on top module
	out << "*Nodes " << numLeafNodes() << "\n";
	moduleNumber = 1;
	for (ModuleData::iterator moduleIt(moduleData.begin()), endIt(moduleData.end());
			moduleIt != endIt; ++moduleIt, ++moduleNumber)
	{
		// Use the name of the biggest leaf node in the module to name the module
		NodeMap& nodeMap = moduleIt->second;
		unsigned int nodeNumber = 1;
		for (NodeMap::iterator it(nodeMap.begin()), itEnd(nodeMap.end());
				it != itEnd; ++it, ++nodeNumber)
		{
			out << moduleNumber << ":" << nodeNumber << " \"" << it->second->name << "\" " <<
				it->first << "\n";
		}
	}

	out << "*Links " << moduleLinks.size() << "\n";
	for (LinkMap::iterator linkIt(moduleLinks.begin()), endIt(moduleLinks.end());
			linkIt != endIt; ++linkIt)
	{
		EdgeType& edge = *linkIt->second;
		out << (edge.source.index+1) << " " << (edge.target.index+1) << " " << edge.data.flow << "\n";
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::sortTree(NodeBase& parent)
{
	if (parent.getSubInfomap() != 0)
	{
		parent.getSubInfomap()->sortTree();
	}
	std::multimap<double, NodeBase*, std::greater<double> > sortedModules;
	for (NodeBase::sibling_iterator childIt(parent.begin_child()), endChildIt(parent.end_child());
			childIt != endChildIt; ++childIt)
	{
		sortTree(*childIt);
		double rank = getNode(*childIt).data.flow;
		sortedModules.insert(std::pair<double, NodeBase*>(rank, childIt.base()));
	}
	parent.releaseChildren();
	unsigned int sortedIndex = 0;
	for (std::multimap<double, NodeBase*>::iterator it(sortedModules.begin()), itEnd(sortedModules.end());
			it != itEnd; ++it, ++sortedIndex)
	{
		parent.addChild(it->second);
		it->second->index = sortedIndex;
	}
}


template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::buildHierarchicalNetwork(HierarchicalNetwork& data, bool includeLinks)
{
	buildHierarchicalNetworkHelper(data, data.getRootNode(), m_treeData);
	if (includeLinks)
	{
		for (TreeData::leafIterator leafIt(m_treeData.begin_leaf()); leafIt != m_treeData.end_leaf(); ++leafIt)
		{
			NodeBase& node = **leafIt;
			for (NodeBase::edge_iterator outEdgeIt(node.begin_outEdge()), endIt(node.end_outEdge());
					outEdgeIt != endIt; ++outEdgeIt)
			{
				EdgeType& edge = **outEdgeIt;
				data.addLeafEdge(edge.source.originalIndex, edge.target.originalIndex, edge.data.flow);
			}
		}
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::buildHierarchicalNetworkHelper(HierarchicalNetwork& data, HierarchicalNetwork::node_type& parent, const TreeData& originalData, NodeBase* rootNode)
{
	if (rootNode == 0)
		rootNode = root();

	if (rootNode->getSubInfomap() != 0)
	{
		getImpl(*rootNode->getSubInfomap()).buildHierarchicalNetworkHelper(data, parent, originalData);
		return;
	}

	for (NodeBase::sibling_iterator childIt(rootNode->begin_child()), endIt(rootNode->end_child());
			childIt != endIt; ++childIt)
	{
		const NodeType& node = getNode(*childIt);
		if (node.isLeaf())
		{
			data.addLeafNode(parent, node.data.flow, node.data.exitFlow, originalData.getLeafNode(node.originalIndex).name, node.originalIndex);
		}
		else
		{
			SNode& newParent = data.addNode(parent, node.data.flow, node.data.exitFlow);
			buildHierarchicalNetworkHelper(data, newParent, originalData, childIt.base());
		}
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printSubInfomapTree(std::ostream& out, const TreeData& originalData, const std::string& prefix)
{
	unsigned int moduleIndex = 1;
	for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), endIt(root()->end_child());
			moduleIt != endIt; ++moduleIt, ++moduleIndex)
	{
		std::ostringstream subPrefix;
		subPrefix << prefix << moduleIndex << ":";
		const std::string subPrefixStr = subPrefix.str();
		if (moduleIt->getSubInfomap() == 0)
			printSubTree(out, *moduleIt, originalData, subPrefixStr);
		else
			moduleIt->getSubInfomap()->printSubInfomapTree(out, originalData, subPrefixStr);
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printSubTree(std::ostream& out, NodeBase& module, const TreeData& originalData, const std::string& prefix)
{
	if (module.isLeaf())
	{
		const NodeType& node = getNode(originalData.getLeafNode(module.originalIndex));
		out << prefix << " " << node.data.flow << " \"" << node.name << "\"\n";
		return;
	}

	unsigned int moduleIndex = 1;
	for (NodeBase::sibling_iterator childIt(module.begin_child()), endIt(module.end_child());
			childIt != endIt; ++childIt, ++moduleIndex)
	{
		const std::string subPrefixStr = io::Str() << prefix << moduleIndex << ":";
		if (childIt->getSubInfomap() == 0)
		{
			if (childIt->isLeaf())
			{
				const NodeType& node = getNode(originalData.getLeafNode(childIt->originalIndex));
				out << prefix << moduleIndex << " " << node.data.flow << " \"" << node.name << "\"\n";
			}
			else
			{
				printSubTree(out, *childIt, originalData, subPrefixStr);
			}
		}
		else
		{
			childIt->getSubInfomap()->printSubInfomapTree(out, originalData, subPrefixStr);
		}
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printSubInfomapTreeDebug(std::ostream& out, const TreeData& originalData, const std::string& prefix)
{
	NodeType& node = getNode(*root());
//	out << prefix << " " << node.data.flow << " (" << node.data.exitFlow << ")\n";
//	out << prefix << " " << node.data.flow << " \"" <<
//			node.name << "\" (" << node.id << ")\n";
	out << prefix << " (flow: " << node.data.flow << ", enter: " <<
					node.data.enterFlow << ", exit: " << node.data.exitFlow <<
					", L: " << node.codelength << ")\n";
//	out << prefix << " (flow: " << node.data.flow << ", enter: " <<
//					node.data.enterFlow << ", exit: " << node.data.exitFlow << ")\n";


	unsigned int moduleIndex = 1;
	for (NodeBase::sibling_iterator moduleIt(root()->begin_child()), endIt(root()->end_child());
			moduleIt != endIt; ++moduleIt, ++moduleIndex)
	{
		std::ostringstream subPrefix;
		subPrefix << prefix << moduleIndex << ":";
		const std::string subPrefixStr = subPrefix.str();
		if (moduleIt->getSubInfomap() == 0)
			printSubTreeDebug(out, *moduleIt, originalData, subPrefixStr);
		else
			moduleIt->getSubInfomap()->printSubInfomapTreeDebug(out, originalData, subPrefixStr);
	}
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::printSubTreeDebug(std::ostream& out, NodeBase& module, const TreeData& originalData, const std::string& prefix)
{
	if (module.isLeaf())
	{
		const NodeType& node = getNode(originalData.getLeafNode(module.originalIndex));
//		out << prefix << " " << node.data.flow <<
//								" (" << node.data.exitFlow << ") \"" << node.name << "\"\n";
//		out << prefix << " " << node.data.flow << " \"" <<
//				node.name << "\" (" << node.id << ")\n";
		out << prefix << " \"" << node.name << "\" (flow: " << node.data.flow << ", enter: " <<
				node.data.enterFlow << ", exit: " << node.data.exitFlow <<
				", L: " << node.codelength << ")\n";
//		out << prefix << " \"" << node.name << "\" (flow: " << node.data.flow << ", enter: " <<
//				node.data.enterFlow << ", exit: " << node.data.exitFlow << ")\n";
		return;
	}
	const FlowType& moduleData = getNode(module).data;
//	out << prefix << " " << moduleData.flow << " (" << moduleData.exitFlow << ")\n";
//	out << prefix << " " << moduleData.flow << " \"" <<
//					module.name << "\" (" << module.id << ")\n";
	out << prefix << " (flow: " << moduleData.flow << ", enter: " <<
			moduleData.enterFlow << ", exit: " << moduleData.exitFlow <<
			", L: " << module.codelength << ")\n";
//	out << prefix << " (flow: " << moduleData.flow << ", enter: " <<
//			moduleData.enterFlow << ", exit: " << moduleData.exitFlow << ")\n";

	unsigned int moduleIndex = 1;
	for (NodeBase::sibling_iterator childIt(module.begin_child()), endIt(module.end_child());
			childIt != endIt; ++childIt, ++moduleIndex)
	{
		const std::string subPrefixStr = io::Str() << prefix << moduleIndex << ":";
		if (childIt->getSubInfomap() == 0)
		{
			if (childIt->isLeaf())
			{
				const NodeType& node = getNode(originalData.getLeafNode(childIt->originalIndex));
//				out << prefix << moduleIndex << " " << node.data.flow <<
//						" (" << node.data.exitFlow << ") \"" << node.name << "\"\n";
//				out << prefix << moduleIndex << " " << node.data.flow <<
//						" \"" << node.name << "\" (" <<	node.id << ")\n";
				out << prefix << moduleIndex << " \"" << node.name << "\" (flow: " <<
						node.data.flow << ", enter: " << node.data.enterFlow << ", exit: " <<
						node.data.exitFlow << ", L: " << node.codelength << ")\n";
//				out << prefix << moduleIndex << " \"" << node.name << "\" (flow: " <<
//						node.data.flow << ", enter: " << node.data.enterFlow << ", exit: " <<
//						node.data.exitFlow << ")\n";

			}
			else
			{
				printSubTreeDebug(out, *childIt, originalData, subPrefixStr);
			}
		}
		else
		{
			childIt->getSubInfomap()->printSubInfomapTreeDebug(out, originalData, subPrefixStr);
		}
	}
}


template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::resetModuleFlowFromLeafNodes()
{
	// Reset from top to bottom
	resetModuleFlow(*root());

	// Aggregate from bottom to top
	for (TreeData::leafIterator leafIt(m_treeData.begin_leaf()); leafIt != m_treeData.end_leaf(); ++leafIt)
	{
		NodeBase* node = *leafIt;
		double flow = getNode(*node).data.flow;
		while (node = node->parent, node != 0)
		{
			getNode(*node).data.flow += flow;
		}
	}
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::resetModuleFlow(NodeBase& node)
{
	getNode(node).data.flow = 0.0;
	for (NodeBase::sibling_iterator childIt(node.begin_child()), endIt(node.end_child());
			childIt != endIt; ++childIt)
	{
		if (!childIt->isLeaf())
			resetModuleFlow(*childIt);
	}
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::generateNetworkFromChildren(NodeBase& parent)
{
	exitNetworkFlow = 0.0;

	std::set<unsigned int> setOfPhysicalNodes;

	// Clone all nodes
	unsigned int numNodes = parent.childDegree();
	m_treeData.reserveNodeCount(numNodes);
	unsigned int i = 0;
	for (NodeBase::sibling_iterator childIt(parent.begin_child()), endIt(parent.end_child());
			childIt != endIt; ++childIt, ++i)
	{
		NodeType& otherNode = getNode(*childIt);
		NodeBase* node = new NodeType(otherNode);
		node->originalIndex = childIt->originalIndex;
		m_treeData.addClonedNode(node);
		childIt->index = i; // Set index to its place in this subnetwork to be able to find edge target below
		node->index = i;

		for (unsigned int j = 0; j < otherNode.physicalNodes.size(); ++j)
		{
			PhysData& physData = otherNode.physicalNodes[j];
			setOfPhysicalNodes.insert(physData.physNodeIndex);
		}
	}

	// Re-index physical nodes
	std::map<unsigned int, unsigned int> subPhysIndexMap;
	unsigned int subPhysIndex = 0;
	for (std::set<unsigned int>::iterator it(setOfPhysicalNodes.begin()); it != setOfPhysicalNodes.end(); ++it, ++subPhysIndex)
	{
		subPhysIndexMap.insert(std::make_pair(*it, subPhysIndex));
	}

	for (TreeData::leafIterator leafIt(m_treeData.begin_leaf()); leafIt != m_treeData.end_leaf(); ++leafIt, ++i)
	{
		NodeType& node = getNode(**leafIt);
		for (unsigned int j = 0; j < node.physicalNodes.size(); ++j)
		{
			PhysData& physData = node.physicalNodes[j];
			physData.physNodeIndex = subPhysIndexMap[physData.physNodeIndex];
		}
	}

	m_numPhysicalNodes = setOfPhysicalNodes.size();

//	double exitFlowFromLinks = 0.0;
	NodeBase* parentPtr = &parent;
	// Clone edges
	for (NodeBase::sibling_iterator childIt(parent.begin_child()), endIt(parent.end_child());
			childIt != endIt; ++childIt)
	{
		NodeBase& node = *childIt;
		for (NodeBase::edge_iterator outEdgeIt(node.begin_outEdge()), endIt(node.end_outEdge());
				outEdgeIt != endIt; ++outEdgeIt)
		{
			const EdgeType& edge = **outEdgeIt;
			// If neighbour node is within the same module, add the link to this subnetwork.
			if (edge.target.parent == parentPtr)
			{
				m_treeData.addEdge(node.index, edge.target.index, edge.data.weight, edge.data.flow);
			}
//			else
//			{
//				exitFlowFromLinks += edge.data.flow;
//			}
		}

		// Override for undirected network and add exit flow from in-links here.
		// TODO: Do it here with template specialization!
	}

	double parentExit = getNode(parent).data.exitFlow;


	exitNetworkFlow = parentExit;
	exitNetworkFlow_log_exitNetworkFlow = infomath::plogp(exitNetworkFlow);
}


template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::transformNodeFlowToEnterFlow(NodeBase* parent)
{
	for (NodeBase::sibling_iterator moduleIt(parent->begin_child()), endIt(parent->end_child());
			moduleIt != endIt; ++moduleIt)
	{
		NodeType& module = getNode(*moduleIt);
		module.data.flow = module.data.enterFlow;
	}
}

template<typename InfomapImplementation>
void InfomapGreedy<InfomapImplementation>::cloneFlowData(const NodeBase& source, NodeBase& target)
{
	getNode(target).data = getNode(source).data;
}

template<typename InfomapImplementation>
inline
typename InfomapGreedy<InfomapImplementation>::NodeType& InfomapGreedy<InfomapImplementation>::getNode(NodeBase& node)
{
	return static_cast<NodeType&>(node);
}

template<typename InfomapImplementation>
inline
const typename InfomapGreedy<InfomapImplementation>::NodeType& InfomapGreedy<InfomapImplementation>::getNode(const NodeBase& node) const
{
	return static_cast<const NodeType&>(node);
}

template<typename InfomapImplementation>
inline
unsigned int InfomapGreedy<InfomapImplementation>::numActiveModules()
{
	return m_activeNetwork.size() - m_emptyModules.size();
}

template<typename InfomapImplementation>
inline
unsigned int InfomapGreedy<InfomapImplementation>::numDynamicModules()
{
	return m_activeNetwork.size() - m_emptyModules.size();
}

template<typename InfomapImplementation>
inline
InfomapImplementation& InfomapGreedy<InfomapImplementation>::getImpl()
{
	return static_cast<InfomapImplementation&>(*this);
}

template<typename InfomapImplementation>
inline
InfomapImplementation& InfomapGreedy<InfomapImplementation>::getImpl(InfomapBase& infomap)
{
	return static_cast<InfomapImplementation&>(infomap);
}

template<typename InfomapImplementation>
inline
FlowDummy InfomapGreedy<InfomapImplementation>::getNodeData(NodeBase& node)
{
	return FlowDummy(getNode(node).data);
}

template<typename InfomapImplementation>
inline
std::vector<PhysData>& InfomapGreedy<InfomapImplementation>::getPhysicalMembers(NodeBase& node)
{
	return getNode(node).physicalNodes;
}

template<typename InfomapImplementation>
inline
M2Node& InfomapGreedy<InfomapImplementation>::getPhysical(NodeBase& node)
{
	return getNode(node).m2Node;
}

template<typename InfomapImplementation>
inline
void InfomapGreedy<InfomapImplementation>::debugPrintInfomapTerms()
{
	std::cout << "(moduleLength: " << -exit_log_exit << " + " << flow_log_flow << " - " << nodeFlow_log_nodeFlow << " = " << moduleCodelength <<")\n";
}


#endif /* INFOMAPGREEDY_H_ */
