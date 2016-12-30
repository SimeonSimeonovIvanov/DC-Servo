// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include "pid.h"

pidData_t pidCurrentData;
int16_t referenceValue = 0, measurementValue = 0, inputValue;

int main(int argc, char* argv[])
{
	char flag = 1;

	referenceValue = 511;

	while( 1 ) {

		if( flag ) {
			int iKP = 1;
			int iKI = 0;
			int iKD = 0;
			int SCALING_FACTOR = 128;

			pid_Init(
				iKP,
				iKI,
				iKD,
				SCALING_FACTOR,
				(pidData_t*)&pidCurrentData
			);

			flag = 0;
		}

		//referenceValue = Get_Reference();
		//measurementValue = Get_Measurement();

		inputValue = pid_Controller( referenceValue, measurementValue, (pidData_t*)&pidCurrentData );

		Sleep( 100 );
	}

	return 0;
}