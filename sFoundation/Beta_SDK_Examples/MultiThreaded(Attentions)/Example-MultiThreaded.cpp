/*
 * Title: Example-MultiThreaded.cpp
 *
 * Objective:
 *    - Show a real-world event driven multi-threaded program example for ClearPath-SC.
 *    - Demonstrate how to use "interrupt-like" attentions and triggered moves. Features that
 *      are only available with the Advanced Firmware Option.
 *
 * Description:
 *    This program will discover SC4-HUBs connected to your computer.
 *    For each SC4-HUB (Port) the program will discover all Nodes connected to it.
 *    An Axis object is created for each Node found, and that Axis is added to listOfAxes.
 *	  Create the supervisor thread, giving it access to the list of axes we just created.
 *    Next, the supervisor starts running, executing synchronized moves on all axes.
 * 
 *    The synchronization is fairly simple - the moves are all started at the
 *	  same time (using a trigger) and the new moves are not sent until all the moves have completed. Each node has a different
 *	  length of move, so they all finish at different times. The Axis class handles the interaction with a node on the
 *	  network. The Supervisor class interacts with the Axis objects, telling them when to send their moves to the node.
 *	  After each node has sent its move, the Supervisor starts all the moves with a Trigger command. The Axis then waits for
 *	  MoveDone, and reports back to the Supervisor. The Supervisor waits for all the moves to finish, and starts the cycle
 *	  again.
 * 
 *    Finally, when you press a key, the program ends.
 *    The supervisor quits/terminates, the listOfAxes is deleted, and the ports are closed.
 *
 * Supervisor Class Description:
 *    This class holds the references to all the nodes in the network, and synchronizes the moves among
 *    those nodes. This class runs its own thread to handle the synchronization without having to wait for
 *    the user input to end the program. The Supervisor thread is a state machine that waits for the Axes
 *    to be idle, kicks the Axes to tell them to send their move to their node, waits for all the moves to
 *    be sent, sends a Trigger to start the moves, waits for all the Axes to report MoveDone, then starts
 *    all over again.
 * 
 * Axis Class Description:
 *	  This class talks to the nodes directly; there is one Axis object per node on the network. Each Axis runs in
 *	  its own thread, and is synchronized with the other nodes through the Supervisor class. The Axis class
 *	  basically waits for the Supervisor to tell it to send its move, then waits for MoveDone, reporting back to
 *	  the Supervisor when it is received. It also reports to the Supervisor if an error was encountered with the node.
 *
 * Requirements:
 *	1. Have 1-3 SC4-HUB(s) connected to your computer via USB.
 *  2. Each SC4-HUB connected must have at least 1 ClearPath SC motor with Advanced firmware connected to it
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

#include <stdio.h>
#include <ctime>
#include <chrono>
#include <string>
#include <iostream>
#include "Axis.h"
#include "Supervisor.h"

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

/*****************************************************************************
*  Function Name: AttentionDetected
*	 Description:	This is the port-level attention handler function.
*					This handler simply prints out the attention information
*					to the console window. 
*	  Parameters:
*          Input:	detected		- contains the attention information
*         Return:		none
*****************************************************************************/
void MN_DECL AttentionDetected(const mnAttnReqReg &detected)
{
	// Make a local, non-const copy for printing purposes
	mnAttnReqReg myAttns = detected;
	// Create a buffer to hold the attentionReg information
	char attnStringBuf[512];
	// Load the buffer with the string representation of the attention information
	myAttns.AttentionReg.StateStr(attnStringBuf, 512);
	// Print it out to the console
	printf("  --> ATTENTION: port %d, node=%d, attn=%s\n",
		detected.MultiAddr >> 4, detected.MultiAddr, attnStringBuf);
}


#if _MSC_VER
#pragma warning(disable:4996)
#endif
// A nice way of printing out the system time
string CurrentTimeStr() {
	time_t now = time(NULL);
	return string(ctime(&now));
}
#define CURRENT_TIME_STR CurrentTimeStr().c_str()
#if _MSC_VER
#pragma warning(default:4996)
#endif

