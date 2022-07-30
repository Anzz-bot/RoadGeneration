/**
 * Road-Generation
 * --------------------------------------------------------
 * Copyright (c) 2021 Institut für Kraftfahrzeuge, RWTH Aachen, ika
 * Report bugs and download new versions https://github.com/ika-rwth-aachen/RoadGeneration
 *
 * This library is distributed under the MIT License.
 * 
 * @file linkSegments.h
 *
 * @brief file contains methodology for linking two segments 
 *
 * @author Christian Geller
 * Contact: christian.geller@rwth-aachen.de
 *
 */

#include<map>
#include<queue>

extern settings setting;

/**
 * @brief function links all specified segments 
 *  	the reference frame has to be specified
 * 		two segments can be linked by determine two contactpoints of the roads 
 * 
 * @param doc 	tree structure which contains the input data
 * @param data 	roadNetwork structure generated by the tool
 * @return int 	error code
 */
int linkSegments(xmlTree &inputxml, roadNetwork &data)
{
	if(!setting.silentMode)
		cout << "Processing linkSegments" << endl;


	DOMElement *links;
	if (inputxml.findNodeWithName("links", links))
	{
		cerr << "ERR: 'links' are not specified in input file." << endl;
		cerr << "\t -> skip segment linking" << endl;
		return 0;
	}

	// define reference system
	int refId = readIntAttrFromNode(links, "refId");
	double hdgOffset = readDoubleAttrFromNode(links, "hdgOffset");
	double xOffset = readDoubleAttrFromNode(links, "xOffset");
	double yOffset = readDoubleAttrFromNode(links, "yOffset");


	for (auto &&r : data.roads)
	{
		if (r.junction != refId)
			continue;

		for (auto &&g : r.geometries)
		{

			// transform geometries of reference segement into reference system
			double x = g.x * cos(hdgOffset) - g.y * sin(hdgOffset);
			double y = g.x * sin(hdgOffset) + g.y * cos(hdgOffset);

			g.x = x + xOffset;
			g.y = y + yOffset;
			g.hdg = g.hdg + hdgOffset;
		}
	}

	//generate a map to store all the outgoing links of each segment
	std::map<int, vector<int>> m;

	for (DOMElement *segmentLink = links->getFirstElementChild();segmentLink != NULL; segmentLink = segmentLink->getNextElementSibling())
	{
		int fromSegment = readIntAttrFromNode(segmentLink, "fromSegment");
		int toSegment = readIntAttrFromNode(segmentLink, "toSegment");

		int found = m.count(fromSegment);
		if(found <= 0)
		{
			vector<int> v = vector<int>();
			v.push_back(toSegment);
			m[fromSegment] = v;
		} 
		else {
			m[fromSegment].push_back(toSegment);
		}

		cout << "here" << endl;
	}

	//DEBUG print out the order in which to process elements
	//TODO all roads that got linked to the reference netowrk as a to Segment dont get considered here. FIX THIS

	bool done = false;
	queue<int> toDo = queue<int>();
	vector<int> transformedIds;
	toDo.push(refId);

	while(!toDo.empty())
	{
		int curId = toDo.front();
		toDo.pop();

		if(isIn(transformedIds, curId)) continue;

		cout << "processing " << curId << endl;
		//process the element------------------

		//first find the element in the list. skip if it is the ref element 
		if(curId != refId)
		{
			for (DOMElement *segmentLink = links->getFirstElementChild();segmentLink != NULL; segmentLink = segmentLink->getNextElementSibling())
			{

			}
		}
		//end processessing the element--------
		transformedIds.push_back(curId);
		
		auto adj =  m[curId];
		for(int e: adj)
		{
			toDo.push(e);
		}
	}
	//End debug!



	return 0;
}


/**
 * @brief Transforms one toSegments position according to the from segment and the semgment link data to the coordinate system of the reference segment
 * 
 * @param segmentLink segment to link
 * @param data road network data
 * @return int error code
 */
