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

static int callback(void*, int, char**, char**);

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

	// Delete first table
	statements.push_back("drop table if exists data");

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
	//statements.push_back("delete from data");
	//statements.push_back("drop table data");

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

		// Create all nodes
		std::vector<Node*> nodes = {
		new Node(Altersgruppe, "Altersgruppe", {"<18","19-24","25-35","36-49","50-65",">65"}),
		new Node(Verheiratet, "Verheiratet", { "nein", "ja" }),
		new Node(Kinderzahl, "Kinderzahl", { "0","1","2","3","4" }),
		new Node(Geschlecht, "Geschlecht", {"m","w"}),
		new Node(Abschluss, "Abschluss", {"keiner","Hauptschule","Realschule","Gymnasium","Hochschule","Promotion"}),
		new Node(Beruf, "Beruf", {"Angestellter","Arbeiter","Arbeitslos","Fuehrungskraft","Hausfrau","Lehrer","Rentner","Selbstaendig"}),
		new Node(Familieneinkommen, "Familieneinkommen", { "<1000","2000-2999","3000-3999","4000-4999","5000 und mehr" }),
		new Node(Buch, "Buch", {"Buch_A", "Buch_B", "Buch_C"})
		};

		// Create edges
		nodes[Verheiratet]->parents = { nodes[Altersgruppe] };
		nodes[Kinderzahl]->parents = { nodes[Altersgruppe] };
		nodes[Beruf]->parents = { nodes[Abschluss], nodes[Geschlecht] };
		nodes[Familieneinkommen]->parents = { nodes[Beruf] };
		nodes[Buch]->parents = { nodes[Verheiratet], nodes[Kinderzahl], nodes[Familieneinkommen] };

		// Transfer model logic
		bn.set_number_of_nodes(nodes.size());
		for (int i = 0; i < nodes.size(); i++) {
			// Set edges with parents
			for (int j = 0; j < nodes[i]->parents.size(); j++) {
				bn.add_edge(nodes[i]->value, nodes[i]->parents[j]->value);
			}
			// Set number of states
			set_node_num_values(bn, nodes[i]->value, nodes[i]->states.size());
		}

		// Add conditional probability information for each node with the assignment 
		// object that allows us to specify the state of each nodes parents 
		assignment parent_state;
		
		// Process every node
		for (int i = 0; i < nodes.size(); i++) {
			auto cpt = *(nodes[i]->startGeneratingCPT());

			// Process every combo
			for (int j = 0; j < cpt.size(); j++) {
				// Build statement for each combo
				std::string statement = "";
				std::string statementa = "select 1.00 * (";
				std::string statementb = "select count(*) from data where ";
				std:string statementc = "";
				if (cpt[j].size() > 1) {
					for (int k = 0; k < cpt[j].size() - 1; k++) {
						statementb += nodes[i]->parents[k]->name + " = '" + cpt[j][k] + "' and ";
					}
					statementc = statementb.substr(0, statementb.size() - 4);
				}
				else {
					statementc = statementb.substr(0, statementb.size() - 6);
				}
				statementb += nodes[i]->name + " = '" + cpt[j][cpt[j].size()-1] +"'";
				statement = statementa + statementb + ") / (" + statementc + ") as val";

				rc = sqlite3_exec(db,statement.c_str(),callback,0,&zErrMsg);

				if (rc != SQLITE_OK)
				{
					std::cout << "ERROR: " << sqlite3_errmsg(db) << std::endl;
					sqlite3_free(zErrMsg);
				}
			}
		}

		// Add delete- and drop-statement
		statements.clear();
		statements.push_back("delete from data");
		statements.push_back("drop table data");

		// Execute statement list
		for (int i = 0; i < statements.size(); i++)
		{
			rc = sqlite3_exec(db, statements[i].c_str(), 0, 0, &zErrMsg);
			if (rc != SQLITE_OK)
			{
				std::cout << std::endl << "ERROR: " << sqlite3_errmsg(db) << std::endl;
				sqlite3_free(zErrMsg);
				break;
			}
			std::cout << std::endl << "SUCCESS: " << statements[i] << std::endl;
		}

		sqlite3_close(db);
	}
	catch (std::exception& e)
	{
		cout << "exception thrown: " << endl;
		cout << e.what() << endl;
		cout << "hit enter to terminate" << endl;
		cin.get();
	}

	system("pause");
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for (i = 0; i < argc; i++)
	{
		// If argv[i] is empty, there was a division with 0 most likely because of no occurrences in 
		// the select statement
		cout << azColName[i] << " => " << (argv[i] ? argv[i] : "0") << std::endl;
	}
	cout << "\n";
	return 0;
}