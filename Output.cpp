/*------------------------------------------------------------------------------------------------------------------
-- SOURCE FILE:	Output.cpp		A collection of functions that will be responsible
--								for sending data frames over the link from the output 
--								queue as well as the control frames.
-- PROGRAM:		M-BCP
--
-- FUNCTIONS:
--	BOOL WriteOut(byte* frame, unsigned len)
--	BOOL SendNext();
--	BOOL Resend();
--	BOOL SendACK();
--	BOOL SendNAK();
--	BOOL SendENQ();
--	BOOL SendEOT();
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
-- All functions return a boolean success value.
----------------------------------------------------------------------------------------------------------------------*/
#include "BCP.h"

byte dataFrame[1025] = { NULL };
byte ctrlFrame[3] = { NULL };
int SOTval = 1;

queue<BYTE> *quOutputQueue = NULL;
HANDLE *hOutputCommPort = NULL;
HANDLE hOutputOutLock = CreateMutex(NULL, FALSE, LOCK_OUTPUT);
HANDLE hShutOffOutput = CreateEvent(NULL, FALSE, FALSE, EVENT_OUTPUT_AVAILABLE);

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	WriteOut
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL WriteOut
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Syncronously writes the buffer (frame) to length (len) to the serial port.
----------------------------------------------------------------------------------------------------------------------*/
BOOL WriteOut(byte* frame, unsigned len)
{
	OVERLAPPED ovrOut = { 0 };
	ovrOut.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	// Start Async write
	WriteFile(*hOutputCommPort, frame, len, NULL, &ovrOut);
	
	// wait for event imbedded in overlapped struct
	int result = WaitForSingleObject(ovrOut.hEvent, INFINITE);

	ResetEvent(ovrOut.hEvent);
	
	switch (result)
	{
		case WAIT_OBJECT_0:
			return TRUE;
		case WAIT_TIMEOUT:
			return FALSE;
		case WAIT_ABANDONED:
			return FALSE;
	}

	// how you would get here is beyond me, but probably failed horribly
	return FALSE;

}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	Resend
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL Resend(BOOL ack)
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Sends the previously sent frame again using WriteOut.
-- If ack is TRUE, piggys an ACK, if FALSE, a NAK.
----------------------------------------------------------------------------------------------------------------------*/
BOOL Resend(BOOL ack)
{
	if (dataFrame[0] == NULL)
	{
		// no previously sent frame.
		// failed to resend
		return FALSE;
	}

	dataFrame[1] = (ack ? ACK : NAK);

	// write to port
	return WriteOut(dataFrame, 1025);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	SendNext
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL SendNext(BOOL ack)
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Packetizes and sends a new data frame using WriteOut.
-- If ack is TRUE, piggys an ACK, if FALSE, a NAK.
----------------------------------------------------------------------------------------------------------------------*/
BOOL SendNext(BOOL ack)
{
	// check for no data to send
	WaitForSingleObject(hOutputOutLock, INFINITE);
		BOOL isEmpty = quOutputQueue->empty();
	ReleaseMutex(hOutputOutLock);
	if (isEmpty)
		return FALSE;

	// start of frame
	dataFrame[0] = SYN;

	//piggybacked ack
	dataFrame[1] = (ack ? ACK : NAK);

	//SOT byte
	if (SOTval == 1)
	{
		dataFrame[2] = DC1;
		SOTval = 2;
	}
	else if (SOTval == 2)
	{
		dataFrame[2] = DC2;
		SOTval = 1;
	}


	// data portion
	int i = 3;
	for (i = 3; i < 1023; ++i)
	{
		WaitForSingleObject(hOutputOutLock, INFINITE);
			if (quOutputQueue->empty())
			{
				ReleaseMutex(hOutputOutLock);
				WaitForSingleObject(hShutOffOutput, 0);
				break;
			}
			dataFrame[i] = quOutputQueue->front();
			quOutputQueue->pop();
		ReleaseMutex(hOutputOutLock);
	}
	// pad if nessesary
	while (i < 1023)
	{
		dataFrame[i] = NULL;
		++i;
	}

	// add crc
	if (!MakeCRC(&dataFrame[1]))
		return FALSE;

	// write to port
	return WriteOut(dataFrame, 1025);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	SendENQ
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL SendENQ(BOOL ack)
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Sends and ENQ using WriteOut.
-- If ack is TRUE, piggys an ACK, if FALSE, a NULL.
----------------------------------------------------------------------------------------------------------------------*/
BOOL SendENQ(BOOL ack)
{
	ctrlFrame[0] = SYN;
	ctrlFrame[1] = (ack ? ACK : NULL);
	ctrlFrame[2] = ENQ;
	return WriteOut(ctrlFrame, 3);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	SendEOT
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	BOOL SendEOT(BOOL)
--
-- RETURNS: 	BOOL
--
-- NOTES:
-- Sends and EOT using WriteOut.
-- If ack is TRUE, piggys an ACK, if FALSE, a NAK.
----------------------------------------------------------------------------------------------------------------------*/
BOOL SendEOT(BOOL ack)
{
	ctrlFrame[0] = SYN;
	ctrlFrame[1] = (ack ? ACK : NAK);
	ctrlFrame[2] = EOT;
	return WriteOut(ctrlFrame, 3);
}

/*------------------------------------------------------------------------------------------------------------------
-- FUNCTION:	SetupOutput
--
-- DATE: 		November 2, 2013
--
-- REVISIONS: 	none
--
-- DESIGNER: 	Andrew Burian
--
-- PROGRAMMER: 	Andrew Burian
--
-- INTERFACE: 	VOID SetupOutput()
--
-- RETURNS: 	VOID
--
-- NOTES:
-- Sets up the output with pointers to the output queue and a handle to the comm port.
----------------------------------------------------------------------------------------------------------------------*/
VOID SetupOutput(SHARED_DATA_POINTERS* dat)
{
	quOutputQueue = dat->p_quOutputQueue;
	hOutputCommPort = dat->p_hCommPort;
}