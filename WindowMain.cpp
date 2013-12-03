/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE: WindowMain.cpp		Responsible for setting up the program.
--
-- PROGRAM:		BCP
--
-- FUNCTIONS:
--	int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)
--
-- DATE: 		October 23, 2013
--
-- REVISIONS: 	None
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- Project must be in Unicode to accomodate the window title
----------------------------------------------------------------------------------------------------------------------*/

#include "BCP.h"

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	WinMain
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
--				LPSTR lspszCmdParam, int nCmdShow)
--
-- RETURNS: 	int
--
-- NOTES:
-- Entry point for the program
----------------------------------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	LPSTR lspszCmdParam, int nCmdShow)
{
	HWND hwnd;
	MSG Msg;
	WNDCLASSEX Wcl;

	static TCHAR Name[] = TEXT("BeCreativeProtocol");
	static TCHAR Title[] = TEXT("(BЗ CЯЗATIVЗ) Protocol Engine");

	// Define a Window class
	Wcl.cbSize = sizeof (WNDCLASSEX);
	Wcl.style = 0; // default style
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);			// large icon 
	Wcl.hIconSm = NULL;										// use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);				// cursor style

	Wcl.lpfnWndProc = WndProc;								// window function
	Wcl.hInstance = hInst;									// handle to this instance
	Wcl.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));// background
	Wcl.lpszClassName = Name;								// window class name

	Wcl.lpszMenuName = NULL;								// no class menu 
	Wcl.cbClsExtra = 0;										// no extra memory needed
	Wcl.cbWndExtra = 0;

	// Register the class
	if (!RegisterClassEx(&Wcl))
		return 0;

	hwnd = CreateWindow(
		Name,					// name of window class
		Title,					// title 
		WS_OVERLAPPEDWINDOW,	// window style - normal
		CW_USEDEFAULT,			// X coord
		CW_USEDEFAULT,			// Y coord
		800,					// width
		600,					// height
		NULL,					// no parent window
		NULL,					// no menu
		hInst,					// instance handle
		NULL					// no additional arguments
		);

	// Display the window
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// Create the message loop
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);		// translate keybpard messages
		DispatchMessage(&Msg);		// dispatch message and return control to windows
	}

	return Msg.wParam;
}