///////////////////////TODO : SWapchain 정보 변경
#include <windows.h>

// ImGui 관련 헤더
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include <iostream>
#include <algorithm> // clamp

#include "URenderer.h"
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

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam); // 함수는 선언 시 자동으로 extern이 붙음(안붙여도된다는 뜻) / 사용자 정의 함수여서 명시적으로 어디에 있는지 알아야함 -> extern 사용 (<->scanf는 이미 컴파일된 라이브러리 O)

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

	FVector2 wndSize = { 600, 600 };
	float aspectRatio = wndSize.x / wndSize.y;
	// 1024 x 1024 크기에 윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, wndSize.x, wndSize.y,
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
	// [복습] (numvertices는 다른 구조체를 가질 때 코드가 변경되는데, 이를 매크로로 자동으로 추가할 방법 + 소유권 누구한테 줘야 적당할지
	UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	UINT numVerticesPlayer = sizeof(player1_vertices) / sizeof(FVertexSimple);

	std::unique_ptr<FVertexSimple[]> nxtsphere_vertices{ new FVertexSimple[numVerticesSphere] }; // 해상도에 따라 변경될 구 정점 저장할 배열

	// Vertex Buffer로 넘기기 전에 Scale Down함
	float scaleMod = 0.1f;
	for (UINT i = 0; i < numVerticesSphere; ++i) {
		sphere_vertices[i].x *= scaleMod;
		sphere_vertices[i].y *= scaleMod;
		sphere_vertices[i].z *= scaleMod;
	}
	std::copy(sphere_vertices, sphere_vertices+numVerticesSphere, nxtsphere_vertices.get());

	ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	// 공1개, 패들 2개 정의
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

	const int targetFPS = 30;							// 초당 최대 30번 갱신 (targetFPS를 높일수록, ballSpeed도 빨라진다)
	const double targetFrameTime = 1000.0f / targetFPS; // 한 프레임의 목표 시간 (밀리초 단위)

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
				if (!bPinballMovement) {
					switch (msg.wParam) {
					case VK_LEFT:	objects[0].offset.x -= 0.1f; break;
					case VK_UP:		objects[0].offset.y += 0.1f; break;
					case VK_RIGHT:	objects[0].offset.x += 0.1f; break;
					case VK_DOWN:	objects[0].offset.y -= 0.1f; break;
					}
				}
				if (GetAsyncKeyState(0x57) & 0x8000) { // W 키 : Player1 상
					objects[1].offset.y += 0.1f;
				}
				if (GetAsyncKeyState(0x53) & 0x8000) { // S 키 : Player1 하
					objects[1].offset.y -= 0.1f;
				}
				if (GetAsyncKeyState(0x49) & 0x8000) { // I 키 : Player2 상
					objects[2].offset.y += 0.1f;
				}
				if (GetAsyncKeyState(0x4A) & 0x8000) { // J 키 : Player2 하
					objects[2].offset.y -= 0.1f;
				}
			} 
		}
		// 키보드 처리 직후 화면 밖을 벗어났다면 화면 안쪽으로 재위치시킨다. (화면 벗어나지 않는 옵션이라면)
		if (bBoundBallToScreen) {
			float renderRadius = sphereRadius * scaleMod;
			auto& offset = objects[0].offset;
			renderRadius *= (aspectRatio > 1.0f) ?  (1.0f / (aspectRatio)) : aspectRatio;			// 창크기에 따른 왜곡 보정
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
			// 속도를 공위치에 더해 공을 실질적으로 움직임
			UObject& sphere = objects[0];
			auto& offset = sphere.offset;
			auto& velocity = sphere.velocity;
			offset.x += velocity.x;
			offset.y += velocity.y;
			offset.z += velocity.z;

			// 벽과 충돌 여부를 체크하고 충돌시 속도에 음수를 곱해 방향을 바꿈
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
		// 매번 실행되는 코드를 여기에 추가
		
		// [1] 렌더러 준비 작업
		renderer.Prepare();
		renderer.PrepareShader();

		for (size_t i = 0; i < numObjects; ++i) {
			auto& object = objects[i];
			renderer.UpdateConstant(object.offset);
			renderer.DeviceContext->VSSetConstantBuffers(0, 1, &renderer.ConstantBuffer);
			renderer.RenderPrimitive(vertexbuffers[i], numVertices[i]);
		}

		// [2] ImGui 렌더링 준비, 컨트롤 설정, 렌더링 요청
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// [2] 이후 ImGui UI 컨트롤 추가는 ImGui::NewFrame()과 Render() 사이인 여기에 위치합니다
		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle world!");
		ImGui::Checkbox("Bound Ball to Screen", &bBoundBallToScreen);
		ImGui::Checkbox("Bound Paddle to Screen", &bBoundPaddleToScreen);
		ImGui::Checkbox("Pinball Movement", &bPinballMovement);
		ImGui::InputFloat2("Change Window Size", &wndSize.x, "%.0f");

		if (ImGui::Button("Apply size Change")) {
			// 윈도우 크기 변경
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
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // 렌더링 요청
		}
		// [3] 현재 화면에 보여지는 버퍼와 그리기 작업을 위한 버퍼를 서로 교환
		renderer.SwapBuffer();

		do {
			Sleep(0);

			QueryPerformanceCounter(&endTime);

			// 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환) = (클럭 주기 차이) * 1000.0f / 초당 클럭 횟수 = c * 1000.0f / (c/s) = (ms)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0f / frequency.QuadPart; // .QuadPart(공용체의 64비트 정수 멤버변수)
		} while (elapsedTime < targetFrameTime);
		
	}

	delete[] objects;
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