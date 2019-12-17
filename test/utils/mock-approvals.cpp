#include <iostream>

#include "mock-approvals.hpp"

approval::approval(std::string file){
    approvedFile = file;
    fileStream.open(approvedFile);

    if(fileStream.is_open()){
	std::string line;
	while(std::getline(fileStream, line)){
	    approvedBuffer.append(line);
	}
	fileStream.close();
    }
}

bool approval::testEquality(std::string _buffer)
{
    std::string buffer = 
	_buffer.erase(_buffer.length() - 1);
    std::cout << "in: " << buffer;

    std::cout << "approved: " << approvedBuffer;
    int comp = buffer.compare(approvedBuffer);
    std::cout << comp << std::endl;
    return  comp == 0 ? true: false;
}
