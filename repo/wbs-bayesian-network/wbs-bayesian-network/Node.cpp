#include "Node.h"
#include <iostream>

Node::Node() {
	this->value = -1;
	this->name = "";
	this->states = {};
	this->parents = {};
}

Node::Node(int value, std::string name, std::vector<std::string> states, std::vector<Node*> parents) {
	this->value = value;
	this->name = name;
	this->states = states;
	this->parents = parents;
}

Node::~Node() {

}

std::vector<std::vector<std::string>>* Node::startGeneratingCPT() {
	std::vector<Node*> list = Node::parents;
	list.push_back(this);
	std::vector<std::vector<std::string>>* result = new std::vector<std::vector<std::string>>();
	*result = {};
	generateCPT(list, result, 0, {});
	return result;
}

void Node::generateCPT(std::vector<Node*> list, std::vector<std::vector<std::string>>* result, int depth, std::vector<std::string> combo) {
	if (depth >= list.size()) {
		for (int i = 0; i < combo.size(); i++) {
			result->push_back(combo);
		}
	}
	else {
		for (int i = 0; i < list[depth]->states.size(); i++)
		{
			std::vector<std::string> tmp = combo;
			tmp.push_back(list[depth]->states[i]);
			generateCPT(list, result, depth + 1, tmp);
		}
	}
}

void Node::printCPT() {
	auto cpt = *startGeneratingCPT();
	for (int j = 0; j < cpt.size(); j++) {
		for (int k = 0; k < cpt[j].size(); k++) {
			std::cout << cpt[j][k] << " ";
		}
		std::cout << std::endl;
	}
}