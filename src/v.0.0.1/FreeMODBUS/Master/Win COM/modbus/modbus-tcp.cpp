#include "stdafx.h"

#include "modbus.h"
#include "modbus-tcp.h"

// FC2 - Read Discrete Inputs
int mdTCPReadDiscreteInputs
(
	LPMB lpMb,
	unsigned char deviceID,
	char *input, unsigned int start,
	unsigned int end
)
{
	int iResult;
	unsigned int i;
	char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 10 + ( end - start ) / 8 );
	if( iResult ) {
		return iResult;
	}

	//-------------------------------------------------------------------------------
	if( 0x02 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	if( (1 + ( end - start ) / 8 ) != (unsigned)rxBuffer[8] ) {
		return 8; // Error: Byte Count
	}
	//-------------------------------------------------------------------------------

	for(i = start; i < end - start; i++) {
		input[i] = 1 & ( rxBuffer[ 9 + i / 8 ]>>(i - 8 * ( i / 8)) );
	}

	return 0;
}

// FC1 - Read Internal Bits or Physical Coils
int mbTCPReadCoils
(
	LPMB lpMb,
	unsigned char deviceID,
	char *output,
	unsigned int start,
	unsigned int lenght
)
{
	int iResult;
	unsigned int i;
	char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 10 + ( lenght - start ) / 8 );
	if( iResult ) {
		return iResult;
	}

	if( 1 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	if( (1 + (lenght - start) / 8 ) != (unsigned)rxBuffer[8] ) {
		return 8; // Error: Byte Count
	}

	for(i = start; i < start + lenght; i++) {
		output[i] = 1 & ( rxBuffer[9 + i / 8]>>(i - 8 * ( i / 8)) );
	}

	return 0;
}

// FC5 - Write Internal Bits or Physical Coils
int mbTCPWriteSingleCoil
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int coilAddress,
	unsigned char coilIsOn
)
{
	int iResult;
	char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 12 );
	if( iResult ) {
		return iResult;
	}

	if( 5 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	if( 1 != ((unsigned)rxBuffer[8]<<8 | rxBuffer[9]) ) {
		return 8; // Error: Reference Number
	}

	return 0;
}

// FC15 - Internal Bits or Physical Coils
int mbTCPWriteMultipleCoils
(
	LPMB lpMb,
	unsigned char deviceID
) // ????????????
{
	int iResult;
	char rxBuffer[100];
	
	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 12 );
	if( iResult ) {
		return iResult;
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////
// 16-bit access:
//
// FC4 - Read Physical Input Registers
int mbTCPReadInputRegister
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int regAddress,
	unsigned char wordCount,
	unsigned int *regValue
)
{
	int iResult;
	unsigned int i, j = 8;
	unsigned char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, (char*)rxBuffer, 9 + 2 * wordCount );
	if( iResult ) {
		return iResult;
	}

	if( 4 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	if( (2 * wordCount) != rxBuffer[8] ) {
		return 9; // Error: Byte Count
	}

	for(i = 0; i < wordCount; i++) {
		regValue[i] = rxBuffer[++j]<<8 | rxBuffer[++j];
	}

	return 0;
}
// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / /
// FC3 - Read Holding Registers
int mbTCPReadHoldingRegisters
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int regAddress,
	unsigned int wordCount,
	unsigned int *regValue
)
{
	int iResult;
	unsigned int i, j = 8;
	unsigned char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, (char*)rxBuffer, 9 + 2 * wordCount );
	if( iResult ) {
		return iResult;
	}

	if( 3 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	if( 2 * wordCount != rxBuffer[8] ) {
		return 9; // Error: Byte Count
	}

	for(i = 0; i < wordCount; i++ ) { // ???
		regValue[i] = rxBuffer[++j]<<8 | rxBuffer[++j];
	}

	return 0;
}

// FC6 - Write Internal Registers or Physical Output Registers
int mbTCPWriteSingleRegister
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int regAddress,
	unsigned int regValue
)
{
	int iResult;
	//unsigned int i;
	char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 12 );
	if( iResult ) {
		return iResult;
	}

	if( 6 != rxBuffer[7] ) {
		return 7; // Error: Function Code
	}

	//if( 1 != (unsigned)rxBuffer[8]<<8 | (unsigned)rxBuffer[9]) {
	//	return 8; // Error: Reference Number
	//}

	return 0;
}

