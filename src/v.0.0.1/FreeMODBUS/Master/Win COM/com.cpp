//	Last Update:	20.08.2011 - Add DAC Support.
//	Last Update:	20.08.2011 - Add DAC Support.
//	Last Update:	08.05.2012
//	Full Channge:	09.01.2013 - Add ModBUS Master LIB
//	Last Update:	11.01.2013
//	Last Update:	26.02.2013

#include "stdafx.h"
#include "com.h"

unsigned char inPort[16], outPort[16];
unsigned int arrADC[7], arrDAC[2] = { 0, 0 };

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	return DialogBox(hInstance, MAKEINTRESOURCE( IDD_MAIN ), NULL, (DLGPROC)mainWndFunc);
}

LRESULT CALLBACK mainWndFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int wmId	= LOWORD(wParam);
	int wmEvent	= HIWORD(wParam);
	LP_MAIN_DATA lpMainData = mainDataGet(hwnd);

	switch(msg) {
	case WM_INITDIALOG: {
		int i;
		HWND hwndCombo;
		char szBuffer[100];

		InitCommonControls();
		LoadIcon((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), (LPCTSTR)IDD_MAIN);

		lpMainData = (LP_MAIN_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MAIN_DATA));
		if(NULL == lpMainData) {	
			EndDialog(hwnd, 0);
			return FALSE;
		}
		mainDataSet(hwnd, lpMainData);

		lpMainData->hwnd = hwnd;
		lpMainData->hInst = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
		//////////////////////////////////////////////////////////////////////////////
		wsprintf( lpMainData->mbMaster.szIP, "192.168.0.11" );
		wsprintf( lpMainData->mbMaster.szTcpPort, "502" );
		//////////////////////////////////////////////////////////////////////////////
		lpMainData->mbMaster.hCom = INVALID_HANDLE_VALUE;
		//////////////////////////////////////////////////////////////////////////////
		lpMainData->hComThread = INVALID_HANDLE_VALUE;
		////////////////////////////////////////////////////////////////////////////////////////
		hwndCombo = GetDlgItem(hwnd, IDC_COMBO_SLAVE_ID);
		for(i = 1; i < 248; i++) {
			sprintf(szBuffer, "%d", i);
			ComboBox_AddString(hwndCombo, szBuffer);
		}
		lpMainData->SlaveID = 1;
		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_SLAVE_ID), lpMainData->SlaveID - 1);

		memset(inPort, 0, sizeof(inPort));
		memset(outPort, 0, sizeof(outPort));

		Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO3), TRUE);
		LoadMenu((HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE), MAKEINTRESOURCE(IDR_MENU_MAIN));

		for(i = 1; i < 9; i++) {
			char buffer[33];
			sprintf(buffer, "COM%d", i);
			ComboBox_AddString(GetDlgItem(hwnd, IDC_COMBO_PORT), buffer);
		}

		ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_COMBO_PORT), 0);

		for(i = 0; i < 2; i++) {
			HWND hwndTrack = GetDlgItem(hwnd, i + IDC_SLIDER_AO0);

			SendMessage(hwndTrack, TBM_SETPOS, TRUE, 0);
			SendMessage(hwndTrack, TBM_SETTICFREQ, 10, 0);
		}

		Edit_SetText(GetDlgItem(hwnd, IDC_EDIT_DAC0_VALUE), "Set DAC0 Value");
		Edit_SetText(GetDlgItem(hwnd, IDC_EDIT_DAC1_VALUE), "Set DAC1 Value");
		///////////////////////////////////////////////////////////////////////////////
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
		///////////////////////////////////////////////////////////////////////////////
	}
	 return TRUE;
	
	case WM_COMMAND:
		switch(wmEvent) {
		
		case CBN_SELCHANGE: {
			HWND hwndCombo = (HWND)lParam;

			switch(wmId) {
			case IDC_COMBO_SLAVE_ID:
				lpMainData->SlaveID = ComboBox_GetCurSel(hwndCombo) + 1;
			 break;
			}
		}
		 return TRUE;

		case BN_CLICKED:
			if(wmId >= IDC_Q0 && wmId <= IDC_Q11) {

				if( !lpMainData->flagWriteSigleCoil ) {
					lpMainData->idWriteSigleCoil = wmId - IDC_Q0;
					lpMainData->flagWriteSigleCoil = 1;
				}

				return TRUE;
			}

			switch(wmId) {

			case IDM_CONNECTION_SETINGS:
				if( INVALID_HANDLE_VALUE != lpMainData->mbMaster.hCom ) {
					COMMCONFIG CC;
					char szBuffer[11];

					sprintf(szBuffer, "COM%d", lpMainData->mbMaster.uiComIndex);

					CC.dcb = lpMainData->mbMaster.dcb;
					CommConfigDialog(szBuffer, NULL, &CC);
					lpMainData->mbMaster.dcb.Parity = CC.dcb.Parity;
					lpMainData->mbMaster.dcb.BaudRate = CC.dcb.BaudRate;
					lpMainData->mbMaster.dcb.StopBits = CC.dcb.StopBits;
					SetCommState(lpMainData->mbMaster.hCom, &lpMainData->mbMaster.dcb);
				}
			 return TRUE;

			case IDM_CONNECTION_COMSELECT_COM1: case IDM_CONNECTION_COMSELECT_COM2:
			case IDM_CONNECTION_COMSELECT_COM3: case IDM_CONNECTION_COMSELECT_COM4:
			case IDM_CONNECTION_COMSELECT_COM5: case IDM_CONNECTION_COMSELECT_COM6:
			case IDM_CONNECTION_COMSELECT_COM7: case IDM_CONNECTION_COMSELECT_COM8: {
				HMENU hMenu = GetMenu(hwnd);
				unsigned int com = 1 + (wmId - IDM_CONNECTION_COMSELECT_COM1);

				SendMessage(hwnd, WM_COMMAND, BN_CLICKED<<16 | IDM_CONNECTION_DISCONNECT, 0);

				if( mbMasterInit(&lpMainData->mbMaster, 1) ) {
					return FALSE;
				}

				if( !mbRTUMasterConnect( &lpMainData->mbMaster, com ) ) {
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
				}
			}
			 return TRUE;

			case IDM_CONNECTION_TCP_IP: {
				HMENU hMenu = GetMenu(hwnd);

				SendMessage(hwnd, WM_COMMAND, BN_CLICKED<<16 | IDM_CONNECTION_DISCONNECT, 0);

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

				if( mbTCPMasterConnect(&lpMainData->mbMaster, lpMainData->mbMaster.szIP, lpMainData->mbMaster.szTcpPort) ) {
					return FALSE;
				}
				////////////////////////////////////////////////////////////////////////////////////////////
				EnableMenuItem(hMenu, IDM_CONNECTION_DISCONNECT, MF_ENABLED);
				CheckMenuRadioItem( hMenu, IDM_CONNECTION_TCP_IP,
									IDM_CONNECTION_TCP_IP,
									wmId, MF_BYCOMMAND | MF_CHECKED
				);
				////////////////////////////////////////////////////////////////////////////////////////////
				DWORD dwThreadId;
				lpMainData->hComThread = CreateThread(NULL, 4096, comThreadFunc, lpMainData, 0, &dwThreadId);
				SetThreadPriority(lpMainData->hComThread,	THREAD_PRIORITY_TIME_CRITICAL);
				////////////////////////////////////////////////////////////////////////////////////////////
			}
			 return TRUE;

			case IDM_CONNECTION_DISCONNECT: {
				HMENU hMenu = GetMenu(hwnd);

				EnableMenuItem(hMenu, IDM_CONNECTION_SETINGS, MF_GRAYED);
				EnableMenuItem(hMenu, IDM_CONNECTION_DISCONNECT, MF_GRAYED);

				CheckMenuRadioItem( hMenu, IDM_CONNECTION_COMSELECT_COM1,
									IDM_CONNECTION_TCP_IP,
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
			case ID_HELP_ABOUT: DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, (DLGPROC)AboutWndProc); break;
				case IDM_SERVO_CONTROL: DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_SERVO_CONTROL), hwnd, (DLGPROC)ServoControlWndProc); return TRUE;
			// --------------------------------------------------------------------------------------------------------------------
			case ID_ADC_CONST: DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_ADC_CONST), hwnd, (DLGPROC)adcConstWndProc); break;
			case ID_DAC_CONST: DialogBox(lpMainData->hInst, MAKEINTRESOURCE(IDD_DAC_CONST), hwnd, (DLGPROC)dacConstWndProc); break;

			case ID_DAC_DAC0_DIRECTACCESS:
			case ID_DAC_DAC0_000VTO1000V:
				if( !lpMainData->arrDacNewAccess[0] ) {
					lpMainData->arrDacNewAccess[0] = 1 + (wmId - ID_DAC_DAC0_DIRECTACCESS);
				}
			 return TRUE;

			case ID_DAC_DAC1_DIRECTACCESS:
			case ID_DAC_DAC1_000VTO1000V:
				if( !lpMainData->arrDacNewAccess[1] ) {
					lpMainData->arrDacNewAccess[1] = 1 + (wmId - ID_DAC_DAC1_DIRECTACCESS);
				}
			 return TRUE;

			case IDCANCEL:
				if(NULL != lpMainData) {

					if( INVALID_HANDLE_VALUE != lpMainData->hComThread ) {
						TerminateThread(lpMainData->hComThread, 0);
						CloseHandle(lpMainData->hComThread);
						lpMainData->hComThread = INVALID_HANDLE_VALUE;
					}

					mbMasterDisconnect( &lpMainData->mbMaster );
					
					HeapFree(GetProcessHeap (), GWL_USERDATA, lpMainData);
				}

				EndDialog(hwnd, 0);
			 return TRUE;
			}
		 break;
		}
	 break;

	case WM_HSCROLL: {
		int i;

		UINT wNotifyCode	= HIWORD(wParam);	// notification code 
		UINT wmId			= LOWORD(wParam);	// item, control, or accelerator identifier 
		HWND hwndCtl		= (HWND)lParam;		// handle of control 

		int nScrollCode = (int)LOWORD(wParam);			// scroll bar value 
		unsigned int nPos = (unsigned int)HIWORD(wParam);		// scroll box position 

		switch(nScrollCode) {
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			for(i = 0; i < 2; i++) {
				HWND hwndTrack = GetDlgItem(hwnd, i + IDC_SLIDER_AO0);

				if(hwndTrack == hwndCtl) {
					char buffer[100];
					
					arrDAC[i] = nPos;
					switch(lpMainData->DAC_Access[i]) {
					case 1: sprintf(buffer, "DAC[%d] = %u", i, 2 * nPos); break;
					case 2: sprintf(buffer, "DAC[%d] = %.2f V", i, nPos / 100.0); break;
					}

					Edit_SetText(GetDlgItem(hwnd, i + IDC_EDIT_DAC0_VALUE), buffer);
					break;
				}
			}
		 break;
		}
	}
	 break;

	case 1045: {

		switch( lParam ) {
		case FD_CONNECT:
			//MessageBeep( MB_OK );
			//SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)"Connection Established." );
		 break;

		case FD_CLOSE:
			//if( s ) {
			//	closesocket( s );
			//}

			//WSACleanup( );

			//SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)"Connection to Remote Host Lost." );
		 break;

		case FD_READ:
			char buffer[80];

			memset( buffer, 0, sizeof( buffer ) );
			//recv( s, buffer, sizeof( buffer ) - 1, 0 );
			//GetTextandAddLine( buffer, hwnd, ID_EDIT_DATA );
		 break;

		case FD_ACCEPT: {
			//SOCKET TempSock = accept( s, (struct sockaddr*)&from, &fromlen );
			//char szAcceptAddr[100];
			
			//s = TempSock;

			//MessageBeep( MB_OK );

			//wsprintf( szAcceptAddr, "Connection from [%s] accepted.", inet_ntoa(from.sin_addr) );

			//SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)szAcceptAddr );
		}
		 break;
		}

	}
	 return TRUE;

	}

	return FALSE;
}