int main(int argc, char* argv[])
{
	msgUser("Multithreaded Example starting. Motion will occur on all Nodes. Press Enter to start.");

	size_t portCount = 0;
	std::vector<std::string> comHubPorts;

	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();							//Create System Manager myMgr

	//This will try to open the port. If there is an error/exception during the port opening,
	//the code will jump to the catch loop where detailed information regarding the error will be displayed;
	//otherwise the catch loop is skipped over
	try
	{ 
		
		SysManager::FindComHubPorts(comHubPorts);
		printf("Found %ld SC Hubs\n", comHubPorts.size());

		for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++) {
			
			myMgr->ComHubPort(portCount, comHubPorts[portCount].c_str()); 	//define the first SC Hub port (port 0) to be associated 
											// with COM portnum (as seen in device manager)
		}

		if (portCount > 0) {
			//printf("\n I will now open port \t%i \n \n", portnum);
			myMgr->PortsOpen(portCount);				//Open the port

			for (size_t i = 0; i < portCount; i++) {
				IPort &myPort = myMgr->Ports(i);

				printf(" Port[%d]: state=%d, nodes=%d\n",
					myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());
			}
		}
		else {
			printf("Unable to locate SC hub port\n");

			msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key

			return -1;  //This terminates the main program
		}

		// Create a list of axes - one per node
		vector<Axis*> listOfAxes;
		// Assume that the nodes are of the right type and that this app has full control
		bool nodeTypesGood = true, accessLvlsGood = true; 

		for (size_t iPort = 0; iPort < portCount; iPort++){
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr->Ports(iPort);

			// Enable the attentions for this port
			myPort.Adv.Attn.Enable(true);
			// The attentions will be handled by the individual nodes, but register
			// a handler at the port level, just for illustrative purposes.
			myPort.Adv.Attn.AttnHandler(AttentionDetected);

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++){

				// Get a reference to the node, to make accessing it easier
				INode &theNode = myPort.Nodes(iNode);
				theNode.Motion.AccLimit = 100000;

				// Make sure we are talking to a ClearPath SC
				if (theNode.Info.NodeType() != IInfo::CLEARPATH_SC_ADV) {
					printf("---> ERROR: Uh-oh! Node %d is not a ClearPath-SC Advanced Motor\n", iNode);
					nodeTypesGood = false;
				}

				if (nodeTypesGood) {
					// Create an axis for this node
					listOfAxes.push_back(new Axis(&theNode));

					if (!theNode.Setup.AccessLevelIsFull()) {
						printf("---> ERROR: Oh snap! Access level is not good for node %u\n", iNode);
						accessLvlsGood = false;
					}
					else {
						// Set the move distance based on where it is in the network
						listOfAxes.at(iNode)->SetMoveRevs((iNode + 1) * 2);
						// Set the trigger group indicator
						theNode.Motion.Adv.TriggerGroup(1);
					}
				}

			}

			#if 0
			// Set the last node in the ring to a longer move
			listOfAxes.at(listOfAxes.size()-1)->SetMoveRevs(listOfAxes.size()*10);
			#endif
		}

		// If we have full access to the nodes and they are all ClearPath-SC advanced nodes, 
		// then continue with the example
		if (nodeTypesGood && accessLvlsGood){

			// Create the supervisor thread, giving it access to the list of axes
			Supervisor theSuper(listOfAxes, *myMgr);
			theSuper.CreateThread();

			printf("\nMachine starting: %s\n", CURRENT_TIME_STR);

			// Everything is running - wait for the user to press a key to end it
			msgUser("Press any key to stop the machine."); //pause so the user can see the error message; waits for user to press a key
			
			printf("Machine stopping: %s\n", CURRENT_TIME_STR);

			// Tell the supervisor to stop
			theSuper.Quit();
			theSuper.Terminate();

			printf("\nFinalStats:\tnumMoves\t\tPosition\n");
		}
		else{
			// If something is wrong with the nodes, tell the user about it and quit
			if (!nodeTypesGood){
				printf("\n\tFAILURE: Please attach only ClearPath-SC Advanced nodes.\n\n");
			}
			else if (!accessLvlsGood){
				printf("\n\tFAILURE: Please get full access on all your nodes.\n\n");
			}
		}

		// Delete the list of axes that were created
		for (size_t iAxis = 0; iAxis < listOfAxes.size(); iAxis++){
			delete listOfAxes.at(iAxis);
		}

		// Close down the ports
		myMgr->PortsClose();

	}
	catch (mnErr& theErr) {
		fprintf(stderr, "Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
				theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n",
				theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
		return(2);
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
		return(3);
	}

	// Good-bye
	msgUser("Press any key to exit."); //pause so the user can see the error message; waits for user to press a key
	return 0;
}
//																			   *
//******************************************************************************

