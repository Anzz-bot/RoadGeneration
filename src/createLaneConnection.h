// file createLaneConnection.h

/**
 * @brief function creates a new lane connection
 * 
 * @param r         connecting road which contains reference line
 * @param lS1       lanesection at start 
 * @param lS2       lanesection at end
 * @param from      start lane Id
 * @param to        end lane Id
 * @param left      left roadmarking 
 * @param right     right roadmarking
 * @return int      errorcode
 */
int createLaneConnection(road &r, laneSection lS1, laneSection lS2, int from, int to, string left, string right, int type)
{
       
    if (from == 0 || to == 0) {
        cout << "ERR: cannot connect lane Id 0." << endl;
    }
    
    int dir = sgn(to);

    lane l1;
    lane l2;
    findLane(lS1, l1, from);
    findLane(lS2, l2, to);

    // calculate tOffsets and widths
    double tOffSet1 = abs(findTOffset(lS1,from-sgn(from),0));
    double tOffSet2 = abs(findTOffset(lS2,to-sgn(to),0));

    double w1 = laneWidth(l1,0);
    double w2 = laneWidth(l2,0);

    // check if center already contained in laneSection
    bool foundCenter = false;
    for (int i = 0; i < r.laneSections.back().lanes.size(); i++)
    {
        if (r.laneSections.back().lanes[i].id == 0) 
        {
            foundCenter = true;
            r.laneSections.back().lanes[i].rm.type = "none";
            break;
        }
    }
    
    lane center;
    center.id = 0;
    center.type = "none";
    center.w.a = 0;

    lane helpLane;
    helpLane.id = dir;
    helpLane.type = "none";
    
    lane newLane;
    newLane.id = type * dir;

    if (dir > 0)
    {
        if (type == 1) center.rm.type = right;
        if (type == 2) center.rm.type = "none";
        helpLane.rm.type = right;
        newLane.rm.type = left;
    }
    else if (dir < 0)
    {
        if (type == 1) center.rm.type = left;
        if (type == 2) center.rm.type = "none";
        helpLane.rm.type = left;
        newLane.rm.type = right;
    }

    // width of helplane
    helpLane.w.d = -2 * (tOffSet2 - tOffSet1) / pow(r.length,3);
    helpLane.w.c =  3 * (tOffSet2 - tOffSet1) / pow(r.length,2);
    helpLane.w.b = 0;
    helpLane.w.a = tOffSet1;

    // width of newLane
    newLane.w.d = -2 * (w2 - w1) / pow(r.length,3);
    newLane.w.c =  3 * (w2 - w1) / pow(r.length,2);
    newLane.w.b = 0;
    newLane.w.a = w1;

    // add lanes to laneSection
    if (!foundCenter) 
    {
        r.laneSections.back().lanes.push_back(center);
    }

    if (type == 2)
        r.laneSections.back().lanes.push_back(helpLane);
    
    r.laneSections.back().lanes.push_back(newLane);

    return 0;
}