/*
		 The^day^of^DooM

	  For AVR-LPC (v.0.0.7):

	Last Update:	20.08.2011 - Add DAC Support.
	Last Update:	08.05.2012
	Full Channge:	09.01.2013 - Add ModBUS Master LIB
	Last Update:	11.01.2013
	Last Update:	26.02.2013

		  For ROBKO 01:

	Full Channge:	19.05.2013 - Add ModBUS Suport
	Last Update:	21.05.2013
	Last Update:	30.05.2013
	Last Update:	01.06.2013
	Last Update:	08.06.2013
	Full Channge:	05.12.2013 - Bug Fix in CNC function G01
	Last Update:	05.12.2013

		  For DC Servo:

	Full Channge:	01.02.2014
	Last Update:	05.01.2014
	Last Update:	06.01.2014
*/

#include "stdafx.h"
#include "resource.h"

#include "modbus/modbus.h"
#include "modbus/modbus-rtu.h"
#include "modbus/modbus-tcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <process.h>
#include <windowsx.h>
#include <commctrl.h>

typedef struct PID_DATA {
	//! Last process value, used to find derivative of process value.
	//int16_t
	WORD wLastProcessValue;
	//! Summation of errors, used for integrate calculations
	//int32_t
	DWORD wSumError;
	//! The Proportional tuning constant, multiplied with SCALING_FACTOR
	//int16_t
	int wP_Factor;
	//! The Integral tuning constant, multiplied with SCALING_FACTOR
	//int16_t
	int wI_Factor;
	//! The Derivative tuning constant, multiplied with SCALING_FACTOR
	//int16_t
	int wD_Factor;
	//! Maximum allowed error, avoid overflow
	//int16_t
	WORD wMaxError;
	//! Maximum allowed sumerror, avoid overflow
	DWORD dwMaxSumError;
} pidData_t;

typedef struct {
	///////////////////////////////////////////////////////////////////////
	HWND hwnd;
	HINSTANCE hInst;

	HANDLE hComThread;

	MB mbMaster;
	unsigned char SlaveID;

	unsigned int flagPidWrite;
	HWND hwndPidSettingsDialog;

	pidData_t pidData;
	///////////////////////////////////////////////////////////////////////
	unsigned int fButton, idButton;
	
	unsigned int fButtonRun, idButtonRun;

	unsigned char inPort[15], outPort[15];

	char flagWriteCoils, idCoilForChannge;
} MAIN_DATA, *LP_MAIN_DATA;

LP_MAIN_DATA mainDataGet( HWND hwnd );
void mainDataSet( HWND hwnd, LP_MAIN_DATA lpMainData );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI comThreadFunc( LPVOID lpParam );

LRESULT CALLBACK mainWndFunc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK AboutWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK tcpSetingsWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK PidSettingsWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );

void inPortToDiscBuff( unsigned char *ucRegDiscBuf, unsigned char *inPort, unsigned char n )
{
	int i, j;

	for( i = 0; i < n; i++ ) {
		j = i / 8;

		if( inPort[ i ] ) {
			ucRegDiscBuf[ j ] |=  ( 1<<( i - ( 8 * j ) ) );
		} else {
			ucRegDiscBuf[ j ] &= ~( 1<<( i - ( 8 * j ) ) );
		}
	}
}

int APIENTRY WinMain
(
	HINSTANCE	hInstance,
	HINSTANCE	hPrevInstance,
	LPSTR		lpCmdLine,
	int			nCmdShow
)
{
	unsigned char inPort[20] = {
		0, 1, 0, 1, 0, 1, 0, 1,
		0, 1, 0, 1, 0, 1, 0, 1,
		0, 1, 0, 1
	};

	unsigned char t[3] = { 0 };

	inPortToDiscBuff( t, inPort, 20 );

	//return 0;

	InitCommonControls();

	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)mainWndFunc);
}

