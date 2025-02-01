///////////////////////TODO : SWapchain 정보 변경
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

#include <iostream>
#include <algorithm> // clamp

// 1. Define the triangle vertices 
struct FVertexSimple {
	float x, y, z;		// Position (동차좌표계로 변환하면 4차원)
	float r, g, b, a;	// Color 
};

#include "Sphere.h"

#define SAFE_RELEASE( x ) {if(x){(x)->Release();(x)=NULL;}}

#pragma region FVectors
// structure for a 3D vector
struct FVector3 {
	float x, y, z;
	FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
	FVector3 operator+(const FVector3& other) const noexcept {
		return { x + other.x, y + other.y, z + other.z };
	}
	FVector3 operator-(const FVector3& other) const noexcept {
		return { x - other.x, y - other.y, z - other.z };
	}
	FVector3 operator*(float scale) const noexcept {
		return { x * scale, y * scale, z * scale };
	}
	FVector3 operator/(float scale) const noexcept {
		return { x / scale, y / scale, z / scale };
	}
	void normalize() {
		float w = sqrt(x * x + y * y + z * z);
		*this = { x / w , y / w , z / w };
	}
	float dot(const FVector3& other) const noexcept {
		return x * other.x + y * other.y + z * other.z;
	}

	FVector3 cross(const FVector3& other) const noexcept {
		return { y * other.z - z * other.y, x * other.z - z * other.x, x * other.y - y * other.x, };
	}
	float length() const {
		return sqrt(x * x + y * y + z * z);
	}

	// 로드리게스 회전 공식  vnxt = vcos + (k x v) * s + k * (k * v) ( 1 - cos)
	FVector3 rotateAround(FVector3 axis, float angle) const {
		FVector3 ret;
		float s = sin(angle);
		float c = cos(angle);
		axis.normalize();
		return ret = { *(this) * c + axis.cross(*this) * s + axis * (axis.dot(*this) * (1 - c)) };
	}
};
struct FVector2 {
	float x, y;
	FVector2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
	FVector2 operator+(const FVector2& other) const noexcept {
		return { x + other.x, y + other.y };
	}
};
#pragma endregion

#pragma region URenderer
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
	unsigned int Stride;

	// 상수 버퍼
	struct FConstants {
		FVector3 Offset;
		float Radius;
	};

	ID3D11Buffer* ConstantBuffer = nullptr; // 셰이더에 데이터를 전달하기 위한 상수 버퍼 (공용, 매 프레임 상수 버퍼 값을 바꿔가며 출력)

	struct FInstances { // 같은 버텍스 버퍼를 공유하면서, 인스턴스 별로 달리할 자료들
		FVector3 Offset;
		float Radius;
	};
	ID3D11Buffer* InstanceBuffer = nullptr;

public:
	// 렌더러 초기화 함수
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);	// Direct3D 장치 및 스왑체인 
		CreateFrameBuffer();				// 프레임 버퍼 생성
		CreateRasterizerState();			// 래스터라이저 상태 생성
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

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderCSO->GetBufferPointer(), vertexShaderCSO->GetBufferSize(), &SimpleInputLayout);

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
	void UpdateConstant(FVector3 Offset, float radius)
	{
		if (ConstantBuffer) {
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;			// CPU접근 가능하도록 상수버퍼를 임시로 가져올 버퍼

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // 매프레임 상수 버퍼 업데이트 (맵핑시 CPU가 데이터 수정 가능)
			FConstants* constants = (FConstants*)constantbufferMSR.pData; { // 맵핑된 버퍼의 시스템 메모리 주소 가져옴 | 해당 시스템 메모리를 수정 가능
				constants->Offset = Offset;
				constants->Radius = radius;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);		// 리소스를 GPU와의 정상적인 상태로 되돌림 (GPU가 이제 읽기 가능)
		}
	}

	void CreateInstanceBuffer()
	{
		D3D11_BUFFER_DESC instancebufferdesc = {};
		instancebufferdesc.ByteWidth = sizeof(FInstances) + 0xf & 0xfffffff0; // ensure constant buffer	size is multiple of 16 bytes (최소 16바이트 이상 & 0xfffffff0 = 가장 가깝고 큰 16의 배수)
		instancebufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame (매 물체마다 동적으로 갱신 가능)
		instancebufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		instancebufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		Device->CreateBuffer(&instancebufferdesc, nullptr, &InstanceBuffer);
	}
};

#pragma endregion

