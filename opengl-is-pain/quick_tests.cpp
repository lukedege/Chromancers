#include "utils/shader.h"
#include "utils/utils.h"



int mainz()
{
	std::string         sourceText;
	std::ifstream       sourceFile;
	std::stringstream sourceStream;

	sourceFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		sourceFile.open("shaders/test_basic_spv.vert");
		sourceStream << sourceFile.rdbuf();
		sourceFile.close();
		sourceText = sourceStream.str();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

	// Pre-process for merged text compilation 
	// Find and remove #version (will be added later) and eventual #include 
	sourceText = utils::strings::eraseLineContaining(sourceText, "#version");
	sourceText = utils::strings::eraseAllLinesContaining(sourceText, "#include");

	return 0;
}