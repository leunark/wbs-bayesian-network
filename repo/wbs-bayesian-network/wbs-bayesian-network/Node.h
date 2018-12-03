#pragma once

#include <string>
#include <vector>

class Node
{
public:
	Node();
	Node(int value, std::string name, std::vector<std::string> states = {}, std::vector<Node*> parents = {});
	~Node();
	std::vector<std::string> states;
	std::vector<Node*> parents;
	std::string name;
	int value;
	std::vector<std::vector<std::string>>* startGeneratingCPT();
	void printCPT();
private:
	void generateCPT(std::vector<Node*> list, std::vector<std::vector<std::string>>* result, int depth, std::vector<std::string> combo);
};

