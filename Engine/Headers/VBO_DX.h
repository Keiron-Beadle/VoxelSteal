#pragma once
#if BUILD_DX
#include <d3d11.h>
#include <atlbase.h>
#include "VBO.h"

class VBO_DX : public VBO {
protected:
	CComPtr<ID3D11Buffer> _gVertices;
public:
	VBO_DX();
	virtual ~VBO_DX();

	nodconst CComPtr<ID3D11Buffer> GetVertices() const { return _gVertices; }
	nodconst D3D_PRIMITIVE_TOPOLOGY GetTopology() const { return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; }

	virtual void Create(std::shared_ptr<Renderer> renderer, Vertex vertices[], SK::INT32 numVertices);
	virtual void Draw(Renderer* renderer);


};

#endif