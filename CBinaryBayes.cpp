#include <heur/CBinaryBayes.h>

#include <map>

#include <data/CDataset.h>
#include <data/CDataset.h>
#include <util/Util.h>
#include <algorithm>
#include <vector>
#include <stdlib.h>
#include <string>
#include <cmath>

//#define DEBUG
#define INTERVAL_TIME 30

CBinaryBayes::CBinaryBayes() {}

//used for cross-validation: allow to ignore a certain part of the trainfile
void CBinaryBayes::trainOnline(std::string filename, int n, int numFeat, int ignore_start, int ignore_end) {
	this->m = numFeat;
	std::ifstream input(filename.c_str());	
	std::string line;	

	if(!input)
		reportError("File not found ...");
	
	clock_t tbegin = clock();
	clock_t tprev  = clock();
	clock_t tend   = clock();

	//Prepare the vectors to avoid unnecessary space usage
	this->P0N.reserve(m);
	this->P1N.reserve(m);
	this->P0P.reserve(m);
	this->P1P.reserve(m);
	this->isKnown.reserve(m);
	
	//PASS 1: determine probabilities for each class
	int classCountPos = 0;
	int classCountNeg = 0;
	
	std::vector<int> oneCountPos;	//How many ones in for a features given that the class value is Pos
	std::vector<int> oneCountNeg;   //How many ones in for a features given that the class value is Neg
	
	for(int i = 0; i < m; i++) {
		oneCountPos  .push_back(0);
		oneCountNeg  .push_back(0);
		this->isKnown.push_back(false);
	}
	
	//PASS 1: Add all training points
	for(int i = 0; i<n; i++) {//i<n
		getline( input, line ); 
		if(!input) break;														//Stop at end of file
		if(i > ignore_start && i < ignore_end) continue;						//Ignore part of the file

		std::vector<std::string> tokens = lineToTokens(line.c_str(),' ');		
		//Update prior probability count
		int classval	= atoi(tokens[0].c_str());
		
		if(classval != -1 && classval != 1) 
			reportError("Class values should be either -1 or 1.");

		bool hit		= classval > 0;
		if(hit) classCountPos ++;
		else    classCountNeg ++;


		int prev_item = 0;
		for(std::vector<std::string>::size_type j = 1; j < tokens.size(); j++) {
			std::string token = tokens[j];
			std::vector<std::string> pair = lineToTokens(token.c_str(),':');
			if(pair.size() < 2) continue; //extra space element
			int feat   = lexical_cast<int,std::string>(pair[0]);
			//double val = lexical_cast<double,std::string>(pair[1]); //Not used, assumed to be equal to one
			if(feat > this->m) {
				std::stringstream s;
				s << "Found a feature with dimension higher than actual dataset dimension (" << m << " vs " << feat << ").";
				reportError(s.str());
			}
			
			if(hit) {
				oneCountPos[feat]++;
			}
			else {
				oneCountNeg[feat]++;
			}
		}
		#ifdef DEBUG
		tend   = clock();
		if( (tend-tprev)/ CLOCKS_PER_SEC  > INTERVAL_TIME ) {
			int done = i;
			int left = 2*n-i;
			double timedone = double(tend-tbegin) / CLOCKS_PER_SEC;
			double timeleft = timedone / done * left;
			LOG << "Time " << done << "/" << 2*n << ":" << timeleft << "s\n";
			tprev = tend;
		}
		#endif
	}
	//Intermediate: format all counts to (log)probabilities
	this->prior			= classCountPos * 1.0 / n;
	double priorNeg		= 1-prior;

	this->allZeroSumPos = 0;
	this->allZeroSumNeg = 0;
	for(int i = 0; i < this->m; i++) {
		int zeroCountPos = classCountPos - oneCountPos[i];
		int zeroCountNeg = classCountNeg - oneCountNeg[i];

		//Score class 1		
		double d = 1;
		double P0Pi = log((1.0*zeroCountPos+1) / (classCountPos+2));
		double P1Pi = log((1.0*oneCountPos[i]+1) / (classCountPos+2));
		this->P0P.push_back(P0Pi);
		this->P1P.push_back(P1Pi);				
		
		//Score class 2
		double P0Ni = log((1.0*zeroCountNeg+1) / (classCountNeg+1));
		double P1Ni = log((1.0* oneCountNeg[i]+2) / (classCountNeg+2));
		this->P0N.push_back(P0Ni);
		this->P1N.push_back(P1Ni);
		
		//Score class 0 (is the negative value of the other one ...)
		isKnown[i] = (oneCountPos[i] > 0) && (oneCountNeg[i] > 0);

		this->allZeroSumPos += P0Pi;
		this->allZeroSumNeg += P0Ni;

		#ifdef DEBUG
		//Timer update
		tend   = clock();		
		if( (tend-tprev)/ CLOCKS_PER_SEC  > INTERVAL_TIME ) {
			int done = n+i;
			int left = 2*n-i;			
			float timedone = double(tend-tbegin) / CLOCKS_PER_SEC;
			float timeleft = timedone / done * left;
			LOG << "Time " << done << "/" << n << ":" << timeleft << "s\n";
			tprev = tend;
		}
		#endif
	}
	
	this->treshold = 0;
}
void CBinaryBayes::trainOnline(std::string filename,int cut, int m) {
	int n = cut;
	int numFeat = m;
	this->trainOnline(filename, cut, m, cut,cut);
}

