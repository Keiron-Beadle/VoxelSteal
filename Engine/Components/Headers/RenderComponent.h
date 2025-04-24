#pragma once
#include "Component.h"
#include "../../Headers/Mesh.h"

class RenderComponent : public Component {
protected:
	std::shared_ptr<Mesh> _mesh;
	DirectX::XMFLOAT4 _colour;

public:
	RenderComponent(std::shared_ptr<Entity> owner);
	virtual ~RenderComponent();
	RenderComponent(const RenderComponent& rc);
	
	nodconst std::shared_ptr<Mesh> GetMesh() const { return _mesh; }
	void SetMesh(std::shared_ptr<Mesh> m) { _mesh = m; }
	
	nodconst DirectX::XMFLOAT4& Colour() const { return _colour; }
	void SetColour(DirectX::XMFLOAT4 c) { _colour = c; }
	void AddColour(DirectX::XMFLOAT4 c);
	void SubColour(DirectX::XMFLOAT4 c);

	void Start() override;
	void Update(const SK::DOUBLE dT) override;
	void End() override;
	void OnMessage(const std::shared_ptr<Message> msg) override;

};