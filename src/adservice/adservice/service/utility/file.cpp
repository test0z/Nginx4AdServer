//
// Created by guoze.lin on 16/3/30.
//

#include "file.h"
#include "common/functions.h"
#include "logging.h"

namespace adservice {
namespace utility {
	namespace file {

		void loadFile(char * buffer, const char * filePath)
		{
			{
				std::fstream fs(filePath, std::ios_base::in);
				if (!fs.good()) {
					LOG_ERROR << " can't open json file:" << filePath;
					return;
                }
				std::stringstream ss;
				do {
					std::string str;
					std::getline(fs, str, '\n');
					ss << str;
				} while (!fs.eof());
				fs.close();
				std::string str = ss.str();
				LOG_INFO << "file length:" << str.length();
				memcpy(buffer, str.c_str(), str.length());
				buffer[str.length()] = '\0';
			}
        }
	}
}
}
