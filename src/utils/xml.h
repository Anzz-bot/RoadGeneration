/**
 * Road-Generation
 * --------------------------------------------------------
 * Copyright (c) 2021 Institut für Kraftfahrzeuge, RWTH Aachen, ika
 * Report bugs and download new versions https://github.com/ika-rwth-aachen/RoadGeneration
 *
 * This library is distributed under the MIT License.
 * 
 * This file makes use of the pugixml library.
 * 
 * Copyright (c) 2006-2020 Arseny Kapoulkine
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
 * associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation the rights to use, 
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * @file xml.cpp
 *
 * @brief file contains the import and export, as well as the validation, of xml files
 *
 * @author Christian Geller
 * Contact: christian.geller@rwth-aachen.de
 *
 */

#include "pugixml.hpp"
#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/validators/common/Grammar.hpp>
#include <algorithm>
#include "xmlParser.h"

using namespace XERCES_CPP_NAMESPACE;
using namespace std;
using namespace xercesc;

extern settings setting;

/**
 * @brief function checks the input file against the corresponding input.xsd
 * 
 * @param file  input file
 * @return int  error code
 */
int validateInput(char *file)
{
    XMLPlatformUtils::Initialize();
    string schema = string_format("%s/xml/input.xsd", PROJ_DIR);
    const char *schema_file = schema.c_str();
    const char *xml_file = file;

    XercesDOMParser domParser;
    if (domParser.loadGrammar(schema_file, Grammar::SchemaGrammarType) == NULL)
    {
        cerr << "ERR: couldn't load schema" << endl;
        return 1;
    }

    domParser.setValidationScheme(XercesDOMParser::Val_Auto);
    domParser.setDoNamespaces(true);
    domParser.setDoSchema(true);
    domParser.setValidationConstraintFatal(true);

    domParser.parse(xml_file);

    if (domParser.getErrorCount() == 0){
        if(!setting.silentMode)
            cout << "XML input file validated against the schema successfully" << endl;
    }
    else
    {
        cerr << "ERR: XML input file doesn't conform to the schema" << endl;
        return 1;
    }

    return 0;
}

/**
 * @brief function checks the output file against the corresponding output.xsd
 * 
 * @param data  output data
 * @return int  error code
 */
int validateOutput(roadNetwork data)
{
    // setup file
    string file = data.outputFile;
    file.append(".xodr");
    const char *xml_file = file.c_str();

    XMLPlatformUtils::Initialize();

    string schema = string_format("%s/xml/output.xsd", PROJ_DIR);
    const char *schema_path = schema.c_str();

    // load output file
    XercesDOMParser domParser;
    if (domParser.loadGrammar(schema_path, Grammar::SchemaGrammarType) == NULL)
    {
        cerr << "ERR: couldn't load schema" << endl;
        return 1;
    }

    // check output file
    domParser.setValidationScheme(XercesDOMParser::Val_Auto);
    domParser.setDoNamespaces(true);
    domParser.setDoSchema(true);
    domParser.setValidationConstraintFatal(true);

    domParser.parse(xml_file);
    if (domParser.getErrorCount() == 0){
        if(!setting.silentMode)
            cout << "XML output file validated against the schema successfully" << endl;
    }
    else
    {
        cerr << "ERR: XML output file doesn't conform to the schema" << endl;
        return 1;
    }

    return 0;
}

/**
 * @brief function parses the input xml file with the external tool pugi_xml
 *  input is then accessable in a pugi::xml_document tree structure 
 * 
 * @param doc   tree structure which contains the data of the xml input file
 * @param data  roadNetwork data which holds the input file name
 * @param file  xml input file
 * @return int  error code
 */
int parseXML(pugi::xml_document &doc, roadNetwork &data, char *file)
{
    // save file name in data
    string f = file;
    data.file = f.substr(0, f.find(".xml"));

    if (doc.load_file(file))
    {
        return 0;
    }
    else
    {
        cerr << "ERR: InputFile not found" << endl;
        return 1;
    }
}


/**
 * @brief helper function to append the link node to road node
 * 
 * @param road the road xml node 
 * @param sucessor the potential successor to the road segment
 * @param sucessor the potential predecessor to the road segment * 
 */