int transformRoad(DOMElement *segmentLink, roadNetwork &data)
{
	// --- add other specified segments ----------------------------------------


	// get properties
	int fromSegment = readIntAttrFromNode(segmentLink, "fromSegment");
	int toSegment = readIntAttrFromNode(segmentLink, "toSegment");
	int fromRoadId = readIntAttrFromNode(segmentLink, "fromRoad");
	int toRoadId = readIntAttrFromNode(segmentLink, "toRoad");
	string fromPos = readStrAttrFromNode(segmentLink, "fromPos");
	string toPos = readStrAttrFromNode(segmentLink, "toPos");
	road fromRoad;
	road toRoad;
	int toJunctionGroup = -1; //in case we like TO a junction group, store the pointer

	

	// assumption is that "fromSegement" is already linked to reference frame

	double fromX, fromY, fromHdg;
	double toX, toY, toHdg;

	// check if fromSegment is junction
	bool fromIsJunction = false;
	for (auto &&j : data.junctions)
		if (j.id == fromSegment)
			fromIsJunction = true;

	// check if toSegment is junction
	bool toIsJunction = false;
	for (auto &&j : data.junctions)
		if (j.id == toSegment)
			toIsJunction = true;

	/*fix for the roundabout junction namespace problem. Since multiple junctions get generated for each roundabout
	the linking is problematic with the input format (since the user does not know the id that will be generated for the junctions).
	So we check if the from and to segments belong to a roundabout*/

	for(junctionGroup &jg: data.juncGroups)
	{
		if(jg.type != roundaboutType) continue;

		if(toSegment == jg.id)
		{
			//toSegment is a roundabout
			toJunctionGroup = jg.id;

		}
	
	}
	//-------------------END roundabout namespace fix---------------------------------


	// save from position
	for (auto &&r : data.roads)
	{
			
		if (r.junction != fromSegment || r.inputId != fromRoadId)
			continue;
		if (fromIsJunction && r.inputPos != fromPos)
			continue;

		fromRoad = r;
		fromRoadId = r.id;
		// if junction, the contact point is always at "end" of a road
		if (fromIsJunction)
			fromPos = "end";

		if (fromPos == "start")
		{
			fromX = r.geometries.front().x;
			fromY = r.geometries.front().y;
			fromHdg = r.geometries.front().hdg;
		}
		else if (fromPos == "end")
		{
			geometry g = r.geometries.back();
			curve(g.length, g, g.x, g.y, g.hdg, 1);
			fromX = g.x;
			fromY = g.y;
			fromHdg = g.hdg;
		}
	}

	// save to position
	for (auto &&r : data.roads)
	{


		if (r.junction != toSegment || r.inputId != toRoadId) 
			continue;
		if (toIsJunction && r.inputPos != toPos)
			continue;

		toRoad = r;
		toRoadId = r.id;

		// if junction, the contact point is always at "end" of a road
		if (toIsJunction)
			toPos = "end";

		if (toPos == "start")
		{
			toX = r.geometries.front().x;
			toY = r.geometries.front().y;
			toHdg = r.geometries.front().hdg;
		}
		else if (toPos == "end")
		{
			geometry g = r.geometries.back();
			toX = g.x;
			toY = g.y;
			toHdg = g.hdg;
			curve(g.length, g, toX, toY, toHdg, 1);
		}
	}

	// --- rotate and shift current road according to from position --------
	double dx, dy;

	// compute hdgOffset between the two segments
	double dPhi = fromHdg - toHdg + M_PI;
	if (fromPos == "start")
		dPhi += M_PI;
	if (toPos == "start")
		dPhi += M_PI;
	fixAngle(dPhi);

	// if toPos is end, the actual toPos has to be computed
	if (toPos == "end")
	{
		if(toRoad.id == -1){
			std::cerr << "ERR: 'Road linking is wrong!'" << std::endl;
			std::cerr << "    couldn't find toSegment " << toSegment << " or toRoadID " << toRoadId << std::endl;
			return -1;
		}
		geometry g = toRoad.geometries.back();
		toX = g.x * cos(dPhi) - g.y * sin(dPhi);
		toY = g.x * sin(dPhi) + g.y * cos(dPhi);
		toHdg = g.hdg + dPhi;
		curve(g.length, g, toX, toY, toHdg, 1);
	}

	// compute x / y offset between the two segments
	dx = fromX - toX;
	dy = fromY - toY;

	// shift all geometries which belong to the toSegment according two the offsets determined above
	for (auto &&r : data.roads)
	{
		if (r.junction != toSegment && r.roundAboutInputSegment != toSegment)
			continue;

		for (auto &&g : r.geometries)
		{
			double x = g.x * cos(dPhi) - g.y * sin(dPhi);
			double y = g.x * sin(dPhi) + g.y * cos(dPhi);

			g.x = x + dx;
			g.y = y + dy;
			g.hdg = g.hdg + dPhi;
			fixAngle(g.hdg);
		}
	}

	for (auto &&r : data.roads)
	{
		if (r.id == toRoadId){
			r.predecessor.id = fromRoadId;
			r.predecessor.contactPoint = (fromPos == "start") ? startType : endType ;
		}
		
		if (r.id == fromRoadId){
			r.successor.id = toRoadId;
			r.successor.contactPoint = (toPos == "start") ? startType : endType;
		}
	}
	return 0;
}