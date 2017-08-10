#include "NumClock.h"
#include "resource.h"

/******************************************************************
WinMain
�������
******************************************************************/
int WINAPI WinMain(
    HINSTANCE	/* hInstance */		,
    HINSTANCE	/* hPrevInstance */	,
    LPSTR		/* lpCmdLine */		,
    int			/* nCmdShow */		)
{
    // Ignoring the return value because we want to continue running even in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            DemoApp app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}

/******************************************************************
DemoAppʵ��
******************************************************************/

DemoApp::DemoApp() :
    m_hwnd(NULL),
	m_submenu(NULL),
    m_pD2DFactory(NULL),
	m_pDWriteFactory(NULL),
    m_pRT(NULL),
    m_pBrush(NULL),
	m_pPathGeometry(NULL),
	m_pGeometrySink(NULL)
{
}

DemoApp::~DemoApp()
{
    SafeRelease(&m_pD2DFactory);
	SafeRelease(&m_pDWriteFactory);
    SafeRelease(&m_pRT);
    SafeRelease(&m_pBrush);
	SafeRelease(&m_pPathGeometry);
	SafeRelease(&m_pGeometrySink);
}

HRESULT DemoApp::Initialize()
{
    HRESULT hr;

    //register window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = DemoApp::WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = sizeof(LONG_PTR);
    wcex.hInstance     = HINST_THISCOMPONENT;
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName  = NULL;
    wcex.lpszClassName = L"D2DDemoApp";

    RegisterClassEx(&wcex);

    hr = CreateDeviceIndependentResources();
    if (SUCCEEDED(hr))
    {
		// ������ʾ���ߴ磬���ó��򴰿ڵ����½ǵ�λ��
		RECT workerArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workerArea, 0);
		LONG x = workerArea.right - 200;
		LONG y = workerArea.bottom - 70;

		// ��������

        FLOAT dpiX, dpiY;
        m_pD2DFactory->GetDesktopDpi(&dpiX, &dpiY);

        m_hwnd = CreateWindow(
            L"D2DDemoApp",
            L"NumClock",
            WS_POPUP,
            x,
            y,
            static_cast<UINT>(ceil(200 * dpiX / 96.f)),
            static_cast<UINT>(ceil(70 * dpiY / 96.f)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
            );
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
			ShowWindow(m_hwnd, SW_SHOWNORMAL);

			UpdateWindow(m_hwnd);
        }
    }

    return hr;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr;
    ID2D1GeometrySink *pSink = NULL;

    // ����D2D����
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

	if (m_pDWriteFactory == NULL && SUCCEEDED(hr))
	{
		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_pDWriteFactory),
			reinterpret_cast<IUnknown **>(&m_pDWriteFactory));
	}

    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRT)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
            );

        // ����render target
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRT
            );

		// ������ˢ
        if (SUCCEEDED(hr))
        {
            hr = m_pRT->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::MediumPurple),
                &m_pBrush
                );
        }
	}

    return hr;
}

HRESULT DemoApp::CreateCustomFont(const char* text)
{
	HRESULT hr = S_FALSE;

	// ��������ͼ��
	if (m_pDWriteFactory != NULL)
	{
		IDWriteFontFace* pFontFace = NULL;
		IDWriteFontFile* pFontFiles = NULL;

		if (SUCCEEDED(hr))
		{
			WCHAR strPath[MAX_PATH] = {0};
			WCHAR fontPath[MAX_PATH] = {0};

			// ��ȡ��ִ���ļ�·��
			::GetModuleFileName(0, strPath, MAX_PATH);
			::PathRemoveFileSpec(strPath);

			// �������ļ�ת���������ļ�·��
			wsprintf(fontPath, L"%s\\%s", strPath, L"UnidreamLED.ttf");

			hr = m_pDWriteFactory->CreateFontFileReference(
				fontPath,
				NULL,
				&pFontFiles);
		}

		// ����FontFace
		IDWriteFontFile* fontFileArray[] = {pFontFiles};
		m_pDWriteFactory->CreateFontFace(
			DWRITE_FONT_FACE_TYPE_TRUETYPE,
			1, // file count
			fontFileArray,
			0,
			DWRITE_FONT_SIMULATIONS_NONE,
			&pFontFace
			);

		//char* text = "DriteWrite custom font";
		UINT lengthText = strlen(text);

		// ��FontFace�����㼯��
		UINT* pCodePoints = new UINT[lengthText];
		UINT16* pGlyphIndices = new UINT16[lengthText];
		ZeroMemory(pCodePoints, sizeof(UINT) * lengthText);
		ZeroMemory(pGlyphIndices, sizeof(UINT16) * lengthText);
		for(int i=0; i<lengthText; ++i)
		{
			pCodePoints[i] = text[i];
		}
		pFontFace->GetGlyphIndicesW(pCodePoints, lengthText, pGlyphIndices);

		// ���ı��ĵ㼯�ϴ���·��ͼ��
		m_pD2DFactory->CreatePathGeometry(&m_pPathGeometry);

		m_pPathGeometry->Open((ID2D1GeometrySink**)&m_pGeometrySink);

		pFontFace->GetGlyphRunOutline(
			(48.0f/72.0f)*96.0f, 
			pGlyphIndices, 
			NULL,
			NULL,
			lengthText,
			FALSE,
			FALSE,
			m_pGeometrySink);

		m_pGeometrySink->Close();

		if(pCodePoints)
		{
			delete [] pCodePoints;
			pCodePoints = NULL;
		}

		if(pGlyphIndices)
		{
			delete [] pGlyphIndices;
			pGlyphIndices = NULL;
		}
	}

	return hr;
}

