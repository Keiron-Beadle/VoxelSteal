#pragma once
#include "gil.h"

class Observer {
public:
	virtual ~Observer() {}
	virtual void OnMessage(const std::shared_ptr<Message> msg) = 0;
};

template <typename Payload>
class ObserverImpl : public Observer
{
public:
	virtual void OnMessage(const std::shared_ptr<Message> msg) {
		std::shared_ptr<MessageImpl<Payload>> castedMsg = std::dynamic_pointer_cast<MessageImpl<Payload>>(msg);
		if (castedMsg) {
			ProcessMessage(castedMsg->GetPayload());
		}
	}
private:
	virtual void ProcessMessage(const Payload& payload) = 0;
};


