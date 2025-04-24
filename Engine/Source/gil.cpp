#include "../Headers/gil.h"

Addressee& operator |= (Addressee& lhs, Addressee rhs) {
	lhs = static_cast<Addressee>(static_cast<int>(lhs) | static_cast<int>(rhs));
	return lhs;
}