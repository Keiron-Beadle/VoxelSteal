#if BUILD_DX
#include "../Headers/VBO_DX.h"
#include "../Headers/Renderer_DX.h"

VBO_DX::VBO_DX() {

}

VBO_DX::~VBO_DX() {

}

void VBO_DX::Create(std::shared_ptr<Renderer> r, Vertex vertices[], SK::INT32 numVertices) {
	auto* renderer = (Renderer_DX*)r.get();
	_numVertices = numVertices;
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * numVertices;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	renderer->Device()->CreateBuffer(&bd, nullptr, &_gVertices.p);

	D3D11_MAPPED_SUBRESOURCE ms{};
	renderer->Context()->Map(_gVertices.p, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	SK::INT32 bb = sizeof(Vertex) * numVertices;
	memcpy(ms.pData, vertices, sizeof(Vertex) * numVertices);
	renderer->Context()->Unmap(_gVertices.p, 0);
}

void VBO_DX::Draw(Renderer* r)
{
	auto* renderer = dynamic_cast<Renderer_DX*>(r);
	//UINT stride = sizeof(Vertex);
	//UINT offset = 0;

	//ID3D11Buffer* mb[1] = { _gVertices.p };
	//auto& context = renderer->Context();

	//context->IASetVertexBuffers(0, 1, mb, &stride, &offset);
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	renderer->Context()->Draw(_numVertices, 0);
}

#endif