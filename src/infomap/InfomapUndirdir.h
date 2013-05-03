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


#ifndef INFOMAPUNDIRDIR_H_
#define INFOMAPUNDIRDIR_H_

#include "InfomapGreedy.h"
#include "flowData.h"

class InfomapUndirdir : public InfomapGreedy<InfomapUndirdir>
{
	friend class InfomapGreedy<InfomapUndirdir>;
	typedef Node<FlowType>												NodeType;
public:
	InfomapUndirdir(const Config& conf);
	virtual ~InfomapUndirdir() {};

protected:
	virtual void calculateFlow();
	virtual unsigned int optimizeModulesImpl();
	virtual unsigned int moveNodesToPredefinedModulesImpl();

	virtual void calculateCodelengthFromActiveNetwork();
	virtual void recalculateCodelengthFromActiveNetwork();

private:
	double getDeltaCodelength(NodeType& current, double additionalExitOldModuleIfMoved,
			unsigned int newModule, double reductionInExitFlowNewModule);
	void updateCodelength(NodeType& current, double deltaInOutOldModule,
			unsigned int newModule, double deltaInOutNewModule);

	//XXX Take back from base after debug
//	double enter_log_enter;
//	double enterFlow;
//	double enterFlow_log_enterFlow;
};

#endif /* INFOMAPUNDIRDIR_H_ */
