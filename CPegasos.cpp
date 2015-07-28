#include <algorithm/CPegasos.h>

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vector>

#include <data/CDataset.h>
#include <util/CKernel.h>
#include <data/CSparseVector.h>
#include <util/Util.h>
#include <data/CResult.h>

#include <iomanip>
//#define DEBUG

CPegasos::CPegasos(){
}
void CPegasos::initTraining() {
	srand(1);
	if(this->parameters.find("C") == this->parameters.end()) {
		LOG << "[CPegasos] : no C parameter given, defaulting to 1.\n";
		this->parameters["C"] = 1.0;
	}
	if(this->parameters.find("kernel") == this->parameters.end()) {
		LOG << "[CPegasos] : no kernel given, defaulting to linear.\n";
		this->parameters["kernel"] = new CKernel(CKernel::LINEAR);
	}

	this->bestgap		= (double) INT_MAX;
	this->tol			= pow(10.0f,-5);
	this->lambda        =  2.0 / boost::any_cast<double>(this->parameters["C"]);	
	this->kernel		= boost::any_cast<CKernel*>(this->parameters["kernel"]);	
	this->elapsed_time	= 0;
	this->n_iter		= 1;
	this->baditer		= 0;
	this->lossCacheSum  = 0;
	//this->maxtime		= INT_MAX;

	this->x_cache.clear();
	this->alpha.clear();
}
void CPegasos::train(CDataset* dataset) {
	reportError("Don't use CPegasos::train yet");
	initTraining();		
	int n = dataset->n;
	int m = dataset->m;
	
	this->x_cache.reserve(n);
    this->alpha.reserve(n);
	for(int i=0;i<n;i++) {
		alpha.push_back(0);
		x_cache.push_back(dataset->getInstance(i));
	}
	//Start procedure
	clock_t begin = clock();
	
	int index = 0;
	unsigned int * perm =generateRandIndices(dataset->n);
	double currentLoss = 0,prevLoss=0;
	while(this->elapsed_time < this->maxtime && this->n_iter < this->maxiter  && (this->baditer < this->maxbaditer)) {
		//Pick random sample
		if((this->n_iter-1) % n == 0) {
			delete perm;
			perm	= generateRandIndices(dataset->n);
		}
		index = perm[(this->n_iter-1) % n];
		this->nu		= 1.0/(this->lambda*this->n_iter);

		CInstance instance = dataset->getInstance(index);
		double checkval = (instance.classValue*this->score(&instance.attributes));

		if(checkval < 1) {
			this->alpha[index]	 = this->alpha[index] + 1;
		}
			
		//update loss
		prevLoss	= currentLoss;
		currentLoss = this->lossCacheSum / this->lossCache.size();
		currentLoss = this->lossCacheSum / this->lossCache.size();
		if( (currentLoss - prevLoss) > 0 || (currentLoss - prevLoss) < this->epsilon) { //loss was worse
			this->baditer++;
		} else this->baditer = 0;

		this->n_iter++;
		this->elapsed_time = double(clock() - begin) / CLOCKS_PER_SEC;
	}
	delete perm;
}

