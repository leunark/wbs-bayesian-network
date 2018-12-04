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
#include <bayes_net_ex.h>

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
			Geschlecht = 3,
			Abschluss = 4,
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
		new Node(Familieneinkommen, "Familieneinkommen", { "<1000","1000-1999","2000-2999","3000-3999","4000-4999","5000 und mehr" }),
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
				bn.add_edge(nodes[i]->parents[j]->value, nodes[i]->value);
			}
			// Set number of states
			set_node_num_values(bn, nodes[i]->value, nodes[i]->states.size());
		}

		// Add conditional probability information for each node with the assignment 
		// object that allows us to specify the state of each nodes parents 
		assignment parent_state;
		
		// Checker
		double checksum = 0;

		// Process every node
		for (int i = 0; i < nodes.size(); i++) {
			if (i == 5) {
				std::cout << "\n break \n";
			}
			
			// Generate CPT
			auto cpt = *(nodes[i]->startGeneratingCPT());

			// Set parent state 
			parent_state.clear();
			for (int k = 0; k < nodes[i]->parents.size(); k++) {
				parent_state.add(nodes[i]->parents[k]->value,0);
			}

			// Process every combination
			for (int j = 0; j < cpt.size(); j++) {
				// Build statement for each combination
				std::string statement = "";
				std::string statementa = "select 1.00 * (";
				std::string statementb = "select count(*) from data where ";
				std::string statementc = "";
				if (cpt[j].size() > 1) {
					// Iterate over every state of the combination
					for (int k = 0; k < cpt[j].size() - 1; k++) {
						statementb += nodes[i]->parents[k]->name + " = '" + cpt[j][k].name + "' and "; // Add combo to statement
						std::cout << cpt[j][k].name << " "; // Log combination
						parent_state[cpt[j][k].node->value] = cpt[j][k].pos; // Set parent state
					}
					statementc = statementb.substr(0, statementb.size() - 4);
				}
				else {
					statementc = statementb.substr(0, statementb.size() - 6);
				}

				// Add last condition to statement, whereas last item will be the node itself!
				statementb += nodes[i]->name + " = '" + cpt[j][cpt[j].size()-1].name +"'";
				std::cout << cpt[j][cpt[j].size() - 1].name << " "; // Log combo

				// Statement to get number of occurrences of current combo
				statement = statementa + statementb + ") as z, (" + statementc + ") as n";
				double* p = new double();
				*p = -1;
				rc = sqlite3_exec(db,statement.c_str(),callback,p,&zErrMsg);
				if (rc != SQLITE_OK)
				{
					std::cout << "ERROR: " << sqlite3_errmsg(db) << std::endl;
					sqlite3_free(zErrMsg);
					break;
				}
				std::cout << endl;

				if (*p == -1) {
					*p = (double)1 / nodes[i]->states.size();
				}
				std::cout << " p=" << *p << std::endl;
				checksum += *p;

				// Print parent_state for debugging purposes
				for (int k = 0; k < cpt[j].size() - 1; k++) {
					std::cout << "Node" << cpt[j][k].node->value << "::State" << parent_state[cpt[j][k].node->value] << "; ";
				}
				std::cout << std::endl;

				// Set node probability  p(A=?| P1=?, P2=?, ...) = *p
				std::cout << "set_node_probability" << "(bn," << nodes[i]->value << "," << cpt[j][cpt[j].size() - 1].pos << "," << parent_state.size() << "," << (float)*p << ")" << std::endl;
				set_node_probability(bn, nodes[i]->value, cpt[j][cpt[j].size() - 1].pos, parent_state, *p);

				// Checker
				if (cpt[j][cpt[j].size() - 1].pos == cpt[j][cpt[j].size() - 1].node->states.size() - 1)
				{
					std::cout << "Checksum: " << checksum << "\n";
					checksum = 0;
				}
			}
		}

		// We have now finished setting up our bayesian network.  So let's compute some 
		// probability values.  The first thing we will do is compute the prior probability
		// of each node in the network.  To do this we will use the join tree algorithm which
		// is an algorithm for performing exact inference in a bayesian network.   

		// First we need to create an undirected graph which contains set objects at each node and
		// edge.  This long declaration does the trick.
		typedef dlib::set<unsigned long>::compare_1b_c set_type;
		typedef graph<set_type, set_type>::kernel_1a_c join_tree_type;
		join_tree_type join_tree;

		// Now we need to populate the join_tree with data from our bayesian network.  The next  
		// function calls do this.  Explaining exactly what they do is outside the scope of this
		// example.  Just think of them as filling join_tree with information that is useful 
		// later on for dealing with our bayesian network.  
		create_moral_graph(bn, join_tree);
		create_join_tree(join_tree, join_tree);

		// Now that we have a proper join_tree we can use it to obtain a solution to our
		// bayesian network.  Doing this is as simple as declaring an instance of
		// the bayesian_network_join_tree object as follows:
		bayesian_network_join_tree solution(bn, join_tree);

		// Now print out all the probabilities for each node
		// e.g. P(Altersgruppe=0), P(Altersgruppe=5), P(Geschlecht=1), ...
		cout << "Using the join tree algorithm:\n";
		for (int i = 0; i < nodes.size(); i++) {
			for (int j = 0; j < nodes[i]->states.size(); j++) {
				cout << "p(" << nodes[i]->name.c_str() << "=" << j << ") = " << solution.probability(i)(j) << endl;
			}
		}

		// Finally, before the program ends, we clear the SQLITE database by executing
		// a delete and drop statement on the table "data".

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

/*
 * void*, :: Data provided in the 4th argument of sqlite3_exec()
 * int,   :: The number of columns in row
 * char**,:: An array of strings representing fields in the row
 * char** :: An array of strings representing column names
 */
static int callback(void *param, int argc, char **argv, char **azColName)
{
	int z, n;
	std::cout << std::endl;
	
	if (argc == 2) {
		z = stoi(argv[0]);
		n = stoi(argv[1]);
		cout << "Z:" << z << " N:" << n;
	}
	else {
		std::cout << "ERROR!!!!" << std::endl;
	}
	
	double* p = reinterpret_cast<double*>(param);
	if (n == 0) {
		// If there are zero occurrences in the data set, p is set to -1 to indicate this state,
		// p should not be set to 0, cause e.g. 0/12 is a valid statistic that shows that the propability is very unlikely.
		*p = -1;
	}
	else {
		*p = (double)z / n;
	}

	return 0;
}