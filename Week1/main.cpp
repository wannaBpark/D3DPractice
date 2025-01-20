///////////////////////TODO : SWapchain ���� ����
#include <windows.h>

#define SAFE_RELEASE( x ) {if(x){(x)->Release();(x)=NULL;}}

// D3D ��뿡 �ʿ��� ���̺귯������ ��ũ�մϴ�.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D ��뿡 �ʿ��� ������ϵ��� �����մϴ�.
#include <d3d11.h>
#include <d3dcompiler.h>

// ImGui ���� ���
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include <iostream>
#include <algorithm> // clamp

// 1. Define the triangle vertices 
struct FVertexSimple {
	float x, y, z;		// Position (������ǥ��� ��ȯ�ϸ� 4����)
	float r, g, b, a;	// Color 
};

// structure for a 3D vector
struct FVector3 {
	float x, y, z;
	FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

struct FVector2 {
	float x, y;
	FVector2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

#include "Sphere.h"

// �ﰢ���� �ϵ� �ڵ� (vertex���� ��ġ, ������ ������ ����) / �޼���ǥ��(�ð����, �޼��� ī�޶� ���ϵ���)�̹Ƿ� vertex������ ��������
FVertexSimple triangle_vertices[] = {
	{0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, // Top vertex(red)
	{1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right vertex(green)
	{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom-left vertex(blue)
};

// With No Z-depth test .. �׷��� ������ ���ϼ��� ���δ�
FVertexSimple cube_vertices[] = {
	// Front face (Z+) (ȭ�� �ٷ� ���� �簢��, z�� ����ӿ� ����)
	{-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // Bottom-left (red)
	{-0.5f,  0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-left (yellow)
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)
	{-0.5f,  0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-left (yellow)
	{ 0.5f,	 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-right (blue)
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)

	// Back face (Z-) (�޼���ǥ��� ������ ��, �� ��ڸ� ���� | ���� �� ������ ���ϰ� �޼��� ������ 1) ���� 2) Z 0.5�� �����ϸ� �ش� ���� �׷�����)
	{-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f}, // Bottom-left (cyan)
	{ 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f}, // Bottom-right (magenta)
	{-0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-left (blue)
	{-0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-left (blue)
	{ 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f}, // Bottom-right (magenta)
	{ 0.5f,  0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-right (yellow)

	// Left face (X-)
	{-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f }, // Bottom-left (purple)
	{-0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-left (blue)
	{-0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)
	{-0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-left (blue)
	{-0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-right (yellow)
	{-0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)

	// Right face (X+) (�̰� �� �߸��� ��)
	{0.5f, -0.5f, -0.5f, 1.0f, 0.5f, 0.0f, 1.0f }, // Bottom-left (orange)
	{0.5f, -0.5f,  0.5f, 0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{0.5f,  0.5f, -0.5f, 0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{0.5f,  0.5f, -0.5f, 0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{0.5f, -0.5f,  0.5f, 0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 0.5f, 1.0f }, // Top-right (dark blue)

	// Top face (Y+) Bottom�� front �� 2�� ����, z=-0.5f
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.5f, 1.0f }, // Bottom-left (light green)
	{ -0.5f,  0.5f,  0.5f,  0.0f, 0.5f, 1.0f, 1.0f }, // Top-left (cyan)
	{  0.5f,  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f }, // Bottom-right (white)
	{ -0.5f,  0.5f,  0.5f,  0.0f, 0.5f, 1.0f, 1.0f }, // Top-left (cyan)
	{  0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.0f, 1.0f }, // Top-right (brown)
	{  0.5f,  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 1.0f }, // Bottom-right (white)

	// Bottom face (Y-)
	{ -0.5f, -0.5f, -0.5f,  0.5f, 0.5f, 0.0f, 1.0f }, // Bottom-left (brown)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top-left (red)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f }, // Bottom-right (purple)
	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, 1.0f }, // Top-left (red)
	{  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 1.0f }, // Top-right (green)
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.5f, 1.0f }, // Bottom-right (purple)
};

// width 0.05, height 0.2 (1:4)
FVertexSimple player1_vertices[] = {
	{-0.95f, -0.1f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // Bottom left  (red)
	{-0.95f,  0.1f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top left	  (yellow)	 
	{-0.9f,  -0.1f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom right (blue)
	{-0.95f,  0.1f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top left		(yellow)
	{-0.9f,   0.1f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f }, // Top right	(green)
	{-0.9f,  -0.1f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom right	(blue)
};

FVertexSimple player2_vertices[] = {
	{0.9f, -0.1f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // Bottom left  (red)
	{0.9f,  0.1f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top left	  (yellow)	 
	{0.95f,  -0.1f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom right (blue)
	{0.9f,  0.1f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top left		(yellow)
	{0.95f,   0.1f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f }, // Top right	(green)
	{0.95f,  -0.1f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom right	(blue)
};

class URenderer
{
public:
	// Direct3D 11 ��ġ(Device)�� ��ġ ���ؽ�Ʈ(Device Context) �� ����ü��(Swap chain)�� �����ϱ� ���� �����͵�
	ID3D11Device* Device = nullptr;	// GPU�� ����ϱ� ���� D3D ��ġ (��� ���� ���˰� �ڿ� �Ҵ�)
	ID3D11DeviceContext* DeviceContext = nullptr; // GPU ��� ������ ����ϴ� ���ؽ�Ʈ (GPU�� ������ ������ ���.. ������ �ڿ��� ���������ο� ���⵵ ��)
	IDXGISwapChain* SwapChain = nullptr;		  // ������ ���۸� ��ü�ϴ� �� ���Ǵ� ���� ü�� (I:Interface)

	// �������� �ʿ��� ���ҽ� �� ���¸� �����ϱ� ���� ������
	ID3D11Texture2D* FrameBuffer = nullptr; // ȭ�� ��¿� �ؽ�ó
	ID3D11RenderTargetView* FrameBufferRTV = nullptr; // �ؽ�ó�� ���� Ÿ������ ����ϴ� �ڿ� ��(�ؽ�ó�� ������ ������� ������ �����Ϸ��� �ڿ��䰡 �ʿ�, R)
	ID3D11RasterizerState* RasterizerState = nullptr;  // �����Ͷ����� ����(�ø�, ä��� ��� �� ����)
	

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // ȭ���� �ʱ�ȭ(clear)�� �� ����� ���� (RGBA)
	D3D11_VIEWPORT ViewportInfo; //������ ������ �����ϴ� ����Ʈ ����

	// ���̴� �ڵ� �߰�
	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

	// ��� ����
	struct FConstants {
		FVector3 Offset;
		float Pad;
	};
	ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ���� (����, �� ������ ��� ���� ���� �ٲ㰡�� ���)

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
		swapchaindesc.BufferDesc.Width = 0;								// â ũ�⿡ �°� �ڵ����� ����
		swapchaindesc.BufferDesc.Height = 0;							// â ũ�⿡ �°� �ڵ����� ����
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	// ���� ���� | �ؽ�ó���� DXGI ������ ���ǵ� �ڷ����ĸ� ���� �� ����
		swapchaindesc.SampleDesc.Count = 1;								// ��Ƽ ���ø� ��Ȱ��ȭ
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ���� Ÿ������ ���
		swapchaindesc.BufferCount = 2;									// ���� ���۸�
		swapchaindesc.OutputWindow = hWindow;							// �������� â �ڵ�
		swapchaindesc.Windowed = TRUE;									// â���
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// ���� ���

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

		SAFE_RELEASE(SwapChain);
		SAFE_RELEASE(Device);
		SAFE_RELEASE(DeviceContext);
	}

	// ����ü�� ũ�� ���� (â�� �� Ŀ�� �� pixel�� �þ���̴� ���� ����)
	void ResizeSwapChain(int nxtWidth, int nxtHeight)
	{
		if (SwapChain) {
			// [1] Swapchain�� ������ Resource ���� (pre-resize) : ���� ������� FrameBuffer, RTV .. (DepthStencilView�� �ؾ� ��)
			SAFE_RELEASE(FrameBuffer);			
			SAFE_RELEASE(FrameBufferRTV);

			// [2] SwapChain ũ�� ���� (ù��° ���� 0�̸� ���� ����ü�� ���� �� ����)
			SwapChain->ResizeBuffers(0, nxtWidth, nxtHeight, DXGI_FORMAT_UNKNOWN, 0);
			
			// [3] ���ο� ���� Ÿ�� �� ���� / ũ�⿡ �°� viewport ũ�� ���� (post-resize)
			CreateFrameBuffer();
			ViewportInfo.Width = nxtWidth;
			ViewportInfo.Height = nxtHeight;
		}
	}

	// [2] [������ ���۸� �����ϴ� �Լ�] : Swapchain ������ �ؽ�ó�� ���, ������������ ���������ο� ���´�
	void CreateFrameBuffer()
	{
		// ���� ü�����κ��� �� ���� �ؽ�ó �������� (������ �ε��� 0)  | �ؽ�ó�� ����Ϸ��� �ؽ�ó �������̽��� ���;� ��
		// �� �� FrameBuffer�� �������� �̹����� �����ϴ� ���� ���
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// ���� Ÿ�� �� ����
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // ���� ����
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D �ؽ�ó�� ���� Ÿ������ ����

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV); // FrameBuffer�� RTV�� ���� | FrameBuffer�� 0�ϼ��ִ� ����:GetBufferȣ�� �����߰ų�, FB�� nullptr�϶�
	}

	// ������ ���۸� �����ϴ� �Լ�
	void ReleaseFrameBuffer()
	{
		SAFE_RELEASE(FrameBuffer);
		SAFE_RELEASE(FrameBufferRTV);
	}

	// �����Ͷ����� ���¸� �����ϴ� �Լ�
	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.FillMode = D3D11_FILL_SOLID; // �ܻ� ä��� ���
		rasterizerdesc.CullMode = D3D11_CULL_BACK;	// �� ���̽� �ø�:���� ���� �׸��� �ʵ���

		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState); // ������ �����Ͷ����� ���¸� Device*�� ����
	}

	// �����Ͷ����� ���¸� �����ϴ� �Լ�
	void ReleaseRasterizerState()
	{
		SAFE_RELEASE(RasterizerState);
	}

	// �������� ���� ��� ���ҽ��� �����ϴ� �Լ�
	void Release()
	{
		RasterizerState->Release();

		// ���� Ÿ���� �ʱ�ȭ
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // ����) Deffered Context���� OMS �Ƚᵵ �������� ������ RTV�� �׸�

		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	// ���� ü���� �� ���ۿ� ����Ʈ ���۸� ��ü�Ͽ� ȭ�鿡 ���
	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1 : VSync Ȱ��ȭ | �ĸ�� ���� ���۸� ��ȯ�Ͽ� ȭ�� ǥ���ϴ� ��(=Present ����)
	}

	void CreateShader()
	{
		ID3DBlob* vertexShaderCSO;			
		ID3DBlob* pixelShaderCSO;

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexShaderCSO, nullptr);					// ������ ���� : ������ ���н� Blob ��ü ���ڿ��� ���� �޽��� ����
		Device->CreateVertexShader(vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);
		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelShaderCSO, nullptr);
		Device->CreatePixelShader(pixelShaderCSO->GetBufferPointer(), pixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);
		
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout); 

		Stride = sizeof(FVertexSimple);
	}

	void ReleaseShader()
	{
		// Release�� �ڵ� ��������  ȣ��
		SAFE_RELEASE(SimpleInputLayout);
		SAFE_RELEASE(SimplePixelShader);
		SAFE_RELEASE(SimpleVertexShader);
	}

	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
		
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		DeviceContext->RSSetViewports(1, &ViewportInfo); // �� �޼���� �������� ���� ����Ʈ���� �ڵ����� ������
		DeviceContext->RSSetState(RasterizerState);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr); // �� ���ۿ� �ش��ϴ� RTV�� �����ϰ� ������� (��º��ձ�)
		DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	}

	void PrepareShader()
	{
		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
		DeviceContext->IASetInputLayout(SimpleInputLayout);

		// ���ؽ� ���̴��� ��� ���� ����
		if (ConstantBuffer) {
			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		
		DeviceContext->Draw(numVertices, 0);
	}

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
	{
		// 2. Create a vertex buffer
		D3D11_BUFFER_DESC vertexbufferdesc = {};
		vertexbufferdesc.ByteWidth = byteWidth;
		vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;			// will never be updated
		vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexBufferSRD = { vertices };
		ID3D11Buffer* vertexBuffer;

		Device->CreateBuffer(&vertexbufferdesc, &vertexBufferSRD, &vertexBuffer);
		
		return vertexBuffer;
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		vertexBuffer->Release();
	}

	// ���� ������ �ʴ� ��(CPU���� GPU�� ������ ����)�� ������ ������� ���� (D3D���� 16����Ʈ ��� ũ�� �Ծ�)
	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; // ensure constant buffer	size is multiple of 16 bytes (�ּ� 16����Ʈ �̻� & 0xfffffff0 = ���� ������ ū 16�� ���)
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame (�� ��ü���� �������� ���� ����)
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; 

		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		SAFE_RELEASE(ConstantBuffer);
	}

	// GPU�� ���� ConstantBuffer�� �ӽ÷� MSR�� ������ �����ϰ�, �ٽ� ����� GPU�� ���� ����
	void UpdateConstant(FVector3 Offset)
	{
		if (ConstantBuffer) {
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;			// CPU���� �����ϵ��� ������۸� �ӽ÷� ������ ����

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // �������� ��� ���� ������Ʈ (���ν� CPU�� ������ ���� ����)
			FConstants* constants = (FConstants*)constantbufferMSR.pData; { // ���ε� ������ �ý��� �޸� �ּ� ������ | �ش� �ý��� �޸𸮸� ���� ����
				constants->Offset = Offset;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);		// ���ҽ��� GPU���� �������� ���·� �ǵ��� (GPU�� ���� �б� ����)
		}
	}
};

class UObject {
public:
	FVertexSimple* vertices;
	FVector3 pos;
	FVector3 offset;
	FVector3 velocity;
	union {
		FVector2 renderRadius;
		struct {
			float width;
			float height;
		};
	};
	UObject() : vertices(nullptr), offset(0.0f), velocity(0.0f), width(0.0f), height(0.0f), pos(0.0f)  {}
};

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // �Լ��� ���� �� �ڵ����� extern�� ����(�Ⱥٿ����ȴٴ� ��) / ����� ���� �Լ����� ��������� ��� �ִ��� �˾ƾ��� -> extern ��� (<->scanf�� �̹� �����ϵ� ���̺귯�� O)

struct AABB {
	float xMin, yMin;
	float xMax, yMax;
};
static inline int IsCollision(UObject& sphere, UObject& paddle, float sphereRadius)
{
	float dx = std::fabs(sphere.offset.x - (paddle.pos.x + paddle.offset.x));
	float dy = std::fabs(sphere.offset.y - (paddle.pos.y + paddle.offset.y));

	if (dx <= (sphereRadius + paddle.width / 2.0f) && dy <= (sphereRadius + paddle.height / 2.0f)) {
		return true;
	}
	return false;
}
// ���� �޽����� ó���� �Լ�
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}

	switch (message) {
	case WM_DESTROY:
		// Signal that the app should quit
		PostQuitMessage(0); break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ������ Ŭ���� �̸�
	WCHAR WindowClass[] = L"JungleWindowClass";

	// ������ Ÿ��Ʋ�ٿ� ǥ�õ� �̸�
	WCHAR Title[] = L"Game Tech Lab";

	// ���� �޽����� ó���� �Լ��� WndProc�� �Լ� �����͸� WindowClass ����ü�� �ִ´�
	WNDCLASS wndclass = { 0, WndProc, 0,0,0,0,0,0,0, WindowClass };

	// ������ Ŭ���� ���
	RegisterClassW(&wndclass);

	FVector2 wndSize = { 600, 600 };
	float aspectRatio = wndSize.x / wndSize.y;
	// 1024 x 1024 ũ�⿡ ������ ����
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, wndSize.x, wndSize.y,
		nullptr, nullptr, hInstance, nullptr);

	// Renderer Class�� ����
	URenderer renderer;

	renderer.Create(hWnd);   // D3D11 �����ϴ� �Լ��� ȣ��
	renderer.CreateShader(); // ������ ���� ���� ���̴� ����
	renderer.CreateConstantBuffer(); // ��� ���� ����

	// ImGui ���� �� �ʱ�ȭ �Լ� ȣ��
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// ������,���̴� ���� �� VertexBuffer ����	
	// [����] (numvertices�� �ٸ� ����ü�� ���� �� �ڵ尡 ����Ǵµ�, �̸� ��ũ�η� �ڵ����� �߰��� ��� + ������ �������� ��� ��������
	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	UINT numVerticesPlayer = sizeof(player1_vertices) / sizeof(FVertexSimple);

	std::unique_ptr<FVertexSimple[]> nxtsphere_vertices{ new FVertexSimple[numVerticesSphere] }; // �ػ󵵿� ���� ����� �� ���� ������ �迭

	// Vertex Buffer�� �ѱ�� ���� Scale Down��
	float scaleMod = 0.1f;
	for (UINT i = 0; i < numVerticesSphere; ++i) {
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}
	std::copy(sphere_vertices, sphere_vertices+numVerticesSphere, nxtsphere_vertices.get());

	ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	// ��1��, �е� 2�� ����
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
	ID3D11Buffer* vertexBufferPlayer1 = renderer.CreateVertexBuffer(player1_vertices, sizeof(player1_vertices));
	ID3D11Buffer* vertexBufferPlayer2 = renderer.CreateVertexBuffer(player2_vertices, sizeof(player2_vertices));

	enum ETypePrimitive {
		EPT_Triangle,
		EPT_Cube,
		EPT_Sphere,
		EPT_Max,
	};

	ETypePrimitive typePrimitive = EPT_Sphere;
	
	UINT numObjects = 3;
	UObject* objects = new UObject[numObjects];
	ID3D11Buffer* vertexbuffers[3] = { vertexBufferSphere, vertexBufferPlayer1, vertexBufferPlayer2 };
	UINT numVertices[3] = { numVerticesSphere , numVerticesPlayer,numVerticesPlayer };
	objects[0].vertices = sphere_vertices; objects[1].vertices = player1_vertices; objects[2].vertices = player2_vertices;
	objects[1].width = objects[2].width = 0.05f;	// Paddle width
	objects[1].height = objects[2].height = 0.2f;	// Paddle height
	objects[1].pos.x = -0.925f;
	objects[2].pos.x = 0.925f;

	const float leftBorder = -1.0f;
	const float rightBorder = 1.0f;
	const float topBorder = -1.0f;
	const float bottomBorder = 1.0f;
	const float sphereRadius = 1.0f;

	bool bBoundBallToScreen = true;
	bool bBoundPaddleToScreen = true;
	bool bPinballMovement = true;

	const float ballSpeed = 0.001f;
	objects[0].velocity.y = ((float)(rand() % 100 - 50)) * ballSpeed;
	objects[0].velocity.x = ((float)(rand() % 100 - 50)) * ballSpeed;

	const int targetFPS = 30;							// �ʴ� �ִ� 30�� ���� (targetFPS�� ���ϼ���, ballSpeed�� ��������)
	const double targetFrameTime = 1000.0f / targetFPS; // �� �������� ��ǥ �ð� (�и��� ����)

	// ���� Ÿ�̸� �ʱ�ȭ
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;

	bool bIsExit = false;

	// Main Loop (Quit Message�� ������ ������ �Ʒ� Loop�� ������ �����ϰ� ��)
	while (bIsExit == false) {
		QueryPerformanceCounter(&startTime);
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
			} else if (msg.message == WM_KEYDOWN) { // ������ �������� ���� ���̺� �����Ͽ� ���� ����ȭ �ϴ� switch-case�� ���� (������ �������� ����/���� ������)
				if (!bPinballMovement) {
					switch (msg.wParam) {
					case VK_LEFT:	objects[0].offset.x -= 0.1f; break;
					case VK_UP:		objects[0].offset.y += 0.1f; break;
					case VK_RIGHT:	objects[0].offset.x += 0.1f; break;
					case VK_DOWN:	objects[0].offset.y -= 0.1f; break;
					}
				}
				if (GetAsyncKeyState(0x57) & 0x8000) { // W Ű : Player1 ��
					objects[1].offset.y += 0.1f;
				}
				if (GetAsyncKeyState(0x53) & 0x8000) { // S Ű : Player1 ��
					objects[1].offset.y -= 0.1f;
				}
				if (GetAsyncKeyState(0x49) & 0x8000) { // I Ű : Player2 ��
					objects[2].offset.y += 0.1f;
				}
				if (GetAsyncKeyState(0x4A) & 0x8000) { // J Ű : Player2 ��
					objects[2].offset.y -= 0.1f;
				}
			} 
		}
		// Ű���� ó�� ���� ȭ�� ���� ����ٸ� ȭ�� �������� ����ġ��Ų��. (ȭ�� ����� �ʴ� �ɼ��̶��)
		if (bBoundBallToScreen) {
			float renderRadius = sphereRadius * scaleMod;
			auto& offset = objects[0].offset;
			renderRadius *= (aspectRatio > 1.0f) ?  (1.0f / (aspectRatio)) : aspectRatio;			// âũ�⿡ ���� �ְ� ����
			offset.x = std::clamp(offset.x, leftBorder + renderRadius, rightBorder - renderRadius);
			offset.y = std::clamp(offset.y, topBorder + renderRadius, bottomBorder - renderRadius);
		}
		if (bBoundPaddleToScreen) {
			for (size_t i = 1; i < 3; ++i) {
				auto& offset = objects[i].offset;
				FVector2 renderRadius = { objects[i].width/2.0f, objects[i].height/2.0f };
				offset.x = std::clamp(offset.x, leftBorder + renderRadius.x, rightBorder -  renderRadius.x);
				offset.y = std::clamp(offset.y, topBorder +  renderRadius.y, bottomBorder - renderRadius.y);
			}
		}
		if (bPinballMovement) {
			// �ӵ��� ����ġ�� ���� ���� ���������� ������
			UObject& sphere = objects[0];
			auto& offset = sphere.offset;
			auto& velocity = sphere.velocity;
			offset.x += velocity.x;
			offset.y += velocity.y;
			offset.z += velocity.z;

			// ���� �浹 ���θ� üũ�ϰ� �浹�� �ӵ��� ������ ���� ������ �ٲ�
			float renderRadius = sphereRadius * scaleMod;
			if (offset.x < leftBorder + renderRadius) {
				velocity.x *= -1.0f;
			}
			if (offset.x > rightBorder - renderRadius) {
				velocity.x *= -1.0f;
			}
			if (offset.y < topBorder + renderRadius) {
				velocity.y *= -1.0f;
			}
			if (offset.y > bottomBorder - renderRadius) {
				velocity.y *= -1.0f;
			}

			for (size_t i = 1; i < 3; ++i) {
				UObject& paddle = objects[i];
				if (IsCollision(sphere, paddle, renderRadius)) {
					sphere.velocity.x *= -1.0f;
				}
			}
		}
		////////////////////////////////////////////
		// �Ź� ����Ǵ� �ڵ带 ���⿡ �߰�
		
		// [1] ������ �غ� �۾�
		renderer.Prepare();
		renderer.PrepareShader();

		for (size_t i = 0; i < numObjects; ++i) {
			auto& object = objects[i];
			renderer.UpdateConstant(object.offset);
			renderer.DeviceContext->VSSetConstantBuffers(0, 1, &renderer.ConstantBuffer);
			renderer.RenderPrimitive(vertexbuffers[i], numVertices[i]);
		}

		// [2] ImGui ������ �غ�, ��Ʈ�� ����, ������ ��û
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// [2] ���� ImGui UI ��Ʈ�� �߰��� ImGui::NewFrame()�� Render() ������ ���⿡ ��ġ�մϴ�
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle world!");
		ImGui::Checkbox("Bound Ball to Screen", &bBoundBallToScreen);
		ImGui::Checkbox("Bound Paddle to Screen", &bBoundPaddleToScreen);
		ImGui::Checkbox("Pinball Movement", &bPinballMovement);
		ImGui::InputFloat2("Change Window Size", &wndSize.x, "%.0f");

		if (ImGui::Button("Apply size Change")) {
			// ������ ũ�� ����
			RECT rect = { 0, 0, static_cast<LONG>(wndSize.x), static_cast<LONG>(wndSize.y) };
			AdjustWindowRect(&rect, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW, FALSE);
			SetWindowPos(hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, (int)wndSize.x, (int)wndSize.y, SWP_NOMOVE | SWP_NOZORDER);

			renderer.ResizeSwapChain(static_cast<int>(wndSize.x), static_cast<int>(wndSize.y));

			// ImGui �浹������ ���� ���� ���÷��� ũ��� �����ӹ��� ũ�� ���� / ImGui �Ҹ�
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			// ���ο� ���ؽ�Ʈ ����, ���ο� hWnd�� �ʱ�ȭ
			ImGui::CreateContext();
			ImGui_ImplWin32_Init((void*)hWnd);
			ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

			SAFE_RELEASE(vertexBufferSphere);
			
			aspectRatio = wndSize.x / wndSize.y;
			for (UINT i = 0; i < numVerticesSphere; ++i) {
				nxtsphere_vertices[i].x = (aspectRatio > 1.0f) ? sphere_vertices[i].x / aspectRatio : sphere_vertices[i].x;
				nxtsphere_vertices[i].y = (aspectRatio < 1.0f) ? sphere_vertices[i].y * aspectRatio : sphere_vertices[i].y;
			}
			vertexBufferSphere = renderer.CreateVertexBuffer(nxtsphere_vertices.get(), sizeof(sphere_vertices));
			vertexbuffers[0] = vertexBufferSphere;
		} else {
			ImGui::End();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // ������ ��û
		}
		// [3] ���� ȭ�鿡 �������� ���ۿ� �׸��� �۾��� ���� ���۸� ���� ��ȯ
		renderer.SwapBuffer();

		do {
			Sleep(0);

			QueryPerformanceCounter(&endTime);

			// �� �������� �ҿ�� �ð� ��� (�и��� ������ ��ȯ) = (Ŭ�� �ֱ� ����) * 1000.0f / �ʴ� Ŭ�� Ƚ�� = c * 1000.0f / (c/s) = (ms)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0f / frequency.QuadPart; // .QuadPart(����ü�� 64��Ʈ ���� �������)
		} while (elapsedTime < targetFrameTime);
		
	}

	delete[] objects;
	// ImGui �Ҹ�
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM ������Ʈ�� Release�θ� ���� �Ҹ� ���� (delete X)
	renderer.ReleaseVertexBuffer(vertexBufferTriangle);	// ���ؽ� ���� �Ҹ��� ������ �Ҹ� ���� ó��
	renderer.ReleaseVertexBuffer(vertexBufferCube);
	renderer.ReleaseVertexBuffer(vertexBufferSphere);

	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();   // ������ �Ҹ� ���� ���̴��� �Ҹ��Ű�� �Լ� ȣ��
	renderer.Release();			// D3D11 �Ҹ��Ű�� �Լ��� ȣ���մϴ�.

	return 1;  
}