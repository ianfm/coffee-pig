/*
 * Title: LoadingConfigFile.cpp
 *
 * Objective:
 *    Demonstrate how to load a configuration file onto a single Node.
 *
 * Description:
 *    This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    Next, the program will ensure it has full access to the node.
 *    The program will load the configuration file defined by the user editable macro: CONFIGURATION_FILEPATH
 *    By default, this path is a relative path in the same directory as this program, "./myConfig.mtr".
 * 
 *    To save a configuration file, open ClearView, click on the menu File->Save Configuration.
 *
 * Requirements:
 *	1. Have 1 SC4-HUB connected to your computer via USB.
 *  2. That port connected must have 1 ClearPath SC motor connected to it.
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
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;

// User Editable Macros
#define CONFIGURATION_FILEPATH "./myConfig.mtr" // This is a relative path to a file called myConfig.mtr in the same directory as this program
// You could use a path like "C:/Users/<USERNAME>/Desktop/myConfig.mtr" to use a configuration file on your Windows Desktop. 

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);

int main(int argc, char* argv[]){
	msgUser("Loading Configuration File Example starting. No motion will occur. Press Enter to start.");

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();	//Create System Manager myMgr

	try {
		// Search for ClearPath SC4-HUB's connected to this machine and store them in comHubPorts
		std::vector<std::string> comHubPorts;
		SysManager::FindComHubPorts(comHubPorts);

		if (comHubPorts.empty()) {
			printf("No SC4-HUB's found. Ensure your SC4-HUB is connected to your computer via USB and has 24V power.");
			return 1;
		}

		// Define port 0 as the first SC4-HUB we found above
		myMgr->ComHubPort(0, comHubPorts[0].c_str());

		// Tell the system to open this one port
		myMgr->PortsOpen(1);

		// Create a useful reference to the first port object
		IPort& myPort = myMgr->Ports(0);
		printf(" Port[%d]: state=%d, nodes=%d\n",
			myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

		// Create a useful referene to the first node on myPort
		INode& myNode = myPort.Nodes(0);
		printNodeInfo(myNode);

		/*
		* Ensure we have full access to the Node.
		* The only time a node may not be in full access mode is if ClearView is connected through the USB hub 
		* on the motor and the user has taken Full Access through the USB diagnostic port via ClearView.
		*/ 
		if (!myNode.Setup.AccessLevelIsFull()) {
			printf("We do not have full access to Node[0], please close ClearView or set Clearview to Monitor Mode before continuing...\n");
			myMgr->PortsClose();
			return 1;
		}
		printf("We have full access to the Node! Proceeding...\n");

		printf("Loading a configuration file onto Node[0]\n");
		printf("Configuration File Path = %s\n", CONFIGURATION_FILEPATH);
		myNode.Setup.ConfigLoad(CONFIGURATION_FILEPATH);
		printf("Configuration Loaded!\n");

		// Close down the port
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
	return 0; // End program
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
