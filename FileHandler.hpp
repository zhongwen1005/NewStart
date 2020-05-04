#pragma once
#include <iostream>
#include <fstream>

using namespace std;

#define lineLength 100

template<typename Line, typename Allocator = std::allocator<Line>>
class FileHandler {
private:
	ifstream fin;
	typedef Allocator allocator_type;
	allocator_type allocator_;

public:
	FileHandler() {
	}
	~FileHandler() {
		fin.close();
	}

	bool open(const char * filename) {
		fin.open(filename, ios::in);
		if (!fin) {
			cout << "Failed to open file!" << endl;
			return false;
		}

		return true;
	}

	void skipFirstLine() {
		// prevent diffenet format file
		char str[lineLength];
		// skip first line
		fin.getline(str, lineLength);
	}

	Line * readLine() {
		char str[lineLength];
		memset(str, '\0', lineLength);
		while (fin.getline(str, lineLength)) {
			//Line* lineItem = allocator_.allocate(sizeof(Line));
			if (str[0] == '#') {
				continue;
			}
			Line* lineItem = new Line();
			if (!lineItem->parseLine(str, lineLength)) {
				cout << "Parse line error: " << str << endl;
				break;
			}

			return lineItem;
		}

		return nullptr;
	}
};