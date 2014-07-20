#define __REENTRANT

#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include "threadhandler.h"

using namespace std;

struct ActivePage {
	unsigned long tickIn;
	unsigned long tickOut;
	unsigned long frameNumber;
	map<unsigned long, pair<unsigned long, unsigned int> > mReferences;
	map<unsigned long, pair<unsigned long, unsigned int> > cReferences;
};


static ofstream xmlAnalysisFile;

void writeLongToFile(ofstream& inFile, unsigned long value)
{
	stringstream stringy;
	stringy << value;
	inFile << stringy.rdbuf();
}

void writeLongToFile(ofstream& inFile, long value)
{
	stringstream stringy;
	stringy << value;
	inFile << stringy.rdbuf();
}


void writeIntToFile(ofstream& inFile, unsigned int value)
{
	stringstream stringy;
	stringy << value;
	inFile << stringy.rdbuf();
}

ofstream& openXMLAnalysisFile()
{
	xmlAnalysisFile.open("pageanalysis.xml");

	xmlAnalysisFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	xmlAnalysisFile << "<!DOCTYPE pageanalysis [\n";
	xmlAnalysisFile << "<!ELEMENT pageanalysis (page*)>\n";
	xmlAnalysisFile <<
		"<!ATTLIST pageanalysis version CDATA #FIXED \"0.2\">\n";
	xmlAnalysisFile << "<!ATTLIST pageanalysis xmlns CDATA #FIXED";
	xmlAnalysisFile << " \"http://cartesianproduct.wordpress.com\">\n";
	xmlAnalysisFile << "<!ELEMENT page (code*, rw*)>\n";
	xmlAnalysisFile << "<!ATTLIST page frame CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ATTLIST page in CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ATTLIST page out CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ELEMENT code (#PCDATA)>\n";
	xmlAnalysisFile << "<!ATTLIST code address CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ATTLIST code size CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ELEMENT rw (#PCDATA)>\n";
	xmlAnalysisFile << "<!ATTLIST rw address CDATA #REQUIRED>\n";
	xmlAnalysisFile << "<!ATTLIST rw size CDATA #REQUIRED>\n";
	xmlAnalysisFile << "]>\n";
	xmlAnalysisFile << "<pageanalysis ";
	xmlAnalysisFile << "xmlns=\"http://cartesianproduct.wordpress.com\">\n";

	return xmlAnalysisFile;
}

extern "C" {

void insertRecord(struct ThreadResources* thResources)
{
	struct ThreadLocal* local = thResources->local;
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*>*>
		(globals->activePages);

	map<unsigned long, struct ActivePage*>::iterator it;
	pthread_mutex_lock(&globals->threadGlobalLock);
	it = activePages->find(local->anPage);
	if (it == activePages->end()) {
		struct ActivePage* nextPage = new ActivePage();
		nextPage->frameNumber = local->anPage;
		nextPage->tickIn = globals->totalTicks;
		nextPage->tickOut = 0;
		activePages->insert(pair<unsigned long, struct ActivePage*>
			(local->anPage, nextPage));
		it = activePages->find(local->anPage);
	}
	if (local->anType == 'c') {
		it->second->cReferences.
			insert(pair<unsigned long,
			pair<unsigned long, unsigned int> >
			(globals->totalTicks,
			pair<unsigned long, unsigned int>
			(local->anDestination, local->anSize)));
	} else {
		it->second->mReferences.
			insert(pair<unsigned long,
			pair<unsigned long, unsigned int> >
			(globals->totalTicks,
			pair<unsigned long, unsigned int>
			(local->anDestination, local->anSize)));
	}
	pthread_mutex_unlock(&globals->threadGlobalLock);
}

//called with lock on
void doneWithRecord(long page, struct ThreadResources* thResources)
{
	struct ThreadGlobal* globals = thResources->globals;
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*>*>
		(globals->activePages);
	it = activePages->find(page);
	if (it == activePages->end()) {
		cout << "ERROR - not found\n";
		//failed
	}
	//write out record
	xmlAnalysisFile << "<page frame=\"";
	writeLongToFile(xmlAnalysisFile, it->second->frameNumber);
 	xmlAnalysisFile << "\" in=\"";
	writeLongToFile(xmlAnalysisFile, it->second->tickIn);
	xmlAnalysisFile << "\" out=\"";
	writeLongToFile(xmlAnalysisFile, globals->totalTicks);
	xmlAnalysisFile << "\" >\n";
	map<unsigned long, pair<unsigned long, unsigned int> >::iterator recIt;
	for (recIt = it->second->cReferences.begin();
		recIt != it->second->cReferences.end(); recIt++) {
		xmlAnalysisFile << "<code address=\"";
		writeLongToFile(xmlAnalysisFile, recIt->second.first);
		xmlAnalysisFile << "\" size=\"";
		writeIntToFile(xmlAnalysisFile, recIt->second.second);
		xmlAnalysisFile << "\">";
		writeLongToFile(xmlAnalysisFile, recIt->first);
		xmlAnalysisFile << "</code>\n";
	}
	for (recIt = it->second->mReferences.begin();
		recIt != it->second->mReferences.end(); recIt++) {
		xmlAnalysisFile << "<rw address=\"";
		writeLongToFile(xmlAnalysisFile, recIt->second.first);
		xmlAnalysisFile << "\" size=\"";
		writeIntToFile(xmlAnalysisFile, recIt->second.second);
		xmlAnalysisFile << "\">";
		writeLongToFile(xmlAnalysisFile, recIt->first);
		xmlAnalysisFile << "</rw>\n";
	}
	xmlAnalysisFile << "</page>\n";
	//remove record
	delete it->second;
	activePages->erase(it);
}

void createRecordsTree(struct ThreadResources* thResources)
{
	thResources->globals->activePages = 
		(new map<unsigned long, struct ActivePage*>());
	openXMLAnalysisFile();
}

void removeRecordsTree(struct ThreadResources* thResources)
{
	xmlAnalysisFile << "</pageanalysis>\n";
	xmlAnalysisFile.close();
	map<unsigned long, struct ActivePage*>::iterator it;
	map<unsigned long, struct ActivePage*>* activePages =
		static_cast<map<unsigned long, struct ActivePage*>*>
		(thResources->globals->activePages);
	for (it = activePages->begin(); it != activePages->end(); it++) {
		delete it->second;
	}
}

} //end extern "C"
