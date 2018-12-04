#include "State.h"

State::State(std::string name, int pos, Node* node) 
{
	this->name = name;
	this->pos = pos;
	this->node = node;
}

State::~State()
{
}
