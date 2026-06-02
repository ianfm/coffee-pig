/*
 * Title: MotionVelocity.cpp
 *
 * Objective:
 *    - Demonstrate how to execute sequential velocity moves on a single Node.
 *
 * Description:
 *    This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    Next, the program enables the first Node, then commands a series of velocity moves to that motor in a both directions.
 *    The program will wait for each velocity to be reached before executing the next move.
 * 
 *    The Acceleration Limit (rpm/s) of the Node can be modified using macro ACC_LIM_RPM_PER_SEC
 * 
 *    The Velocity units are set to RPM by default.
 *
 * Requirements:
 *	1. Have 1 SC4-HUB connected to your computer via USB.
 *  2. That port connected must have 1 ClearPath SC motor connected to it.
 *  3. The ClearPath SC connected should be unloaded with factory default settings.
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
#define ACC_LIM_RPM_PER_SEC	1000 // Acceleration Limit in RPM/s for Node 0

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);
bool moveAtVelocity(INode&, int);

int main(int argc, char* argv[]) {
	msgUser("Velocity Moves Example starting. Motion will occur on Node 0 with speeds up to 500RPM. Press Enter to start.");

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

		// Configure acceleration units and limits
		myNode.AccUnit(INode::RPM_PER_SEC);				//Set the units for Acceleration to RPM/SEC
		myNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;   //Set Acceleration Limit (RPM/Sec)

		// Set the units for Velocity to RPM
		// Note: The target parameter in the MoveVelStart function uses these units.
		myNode.VelUnit(INode::RPM);	
		
		// Clear potential alerts and NodeStops before trying to enable the node
		printf("Clearing any alerts...\n");
		myNode.Motion.NodeStopClear();
		myNode.Status.AlertsClear();

		// Enable the node
		printf("Enabling node...\n");
		myNode.EnableReq(true);

		// Wait for node to be ready after enable...
		double timeout = myMgr->TimeStampMsec() + 10000;	//define a timeout in case the node is unable to enable
		//This will loop checking on the Real time values of the node's Ready status
		while (!myNode.Motion.IsReady()) {
			if (myMgr->TimeStampMsec() > timeout) {
				printf("Error: Timed out waiting for Node 0 to enable\n");
				myMgr->PortsClose();
				msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
				return 1;
			}
		}
		printf("Node enabled successfully.\n");

		printf("\nStarting velocity moves...\n");
		// Move at 60rpm for 2000ms
		moveAtVelocity(myNode, 60);
		myMgr->Delay(2000);
		// Move at -250rpm for 2000ms
		moveAtVelocity(myNode, -250);
		myMgr->Delay(2000);
		// Move at 500rpm for 2000ms
		moveAtVelocity(myNode, 500);
		myMgr->Delay(2000);
		// Move at -500rpm for 2000ms
		moveAtVelocity(myNode, -500);
		myMgr->Delay(2000);
		// Command a 0rpm velocity to stop motion for 2000ms
		moveAtVelocity(myNode, 0);
		myMgr->Delay(2000);
		printf("\nFinished velocity moves.\n");
		
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

/**
* Command a given node to start moving at a target velocity and waits for the motor to reach the velocity
* This function will also monitor the motor's status register in order to confirm whether the velocity was 
* reached or whether an alert or stop commanded aborted the move.						 
*
* @param node The node you want to command to move
* @param target Target velocity. 

  @return true/false depending on whether the node reached the target velocity
*/
bool moveAtVelocity(INode& node, int target) {
	// we can't execute a move unless the node is ready to recieve motion commands
	if (!node.Motion.IsReady()) {
		printf("Node[%d]: Move canceled because the Node is not ready\n", node.Info.Ex.NodeIndex());
		printf("Make sure to clear Alerts and NodeStops and ensure the node is Enabled.\n");
		return false;
	}

	// Refresh then Clear the rising register before starting the move so that
	// we can detect events that occured during the move.
	node.Status.Rise.Refresh();
	node.Status.Rise.Clear();

	printf("Node[%d]: Ramping to velocity %d\n", node.Info.Ex.NodeIndex(), target);
	node.Motion.MoveVelStart(target); //Execute velocity move

	// Wait for the node to reach it's target velocity
	// Note: MoveDone is asserted when a move completes successfully or when the node has a shutdown.
	while (!node.Motion.VelocityAtTarget());

	mnStatusReg mask; // Create a status register mask to test the Rising register
	mask.cpm.AlertPresent = 1; // set AlertPresent
	mask.cpm.MoveCanceled = 1; // set MoveCanceled

	mnStatusReg result; // store results from test
	// Verify the motor didn't go into alert during the move
	// and that the move wasn't cancelled
	if (node.Status.Rise.TestAndClear(mask, result)) {
		if (result.cpm.AlertPresent) {
			printf("Node[%d]: Move failed due to AlertPresent\n", node.Info.Ex.NodeIndex());
		}
		if (result.cpm.MoveCanceled) {
			printf("Node[%d]: Move failed due to MoveCanceled\n", node.Info.Ex.NodeIndex());
		}
		return false; // move did not complete successfully
	}
	
	printf("Node[%d]: Velocity Reached\n", node.Info.Ex.NodeIndex());
	return true;
}