DWORD WINAPI comThreadFunc(LPVOID lpParam)
{
	int t1 = 0, t2 = 0;
	char t[100];
	int iResult;
	char szBuffer[100];
	unsigned int i = 0, counter = 0;
	LP_MAIN_DATA lpMainData = (LP_MAIN_DATA)lpParam;

	lpMainData->DAC_Access[0] = MODBusDacGetAccess(&lpMainData->mbMaster, lpMainData->SlaveID, 0);
	lpMainData->DAC_Access[1] = MODBusDacGetAccess(&lpMainData->mbMaster, lpMainData->SlaveID, 1);

	while( 1 ) {
		Sleep(1);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ++t1 > 4 ) {
			t1 = 0;
			iResult = mdReadDiscreteInputs(&lpMainData->mbMaster, lpMainData->SlaveID, (char*)inPort, 0, 15);
			if( !iResult ) {
				for(i = 0; i < 10; i++) {
					sprintf(szBuffer, "I%d: %s", i, (1 & inPort[i]) ? "ON" : "OFF");
					Static_SetText(GetDlgItem(lpMainData->hwnd, i + IDC_I0), szBuffer);
				}

				sprintf(szBuffer, "FB: %s", (1 & inPort[i]) ? "1" : "0");
				Static_SetText(GetDlgItem(lpMainData->hwnd, IDC_FB), szBuffer);

				for(i = 11; i < 14; i++) {
					sprintf(szBuffer, "FB%d: %s", i - 11, (1 & inPort[i]) ? "1" : "0");
					Static_SetText(GetDlgItem(lpMainData->hwnd, (i - 11) + IDC_FB0), szBuffer);
				}
			} else {
//				break;
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if( ++t2 > 5 || lpMainData->flagWriteSigleCoil) {
			t2 = 0;
			iResult = mbReadCoils(&lpMainData->mbMaster, lpMainData->SlaveID, (char*)outPort, 0, 12);
			if( !iResult ) {
				for( i = 0; i < 12; i++ ) {
					sprintf(szBuffer, "Q%d (%d)", i, outPort[i]);
					Button_SetText( GetDlgItem( lpMainData->hwnd, i + IDC_Q0), szBuffer );
				}

				if( lpMainData->flagWriteSigleCoil ) {
					/*mbWriteSingleCoil(
						&lpMainData->mbMaster, lpMainData->SlaveID,
						lpMainData->idWriteSigleCoil,
						outPort[lpMainData->idWriteSigleCoil] ^= 1
					);*/

					unsigned char temp[12];

					t2 = 5;

					lpMainData->flagWriteSigleCoil = 0;

					memcpy( temp, outPort, sizeof(temp) );
					temp[ lpMainData->idWriteSigleCoil ] ^= 1;

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
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ADC:
		switch( lpMainData->flagAdcConst ) {

		case IDC_BUTTON_ADC0_SET_CONST:
			lpMainData->flagAdcConst = 0;

			Edit_GetText( GetDlgItem(lpMainData->hwndAdcDialog, IDC_EDIT_ADC0_CONST + lpMainData->flagAdcConstId), szBuffer, 10 );

			iResult = MODBusSetAdcConst( &lpMainData->mbMaster, lpMainData->SlaveID, lpMainData->flagAdcConstId, atoi(szBuffer) );
			if( !iResult ) {
			} else {
//				break;
			}
		 break;

		case IDC_BUTTON_ADC_READ_CONST:
			lpMainData->flagAdcConst = 0;

			iResult = MODBusReadADCConst( &lpMainData->mbMaster, lpMainData->SlaveID, (unsigned int*)&lpMainData->arrAdcConst );
			if( !iResult ) {
				for( i = 0; i < 7; i++ ) {
					sprintf(szBuffer, "%d", lpMainData->arrAdcConst[i]);
					Edit_SetText( GetDlgItem(lpMainData->hwndAdcDialog, IDC_EDIT_ADC0_CONST + i), szBuffer );
				}
			} else {
//				break;
			}
		 break;

		case IDC_BUTTON_ADC_LOAD_DEF_CONST:
			lpMainData->flagAdcConst = 0;

			iResult = MODBusAdcLoadDefConst( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult ) {
				SendMessage( lpMainData->hwndAdcDialog, WM_COMMAND, IDC_BUTTON_ADC_READ_CONST, 0 );
			} else {
//				break;
			}
		 break;

		case IDC_BUTTON_ADC_LOAD_FROM_EEPROM:
			lpMainData->flagAdcConst = 0;

			iResult = MODBusAdcLoadConstFromEEPROM( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult ) {
				SendMessage( lpMainData->hwndAdcDialog, WM_COMMAND, IDC_BUTTON_ADC_READ_CONST, 0 );
			} else {
//				break;
			}
		 break;

		case ID_ADC_CONST_SAVE_TO_EEPROM:
			lpMainData->flagAdcConst = 0;

			iResult = MODBusAdcSaveConst2EEPROM( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult ) {
			} else {
//				break;
			}
		 break;
		}

		if( TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO1)) ||
			TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO3)) ||
			TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO4))
		) {
			iResult = MODBusReadRawADC( &lpMainData->mbMaster, lpMainData->SlaveID, arrADC );
			if( !iResult ) {
				if(TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO1))) {
					for(i = 0; i < 7; i++) {
						sprintf(t, " ADC[%d] = %d", i, (0xfff & arrADC[i]));
						Static_SetText(GetDlgItem(lpMainData->hwnd, i + IDC_ADC0), t);
					}
				} else {
					unsigned int adcConst[7];

					iResult = MODBusReadADCConst( &lpMainData->mbMaster, lpMainData->SlaveID, adcConst );
					if( !iResult ) {
						for( i = 0; i < 7; i++ ) {
							float f = (float)100.00 / (float)adcConst[i];

							f *= (0xffff & arrADC[i]);

							if( TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO3)) ) {
								sprintf( (char*)t, "ADC[%d] = %.2f V", i, f );
							} else
							if( TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO4)) ) {
								sprintf( (char*)t, "ADC[%d] = %.3fV", i, f );								
							}
							Static_SetText(GetDlgItem(lpMainData->hwnd, i + IDC_ADC0), t);
						}
					} else {
						break;
					}
				}

			} else {
//				break;
			}
		} else {
			if( TRUE == Button_GetCheck(GetDlgItem(lpMainData->hwnd, IDC_RADIO2)) ) {
				iResult = MODBusReadADCValue1000( &lpMainData->mbMaster, lpMainData->SlaveID, arrADC );
				if( !iResult ) {
					for(i = 0; i < 7; i++) {
						sprintf( szBuffer, "ADC[%d] = %d", i, (0xfff & arrADC[i]) );
						Static_SetText( GetDlgItem(lpMainData->hwnd, i + IDC_ADC0), szBuffer );
					}
				} else {
//					break;
				}
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// DAC1 and DAC0:
		for( i = 0; i < 2; i++ ) {
			if( lpMainData->arrDacNewAccess[i] ) {
				iResult = MODBusDacSetAccess( &lpMainData->mbMaster, lpMainData->SlaveID, i, lpMainData->arrDacNewAccess[i] );
				if( !iResult) {
				} else {
//					break;
				}
				lpMainData->arrDacNewAccess[i] = 0;
			}

			lpMainData->DAC_Access[i] = MODBusDacGetAccess( &lpMainData->mbMaster, lpMainData->SlaveID, i );
			if( !iResult) {
			} else {
//				break;
			}

			switch( lpMainData->DAC_Access[i] ) {
			case 1: {
				iResult = MODBusWriteRawDAC( &lpMainData->mbMaster, lpMainData->SlaveID, i, 2 * arrDAC[i] );
				if( !iResult) {
				} else {
//					break;
				}

				CheckMenuRadioItem(
					GetMenu(lpMainData->hwnd),
					2 * i + ID_DAC_DAC0_DIRECTACCESS,
					2 * i + ID_DAC_DAC0_000VTO1000V,
					2 * i + ID_DAC_DAC0_DIRECTACCESS,
					MF_BYCOMMAND | MF_CHECKED
				);

				SendMessage(
					GetDlgItem( lpMainData->hwnd, IDC_SLIDER_AO0 + i ),
					TBM_SETRANGE,
					(WPARAM)TRUE,				// redraw flag
					(LPARAM)MAKELONG(0, 32767)	// min. & max. positions
				);
			}
			 break;

			case 2: {
				iResult = MODBusWriteDAC_1000( &lpMainData->mbMaster, lpMainData->SlaveID, i, arrDAC[i] );
				if( !iResult) {
				} else {
//					break;
				}

				CheckMenuRadioItem(
					GetMenu(lpMainData->hwnd),
					2 * i + ID_DAC_DAC0_DIRECTACCESS,
					2 * i + ID_DAC_DAC0_000VTO1000V,
					2 * i + ID_DAC_DAC0_000VTO1000V,
					MF_BYCOMMAND | MF_CHECKED
				);

				SendMessage(
					GetDlgItem( lpMainData->hwnd, IDC_SLIDER_AO0 + i ),
					TBM_SETRANGE,
					(WPARAM)TRUE,				// redraw flag
					(LPARAM)MAKELONG(0, 1000)	// min. & max. positions
				);
			}
//			 break;
			}
		}

		switch( lpMainData->flagDacConst ) {
		case ID_DAC0_SET_CONST:
			lpMainData->flagDacConst = 0;
			
			Edit_GetText( GetDlgItem(lpMainData->hwndDacDialog, IDC_EDIT_DAC0), szBuffer, 10 );
			
			iResult = MODBusSetDacConst( &lpMainData->mbMaster, lpMainData->SlaveID, 0, atoi(szBuffer) );
			if( !iResult) {
			} else {
//				break;
			}
		 break;

		case ID_DAC1_SET_CONST:
			lpMainData->flagDacConst = 0;
			
			Edit_GetText( GetDlgItem(lpMainData->hwndDacDialog, IDC_EDIT_DAC1), szBuffer, 10 );
			
			iResult = MODBusSetDacConst( &lpMainData->mbMaster, lpMainData->SlaveID, 1, atoi(szBuffer) );
			if( !iResult) {
			} else {
//				break;
			}
		 break;

		case IDC_BUTTON_DAC_READ_CONST: {
			unsigned int value;

			lpMainData->flagDacConst = 0;

			iResult = MODBusDacReadConst( &lpMainData->mbMaster, lpMainData->SlaveID, 0, &value );
			if( !iResult) {
				sprintf(szBuffer, "%d", value);
				Edit_SetText( GetDlgItem(lpMainData->hwndDacDialog, IDC_EDIT_DAC0), szBuffer );
			} else {
//				break;
			}

			iResult = MODBusDacReadConst( &lpMainData->mbMaster, lpMainData->SlaveID, 1, &value );
			if( !iResult) {
				sprintf(szBuffer, "%d", value);
				Edit_SetText( GetDlgItem(lpMainData->hwndDacDialog, IDC_EDIT_DAC1), szBuffer );
			} else {
//				break;
			}
		}
		 break;

		case IDC_BUTTON_DAC_LOAD_DEF_CONST:
			lpMainData->flagDacConst = 0;

			iResult = MODBusDacLoadDefConst( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult) {
				SendMessage( lpMainData->hwndDacDialog, WM_COMMAND, IDC_BUTTON_DAC_READ_CONST, 0 );
			} else {
//				break;
			}
		 break;

		case IDC_BUTTON_DAC_LOAD_FROM_EEPROM:
			lpMainData->flagDacConst = 0;
			
			iResult = MODBusDacLoadConstFromEEPROM( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult) {
				SendMessage( lpMainData->hwndDacDialog, WM_COMMAND, IDC_BUTTON_DAC_READ_CONST, 0 );
			} else {
//				break;
			}
		 break;

		case ID_DAC_CONST_SAVE_TO_EEPROM:
			lpMainData->flagDacConst = 0;

			iResult = MODBusDacSaveConst2EEPROM( &lpMainData->mbMaster, lpMainData->SlaveID );
			if( !iResult) {
			} else {
//				break;
			}
		 break;
		}

		//continue;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		unsigned int arr_new[100];

		if( lpMainData->hwndServoControlDialog ) {
			unsigned int encoder[2];
			if( !mbReadInputRegister( &lpMainData->mbMaster, lpMainData->SlaveID, 39, 2, encoder) ) {
				int temp;

				temp = encoder[1]<<16 | ( 0xffff & encoder[0] );

				sprintf( szBuffer, "%d", temp );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_ENCODER), szBuffer );
			}
		}

		switch( lpMainData->flagPidWrite ) {

		case IDC_BUTTON_WRITE_POSITION_PID_P_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				49,
				mbSItoU16( lpMainData->pidData.wP_Factor )
			);
			lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_I_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				50,
				mbSItoU16( lpMainData->pidData.wI_Factor )
			);
			lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_D_FACTOR: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				51,
				mbSItoU16( lpMainData->pidData.wD_Factor )
			);
			lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_MAX_I_TERM: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				52,
				mbSItoU16( lpMainData->pidData.dwMaxSumError )
			);
			lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_SCALE: {
			mbWriteSingleRegister( &lpMainData->mbMaster,
				lpMainData->SlaveID,
				53,
				mbSItoU16( lpMainData->scale )
			);
			lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
		}
		 break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTON_READ_POSITION_PID_SETINGS: {
			iResult = mbReadHoldingRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 49, 5, arr_new );
			if( !iResult ) {
				for( i = 0; i < 5; i++ ) {
					arr_new[i] = mbU16toSI( arr_new[i] );
				}

				lpMainData->pidData.dwMaxSumError = arr_new[3];
				lpMainData->scale = arr_new[4];

				/*lpMainData->kp = (double)arr_new[0] / (double)lpMainData->scale;
				lpMainData->ki = (double)arr_new[1] / (double)lpMainData->scale;
				lpMainData->kd = (double)arr_new[2] / (double)lpMainData->scale;

				sprintf( szBuffer, "%2.2f", lpMainData->kp );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KP), szBuffer );

				sprintf( szBuffer, "%2.2f", lpMainData->ki );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KI), szBuffer );

				sprintf( szBuffer, "%2.2f", lpMainData->kd );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KD), szBuffer );*/

				sprintf( szBuffer, "%d", arr_new[0] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KP), szBuffer );

				sprintf( szBuffer, "%d", arr_new[1] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KI), szBuffer );

				sprintf( szBuffer, "%d", arr_new[2] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_KD), szBuffer );

				sprintf( szBuffer, "%d", lpMainData->pidData.dwMaxSumError );
				Static_SetText( GetDlgItem( lpMainData->hwndServoControlDialog, IDC_STATIC_MAX_I_TERM ), szBuffer );

				sprintf( szBuffer, "%d", lpMainData->scale );
				Static_SetText( GetDlgItem( lpMainData->hwndServoControlDialog, IDC_STATIC_POSITION_PID_SCALE ), szBuffer );
			}

			lpMainData->flagPidWrite = 0;
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_NEW_POSITION: {
			unsigned int regValue[2];
			regValue[0] = lpMainData->SP_Position;
			regValue[1] = lpMainData->SP_Position>>16;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 55, 2, regValue);
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;

		case IDC_EDIT_SET_VELOCITY: {
			unsigned int regValue[2];
			regValue[0] = lpMainData->SET_VELOCITY;
			regValue[1] = lpMainData->SET_VELOCITY>>16;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 57, 2, regValue);
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;

		case IDC_BUTTON_WRITE_ACCEL: {
			unsigned int regValue[2];
			regValue[0] = lpMainData->SET_ACCEL;
			regValue[1] = lpMainData->SET_ACCEL>>16;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 59, 2, regValue );
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;

		case IDC_BUTTON_WRITE_MAX_ACC2_AND_VELOCITY2: {
			unsigned int regValue[2];
			regValue[0] = lpMainData->SET_ACCEL2;
			regValue[1] = lpMainData->SET_VELOCITY2;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 61, 2, regValue );
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;

		case IDC_BUTTON_WRITE_MAX_SPEED: {
			unsigned int regValue[2];
			regValue[0] = lpMainData->SET_MAX_SPEED;
			regValue[1] = lpMainData->SET_MAX_SPEED>>16;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 63, 2, regValue );
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTON_WRITE_MAX_PID_OUT: {
			unsigned int regValue[1];
			regValue[0] = lpMainData->MAX_PID_OUT;
			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 66, 1, regValue );
			lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
		}
		 break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTON_SP_POSITION_READ_ALL: {
			unsigned int regValue[12];

			iResult = mbReadHoldingRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 55, 12, regValue );
			if( !iResult ) {
				sprintf( szBuffer, "%d", regValue[3]<<16 | regValue[2] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_VELOCITY), szBuffer );

				sprintf( szBuffer, "%d", regValue[5]<<16 | regValue[4] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_ACCELERATION), szBuffer );

				sprintf( szBuffer, "%d", regValue[6] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_ACCELERATION2), szBuffer );

				sprintf( szBuffer, "%d", regValue[7] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_VELOCITY2), szBuffer );

				sprintf( szBuffer, "%d", regValue[9]<<16 | regValue[8] );
				Static_SetText( GetDlgItem(lpMainData->hwndServoControlDialog, IDC_STATIC_MAX_SPEED), szBuffer );

				sprintf( szBuffer, "%d", regValue[11] );
				Static_SetText( GetDlgItem( lpMainData->hwndServoControlDialog, IDC_STATIC_MAX_PID_OUT), szBuffer );
			}

			lpMainData->flagPidWrite = 0;
		}
		 break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTON_SP_POSITION_WRITE_ALL: {
			unsigned int regValue[12];

			regValue[0] = lpMainData->SP_Position;			// regValue[55]
			regValue[1] = lpMainData->SP_Position>>16;		// regValue[56]

			regValue[2] = lpMainData->SET_VELOCITY;			// regValue[57]
			regValue[3] = lpMainData->SET_VELOCITY>>16;		// regValue[58]

			regValue[4] = lpMainData->SET_ACCEL;			// regValue[59]
			regValue[5] = lpMainData->SET_ACCEL>>16;		// regValue[60]

			regValue[6] = lpMainData->SET_ACCEL2;			// regValue[61]
			regValue[7] = lpMainData->SET_VELOCITY2;		// regValue[62]

			regValue[8] = lpMainData->SET_MAX_SPEED;		// regValue[63]
			regValue[9] = lpMainData->SET_MAX_SPEED>>16;	// regValue[64]

			regValue[10] = lpMainData->MAX_PID_OUT;			// regValue[65]

			regValue[11] = 0xAA;							// regValue[66] - temp

			mbWriteMultipleRegisters( &lpMainData->mbMaster, lpMainData->SlaveID, 55, 12, regValue);
			lpMainData->flagPidWrite = 0;
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		}
	}

	mbMasterDisconnect(&lpMainData->mbMaster);

	if( 1 == lpMainData->mbMaster.ascii_or_rtu_or_tcp ) {
		TerminateThread(lpMainData->hComThread, 0);
		CloseHandle(lpMainData->hComThread);
		lpMainData->hComThread = INVALID_HANDLE_VALUE;
	}

	return false;
}
///////////////////////////////////////////////////////////////////////////////////
int MODBusReadADCValue1000
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int *adcValue
)
{
	return mbReadInputRegister(lpMb, deviceID, 7, 7, adcValue);
}