LRESULT CALLBACK mainWndFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char szBuffer[256];
	int wmId	= LOWORD(wParam);
	int wmEvent	= HIWORD(wParam);
	LP_MAIN_DATA lpMainData = mainDataGet(hwnd);

	switch(msg) {
	case WM_INITDIALOG: {
		int i;
		HWND hwndCombo;

		LoadIcon((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), (LPCTSTR)IDI_IDE);

		lpMainData = (LP_MAIN_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MAIN_DATA));
		if(NULL == lpMainData) {	
			EndDialog(hwnd, 0);
			return FALSE;
		}
		mainDataSet(hwnd, lpMainData);

		lpMainData->hwnd = hwnd;
		lpMainData->hInst = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
		//////////////////////////////////////////////////////////////////////////////
		lpMainData->SlaveID = 10;

		wsprintf(lpMainData->mbMaster.szIP, "192.168.0.11");
		wsprintf(lpMainData->mbMaster.szTcpPort, "502");

		serialInitDefDCB( &lpMainData->mbMaster.dcb );
		lpMainData->mbMaster.hwnd = lpMainData->hwnd;
		lpMainData->mbMaster.hCom = INVALID_HANDLE_VALUE;
		////////////////////////////////////////////////////////////////////////////////////////
		hwndCombo = GetDlgItem(hwnd, IDC_COMBO_SLAVE_ID);
		for(i = 1; i < 248; i++) {
			sprintf(szBuffer, "%d", i);
			ComboBox_AddString(hwndCombo, szBuffer);
		}
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SLAVE_ID), lpMainData->SlaveID - 1);

		LoadMenu((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDR_MENU_MAIN));

		for(i = 1; i < 9; i++) {
			sprintf( szBuffer, "COM%d", i );
			ComboBox_AddString( GetDlgItem(hwnd, IDC_COMBO_PORT), szBuffer );
		}

		const HWND hDesktop = GetDesktopWindow( );
		RECT desktopRect, rect;

		GetWindowRect( hDesktop, &desktopRect );
		GetWindowRect( hwnd, &rect );

		MoveWindow(
			hwnd,
			( desktopRect.right - (rect.right - rect.left) ) / 2,
			( desktopRect.bottom - (rect.bottom - rect.top) ) / 2,
			rect.right - rect.left,
			rect.bottom - rect.top,
			TRUE
		);
	}
	 return TRUE;

	case WM_COMMAND: {

		switch(wmEvent) {
		
		case CBN_SELCHANGE: {
			HWND hwndCombo = (HWND)lParam;

			switch(wmId) {
			case IDC_COMBO_SLAVE_ID:
				lpMainData->SlaveID = ComboBox_GetCurSel( hwndCombo ) + 1;
			 break;
			}
		}
		 return TRUE;

		case BN_CLICKED: {
			
			if( wmId >= IDC_Q0 && wmId <= IDC_Q5 ) {
				if( !lpMainData->flagWriteCoils ) {
					lpMainData->idCoilForChannge = wmId - IDC_Q0;
					lpMainData->flagWriteCoils = 1;
				}
				return TRUE;
			}

			switch(wmId) {
			
			//case IDC_BUTTON_EMS: {
			//	lpMainData->fButton = IDC_BUTTON_EMS;
			//}
			 return TRUE;

			case IDM_CONNECTION_SETINGS:
				if( INVALID_HANDLE_VALUE != lpMainData->mbMaster.hCom ) {
					COMMCONFIG CC;

					sprintf(szBuffer, "COM%d", lpMainData->mbMaster.uiComIndex);

					CC.dcb = lpMainData->mbMaster.dcb;
					CommConfigDialog( szBuffer, lpMainData->hwnd, &CC );
					lpMainData->mbMaster.dcb.Parity = CC.dcb.Parity;
					lpMainData->mbMaster.dcb.BaudRate = CC.dcb.BaudRate;
					lpMainData->mbMaster.dcb.StopBits = CC.dcb.StopBits;
					SetCommState(lpMainData->mbMaster.hCom, &lpMainData->mbMaster.dcb);

					return TRUE;
				}
			 break;

			case IDM_CONNECTION_COMSELECT_COM1: case IDM_CONNECTION_COMSELECT_COM2:
			case IDM_CONNECTION_COMSELECT_COM3: case IDM_CONNECTION_COMSELECT_COM4:
			case IDM_CONNECTION_COMSELECT_COM5: case IDM_CONNECTION_COMSELECT_COM6:
			case IDM_CONNECTION_COMSELECT_COM7: case IDM_CONNECTION_COMSELECT_COM8: {
				HMENU hMenu = GetMenu(hwnd);
				unsigned int com = 1 + (wmId - IDM_CONNECTION_COMSELECT_COM1);

				SendMessage(hwnd, WM_COMMAND, BN_CLICKED<<16 | IDM_DISCONNECT, 0);

				if( mbMasterInit(&lpMainData->mbMaster, 1) ) {
					return FALSE;
				}

				if( !mbRTUMasterConnect(&lpMainData->mbMaster, com) ) {
					lpMainData->mbMaster.uiComIndex = com;
					////////////////////////////////////////////////////////////////////////////////////////////
					EnableMenuItem(hMenu, IDM_CONNECTION_SETINGS, MF_ENABLED);
					EnableMenuItem(hMenu, IDM_CONNECTION_DISCONNECT, MF_ENABLED);

					CheckMenuRadioItem( hMenu, IDM_CONNECTION_COMSELECT_COM1,
										IDM_CONNECTION_COMSELECT_COM8,
										wmId, MF_BYCOMMAND | MF_CHECKED
					);
					////////////////////////////////////////////////////////////////////////////////////////////
					DWORD dwThreadId;
					lpMainData->hComThread = CreateThread(NULL, 4096, comThreadFunc, lpMainData, 0, &dwThreadId);
					SetThreadPriority(lpMainData->hComThread,	THREAD_PRIORITY_TIME_CRITICAL);
					////////////////////////////////////////////////////////////////////////////////////////////
					return TRUE;
				}
			}
			 break;

			case IDM_CONNECTION_TCP_IP: {
//				HMENU hMenu = GetMenu(hwnd);

				SendMessage( hwnd, WM_COMMAND, BN_CLICKED<<16 | IDM_DISCONNECT, 0 );

				if( mbMasterInit(&lpMainData->mbMaster, 2) ) {
					return FALSE;
				}

				switch(DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_TCP_SETINGS), hwnd, (DLGPROC)tcpSetingsWndProc)) {
				case 0:
				 return false;
				case 1:
				 break;
				case 2:
				 return false;

				default: return false;
				}

				lpMainData->fButton = IDM_CONNECTION_TCP_IP;

				DWORD dwThreadId;
				lpMainData->hComThread = CreateThread(NULL, 4096, comThreadFunc, lpMainData, 0, &dwThreadId);
				SetThreadPriority(lpMainData->hComThread,	THREAD_PRIORITY_TIME_CRITICAL);
				////////////////////////////////////////////////////////////////////////////////////////////
			}
			 return TRUE;

			case IDM_DISCONNECT: {
				HMENU hMenu = GetMenu(hwnd);

				EnableMenuItem(hMenu, IDM_CONNECTION_SETINGS, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_CONNECTION_DISCONNECT, MF_GRAYED);

				CheckMenuRadioItem( hMenu, IDM_CONNECTION_COMSELECT_COM1,
									IDM_CONNECTION_COMSELECT_TCP_IP,
									wmId, MF_BYCOMMAND | MF_UNCHECKED
				);

				if( INVALID_HANDLE_VALUE != lpMainData->hComThread ) {
					TerminateThread(lpMainData->hComThread, 0);
					CloseHandle(lpMainData->hComThread);
					lpMainData->hComThread = INVALID_HANDLE_VALUE;
				}

				mbMasterDisconnect( &lpMainData->mbMaster );
			}
			 return TRUE;
			// --------------------------------------------------------------------------------------------------------------------
			case IDCANCEL:
				if(NULL != lpMainData) {

					if( INVALID_HANDLE_VALUE != lpMainData->hComThread ) {
						TerminateThread( lpMainData->hComThread, 0 );
						CloseHandle( lpMainData->hComThread );
						lpMainData->hComThread = INVALID_HANDLE_VALUE;
					}

					mbMasterDisconnect( &lpMainData->mbMaster );

					HeapFree(GetProcessHeap (), GWL_USERDATA, lpMainData);
				}

				EndDialog(hwnd, 0);
			 return TRUE;

			case ID_HELP_ABOUT:
				DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, (DLGPROC)AboutWndProc);
			 return TRUE;

			case ID_PID_SETTINGS:
				DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_DIALOG_PID_SETTINGS), hwnd, (DLGPROC)PidSettingsWndProc);
			 return TRUE;
			}

		}
		 break;
		} // END: BN_CLICKED

	} // END: WM_COMMAND
	 return TRUE;
	}

	return FALSE;
}