enum class Border {
	Right = 1,
	Left = -1,
	Top = -1,
	Bottom = 1,
};

class UBall {
public:
	// 클래스 이름과, 아래 두개의 변수 이름은 변경하지 않습니다.
	FVector3 Location;
	FVector3 Velocity;
	float Radius;
	float Mass;

	// Sphere가 공통으로 사용할 버텍스 버퍼 , 버텍스 개수, 중력속도
	static ID3D11Buffer* pVertexBuffer;
	static const UINT numVertices = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	static const float gravitySpeed;
	FVector3 AngularVelocity;
	int Polarity;																		// N, S 극성
	

	// 두 물체간 만유 인력 처리
	void ResolveGravityBall(UBall*& other, float deltaTime)
	{
		static constexpr float G = 6.67384e-11f;

		auto& m1 = Mass; auto& m2 = other->Mass;
		const float massSum = m1 + m2;
		const float combinedRadii = Radius + other->Radius;

		FVector3& x1 = Location; auto& x2 = other->Location;
		FVector3& v1 = Velocity; auto& v2 = other->Velocity;

		const FVector3 dX = x1 - x2;
		const float r = sqrt(dX.dot(dX));
		const float F = G * (m1 * m2) / (r * r);
		const FVector3 dir = (dX) / (r * r);

		const FVector3 fDir = dir * F;
		
		// 각각의 가속도만큼, 서로 끌어당기는 방향으로 속도 갱신
		v1 = v1 - (fDir / m1) * deltaTime;
		v2 = v2 + (fDir / m2) * deltaTime;
	}

	// 두 물체간 충돌 처리 (비탄성 / 탄성)
	void ResolveCollisionBall(UBall*& other, bool bInElastic, float e)
	{
		constexpr float epsilon = 1e-4f;
		auto& m1 = Mass; auto& m2 = other->Mass;
		const float massSum = m1 + m2;
		const float combinedRadii = Radius + other->Radius;

		FVector3& x1 = Location; auto& x2 = other->Location;
		FVector3& v1 = Velocity; auto& v2 = other->Velocity;
		FVector3& av1 = AngularVelocity; auto& av2 = other->AngularVelocity;

		const FVector3 dX = x1 - x2;
		const FVector3 dV = v1 - v2;

		float norm = dX.dot(dX); float dist = sqrt(norm);
		if (norm < epsilon) return;

		// 구끼리 뭉쳐있는 경우 보정 - 각 구끼리 붙어있는 만큼 중점에서 서로 밀어내도록
		if (norm < combinedRadii * combinedRadii) {
			float overlap = (combinedRadii - dist) * 0.5f;
			FVector3 correction = dX * overlap;								
			x1 = x1 + correction;
			x2 = x2 - correction;
		}

		if (bInElastic) { // 비탄성 충돌 옵션 : 충돌 후 상대 속도의 크기가 충돌 전보다 작아짐
			v1 = v1 + dX * ( ((-1 - e) * m2) / massSum) * (dV.dot(dX) / norm);
			v2 = v2 - dX * ( ((-1 - e) * m1) / massSum) * (dV.dot(dX) / norm);
		} else { // 탄성 충돌 전후 : 전체 운동 에너지가 보존, 상대 속도 크기가 같음
			v1 = v1 - dX * ((2 * m2) / massSum) * (dV.dot(dX) / norm);
			v2 = v2 + dX * ((2 * m1) / massSum) * (dV.dot(dX) / norm);
		}

		// 토크 -> 회전 효과
		float impulseMagnitude = -2 * dV.dot((dX / dist)) / (1 / m1 + 1 / m2);
		FVector3 impulse = dX / dist * impulseMagnitude;

		// 충격량 반영
		//v1 = v1 + impulse / m1;
		//v2 = v2 - impulse / m2;

		// 회전 효과 적용 (토크 기반)
		FVector3 contactPoint = dX / dist * -Radius;  // 충돌 지점
		FVector3 t1 = contactPoint.cross(impulse); // τ = r × F
		contactPoint = contactPoint * -1.0f;
		FVector3 t2 = contactPoint.cross(impulse);


		float I1 = (2.0f / 5.0f) * m1 * Radius * Radius; // I = (2/5) * m * r²
		float I2 = (2.0f / 5.0f) * m2 * other->Radius * other->Radius;

		av1 = av1 + t1 / I1; // 각속도 갱신
		av2 = av2 + t2 / I2;
	}