void appendLinkToNode(pugi::xml_node road, link &successor, link &predecessor)
{
    if (successor.id == -1 && predecessor.id == -1){
        return;
    }
    pugi::xml_node link = road.append_child("link");
   
    if( predecessor.id != -1){
        pugi::xml_node pre = link.append_child("predecessor");
        pre.append_attribute("elementId") = predecessor.id;
        pre.append_attribute("elementType") = getLinkType(predecessor.elementType).c_str();
        if(predecessor.contactPoint != noneType)
            pre.append_attribute("contactPoint") = getContactPointType(predecessor.contactPoint).c_str();
    }

    if( successor.id != -1){
        pugi::xml_node suc = link.append_child("successor");
        suc.append_attribute("elementId") = successor.id;
        suc.append_attribute("elementType") = getLinkType(successor.elementType).c_str();
        if(successor.contactPoint != noneType)
            suc.append_attribute("contactPoint") = getContactPointType(successor.contactPoint).c_str();
    }
}

/**
 * @brief function stores the generated structure of type roadNetwork as OpenDrive format with the external tool pugi_xml
 * 
 * @param doc   tree structure which contains the generated output data in OpenDrive format
 * @param data  roadNetwork data filled by the road generation tool
 * @return int  error code
 */
int createXML(pugi::xml_document &doc, roadNetwork data)
{
    pugi::xml_node root = doc.append_child("OpenDRIVE");

    // specify location of .xsd file
    if (false)
    {
        root.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";
        string tmp = string_format("%s/xml/output.xsd", PROJ_DIR);
        root.append_attribute("xsi:noNamespaceSchemaLocation") = tmp.c_str();
    }

    // write header
    pugi::xml_node header = root.append_child("header");
    header.append_attribute("revMajor") = setting.versionMajor;
    header.append_attribute("revMinor") = setting.versionMinor;
    header.append_attribute("north") = setting.north;
    header.append_attribute("south") = setting.south;
    header.append_attribute("west") = setting.west;
    header.append_attribute("east") = setting.east;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // uncomment that for allowing CI, due to same output files
    //header.append_attribute("date") = oss.str().c_str();

    // geoReference tag
    pugi::xml_node geoReference = header.append_child("geoReference");
    geoReference.append_child(pugi::node_cdata).set_value("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");

    // --- write roads ---------------------------------------------------------
    for (std::vector<road>::iterator it = data.roads.begin(); it != data.roads.end(); ++it)
    {
        pugi::xml_node road = root.append_child("road");
        road.append_attribute("id") = it->id;
        road.append_attribute("length") = it->length;
       
        //below is a fix for the reuse of the junction attribute. Before this was just set to it->junction
        road.append_attribute("junction") = (it->isConnectingRoad) ? -1 : it->junction;
        appendLinkToNode(road, it->successor, it->predecessor);

        road.append_child("type").append_attribute("s") = "0";
        road.child("type").append_attribute("type") = it->type.c_str();

        // --- write geometries ------------------------------------------------
        pugi::xml_node planView = road.append_child("planView");

        for (std::vector<geometry>::iterator itt = it->geometries.begin(); itt != it->geometries.end(); ++itt)
        {
            pugi::xml_node geo = planView.append_child("geometry");

            geo.append_attribute("s") = (itt->s);
            geo.append_attribute("x") = (itt->x);
            geo.append_attribute("y") = (itt->y);
            geo.append_attribute("hdg") = (itt->hdg);
            geo.append_attribute("length") = (itt->length);

            if (itt->type == line)
            {
                geo.append_child("line");
            }
            if (itt->type == arc)
            {
                geo.append_child("arc").append_attribute("curvature") = (itt->c);
            }
            if (itt->type == spiral)
            {
                pugi::xml_node spiral = geo.append_child("spiral");
                spiral.append_attribute("curvStart") = (itt->c1);
                spiral.append_attribute("curvEnd") = (itt->c2);
            }
        }

        // --- write lanes -----------------------------------------------------
        pugi::xml_node lanes = road.append_child("lanes");

        for (std::vector<laneSection>::iterator itt = it->laneSections.begin(); itt != it->laneSections.end(); ++itt)
        {
            pugi::xml_node laneOffset = lanes.append_child("laneOffset");

            laneOffset.append_attribute("s") = (itt->s);
            laneOffset.append_attribute("a") = (itt->o.a);
            laneOffset.append_attribute("b") = (itt->o.b);
            laneOffset.append_attribute("c") = (itt->o.c);
            laneOffset.append_attribute("d") = (itt->o.d);
        }

        for (std::vector<laneSection>::iterator itt = it->laneSections.begin(); itt != it->laneSections.end(); ++itt)
        {
            pugi::xml_node laneSection = lanes.append_child("laneSection");
            laneSection.append_attribute("s") = (itt->s);

            pugi::xml_node left;
            pugi::xml_node center;
            pugi::xml_node right;

            if (findMaxLaneId(*itt) > 0)
                left = laneSection.append_child("left");
            center = laneSection.append_child("center");
            if (findMinLaneId(*itt) < 0)
                right = laneSection.append_child("right");

            std::sort(itt->lanes.begin(), itt->lanes.end(), compareLanes);
            for (std::vector<lane>::iterator ittt = itt->lanes.begin(); ittt != itt->lanes.end(); ++ittt)
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

                if (ittt->id != 0)
                {
                    pugi::xml_node link = lane.append_child("link");
                    if (ittt->preId != 0)
                    {
                        pugi::xml_node pre = link.append_child("predecessor");
                        pre.append_attribute("id") = ittt->preId;
                    }
                    if (ittt->sucId != 0)
                    {
                        pugi::xml_node suc = link.append_child("successor");
                        suc.append_attribute("id") = ittt->sucId;
                    }

                    pugi::xml_node width = lane.append_child("width");
                    width.append_attribute("sOffset") = (ittt->w.s);
                    width.append_attribute("a") = (ittt->w.a);
                    width.append_attribute("b") = (ittt->w.b);
                    width.append_attribute("c") = (ittt->w.c);
                    width.append_attribute("d") = (ittt->w.d);
                }

                pugi::xml_node roadmark = lane.append_child("roadMark");
                roadmark.append_attribute("sOffset") = (ittt->rm.s);
                roadmark.append_attribute("type") = ittt->rm.type.c_str();
                roadmark.append_attribute("weight") = ittt->rm.weight.c_str();
                roadmark.append_attribute("color") = ittt->rm.color.c_str();
                roadmark.append_attribute("width") = (ittt->rm.width);

                if (ittt->id != 0)
                {
                    pugi::xml_node material = lane.append_child("material");
                    material.append_attribute("sOffset") = (ittt->m.s);
                    material.append_attribute("surface") = ittt->m.surface.c_str();
                    material.append_attribute("friction") = (ittt->m.friction);
                    material.append_attribute("roughness") = (ittt->m.roughness);

                    pugi::xml_node speed = lane.append_child("speed");
                    speed.append_attribute("sOffset") = 0;
                    speed.append_attribute("max") = (ittt->speed);
                }
            }
        }

        // --- write objects ---------------------------------------------------
        pugi::xml_node objects = road.append_child("objects");

        std::sort(it->objects.begin(), it->objects.end(), compareObjects);
        for (std::vector<object>::iterator itt = it->objects.begin(); itt != it->objects.end(); ++itt)
        {
            object o = *itt;
            pugi::xml_node obj = objects.append_child("object");

            obj.append_attribute("type") = o.type.c_str();
            obj.append_attribute("name") = o.type.c_str();
            obj.append_attribute("dynamic") = "no";
            obj.append_attribute("id") = o.id;
            obj.append_attribute("s") = (o.s);
            obj.append_attribute("t") = (o.t);
            obj.append_attribute("zOffset") = (o.z);
            obj.append_attribute("hdg") = (o.hdg);
            obj.append_attribute("pitch") = 0;
            obj.append_attribute("roll") = 0;
            obj.append_attribute("validLength") = 0;
            obj.append_attribute("orientation") = o.orientation.c_str();
            obj.append_attribute("length") = (o.length);
            obj.append_attribute("width") = (o.width);
            obj.append_attribute("height") = (o.height);

            if (o.repeat)
            {
                obj = obj.append_child("repeat");
                obj.append_attribute("s") = (o.s);
                obj.append_attribute("length") = (o.len);
                obj.append_attribute("distance") = (o.distance);
                obj.append_attribute("tStart") = (o.t);
                obj.append_attribute("tEnd") = (o.t);
                obj.append_attribute("widthStart") = (o.width);
                obj.append_attribute("widthEnd") = (o.width);
                obj.append_attribute("heightStart") = (o.height);
                obj.append_attribute("heightEnd") = (o.height);
                obj.append_attribute("zOffsetStart") = (o.z);
                obj.append_attribute("zOffsetEnd") = (o.z);
                obj.append_attribute("lengthStart") = (o.length);
                obj.append_attribute("lengthEnd") = (o.length);
            }
        }

        // --- write signs ---------------------------------------------------

        // signs format is different in version 1.4
        if (setting.versionMajor >= 1 && setting.versionMinor >= 5)
        {
            pugi::xml_node signs = road.append_child("signals");

            std::sort(it->signs.begin(), it->signs.end(), compareSignals);
            for (std::vector<sign>::iterator itt = it->signs.begin(); itt != it->signs.end(); ++itt)
            {
                sign s = *itt;
                pugi::xml_node sig = signs.append_child("signal");
                //pugi::xml_node sig = objects.append_child("object");

                sig.append_attribute("id") = s.id;

                sig.append_attribute("name") = s.type.c_str();
                sig.append_attribute("type") = s.type.c_str();
                sig.append_attribute("subtype") = s.subtype.c_str();
                sig.append_attribute("country") = s.country.c_str();
                sig.append_attribute("s") = (s.s);
                sig.append_attribute("t") = (s.t);
                sig.append_attribute("zOffset") = (s.z);
                sig.append_attribute("orientation") = s.orientation.c_str();
                if (s.dynamic)
                    sig.append_attribute("dynamic") = "yes";
                else
                    sig.append_attribute("dynamic") = "no";
                sig.append_attribute("value") = (s.value);
                sig.append_attribute("width") = (s.width);
                sig.append_attribute("height") = (s.height);
            }
        }
    }

    // --- write controllers ---------------------------------------------------

    // controllers format is different in version 1.4
    if (setting.versionMajor >= 1 && setting.versionMinor >= 5)
    {
        for (std::vector<control>::iterator it = data.controller.begin(); it != data.controller.end(); ++it)
        {
            pugi::xml_node controller = root.append_child("controller");

            controller.append_attribute("id") = it->id;

            for (std::vector<sign>::iterator itt = it->signs.begin(); itt != it->signs.end(); ++itt)
            {
                pugi::xml_node con = controller.append_child("control");

                con.append_attribute("signalId") = itt->id;
            }
        }
    }

    // --- write junctions -----------------------------------------------------
    for (std::vector<junction>::iterator it = data.junctions.begin(); it != data.junctions.end(); ++it)
    {
        pugi::xml_node junc = root.append_child("junction");

        junc.append_attribute("id") = it->id;

        for (std::vector<connection>::iterator itt = it->connections.begin(); itt != it->connections.end(); ++itt)
        {
            pugi::xml_node con = junc.append_child("connection");

            con.append_attribute("id") = itt->id;
            con.append_attribute("incomingRoad") = itt->from;
            con.append_attribute("connectingRoad") = itt->to;
            con.append_attribute("contactPoint") = getContactPointType(itt->contactPoint).c_str();

            pugi::xml_node ll = con.append_child("laneLink");
            ll.append_attribute("from") = itt->fromLane;
            ll.append_attribute("to") = itt->toLane;
        }
    }

    // --- write doc structure to file -----------------------------------------
    string file = data.outputFile;
 
    file.append(".xodr");

    if (doc.save_file(file.c_str()))
    {
        return 0;
    }
    else
    {
        cerr << "ERR: file could not be saved." << endl;
        return 1;
    }
}


