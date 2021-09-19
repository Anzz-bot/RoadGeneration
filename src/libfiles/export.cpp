/**
 * Road-Generation
 * --------------------------------------------------------
 * Copyright (c) 2021 Institut für Kraftfahrzeuge, RWTH Aachen, ika
 * Report bugs and download new versions https://github.com/ika-rwth-aachen/RoadGeneration
 *
 * This library is distributed under the MIT License.
 * 
 * @file export.cpp
 *
 * @brief This file contains the most important functions for using the road generation library
 *
 * @author Jannik Busse
 * Contact: jannik.busse@rwth-aachen.de
 *
 */
#include "export.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <ctime>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#endif

std::string::size_type st;
using namespace std;

#include "utils/settings.h"
#include "utils/interface.h"
#include "utils/helper.h"
#include "utils/xml.h"
#include "generation/buildSegments.h"
#include "connection/linkSegments.h"
#include "connection/closeRoadNetwork.h"

settings setting;

EXPORTED void setFileName(char* file){
	_fileName = file;
}

EXPORTED void setLogFile(char* file){
	_logfile = file;
}

EXPORTED void setOutputName(char* outName){
	_outName = outName;
	_setOutput = true;
}

EXPORTED int execPipeline(){
	return executePipeline(_fileName);
}

EXPORTED void setSilentMode(bool sMode){
	setting.silentMode = sMode;
}


EXPORTED void setXMLSchemaLocation(char* file){
	setting.xmlSchemaLocation = file;
}

EXPORTED int executePipeline(char* file)
{

	if (file == NULL){
		cout << "ERR: no file has been provided!" << endl;
		return -1;
	}

	if(!_setOutput){
		_outName = file;
	}
	
	(void)! freopen(_logfile.c_str(), "a", stderr); //(void)! suppresses the unused return warning..
	cerr << "\nError log for run with attribute: " << file << endl;

	if (setting.xmlSchemaLocation == ""){
		cerr << "ERROR; NOT SET" << endl;
		return -1;
	}

	if(!setting.silentMode){
		cout << file << endl;
		printLogo();
	}
	
	// --- initialization ------------------------------------------------------
	pugi::xml_document in;
	pugi::xml_document out;
	roadNetwork data;

	string outputFile = _outName;
	data.outputFile = outputFile.substr(0, outputFile.find(".xml"));
    data.outputFile = data.outputFile.substr(0, outputFile.find(".xodr"));
	
	// --- pipeline ------------------------------------------------------------
	if (validateInput(file))
	{
		cerr << "ERR: error in validateInput" << endl;
		return -1;
	}
	if (parseXML(in, data, file))
	{
		cerr << "ERR: error in parseXML" << endl;
		return -1;
	}
	if (buildSegments(in, data))
	{
		cerr << "ERR: error in buildSegments" << endl;
		return -1;
	}
	if (linkSegments(in, data))
	{
		cerr << "ERR: error in linkSegments" << endl;
		return -1;
	}
	if (closeRoadNetwork(in, data))
	{
		cerr << "ERR: error in closeRoadNetwork" << endl;
		return -1;
	}
	if (createXML(out, data))
	{
		cerr << "ERR: error during createXML" << endl;
		return -1;
	}

	cout << "TEST XML OUT" <<endl;

	createXMLXercesC(data);

	if (validateOutput(data))
	{
		cerr << "ERR: error in validateOutput" << endl;
		return -1;
	}
	
	return 0;
}
