/*
* Title: ProjectTemplate.cpp
* 
* Description:
*    This is a copyable Visual Studio Project that can be used as a starting point for new programs.
*    
*    The code included in this file should only compile/run when this project is properly set up.
* 
* Requirements:
*    1. The provided batch file, 'sFoundationCopy.bat', must be present in the parent directory of this project.
*			- This batch file is used to copy the compiled sFoundation libraries into this project.
*    2. This projects parent directory must be in the same level as the 'inc' and 'sFoundation Source' folders.
*			- This project has all necessary libraries linked already for your convience. As long as the
*             project is placed in the below directory tree, it should compile without any additional steps.
* 
* 			├───DirectoryYouCanRename
*			│   ├───ProjectTemplate
*			│   └───sFoundationCopy.bat
*			├───inc
*			└───sFoundation Source
* 
* Common Issues:
*	`sFoundationCopy.bat"' is not recognized as an internal or external command, operable program or batch file.`
*		- Ensure sFoundationCopy.bat is placed in the parent directory of this project.
*	
*   `Cannot open include file: 'pubSysCls.h': No such file or directory`
*		- Ensure the 'inc' directory is located as shown in the above directory tree.
*         You may have to copy this folder from your sFoundation install directory.
*   
*   `LINK : fatal error LNK1104: cannot open file 'sFoundation20.lib'`
*       - Ensure the 'sFoundation Source' directory is located as shown in the above directory tree.
*         You may have to copy this folder from your sFoundation install directory.
*    
*   Other issues:
*       - Try restarting your IDE and/or your computer. If you are still having issues please contact technical support.
*         https://teknic.com/contact/ or call us at 585-784-7454
*/

// Include files
#include "pubSysCls.h"

using namespace sFnd;

int main(int argc, char* argv[]) {
	//Create the SysManager object. This object will coordinate actions among various ports
	// and within nodes. In this example we use this object to setup and open our port.
	SysManager* myMgr = SysManager::Instance();	//Create System Manager myMgr
	printf("Program compiled successfully\n");

	return 0; // End program
}