	void BoundToScreen() noexcept
	{
		// 벽에 충돌 시 반대 방향으로 작용
		if (Location.x >= static_cast<float>(Border::Right) - Radius || Location.x <= static_cast<float>(Border::Left) + Radius) {
			Velocity.x *= -1.0f;
		} else if (Location.y >= static_cast<float>(Border::Bottom) - Radius || Location.y <= static_cast<float>(Border::Top) + Radius) {
			Velocity.y *= -1.0f;
		}
	}

	void DoRender(URenderer& renderer) const
	{
		renderer.UpdateConstant(this->Location, this->Radius);
		renderer.DeviceContext->VSSetConstantBuffers(0, 1, &renderer.ConstantBuffer);
		renderer.RenderPrimitive(pVertexBuffer, numVertices);
	}
	void Rotate(float deltaTime)
	{
		FVector3 rotationAxis = FVector3(0.0f);
		rotationAxis.normalize();
		float angle = AngularVelocity.length() * deltaTime;
		if (angle > 1e-5f) {
			Location = Location.rotateAround(rotationAxis, angle);
		}
	}

	void Move(float deltaTime)
	{
		Location.x += Velocity.x * deltaTime;
		Location.y += Velocity.y * deltaTime;
		Location.z += Velocity.z * deltaTime;
		Rotate(deltaTime);
	}

	

	void Update(bool& bGravity, float deltaTime)
	{
		if (bGravity) {
			Velocity.y -= gravitySpeed * deltaTime;
		} 
					
		Move(deltaTime);
		BoundToScreen();

		// Bound to Screen
		Location.x = std::clamp(Location.x, static_cast<float>(Border::Left) + Radius, static_cast<float>(Border::Right) - Radius); // 고민해볼 지점 : 성능 차이?
		Location.y = std::clamp(Location.y, static_cast<float>(Border::Top) + Radius, static_cast<float>(Border::Bottom) - Radius);
	}
	
	UBall() : Location(0.0f), Velocity(0.0f), Radius((float)(rand() % 3 + 1) * 0.04f), Mass(3.0f*Radius), AngularVelocity(0.0f), Polarity(rand()%RAND_MAX < 0.5f ? -1 : 1)
	{
		Location.x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2 - 1;						// [-1, 1]
		Location.y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - Radius - 0.25f;			// [최소 화면 중간 높이, 맨 위]
		Velocity.x = 0.00001f;																					// 미세하게 공이 구르도록
		Velocity.x *= (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) < 0.5f ? 1.0f : -1.0f;
	}
};
ID3D11Buffer* UBall::pVertexBuffer = nullptr;
const float UBall::gravitySpeed = 0.000006f;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // 함수는 선언 시 자동으로 extern이 붙음(안붙여도된다는 뜻) / 사용자 정의 함수여서 명시적으로 어디에 있는지 알아야함 -> extern 사용 (<->scanf는 이미 컴파일된 라이브러리 O)

static inline int IsCollisionBall(UBall*& ball1, UBall*& ball2)
{
	const FVector3 dX = ball1->Location - ball2->Location;
	const float dist = dX.dot(dX);
	const float combinedRadii= ball1->Radius + ball2->Radius;
	const float epsilon = 1e-6f;

	if (dist + epsilon <= combinedRadii * combinedRadii) {
		return true;
	}
	return false;
}
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
#pragma region WindowInit
	// 윈도우 클래스 이름
	WCHAR WindowClass[] = L"JungleWindowClass";

	// 윈도우 타이틀바에 표시될 이름
	WCHAR Title[] = L"Game Tech Lab";

	// 각종 메시지를 처리할 함수인 WndProc의 함수 포인터를 WindowClass 구조체에 넣는다
	WNDCLASS wndclass = { 0, WndProc, 0,0,0,0,0,0,0, WindowClass };

	// 윈도우 클래스 등록
	RegisterClassW(&wndclass);

	FVector2 wndSize = { 800, 800 };
	float aspectRatio = wndSize.x / wndSize.y;
	// 1024 x 1024 크기에 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, wndSize.x, wndSize.y,
		nullptr, nullptr, hInstance, nullptr);