// FC16 - Internal Registers or Physical Output Registers
int mbTCPWriteMultipleRegisters
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int regAddress,
	unsigned int wordCount
)
{
	int iResult;
	char rxBuffer[100];

	iResult = mbTCPMsterRead( lpMb, deviceID, rxBuffer, 9 + 2 * wordCount );
	if( iResult ) {
		return iResult;
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////
int mbTCPMasterSend
(
	LPMB lpMb
)
{
	int i, iResult;
	char txBuffer[1000];

	if( !lpMb->enable ) {
		return 1;
	}

	++lpMb->TID;

	txBuffer[0] = lpMb->TID>>8;				// TID (0..n)
	txBuffer[1] = lpMb->TID;
	txBuffer[2] = 0;						// PID (ModBus: 0)
	txBuffer[3] = 0;
	txBuffer[4] = lpMb->txBufferLenght>>8;	// Lenght
	txBuffer[5] = lpMb->txBufferLenght;

	for( i = 0; i < lpMb->txBufferLenght; i++ ) {
		txBuffer[6 + i] = lpMb->txBuffer[i];
	}

	iResult = send(lpMb->tcpSocket, txBuffer, 6 + lpMb->txBufferLenght, 0);
	if( SOCKET_ERROR == iResult ) {
		return 2;
	}

	return 0;
}

#include <sys/types.h>
//#include <socket.h>
//#include <sys/time.h>
#include <stdio.h>

int mbTCPMsterRead
(
	LPMB lpMb,
	char deviceID,
	char *rxBuffer,
	unsigned int len
)
{
	int iResult = 0;
	int dwBytesRead;

	///////////////////////// ??? /////////////////////////////
	/*fd_set fds;
	struct timeval timeout;
	int rc, result;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	FD_ZERO( &fds );
	FD_SET( lpMb->tcpSocket, &fds );

	rc = select( 8 * sizeof(fds), &fds, NULL, NULL, &timeout );
	if( -1 == rc ) {
		return -1;
	}*/
	///////////////////////////////////////////////////////////

	dwBytesRead = recv(lpMb->tcpSocket, rxBuffer, len, 0);

	if( !dwBytesRead ) {
//		printf("Connection closed\n");
		return 1;
	}

	if( dwBytesRead < 0 ) {
//		printf("recv failed: %d\n", WSAGetLastError());
		return 2;
	}

	mbTCPMasterCheckMBAPHeader(lpMb, deviceID, rxBuffer);

	if( iResult ) {
		iResult += 2;
	}

	return iResult;

}
//////////////////////////////////////////////////////////////////////////////////////////////
int mbTCPMasterCheckMBAPHeader
(
	LPMB lpMb,
	char deviceID,
	char *rxBuffer
)
{
	if( (signed)lpMb->TID != (rxBuffer[0]<<8 | rxBuffer[1]) ) {
		//return 1; // Error: TID
	}

	if( rxBuffer[3] || rxBuffer[2] ) {
		return 2; // Error: Protocol
	}

	if( (3 + rxBuffer[8]) != (rxBuffer[4]<<8 | rxBuffer[5]) ) {
		return 3; // Error: Lenght
	}

	if( deviceID != rxBuffer[6] ) {
		return 4; // Error: deviceID
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int mbTCPMasterConnect
(
	LPMB lpMb,
	char *szIP,
	int port
)
{
	if( socketTryConnect( &lpMb->tcpSocket, inet_addr(szIP), port ) ) {
		return 2;
	}

	lpMb->enable = 1;

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int socketTryConnect
(
	SOCKET *s,
	long hostname,
	int PortNo
)
{
	WSADATA w;
	int iResult;
	SOCKADDR_IN target;

	iResult = WSAStartup(MAKEWORD(2,2), &w);
	if( NO_ERROR != iResult ) {
		return 1;
	}

	if(MAKEWORD(2, 2) != w.wVersion) {
		WSACleanup();
		return 2;
	}

	*s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if( INVALID_SOCKET == *s) {
		return 3;
	}

	target.sin_family = AF_INET;
	target.sin_port = htons(PortNo);
	target.sin_addr.s_addr = hostname;

	iResult = connect(*s, (SOCKADDR*)&target, sizeof(target));
	if( SOCKET_ERROR == iResult ) {
		closesocket(*s);
		return 4;
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