DWORD WINAPI comThreadFunc(LPVOID lpParam)
{
	LP_MAIN_DATA lpMainData = (LP_MAIN_DATA)lpParam;
	unsigned int i = 0, uiResultReadPos;
	char szBuffer[256];
	int iResult = 0;
	int t1 = 0, t2 = 0;

	while( 1 ) {
		if( 2 == lpMainData->mbMaster.ascii_or_rtu_or_tcp ) {
			char fReconect = 0;
			
			if( 1 != send( lpMainData->mbMaster.tcpSocket, " ", 1, 0 ) ) {
				fReconect = 1;
			}

			if( fReconect || IDM_CONNECTION_TCP_IP == lpMainData->fButton  ) {
				if( mbTCPMasterConnect( &lpMainData->mbMaster, lpMainData->mbMaster.szIP, atoi(lpMainData->mbMaster.szTcpPort)) ) {
					Sleep(1);
					continue;
				} else {
					HMENU hMenu = GetMenu( lpMainData->hwnd );

					EnableMenuItem( hMenu, IDM_CONNECTION_DISCONNECT, MF_ENABLED );
					CheckMenuRadioItem( hMenu,
										IDM_CONNECTION_COMSELECT_COM1,
										IDM_CONNECTION_COMSELECT_TCP_IP,
										IDM_CONNECTION_COMSELECT_TCP_IP,
										MF_BYCOMMAND | MF_CHECKED
					);

					lpMainData->fButton = 0;
				}
			}
		}

		if( ++t1 > 4 ) {
			t1 = 0;
			iResult = mdReadDiscreteInputs(
				&lpMainData->mbMaster, lpMainData->SlaveID,
				(char*)lpMainData->inPort,
				0, 15
			);
			if( !iResult ) {
				for(i = 0; i < 10; i++) {
					sprintf( szBuffer, "I%d: %s", i, (1 & lpMainData->inPort[i]) ? "ON" : "OFF" );
					Static_SetText(GetDlgItem(lpMainData->hwnd, i + IDC_I0), szBuffer);
				}

		//		sprintf(buffer, "FB: %s", (1 & inPort[i]) ? "1" : "0");
		//		Static_SetText(GetDlgItem(lpMainData->hwnd, IDC_FB), buffer);

				for(i = 11; i < 14; i++) {
		//			sprintf(buffer, "FB%d: %s", i - 11, (1 & inPort[i]) ? "1" : "0");
		//			Static_SetText(GetDlgItem(lpMainData->hwnd, (i - 11) + IDC_FB0), buffer);
				}
			} else {
//				break;
			}
		}

		if( IDC_BUTTON_EMS == lpMainData->fButton ) {
			iResult = mbWriteSingleCoil( &lpMainData->mbMaster, lpMainData->SlaveID, 0, 1 );
			if( !iResult ) {
				lpMainData->fButton = 0;
			}
		}

		if( ++t2 > 5 || lpMainData->flagWriteCoils ) {
			t2 = 0;
			iResult = mbReadCoils(&lpMainData->mbMaster, lpMainData->SlaveID, (char*)lpMainData->outPort, 0, 12);
			if( !iResult ) {
				sprintf( szBuffer, "EMS: %d", lpMainData->outPort[0] );
				Button_SetText( GetDlgItem( lpMainData->hwnd, IDC_BUTTON_EMS ), szBuffer );

				sprintf( szBuffer, "%s", ( lpMainData->outPort[2] ) ? "ON (Q2)" : "OFF (Q2)" );
				Button_SetText( GetDlgItem( lpMainData->hwnd, IDC_Q2), szBuffer );

				sprintf( szBuffer, "%s", ( lpMainData->outPort[3] ) ? "B (Q3)" : "F (Q3)" );
				Button_SetText( GetDlgItem( lpMainData->hwnd, IDC_Q3 ), szBuffer );

				sprintf( szBuffer, "%s", ( lpMainData->outPort[4] ) ? "CS (Q4)" : "CC (Q4)" );
				Button_SetText( GetDlgItem( lpMainData->hwnd, IDC_Q4 ), szBuffer );

				if( lpMainData->flagWriteCoils ) {
					unsigned char temp[12];

					t2 = 5;

					lpMainData->flagWriteCoils = 0;

					memcpy( temp, lpMainData->outPort, sizeof(temp) );
					temp[ lpMainData->idCoilForChannge ] ^= 1;

					iResult = mbWriteMultipleCoils( &lpMainData->mbMaster, lpMainData->SlaveID, 0, 12, temp );
					if( !iResult ) {
					} else {
//						break;
					}
				}
			} else {
//				break;
			}
		}

		unsigned int arr_new[20] = { 0 };

		uiResultReadPos = mbReadInputRegister( &lpMainData->mbMaster, lpMainData->SlaveID, 0, 13, arr_new );
		if( !uiResultReadPos ) {
			sprintf( szBuffer, "%d", mbU16toSI( arr_new[ 2 ] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_SP_CURRENT ), szBuffer );

			sprintf( szBuffer, "%d", arr_new[ 0 ] );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_FB_CURRENT ), szBuffer );

			sprintf( szBuffer, "%d", mbU16toSI( arr_new[ 3 ] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_SP_SPEED ), szBuffer );

			sprintf( szBuffer, "%d", mbU16toSI( arr_new[ 1 ] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_FB_SPEED ), szBuffer );

			sprintf( szBuffer, "%d", mbU16toSI( arr_new[ 4 ] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_PID_CURRENT_OUT ), szBuffer );

			sprintf( szBuffer, "%d", mbU16toSI( arr_new[ 5 ] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_PID_SPEED_OUT ), szBuffer );

			sprintf( szBuffer, "%d", arr_new[ 6 ] );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_CURRENT_LIMIT_SP ), szBuffer );

			sprintf( szBuffer, "%d", mbU16toSI( arr_new[7] ) );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_PWM ), szBuffer );

			sprintf( szBuffer, "%d", arr_new[8] );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_OCR1A ), szBuffer );

			sprintf( szBuffer, "%d", arr_new[9] );
			Static_SetText( GetDlgItem( lpMainData->hwnd, IDC_STATIC_OCR1B ), szBuffer );
		}

		switch( lpMainData->flagPidWrite ) {

		case IDC_BUTTON_READ_PID_ALL_SETINGS: {
			uiResultReadPos = mbReadHoldingRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 0, 20, arr_new );
			if( !uiResultReadPos ) {
				for( i = 0; i < 4; i++ ) {
					arr_new[i] = mbU16toSI( arr_new[i] );
				}

				sprintf( szBuffer, "%d", arr_new[0] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_CURRENT_P_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[1] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_CURRENT_I_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[2] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_CURRENT_D_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[3] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_CURRENT_SCALING_FACTOR ), szBuffer );
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				sprintf( szBuffer, "%d", arr_new[10] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_SPEED_P_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[11] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_SPEED_I_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[12] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_SPEED_D_FACTOR ), szBuffer );

				sprintf( szBuffer, "%d", arr_new[13] );
				Static_SetText( GetDlgItem( lpMainData->hwndPidSettingsDialog, IDC_STATIC_PID_SPEED_SCALING_FACTOR ), szBuffer );
			}

			lpMainData->flagPidWrite = 0;
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_PID_CURRENT_P_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				0,
				mbSItoU16( lpMainData->pidData.wP_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;

		case IDC_BUTTON_WRITE_PID_CURRENT_I_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				1,
				mbSItoU16( lpMainData->pidData.wI_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;

		case IDC_BUTTON_WRITE_PID_CURRENT_D_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				2,
				mbSItoU16( lpMainData->pidData.wD_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_PID_SPEED_P_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				10,
				mbSItoU16( lpMainData->pidData.wP_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;

		case IDC_BUTTON_WRITE_PID_SPEED_I_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				11,
				mbSItoU16( lpMainData->pidData.wI_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;

		case IDC_BUTTON_WRITE_PID_SPEED_D_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				12,
				mbSItoU16( lpMainData->pidData.wD_Factor )
			);
			lpMainData->flagPidWrite = 0;
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		}

		Sleep(1);
	}

	mbMasterDisconnect( &lpMainData->mbMaster );

	return false;
}

LRESULT CALLBACK PidSettingsWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	LP_MAIN_DATA lpMainData = mainDataGet( GetParent(hDlg) );
	char szBuffer[100];

	switch(message){
	case WM_INITDIALOG:
		//Edit_SetText(GetDlgItem(hDlg, IDC_IPADDRESS), lpMainData->mbMaster.szIP);
		//Edit_SetText(GetDlgItem(hDlg, IDC_EDIT_TCP_PORT), lpMainData->mbMaster.szTcpPort);
	 return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam) ) {
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_READ_PID_SETINGS: {
			if( !lpMainData->flagPidWrite ) {
				lpMainData->flagPidWrite = IDC_BUTTON_READ_PID_SETINGS;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_PID_CURRENT_P_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_CURRENT_P_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wP_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_CURRENT_P_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_PID_CURRENT_I_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_CURRENT_I_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wI_Factor = atoi( szBuffer );
				
				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_CURRENT_I_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_PID_CURRENT_D_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_CURRENT_D_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wD_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_CURRENT_D_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_PID_SPEED_P_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_SPEED_P_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wP_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_SPEED_P_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_PID_SPEED_I_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_SPEED_I_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wI_Factor = atoi( szBuffer );
				
				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_SPEED_I_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_PID_SPEED_D_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_PID_SPEED_D_FACTOR ), szBuffer, 16 );
				lpMainData->pidData.wD_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_PID_SPEED_D_FACTOR;
				lpMainData->hwndPidSettingsDialog = hDlg;
			}
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDOK: {
			if( NULL != lpMainData ) {
				EndDialog(hDlg, 1);
			} else {
				EndDialog(hDlg, 0);
			}
		}
		 break;

		case IDCANCEL: {
			EndDialog(hDlg, 2);
		}
		 break;

		}

	 return TRUE;
	}
    return FALSE;
}

