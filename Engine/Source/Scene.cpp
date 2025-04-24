#include "../Headers/Scene.h"
#include "../Headers/Entity.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::OnMessage(std::shared_ptr<Message> msg)
{
}

void Scene::Update(const SK::DOUBLE dT)
{
	//for (auto i = 0; i < _entities.size(); ++i){
	//	if (_entities[i]->DeleteFlag()) {
	//		_entities.erase(_entities.begin() + i);
	//		--i;
	//	}
	//}
}