//Ranking NB
double  CBinaryBayes::score(CSparseList * list) {
	std::vector<sparse_element> elements= *(list->getElements());
	double possum = log(this->prior) + this->allZeroSumPos;	
	double negsum = log(1-this->prior) + this->allZeroSumNeg;
	int skipped = 0;
	for(size_t  d = 0; d < elements.size(); d++) {
		sparse_element element = elements[d];
		if(isKnown[element.index]) {
			possum += this->P1P[element.index];
			possum -= this->P0P[element.index];
			negsum += this->P1N[element.index];
			negsum -= this->P0N[element.index];
		}else skipped ++;
	}
	return possum-negsum;
}
double CBinaryBayes::predict(CSparseList * list) {
	double score = this->score(list);
	if((score - this->treshold) > 0)
		return 1;
	else
		return -1;
}

void CBinaryBayes::scoreAndPredict(CSparseList * list, int & prediction, double & score) {
	score		= this->score(list);
	prediction	= (score - this->treshold  > 0)? 1 : -1;
}
void CBinaryBayes::saveModel(std::string fileName) {
	std::ofstream fh (fileName.c_str());
	//Output general info on first line
	fh << this->m <<" "<<this->Cpos <<" "<<this->Cneg <<" "<<this->prior<<" "<<this->allZeroSumPos<<" "<<this->allZeroSumNeg<<"\n";

	//Output class probabilities line per line
	for(int i = 0; i < m; i++) {
		fh <<  this->isKnown[i]<<" "<<this->P0P[i]<<" "<<this->P0N[i] <<" "<<this->P1P[i]<<" "<<this->P1N[i]<<"\n";
	}
	fh.close();
}
void CBinaryBayes::loadModel(std::string fileName) { //TODO: 
	if(!fileExists(fileName))
		reportError("Couldn't load model file.");

	std::ifstream input (fileName.c_str());
	std::string line;

	//Get general data from first line
	getline(input, line); 	
	std::vector<std::string> splits = lineToTokens(line.c_str(),' ');
	this->m    = lexical_cast<int,std::string>(splits[0]);

	this->Cpos = lexical_cast<double,std::string>(splits[1]);
	this->Cneg = lexical_cast<double,std::string>(splits[2]);
	this->prior= lexical_cast<double,std::string>(splits[3]);
	this->allZeroSumPos = lexical_cast<double,std::string>(splits[4]);
	this->allZeroSumNeg = lexical_cast<double,std::string>(splits[5]);

	//Input feature priors
	this->isKnown.clear();
	this->P0N.clear();
	this->P0P.clear();
	this->P1N.clear();
	this->P1P.clear();
	this->P0N.reserve(m);
	this->P0P.reserve(m);
	this->P1N.reserve(m);
	this->P1P.reserve(m);
	this->isKnown.reserve(m);

	for(int i = 0; i < m; i++) {
		getline(input, line); 	
		std::vector<std::string> splits = lineToTokens(line.c_str(),' ');	
		this->isKnown.push_back(lexical_cast<bool,std::string>(splits[0]));
		this->P0P.push_back(lexical_cast<double,std::string>(splits[1]));
		this->P0N.push_back(lexical_cast<double,std::string>(splits[2]));
		this->P1P.push_back(lexical_cast<double,std::string>(splits[3]));
		this->P1N.push_back(lexical_cast<double,std::string>(splits[4]));
	}
	input.close();
}

CBinaryBayes::~CBinaryBayes() {
}
