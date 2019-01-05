#include <windows.h>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <thread>


WCHAR RandChar() {
	return (WCHAR)((rand() % 320) + 191);
}



SHORT nScreenWidth = 150, nScreenHeight = 40;
INT nScreenResolution;
INT nChars;
INT nCharTrailLen;


BYTE bColor = 1;

BOOL CreateMatrixConsole(HANDLE & hConsole, PSMALL_RECT pBufferRect, PCONSOLE_SCREEN_BUFFER_INFOEX pConsoleInfoEx) {
	hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	if (hConsole == INVALID_HANDLE_VALUE) return FALSE;


	*pBufferRect = { 0, 0, nScreenWidth - 1, nScreenHeight - 1 };

	BOOL succeed = SetConsoleWindowInfo(hConsole, TRUE, pBufferRect);
	if (!GetConsoleScreenBufferInfoEx(hConsole, pConsoleInfoEx)) goto Error;

	pConsoleInfoEx->ColorTable[0] = RGB(0, 0, 0);
	if (bColor == 0) pConsoleInfoEx->ColorTable[1] = RGB(250, 100, 100);				// Red
	if (bColor == 1) pConsoleInfoEx->ColorTable[1] = RGB(100, 250, 100);				// Green
	if (bColor == 2) pConsoleInfoEx->ColorTable[1] = RGB(100, 100, 250);				// Blue
	for (int c = 25; c <= 250; c += 25) {
		if (bColor == 0) pConsoleInfoEx->ColorTable[12 - c / 25] = RGB(c, 0, 0);		// Red
		if (bColor == 1) pConsoleInfoEx->ColorTable[12 - c / 25] = RGB(0, c, 0);		// Green
		if (bColor == 2) pConsoleInfoEx->ColorTable[12 - c / 25] = RGB(0, 0, c);		// Blue
	}
	if (bColor == 0) pConsoleInfoEx->ColorTable[12] = RGB(255, 220, 220);				// Red
	if (bColor == 1) pConsoleInfoEx->ColorTable[12] = RGB(220, 255, 220);				// Green
	if (bColor == 2) pConsoleInfoEx->ColorTable[12] = RGB(220, 220, 255);				// Blue

	pConsoleInfoEx->dwSize = { nScreenWidth, nScreenHeight };
	pConsoleInfoEx->srWindow = { pConsoleInfoEx->srWindow.Left, pConsoleInfoEx->srWindow.Top, nScreenWidth, nScreenHeight };
	pConsoleInfoEx->dwMaximumWindowSize = { nScreenWidth, nScreenHeight };

	if (!SetConsoleScreenBufferInfoEx(hConsole, pConsoleInfoEx)) goto Error;


	if (!SetConsoleTitle(L"Matrix.exe")) goto Error;
	if (!SetConsoleActiveScreenBuffer(hConsole)) goto Error;


	return TRUE;

Error:
	CloseHandle(hConsole);
	return FALSE;
}


/*
 ** returns
 * (BYTE) -1	- char is still above screen buffer (y < 0)
 * (BYTE) 0		- char and its trail are below screen buffer (y < screen height + trail length)
 * (BYTE) 1		- char was drawn to screen buffer
 * (BYTE) 2		- char is below screen buffer (y < screen height) but trail is able to be drawn (y >= screen height + trail length)
 */
BYTE DrawChar(CHAR_INFO * pScreenBuffer, COORD rgCharCoord, INT nColorTableIndex, WCHAR chDisplayChar) {
	if (rgCharCoord.Y < 0) return -1;
	if (rgCharCoord.Y >= nScreenHeight + nCharTrailLen) {
		return 0;
	}

	if (rgCharCoord.X + rgCharCoord.Y * nScreenWidth < nScreenResolution) {
		pScreenBuffer[rgCharCoord.X + rgCharCoord.Y * nScreenWidth].Char.UnicodeChar = chDisplayChar;
		pScreenBuffer[rgCharCoord.X + rgCharCoord.Y * nScreenWidth].Attributes = nColorTableIndex;
		return 1;
	}

	return 2;
}


void Resize() {
	nScreenResolution = nScreenWidth * nScreenHeight;
	nChars = nScreenResolution / 43;
	nCharTrailLen = (nScreenResolution - 1500) / 300 + 5;
}

int main(int argc, char ** argv) {
	srand((unsigned)time(NULL));
	if (argc > 1) {
		if (argv[1][0] == 'r')
			bColor = 0;
		else if (argv[1][0] == 'b')
			bColor = 2;
		else
			bColor = 1;
	}
	Resize();


	HANDLE hConsole = NULL;
	SMALL_RECT rBufferRect;
	CONSOLE_SCREEN_BUFFER_INFOEX csbi; csbi.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

	if (!CreateMatrixConsole(hConsole, &rBufferRect, &csbi)) {
		std::printf("CreateMatrixConsole: 0x%X\n", GetLastError());
		return 1;
	}

	
	std::vector<CHAR_INFO> vScreenBuffer(nScreenResolution);
	std::vector<COORD> vCharCoords(nChars);

	INT i;
	for (i = 0; i < nChars; ++i)
		vCharCoords[i] = { (SHORT)(rand() % nScreenWidth), -(SHORT)(rand() % nChars) };

	BYTE bDrawChar;
	BOOL bOnce = TRUE;
	while (1) {
		//Check if screen size has changed
		if (GetConsoleScreenBufferInfoEx(hConsole, &csbi)) {
			if (csbi.dwSize.X != nScreenWidth || csbi.dwSize.Y != nScreenHeight || bOnce) {
				bOnce = FALSE;
				nScreenWidth = csbi.dwSize.X; nScreenHeight = csbi.dwSize.Y;
				Resize();

				vScreenBuffer.resize(nScreenResolution);
				vCharCoords.resize(nChars);

				rBufferRect = { 0, 0, nScreenWidth - 1, nScreenHeight - 1 };

				SetConsoleWindowInfo(hConsole, TRUE, &rBufferRect);
			}
		}

		//Clear screen
		for (i = 0; i < nScreenResolution; ++i) {
			vScreenBuffer[i].Char.UnicodeChar = ' ';
			vScreenBuffer[i].Attributes = 0;
		}

		//Draw chars
		for (i = 0; i < nChars; ++i) {
			++vCharCoords[i].Y;
			if (!(bDrawChar = DrawChar(&vScreenBuffer[0], vCharCoords[i], 12, RandChar()))) {
				vCharCoords[i] = { (SHORT)(rand() % nScreenWidth), -(SHORT)(rand() % nChars / 2) };
			}
			else if (bDrawChar < 0) continue;

			for (SHORT n = 1; n < nCharTrailLen; ++n) {
				DrawChar(&vScreenBuffer[0], {vCharCoords[i].X, vCharCoords[i].Y - n},
					(INT)(10 * n / (nCharTrailLen - 1) + (1 - 10 / (nCharTrailLen - 1))),
					RandChar());
			}
		}

		//Draw to console
		if (!WriteConsoleOutput(hConsole, &vScreenBuffer[0], { nScreenWidth, nScreenHeight }, { 0, 0 }, &rBufferRect)) {
			std::printf("WriteConsoleOutput: 0x%X\n", GetLastError());
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(75));
	}

	CloseHandle(hConsole);

	return 0;
}
