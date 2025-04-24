#pragma once
#include "Component.h"

class TransformComponent : public Component{
private:
	DirectX::XMFLOAT3 _position;
	DirectX::XMFLOAT3 _scale;

public:
	TransformComponent(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, std::shared_ptr<Entity> owner = nullptr);
	virtual ~TransformComponent();
	TransformComponent(const TransformComponent&);

	nodconst DirectX::XMFLOAT3 Position() const { return _position; }
	void SetPosition(DirectX::XMFLOAT3 newPosition) { _position = newPosition; }
	nodconst DirectX::XMFLOAT3 Scale() const { return _scale; }

	void Start() override;
	void Update(const SK::DOUBLE dT) override;
	void End() override;
	void OnMessage(const std::shared_ptr<Message> msg) override;
};