void DemoApp::DiscardDeviceResources()
{
    SafeRelease(&m_pRT);
    SafeRelease(&m_pBrush);
	SafeRelease(&m_pPathGeometry);
}

void DemoApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

HRESULT DemoApp::OnRender()
{
    HRESULT hr;

    hr = CreateDeviceResources();
    if (SUCCEEDED(hr) && !(m_pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
    {
        // ��ʼ����
        m_pRT->BeginDraw();

        m_pRT->Clear(D2D1::ColorF(D2D1::ColorF::Aquamarine));

		m_pRT->SetTransform(D2D1::Matrix3x2F::Translation(20, 60));

		// ��ʱ���ʽ���ı�
		SYSTEMTIME time;
		GetLocalTime(&time);

		char text[256] = {0};
		sprintf(text, "%02d:%02d:%02d", time.wHour, time.wMinute, time.wSecond);

		hr = CreateCustomFont(text);

		// ����
		m_pRT->FillGeometry(m_pPathGeometry, m_pBrush);
		
        // ��������
        hr = m_pRT->EndDraw();

		m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());

        if (hr == D2DERR_RECREATE_TARGET)
        {
            hr = S_OK;
            DiscardDeviceResources();
        }
    }
	DiscardDeviceResources();
    InvalidateRect(m_hwnd, NULL, FALSE);

    return hr;
}

void DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRT)
    {
        D2D1_SIZE_U size;
        size.width = width;
        size.height = height;

        // Note: This method can fail, but it's okay to ignore the
        // error here -- it will be repeated on the next call to
        // EndDraw.
        m_pRT->Resize(size);
    }
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        DemoApp *pDemoApp = (DemoApp *)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            PtrToUlong(pDemoApp)
            );

		{
			// ���δ���
			SetLayeredWindowAttributes(hwnd, 0, 180, LWA_ALPHA);
			HRGN hRegion = CreateRectRgn(0, 0, 200, 70);
			SetWindowRgn(hwnd, hRegion, TRUE);

			// ���ص����˵�
			HMENU menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_CLOSE));
			pDemoApp->m_submenu = GetSubMenu(menu, 0);
		}

        result = 1;
    }
    else
    {
        DemoApp *pDemoApp = reinterpret_cast<DemoApp *>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
                )));

        bool wasHandled = false;

        if (pDemoApp)
        {
            switch(message)
            {
			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
				case IDM_CLOSE:
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					break;
				}
				return 0;

            case WM_SIZE:
                {
                    UINT width = LOWORD(lParam);
                    UINT height = HIWORD(lParam);
                    pDemoApp->OnResize(width, height);
                }
                result = 0;
                wasHandled = true;
                break;

			case WM_RBUTTONUP:
				POINT pos;
				pos.x = LOWORD(lParam);
				pos.y = HIWORD(lParam);

				ClientToScreen(hwnd, &pos);
				TrackPopupMenu(pDemoApp->m_submenu, TPM_LEFTALIGN, pos.x, pos.y, 0, hwnd, NULL);
				break;

			case WM_LBUTTONDOWN:
				// ������ڿͻ�������Ϣת�����ǿͻ���
				// ��Ϳ��Ե���ͻ����ƶ�������
				PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
				break;

            case WM_PAINT:
            case WM_DISPLAYCHANGE:
                {
                    PAINTSTRUCT ps;
                    BeginPaint(hwnd, &ps);

                    pDemoApp->OnRender();
                    EndPaint(hwnd, &ps);
                }
                result = 0;
                wasHandled = true;
                break;

            case WM_DESTROY:
                {
                    PostQuitMessage(0);
                }
                result = 1;
                wasHandled = true;
                break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}