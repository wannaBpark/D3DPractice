#include <Windows.h>

// D3D ��뿡 �ʿ��� ���̺귯������ ��ũ�մϴ�.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D ��뿡 �ʿ��� ������ϵ��� �����մϴ�.
#include <d3d11.h>
#include <d3dcompiler.h>

class URenderer
{
public:
	// Direct3D 11 ��ġ(Device)�� ��ġ ���ؽ�Ʈ(Device Context) �� ����ü��(Swap chain)�� �����ϱ� ���� �����͵�
	ID3D11Device* Device = nullptr;	// GPU�� ����ϱ� ���� D3D ��ġ
	ID3D11DeviceContext* DeviceContext = nullptr; // GPU ��� ������ ����ϴ� ���ؽ�Ʈ 
	IDXGISwapChain* SwapChain = nullptr;		  // ������ ���۸� ��ü�ϴ� �� ���Ǵ� ���� ü��

	// �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������
	ID3D11Texture2D* FrameBuffer = nullptr; // ȭ�� ��¿� �ؽ�ó
	ID3D11RenderTargetView* FrameBufferRTV = nullptr; // �ؽ�ó�� ���� Ÿ������ ����ϴ� ��
	ID3D11RasterizerState* RasterizerState = nullptr;  // �����Ͷ����� ����(�ø�, ä��� ��� �� ����)
	ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ����

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
	D3D11_VIEWPORT ViewportInfo; //������ ������ �����ϴ� ����Ʈ ����

public:
	// ������ �ʱ�ȭ �Լ�
	void Create(HWND hWindow)
	{
		// Direct3D ��ġ �� ����ü�� ����
		CreateDeviceAndSwapChain(hWindow);

		// ������ ���� ����
		CreateFrameBuffer();

		// �����Ͷ����� ���� ����
		CreateRasterizerState();

		// ���� ���ٽ� ���� �� ���� ���´� �� �ڵ忡�� �ٷ��� ����
	}

	// Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// �����ϴ� Direct3D ��� ������ ����
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// ���� ü�� ���� ����ü �ʱ�ȭ
		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		swapchaindesc.BufferDesc.Width = 0; // â ũ�⿡ �°� �ڵ����� ����
		swapchaindesc.BufferDesc.Height = 0; // â ũ�⿡ �°� �ڵ����� ����
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ���� 
		swapchaindesc.SampleDesc.Count = 1; // ��Ƽ ���ø� ��Ȱ��ȭ
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // ���� Ÿ������ ���
		swapchaindesc.BufferCount = 2; // ���� ���۸�
		swapchaindesc.OutputWindow = hWindow; // �������� â �ڵ�
		swapchaindesc.Windowed = TRUE; // â���
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // ���� ���

		// Direct3D ��ġ�� ���� ü���� ����
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
			&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// ������ ���� ü���� ���� �������� 
		SwapChain->GetDesc(&swapchaindesc);

		// ����Ʈ ���� ����
		ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	// Direct3D ��ġ �� ���� ü���� �����ϴ� �Լ�
	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext) {
			DeviceContext->Flush(); // �����ִ� GPU ��� ����
		}

		if (SwapChain) {
			SwapChain->Release();
			SwapChain = nullptr;
		}

		if (Device) {
			Device->Release();
			Device = nullptr;
		}

		if (DeviceContext) {
			DeviceContext->Release();
			DeviceContext = nullptr;
		}
	}

	// ������ ���۸� �����ϴ� �Լ�
	void CreateFrameBuffer()
	{
		// ���� ü�����κ��� �� ���� �ؽ�ó ��������
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// ���� Ÿ�� �� ����
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ����
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D �ؽ�ó

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV); // FrameBuffer�� 0�ϼ��ִٴ� ����?
	}

	// ������ ���۸� �����ϴ� �Լ�
	void ReleaseFrameBuffer()
	{
		if (FrameBuffer) {
			FrameBuffer->Release();
			FrameBuffer = nullptr;
		}
		if (FrameBufferRTV) {
			FrameBufferRTV->Release();
			FrameBufferRTV = nullptr;
		}
	}

	// �����Ͷ����� ���¸� �����ϴ� �Լ�
	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.FillMode = D3D11_FILL_SOLID; // ä��� ���
		rasterizerdesc.CullMode = D3D11_CULL_BACK;	// �� ���̽� �ø�

		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
	}

	// �����Ͷ����� ���¸� �����ϴ� �Լ�
	void ReleaseRasterizerState()
	{
		if (RasterizerState) {
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
	}

	// �������� ���� ��� ���ҽ��� �����ϴ� �Լ�
	void Release()
	{
		RasterizerState->Release();

		// ���� Ÿ���� �ʱ�ȭ
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // Deffered Context���� OMS �Ƚᵵ �������� ������ RTV�� �׸�

		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	// ���� ü���� �� ���ۿ� ����Ʈ ���۸� ��ü�Ͽ� ȭ�鿡 ���
	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1 : VSync Ȱ��ȭ
	}


};

// ���� �޽����� ó���� �Լ�
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		// Signal that the app should quit
		PostQuitMessage(0); break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ������ Ŭ���� �̸�
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ������ Ÿ��Ʋ�ٿ� ǥ�õ� �̸�
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޽����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� �ִ´�
	WNDCLASS wndclass = { 0, WndProc, 0,0,0,0,0,0, WindowClass };

	// ������ Ŭ���� ���
	RegisterClassW(&wndclass);

	// 1024 x 1024 ũ�⿡ ������ ����
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	// Renderer Class�� ����
	URenderer renderer;

	// D3D11 �����ϴ� �Լ��� ȣ���մϴ�.
	renderer.Create(hWnd);


	bool bIsExit = false;

	// ���� �����ϴ� �ڵ���� ���⿡ �߰��մϴ�.

	// Main Loop (Quit Message�� ������ ������ �Ʒ� Loop�� ������ �����ϰ� ��)
	while (bIsExit == false) {
		MSG msg;

		// ó���� �޽����� �� �̻� ���� ������ ����
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			// Ű �Է� �޽����� ����
			TranslateMessage(&msg);

			// �޽����� ������ ������ ���ν����� ����, �޽����� ������ ����� WndProc�� ���޵�
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				bIsExit = true;
				break;
			}
		}

		////////////////////////////////////////////
		// �Ź� ����Ǵ� �ڵ带 ���⿡ �߰��մϴ�.
		
		// ���� ȭ�鿡 �������� ���ۿ� �׸��� �۾��� ���� ���۸� ���� ��ȯ�մϴ�.
		renderer.SwapBuffer();

		////////////////////////////////////////////

	}
	// D3D11 �Ҹ��Ű�� �Լ��� ȣ���մϴ�.
	renderer.Release();

	return 0;  
}