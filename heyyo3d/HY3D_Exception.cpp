#include "HY3D_Exception.h"
#include <sstream>

HY3D_Exception::HY3D_Exception(int line, const char * file) noexcept
	:
	line(line), file(file)
{
}

const char * HY3D_Exception::what() const noexcept
{
	std::ostringstream result;
	result << GetType() << std::endl << GetOriginString();
	// after this function, the stringstream dies, so we need to save the
	// message into a provided buffer.
	whatBuffer = result.str();
	return whatBuffer.c_str();
}

const char * HY3D_Exception::GetType() const noexcept
{
	return "HY3D Exception";
}

int HY3D_Exception::GetLine() const noexcept
{
	return line;
}

const std::string & HY3D_Exception::GetFile() const noexcept
{
	return file;
}

std::string HY3D_Exception::GetOriginString() const noexcept
{
	std::ostringstream result;
	result	<< "File: " << file << ", Line: " << line;
	return result.str();
}
