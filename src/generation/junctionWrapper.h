/**
 * Road-Generation
 * --------------------------------------------------------
 * Copyright (c) 2021 Institut für Kraftfahrzeuge, RWTH Aachen, ika
 * Report bugs and download new versions https://github.com/ika-rwth-aachen/RoadGeneration
 *
 * This library is distributed under the MIT License.
 * 
 * @file junctionWrapper.h
 *
 * @brief file contains method for generating junctions
 *
 * @author Christian Geller
 * Contact: christian.geller@rwth-aachen.de
 *
 */

#include "xjunction.h"
#include "tjunction.h"

/**
 * @brief function takes the junction as input argument and calls the junction type function
 * 
 * @param node  input data from the input file for the tjunction
 * @param data  roadNetwork structure where the generated roads and junctions are stored
 * @return int  error code
 */
int junctionWrapper(pugi::xml_node &node, roadNetwork &data)
{
    // check type of the junction (M = mainroad, A = accessroad)
    int mode = 0;
    if ((string)node.attribute("type").value() == "2M")
        mode = 1;
    if ((string)node.attribute("type").value() == "M2A")
        mode = 1;
    if ((string)node.attribute("type").value() == "4A")
        mode = 1;
    if ((string)node.attribute("type").value() == "MA")
        mode = 2;
    if ((string)node.attribute("type").value() == "3A")
        mode = 2;

    if (mode == 1)
    {
        if (xjunction(node, data))
        {
            cerr << "ERR: error in xjunction." << endl;
            return 1;
        }
    }
    if (mode == 2)
    {
        if (tjunction(node, data))
        {
            cerr << "ERR: error in tjunction." << endl;
            return 1;
        }
    }

    if (mode == 0)
    {
        cerr << "ERR: junction type is not defined correct." << endl;
        return 1;
    }
    return 0;
}