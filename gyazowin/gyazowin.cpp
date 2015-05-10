﻿// gyazowin.cpp : Application entry point

#include "stdafx.h"
#include "gyazowin.h"

// Global variables
HINSTANCE hInst;							// Current window
TCHAR *szTitle			= _T("Gyazo");		// Text in the window title
TCHAR *szWindowClass	= _T("GYAZOWIN");	// Main window class name
TCHAR *szWindowClassL	= _T("GYAZOWINL");	// Layer window class name
HWND hLayerWnd;

int ofX, ofY;	// Screen offset

// Prototype declaration
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	LayerWndProc(HWND, UINT, WPARAM, LPARAM);

int					GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

BOOL				isPng(LPCTSTR fileName);
VOID				drawRubberband(HDC hdc, LPRECT newRect, BOOL erase);
VOID				execUrl(const char* str);
VOID				setClipBoardText(const char* str);
BOOL				convertPNG(LPCTSTR destFile, LPCTSTR srcFile);
BOOL				savePNG(LPCTSTR fileName, HBITMAP newBMP);
BOOL				uploadFile(HWND hwnd, LPCTSTR fileName, BOOL isPng);
std::string			getId();
BOOL				saveId(const WCHAR* str);
void				LastErrorMessageBox(HWND hwnd, LPTSTR lpszError);


// Main
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;

	TCHAR	szThisPath[MAX_PATH];
	DWORD   sLen;

	// Find the path of this program
	sLen = GetModuleFileName(NULL, szThisPath, MAX_PATH);
	for(unsigned int i = sLen; i >= 0; i--) {
		if(szThisPath[i] == _T('\\')) {
			szThisPath[i] = _T('\0');
			break;
		}
	}

	// Set path of where the program is currently running
	SetCurrentDirectory(szThisPath);

	// File specified when the application was launched
	if ( 2 == __argc )
	{
		// If image is PNG, upload it
		//if (isPng(__targv[1])) {
			// PNG Upload the PNG file
		//	uploadFile(NULL, __targv[1]);
		//}else {
			// Attempt to convert the PNG file
			//TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
			//GetTempPath(MAX_PATH, tmpDir);
			//GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);
			
			//if (convertPNG(tmpFile, __targv[1])) {
				// If the PNG convertion succeeded, upload it
			//	uploadFile(NULL, tmpFile);
			//} else {
				// PNG conversion failed, notify the user
			//	MessageBox(NULL, _T("Cannot convert this image"), szTitle, 
			//		MB_OK | MB_ICONERROR);
			//}
			//DeleteFile(tmpFile);
		//}

		uploadFile(NULL, __targv[1], false);
		return TRUE;
	}

	// Register the window class
	MyRegisterClass(hInstance);

	// Attempt to initialize the application
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
	
	// Main program loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