void CPegasos::trainOnline(CDatasetOnline & dataset) {
	initTraining();		
	dataset.rewind();
	this->x_cache.reserve(1024);
    this->alpha.reserve(1024);

	//Declare necessary control variables
	clock_t begin = clock();
	bool success=true;
	unsigned int index = 0;
	unsigned int batchsize = 50;
	double currentLoss=0,minLoss = INT_MAX;
	
	double previoustime = 0;

	//Main training loop
	while(
			(this->elapsed_time < this->maxtime) &&					//max time control
			(this->n_iter * batchsize < this->maxiter)  &&			//max iterations control
			(this->baditer *batchsize < this->maxbaditer))			//max bad iterations control
	{
		this->nu			= 1.0/(this->lambda*this->n_iter);		
		std::vector<CInstance> batchSet;
		for(unsigned int i = 0; i < batchsize;i++) {
			CInstance instance	= dataset.nextInstance(success);
			if(!success) {
				dataset.rewind();
				instance = dataset.nextInstance(success);
				if(!success)
					reportError("[CPegasos] I can't access the file decently ..");
			}
			batchSet.push_back(instance);
		}
		this->trainInstances(batchSet);
		
		//update loss
		currentLoss = this->lossCacheSum / this->lossCache.size();
		double dloss = (currentLoss - minLoss);
		if( dloss > -this->epsilon) { //gradient is positive: we gained loss
			this->baditer++;
		} else {
			this->baditer = 0;
			minLoss	= currentLoss;
		}

		this->n_iter++;
		this->elapsed_time = double(clock() - begin) / CLOCKS_PER_SEC;
		//consoleTick();
		if(this->elapsed_time - previoustime > 1)  {
			LOGTEMP.clear() << " " << this->elapsed_time <<"/" <<this->maxtime<<"s (iter="<< this->n_iter<<",baditer="<< this->baditer <<") "<< currentLoss;
			previoustime = this->elapsed_time;
		}
	}
	LOGTEMP.clear();
	//consoleTickClear();
}
void CPegasos::trainInstances(std::vector<CInstance> instances) {
	std::vector<CInstance> predictedWrong;
	std::vector<double> checkvals;
	for(unsigned int i = 0; i < instances.size(); ++i) {
		CInstance instance = instances[i];
		int index		= instance.index;
		double checkval = (instance.classValue*this->score(&instance.attributes));
		if(checkval < 1) {
			predictedWrong.push_back(instance);
			checkvals.push_back(checkval);
		}
	}
	//TODO: make sure the loss cache is being used correctly here
	for(unsigned int i =0; i < predictedWrong.size(); ++i) {
		unsigned int index = predictedWrong[i].index;
		if(index+1 > this->alpha.size()) {
			this->alpha.resize(index+1,0);
			this->x_cache.resize(index+1);
		} 
		this->alpha[index]	 = this->alpha[index] + 1;
		this->x_cache[index] = predictedWrong[i];

		//update loss cache
		double loss		= 1 - checkvals[i];
		if(loss < 0.0) loss = 0.0;
		if(this->lossCache.size() < index+1) {
			this->lossCacheSum += (index+1-lossCache.size());
			this->lossCache.resize(index+1,1);			
		}
		this->lossCacheSum += ( loss - this->lossCache[index] );
		this->lossCache[index] = loss;		
	}				
}

double CPegasos::score(CSparseVector * x) {
	double score = 0;
	addBias(x);
	//temporary
	for(unsigned int i = 0; i < this->x_cache.size(); i++) {
		CSparseVector * xi	= &this->x_cache[i].attributes;
		int yi				= this->x_cache[i].classValue;
		addBias(xi);
		if(this->alpha[i] > 0) {
			score += (yi * this->alpha[i] * kernel->k(xi,x)); 
		}
		removeBias(xi);
	}
	removeBias(x);
	score *= this->nu;
	return score;
}
double CPegasos::cachedScore(CInstance x) {
	double score = 0;
	addBias(&x.attributes);
	
	//get first 4096 products from cache temporary cache
	for(int i = 0; i < this->x_cache.size(); i++) {
		CInstance		xi  = this->x_cache[i];
		int yi				= xi.classValue;
		addBias(&xi.attributes);
		if(this->alpha[i] > 0) {
			score += (yi * this->alpha[i] * kernel->get(xi,x));
		}
		removeBias(&xi.attributes);
	}

	removeBias(&x.attributes);
	score *= this->nu;
	return score;
}

void CPegasos::save(std::string fileName) {
	std::ofstream fh (fileName);
	//Output general info on first line
	fh << std::fixed << std::setprecision(8) << " " << this->nu << " " << this->bias << " " << this->C << " " << this->bestgap << " " << this->lambda << " " << this->tol << " " << this->lossCacheSum << " " << this->kernel->kernelType << "\n";
	
	for(size_t i = 0; i < this->alpha.size(); i++) 
		fh << this->alpha[i] << " " << this->x_cache[i].index;
	fh << "\n";
	for(size_t i =0; i < this->kernel->parameter.size(); i++) {
		fh << this->kernel->parameter[i];
	}
	fh.close();
}
void CPegasos::printInfo() {
	LOG << "Pegasos model info\n";
	LOG << "\tNu=" << this->nu << "\n";
	LOG << "\tC=" << this->C << "\n";
	LOG << "\tt=" << this->n_iter << "\n";
}
CPegasos::~CPegasos() {
	delete this->kernel;
}