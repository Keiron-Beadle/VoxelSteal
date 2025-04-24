#pragma once
#include "gil.h"

class Message {
protected:
	Addressee _addressee;

public:
	Message(Addressee addressee) : _addressee(addressee) {}
	virtual ~Message() {}
	nodconst Addressee& Type() const { return _addressee; }
};


template <typename Payload>
class MessageImpl : public Message
{
private:
	Payload _payload;

public:
	MessageImpl(Addressee t, const Payload& payload)
		: Message{ t },
			_payload(payload) 
	{
	
	}

	nodconst Payload& GetPayload() const { return _payload; }
};