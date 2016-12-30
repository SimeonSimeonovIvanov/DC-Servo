#include "stdafx.h"

#include "../com.h"
#include "ServoControlDlg.h"

LRESULT CALLBACK ServoControlWndProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	LP_MAIN_DATA lpMainData = mainDataGet( GetParent(hDlg) );
	char szBuffer[100];

	switch(message){
	case WM_INITDIALOG:
		lpMainData->hwndServoControlDialog = hDlg;
		lpMainData->flagPidWrite = 0;
	 return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam) ) {
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_POSITION_PID_P_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_POSITION_PID_P_FACTOR ), szBuffer, 16 );
				
				/*lpMainData->kp = atof( szBuffer );
				lpMainData->pidData.wP_Factor =
					(int)( lpMainData->kp * (double)lpMainData->scale );*/

				lpMainData->pidData.wP_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_POSITION_PID_P_FACTOR;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_I_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_POSITION_PID_I_FACTOR ), szBuffer, 16 );

				/*lpMainData->ki = 0.001f * atof( szBuffer );
				lpMainData->pidData.wI_Factor =
					(int)( lpMainData->ki * (double)lpMainData->scale );*/

				lpMainData->pidData.wI_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_POSITION_PID_I_FACTOR;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_D_FACTOR: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_POSITION_PID_D_FACTOR ), szBuffer, 16 );

				/*lpMainData->kd = atof( szBuffer );
				lpMainData->pidData.wD_Factor =
					(int)( lpMainData->kd * (double)lpMainData->scale );*/

				lpMainData->pidData.wD_Factor = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_POSITION_PID_D_FACTOR;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_MAX_I_TERM: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_POSITION_PID_MAX_I_TERM ), szBuffer, 16 );

				lpMainData->pidData.dwMaxSumError = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_POSITION_PID_MAX_I_TERM;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_POSITION_PID_SCALE: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_POSITION_PID_SCALE ), szBuffer, 16 );
				
				lpMainData->scale = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_POSITION_PID_SCALE;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;
		// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
		case IDC_BUTTON_READ_POSITION_PID_SETINGS: {
			if( !lpMainData->flagPidWrite ) {
				lpMainData->flagPidWrite = IDC_BUTTON_READ_POSITION_PID_SETINGS;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case IDC_BUTTON_WRITE_NEW_POSITION: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_NEW_POSITION ), szBuffer, 16 );
				lpMainData->SP_Position = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_NEW_POSITION;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_VELOCITY: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_VELOCITY ), szBuffer, 16 );
				lpMainData->SET_VELOCITY = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_EDIT_SET_VELOCITY;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_ACCEL: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_ACCELERATION ), szBuffer, 16 );
				lpMainData->SET_ACCEL = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_ACCEL;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_MAX_ACC2_AND_VELOCITY2: {
			if( !lpMainData->flagPidWrite ) {
				
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_ACCELERATION2 ), szBuffer, 16 );
				lpMainData->SET_ACCEL2 = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_MAX_VELOCITY2 ), szBuffer, 16 );
				lpMainData->SET_VELOCITY2 = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_MAX_ACC2_AND_VELOCITY2;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_MAX_SPEED: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_MAX_SPEED ), szBuffer, 16 );
				lpMainData->SET_MAX_SPEED= atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_MAX_SPEED;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_WRITE_MAX_PID_OUT: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_MAX_PID_OUT ), szBuffer, 16 );
				lpMainData->MAX_PID_OUT = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_WRITE_MAX_PID_OUT;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_SP_POSITION_READ_ALL: {
			if( !lpMainData->flagPidWrite ) {
				lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_READ_ALL;
				lpMainData->hwndServoControlDialog = hDlg;
			}
		}
		 break;

		case IDC_BUTTON_SP_POSITION_WRITE_ALL: {
			if( !lpMainData->flagPidWrite ) {
				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_NEW_POSITION ), szBuffer, 16 );
				lpMainData->SP_Position = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_VELOCITY ), szBuffer, 16 );
				lpMainData->SET_VELOCITY = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_ACCEL ), szBuffer, 16 );
				lpMainData->SET_ACCEL = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_ACCELERATION2 ), szBuffer, 16 );
				lpMainData->SET_ACCEL2 = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_MAX_VELOCITY2 ), szBuffer, 16 );
				lpMainData->SET_VELOCITY2 = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_SET_MAX_SPEED ), szBuffer, 16 );
				lpMainData->SET_MAX_SPEED = atoi( szBuffer );

				Edit_GetText( GetDlgItem( hDlg, IDC_EDIT_MAX_PID_OUT ), szBuffer, 16 );
				lpMainData->MAX_PID_OUT = atoi( szBuffer );

				lpMainData->flagPidWrite = IDC_BUTTON_SP_POSITION_WRITE_ALL;
				lpMainData->hwndServoControlDialog = hDlg;
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
			lpMainData->hwndServoControlDialog = NULL;
			EndDialog(hDlg, 2);
		}
		 break;

		}

	 return TRUE;
	}
    return FALSE;
}