int MODBusReadRawADC
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int *adcValue
)
{
	return mbReadInputRegister(lpMb, deviceID, 0, 7, adcValue);
}

int MODBusReadADCConst
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned int *adcConst
)
{
	return mbReadHoldingRegisters( lpMb, deviceID, 5, 7, adcConst );
}

int MODBusDacReadConst
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char dac, unsigned int *value
)
{
	return mbReadHoldingRegisters( lpMb, deviceID, 12 + dac, 1, value );
}

int MODBusDacSetAccess
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char dac,
	unsigned int access
)
{
	if( dac > 1 || access < 1 || access > 2 ) {
		return 1;
	}

	if(1 == dac) {
		access <<= 2;
	}

	access <<= 8;

	return mbWriteSingleRegister(lpMb , deviceID, 14, 0x1000 | access);
}

int MODBusDacGetAccess( LPMB lpMb, unsigned char deviceID, unsigned char dac )
{
	unsigned int access = 0, regValue;
	
	if( dac < 2 ) {
		if( !mbReadHoldingRegisters( lpMb, deviceID, 14, 1, &regValue) ) {
			regValue >>= 8;

			if(!dac) {
				access = 3 & regValue;
			} else {
				access = 3 & (regValue>>2);
			}
		}
	}

	return access;
}

