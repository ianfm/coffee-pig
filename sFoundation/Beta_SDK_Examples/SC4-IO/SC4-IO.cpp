/*
 * Title: SC4-IO.cpp
 *
 * Objective:
 *    - Demonstrate how to control the two outputs on the SC4-HUB. These outputs are
 *      commonly used to control 24V Power Off Brakes, or as general purpose outputs.
 *	  - Demonstrate how to read the state of the Global Stop Input.
 *    - Demonstrate how to read the state of the two general purpose inputs available on each Node.
 *
 * Description:
 *	  This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    Next, the program asserts the 1st general purpose output (BRAKE_0) and print the current brake
 *    control mode of both outputs.
 *    Then, it print the state of the global stop input.
 *    Finally, the program will print the current state of inputs A, and B on each node before shutting down.
 *
 *
 * Requirements:
 *	1. Have at least 1 SC4-HUB connected to your computer via USB.
 *  2. Each port connected must have at least 1 ClearPath SC connected to it.
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

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);

int main(int argc, char* argv[]) {
	msgUser("GPIO Example starting. This example will turn on BRAKE_0 and read all inputs. Press Enter to start.");

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();	// Create System Manager myMgr

	try {
		// Search for ClearPath SC4-HUB's connected to this machine and store them in comHubPorts
		std::vector<std::string> comHubPorts;
		myMgr->FindComHubPorts(comHubPorts);

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
		printf("Port[%d]: state=%d, nodes=%d\n",
			myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

		// Sets the state of BRAKE_0 to be ON
		myPort.BrakeControl.BrakeSetting(0, GPO_ON);
		
		// Alternativley, you could use BRAKE_AUTOCONTROL mode (commented below)
		// When using this mode, BRAKE_0 will activate and deactivate based on the state of the Node 0.
		// Similarly, BRAKE_1 will correspond to Node 1.
		// myPort.BrakeControl.BrakeSetting(0, BRAKE_AUTOCONTROL);
		
		// Print the current control mode for each output
		for (size_t iBrake = 0; iBrake < 2; iBrake++) {
			size_t controlMode = myPort.BrakeControl.BrakeSetting(iBrake);
			printf("\nBrake %zi control mode: ", iBrake);
			switch (controlMode) {
			case BRAKE_AUTOCONTROL:
				printf("BRAKE_AUTOCONTROL");
				break;
			case BRAKE_PREVENT_MOTION:
				printf("BRAKE_PREVENT_MOTION");
				break;
			case BRAKE_ALLOW_MOTION:
				printf("BRAKE_ALLOW_MOTION");
				break;
			case GPO_ON:
				printf("GPO_ON");
				break;
			case GPO_OFF:
				printf("GPO_OFF");
				break;
			}
		}
		printf("\n\n");

		// Print out the state of the global stop input
		if (myPort.GrpShutdown.GetGlobalStopInputState()) { // TODO: should be GetGlobalStop not GetGlobalStopInputState - Issue #391
			printf("Global Stop Input is not asserted.\n\n");
		}
		else {
			printf("Global Stop Input is asserted.\n\n");
		}

		// Checking Inputs on each Node
		for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
			INode& theNode = myPort.Nodes(iNode);
			// Reading input A
			if (theNode.Status.RT.Value().cpm.InA) {
				printf("Node %zi Input A is asserted \n", iNode);
			} else {
				printf("Node %zi Input A is not asserted \n", iNode);
			}
			// Reading input B
			if (theNode.Status.RT.Value().cpm.InB) {
				printf("Node %zi Input B is asserted \n", iNode);
			} else {
				printf("Node %zi Input B is not asserted\n", iNode);
			}
		}
		printf("\n");

		// Close down all ports
		myMgr->PortsClose();
	}
	catch (mnErr& theErr) {	//This catch statement will intercept any error from the Class library
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

