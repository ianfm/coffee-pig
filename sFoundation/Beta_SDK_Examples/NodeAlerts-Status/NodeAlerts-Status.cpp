/*
 * Title: NodeAlerts-Status.cpp
 *
 * Objective:
 *    Demonstrate how to do the following:
 *    - Access the Status Register
 *    - Check if any alerts are present
 *    - Check if a specific alert is present
 *    - Clear Alerts and NodeStops
 *    - Read/Refresh Current Position, Velocity, & Torque Readings
 *
 * Description:
 *    This program finds SC4-HUBs connected to this computer and opens the first port found.
 *    
 *    - Status Register -
 *    The program prints all fields that are currently set in the Real-Time (RT) status register on Node 0.
 *    This example also demonstrates how to read individual fields of the status register.
 *    
 *    - Alerts - 
 *    Checks if Node 0 has alerts present.
 *    If there are alerts present, the program prints all alerts that are present.
 *      Then, the program checks if the node has the EStopped alert present.
 *      If the EStopped alert is present we will clear it.
 *      The program will then clear all other alerts.
 * 
 *    - Reading Information - 
 *    The program displays the real-time position and velocity of the motor and gives you
 *    10 seconds to spin the motor shaft and see these values change.
 *    Next, Node 0 is enabled for 10 seconds, meaning the motor applies torque to maintain its current position
 *    or to follow any given command. The motor will output torque to resist any motion during this time.
 *    This real-time torque value (percent of peak torque) is displayed so you can see the torque change.
 *
 * Requirements:
 *	1. Have 1 SC4-HUB connected to your computer via USB.
 *  2. That port connected must have 1 ClearPath SC motor connected to it.
 *  3. The ClearPath SC connected should be unloaded with factory default settings.
 *
 * Links:
 * ** ClearPath SC User Manuals:
 * ** - NEMA 23 and 34 IP53 Models:             https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual.pdf
 * ** - NEMA 23 and 34 IP67/66K Sealed Models:  https://teknic.com/files/downloads/Clearpath-SC%20User%20Manual_ip67.pdf
 * ** - NEMA 56/143 and IEC D100 Models:        https://teknic.com/files/downloads/ac_clearpath-sc_manual.pdf
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
void printNodeInfo(INode&);
void printStatusRegister(INode&);
void printCurrentAlertsRegister(INode&);

int main(int argc, char* argv[]) {
	msgUser("Status and Alerts Example starting. Node 0 will be enabled in this example, but no motion will occur. Press Enter to start.");

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
		// ==============================
		// SETUP
		// ==============================
		
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
		
		// ==============================
		// STATUS REGISTER
		// ==============================
		
		// Print the status register to see all fields that are currently set
		printStatusRegister(myNode);

		/*
		* Here is how you could read a specific field in the status register.
		* In our demonstration below, we are going to read the "Enabled" status
		* register field and see how it changes when we enable/disable the Node.
		*/
		
		myNode.EnableReq(true); // Enable the node
		myNode.Status.RT.Refresh(); // Update our Status Register with data from the Node
		printf("Enabled: %d\n", myNode.Status.RT.Value().cpm.Enabled);
		
		myNode.EnableReq(false); // Disable the node
		myNode.Status.RT.Refresh(); // refresh again before we read the Enabled field
		printf("Enabled: %d\n", myNode.Status.RT.Value().cpm.Enabled);

		// ==============================
		// ALERTS REGISTER
		// ==============================
		myNode.Status.Alerts.Refresh(); // Refresh alerts register before reading to ensure we have up-to-date data

		// If our node has alerts
		if (myNode.Status.Alerts.Value().isInAlert()) {
			printf("Node[0] has alerts.\n");
			printCurrentAlertsRegister(myNode); // print all alerts

			/*
			* Check if the node has the EStopped alert present.
			* 
			* EStopped means an ESTOP Type NodeStop has been issued to the node.
			* When EStopped, a node will no longer accept motion commands.
			* To resume motion, this alert must first be cleared using `myNode.Motion.NodeStopClear();` as shown below.
			* Unlike other alerts, EStopped will not be cleared by `myNode.Status.AlertsClear();`.
			*/
			if (myNode.Status.Alerts.Value().cpm.Common.EStopped) {
				// If the EStopped alert is present we will clear it
				printf("Node has EStopped alert present: Clearing Node Stops\n");
				myNode.Motion.NodeStopClear();
			}

			// Clear the other alerts
			myNode.Status.AlertsClear();
		}
		else {
			printf("Node[0] had no alerts.\n");
		}

		// ==============================
		// READ POSITION AND VELOCITY
		// ==============================
		printf("Showing the current measured position and velocity of Node[0] for the next 10 seconds\n");
		printf("Try spinning the motor shaft to see the values change!\n");
		double startTime = myMgr->TimeStampMsec(); // store the starting time in ms
		
		// Print the measured values while the current time is less than the starting time plus 10 seconds
		while (myMgr->TimeStampMsec() < startTime + 10000) {
			myNode.Motion.PosnMeasured.Refresh();                     // refresh the value before accessing
			double posnMeasured = myNode.Motion.PosnMeasured.Value(); // get measured position value from the Node

			myNode.Motion.VelMeasured.Refresh();					  // refresh the value before accessing
			double velMeasured = myNode.Motion.VelMeasured.Value();   // get measured velocity value from the Node

			printf("\rPosition: %10.2f counts | Velocity: %10.2f RPM", posnMeasured, velMeasured);
		}
		printf("\n");

		// ==============================
		// READ TORQUE
		// ==============================
		printf("Node[0] will now apply torque to maintain its commanded position\n");
		msgUser("Press enter to enable Node[0]");
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

		printf("Showing the current measured torque and velocity of Node[0] for the next 10 seconds\n");
		printf("Try to spin the motor shaft (you will feel resistance) to see the torque value change!\n");
		startTime = myMgr->TimeStampMsec(); // store the starting time in ms

		// print the measured value while the current time is less than the starting time plus 10 seconds
		while (myMgr->TimeStampMsec() < startTime + 10000) {
			myNode.Motion.TrqMeasured.Refresh();                  // refresh the value before accessing
			double trqMeasured = myNode.Motion.TrqMeasured.Value(); // get measured torque value from the Node

			printf("\rMeasured Torque (percent of peak): %8.2f%%", trqMeasured);
		}

		// ==============================
		// SHUTDOWN
		// ==============================
		printf("\nDisabling Node[0] and closing port\n");
		myNode.EnableReq(false);
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

