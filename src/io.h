// file io.h

/**
 * @brief function parses the input xml file with the external tool pugi_xml
 *  input in then accessable in a pugi::xml_document tree structure 
 * 
 * @param doc   tree structure which contains the data of the xml input file
 * @param data  roadNetwork data which will be stored as openDrive format
 * @param file  xml input file
 * @return int  errorcode
 */
int parseXML(pugi::xml_document &doc, roadNetwork &data, char * file)
{   
    if (true)
    {
        data.file = "bin/all.xml";
        doc.load_file("bin/all.xml");
        return 0;
    }

    data.file = file;
	if (doc.load_file(file)) 
    {
        return 0;
    }
    else {
        cerr << "ERR: InputFile not found" << endl;
        return 1;
    }
}

/**
 * @brief function stores the generated structure of type roadNetwork as a openDrive format
 * 
 * @param doc   tree structure which contains the generated data in openDrive format
 * @param data  generated data by the tool
 * @return int  errorcode
 */
int createXML(pugi::xml_document &doc, roadNetwork data)
{   
    pugi::xml_node root = doc.append_child("OpenDRIVE");

    root.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
    root.append_attribute("xsi:noNamespaceSchemaLocation") = "../xml/output.xsd";
    
    // write header
    pugi::xml_node header = root.append_child("header");
    header.append_attribute("revMajor") = 1;
    header.append_attribute("revMinor") = 5;

    // write roads
    for (std::vector<road>::iterator it = data.roads.begin() ; it != data.roads.end(); ++it)
    {
        pugi::xml_node road = root.append_child("road");
        
        road.append_attribute("length") = it->length;
        road.append_attribute("id") = it->id;
        road.append_attribute("junction") = it->junction;

    
        pugi::xml_node pre = road.append_child("link").append_child("predecessor");
        pre.append_attribute("elementType") = it->predecessor.elementType.c_str();
        pre.append_attribute("elementId") = it->predecessor.elementId;
        pre.append_attribute("contactPoint") = it->predecessor.contactPoint.c_str();
        
        pugi::xml_node suc = road.child("link").append_child("successor");
        suc.append_attribute("elementType") = it->successor.elementType.c_str();
        suc.append_attribute("elementId") = it->successor.elementId;
        suc.append_attribute("contactPoint") = it->successor.contactPoint.c_str();
        
        road.append_child("type").append_attribute("s") = "0";
        road.child("type").append_attribute("type") = "town";

        pugi::xml_node planView = road.append_child("planView");

        // write geometries
        for (std::vector<geometry>::iterator itt = it->geometries.begin() ; itt != it->geometries.end(); ++itt)
        {
            pugi::xml_node geo = planView.append_child("geometry");

            geo.append_attribute("s") = itt->s;
            geo.append_attribute("x") = itt->x;
            geo.append_attribute("y") = itt->y;
            geo.append_attribute("hdg") = itt->hdg;
            geo.append_attribute("length") = itt->length;

            if (itt->type == 1)
            {
                geo.append_child("line");
            }
            if (itt->type == 2)
            {
                geo.append_child("arc").append_attribute("curvature") = itt->c;
            }
            if (itt->type == 3)
            {
                pugi::xml_node spiral = geo.append_child("spiral");
                spiral.append_attribute("curvStart") = itt->c1;
                spiral.append_attribute("curvEnd") = itt->c2;
            }
        }
        
        pugi::xml_node lanes = road.append_child("lanes");

        // write lanes        
        for (std::vector<laneSection>::iterator itt = it->laneSections.begin() ; itt != it->laneSections.end(); ++itt)
        {
            pugi::xml_node laneSection = lanes.append_child("laneSection");
            laneSection.append_attribute("s") = itt->s;

            pugi::xml_node left;
            pugi::xml_node center;
            pugi::xml_node right;

            if(findMaxLaneId(*itt) > 0) left = laneSection.append_child("left");
            center = laneSection.append_child("center");
            if(findMinLaneId(*itt) < 0) right = laneSection.append_child("right");

            for (std::vector<lane>::iterator ittt = itt->lanes.begin() ; ittt != itt->lanes.end(); ++ittt)
            {
                pugi::xml_node lane;

                if (ittt->id > 0)
                {
                    lane = left.append_child("lane");
                }  
                if (ittt->id < 0)  
                {
                    lane = right.append_child("lane");
                } 
                if (ittt->id == 0) 
                {
                    lane = center.append_child("lane");
                }
                
                lane.append_attribute("id") = ittt->id;
                lane.append_attribute("type") = ittt->type.c_str();

                //pugi::xml_node link = lane.append_child("link");

                if (ittt->id != 0)
                {
                    pugi::xml_node width = lane.append_child("width");
                    width.append_attribute("sOffset") = ittt->w.s;
                    width.append_attribute("a") = ittt->w.a;
                    width.append_attribute("b") = ittt->w.b;
                    width.append_attribute("c") = ittt->w.c;
                    width.append_attribute("d") = ittt->w.d;
                }
                pugi::xml_node roadmark = lane.append_child("roadMark");

                roadmark.append_attribute("sOffset") = ittt->rm.s;
                roadmark.append_attribute("type") = ittt->rm.type.c_str();
                roadmark.append_attribute("weight") = ittt->rm.weight.c_str();
                roadmark.append_attribute("color") = ittt->rm.color.c_str();
                roadmark.append_attribute("width") = ittt->rm.width;
            }
        }

        pugi::xml_node objects = road.append_child("objects");
        // write objects        
        for (std::vector<object>::iterator itt = it->objects.begin() ; itt != it->objects.end(); ++itt)
        {
            object o = *itt;
            pugi::xml_node obj = objects.append_child("object");
            
            obj.append_attribute("type") = o.type.c_str();
            obj.append_attribute("name") = o.type.c_str();
            obj.append_attribute("dynamic") = "no";
            obj.append_attribute("id") = o.id;
            obj.append_attribute("s") = o.s;
            obj.append_attribute("t") = o.t;
            obj.append_attribute("zOffset") = o.z;
            obj.append_attribute("hdg") = o.hdg;
            obj.append_attribute("pitch") = 0;
            obj.append_attribute("roll") = 0;
            obj.append_attribute("validLength") = 0;
            obj.append_attribute("orientation") = o.orientation.c_str();
            obj.append_attribute("length") = o.length;
            obj.append_attribute("width") = o.width;
            obj.append_attribute("height") = o.height;

            if (o.repeat)
            {
                obj = obj.append_child("repeat");
                obj.append_attribute("s") = o.s;
                obj.append_attribute("length") = o.len;
                obj.append_attribute("distance") = o.distance;
                obj.append_attribute("tStart") = o.t;
                obj.append_attribute("tEnd") = o.t;
                obj.append_attribute("widthStart") = o.width;
                obj.append_attribute("widthEnd") = o.width;
                obj.append_attribute("heightStart") = o.height;
                obj.append_attribute("heightEnd") = o.height;
                obj.append_attribute("zOffsetStart") = o.z;
                obj.append_attribute("zOffsetEnd") = o.z;
                obj.append_attribute("lengthStart") = o.length;
                obj.append_attribute("lengthEnd") = o.length;  
            }
        }

        pugi::xml_node signals = road.append_child("signals");
        // write signals        
        for (std::vector<signal>::iterator itt = it->signals.begin() ; itt != it->signals.end(); ++itt)
        {
            signal s = *itt;
            //pugi::xml_node sig = signals.append_child("signal");
            pugi::xml_node sig = objects.append_child("object");
            
            sig.append_attribute("id") = s.id;
            
            sig.append_attribute("name") = s.type.c_str();
            //sig.append_attribute("type") = s.type.c_str();
            
            sig.append_attribute("subtype") = "-";
            sig.append_attribute("s") = s.s;
            sig.append_attribute("t") = s.t;
            sig.append_attribute("zOffset") = s.z;
            sig.append_attribute("orientation") = s.orientation.c_str();
            if (s.dynamic) sig.append_attribute("dynamic") = "yes";
            else sig.append_attribute("dynamic") = "no";
            sig.append_attribute("value") = s.value;
            sig.append_attribute("width") = s.width;
            sig.append_attribute("height") = s.height;
        }
    }
        
    // write junctions
    for (std::vector<junction>::iterator it = data.junctions.begin() ; it != data.junctions.end(); ++it)
    {
        pugi::xml_node junc = root.append_child("junction");

        junc.append_attribute("id") = it->id;

        for (std::vector<connection>::iterator itt = it->connections.begin() ; itt != it->connections.end(); ++itt)
        {
            pugi::xml_node con = junc.append_child("connection");

            con.append_attribute("id") = itt->id;
            con.append_attribute("incomingRoad") = itt->from;
            con.append_attribute("connectingRoad") = itt->to;
            con.append_attribute("contactPoint") = itt->contactPoint.c_str();

            pugi::xml_node lL = con.append_child("laneLink");
            lL.append_attribute("from") = itt->fromLane;
            lL.append_attribute("to") = itt->toLane;
        }
    }

    // write controllers
    for (std::vector<control>::iterator it = data.controller.begin() ; it != data.controller.end(); ++it)
    {
        pugi::xml_node controller = root.append_child("controller");

        controller.append_attribute("id") = it->id;

        for (std::vector<signal>::iterator itt = it->signals.begin() ; itt != it->signals.end(); ++itt)
        {
            pugi::xml_node con = controller.append_child("control");

            con.append_attribute("signalId") = itt->id;
        }
    }

    // PRINT: doc.print(std::cout);

    // write doc to file
    string file = data.file.substr(0,data.file.find(".xml"));
    file.append(".xodr");

    if (doc.save_file(file.c_str())) 
    {
        return 0;
    }
    else {
        cerr << "ERR: file could not be saved." << endl;
        return 1;
    }
}