#include <windows.h>

// D3D 사용에 필요한 라이브러리들을 링크합니다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함합니다.
#include <d3d11.h>
#include <d3dcompiler.h>

// ImGui 관련 헤더
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

// 1. Define the triangle vertices 
struct FVertexSimple {
	float x, y, z;		// Position (동차좌표계로 변환하면 4차원)
	float r, g, b, a;	// Color 
};

// structure for a 3D vector
struct FVector3 {
	float x, y, z;
	FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

#include "Sphere.h"

// 삼각형을 하드 코딩 (vertex마다 위치, 색상을 가지는 구조) / 왼손좌표계(시계방향, 왼손이 카메라를 향하도록)이므로 vertex순서도 위오른왼
FVertexSimple triangle_vertices[] = {
	{0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, // Top vertex(red)
	{1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right vertex(green)
	{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}, // Bottom-left vertex(blue)
};

// With No Z-depth test .. 그려진 순서가 뒤일수록 보인다
FVertexSimple cube_vertices[] = {
	// Front face (Z+) (화면 바로 앞의 사각형, z가 양수임에 유의)
	{-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f}, // Bottom-left (red)
	{-0.5f,  0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-left (yellow)
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)
	{-0.5f,  0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f}, // Top-left (yellow)
	{ 0.5f,	 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f}, // Top-right (blue)
	{ 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f}, // Bottom-right (green)

	// Back face (Z-) (왼손좌표계로 말았을 때, 내 등뒤를 향함 | 만약 내 정면을 향하게 왼손을 말도록 1) 순서 2) Z 0.5로 변경하면 해당 면이 그려진다)
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

	// Right face (X+) (이거 좀 잘못된 듯)
	{0.5f, -0.5f, -0.5f, 1.0f, 0.5f, 0.0f, 1.0f }, // Bottom-left (orange)
	{0.5f, -0.5f,  0.5f, 0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{0.5f,  0.5f, -0.5f, 0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{0.5f,  0.5f, -0.5f, 0.5f, 0.0f, 0.5f, 1.0f }, // Top-left (purple)
	{0.5f, -0.5f,  0.5f, 0.5f, 0.5f, 0.5f, 1.0f }, // Bottom-right (gray)
	{0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 0.5f, 1.0f }, // Top-right (dark blue)

	// Top face (Y+) Bottom이 front 위 2개 정점, z=-0.5f
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

class URenderer
{
public:
	// Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑체인(Swap chain)을 관리하기 위한 포인터들
	ID3D11Device* Device = nullptr;	// GPU와 통신하기 위한 D3D 장치 (기능 지원 점검과 자원 할당)
	ID3D11DeviceContext* DeviceContext = nullptr; // GPU 명령 실행을 담당하는 컨텍스트 (GPU가 수행할 렌더링 명령.. 설정과 자원을 파이프라인에 묶기도 함)
	IDXGISwapChain* SwapChain = nullptr;		  // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인 (I:Interface)

	// 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
	ID3D11Texture2D* FrameBuffer = nullptr; // 화면 출력용 텍스처
	ID3D11RenderTargetView* FrameBufferRTV = nullptr; // 텍스처를 렌더 타겟으로 사용하는 뷰
	ID3D11RasterizerState* RasterizerState = nullptr;  // 래스터라이저 상태(컬링, 채우기 모드 등 정의)
	

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
	D3D11_VIEWPORT ViewportInfo; //렌더링 영역을 정의하는 뷰포트 정보

	// 셰이더 코드 추가
	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

	// 상수 버퍼
	struct FConstants {
		FVector3 Offset;
		float Pad;
	};
	ID3D11Buffer* ConstantBuffer = nullptr; // 셰이더에 데이터를 전달하기 위한 상수 버퍼

public:
	// 렌더러 초기화 함수
	void Create(HWND hWindow)
	{
		// Direct3D 장치 및 스왑체인 생성
		CreateDeviceAndSwapChain(hWindow);

		// 프레임 버퍼 생성
		CreateFrameBuffer();

		// 래스터라이저 상태 생성
		CreateRasterizerState();

		// 깊이 스텐실 버퍼 및 블렌드 상태는 이 코드에서 다루지 않음
	}

	// Direct3D 장치 및 스왑 체인을 생성하는 함수
	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		// 지원하는 Direct3D 기능 레벨을 정의
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		// 스왑 체인 설정 구조체 초기화
		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		swapchaindesc.BufferDesc.Width = 0; // 창 크기에 맞게 자동으로 설정
		swapchaindesc.BufferDesc.Height = 0; // 창 크기에 맞게 자동으로 설정
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷 | 텍스처에는 DXGI 열겨형 정의된 자료형식만 담을 수 있음
		swapchaindesc.SampleDesc.Count = 1; // 멀티 샘플링 비활성화
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 렌더 타겟으로 사용
		swapchaindesc.BufferCount = 2; // 더블 버퍼링
		swapchaindesc.OutputWindow = hWindow; // 렌더링할 창 핸들
		swapchaindesc.Windowed = TRUE; // 창모드
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 스왑 방식

		// Direct3D 장치와 스왑 체인을 생성
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
			&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

		// 생성된 스왑 체인의 정보 가져오기 
		SwapChain->GetDesc(&swapchaindesc);

		// 뷰포트 정보 설정
		ViewportInfo = { 0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f };
	}

	// Direct3D 장치 및 스왑 체인을 해제하는 함수
	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext) {
			DeviceContext->Flush(); // 남아있는 GPU 명령 실행
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

	// 프레임 버퍼를 생성하는 함수 (텍
	void CreateFrameBuffer()
	{
		// 스왑 체인으로부터 백 버퍼 텍스처 가져오기 (지정된 인덱스 0)
		// 이 때 FrameBuffer는 렌더링된 이미지를 저장하는 데에 사용
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		// 렌더 타겟 뷰 생성
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처를 렌더 타겟으로 설정

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV); // FrameBuffer를 RTV로 설정 | FrameBuffer가 0일수있다 에러:GetBuffer호출 실패했거나, FB가 nullptr일때
	}

	// 프레임 버퍼를 해제하는 함수
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

	// 래스터라이저 상태를 생성하는 함수
	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.FillMode = D3D11_FILL_SOLID; // 단색 채우기 모드
		rasterizerdesc.CullMode = D3D11_CULL_BACK;	// 백 페이스 컬링:뒤쪽 면을 그리지 않도록

		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState); // 생성된 래스터라이저 상태를 Device*에 적용
	}

	// 래스터라이저 상태를 해제하는 함수
	void ReleaseRasterizerState()
	{
		if (RasterizerState) {
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
	}

	// 렌더러에 사용된 모든 리소스를 해제하는 함수
	void Release()
	{
		RasterizerState->Release();

		// 렌더 타겟을 초기화
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // Deffered Context에선 OMS 안써도 문제없이 여러개 RTV에 그림

		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	// 스왑 체인의 백 버퍼와 프론트 버퍼를 교체하여 화면에 출력
	void SwapBuffer()
	{
		SwapChain->Present(1, 0); // 1 : VSync 활성화 | 후면과 전면 버퍼를 교환하여 화면 표시하는 것(=Present 제시)
	}

	void CreateShader()
	{
		ID3DBlob* vertexShaderCSO;
		ID3DBlob* pixelShaderCSO;

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexShaderCSO, nullptr);
		Device->CreateVertexShader(vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);
		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelShaderCSO, nullptr);
		Device->CreatePixelShader(pixelShaderCSO->GetBufferPointer(), pixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);
		
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

		Stride = sizeof(FVertexSimple);
		
		/*SimpleVertexShader->Release();
		SimplePixelShader->Release();*/
	}

	void ReleaseShader()
	{
		// Release는 코드 역순으로  호출
		if (SimpleInputLayout) {
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader) {
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader) {
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}

	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
		
		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		DeviceContext->RSSetViewports(1, &ViewportInfo);

		DeviceContext->RSSetViewports(1, &ViewportInfo);
		DeviceContext->RSSetState(RasterizerState);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr); // 백 버퍼에 해당하는 RTV를 생성하고 묶어야함 (출력병합기)
		DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

	}

	void PrepareShader()
	{
		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
		DeviceContext->IASetInputLayout(SimpleInputLayout);

		// 버텍스 셰이더의 상수 버퍼 설정
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

	// 보통 변하지 않는 값(CPU에서 GPU로 보내기 위해)을 저장할 상수버퍼 생성 (D3D에선 16바이트 배수 크기 규약)
	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; // ensure constant buffer	size is multiple of 16 bytes (최소 16바이트 이상 & 0xfffffff0 = 가장 가깝고 큰 16의 배수)
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame;
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; 

		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer) {
			ConstantBuffer->Release();
			ConstantBuffer = nullptr;
		}
	}