int MODBusSetAdcConst
( 
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char adc, unsigned int new_const
)
{
	return mbWriteSingleRegister( lpMb, deviceID, 5 + adc, new_const );	
}

int MODBusAdcLoadDefConst( LPMB lpMb, unsigned char deviceID )
{
	return mbWriteSingleRegister( lpMb, deviceID, 14, 32 );	
}

int MODBusAdcLoadConstFromEEPROM( LPMB lpMb, unsigned char deviceID )
{	
	return mbWriteSingleRegister( lpMb, deviceID, 14, 64 );	
}

int MODBusAdcSaveConst2EEPROM( LPMB lpMb, unsigned char deviceID )
{	
	return mbWriteSingleRegister( lpMb, deviceID, 14, 128 );	
}

int MODBusWriteRawDAC
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char dac,
	unsigned int value
)
{
	if( dac > 1 ) {
		return 1;
	}

	return mbWriteSingleRegister( lpMb, deviceID, dac, value);
}

int MODBusWriteDAC_1000
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char dac,
	unsigned int value
)
{
	if( dac > 1 ) {
		return 1;
	}

	return mbWriteSingleRegister(lpMb, deviceID, 2 + dac, value);
}

int MODBusSetDacConst
(
	LPMB lpMb,
	unsigned char deviceID,
	unsigned char dac,
	unsigned int new_const
)
{
	return mbWriteSingleRegister(lpMb, deviceID, 12 + dac, new_const);
}

int MODBusDacLoadDefConst( LPMB lpMb, unsigned char deviceID )
{
	return mbWriteSingleRegister(lpMb, deviceID, 14, 32<<8);
}

int MODBusDacLoadConstFromEEPROM( LPMB lpMb, unsigned char deviceID )
{
	return mbWriteSingleRegister(lpMb, deviceID, 14, 64<<8);
}

int MODBusDacSaveConst2EEPROM( LPMB lpMb, unsigned char deviceID )
{
	return mbWriteSingleRegister(lpMb, deviceID, 14, 128<<8);
}
///////////////////////////////////////////////////////////////////////////////////

LP_MAIN_DATA mainDataGet(HWND hwnd)
{
	return (LP_MAIN_DATA)GetWindowLong(hwnd, GWL_USERDATA);
}

void mainDataSet(HWND hwnd, LP_MAIN_DATA lpMainData)
{
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpMainData);
}
