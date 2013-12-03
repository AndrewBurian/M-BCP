/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:	Controller.cpp
--
-- PROGRAM:		M-BCP
--
-- FUNCTIONS:
--	DWORD WINAPI ProtocolControlThread(LPVOID threadParams);
--	DWORD RunEngine();
--	DWORD Active();
--	BOOL OutputAvailable();
--
-- DATE: 		December 03, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- The single packet version of the controller.
--
-- ISSUES:
-- Program currently set to ENQ continuously
-- Infinetly attempts retransmission
----------------------------------------------------------------------------------------------------------------------*/
#include "BCP.h"

queue<BYTE> *OutQueueCheck = NULL;
BOOL *done = NULL;

BOOL teardownReady = FALSE;
BOOL sendClear = FALSE;

DWORD RunEngine();
DWORD Active();
BOOL OutputAvailable();

int retries = 0;
int timeouts = 0;

HANDLE hQueueMutex = CreateMutex(NULL, FALSE, LOCK_OUTPUT);

HANDLE allEvents[8] = {
	CreateEvent(NULL, FALSE, FALSE, EVENT_ENQ),	// 0
	CreateEvent(NULL, FALSE, FALSE, EVENT_OUTPUT_AVAILABLE), // 1
	CreateEvent(NULL, TRUE, FALSE, EVENT_END_PROGRAM), // 2
	CreateEvent(NULL, FALSE, FALSE, EVENT_ACK), // 3
	CreateEvent(NULL, FALSE, FALSE, EVENT_NAK), // 4
	CreateEvent(NULL, FALSE, FALSE, EVENT_DATA_RECEIVED), // 5
	CreateEvent(NULL, FALSE, FALSE, EVENT_BAD_DATA_RECEIVED), // 6
	CreateEvent(NULL, FALSE, FALSE, EVENT_EOT)  // 7
};

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	ProtocolControlThread
--
-- DATE: 		December 3, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	DWORD WINAPI ProtocolControlThread(LPVOID threadParams)
--
-- RETURNS: 	DWORD
--
-- NOTES:
-- Assigns pointers and calls main run function.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI ProtocolControlThread(LPVOID threadParams)
{
	OutQueueCheck = ((SHARED_DATA_POINTERS*)threadParams)->p_quOutputQueue;
	done = ((SHARED_DATA_POINTERS*)threadParams)->p_bProgramDone;
	
	RunEngine();
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	RunEngine
--
-- DATE: 		December 3, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	DWORD RunEngine()
--
-- RETURNS: 	DWORD
--
-- NOTES:
-- Handles the ENQ handshake and possible ENQ collisions.
-- Passes control to Active() function for exchange handling.
----------------------------------------------------------------------------------------------------------------------*/
DWORD RunEngine()
{
	while (!(*done))
	{
		sendClear = FALSE;

		if (OutputAvailable())
			SetEvent(allEvents[1]);

		int signal = WaitForMultipleObjects(3, allEvents, FALSE, INFINITE);
		switch (signal)
		{
		case WAIT_OBJECT_0:		// ENQ
			SendENQ(TRUE);
			Active();
			break;

		case WAIT_OBJECT_0 + 1:	// Output
			SendENQ(FALSE);
			if (WaitForSingleObject(allEvents[0], TIMEOUT) != WAIT_TIMEOUT)	// Wait: ENQ
			{
				// ENQ recieved in response
				if (WaitForSingleObject(allEvents[3], 10) != WAIT_TIMEOUT)	// Wait: ACK (Check)
				{
					sendClear = TRUE;
					Active();
				}
				else
				{
					// ENQ received had no ack.
					// ENQ collision. Rand Wait
					signal = WaitForSingleObject(allEvents[0], rand() % (TIMEOUT * 2));
					if (signal == WAIT_OBJECT_0)
						SetEvent(allEvents[0]);
				}
			}
			break;

		case WAIT_OBJECT_0 + 2:	// End
			return 0;
		}
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Active
--
-- DATE: 		December 3, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	DWORD Active()
--
-- RETURNS: 	DWORD
--
-- NOTES:
-- Handles the continuous exchange until both sides have released connection.
----------------------------------------------------------------------------------------------------------------------*/
DWORD Active()
{
	BOOL resend = FALSE;
	BOOL reAck = TRUE;
	retries = 0;
	timeouts = 0;
	
	while (!(*done))
	{
		// --------------SEND--------------------

		if (sendClear && (OutputAvailable() || resend)) // data to send or resend
		{
			if (resend)
				Resend(reAck);
			else if (OutputAvailable())
				SendNext(reAck);

			GUI_Sent();
		}
		else if (sendClear)	// clear to send, but no data to send or resend.
		{
			if (teardownReady)
				return 0;	// other side just EOT'd. Transmission done
			SendEOT(reAck);
		}

		// ------------WAIT FOR REPLY--------------------

		int signaled = WaitForMultipleObjects(2, &allEvents[3], FALSE, TIMEOUT);	// Wait: ACK/NAK
		switch (signaled)
		{
		case WAIT_OBJECT_0:	//ACK
			resend = FALSE;
			break;

		case WAIT_TIMEOUT:	//Timeout
			if (++timeouts == 5)
				return 2;
			// fall through
		case WAIT_OBJECT_0 + 1:	//NAK
			if (sendClear)	// timeout waiting for reply
			{
				resend = TRUE;
				GUI_Lost();
				break;
			}
			else
			{
				return 1;	// timeout waiting for initial data
				// !!! Program will reach here if initial data packet contains NAK !!
			}
		}

		// -----------CHECK DATA IN REPLY-----------------

		signaled = WaitForMultipleObjects(3, &allEvents[5], FALSE, 10);	//Wait: Good/Bad/None (Check)
		switch (signaled)
		{
		case WAIT_OBJECT_0:	// Good
			reAck = TRUE;
			teardownReady = FALSE;
			GUI_Received();
			break;

		case WAIT_OBJECT_0 + 1: // Bad
			reAck = FALSE;
			teardownReady = FALSE;
			GUI_ReceivedBad();
			break;

		case WAIT_OBJECT_0 + 2: // EOT/No data
			reAck = TRUE;
			teardownReady = TRUE;
			break;
		}

		sendClear = TRUE;	// listened to one full data packet. Can now send.
	}
	return 0;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	OutputAvailable
--
-- DATE: 		December 3, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL OutputAvailable()
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Checks weather the output queue has content in a thread-safe way.
----------------------------------------------------------------------------------------------------------------------*/
BOOL OutputAvailable()
{
	BOOL available = FALSE;
	
	WaitForSingleObject(hQueueMutex, INFINITE);
	
	if (!OutQueueCheck->empty())
		available = TRUE;
	
	ReleaseMutex(hQueueMutex);

	return available;
}