// Validates if the file on the specified path is a valid PNG file
BOOL isPng(LPCTSTR fileName)
{
	unsigned char pngHead[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	unsigned char readHead[8];
	
	FILE *fp = NULL;
	
	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		8 != fread(readHead, 1, 8, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);
	
	// compare
	for(unsigned int i=0;i<8;i++)
		if(pngHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// Registering the window instance
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	// メインウィンドウ
	wc.style         = 0;							// Do not set WM_PAINT
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// Set cursor properties
	wc.hbrBackground = 0;							// Make background 0
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	RegisterClass(&wc);

	// レイヤーウィンドウ
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = LayerWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// Set cursor properties
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClassL;

	return RegisterClass(&wc);
}


// Initialization of the window instance which covers the entire screen
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
//	HWND hLayerWnd;
	hInst = hInstance; // Save instance to the global variable.

	int x, y, w, h;

	// Get the size of the screen and save to global variables
	x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// x, y のオフセット値を覚えておく
	ofX = x; ofY = y;

	// 完全に透過したウィンドウを作る
	hWnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST
#if(_WIN32_WINNT >= 0x0500)
		| WS_EX_NOACTIVATE
#endif
		,
		szWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	// 作れなかった...?
	if (!hWnd) return FALSE;
	
	// 全画面を覆う
	MoveWindow(hWnd, x, y, w, h, FALSE);
	
	// nCmdShow を無視 (SW_MAXIMIZE とかされると困る)
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// ESCキー検知タイマー
	SetTimer(hWnd, 1, 100, NULL);


	// レイヤーウィンドウの作成
	hLayerWnd = CreateWindowEx(
	 WS_EX_TOOLWINDOW
#if(_WIN32_WINNT >= 0x0500)
		| WS_EX_LAYERED | WS_EX_NOACTIVATE
#endif
		,
		szWindowClassL, NULL, WS_POPUP,
		100, 100, 300, 300,
		hWnd, NULL, hInstance, NULL);

    SetLayeredWindowAttributes(hLayerWnd, RGB(255, 0, 0), 100, LWA_COLORKEY|LWA_ALPHA);

	


	
	return TRUE;
}

// Get the CLSID of the Encoder corresponding to the specified format
// Cited from MSDN Library: Retrieving the Class Identifier for an Encoder
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

// Drawing the rubberband
VOID drawRubberband(HDC hdc, LPRECT newRect, BOOL erase)
{
	
	static BOOL firstDraw = TRUE;	// 1 回目は前のバンドの消去を行わない
	static RECT lastRect  = {0};	// 最後に描画したバンド
	static RECT clipRect  = {0};	// 最後に描画したバンド
	
	if(firstDraw) {
		// Display Window layer
		ShowWindow(hLayerWnd, SW_SHOW);
		UpdateWindow(hLayerWnd);

		firstDraw = FALSE;
	}

	if (erase) {
		// Hide the window layer
		ShowWindow(hLayerWnd, SW_HIDE);
		
	}

	// Check the coordinates
	clipRect = *newRect;
	if ( clipRect.right  < clipRect.left ) {
		int tmp = clipRect.left;
		clipRect.left   = clipRect.right;
		clipRect.right  = tmp;
	}
	if ( clipRect.bottom < clipRect.top  ) {
		int tmp = clipRect.top;
		clipRect.top    = clipRect.bottom;
		clipRect.bottom = tmp;
	}
	MoveWindow(hLayerWnd,  clipRect.left, clipRect.top, 
			clipRect.right-  clipRect.left + 1, clipRect.bottom - clipRect.top + 1,true);

	
	return;

/* rakusai 2009/11/2

	// XOR で描画
	int hPreRop = SetROP2(hdc, R2_XORPEN);

	// 点線
	HPEN hPen = CreatePen(PS_DOT , 1, 0);
	SelectObject(hdc, hPen);
	SelectObject(hdc, GetStockObject(NULL_BRUSH));

	if(!firstDraw) {
		// 前のを消す
		Rectangle(hdc, lastRect.left, lastRect.top, 
			lastRect.right + 1, lastRect.bottom + 1);
	} else {
		firstDraw = FALSE;
	}
	
	// 新しい座標を記憶
	lastRect = *newRect;
	
	


	if (!erase) {

		// 枠を描画
		Rectangle(hdc, lastRect.left, lastRect.top, 
			lastRect.right + 1, lastRect.bottom + 1);

	}


	// 後処理
	SetROP2(hdc, hPreRop);
	DeleteObject(hPen);

*/

}

// Converts the source file to PNG on the specified location
BOOL convertPNG(LPCTSTR destFile, LPCTSTR srcFile)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ の初期化
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Image *b = new Image(srcFile, 0);

	if (0 == b->GetLastStatus()) {
		if (GetEncoderClsid(L"image/png", &clsidEncoder)) {
			// save!
			if (0 == b->Save(destFile, &clsidEncoder, 0) ) {
					// 保存できた
					res = TRUE;
			}
		}
	}

	// 後始末
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// Saves a PNG file to a specified location
BOOL savePNG(LPCTSTR fileName, HBITMAP newBMP)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ の初期化
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
	// HBITMAP から Bitmap を作成
	Bitmap *b = new Bitmap(newBMP, NULL);
	
	if (GetEncoderClsid(L"image/png", &clsidEncoder)) {
		// save!
		if (0 ==
			b->Save(fileName, &clsidEncoder, 0) ) {
				// 保存できた
				res = TRUE;
		}
	}
	
	// 後始末
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// Window layer procedure
LRESULT CALLBACK LayerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT clipRect	= {0, 0, 500, 500};
	HBRUSH hBrush;
	HPEN hPen;
	HFONT hFont;


	switch (message)
	{
	case WM_ERASEBKGND:
		 GetClientRect(hWnd, &clipRect);
		
		hdc = GetDC(hWnd);
        hBrush = CreateSolidBrush(RGB(100,100,100));
        SelectObject(hdc, hBrush);
		hPen = CreatePen(PS_DASH,1,RGB(255,255,255));
		SelectObject(hdc, hPen);
		Rectangle(hdc,0,0,clipRect.right,clipRect.bottom);

		//矩形のサイズを出力
		int fHeight;
		fHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		hFont = CreateFont(fHeight,    //フォント高さ
			0,                    //文字幅
			0,                    //テキストの角度
			0,                    //ベースラインとｘ軸との角度
			FW_REGULAR,            //フォントの重さ（太さ）
			FALSE,                //イタリック体
			FALSE,                //アンダーライン
			FALSE,                //打ち消し線
			ANSI_CHARSET,    //文字セット
			OUT_DEFAULT_PRECIS,    //出力精度
			CLIP_DEFAULT_PRECIS,//クリッピング精度
			PROOF_QUALITY,        //出力品質
			FIXED_PITCH | FF_MODERN,//ピッチとファミリー
			L"Tahoma");    //書体名

		SelectObject(hdc, hFont);
		// show size
		int iWidth, iHeight;
		iWidth  = clipRect.right  - clipRect.left;
		iHeight = clipRect.bottom - clipRect.top;

		wchar_t sWidth[200], sHeight[200];
		swprintf_s(sWidth, L"%d", iWidth);
		swprintf_s(sHeight, L"%d", iHeight);

		int w,h,h2;
		w = -fHeight * 2.5 + 8;
		h = -fHeight * 2 + 8;
		h2 = h + fHeight;

		SetBkMode(hdc,TRANSPARENT);
		SetTextColor(hdc,RGB(0,0,0));
		TextOut(hdc, clipRect.right-w+1,clipRect.bottom-h+1,(LPCWSTR)sWidth,wcslen(sWidth));
		TextOut(hdc, clipRect.right-w+1,clipRect.bottom-h2+1,(LPCWSTR)sHeight,wcslen(sHeight));
		SetTextColor(hdc,RGB(255,255,255));
		TextOut(hdc, clipRect.right-w,clipRect.bottom-h,(LPCWSTR)sWidth,wcslen(sWidth));
		TextOut(hdc, clipRect.right-w,clipRect.bottom-h2,(LPCWSTR)sHeight,wcslen(sHeight));

		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		ReleaseDC(hWnd, hdc);

		return TRUE;

        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;

}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	
	static BOOL onClip		= FALSE;
	static BOOL firstDraw	= TRUE;
	static RECT clipRect	= {0, 0, 0, 0};
	
	switch (message)
	{
	case WM_RBUTTONDOWN:
		// Right button down, abort screen capture
		DestroyWindow(hWnd);
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	case WM_TIMER:
		// User spent too much time on selecting the area, program aborts
		if (GetKeyState(VK_ESCAPE) & 0x8000){
			DestroyWindow(hWnd);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_MOUSEMOVE:
		if (onClip) {
			// Whenever mouse has been moved
			clipRect.right  = LOWORD(lParam) + ofX;
			clipRect.bottom = HIWORD(lParam) + ofY;
			
			hdc = GetDC(NULL);
			drawRubberband(hdc, &clipRect, FALSE);

			ReleaseDC(NULL, hdc);
		}
		break;
	

	case WM_LBUTTONDOWN:
		{
			// Left mouse button pressed, starting point of capture
			onClip = TRUE;
			
			// 初期位置をセット
			clipRect.left = LOWORD(lParam) + ofX;
			clipRect.top  = HIWORD(lParam) + ofY;
			


			// マウスをキャプチャ
			SetCapture(hWnd);
		}
		break;

	case WM_LBUTTONUP:
		{
			// Released the mouse key press, upload the image
			onClip = FALSE;
			
			// マウスのキャプチャを解除
			ReleaseCapture();
		
			// 新しい座標をセット
			clipRect.right  = LOWORD(lParam) + ofX;
			clipRect.bottom = HIWORD(lParam) + ofY;

			// 画面に直接描画，って形
			HDC hdc = GetDC(NULL);

			// 線を消す
			drawRubberband(hdc, &clipRect, TRUE);
			DestroyWindow(hLayerWnd);

			// 座標チェック
			if ( clipRect.right  < clipRect.left ) {
				int tmp = clipRect.left;
				clipRect.left   = clipRect.right;
				clipRect.right  = tmp;
			}
			if ( clipRect.bottom < clipRect.top  ) {
				int tmp = clipRect.top;
				clipRect.top    = clipRect.bottom;
				clipRect.bottom = tmp;
			}
			
			// 画像のキャプチャ
			int iWidth, iHeight;
			iWidth  = clipRect.right  - clipRect.left + 1;
			iHeight = clipRect.bottom - clipRect.top  + 1;

			if(iWidth == 0 || iHeight == 0) {
				// 画像になってない, なにもしない
				ReleaseDC(NULL, hdc);
				DestroyWindow(hWnd);
				break;
			}

			// ビットマップバッファを作成
			HBITMAP newBMP = CreateCompatibleBitmap(hdc, iWidth, iHeight);
			HDC	    newDC  = CreateCompatibleDC(hdc);
			
			// 関連づけ
			SelectObject(newDC, newBMP);

			// 画像を取得
			BitBlt(newDC, 0, 0, iWidth, iHeight, 
				hdc, clipRect.left, clipRect.top, SRCCOPY);
			
			// ウィンドウを隠す!
			ShowWindow(hWnd, SW_HIDE);
			/*
			// 画像をクリップボードにコピー
			if ( OpenClipboard(hWnd) ) {
				// 消去
				EmptyClipboard();
				// セット
				SetClipboardData(CF_BITMAP, newBMP);
				// 閉じる
				CloseClipboard();
			}
			*/
			
			// テンポラリファイル名を決定
			TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
			GetTempPath(MAX_PATH, tmpDir);
			GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);
			
			if (savePNG(tmpFile, newBMP)) {

				// うｐ
				if (!uploadFile(hWnd, tmpFile, true)) {
					// アップロードに失敗...
					// エラーメッセージは既に表示されている

					/*
					TCHAR sysDir[MAX_PATH];
					if (SUCCEEDED(StringCchCopy(sysDir, MAX_PATH, tmpFile)) &&
						SUCCEEDED(StringCchCat(sysDir, MAX_PATH, _T(".png")))) {
						
						MoveFile(tmpFile, sysDir);
						SHELLEXECUTEINFO lsw = {0};
						
						lsw.hwnd	= hWnd;
						lsw.cbSize	= sizeof(SHELLEXECUTEINFO);
						lsw.lpVerb	= _T("open");
						lsw.lpFile	= sysDir;

						ShellExecuteEx(&lsw);
					}
					*/
				}
			} else {
				// PNG保存失敗...
				MessageBox(hWnd, _T("Cannot save png image"), szTitle, 
					MB_OK | MB_ICONERROR);
			}

			// Cleanup and exit
			DeleteFile(tmpFile);
			
			DeleteDC(newDC);
			DeleteObject(newBMP);

			ReleaseDC(NULL, hdc);
			DestroyWindow(hWnd);
			PostQuitMessage(0);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Copy the content of 'str' to the clipboard
VOID setClipBoardText(const char* str)
{

	HGLOBAL hText;
	char    *pText;
	size_t  slen;

	slen  = strlen(str) + 1; // NULL

	hText = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, slen * sizeof(TCHAR));

	pText = (char *)GlobalLock(hText);
	strncpy_s(pText, slen, str, slen);
	GlobalUnlock(hText);
	
	// クリップボードを開く
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hText);
	CloseClipboard();

	// 解放
	GlobalFree(hText);
}

// Open the default browser on the client machine with the address set to 'str'
VOID execUrl(const char* str)
{
	size_t  slen;
	size_t  dcount;
	slen  = strlen(str) + 1; // NULL

	TCHAR *wcUrl = (TCHAR *)malloc(slen * sizeof(TCHAR));
	
	// ワイド文字に変換
	mbstowcs_s(&dcount, wcUrl, slen, str, slen);
	
	// open コマンドを実行
	SHELLEXECUTEINFO lsw = {0};
	lsw.cbSize = sizeof(SHELLEXECUTEINFO);
	lsw.lpVerb = _T("open");
	lsw.lpFile = wcUrl;

	ShellExecuteEx(&lsw);

	free(wcUrl);
}

// Save ID of the image
std::string getId()
{

    TCHAR idFile[_MAX_PATH];
	TCHAR idDir[_MAX_PATH];

    SHGetSpecialFolderPath( NULL, idFile, CSIDL_APPDATA, FALSE );

	 _tcscat_s( idFile, _T("\\Gyazo"));
	 _tcscpy_s( idDir, idFile);
	 _tcscat_s( idFile, _T("\\id.txt"));

	const TCHAR*	 idOldFile			= _T("id.txt");
	BOOL oldFileExist = FALSE;

	std::string idStr;

	// まずはファイルから ID をロード
	std::ifstream ifs;

	ifs.open(idFile);
	if (! ifs.fail()) {
		// ID を読み込む
		ifs >> idStr;
		ifs.close();
	} else{		
		std::ifstream ifsold;
		ifsold.open(idOldFile);
		if (! ifsold.fail()) {
			// 同一ディレクトリからID を読み込む(旧バージョンとの互換性)
			ifsold >> idStr;
			ifsold.close();
		}
	}

	return idStr;
}

// Save ID
BOOL saveId(const WCHAR* str)
{

    TCHAR idFile[_MAX_PATH];
	TCHAR idDir[_MAX_PATH];

    SHGetSpecialFolderPath( NULL, idFile, CSIDL_APPDATA, FALSE );

	 _tcscat_s( idFile, _T("\\Gyazo"));
	 _tcscpy_s( idDir, idFile);
	 _tcscat_s( idFile, _T("\\id.txt"));

	const TCHAR*	 idOldFile			= _T("id.txt");

	size_t  slen;
	size_t  dcount;
	slen  = _tcslen(str) + 1; // NULL

	char *idStr = (char *)malloc(slen * sizeof(char));
	// バイト文字に変換
	wcstombs_s(&dcount, idStr, slen, str, slen);

	// ID を保存する
	CreateDirectory(idDir,NULL);
	std::ofstream ofs;
	ofs.open(idFile);
	if (! ofs.fail()) {
		ofs << idStr;
		ofs.close();

		// 旧設定ファイルの削除
		if (PathFileExists(idOldFile)){
			DeleteFile(idOldFile);
		}
	}else{
		free(idStr);
		return FALSE;
	}

	free(idStr);
	return TRUE;
}


void LastErrorMessageBox(HWND hwnd, LPTSTR lpszError) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        GetModuleHandle(_T("wininet.dll")),
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszError) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s\n\nError %d: %s"), 
        lpszError, dw, lpMsgBuf); 
    MessageBox(hwnd, (LPCTSTR)lpDisplayBuf, szTitle, MB_OK | MB_ICONERROR); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

