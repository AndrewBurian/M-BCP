/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:	OpenDialog.cpp		A collection of functions for displaying output to the window (packet statistics
--									and received data).
--
-- PROGRAM:		BCP
--
-- FUNCTIONS:
--	void FileInitialize(HWND hwnd, OPENFILNAME *ofn);
--	BOOL FileOpenDlg(HWND hwnd, PSTR filename, OPENFILENAME openfilename);
--
-- DATE: 		November 26, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Ashley Tham
--
-- PROGRAMMERS: Ashley Tham
--
-- NOTES:
-- All functions return a boolean success value.
----------------------------------------------------------------------------------------------------------------------*/

#include "BCP.h"
#include <commdlg.h>

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: FileInitialize
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Ashley Tham
--
-- PROGRAMMER: Ashley Tham
--
-- INTERFACE: FileInitialize(HWND hwnd, OPENFILENAME *ofn)
-					HWND hwnd: Handle to the parent window.
--					OPENFILENAME *ofn: Points to a struct containing necessary info for opening a file.
--
-- RETURNS: void;
--
-- NOTES:
-- Sets up the OPENFILENAME structure to allow users to open files.
----------------------------------------------------------------------------------------------------------------------*/
void FileInitialize (HWND hwnd, OPENFILENAME *ofn)
{
	static TCHAR szFilter[] = TEXT ("Text Files (*.TXT)\0*.txt\0")  
		TEXT ("ASCII Files (*.ASC)\0*.asc\0")  
		TEXT ("All Files (*.*)\0*.*\0\0") ;

	ofn->lStructSize       = sizeof (OPENFILENAME) ;
	ofn->hwndOwner         = hwnd ;
	ofn->hInstance         = NULL ;
	ofn->lpstrFilter       = szFilter ;
	ofn->lpstrCustomFilter = NULL ;
	ofn->nMaxCustFilter    = 0 ;
	ofn->nFilterIndex      = 0 ;
	ofn->lpstrFile         = NULL ;          // Set in Open and Close functions
	ofn->nMaxFile          = MAX_PATH ;
	ofn->lpstrFileTitle    = NULL ;          // Set in Open and Close functions
	ofn->nMaxFileTitle     = MAX_PATH ;
	ofn->lpstrInitialDir   = NULL ;
	ofn->lpstrTitle        = NULL ;
	ofn->Flags             = 0 ;             // Set in Open and Close functions
	ofn->nFileOffset       = 0 ;
	ofn->nFileExtension    = 0 ;
	ofn->lpstrDefExt       = TEXT ("txt") ;
	ofn->lCustData         = 0L ;
	ofn->lpfnHook          = NULL ;
	ofn->lpTemplateName    = NULL ;
	ofn->Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION: FileOpenDlg
--
-- DATE: November 30, 2013
--
-- REVISIONS: None
--
-- DESIGNER: Ashley Tham
--
-- PROGRAMMER: Ashley Tham
--
-- INTERFACE: FileOpenDlg(HWND hwnd, PSTR pstrFileName OPENFILENAME *ofn)
--					HWND hwnd:			Handle to the parent window.
--					PSTR pstrFileName:	Name of the file to open
--					OPENFILENAME *ofn:	Points to a struct containing necessary info for opening a file.
--
-- RETURNS: void;
--
-- NOTES:
-- Sets up the OPENFILENAME structure to allow users to open files.
----------------------------------------------------------------------------------------------------------------------*/
BOOL FileOpenDlg (HWND hwnd, PTSTR pstrFileName, OPENFILENAME *ofn)
{
	ofn->hwndOwner         = hwnd ;
	ofn->lpstrFile         = pstrFileName ;
	

	return GetOpenFileName (ofn) ;
}