#pragma endregion

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
	FVertexSimple* nxtsphere_vertices{ new FVertexSimple[UBall::numVertices] }; // 해상도에 따라 변경될 구 정점 저장할 배열
	std::copy(sphere_vertices, sphere_vertices + UBall::numVertices, nxtsphere_vertices);
	float tmp = UBall::numVertices;
	
	UINT numBalls = 1;
	UBall** BallList = new UBall*[numBalls];

	/* ImGui 관련 변수 */
	bool bGravity = false;			// 중력 낙하 여부
	int inputNumBalls = 1;			// 구 생성 개수
	bool bInElastic = false;		// 비탄성 충돌 여부
	float coeffRestitution = 0.5f;	// 비탄성 충돌 시 반발 계수
	bool bUniversalGravity = false;
	
	/* 입학 시험 : 구 생성 관련 버텍스 버퍼, 이중 포인터 */
	UBall* sphere = new UBall;
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));
	UBall::pVertexBuffer = vertexBufferSphere;
	BallList[0] = sphere;

	const int targetFPS = 60;							// 초당 최대 30번 갱신 (targetFPS를 높일수록, 공의 속도가 빨라진다)
	const double targetFrameTime = 1000.0f / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)
	const float deltaTime = 1000.0f / targetFPS;

	// 고성능 타이머 초기화
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;

	bool bIsExit = false;

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false) {
		QueryPerformanceCounter(&startTime);
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
			} else if (msg.message == WM_KEYDOWN) { // 조건이 많을수록 점프 테이블 생성하여 성능 최적화 하는 switch-case가 유리 (가급적 순차적인 감소/증가 순으로)
				switch (msg.wParam) {
				case VK_LEFT:	BallList[0]->Location.x -= 0.1f; break;
				case VK_UP:		BallList[0]->Location.y += 0.1f; break;
				case VK_RIGHT:	BallList[0]->Location.x += 0.1f; break;
				case VK_DOWN:	BallList[0]->Location.y -= 0.1f; break;
				}				
				// 디버깅용 : 1, 2번째 공 움직이기
				if (GetAsyncKeyState(0x57) & 0x8000) { // W 키 : 1번째 공 상 
					BallList[1]->Location.y += 0.1f;
				}
				if (GetAsyncKeyState(0x53) & 0x8000) { // S 키 : 1번째 공 하
					BallList[1]->Location.y -= 0.1f;
				}
				if (GetAsyncKeyState(0x49) & 0x8000) { // I 키 : 2번쨰 공 상
					BallList[2]->Location.y += 0.1f;
				}
				if (GetAsyncKeyState(0x4A) & 0x8000) { // J 키 : 2번째 공 하
					BallList[2]->Location.y -= 0.1f;
				}
			} 
		}
			
		////////////////////////////////////////////
		// 매번 실행되는 코드를 여기에 추가
		
		// [1] 렌더러 준비 작업
		renderer.Prepare();
		renderer.PrepareShader();
		
#pragma region Ball_Render_Update
		/* Ball들을 갱신, 렌더, 충돌 처리 */
		for (UINT i{ 0 }; i < numBalls; ++i) {
			UBall*& pBall = BallList[i];
			pBall->Update(bGravity, deltaTime);
			pBall->DoRender(renderer);
		}
		for (UINT i{ 0 }; i < numBalls - 1; ++i) {
			UBall*& pBall1 = BallList[i];
			for (UINT j {i+1}; j < numBalls; ++j) {
				UBall*& pBall2 = BallList[j];
				if (IsCollisionBall(pBall1, pBall2)) {
					pBall1->ResolveCollisionBall(pBall2, bInElastic, coeffRestitution);
				}
				if (bUniversalGravity) {
					pBall1->ResolveGravityBall(pBall2, deltaTime);
				}
			}
		}
#pragma endregion 

