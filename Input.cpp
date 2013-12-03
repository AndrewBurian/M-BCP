/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:	Inut.cpp		The thread and functions that will continuously monitor
--								the serial port for incoming data and trigger the correct
--								events.
-- PROGRAM:		M-BCP
--
-- FUNCTIONS:
--	int ReadIn(byte* frame, unsigned len, DWORD wait);
--	VOID FillDataFrame();
--	VOID ReadCtrl();
--	DWORD WINAPI SerialReadThread(LPVOID threadParams);
--
-- DATE: 		November 02, 2013
--
-- REVISIONS: 	December 03, 2013
--				Andrew Burian
--				Updated to match M-BCP Protocol
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- NOTES:
-- 
----------------------------------------------------------------------------------------------------------------------*/
#include "BCP.h"
#define READ_COMPLETE	1
#define READ_TIMEOUT	2
#define END_OF_PROGRAM	3
#define READ_ERROR		4


HANDLE hEndProgram		= CreateEvent(NULL, TRUE, FALSE, EVENT_END_PROGRAM);
HANDLE hDataReceived	= CreateEvent(NULL, FALSE, FALSE, EVENT_DATA_RECEIVED);
HANDLE hBadDataReceived = CreateEvent(NULL, FALSE, FALSE, EVENT_BAD_DATA_RECEIVED);
HANDLE hAck				= CreateEvent(NULL, FALSE, FALSE, EVENT_ACK);
HANDLE hNak				= CreateEvent(NULL, FALSE, FALSE, EVENT_NAK);
HANDLE hEot				= CreateEvent(NULL, FALSE, FALSE, EVENT_EOT);
HANDLE hEnq				= CreateEvent(NULL, FALSE, FALSE, EVENT_ENQ);
HANDLE hInputAvailable	= CreateEvent(NULL, FALSE, FALSE, EVENT_INPUT_AVAILABLE);
HANDLE hInputLock		= CreateMutex(NULL, FALSE, LOCK_INPUT);

byte input[1025] = { NULL };

DWORD expected = DC1;

HANDLE *hInputCommPort = NULL;
BOOL *bProgramDone = NULL;
queue<byte>* quInputQueue = NULL;


/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	ReadIn
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	int ReadIn (byte* frame, unsigned len, DWORD wait)
--
-- RETURNS: 	int
--
-- NOTES:
-- Reads in the desired number of bytes (len) into the buffer (frame) for up to the timeout (wait).
-- Event driven and non-blocking internally. Function will however block until the event has been triggered
-- or the timeout has elapsed.
----------------------------------------------------------------------------------------------------------------------*/
int ReadIn(byte* frame, unsigned len, DWORD wait)
{
	OVERLAPPED ovrReadPort = { 0 };
	ovrReadPort.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Start Async Read

	ReadFile(*hInputCommPort, frame, len, NULL, &ovrReadPort);

	// wait for event imbedded in overlapped struct
	HANDLE events[] = { ovrReadPort.hEvent, hEndProgram };
	int result = WaitForMultipleObjects(2, events, FALSE, wait);

	switch (result)
	{
	case WAIT_OBJECT_0:
		ResetEvent(ovrReadPort.hEvent);
		return READ_COMPLETE;
	case WAIT_OBJECT_0 + 1:
		return END_OF_PROGRAM;
	case WAIT_TIMEOUT:
		return READ_TIMEOUT;
	case WAIT_ABANDONED:
		return READ_ERROR;
	}

	// how you would get here is beyond me, but probably failed horribly
	return READ_ERROR;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	FillDataFrame
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void FillDataFrame()
--
-- RETURNS: 	void
--
-- NOTES:
-- Uses ReadIn to attempt to read a full data frame into the buffer.
-- Then checks if the frame is a duplicate of a previous frame.
-- Then checks the CRC is valid.
-- If it is not a duplicate, and the CRC is valid, it will set the dataReceived event and read the content
-- of the frame into the input queue.
----------------------------------------------------------------------------------------------------------------------*/
VOID FillDataFrame()
{
	// fill the frame even if it's a duplicate to clear the buffer
	switch (ReadIn(&input[3], 1022, INFINITE))
	{
	// in all cases of failure, back out and let the main flow deal with it
	case READ_TIMEOUT:
		return;
	case READ_ERROR:
		return;
	case END_OF_PROGRAM:
		return;
	}

	// fill success, is it a duplicate?
	if (input[2] != expected)
	{
		// yes duplicate. Signal we received it and abandon it.
		SetEvent(hDataReceived);
		return;
	}

	// not a duplicate, is the crc valid?
	if (!CheckCRC(&input[1]))
	{
		// bad CRC, signal for bad data and abandon.
		SetEvent(hBadDataReceived);
		return;
	}

	// Data not duplicate, crc ok
	// we have a data frame.
	// signal we have it, update expecting, send to input buffer.
	SetEvent(hDataReceived);
	
	if (expected == DC1)
		expected = DC2;
	else
		expected = DC1;

	for (int i = 3; i < 1024; ++i)
	{
		WaitForSingleObject(hInputLock, INFINITE);
			quInputQueue->push(input[i]);
			SetEvent(hInputAvailable);
		ReleaseMutex(hInputLock);
	}

	// done
	return;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	ReadControl
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	void ReadCtrl()
--
-- RETURNS: 	void
--
-- NOTES:
-- Uses ReadIn to attempt to read the control portion of a frame into the buffer
-- Depending on the control character received, it will either call FillDataFrame
-- or it will signal the corresponding control event.
----------------------------------------------------------------------------------------------------------------------*/
VOID ReadCtrl()
{
	switch (ReadIn(&input[1], 2, INFINITE))
	{
	// in all cases of failure, back out and let the main flow deal with it
	case READ_TIMEOUT:
		return;
	case READ_ERROR:
		return;
	case END_OF_PROGRAM:
		return;
	}

	// read suceeded, check piggyback ack
	switch (input[1])
	{
	case ACK:
		SetEvent(hAck);
		break;
	case NAK:
		SetEvent(hNak);
		break;
	}
	
	
	// see what the ctrl char is.
	switch (input[2])
	{
		case DC1:
			// fall through
		case DC2:
			FillDataFrame();
			break;
		case EOT:
			SetEvent(hEot);
			expected = DC1;
			break;
		case ENQ:
			SetEvent(hEnq);
			break;
		}

	// data frame has been filled or failed
	// return to main flow for further handling
	return;
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	SerialReadThread
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	DWORD WINAPI SerialReadThread(LPVOID threadParams)
--
-- RETURNS: 	DWORD
--
-- NOTES:
-- Continuously attempts to read one character from the serial port while the 
-- program is still running.
-- If it reads a SYN, calls ReadCtrl to continue logic.
----------------------------------------------------------------------------------------------------------------------*/
DWORD WINAPI SerialReadThread(LPVOID threadParams)
{
	SHARED_DATA_POINTERS* dat = (SHARED_DATA_POINTERS*)threadParams;
	bProgramDone = dat->p_bProgramDone;
	quInputQueue = dat->p_quInputQueue;
	hInputCommPort = dat->p_hCommPort;

	while (!(*bProgramDone))
	{
		switch (ReadIn(&input[0], 1, INFINITE))
		{
		case READ_COMPLETE:
			if (input[0] != SYN)
				break;
			ReadCtrl();
			break;
		case READ_ERROR:
			// oh god... you're doomed
			// just kill the thread and abandon I guess.
			return 1;
		case READ_TIMEOUT:
			// wat
			// how
			// I don't even
			break;
		case END_OF_PROGRAM:
			return 0;
		}
	}
	return 0;
}