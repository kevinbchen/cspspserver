// CSPSPServer.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"

#include <stdio.h> // For perror() call
#include <stdlib.h> // For exit() call
#include <windows.h>
#include <winsock.h> // Include Winsock Library
#include <iostream>
#include <ctime>
#include <errno.h>
#include <math.h>
//#include "PlayerConnection.h"
#include "Packet.h"

using namespace CSPSPServer;
using namespace System::Threading;


BOOL CtrlHandler(DWORD fdwCtrlType);


[STAThreadAttribute]
int main(cli::array<System::String ^> ^args)
{
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );

	//Thread^ oThread = gcnew Thread( gcnew ThreadStart( &ServerThread::ThreadProc ) );
	//oThread->Start();

	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	Application::Run(gcnew Form1());

	//oThread->Abort();

	return 0;
}

BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 

	delete server;
	return FALSE;

  /*switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
      printf( "Ctrl-C event\n\n" );
      Beep( 750, 300 ); 
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
      Beep( 600, 200 ); 
      printf( "Ctrl-Close event\n\n" );
      return FALSE; 
 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
      Beep( 900, 200 ); 
      printf( "Ctrl-Break event\n\n" );
      return FALSE; 
 
    case CTRL_LOGOFF_EVENT: 
      Beep( 1000, 200 ); 
      printf( "Ctrl-Logoff event\n\n" );
      return FALSE; 
 
    case CTRL_SHUTDOWN_EVENT: 
      Beep( 750, 500 ); 
      printf( "Ctrl-Shutdown event\n\n" );
      return FALSE; 
 
    default: 
      return FALSE; 
  } */
} 