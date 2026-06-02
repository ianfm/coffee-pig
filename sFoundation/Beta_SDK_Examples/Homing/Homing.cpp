/*
 * Title: Homing.cpp
 *
 * Objective:
 *   - Demonstrate how to initiate the homing procedure on a Node
 *   - Demonstrate how to tell if a Node was homed already
 *
 * Description:
 *   This program finds SC4-HUBs connected to this computer and opens the first port found.
 *   Next, the program enables the first Node, then checks to see if homing was setup on that Node.
 *   
 *   To setup homing: open ClearView, connect to your Motor, click Setup->Homing... in the top menu bar.
 *	 
 *   If homing was setup, then the program prints whether the Node was homed already.
 *   Then, the program will initiate the homing process (even if the Node was homed already).
 *   Once the Node is homed, the program prints out its current position.
 *
 * Requirements:
 *	1. Have 1 SC4-HUB connected to your computer via USB.
 *  2. That port connected must have 1 ClearPath SC motor connected to it.
 *  3. The ClearPath SC connected should be unloaded with factory default settings.
 *  4. Homing should be configured on the ClearPath SC motor via ClearView before running.
 *
 * Links:
 * ** ClearPath SC User Manuals:
 * ** - NEMA 23 and 34 IP53 Models: https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual.pdf
 * ** - NEMA 23 and 34 IP67/66K Sealed Models: https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual_ip67.pdf
 * ** - NEMA 56/143 and IEC D100 Models: https://teknic.com/files/downloads/ac_clearpath-sc_manual.pdf
 * TODO: Add link to web documentation
 *
 */

 //Required include files
#include <stdio.h>	
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;

// User Editable Macros 
#define CHANGE_NUMBER_SPACE	2000	// The change to the numberspace after homing (cnts)
#define HOMING_TIMEOUT	    30000	// (ms) Ensure this duration is longer than the time it takes to perform your longest homing sequence.

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);

int main(int argc, char* argv[]) {
	msgUser("Homing Example starting. Motion will occur on Node 0. Press Enter to start.");

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance(); //Create System Manager myMgr

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

		// Create a useful reference to the first node on myPort
		INode& myNode = myPort.Nodes(0);
		printNodeInfo(myNode);

		// Clear potential alerts and NodeStops before trying to enable the node
		printf("Clearing any NodeStops then Alerts...\n");
		myNode.Motion.NodeStopClear();
		myNode.Status.AlertsClear();

		// Enable the node
		printf("Enabling node...\n");	
		myNode.EnableReq(true);

		// Wait for node to be ready after enable...
		double enableTimeout = myMgr->TimeStampMsec() + 10000;	//define a timeout in case the node is unable to enable
		//This will loop checking on the Real time values of the node's Ready status
		while (!myNode.Motion.IsReady()) {
			if (myMgr->TimeStampMsec() > enableTimeout) {
				printf("Error: Timed out waiting for Node 0 to enable\n");
				myMgr->PortsClose();
				msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
				return 1;
			}
		}
		printf("Node enabled successfully.\n");

		// At this point the Node is enabled, and we will now check to see if homing has been setup properly
		if (myNode.Motion.Homing.HomingValid()) {
			// Check the Node to see if it has already been homed
			if (myNode.Motion.Homing.WasHomed()) {
				myNode.Motion.PosnMeasured.Refresh(); //Refresh our current measured position
				printf("Node has already been homed, current position is: %.0f\n", myNode.Motion.PosnMeasured.Value());
				printf("Rehoming Node... \n");
			}
			else {
				printf("Node has not been homed.  Homing Node now...\n");
			}
			//Now we will home the Node
			myNode.Motion.Homing.Initiate();

			double homingTimeout = myMgr->TimeStampMsec() + HOMING_TIMEOUT;	//define a timeout in case the node is unable to complete homing
			// Poll for WasHomed
			while (!myNode.Motion.Homing.WasHomed()) {
				myNode.Status.RT.Refresh(); // Refresh Real-Time status register
				// If an alert is present or the move was canceled or we reached our timeout
				if (myNode.Status.RT.Value().cpm.AlertPresent || myNode.Status.RT.Value().cpm.MoveCanceled || myMgr->TimeStampMsec() > homingTimeout) {
					printf("Node did not complete homing:\n");
					if (myNode.Status.RT.Value().cpm.AlertPresent) {
						printf("\tAn alert is present - Check for alerts / Shutdowns\n");
					} else if (myNode.Status.RT.Value().cpm.MoveCanceled) {
						printf("\tThe move was canceled.\n");
					} else if (myMgr->TimeStampMsec() > homingTimeout) {
						printf("\tThe timeout was reached - Ensure HOMING_TIMEOUT is longer than the longest possible homing move.\n");
					}

					printf("Disabling node, and closing port\n");
					myNode.EnableReq(false);
					myMgr->PortsClose();
					msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
					return 1;
				}
			}
			myNode.Motion.PosnMeasured.Refresh(); //Refresh our current measured position
			printf("Node completed homing, current position: %.0f\n", myNode.Motion.PosnMeasured.Value());
			printf("Soft limits now active, if configured.\n");
			
			// If you need to change the measured position to a different number after homing: reference the code below		
			/*
			printf("Adjusting Numberspace by %d\n", CHANGE_NUMBER_SPACE);
			
			// This function is used mostly with a manual homing process, or anytime an application need to adjust the number space.
			// It allows the measured and commanded position to be shifted by the specified amount without moving the motor shaft.
			myNode.Motion.AddToPosition(CHANGE_NUMBER_SPACE); //Now the node is no longer considered "homed", and soft limits are turned off
			
			myNode.Motion.Homing.SignalComplete(); //reset the Node's "sense of home" soft limits (unchanged) are now active again
			
			myNode.Motion.PosnMeasured.Refresh(); //Refresh our current measured position
			printf("Numberspace changed, current position: %.0f\n", myNode.Motion.PosnMeasured.Value());
			*/
		}
		else {
			printf("Node has not had homing setup through ClearView.  The node will not be homed.\n");
			printf("To setup homing: open ClearView, connect to your Motor, click Setup->Homing... in the top menu.\n");
		}

		printf("Disabling node, and closing port\n");
		myNode.EnableReq(false);
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

