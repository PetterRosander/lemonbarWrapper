#ifndef _MOCK_APPROVALS_H_
#define _MOCK_APPROVALS_H_
#include <string>
#include <iostream>
#include <fstream>

class approval {
    public:
	approval(std::string file);
	bool testEquality(std::string buffer);
    private:
	std::string approvedFile;
    	std::ifstream fileStream;
	std::string approvedBuffer;
};


#endif /* _MOCK_APPROVALS_H_ */
