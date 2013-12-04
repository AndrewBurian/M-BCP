/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:	UIDisplay.cpp		A collection of functions for displaying output to the window (packet statistics
--									and received data).
--
-- PROGRAM:		BCP
--
-- FUNCTIONS:
--	VOID update();
--	VOID SetupGUI();
--	VOID GUI_Text(TCHAR *text);
--	VOID GUI_Sent();
--	VOID GUI_Received();
--	VOID GUI_ReceivedBad();
--	VOID GUI_Lost();
--
-- DATE: 		November 26, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMERS: Chris Holisky/Ashley Tham
--
-- NOTES:
-- All functions return a boolean success value.
----------------------------------------------------------------------------------------------------------------------*/
#include "BCP.h"

#include <tchar.h>
int sent = 0, received = 0, badReceived = 0, lost = 0;
HWND hwndMainWin = NULL;
HWND hwndTextBox = NULL;
VOID update();

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: SetupGUI
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Ashley Tham
--
-- PROGRAMMER: Ashley Tham
--
-- INTERFACE: SetupGUI(HWND main, HWND edit)
--				HWND main: The main (parent) window
--				HWND edit: The child edit window in which file output will be displayed.
--
-- RETURNS: void;
-- 
-- NOTES:
-- Incrememnts a counter when bad packets are received and displays it.
----------------------------------------------------------------------------------------------------------------------*/
VOID SetupGUI(HWND main, HWND edit)
{
	hwndMainWin = main;
	hwndTextBox = edit;
	update();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_Text
--
-- DATE: November 19, 2013
--
-- REVISIONS: (Date and Description)
--			Dec 1, 2013
--			Chris Holisky
--			Revised code to actually append.  Used part of a Microsoft example.
--			Left code to replace text in edit box for testing purposes
--
-- DESIGNER: Chris Holisky
--
-- PROGRAMMER: Chris Holisky
--
-- INTERFACE: VOID GUI_Text(TCHAR* text)
-- TCHAR* text : Text to be written on screen
--
-- RETURNS: void;
-- 
-- NOTES:
-- This function is used to write data to the edit box.  It appends new data to the currently existing text.
--
----------------------------------------------------------------------------------------------------------------------*/
VOID GUI_Text(TCHAR* text)
{
	//This code just displays each packet in the edit box
	//SetWindowText(hwndTextBox, text);

	//This code appends new packets into the edit box
   int ndx = GetWindowTextLength (hwndTextBox);
   SetFocus (hwndTextBox);
   #ifdef WIN32
      SendMessage (hwndTextBox, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
   #else
      SendMessage (hwndTextBox, EM_SETSEL, 0, MAKELONG (ndx, ndx));
   #endif
	     #ifdef WIN32
      SendMessage (hwndTextBox, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
   #else
      SendMessage (hwndTextBox, EM_SETSEL, 0, MAKELONG (ndx, ndx));
   #endif
      SendMessage (hwndTextBox, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) text));
	
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_Text
--
-- DATE: November 30, 2013
--
-- REVISIONS: (Date and Description)
--
-- DESIGNER: Chris Holisky
--
-- PROGRAMMER: Chris Holisky
--
-- INTERFACE: VOID update()
--
-- RETURNS: void;
-- 
-- NOTES:
-- This function is draws the packet sent, lost and received statistics onto the window
--
----------------------------------------------------------------------------------------------------------------------*/
VOID update()
{
	HDC hdc=	GetDC(hwndMainWin);

	TCHAR num[7];
	const TCHAR *fmt = TEXT("%d");

	_sntprintf_s(num, 10, fmt, sent);
	TextOut(hdc, 550, 350,num,2);

	_sntprintf_s(num, 10, fmt, lost);
	TextOut(hdc, 550, 380, num,2);

	_sntprintf_s(num, 10, fmt, received);
	TextOut(hdc, 550, 410, num, 2);

	_sntprintf_s(num, 10, fmt, badReceived);
	TextOut(hdc, 550, 440, num, 2);
	
	double bitRateErr = (badReceived + received == 0) ? 0 : ((double)badReceived / ((double)badReceived + (double)received));
	_sntprintf_s(num, 10, TEXT("%.3f"), bitRateErr);
	TextOut(hdc, 550, 470, num, 5);

	ReleaseDC(hwndMainWin, hdc);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_Sent
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Chris Holisky
--
-- PROGRAMMER: Chris Holisky
--
-- INTERFACE: GUI_Sent(void)
--
-- RETURNS: void;
-- 
-- NOTES:
-- Incrememnts a counter when packets are sent (regardless of transmission outcome) and displays it.
----------------------------------------------------------------------------------------------------------------------*/
VOID GUI_Sent()
{
	++sent;
	update();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_Received
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Chris Holisky
--
-- PROGRAMMER: Chris Holisky
--
-- INTERFACE: GUI_Received(void)
--
-- RETURNS: void;
-- 
-- NOTES:
-- Incrememnts a counter when bad packets are received and displays it.
----------------------------------------------------------------------------------------------------------------------*/
VOID GUI_Received()
{
	++received;
	update();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_Lost
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Ashley Tham
--
-- PROGRAMMER: Ashley Tham
--
-- INTERFACE: GUI_Lost(void)
--
-- RETURNS: void;
-- 
-- NOTES:
-- Incrememnts a counter when sent packets are lost and displays it.
----------------------------------------------------------------------------------------------------------------------*/
VOID GUI_Lost()
{
	++lost;
	update();
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: GUI_ReceivedBad
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Ashley Tham
--
-- PROGRAMMER: Ashley Tham
--
-- INTERFACE: GUI_ReceivedBad(void)
--
-- RETURNS: void;
-- 
-- NOTES:
-- Incrememnts a counter when bad packets are received and displays it.
----------------------------------------------------------------------------------------------------------------------*/
VOID GUI_ReceivedBad()
{
	++badReceived;
	update();
}

VOID GUI_Message(TCHAR* text, unsigned len)
{
	len = (len > 35) ? 35 : len;
	HDC hdc = GetDC(hwndMainWin);
	TextOut(hdc, 410, 500, text, len);
	ReleaseDC(hwndMainWin, hdc);
}