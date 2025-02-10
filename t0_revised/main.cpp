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
#include "URenderer.hpp"


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
	
	// 해당 오브젝트의 인스턴스 인덱스
	UINT index;

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
		//renderer.UpdateInstance(this->Location, this->Radius, this->index);
		//renderer.DeviceContext->VSSetConstantBuffers(0, 1, &renderer.ConstantBuffer);
		//renderer.RenderPrimitiveInstance(pVertexBuffer, numVertices);
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
	
	UBall() : Location(0.0f), Velocity(0.0f), Radius((float)(rand() % 3 + 1) * 0.04f), Mass(3.0f * Radius), AngularVelocity(0.0f), Polarity(rand() % RAND_MAX < 0.5f ? -1 : 1), index(0)
	{
		Location.x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) * 2 - 1;						// [-1, 1]
		Location.y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - Radius - 0.25f;			// [최소 화면 중간 높이, 맨 위]
		//Velocity.x = 0.00001f;																					// 미세하게 공이 구르도록
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
	renderer.CreateInstanceBuffer(); // 인스턴스 버퍼 생성

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
			//pBall->DoRender(renderer);
		}
		if (renderer.InstanceBuffer) {
			D3D11_MAPPED_SUBRESOURCE instancebufferMSR;

			renderer.DeviceContext->Map(renderer.InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instancebufferMSR);
			URenderer::FInstances* instances = (URenderer::FInstances*)instancebufferMSR.pData; {							// 맵핑된 버퍼의 시스템 메모리 주소 가져옴 | 해당 시스템 메모리를 수정 가능
				for (UINT i = 0; i < numBalls; ++i) {
					instances[i].x =	 BallList[i]->Location.x;
					instances[i].y =	 BallList[i]->Location.y;
					instances[i].z =	 BallList[i]->Location.z;
					instances[i].scale = BallList[i]->Radius;
				}
			}
			renderer.DeviceContext->Unmap(renderer.InstanceBuffer, 0);		// 리소스를 GPU와의 정상적인 상태로 되돌림 (GPU가 이제 읽기 가능)
		
		}
		renderer.RenderPrimitiveInstance(UBall::pVertexBuffer, UBall::numVertices);
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
					nxtBallList[i]->index = i;			 // 인스턴스 인덱스 초기화
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
					nxtBallList[idx - 1]->index = idx - 1;	// 인스턴스 인덱스 갱신
				}
				delete[] BallList;
				BallList = nxtBallList;
				numBalls = inputNumBalls;
			}
			
			renderer.CheckInstanceCountChanged(numBalls);		// 최종 적용된 인스턴스 개수를 체크하고, 현재 인스턴스 버퍼의 사이즈와 다르면 새로 갱신
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

	renderer.ReleaseInstanceBuffer();

	renderer.ReleaseShader();   // 렌더러 소멸 직전 셰이더를 소멸시키는 함수 호출
	renderer.Release();			// D3D11 소멸시키는 함수를 호출합니다.
#pragma endregion

	return 1;  
}