	// GPU와 묶인 ConstantBuffer를 임시로 MSR로 가져와 수정하고, 다시 떼어내어 GPU에 도로 묶음
	void UpdateConstant(FVector3 Offset)
	{
		if (ConstantBuffer) {
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;			// CPU접근 가능하도록 상수버퍼를 임시로 가져올 버퍼

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // 매프레임 상수 버퍼 업데이트 (맵핑시 CPU가 데이터 수정 가능)
			FConstants* constants = (FConstants*)constantbufferMSR.pData; { // 맵핑된 버퍼의 실제 메모리 주소 가져옴 | 멤버 변수 바꿀 땐 중괄호 쓰는듯
				constants->Offset = Offset;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);		// 리소스를 GPU와의 정상적인 상태로 되돌림
		}
	}
};

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // 함수는 선언 시 자동으로 extern이 붙음(안붙여도된다는 뜻) / 사용자 정의 함수여서 명시적으로 어디에 있는지 알아야함 -> extern 사용 (<->scanf는 이미 컴파일된 라이브러리 O)

// 각종 메시지를 처리할 함수
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
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다
	WNDCLASS wndclass = { 0, WndProc, 0,0,0,0,0,0,0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	// 1024 x 1024 크기에 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
		nullptr, nullptr, hInstance, nullptr);

	// Renderer Class를 생성
	URenderer renderer;

	renderer.Create(hWnd);   // D3D11 생성하는 함수를 호출
	renderer.CreateShader(); // 렌더러 생성 직후 셰이더 생성
	renderer.CreateConstantBuffer(); // 상수 버퍼 생성

	// ImGui 생성 및 초기화 함수 호출
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// 렌더러,셰이더 생성 후 VertexBuffer 생성	
	// [질문1] (numvertices는 다른 구조체를 가질 때 코드가 변경되는데, 이를 매크로로 자동으로 추가할 방법 + 소유권 누구한테 줘야 적당할지
	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	// Vertex Buffer로 넘기기 전에 Scale Down함
	float scaleMod = 0.1f;
	for (UINT i = 0; i < numVerticesSphere; ++i) {
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}

	ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	enum ETypePrimitive {
		EPT_Triangle,
		EPT_Cube,
		EPT_Sphere,
		EPT_Max,
	};

	ETypePrimitive typePrimitive = EPT_Triangle;
	FVector3 offset(0.0f);

	bool bIsExit = false;

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false) {
		MSG msg;

		// 처리할 메시지가 더 이상 없을 때까지 수행
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			// 키 입력 메시지를 번역
			TranslateMessage(&msg);

			// 메시지를 적절한 윈도우 프로시저에 전달, 메시지가 위에서 등록한 WndProc로 전달됨
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				bIsExit = true;
				break;
			} else if (msg.message == WM_KEYDOWN) { // 조건이 많다면 점프 테이블 생성하여 성능 최적화 하는 switch-case가 유리
				if (msg.wParam == VK_LEFT) {
					offset.x -= 0.1f;
				} else if (msg.wParam == VK_RIGHT) {
					offset.x += 0.1f;
				} else if (msg.wParam == VK_UP) {
					offset.y += 0.1f;
				} else if (msg.wParam == VK_DOWN) {
					offset.y -= 0.1f;
				}
			}
		}

		////////////////////////////////////////////
		// 매번 실행되는 코드를 여기에 추가
		
		// [1] 렌더러 준비 작업
		renderer.Prepare();
		renderer.PrepareShader();

		renderer.UpdateConstant(offset);	// offset을 상수 버퍼에 넣어 업데이트

		switch (typePrimitive) {			// 생성한 버텍스 버퍼를 넘겨 실질적인 렌더링 요청
		case EPT_Triangle:
			renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle); 
			break;
		case EPT_Cube:
			renderer.RenderPrimitive(vertexBufferCube, numVerticesCube);
			break;
		case EPT_Sphere:
			renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
			break;
		}

		// [2] ImGui 렌더링 준비, 컨트롤 설정, 렌더링 요청
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// [2] 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 Render() 사이인 여기에 위치합니다
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle world!");

		if (ImGui::Button("Change Primitive")) {
			switch (typePrimitive) {
			case EPT_Triangle:
				typePrimitive = EPT_Cube;
				break;
			case EPT_Cube:
				typePrimitive = EPT_Sphere;
				break;
			case EPT_Sphere:
				typePrimitive = EPT_Triangle;
				break;
			}
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // 렌더링 요청

		// [3] 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환
		renderer.SwapBuffer();

		////////////////////////////////////////////

	}
	
	// ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM 오브젝트는 Release로만 각각 소멸 가능 (delete X)
	renderer.ReleaseVertexBuffer(vertexBufferTriangle);	// 버텍스 버퍼 소멸은 렌더러 소멸 전에 처리
	renderer.ReleaseVertexBuffer(vertexBufferCube);
	renderer.ReleaseVertexBuffer(vertexBufferSphere);

	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();   // 렌더러 소멸 직전 셰이더를 소멸시키는 함수 호출
	renderer.Release();			// D3D11 소멸시키는 함수를 호출합니다.

	return 1;  
}