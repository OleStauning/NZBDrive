#include <string>

class HTTPDownloader
{
public:
	static std::string LoadHTTP(const std::string& host, const std::string& port, const std::string& target);
	static std::string LoadHTTPS(const std::string& host, const std::string& port, const std::string& target);
};
