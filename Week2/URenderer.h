#pragma once

#define SAFE_RELEASE( x ) {if(x){(x)->Release();(x)=NULL;}}

// D3D 사용에 필요한 라이브러리들을 링크합니다.
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

// D3D 사용에 필요한 헤더파일들을 포함합니다.
#include <d3d11.h>
#include <d3dcompiler.h>

#include <vector>
#include "FStruct.h"


class URenderer
{
public:
	// Direct3D 11 장치(Device)와 장치 컨텍스트(Device Context) 및 스왑체인(Swap chain)을 관리하기 위한 포인터들
	ID3D11Device* Device = nullptr;	// GPU와 통신하기 위한 D3D 장치 (기능 지원 점검과 자원 할당)
	ID3D11DeviceContext* DeviceContext = nullptr; // GPU 명령 실행을 담당하는 컨텍스트 (GPU가 수행할 렌더링 명령.. 설정과 자원을 파이프라인에 묶기도 함)
	IDXGISwapChain* SwapChain = nullptr;		  // 프레임 버퍼를 교체하는 데 사용되는 스왑 체인 (I:Interface)

	// 렌더링에 필요한 리소스 및 상태를 관리하기 위한 변수들
	ID3D11Texture2D* FrameBuffer = nullptr; // 화면 출력용 텍스처
	ID3D11RenderTargetView* FrameBufferRTV = nullptr; // 텍스처를 렌더 타겟으로 사용하는 자원 뷰(텍스처를 렌더링 출력으로 받으려 연결하려면 자원뷰가 필요, R)
	ID3D11RasterizerState* RasterizerState = nullptr;  // 래스터라이저 상태(컬링, 채우기 모드 등 정의)


	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f }; // 화면을 초기화(clear)할 때 사용할 색상 (RGBA)
	D3D11_VIEWPORT ViewportInfo; //렌더링 영역을 정의하는 뷰포트 정보

	// 셰이더 코드 추가
	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;

	std::vector<ID3D11Buffer*> m_VertexBuffers;
	std::vector<UINT> m_numVertices;
	unsigned int Stride;

	// 상수 버퍼
	struct FConstants {
		FVector3 Offset;
		float Pad;
	};
	ID3D11Buffer* ConstantBuffer = nullptr; // 셰이더에 데이터를 전달하기 위한 상수 버퍼 (공용, 매 프레임 상수 버퍼 값을 바꿔가며 출력)

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
		swapchaindesc.BufferDesc.Width = 0;								// 창 크기에 맞게 자동으로 설정
		swapchaindesc.BufferDesc.Height = 0;							// 창 크기에 맞게 자동으로 설정
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	// 색상 포맷 | 텍스처에는 DXGI 열겨형 정의된 자료형식만 담을 수 있음
		swapchaindesc.SampleDesc.Count = 1;								// 멀티 샘플링 비활성화
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 렌더 타겟으로 사용
		swapchaindesc.BufferCount = 2;									// 더블 버퍼링
		swapchaindesc.OutputWindow = hWindow;							// 렌더링할 창 핸들
		swapchaindesc.Windowed = TRUE;									// 창모드
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// 스왑 방식

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

		SAFE_RELEASE(SwapChain);
		SAFE_RELEASE(Device);
		SAFE_RELEASE(DeviceContext);
	}

	// 스왑체인 크기 변경 (창이 더 커질 때 pixel이 늘어나보이는 현상 방지)
	void ResizeSwapChain(int nxtWidth, int nxtHeight)
	{
		if (SwapChain) {
			// [1] Swapchain과 관련한 Resource 해제 (pre-resize) : 기존 백버퍼인 FrameBuffer, RTV .. (DepthStencilView도 해야 함)
			SAFE_RELEASE(FrameBuffer);
			SAFE_RELEASE(FrameBufferRTV);

			// [2] SwapChain 크기 조정 (첫번째 인자 0이면 기존 스왑체인 버퍼 수 유지)
			SwapChain->ResizeBuffers(0, nxtWidth, nxtHeight, DXGI_FORMAT_UNKNOWN, 0);

			// [3] 새로운 렌더 타겟 뷰 생성 / 크기에 맞게 viewport 크기 조정 (post-resize)
			CreateFrameBuffer();
			ViewportInfo.Width = nxtWidth;
			ViewportInfo.Height = nxtHeight;
		}
	}

	// [2] [프레임 버퍼를 생성하는 함수] : Swapchain 생성후 텍스처를 얻고, 렌더링용으로 파이프라인에 묶는다
	void CreateFrameBuffer()
	{
		// 스왑 체인으로부터 백 버퍼 텍스처 가져오기 (지정된 인덱스 0)  | 텍스처를 사용하려면 텍스처 인터페이스를 얻어와야 함
		// 이 때 FrameBuffer는 렌더링된 이미지를 저장하는 데에 사용
		SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer)); // 확장 : [__uuidof(ID3D11Texture2D), (void**)&FrameBuffer], IUnknown 상속 체크해줌

		// 렌더 타겟 뷰 생성
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // 색상 포맷
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // 2D 텍스처를 렌더 타겟으로 설정

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV); // FrameBuffer를 RTV로 설정 | FrameBuffer가 0일수있다 에러:GetBuffer호출 실패했거나, FB가 nullptr일때
	}

	// 프레임 버퍼를 해제하는 함수
	void ReleaseFrameBuffer()
	{
		SAFE_RELEASE(FrameBuffer);
		SAFE_RELEASE(FrameBufferRTV);
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
		SAFE_RELEASE(RasterizerState);
	}

	// 렌더러에 사용된 모든 리소스를 해제하는 함수
	void Release()
	{
		RasterizerState->Release();

		// 렌더 타겟을 초기화
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr); // 참고) Deffered Context에선 OMS 안써도 문제없이 여러개 RTV에 그림

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

		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexShaderCSO, nullptr);					// 마지막 인자 : 컴파일 실패시 Blob 객체 문자열에 오류 메시지 저장
		Device->CreateVertexShader(vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);
		D3DCompileFromFile(L"ShaderW0.hlsl", nullptr, nullptr, "mainPS", "ps_5_0", 0, 0, &pixelShaderCSO, nullptr);
		Device->CreatePixelShader(pixelShaderCSO->GetBufferPointer(), pixelShaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);


		Device->CreateInputLayout(FVertexCOLOR_DECLARATION::m_ElementDesc, ARRAYSIZE(FVertexCOLOR_DECLARATION::m_ElementDesc), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

		Stride = sizeof(FVertexSimple);
	}

	void ReleaseShader()
	{
		// Release는 코드 역순으로  호출
		SAFE_RELEASE(SimpleInputLayout);
		SAFE_RELEASE(SimplePixelShader);
		SAFE_RELEASE(SimpleVertexShader);
	}

	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DeviceContext->RSSetViewports(1, &ViewportInfo); // 이 메서드로 설정되지 않은 뷰포트들은 자동으로 해제됨
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

	void RenderPrimitiveById(size_t i, size_t vsId, size_t psId)
	{
		UINT offset = 0;
		auto& pBuffer = m_VertexBuffers[i];
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		DeviceContext->Draw(m_numVertices[i], 0);
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

	void ReleaseVertexBuffers()
	{
		for (auto& vb : m_VertexBuffers) {
			vb->Release();
		}
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
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame (매 물체마다 동적으로 갱신 가능)
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		SAFE_RELEASE(ConstantBuffer);
	}

	// GPU와 묶인 ConstantBuffer를 임시로 MSR로 가져와 수정하고, 다시 떼어내어 GPU에 도로 묶음
	void UpdateConstant(FVector3 Offset)
	{
		if (ConstantBuffer) {
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;			// CPU접근 가능하도록 상수버퍼를 임시로 가져올 버퍼

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // 매프레임 상수 버퍼 업데이트 (맵핑시 CPU가 데이터 수정 가능)
			FConstants* constants = (FConstants*)constantbufferMSR.pData; { // 맵핑된 버퍼의 시스템 메모리 주소 가져옴 | 해당 시스템 메모리를 수정 가능
				constants->Offset = Offset;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);		// 리소스를 GPU와의 정상적인 상태로 되돌림 (GPU가 이제 읽기 가능)
		}
	}
};