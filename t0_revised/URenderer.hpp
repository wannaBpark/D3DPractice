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
		return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x, };
	}
	float length() const {
		return sqrt(x * x + y * y + z * z);
	}

	// �ε帮�Խ� ȸ�� ����  vnxt = vcos + (k x v) * s + k * (k * v) ( 1 - cos)
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
		float Radius;
	};

	ID3D11Buffer* ConstantBuffer = nullptr; // ���̴��� �����͸� �����ϱ� ���� ��� ���� (����, �� ������ ��� ���� ���� �ٲ㰡�� ���)

	struct FInstances { // ���� ���ؽ� ���۸� �����ϸ鼭, �ν��Ͻ� ���� �޸��� �ڷ�� (x,y,z�� offset, scale�� radius)
		float x, y, z;
		float scale;
	};
	ID3D11Buffer* InstanceBuffer = nullptr;
	unsigned int m_instanceCount = 1;

public:
	// ������ �ʱ�ȭ �Լ�
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);	// Direct3D ��ġ �� ����ü�� 
		CreateFrameBuffer();				// ������ ���� ����
		CreateRasterizerState();			// �����Ͷ����� ���� ����
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
		SwapChain->GetBuffer(0, IID_PPV_ARGS(&FrameBuffer)); // Ȯ�� : [__uuidof(ID3D11Texture2D), (void**)&FrameBuffer], IUnknown ��� üũ����

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
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	 0, 0,  D3D11_INPUT_PER_VERTEX_DATA,	0},
			{ "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA,	0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA,	1}, // instance ���� �޸� �� �ڷ� (���������� : �ν��Ͻ� ������ ������)
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

	void RenderPrimitiveInstance(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		unsigned int strides[2];
		unsigned int offsets[2] = { 0,0 };
		ID3D11Buffer* bufferPointers[2] = { pBuffer, InstanceBuffer };

		strides[0] = sizeof(FVertexSimple);
		strides[1] = sizeof(FInstances);

		DeviceContext->IASetVertexBuffers(0, 2, bufferPointers, strides, offsets);
		DeviceContext->DrawInstanced(numVertices, m_instanceCount, 0, 0);
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
	void UpdateConstant(FVector3 Offset, float radius)
	{
		if (ConstantBuffer) {
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;			// CPU���� �����ϵ��� ������۸� �ӽ÷� ������ ����

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // �������� ��� ���� ������Ʈ (���ν� CPU�� ������ ���� ����)
			FConstants* constants = (FConstants*)constantbufferMSR.pData; { // ���ε� ������ �ý��� �޸� �ּ� ������ | �ش� �ý��� �޸𸮸� ���� ����
				constants->Offset = Offset;
				constants->Radius = radius;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);		// ���ҽ��� GPU���� �������� ���·� �ǵ��� (GPU�� ���� �б� ����)
		}
	}

	void CheckInstanceCountChanged(int newSize)
	{
		if (m_instanceCount == newSize) {
			return;
		}
		ReleaseInstanceBuffer();
		m_instanceCount = newSize;
		
		D3D11_BUFFER_DESC instancebufferdesc = {};
		instancebufferdesc.ByteWidth = sizeof(FInstances) * m_instanceCount;
		instancebufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame (�� ��ü���� �������� ���� ����)
		instancebufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		instancebufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		Device->CreateBuffer(&instancebufferdesc, nullptr, &InstanceBuffer);
	}

	void CreateInstanceBuffer()
	{
		D3D11_BUFFER_DESC instancebufferdesc = {};
		instancebufferdesc.ByteWidth = sizeof(FInstances) * m_instanceCount;
		instancebufferdesc.Usage = D3D11_USAGE_DYNAMIC; // will be updated from CPU frame (�� ��ü���� �������� ���� ����)
		instancebufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		instancebufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		Device->CreateBuffer(&instancebufferdesc, nullptr, &InstanceBuffer);
	}

	void UpdateInstanceAll(FVector3 Offset, float Radius, UINT index)
	{
		if (InstanceBuffer) {
			D3D11_MAPPED_SUBRESOURCE instancebufferMSR;			

			DeviceContext->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instancebufferMSR);
			FInstances* instances = (FInstances*)instancebufferMSR.pData; {							// ���ε� ������ �ý��� �޸� �ּ� ������ | �ش� �ý��� �޸𸮸� ���� ����
				
				instances[index].x = Offset.x;
				instances[index].y = Offset.y;
				instances[index].z = Offset.z;
				instances[index].scale = Radius;
			}
			DeviceContext->Unmap(InstanceBuffer, 0);		// ���ҽ��� GPU���� �������� ���·� �ǵ��� (GPU�� ���� �б� ����)
		}
	}
	void ReleaseInstanceBuffer()
	{
		SAFE_RELEASE(InstanceBuffer);
	}
};

#pragma endregion