/**
* Prints all fields that are set in the Real Time (RT) status register of a given node
*
* Note: Rise, Fall, & Accum status registers can also be read using similar code
*		Please refer to the documentation on "IStatus" for more information.
*/
void printStatusRegister(INode& node) {
	node.Status.RT.Refresh(); // Update our local RT Status Register with information from the Node
	mnStatusReg rtRegister = node.Status.RT.Value(); // store the current status register values

	char registerBuffer[500]; // character array to hold state string

	// Update the register character array buffer with a newline
	// delimited field names of all bits that are set in the status register.
	rtRegister.StateStr(registerBuffer, sizeof(registerBuffer));

	// print out that string we just stored
	printf("===============\n");
	printf("The following fields are currently set in the status register of Node[%d]:\n", node.Info.Ex.NodeIndex());
	printf(registerBuffer);
	printf("===============\n");
}

/**
* Prints all fields that are set in the Alerts register of a given node
* Note: this function does not refresh the alerts register
*/
void printCurrentAlertsRegister(INode& node) {
	alertReg alertsRegister = node.Status.Alerts.Value(); // store a copy of the current alerts register values

	char registerBuffer[500]; // character array to hold state string

	// Update the register character array buffer with a newline
	// delimited field names of all bits that are set in the alerts register.
	alertsRegister.StateStr(registerBuffer, sizeof(registerBuffer));

	// print out that string we just stored
	printf("===============\n");
	printf("The following fields are currently set in the alerts register of Node[%d]:\n", node.Info.Ex.NodeIndex());
	printf(registerBuffer);
	printf("===============\n");
}