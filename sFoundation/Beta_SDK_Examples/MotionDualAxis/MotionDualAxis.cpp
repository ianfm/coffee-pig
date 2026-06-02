/*
 * Title: MotionDualAxis.cpp
 *
 * Objective:
 *    - Demonstrate how to execute synchronized sequential positional moves on two or more Nodes.
 *
 * Description:
 *    This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    Next, the program enables the first two Nodes, then commands a series of synchronized
 *    positional moves to both motors in both directions.
 *    The positional moves are sent using the provided helper function: `synchronizedMove` which can command positional
 *    motion on two or more nodes at the same time. More details are available in that funcitons docstring.
 *    The program will wait for each move to be finished before executing the next move. Homing is not performed in this example.
 * 
 *	  Position Moves are "Relative" by default, but can be configured to be "Absolute" by changing the `targetIsAbsolute` parameter
 *    when calling the provided 'synchronizedMove' helper function. You can read more about what "Relative" and "Absolute" moves are below.
 *    
 *    "Relative" means that the target position is specified as relative to the current position of the motor.
 *        ex. Motor starts at position 1000, I command it to move relative 5000 counts
 *			  The motor moves 5000 counts from its current position of 1000 to reach a final position of 6000
 *    "Absolute" means that the target position is referring to an absolute location in the commanded position space,
 *    typically defined by homing.
 *        ex. Motor starts at position 1000, I command it to move to the absolute position 0
 *            The motor moves -1000 counts to reach the absolute position 0
 *
 *    The Acceleration Limit (rpm/s) and Velocity Limit (rpm) of both Nodes can be modified using macros ACC_LIM_RPM_PER_SEC and VEL_LIM_RPM
 *
 * Requirements:
 *	1. Have 1 SC4-HUB connected to your computer via USB.
 *  2. That port connected must have 2 ClearPath SC motors connected to it.
 *  3. The ClearPath SC motors connected should be unloaded with factory default settings.
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
#include <functional>
#include <iostream>
#include "pubSysCls.h"	

using namespace sFnd;

// User Editable Macros 
#define ACC_LIM_RPM_PER_SEC	1000 // Acceleration Limit in RPM/s for Node 0 and Node 1
#define VEL_LIM_RPM			500  // Velocity Limit in RPM for Node 0 and Node 1

// Declares user-defined helper functions.
// The definition/implementations of these functions are at the bottom of this program.
void msgUser(const char*);
void printNodeInfo(INode&);
bool synchronizedMove(std::vector<std::reference_wrapper<INode>>, std::vector<bool>, int, bool);

int main(int argc, char* argv[]) {
	msgUser("Motion Dual Axis Example starting. Motion will occur on Node 0 and Node 1 at speeds up to 500RPM. Press Enter to start.");

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

		// Ensure there are 2 nodes connected to this port
		if (myPort.NodeCount() != 2) {
			printf("Expected 2 Nodes, but found %d. You must have 2 nodes connected to this SC4-HUB for this example to run.\n", myPort.NodeCount());
			myMgr->PortsClose();
			return 1;
		}

		// Create a useful reference to the first node on myPort
		INode& myNode0 = myPort.Nodes(0);
		printNodeInfo(myNode0);

		// Create a useful reference to the second node on myPort
		INode& myNode1 = myPort.Nodes(1);
		printNodeInfo(myNode1);

		// Configure acceleration and velocity limits for both nodes
		myNode0.AccUnit(INode::RPM_PER_SEC);				//Set the units for Acceleration to RPM/SEC
		myNode0.VelUnit(INode::RPM);						//Set the units for Velocity to RPM
		myNode0.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;		//Set Acceleration Limit (RPM/Sec)
		myNode0.Motion.VelLimit = VEL_LIM_RPM;				//Set Velocity Limit (RPM)

		myNode1.AccUnit(INode::RPM_PER_SEC);				//Set the units for Acceleration to RPM/SEC
		myNode1.VelUnit(INode::RPM);						//Set the units for Velocity to RPM
		myNode1.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;		//Set Acceleration Limit (RPM/Sec)
		myNode1.Motion.VelLimit = VEL_LIM_RPM;				//Set Velocity Limit (RPM)

		// Clear potential alerts and NodeStops before trying to enable the nodes
		printf("Clearing any NodeStops then Alerts...\n");
		myNode0.Motion.NodeStopClear();
		myNode1.Motion.NodeStopClear();
		myNode0.Status.AlertsClear();
		myNode1.Status.AlertsClear();

		// Enable both nodes
		printf("Waiting for both nodes to enable...\n");
		myNode0.EnableReq(true);
		myNode1.EnableReq(true);

		// Wait for both nodes to be ready after enable...
		double timeout = myMgr->TimeStampMsec() + 10000;	//define a timeout in case the node is unable to enable
		//This will loop checking on the Real time values of the node's Ready status
		while (!myNode0.Motion.IsReady() || !myNode1.Motion.IsReady()) {
			if (myMgr->TimeStampMsec() > timeout) {
				if (!myNode0.Motion.IsReady()) {
					printf("Error: Timed out waiting for Node 0 to enable\n");
				}
				if (!myNode1.Motion.IsReady()) {
					printf("Error: Timed out waiting for Node 1 to enable\n");
				}
				printf("Ensure each Node has bus power and has no alerts.\n");
				myMgr->PortsClose();
				msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
				return 1;
			}
		}
		printf("Nodes enabled successfully.\n");

		// Initialize a list of nodes and their corresponding directions to pass to the `synchronizedMove` function
		std::vector<std::reference_wrapper<INode>> nodesList = { std::ref(myNode0), std::ref(myNode1) };
		std::vector<bool> reverseNodeDirection = { false, false }; // have both motors run the same direction as the target
		
		// the below code would configure the second node to move in the opposite direction
		// std::vector<bool> reverseNodeDirection = { false, true };

		printf("\nStarting synchronized positional moves...\n");

		// Move 6400 counts (positive direction), then wait 2000ms
		synchronizedMove(nodesList, reverseNodeDirection, 6400, false);
		
		// To command an absolute move just change the `targetIsAbsolute` parameter to true:
		// synchronizedMove(myNode, 6400, true);
		
		myMgr->Delay(2000);
		
		// Move 19200 counts farther positive, then wait 2000ms
		synchronizedMove(nodesList, reverseNodeDirection, 19200, false);
		myMgr->Delay(2000);
		// Move back 12800 counts (negative direction), then wait 2000ms
		synchronizedMove(nodesList, reverseNodeDirection, - 12800, false);
		myMgr->Delay(2000);
		// Move back 6400 counts (negative direction), then wait 2000ms
		synchronizedMove(nodesList, reverseNodeDirection, - 6400, false);
		myMgr->Delay(2000);
		// Move back to the start (negative 6400 pulses), then wait 2000ms
		synchronizedMove(nodesList, reverseNodeDirection, -6400, false);
		myMgr->Delay(2000);
		
		printf("\nFinished synchronized positional moves.\n");
		
		printf("Disabling nodes, and closing port\n");
		myNode0.EnableReq(false);
		myNode1.EnableReq(false);
		myMgr->PortsClose();
	}
	catch (mnErr& theErr) {	//This catch statement will intercept any error from the Class library
		switch (theErr.ErrorCode) {
		case MN_ERR_PORT_FAILED_PORTS_0:
			printf("Port failed to open. Ensure that ClearView or any other application is not using the port.\n");
			break;
		case MN_ERR_INIT_FAILED_PORTS_0:
			printf("Port failed to initialize. Ensure that 2 ClearPath SC motors are powered and connected to the SC4-HUB and the jumpers are installed correctly on the SC4-HUB\n");
			break;
		}

		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		
		msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
		myMgr->PortsClose(); // close ports
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
* Command each node to perform a position move to a given target at the same time.
* The direction of each node can be configured using the nodeDirections parameter.
* Changing the direction may be useful or required in applications that have
* motors mounted opposite to each other (ex. some dual motor gantries) 
* Prints the status to the console.
*
* @param nodes List of references to Nodes that will all execute this move
* @param reverseNodeDirection List of booleans that correspond to the nodes list
* `true`  means the commanded direction of the node is the opposite of the given target
* `false` means the commanded direction of the node is the same as the given target
* @param target Target position. This is either the absolute position
* or relative to the current position depending on the targetIsAbsolute argument.
* @param targetIsAbsolute The target is an absolute destination.
*
* @return true/false depending on whether the move was completed successfully by all Nodes
*/
bool synchronizedMove(std::vector<std::reference_wrapper<INode>> nodes, std::vector<bool> reverseNodeDirection,
					  int target, bool targetIsAbsolute) {
	// For each node in nodes: verify each is ready and clear each rising status register
	for (INode& node : nodes) {
		// we can't execute a move unless the node is ready to recieve motion commands
		if (!node.Motion.IsReady()) {
			printf("Node[%d]: Move cancelled because this Node is not ready\n", node.Info.Ex.NodeIndex());
			return false;
		}
		
		// Refresh then Clear the rising register before starting the move so that
		// we can detect if anything happens during the move.
		node.Status.Rise.Refresh();
		node.Status.Rise.Clear();
	}

	// Start the positional move on each node in nodes
	for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
		INode& node = nodes[nodeIndex];
		int nodeTarget;
		if (reverseNodeDirection[nodeIndex]) {
			nodeTarget = -target; // assign to opposite of target since direction is reversed
		} else {
			nodeTarget = target;
		}
		printf("Node[%d]: Moving %dcounts, isAbsolute=%d\n", node.Info.Ex.NodeIndex(), nodeTarget, targetIsAbsolute);
		node.Motion.MovePosnStart(nodeTarget, targetIsAbsolute);
	}

	// Wait for move to be done on all nodes
	bool allNodesDone = false; // initialize to false
	while (!allNodesDone) {
		// assume all nodes will be done unless one isn't
		allNodesDone = true;

		// for each node in nodes
		for (INode& node : nodes) {
			// if any node is not done with it's move
			// Note: MoveDone is asserted when a move completes successfully or when the node has a shutdown.
			if (!node.Motion.MoveIsDone()) {
				allNodesDone = false; // set back to false to ensure we keep waiting
				break; // go to the next cycle
			}
		}
	}

	// Lastly test each node to verify the motor didn't go into alert during the move and that the move wasn't canceled
	for (INode& node : nodes) {
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
	}
	
	// If we get to this point all nodes completed the move successfully
	return true;
}