LRESULT CALLBACK tcpSetingsWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	LP_MAIN_DATA lpMainData = mainDataGet(GetParent(hDlg));

	switch(message){
	case WM_INITDIALOG:
		Edit_SetText(GetDlgItem(hDlg, IDC_IPADDRESS), lpMainData->mbMaster.szIP);
		Edit_SetText(GetDlgItem(hDlg, IDC_EDIT_TCP_PORT), lpMainData->mbMaster.szTcpPort);
	 return TRUE;

	case WM_COMMAND:
		
		switch(LOWORD(wParam)) {
		case IDOK: {
			if(NULL != lpMainData) {
				Edit_GetText(GetDlgItem(hDlg, IDC_IPADDRESS), lpMainData->mbMaster.szIP, 16);
				Edit_GetText(GetDlgItem(hDlg, IDC_EDIT_TCP_PORT), lpMainData->mbMaster.szTcpPort, 12);

				EndDialog(hDlg, 1);
			} else {
				EndDialog(hDlg, 0);
			}
		}
		 break;

		case IDCANCEL: EndDialog(hDlg, 2); break;
		}

	 return TRUE;
	}
    return FALSE;
}

LRESULT CALLBACK AboutWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message){
	case WM_INITDIALOG: return TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}
    return FALSE;
}

LP_MAIN_DATA mainDataGet(HWND hwnd)
{
	return (LP_MAIN_DATA)GetWindowLong(hwnd, GWL_USERDATA);
}

void mainDataSet(HWND hwnd, LP_MAIN_DATA lpMainData)
{
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpMainData);
}
