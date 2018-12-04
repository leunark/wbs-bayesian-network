#pragma once
#include "Node.h"

class State
{
public:
	State(std::string name, int pos, Node* node);
	~State();
	std::string name;
	int pos;
	Node* node;
};

