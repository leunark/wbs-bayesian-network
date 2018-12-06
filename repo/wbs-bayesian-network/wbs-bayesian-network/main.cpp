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

sqlite3* openDatabase(const char* dbname);
std::vector<std::string> generateInitialStatements(ifstream* file);
void executeStatements(sqlite3* db, std::vector<std::string> statements);
double calculateProbability(sqlite3* db, std::string statement, int statesSize);
static int callback(void*, int, char**, char**);

int main() {
	// Parameters
	const char* filename = "P001.csv";
	const char* dbname = "programmentwurf.db";

	// Database
	sqlite3 *db; // DB pointer

	// Others
	std::vector<std::string> statements; // List for sql statements
	std::string pSQL; // Container for one sql statement

	try
	{
		// Open database
		db = openDatabase(dbname);

		// Open csv file
		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cout << "ERROR: File Open" << std::endl;
		}

		// Generate statement list
		statements = generateInitialStatements(&file);

		// Execute statement list
		executeStatements(db, statements);

		// At this point it is the task of the user to define the structure of the bayesian
		// network. By using the class Node, the structure can be implemented into a model
		// independent of dlib.

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

		// Create nodes with its states
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

		// Set parent-child relations
		nodes[Verheiratet]->parents = { nodes[Altersgruppe] };
		nodes[Kinderzahl]->parents = { nodes[Altersgruppe] };
		nodes[Beruf]->parents = { nodes[Abschluss], nodes[Geschlecht] };
		nodes[Familieneinkommen]->parents = { nodes[Beruf] };
		nodes[Buch]->parents = { nodes[Verheiratet], nodes[Kinderzahl], nodes[Familieneinkommen] };

		// Now dlib helps us to transfer the data from the model into the bayesian network.
		// The seperation of the two steps is important to be able to calculate the probabilities
		// automatically.

		// Bayesian network implementation
		using namespace bayes_node_utils;

		// Declare bayesian network bn
		directed_graph<bayes_node>::kernel_1a_c bn;

		// Now the model logic is transferred to the bayesian network.
		// �Parent nodes and state sizes of every node are set.
		bn.set_number_of_nodes(nodes.size());
		for (int i = 0; i < nodes.size(); i++) {
			// Set edges with parents
			for (int j = 0; j < nodes[i]->parents.size(); j++) {
				bn.add_edge(nodes[i]->parents[j]->id, nodes[i]->id);
			}
			// Set number of states
			set_node_num_values(bn, nodes[i]->id, nodes[i]->states.size());
		}

		// Add conditional probability information for each node with the assignment 
		// object that allows us to specify the state of each nodes parents 
		assignment parent_state;
		
		// A checksum shell guarantee that the sum of all states for one parent combination is 1.
		// This is mandatory for a complete bayesian network.
		double checksum = 0;

		// In this part statements have to be generated that retrieve
		// 1. an abosolute number of occurrences for the current current combination: 
		// Let PX be the xth parent of node N, then a combination would be : 
		//    P1 P2 ... PX N 
		// 2. an absolute number of occurrences without the node itself int the combination
		//	  P1 P2 ... PX
		// The division of 1. and 2. gives us the probability of the current combination

		// Iterate over every node
		for (int i = 0; i < nodes.size(); i++) {
			// Generate the CPT for current node
			auto cpt = *(nodes[i]->startGeneratingCPT());

			// Add all parents to parent state for current node
			parent_state.clear();
			for (int k = 0; k < nodes[i]->parents.size(); k++) {
				parent_state.add(nodes[i]->parents[k]->id,0);
			}

			// Process every combination within the CPT
			for (int j = 0; j < cpt.size(); j++) {
				// Build statement for each combination
				std::string statement = "";
				std::string statementa = "select 1.00 * (";
				std::string statementb = "select count(*) from data where ";
				std::string statementc = "";
				if (cpt[j].size() > 1) {
					// Iterate over every state of the combination
					for (int k = 0; k < cpt[j].size() - 1; k++) {
						statementb += nodes[i]->parents[k]->name + " = '" + cpt[j][k].name + "' and "; // Add combination to statement
						// Log the current state to get the whole combination eventually
						std::cout << cpt[j][k].name << " ";
						// Set states of nodes parents
						parent_state[cpt[j][k].node->id] = cpt[j][k].pos;
					}
					statementc = statementb.substr(0, statementb.size() - 4);
				}
				else {
					statementc = statementb.substr(0, statementb.size() - 6);
				}

				// Add last condition to statement, whereas last item will be the node itself!
				statementb += nodes[i]->name + " = '" + cpt[j][cpt[j].size()-1].name +"'";
				// Eventually log the last state of the combination
				std::cout << cpt[j][cpt[j].size() - 1].name << " ";
				statement = statementa + statementb + ") as z, (" + statementc + ") as n";

				// Calculate the probability of current node by fetching occurrences from the database
				double p = calculateProbability(db, statement, nodes[i]->states.size());

				// Increase checksum by the calculated probability
				checksum += p;

				// Print parent_state for debugging purposes
				for (int k = 0; k < cpt[j].size() - 1; k++) {
					std::cout << "Node" << cpt[j][k].node->id << "::State" << parent_state[cpt[j][k].node->id] << "; ";
				}
				std::cout << std::endl;

				// Set node probability  p(A=?| P1=?, P2=?, ...) = p
				std::cout << "set_node_probability" << "(bn," << nodes[i]->id << "," << cpt[j][cpt[j].size() - 1].pos << "," << parent_state.size() << "," << p << ")" << std::endl;
				set_node_probability(bn, nodes[i]->id, cpt[j][cpt[j].size() - 1].pos, parent_state, p);

				// Log checksum for debugging purposes
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
		// function calls do this. 
		create_moral_graph(bn, join_tree);
		create_join_tree(join_tree, join_tree);

		// Now that we have a proper join_tree we can use it to obtain a solution to our
		// bayesian network by declaring an instance of the bayesian_network_join_tree object 
		// as follows:
		bayesian_network_join_tree solution(bn, join_tree);

		// Now print out all the probabilities for each node
		// e.g. P(Altersgruppe=0), P(Altersgruppe=5), P(Geschlecht=1), ...
		cout << "\n\nPrior probability of each node in the network:\n";
		for (int i = 0; i < nodes.size(); i++) {
			for (int j = 0; j < nodes[i]->states.size(); j++) {
				cout << "p(" << nodes[i]->name.c_str() << "=" << j << ") = " << solution.probability(i)(j) << endl;
			}
		}

		// Now to make things more interesting let's say that we have discovered that the C 
		// node really has a value of 1.  That is to say, we now have evidence that 
		// C is 1.  We can represent this in the network using the following two function
		// calls.

		// Hier Marcel in ner Schleife nacheinander f�r jeden Node alle States ausgeben,
		// sowie nat�rlich eine Auswahl einlesen, wobei auch keine Angabe gemacht werden kann.
		// Alle nodes befinden sich in dem Vektor nodes, einfach so zugreifen nodes[i]->...
		/*
		set_node_value(bn, Altersgruppe, 1);
		set_node_as_evidence(bn, Altersgruppe);
		set_node_value(bn, Verheiratet, 1);
		set_node_as_evidence(bn, Verheiratet);
		set_node_value(bn, Kinderzahl, 1);
		set_node_as_evidence(bn, Kinderzahl);
		set_node_value(bn, Geschlecht, 1);
		set_node_as_evidence(bn, Geschlecht);
		set_node_value(bn, Abschluss, 1);
		set_node_as_evidence(bn, Abschluss);
		set_node_value(bn, Beruf, 2);
		set_node_as_evidence(bn, Beruf);
		set_node_value(bn, Familieneinkommen, 1);
		set_node_as_evidence(bn, Familieneinkommen);
		set_node_value(bn, Buch, 1);
		set_node_as_evidence(bn, Buch);
		*/

		int input;
		int success;
		// iterate through all nodes except Buch
		for (int i = 0; i < NodeEnum::Buch; i++) {
			do {
				cout << endl;
				cout << endl;
				cout << endl;
				cout << "Please enter the value for '" << nodes[i]->name << "' (number 0-" << (nodes[i]->states.size() - 1) << ")" << endl;
				cout << endl;

				// print all states of this node
				for (int j = 0; j < nodes[i]->states.size(); j++) {
					cout << "   (" << j << ") :  " << nodes[i]->states[j] << endl;
				}
				cout << "   (9) :  nicht relevant" << endl;
				cout << endl;

				// is input an integer and has a valid value
				if ((cin >> input) && ((input >= 0 && input < nodes[i]->states.size()) || input == 9)) {
					if (input != 9) {
						// set node evidence to the correct value
						set_node_value(bn, i, input);
						set_node_as_evidence(bn, i);
					}
					success = 1;
				}
				else {
					// clear invalid input from cin
					cin.clear();
					cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
					cout << endl;
					cout << "INVALID INPUT, PLEASE RETRY!";
					success = 0;
				}
				// repeat this procedure until the state value has been successfully specified
			} while (!success);
		}




		// Now we want to compute the probabilities of all the nodes in the network again
		// given that we now know that C is 1.  We can do this as follows:
		bayesian_network_join_tree solution_with_evidence(bn, join_tree);

		// now print out the probabilities for each node
		cout << "\n\nSolution with evidence:\n";
		for (int i = 0; i < nodes.size(); i++) {
			for (int j = 0; j < nodes[i]->states.size(); j++) {
				cout << "p(" << nodes[i]->name.c_str() << "=" << j << ") = " << solution_with_evidence.probability(i)(j) << endl;
			}
		}

		// Finally, before the program ends, we clear the SQLITE database by executing
		// a delete and drop statement on the table "data".

		// Add delete- and drop-statement
		statements.clear();
		statements.push_back("delete from data");
		statements.push_back("drop table data");

		// Execute statement list
		executeStatements(db, statements);

		// Cleanup
		sqlite3_close(db);
		file.close();

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

sqlite3* openDatabase(const char* dbname) 
{
	sqlite3* db;
	if (sqlite3_open("dbname", &db))
	{
		std::cout << "Can't open database: " << sqlite3_errmsg(db) << "\n";
	}
	else
	{
		std::cout << "Open database successfully\n\n";
	}
	return db;
}

std::vector<std::string> generateInitialStatements(ifstream* file) 
{
	// Variables
	std::vector<std::string> statements;
	std::string pSQL;

	// Delete first table
	statements.push_back("drop table if exists data");

	// Read header first line and create create-statement
	std::string line = "";
	std::getline(*file, line);
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
	while (std::getline(*file, line)) {
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

	return statements;
}

// Execute statement list
void executeStatements(sqlite3* db, std::vector<std::string> statements) 
{
	char* zErrMsg;
	for (int i = 0; i < statements.size(); i++)
	{
		if (sqlite3_exec(db, statements[i].c_str(), 0, 0, &zErrMsg))
		{
			std::cout << std::endl << "ERROR: " << sqlite3_errmsg(db) << std::endl;
			sqlite3_free(zErrMsg);
			break;
		}
		std::cout << std::endl << "SUCCESS: " << statements[i] << std::endl;
	}
}

double calculateProbability(sqlite3* db, std::string statement, int statesSize) {
	// Create int pointer
	double *p = new double();
	// Set default value (exception case)
	*p = -1;
	// Container for error message
	char* zErrMsg;

	// Execute the statement and process retrieved data in the callback function.
	// The calculated probability will be written into p from another function which is why 
	// p is a pointer.
	if (sqlite3_exec(db, statement.c_str(), callback, p, &zErrMsg) != SQLITE_OK)
	{
		std::cout << "ERROR: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_free(zErrMsg);
		return -1;
	}
	std::cout << endl;
	
	// If there are no occurrences in the data set (case p == -1), 
	// distribute probability equally by dividing 1 with the number of states
	if (*p == -1) {
		*p = (double)1 / statesSize;
	}
	std::cout << " p=" << *p << std::endl;
	return *p;
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