void appendLinkToNodeXercesC(nodeElement road, link &successor, link &predecessor)
{
    if (successor.id == -1 && predecessor.id == -1){
        return;
    }
    nodeElement link("link");
    link.appendToNode(road);

    if( predecessor.id != -1){
        nodeElement pre("predecessor");
        pre.appendToNode(link);
        pre.addAttribute("elementId", predecessor.id);
        pre.addAttribute("elementType", getLinkType(predecessor.elementType).c_str());
        if(predecessor.contactPoint != noneType)
            pre.addAttribute("contactPoint", getContactPointType(predecessor.contactPoint).c_str());
    }

    if( successor.id != -1){
        nodeElement suc("successor");
        suc.appendToNode(link);
        suc.addAttribute("elementId", successor.id);
        suc.addAttribute("elementType", getLinkType(successor.elementType));
        if(successor.contactPoint != noneType)
            suc.addAttribute("contactPoint", getContactPointType(successor.contactPoint));
    }
}


int createXMLXercesC(roadNetwork data)
{

    init("OpenDRIVE");

    DOMElement *root = getRootElement();

    nodeElement header("header");

    header.addAttribute("revMajor", to_string(setting.versionMajor).c_str());
    header.addAttribute("revMinor", to_string(setting.versionMinor).c_str());
    header.addAttribute("north", to_string(setting.north).c_str());
    header.addAttribute("south", to_string(setting.south).c_str());
    header.addAttribute("west", to_string(setting.west).c_str());
    header.addAttribute("east", to_string(setting.east).c_str());

    header.appendToNode(root);

     // geoReference tag
    nodeElement geoReference("geoReference");

    DOMCDATASection *cdata;
    generateCDATA("+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84 +units=m +no_defs", &cdata);
    geoReference.domelement->appendChild(cdata);
    geoReference.appendToNode(root);

     // --- write roads ---------------------------------------------------------
    for (std::vector<road>::iterator it = data.roads.begin(); it != data.roads.end(); ++it)
    {

        nodeElement road("road");
        road.appendToNode(root);
        road.addAttribute("id", it->id);
        road.addAttribute("length", it->length);
       
        //it has to be checked if it is a connecting road, since the junction attribute is missused as the original ID for connecting roads.
        road.addAttribute("junction" ,(it->isConnectingRoad) ? -1 : it->junction);
        appendLinkToNodeXercesC(road, it->successor, it->predecessor); //FIXME COMMENT IN AN IMPLEMENT XERCES PARSER


        nodeElement type("type");
        type.addAttribute("s", 0);
        type.addAttribute("type", it->type);
        type.appendToNode(road);


        // --- write geometries ------------------------------------------------
        nodeElement planView("planView");
        planView.appendToNode(road);

        for (std::vector<geometry>::iterator itt = it->geometries.begin(); itt != it->geometries.end(); ++itt)
        {
            nodeElement geo("geometry");
            geo.appendToNode(planView);

            geo.addAttribute("s", itt->s);
            geo.addAttribute("x", itt->x);
            geo.addAttribute("y", itt->y);
            geo.addAttribute("hdg", itt->hdg);
            geo.addAttribute("length", itt->length);

            if (itt->type == line)
            {
                nodeElement line("line");
                line.appendToNode(geo);
            }
            if (itt->type == arc)
            {
                nodeElement arc("arc");
                arc.addAttribute("curvature", itt->c);
                arc.appendToNode(geo);
            }
            if (itt->type == spiral)
            {
                nodeElement arc("spiral");
                arc.addAttribute("curvStart", itt->c1);
                arc.addAttribute("curvEnd", itt->c2);
                arc.appendToNode(geo);

            }
        }

        // --- write lanes -----------------------------------------------------
        nodeElement lanes("lanes");
        lanes.appendToNode(road);

        for (std::vector<laneSection>::iterator itt = it->laneSections.begin(); itt != it->laneSections.end(); ++itt)
        {
            nodeElement laneOffset("laneOffset");
            laneOffset.appendToNode(lanes);

            laneOffset.addAttribute("s", itt->s);
            laneOffset.addAttribute("a", itt->o.a);
            laneOffset.addAttribute("b", itt->o.b);
            laneOffset.addAttribute("c", itt->o.c);
            laneOffset.addAttribute("d", itt->o.d);
        }

        for (std::vector<laneSection>::iterator itt = it->laneSections.begin(); itt != it->laneSections.end(); ++itt)
        {
            nodeElement laneSection("laneSection");
            laneSection.appendToNode(lanes);
            laneSection.addAttribute("s", itt->s);

            nodeElement left("left");
            nodeElement center("center");
            nodeElement right("right");

            if (findMaxLaneId(*itt) > 0)
                left.appendToNode(laneSection);
            center.appendToNode(laneSection);
            if (findMinLaneId(*itt) < 0)
                right.appendToNode(laneSection);

            std::sort(itt->lanes.begin(), itt->lanes.end(), compareLanes);
            for (std::vector<lane>::iterator ittt = itt->lanes.begin(); ittt != itt->lanes.end(); ++ittt)
            {
                nodeElement lane("lane");

                if (ittt->id > 0)
                {
                    lane.appendToNode(left);
                }
                if (ittt->id < 0)
                {
                    lane.appendToNode(right);
                }
                if (ittt->id == 0)
                {
                    lane.appendToNode(center);
                }

                lane.addAttribute("id", ittt->id);
                lane.addAttribute("type", ittt->type);

                if (ittt->id != 0)
                {
                    nodeElement link("link");
                    link.appendToNode(lane);
                    if (ittt->preId != 0)
                    {
                        nodeElement pre ("predecessor");
                        pre.appendToNode(link);
                        pre.addAttribute("id", ittt->preId);
                    }
                    if (ittt->sucId != 0)
                    {
                        nodeElement suc("successor");
                        suc.appendToNode(link);
                        suc.addAttribute("id", ittt->sucId);
                    }

                    nodeElement width ("width");
                    width.appendToNode(lane);
                    width.addAttribute("sOffset", ittt->w.s);
                    width.addAttribute("a", ittt->w.a);
                    width.addAttribute("b", ittt->w.b);
                    width.addAttribute("c", ittt->w.c);
                    width.addAttribute("d", ittt->w.d);
                }

                nodeElement roadmark = ("roadMark");
                roadmark.appendToNode(lane);
                roadmark.addAttribute("sOffset", ittt->rm.s);
                roadmark.addAttribute("type", ittt->rm.type.c_str());
                roadmark.addAttribute("weight", ittt->rm.weight.c_str());
                roadmark.addAttribute("color", ittt->rm.color.c_str());
                roadmark.addAttribute("width", (ittt->rm.width));

                if (ittt->id != 0)
                {
                    nodeElement material("material");
                    material.appendToNode(lane);
                    material.addAttribute("sOffset", ittt->m.s);
                    material.addAttribute("surface", ittt->m.surface);
                    material.addAttribute("friction", ittt->m.friction);
                    material.addAttribute("roughness", ittt->m.roughness);

                    nodeElement speed("speed");
                    speed.appendToNode(lane);
                    speed.addAttribute("sOffset", 0);
                    speed.addAttribute("max", ittt->speed);
                }
            }
        }
    

        //write objects
        nodeElement objects("objects");
        objects.appendToNode(road);

        std::sort(it->objects.begin(), it->objects.end(), compareObjects);
        for (std::vector<object>::iterator itt = it->objects.begin(); itt != it->objects.end(); ++itt)
        {
            object o = *itt;
            nodeElement obj("object");
            obj.appendToNode(objects);
            obj.addAttribute("type", o.type);
            obj.addAttribute("name", o.type);
            obj.addAttribute("dynamic", "no");
            obj.addAttribute("id", o.id);
            obj.addAttribute("s", (o.s));
            obj.addAttribute("t", (o.t));
            obj.addAttribute("zOffset", (o.z));
            obj.addAttribute("hdg", (o.hdg));
            obj.addAttribute("pitch", 0);
            obj.addAttribute("roll", 0);
            obj.addAttribute("validLength", 0);
            obj.addAttribute("orientation", o.orientation);
            obj.addAttribute("length", o.length);
            obj.addAttribute("width", o.width);
            obj.addAttribute("height", o.height);

            if (o.repeat)
            {
                nodeElement objj("repeat");
                objj.appendToNode(obj);
                objj.addAttribute("s", o.s);
                objj.addAttribute("length", o.len);
                objj.addAttribute("distance", o.distance);
                objj.addAttribute("tStart", o.t);
                objj.addAttribute("tEnd", o.t);
                objj.addAttribute("widthStart", o.width);
                objj.addAttribute("widthEnd", o.width);
                objj.addAttribute("heightStart", o.height);
                objj.addAttribute("heightEnd", o.height);
                objj.addAttribute("zOffsetStart", o.z);
                objj.addAttribute("zOffsetEnd", o.z);
                objj.addAttribute("lengthStart", o.length);
                objj.addAttribute("lengthEnd", o.length);
            }
        }

        // --- write signs ---------------------------------------------------

        // signs format is different in version 1.4
        if (setting.versionMajor >= 1 && setting.versionMinor >= 5)
        {
            nodeElement signs("signals", road);

            std::sort(it->signs.begin(), it->signs.end(), compareSignals);
            for (std::vector<sign>::iterator itt = it->signs.begin(); itt != it->signs.end(); ++itt)
            {
                sign s = *itt;
                nodeElement sig("signal", signs);
                //pugi::xml_node sig = objects.append_child("object");

                sig.addAttribute("id", s.id);
                sig.addAttribute("name", s.type);
                sig.addAttribute("type", s.type);
                sig.addAttribute("subtype", s.subtype);
                sig.addAttribute("country", s.country);
                sig.addAttribute("s", s.s);
                sig.addAttribute("t", s.t);
                sig.addAttribute("zOffset", s.z);
                sig.addAttribute("orientation", s.orientation);
                if (s.dynamic)
                    sig.addAttribute("dynamic", "yes");
                else
                    sig.addAttribute("dynamic", "no");
                sig.addAttribute("value", s.value);
                sig.addAttribute("width", s.width);
                sig.addAttribute("height", s.height);
            }
        }
    }

    // --- write controllers ---------------------------------------------------

    // controllers format is different in version 1.4
    if (setting.versionMajor >= 1 && setting.versionMinor >= 5)
    {
        for (std::vector<control>::iterator it = data.controller.begin(); it != data.controller.end(); ++it)
        {
            nodeElement controller("controller");
            controller.appendToNode(root);
            controller.addAttribute("id", it->id);

            for (std::vector<sign>::iterator itt = it->signs.begin(); itt != it->signs.end(); ++itt)
            {
                nodeElement con("control", controller);
                con.addAttribute("signalId", itt->id);
            }
        }
    }

    // --- write junctions -----------------------------------------------------
    for (std::vector<junction>::iterator it = data.junctions.begin(); it != data.junctions.end(); ++it)
    {
        nodeElement junc("junction");
        junc.appendToNode(root);
        junc.addAttribute("id", it->id);

        for (std::vector<connection>::iterator itt = it->connections.begin(); itt != it->connections.end(); ++itt)
        {
            nodeElement con("connection", junc);

            con.addAttribute("id", itt->id);
            con.addAttribute("incomingRoad", itt->from);
            con.addAttribute("connectingRoad", itt->to);
            con.addAttribute("contactPoint", getContactPointType(itt->contactPoint));

            nodeElement ll("laneLink", con);
            ll.addAttribute("from", itt->fromLane);
            ll.addAttribute("to", itt->toLane);
        }
    }


    cout << (data.outputFile) << endl;
    serialize((data.outputFile + ".testxml").c_str());

    return 0;
}

/**
 * @brief function for displaying the road generation header name
 * 
 */
int printLogo()
{
    cout << "|‾\\  /‾\\  |‾‾| |‾\\       /‾‾  |‾‾  |\\  | |‾‾ |‾\\ |‾‾| ‾|‾ |  /‾\\  |\\  |" << endl;
    cout << "|_/ |   | |--| |  |  -  |  _  |--  | | | |-- |_/ |--|  |  | |   | | | |" << endl;
    cout << "| \\  \\_/  |  | |_/       \\_/  |__  |  \\| |__ | \\ |  |  |  |  \\_/  |  \\|" << endl;
    cout << "=======================================================================" << endl;

    return 0;
}