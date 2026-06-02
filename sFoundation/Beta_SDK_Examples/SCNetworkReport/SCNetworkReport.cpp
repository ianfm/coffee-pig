/*
 * Title: SCNetworkReport.cpp
 *
 * Objective:
 *    - Show information about all Ports and Nodes on a network.
 *	  - Demonstrate how to create an array (vector) of Nodes.	
 *
 * Description:
 *	  This program will discover SC4-HUBs connected to your computer.
 *    For each SC4-HUB (Port) the program will discover all Nodes connected to it.
 *    Each Node found will be added to a list of nodes for that port and information
 *    about that Node will be printed to the console.
 *
 *
 * Requirements:
 *	1. Have 1-3 SC4-HUB(s) connected to your computer via USB.
 *     If you are using RS-232, automatic discovery of SC4-HUBs will not work. You must
 *     manually specify the COM port you are connecting to. See comment below FindComHubPorts.
 *  2. Each SC4-HUB connected must have at least 1 ClearPath SC motor connected to it
 *
 * Links:
 * ** ClearPath SC User Manuals:
 * ** - NEMA 23 and 34 IP53 Models: https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual.pdf
 * ** - NEMA 23 and 34 IP67/66K Sealed Models: https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual_ip67.pdf
 * ** - NEMA 56/143 and IEC D100 Models: https://teknic.com/files/downloads/ac_clearpath-sc_manual.pdf
 * TODO: Add link to web documentation
 *
 */

// Include files
#include <stdio.h>	
#include <functional>
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);
void discoverNodes(IPort&, std::vector<std::reference_wrapper<INode>>);

int main(int argc, char* argv[]) {
	msgUser("SC Network Report starting. No motion will occur. Press Enter to start.");

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();	//Create System Manager myMgr

	try {
		// Search for ClearPath SC4-HUBs connected to this machine and store them in comHubPorts
		std::vector<std::string> comHubPorts;
		printf("Discovering all SC4-HUBs connected to this system...\n");
		SysManager::FindComHubPorts(comHubPorts);
		
		// Note: FindComHubPorts will not work with RS-232, only USB
		// For RS-232 Setup use the line below as a reference
		// myMgr->ComHubPort(0, 1); // Defining COM-1 as net number 0

		if (comHubPorts.empty()) {
			printf("No SC4-HUB's found. Ensure at least 1 SC4-HUB is connected to your computer via USB and has 24V power.");
			return 1;
		}

		// Iterate through each COM Port we found with an SC4-HUB
		// Note: sFoundation can communicate with up to 3 Ports.
		for (int port = 0; port < comHubPorts.size(); port++) {
			myMgr->ComHubPort(port, comHubPorts[port].c_str()); // Define port with SystemManager
		}

		// Open each of these ports
		myMgr->PortsOpen(comHubPorts.size());

		printf("\nDiscovering all Nodes connected to Port 0\n");
		IPort& myPort0 = myMgr->Ports(0); // create a reference to Port 0
		
		// Create an array to hold the references to all Nodes on Port 0
		std::vector<std::reference_wrapper<INode>> port0Nodes;
		
		// populate the array and print the information about each node
		for (int nodeIndex = 0; nodeIndex < myPort0.NodeCount(); nodeIndex++) {
			INode& node = myPort0.Nodes(nodeIndex);
			port0Nodes.push_back(std::ref(node)); // add the node reference to the array
			printNodeInfo(node);
		}

		// If we have more than 1 Port connected let's discover the Nodes on Port 1 as well
		if (comHubPorts.size() > 1) {
			printf("\nDiscovering all Nodes connected to Port 1\n");
			IPort& myPort1 = myMgr->Ports(1); // create a reference to Port 1

			// Create an array to hold the references to all Nodes on Port 1
			std::vector<std::reference_wrapper<INode>> port1Nodes;

			// populate the array and print the information about each node
			for (int nodeIndex = 0; nodeIndex < myPort1.NodeCount(); nodeIndex++) {
				INode& node = myPort1.Nodes(nodeIndex);
				port1Nodes.push_back(std::ref(node)); // add the node reference to the array
				printNodeInfo(node);
			}
		}

		// Lastly, if we have more than 2 Ports connected let's discover the Nodes on Port 2 as well
		if (comHubPorts.size() > 2) {
			printf("\nDiscovering all Nodes connected to Port 2\n");
			IPort& myPort2 = myMgr->Ports(2); // create a reference to Port 2

			// Create an array to hold the references to all Nodes on Port 2
			std::vector<std::reference_wrapper<INode>> port2Nodes;

			// populate the array and print the information about each node
			for (int nodeIndex = 0; nodeIndex < myPort2.NodeCount(); nodeIndex++) {
				INode& node = myPort2.Nodes(nodeIndex);
				port2Nodes.push_back(std::ref(node)); // add the node reference to the array
				printNodeInfo(node);
			}
		}

		// Close all of the ports
		myMgr->PortsClose();
	}
	catch (mnErr& theErr)	//This catch statement will intercept any error from the Class library
	{
		switch (theErr.ErrorCode) {
		case MN_ERR_PORT_FAILED_PORTS_0:
			printf("Port failed to open. Ensure that ClearView or any other application is not using the port.\n");
			break;
		case MN_ERR_INIT_FAILED_PORTS_0:
			printf("Port failed to initialize. Ensure that a ClearPath SC motor is powered and connected to the SC4-HUB and the jumpers are installed correctly on the SC4-HUB\n");
			break;
		}

		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
		myMgr->PortsClose();
		return 1;  //This terminates the main program
	}

	msgUser("Press any key to exit.");
	return 0; //End program
}

// Send message and wait for newline
void msgUser(const char* msg) {
	std::cout << msg;
	getchar();
}

// Print information about a given node
void printNodeInfo(INode& node) {
	printf("   Node[%d]: type=%d\n", node.Info.Ex.NodeIndex(), node.Info.NodeType());
	printf("            userID: %s\n", node.Info.UserID.Value());
	printf("        FW version: %s\n", node.Info.FirmwareVersion.Value());
	printf("          Serial #: %d\n", node.Info.SerialNumber.Value());
	printf("             Model: %s\n", node.Info.Model.Value());
}
