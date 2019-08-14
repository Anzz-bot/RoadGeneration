// file buildSegments.h

/**
 * @brief function links all specified segments 
 *  the reference frame has to be specified in the input file
 * 	each segment can be linked by determine two connecting roads in the input file 
 * 
 * @param doc 	tree structure which contains the input data
 * @param data 	roadNetwork data where the openDrive structure should be generated
 * @return int 	errorcode
 */
int linkSegments(pugi::xml_document &doc, roadNetwork &data)
{
	pugi::xml_node interfaces = doc.child("roadNetwork").child("interfaces");

	if(!interfaces) 
	{
		cerr << "ERR: 'interfaces' are not specified in input file."  << endl;
		cerr << "\t -> skip segment linking."  << endl;
		return 0;
	}

	// define reference system
	int referenceId = interfaces.attribute("referenceId").as_int();
	int angleOffset = interfaces.attribute("angleOffset").as_int();
	int xOffset = interfaces.attribute("xOffset").as_int();
	int yOffset = interfaces.attribute("yOffset").as_int();

	for(auto&& r : data.roads)
	{
		if (r.junction != referenceId) continue;

		for (auto&& g : r.geometries)
		{
			double x = g.x * cos(angleOffset) - g.y * sin(angleOffset);
			double y = g.x * sin(angleOffset) + g.y * cos(angleOffset);

			g.x = x + xOffset;
			g.y = y + yOffset;
			g.hdg = g.hdg + angleOffset;
		}
	}

	// add other specified segments
	for (pugi::xml_node segmentLink : interfaces.children("segmentLink"))
	{
		int fromSegment = segmentLink.attribute("fromSegment").as_int();
		int toSegment = segmentLink.attribute("toSegment").as_int();
		int fromRoad = segmentLink.attribute("fromRoad").as_int();
		int toRoad = segmentLink.attribute("toRoad").as_int();	
		string fromPos = (string)segmentLink.attribute("fromPos").value();
		string toPos = (string)segmentLink.attribute("toPos").value();

		// we assume that "fromSegement" was already linked to referenceframe

		double fromX,fromY,fromHdg;
		double toX,toY,toHdg;

		// save from position
		for(auto&& r : data.roads)
		{
			if (r.junction != fromSegment || r.id != fromRoad) continue;

			r.successor.elementId = toRoad;

			if (fromPos == "start")
			{
				fromX = r.geometries.front().x;
				fromY = r.geometries.front().y;
				fromHdg = r.geometries.front().hdg;
			}
			else if (fromPos == "end")
			{
				geometry g = r.geometries.back();
				curve(g.length,g, g.x, g.y, g.hdg, 1);
				fromX = g.x;
				fromY = g.y;
				fromHdg = g.hdg;
			}
			else
			{
				cerr << "ERR: wrong position for fromPos is specified." << endl;
				cerr << "\t -> use 'start' or 'end'" << endl;
				return 1;
			}
		}

		// save to position
		for(auto&& r : data.roads)
		{
			if (r.junction != toSegment || r.id != toRoad) continue;

			r.predecessor.elementId = fromRoad;

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
				curve(g.length,g, toX, toY, toHdg, 1);				
			}
			else
			{
				cerr << "ERR: wrong position for toPos is specified." << endl;
				cerr << "\t -> use 'start' or 'end'" << endl;
				return 1;
			}
			
			// rotate and shift current road according to from position
			double dx,dy;
			
			// compute angleOffset between the two segments
			double dPhi = fromHdg - toHdg + M_PI;
			if (fromPos == "start") dPhi += M_PI;
			if (toPos == "start") dPhi += M_PI;
			fixAngle(dPhi);

			if (toPos == "end")
			{
				geometry g = r.geometries.back();
				toX = g.x * cos(dPhi) - g.y * sin(dPhi);
				toY = g.x * sin(dPhi) + g.y * cos(dPhi);
				toHdg = g.hdg + dPhi;
				curve(g.length,g, toX, toY, toHdg, 1);	
			}
			
			// compute x/y offset between the two segments
			dx = fromX - toX;
			dy = fromY - toY;

			
			// shift all geometries which belong to the toSegment according two the offsets determined above
			for(auto&& r2 : data.roads)
			{
				if (r2.junction != toSegment) continue;
			
				for (auto&& g : r2.geometries)
				{
					double x = g.x * cos(dPhi) - g.y * sin(dPhi);
					double y = g.x * sin(dPhi) + g.y * cos(dPhi);

					g.x = x + dx;
					g.y = y + dy;
					g.hdg = g.hdg + dPhi;
					fixAngle(g.hdg);
				}	
			}
		}
	}
    return 0;
}