#pragma region ImGui
		// [2] ImGui 렌더링 준비, 컨트롤 설정, 렌더링 요청
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// [2] 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 Render() 사이에 위치
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle Contest!");
		ImGui::Checkbox("Gravity", &bGravity);
		if (ImGui::InputInt("Number of Balls", &inputNumBalls, 1, 1, 0)) {
			if (inputNumBalls < 1) inputNumBalls = 1;
			int nxtSize = (numBalls < inputNumBalls) ? inputNumBalls : numBalls;
			UBall** nxtBallList = nullptr;

			// 비교적 랜덤 삭제 cost가 작으므로 배열 사용
			if (numBalls < inputNumBalls) {			// 현재 공보다 입력 개수 클 경우 생성 및 동적 할당
				nxtBallList = new UBall* [nxtSize];
				memcpy(nxtBallList, BallList, sizeof(UBall*) * numBalls);
				for (UINT i{ numBalls }; i < inputNumBalls; ++i) {
					nxtBallList[i] = new UBall;
				}
				delete[] BallList;
				BallList = nxtBallList;
				numBalls = inputNumBalls;
			} 
			else if (numBalls > inputNumBalls) {	// 현재 공보다 입력 개수 작을 경우 랜덤 삭제, 개수가 같을 경우 삭제하지 않는다.
				nxtBallList = new UBall*[nxtSize];
				int deleteCnt = numBalls - inputNumBalls;
				while (deleteCnt) {
					int i = rand() % numBalls;
					if (BallList[i] == nullptr) continue;
					
					delete BallList[i];
					BallList[i] = nullptr;
					--deleteCnt;
				}
				UINT idx = 0;
				for (UINT i{ 0 }; i < numBalls; ++i) {
					if (BallList[i] == nullptr) continue;
					nxtBallList[idx++] = BallList[i];
				}
				delete[] BallList;
				BallList = nxtBallList;
				numBalls = inputNumBalls;
			}
			
		}
		
		// 비탄성 충돌 옵션 (반발 계수와 그룹)
		ImGui::Checkbox("InElastic Collision?", &bInElastic); 
		if (bInElastic) {
			ImGui::BeginGroup();
			ImGui::SliderFloat("COR e", &coeffRestitution, 0.0f, 1.0f, "%.3f");
			ImGui::EndGroup();
		}
		ImGui::Checkbox("Universal Gravity", &bUniversalGravity); // 만유인력 중력 옵션

		// 윈도우 크기 변경 (오류, TODO)
		ImGui::InputFloat2("Change Window Size", &wndSize.x, "%.0f");
		if (ImGui::Button("Apply size Change")) {
			RECT rect = { 0, 0, static_cast<LONG>(wndSize.x), static_cast<LONG>(wndSize.y) };
			AdjustWindowRect(&rect, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW, FALSE);
			SetWindowPos(hWnd, NULL, CW_USEDEFAULT, CW_USEDEFAULT, (int)wndSize.x, (int)wndSize.y, SWP_NOMOVE | SWP_NOZORDER);
			renderer.ResizeSwapChain(static_cast<int>(wndSize.x), static_cast<int>(wndSize.y));

			// ImGui 충돌범위도 변경 위해 디스플레이 크기와 프레임버퍼 크기 설정 / ImGui 소멸
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			
			// 새로운 컨텍스트 생성, 새로운 hWnd로 초기화
			ImGui::CreateContext();
			ImGui_ImplWin32_Init((void*)hWnd);
			ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);
			
			aspectRatio = wndSize.x / wndSize.y;

			// 해상도 변경에 따른 구 찌그러짐 해결 			
			for (UINT i{ 0 }; i < UBall::numVertices; ++i) {
				nxtsphere_vertices[i].x = (aspectRatio > 1.0f) ? sphere_vertices[i].x / aspectRatio : sphere_vertices[i].x;
				nxtsphere_vertices[i].y = (aspectRatio < 1.0f) ? sphere_vertices[i].y * aspectRatio : sphere_vertices[i].y;
			}
			ID3D11Buffer* nxtVertexBufferSphere = renderer.CreateVertexBuffer(nxtsphere_vertices, UBall::numVertices);
			UBall::pVertexBuffer = nxtVertexBufferSphere;
			SAFE_RELEASE(vertexBufferSphere);
			vertexBufferSphere = nxtVertexBufferSphere;
		} else {
			ImGui::End();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // 렌더링 요청
		}
		
#pragma endregion 
		// [3] 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환
		renderer.SwapBuffer();
		do {
			Sleep(0);

			QueryPerformanceCounter(&endTime);

			// 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환) = (클럭 주기 차이) * 1000.0f / 초당 클럭 횟수 = c * 1000.0f / (c/s) = (ms)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0f / frequency.QuadPart; // .QuadPart(공용체의 64비트 정수 멤버변수)
		} while (elapsedTime < targetFrameTime);
		
	}

#pragma region Release
	for (UINT i{ 0 }; i < numBalls; ++i) {
		delete BallList[i]; BallList[i] = nullptr;
	}
	delete[] BallList; BallList = nullptr;
	delete nxtsphere_vertices; nxtsphere_vertices = nullptr;
	// ImGui 소멸
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// COM 오브젝트는 Release로만 각각 소멸 가능 (delete 불가능)
	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();   // 렌더러 소멸 직전 셰이더를 소멸시키는 함수 호출
	renderer.Release();			// D3D11 소멸시키는 함수를 호출합니다.
#pragma endregion

	return 1;  
}