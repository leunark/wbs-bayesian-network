#pragma once

#include <string>
#include <vector>

class Node
{
public:
	struct State {
		std::string name;
		int pos;
		Node* node;
	};
	Node();
	Node(int value, std::string name, std::vector<std::string> states = {}, std::vector<Node*> parents = {});
	~Node();
	std::vector<std::string> states;
	std::vector<Node*> parents;
	std::string name;
	int id;
	std::vector<std::vector<State>>* startGeneratingCPT();
	void printCPT();
	std::string getNameOfState(int pos) const;
private:
	void generateCPT(std::vector<Node*> list, std::vector<std::vector<State>>* result, int depth, std::vector<State> combo);
};

