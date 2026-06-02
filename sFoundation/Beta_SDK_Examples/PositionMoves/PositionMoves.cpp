/*
 * Title: PositionMoves.cpp
 *
 * Objective:
 *    - Demonstrate how to execute sequential positional moves on a single Node.
 *
 * Description:
 *    This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    Next, the program enables the first Node, then commands a series of positional moves to that motor in both directions.
 *    The program will wait for each move to be finished before executing the next move. Homing is not performed in this example.
 * 
 *	  Position Moves are "Relative" by default, but can be configured to be "Absolute" by changing the `targetIsAbsolute` parameter
 *    when calling the provided 'moveDistance' helper function. You can read more about what "Relative" and "Absolute" moves are below.
 * 
 *    "Relative" means that the target position is specified as relative to the current position of the motor.
 *        ex. Motor starts at position 1000, I command it to move relative 5000 counts
 *			  The motor moves 5000 counts from its current position of 1000 to reach a final position of 6000
 *    "Absolute" means that the target position is referring to an absolute location in the commanded position space,
 *    typically defined by homing.
 *        ex. Motor starts at position 1000, I command it to move to the absolute position 0
 *            The motor moves -1000 counts to reach the absolute position 0
 * 
 *    The Acceleration Limit (rpm/s) and Velocity Limit (rpm) of the Node can be modified using macros ACC_LIM_RPM_PER_SEC and VEL_LIM_RPM
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
#define VEL_LIM_RPM			500  // Velocity Limit in RPM for Node 0

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);
bool moveDistance(INode&, int, bool);

int main(int argc, char* argv[]) {
	msgUser("Position Moves Example starting. Motion will occur on Node 0 at speeds up to 500RPM. Press Enter to start.");
	
	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();	//Create System Manager myMgr

	try {
		// Search for ClearPath SC4-HUB's connected to this machine and store them in comHubPorts
		std::vector<std::string> comHubPorts;
		SysManager::FindComHubPorts(comHubPorts);

		if (comHubPorts.empty()) {
			printf("No SC4-HUB's found. Ensure your SC4-HUB is connected to your computer via USB or RS-232 and has 24V power.");
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

		// Configure acceleration and velocity limits
		myNode.AccUnit(INode::RPM_PER_SEC);				//Set the units for Acceleration to RPM/SEC
		myNode.VelUnit(INode::RPM);						//Set the units for Velocity to RPM
		myNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;	//Set Acceleration Limit (RPM/Sec)
		myNode.Motion.VelLimit = VEL_LIM_RPM;			//Set Velocity Limit (RPM)

		// Clear potential alerts and NodeStops before trying to enable the node
		printf("Clearing any NodeStops then Alerts...\n");
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
		
		printf("\nStarting positional moves...\n");
		
		// Move 6400 counts (positive direction), then wait 2000ms
		moveDistance(myNode, 6400, false);
		
		// To command an absolute move just change the `targetIsAbsolute` parameter to true:
		// moveDistance(myNode, 6400, true);
		
		myMgr->Delay(2000);
		
		// Move 19200 counts farther positive, then wait 2000ms
		moveDistance(myNode, 19200, false);
		myMgr->Delay(2000);
		// Move back 12800 counts (negative direction), then wait 2000ms
		moveDistance(myNode, -12800, false);
		myMgr->Delay(2000);
		// Move back 6400 counts (negative direction), then wait 2000ms
		moveDistance(myNode, -6400, false);
		myMgr->Delay(2000);
		// Move back to the start (negative 6400 pulses), then wait 2000ms
		moveDistance(myNode, -6400, false);
		myMgr->Delay(2000);
		
		printf("\nFinished positional moves.\n");
	
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
* Command a given node to start a positional move to a given target.
* Prints the status to the console.
*
* @param node The node you want to command to move
* @param target Target position. This is either the absolute position
  or relative to the current position depending on the targetIsAbsolute argument.
  @param targetIsAbsolute The target is an absolute destination.

  @return true/false depending on whether the move was completed successfully
*/
bool moveDistance(INode& node, int target, bool targetIsAbsolute) {
	// we can't execute a move unless the node is ready to recieve motion commands
	if (!node.Motion.IsReady()) {
		printf("Node[%d]: Move cancelled because the Node is not ready\n", node.Info.Ex.NodeIndex());
		return false;
	}
	
	// Refresh then Clear the rising register before starting the move so that
	// we can detect if anything happens during the move.
	node.Status.Rise.Refresh();
	node.Status.Rise.Clear();

	printf("Node[%d]: Moving %dcounts, isAbsolute=%d\n", node.Info.Ex.NodeIndex(), target, targetIsAbsolute);
	node.Motion.MovePosnStart(target, targetIsAbsolute); //Execute positional move
	double moveTimeEstimateMsec = node.Motion.MovePosnDurationMsec(target, targetIsAbsolute);
	printf("%.2fms estimated time.\n", moveTimeEstimateMsec);

	// Wait for move to be done
	// Note: MoveDone is asserted when a move completes successfully or when the node has a shutdown.
	while (!node.Motion.MoveIsDone());

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

	printf("Node[%d]: Move Done\n", node.Info.Ex.NodeIndex());
	return true;
}