// Uploads the PNG file to Gyazo
BOOL uploadFile(HWND hwnd, LPCTSTR fileName, BOOL isPng)
{
	const int nSize = 256;
	LPCTSTR DEFAULT_UPLOAD_SERVER = _T("upload.gyazo.com");
	LPCTSTR DEFAULT_UPLOAD_PATH   = _T("/upload.cgi");
	//LPCTSTR DEFAULT_USER_AGENT    = _T("User-Agent: Gyazowin/1.0\r\n");
	const int DEFAULT_UPLOAD_SERVER_PORT = INTERNET_DEFAULT_HTTP_PORT;

	TCHAR upload_server[nSize];
	TCHAR upload_path[nSize];
	//TCHAR ua[nSize];
	lstrcpy(upload_server, DEFAULT_UPLOAD_SERVER);
	lstrcpy(upload_path, DEFAULT_UPLOAD_PATH);
	//lstrcpy(ua, DEFAULT_USER_AGENT);
	int upload_server_port = DEFAULT_UPLOAD_SERVER_PORT;

	TCHAR runtime_path[MAX_PATH+1];
	TCHAR runtime_dirname[MAX_PATH+1];
	TCHAR config_file[MAX_PATH+1];
	if (0 != ::GetModuleFileName(NULL, runtime_path, MAX_PATH)) {
		TCHAR tmp[MAX_PATH+1];
		_tsplitpath_s(runtime_path, tmp, runtime_dirname, tmp, tmp);
	}
	lstrcpy(config_file, runtime_dirname);
	lstrcat(config_file, _T("\\gyazo.ini"));
	if (PathFileExists(config_file)) {
		LPCTSTR SECTION_NAME = _T("gyazo");
		GetPrivateProfileString(SECTION_NAME, _T("server"), DEFAULT_UPLOAD_SERVER, upload_server, sizeof(upload_server), config_file);
		GetPrivateProfileString(SECTION_NAME, _T("path"), DEFAULT_UPLOAD_PATH, upload_path, sizeof(upload_path), config_file);
		//GetPrivateProfileString(SECTION_NAME, _T("user_agent"), DEFAULT_USER_AGENT, ua, sizeof(ua), config_file);
		upload_server_port = GetPrivateProfileInt(SECTION_NAME, _T("port"), DEFAULT_UPLOAD_SERVER_PORT, config_file);
	}

	const char*  sBoundary = "----BOUNDARYBOUNDARY----";		// boundary
	const char   sCrLf[]   = { 0xd, 0xa, 0x0 };					// 改行(CR+LF)
	const TCHAR* szHeader  = 
		_T("Content-type: multipart/form-data; boundary=----BOUNDARYBOUNDARY----");

	std::ostringstream	buf;	// 送信メッセージ
	std::string			idStr;	// ID
	
	// ID を取得
	idStr = getId();
	wchar_t fname[_MAX_FNAME];
	wchar_t ext[_MAX_EXT];
	_wsplitpath(fileName, NULL, NULL, fname, ext );
	std::string data = (isPng) ? "imagedata" : "data";
	LPCTSTR file = (isPng) ? _T("gyazo") : wcsncat(fname, ext, _MAX_FNAME);
	size_t size = wcstombs(NULL, file, 0);
	char* CharStr = new char[size + 1];
	wcstombs(CharStr, file, size + 1);

	// メッセージの構成
	// -- "id" part
	buf << "--";
	buf << sBoundary;
	buf << sCrLf;
	//buf << ua;
	//buf << sCrLf;
	buf << "content-disposition: form-data; name=\"id\"";
	buf << sCrLf;
	buf << sCrLf;
	buf << idStr;
	buf << sCrLf;

	// -- "imagedata" part
	buf << "--";
	buf << sBoundary;
	buf << sCrLf;
	buf << "content-disposition: form-data; name=\"";
	buf << data;
	buf << "\"; filename=\"";
	buf << CharStr;
	buf << "\"";
	buf << sCrLf;
	//buf << "Content-type: image/png";	// 一応
	//buf << sCrLf;
	buf << sCrLf;

	// 本文: PNG ファイルを読み込む
	std::ifstream png;
	png.open(fileName, std::ios::binary);
	if (png.fail()) {
		MessageBox(hwnd, _T("PNG open failed"), szTitle, MB_ICONERROR | MB_OK);
		png.close();
		return FALSE;
	}
	buf << png.rdbuf();		// read all & append to buffer
	png.close();

	// 最後
	buf << sCrLf;
	buf << "--";
	buf << sBoundary;
	buf << "--";
	buf << sCrLf;

	// メッセージ完成
	std::string oMsg(buf.str());

	// WinInet を準備 (proxy は 規定の設定を利用)
	HINTERNET hSession    = InternetOpen(_T("Gyazowin/1.0"), 
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(NULL == hSession) {
		LastErrorMessageBox(hwnd, _T("Cannot configure wininet."));
		return FALSE;
	}
	
	// 接続先
	HINTERNET hConnection = InternetConnect(hSession, 
		upload_server, upload_server_port,
		NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
	if(NULL == hConnection) {
		LastErrorMessageBox(hwnd, _T("Cannot initiate connection."));
		InternetCloseHandle(hSession);
		return FALSE;
	}

	// 要求先の設定
	HINTERNET hRequest    = HttpOpenRequest(hConnection,
		_T("POST"), upload_path, NULL,
		NULL, NULL, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD, NULL);
	if(NULL == hRequest) {
		LastErrorMessageBox(hwnd, _T("Cannot compose post request."));
		InternetCloseHandle(hConnection);
		InternetCloseHandle(hSession);
		return FALSE;
	}
	
	// 要求を送信
	BOOL bSuccess = FALSE;
	if (HttpSendRequest(hRequest,
                    szHeader,
					lstrlen(szHeader),
                    (LPVOID)oMsg.c_str(),
					(DWORD) oMsg.length()))
	{
		// 要求は成功
		
		DWORD resLen = 8;
		TCHAR resCode[8];

		// status code を取得
		if(!HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, resCode, &resLen, 0))
		{
			LastErrorMessageBox(hwnd, _T("Cannot get status code."));
			InternetCloseHandle(hRequest);
			InternetCloseHandle(hConnection);
			InternetCloseHandle(hSession);
			return FALSE;
		}

		if( _ttoi(resCode) != 200 ) {
			// upload 失敗 (status error)
			TCHAR errorBuf[200];
			StringCchPrintf((LPTSTR)errorBuf, 200, TEXT("Cannot upload the image. Error %s"),resCode);
			MessageBox(hwnd, errorBuf, szTitle, MB_ICONERROR | MB_OK);
		} else {
			// upload succeeded

			// get new id
			DWORD idLen = 100;
			TCHAR newid[100];
			
			memset(newid, 0, idLen*sizeof(TCHAR));	
			_tcscpy_s(newid, _T("X-Gyazo-Id"));

			HttpQueryInfo(hRequest, HTTP_QUERY_CUSTOM, newid, &idLen, 0);
			if (GetLastError() != ERROR_HTTP_HEADER_NOT_FOUND && idLen != 0) {
				//save new id
				saveId(newid);
			}

			// 結果 (URL) を読取る
			DWORD len;
			char  resbuf[1024];
			std::string result;
			
			// そんなに長いことはないけどまあ一応
			while(InternetReadFile(hRequest, (LPVOID) resbuf, 1024, &len) 
				&& len != 0)
			{
				result.append(resbuf, len);
			}

			// 取得結果は NULL terminate されていないので
			result += '\0';

			// クリップボードに URL をコピー
			setClipBoardText(result.c_str());
			
			// URL を起動
			execUrl(result.c_str()); 

			bSuccess = TRUE;
		}
	} else {
		// アップロード失敗...
		LastErrorMessageBox(hwnd, _T("Cannot connect to the server."));
	}

	// ハンドルクローズ
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnection);
	InternetCloseHandle(hSession);

	return bSuccess;

}
