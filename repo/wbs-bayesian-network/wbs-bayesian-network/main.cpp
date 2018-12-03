#include <iostream>
#include <fstream>
#include <istream>
#include <vector>
#include <string>
#include <sstream>
#include <sqlite3.h>
#include <dlib/bayes_utils.h>
#include <dlib/graph_utils.h>
#include <dlib/graph.h>
#include <dlib/directed_graph.h>
#include <Node.h>

using namespace dlib;
using namespace std;

int main() {
	// Setup database
	sqlite3 *db; // DB pointer
	char *zErrMsg = 0; // Error message
	std::vector<std::string> statements;
	std::string pSQL; // Sql statement
	int rc; // Connection

	rc = sqlite3_open("programmentwurf.db", &db);

	if (rc)
	{
		std::cout << "Can't open database: " << sqlite3_errmsg(db) << "\n";
	}
	else
	{
		std::cout << "Open database successfully\n\n";
	}

	// Open file
	std::ifstream file("P001.csv");
	if (!file.is_open()) {
		std::cout << "ERROR: File Open" << std::endl;
	}

	// Read header first line and create create-statement
	std::string line = "";
	std::getline(file, line);
	std::istringstream iss(line);
	std::string token;
	pSQL = "create table data (";
	while (std::getline(iss, token, ';')) {
		pSQL.append(token);
		pSQL.append(" varchar(30),");
	}
	pSQL = pSQL.substr(0, pSQL.size() - 1);
	pSQL.append(")");
	statements.push_back(pSQL);

	// Read every line and create insert-statements
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string token;
		pSQL = "insert into data values (";
		while (std::getline(iss, token, ';')) {
			pSQL.append("'" + token + "',");
		}
		pSQL = pSQL.substr(0, pSQL.size() - 1);
		pSQL.append(")");
		statements.push_back(pSQL);
	}

	// Add delete- and drop-statement
	statements.push_back("delete from data");
	statements.push_back("drop table data");

	// Execute statement list
	for (int i = 0; i < statements.size(); i++)
	{
		rc = sqlite3_exec(db, statements[i].c_str(), 0, 0, &zErrMsg);
		if (rc != SQLITE_OK)
		{
			std::cout << "SQL error: " << sqlite3_errmsg(db) << std::endl;
			sqlite3_free(zErrMsg);
			break;
		}
		std::cout << "sql statement \"" << statements[i] << "\" executed successfully" << std::endl;
	}
	file.close();
	
	// Bayesian network implementation
	try
	{
		using namespace bayes_node_utils;
		
		// Declare bayesian network bn
		directed_graph<bayes_node>::kernel_1a_c bn;

		// Use an enum to make some more readable names for our nodes
		enum NodeEnum
		{
			Altersgruppe = 0,
			Verheiratet = 1,
			Kinderzahl = 2,
			Abschluss = 3,
			Geschlecht = 4,
			Beruf = 5,
			Familieneinkommen = 6,
			Buch = 7,
			LAST = 8
		};

		/*
		// Setup our bayesian network
		bn.set_number_of_nodes(8);
		bn.add_edge(Altersgruppe, Verheiratet);
		bn.add_edge(Altersgruppe, Kinderzahl);
		bn.add_edge(Verheiratet, Buch);
		bn.add_edge(Kinderzahl, Buch);
		bn.add_edge(Abschluss, Beruf);
		bn.add_edge(Geschlecht, Beruf);
		bn.add_edge(Beruf, Familieneinkommen);
		bn.add_edge(Familieneinkommen, Buch);

		// Setup categories for each node
		set_node_num_values(bn, Altersgruppe, 6);
		set_node_num_values(bn, Verheiratet, 2);
		set_node_num_values(bn, Kinderzahl, 5);
		set_node_num_values(bn, Abschluss, 6);
		set_node_num_values(bn, Geschlecht, 2);
		set_node_num_values(bn, Beruf, 8);
		set_node_num_values(bn, Familieneinkommen, 6);
		set_node_num_values(bn, Buch, 3);

		// Add conditional probability information for each node with the assignment 
		// object that allows us to specify the state of each nodes parents. 
		assignment parent_state;

		// Workflow
		// 0. Loop through all nodes
		// 1. Clear parent state: parent_state.clear();
		// 2. Add parents of node: parent_state.add(X, 1);
		// 3. Set node probability of all combinations
		
		for (int i = 0; i < LAST; i++) 
		{
			Node n = static_cast<Node>(i);
		}

		// parent_state is empty in this case since Altersgruppe is a root node
		set_node_probability(bn, Altersgruppe, 0, parent_state, 0.01);
		set_node_probability(bn, Altersgruppe, 1, parent_state, 1 - 0.01);
		*/

		// Create all nodes
		std::vector<Node*> nodes = {
		new Node(Altersgruppe, "Altersgruppe", {"u18","v19b24","v25b35","v36b49","v50b65","ue65"}),
		new Node(Verheiratet, "Verheiratet", { "nein", "ja" }),
		new Node(Kinderzahl, "Kinderzahl", { "a0","a1","a2","a3","a4" }),
		new Node(Geschlecht, "Geschlecht", {"m","w"}),
		new Node(Abschluss, "Abschluss", {"keiner","Hauptschule","Realschule","Gymnasium","Hochschule","Promotion"}),
		new Node(Beruf, "Beruf", {"Angestellter","Arbeiter","Arbeitslos","Fuehrungskraft","Hausfrau","Lehrer","Rentner","Selbstaendig"}),
		new Node(Familieneinkommen, "Familieneinkommen", { "u1000","v2000b2999","v3000b3999","v4000b4999","ue5000" }),
		new Node(Buch, "Buch", {"Buch A", "Buch B", "Buch C"})
		};

		// Create edges
		nodes[Verheiratet]->parents = { nodes[Altersgruppe] };
		nodes[Kinderzahl]->parents = { nodes[Altersgruppe] };
		nodes[Beruf]->parents = { nodes[Abschluss], nodes[Geschlecht] };
		nodes[Familieneinkommen]->parents = { nodes[Beruf] };
		nodes[Buch]->parents = { nodes[Verheiratet], nodes[Kinderzahl], nodes[Familieneinkommen] };

		// Transfer model logic
		bn.set_number_of_nodes(nodes.size());
		bn.add_edge(Altersgruppe, Verheiratet);
		for (int i = 0; i < nodes.size(); i++) {
			for (int j = 0; j < nodes[i]->parents.size(); j++) {
				bn.add_edge(nodes[i]->value, nodes[i]->parents[j]->value);
			}
		}

		// Generate probabilities for each node
		for (int i = 0; i < nodes.size(); i++) {
			std::cout << "Node: " << nodes[i]->name << endl << endl;
			nodes[i]->printCPT();
		}

	}
	catch (std::exception& e)
	{
		cout << "exception thrown: " << endl;
		cout << e.what() << endl;
		cout << "hit enter to terminate" << endl;
		cin.get();
	}

	sqlite3_close(db);
	system("pause");
}