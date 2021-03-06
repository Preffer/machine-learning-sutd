#include <iostream>
#include <fstream>
#include <memory>
#include <boost/algorithm/string.hpp>

using namespace std;

/* Becasue I want to use OpenMP,
 * So I must use vector as the container for paras, because openmp require random access iterator.
 * And must use list as the container for results, because vector need to relocate, and therefore not thread-safe.
 */

// type define
typedef array<double, 3> Row;
typedef shared_ptr< list<Row> > RowList;
typedef array<double, 3> Para;
typedef shared_ptr< vector<Para> > ParaList;
typedef pair<Para, int> Score;

// function defines
RowList readFile(const string& fileName);
int sign(const Row& row, const Para& para);
int errors(const Para& para);
int predictErrors(const Para& para);
Para patch(const Para& para, const Row& row);
ParaList analysys(const Para& para);
Score deepin(const int now, const Para& para, const int limit);

// overloading define
ostream &operator<<(ostream& out, const Para& para) {
	out << "[" << para[0] << ", " << para[1] << ", " << para[2] << "]";
	return out;
}

ostream &operator<<(ostream& out, const Score& score) {
	out << "<" << score.first << ", " << score.second << ">";
	return out;
}

// preset data defines
Para initPara = {0, 0, 0};
auto trainFileContent = readFile("train_1_5.csv");
auto testFileContent = readFile("test_1_5.csv");

// main
int main(void) {
	Score trainResult = deepin(0, initPara, 1);
	int predictionError = predictErrors(trainResult.first);

	cout << "Train Result: " << trainResult << endl;
	cout << "Prediction Error: " << predictionError << endl;
}

// function implementation
RowList readFile(const string& fileName) {
	ifstream inFile(fileName);
	string line;
	vector<string> words;
	Row nums;
	RowList fileContent = static_cast<RowList>(new list<Row>);

	while(getline(inFile, line)){
		boost::split(words, line, boost::is_any_of(","));
		nums = {
			stod(words[0]),
			stod(words[1]),
			stod(words[2])
		};
		fileContent->push_back(nums);
	}

	return fileContent;
}

inline int sign(const Row& row, const Para& para) {
	double result = para[0] * row[0] + para[1] * row[1] + para[2];
	if(result >= 0){
		return 1;
	} else{
		return -1;
	}
}

int errors(const Para& para) {
	int sum = 0;

	for(auto it = trainFileContent->begin(); it != trainFileContent->end(); it++){
		Row& row = *it;
		if(sign(row, para) != row[2]){
			sum++;
		}
	}

	return sum;
}

int predictErrors(const Para& para) {
	int sum = 0;

	for(auto it = testFileContent->begin(); it != testFileContent->end(); it++){
		Row& row = *it;
		if(sign(row, para) != row[2]){
			sum++;
		}
	}

	return sum;
}

Para patch(const Para& para, const Row& row) {
	Para tryPara = {
		para[0] + row[2] * row[0],
		para[1] + row[2] * row[1],
		para[2] + row[2]
	};

	return tryPara;
}

ParaList analysys(const Para& para) {
	ParaList candidatePara = static_cast<ParaList>(new vector<Para>);

	for(auto it = trainFileContent->begin(); it != trainFileContent->end(); it++){
		Row& row = *it;
		if(sign(row, para) != row[2]){
			candidatePara->push_back(patch(para, row));
		}
	}

	return candidatePara;
}


/* @parameter: init para
 * @return: the best para among the child and sub-child para created by this para.
 */
Score deepin(const int now, const Para& para, const int limit) {
	list<Score> thisResult;
	thisResult.push_back(Score(para, errors(para)));

	if(now < limit){
		auto tryParas = *analysys(para);
		const unsigned int size = tryParas.size();

		//#pragma omp parallel for
		for(unsigned int i = 0; i < size; i++){
			Para& childPara = tryParas[i];
			thisResult.push_back(deepin(now+1, childPara, limit));
			if(now == 0){
				cout << "No: " << i << ", Score: " << *thisResult.rbegin() << endl;
			}
		}
	} else{
		auto tryParas = *analysys(para);
		const unsigned int size = tryParas.size();

		for(unsigned int i = 0; i < size; i++){
			Para& subPara = tryParas[i];
			thisResult.push_back(Score(subPara, errors(subPara)));
		}
	}

	Para bestPara = thisResult.begin()->first;
	int bestError = thisResult.begin()->second;

	for(auto it = thisResult.begin(); it != thisResult.end(); it++){
		if(it->second < bestError){
			bestPara = it->first;
			bestError = it->second;
		}
	}

	return(Score(bestPara, bestError));
}
