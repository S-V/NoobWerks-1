/* ------------------------------------------------------------------------- */
// File       : SG_File_TXT.cpp
// Project    : Star Gen
// Author C++ : David de Lorenzo
/* ------------------------------------------------------------------------- */
#include <Base/CPP/C++11_in_C++03.h>
#include "SG_File_TXT.h"

/*----------------------------------------------------------------------*/
/// Constructor
/*----------------------------------------------------------------------*/
SG_File_TXT::SG_File_TXT(std::string filename, long seed) : SG_File(filename,seed)
{}

/*----------------------------------------------------------------------*/
/// Destructor. 
/*----------------------------------------------------------------------*/
SG_File_TXT::~SG_File_TXT()
{
	closeSection("system");
}

/*----------------------------------------------------------------------*/
/// Write a data in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addValue(std::string name, long double value, std::string comment)
{
    mOutputFile << "  " << name << " = " << std::to_string(value) ;
	if (comment!="") mOutputFile << " (" << comment << ")";
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Write a data in the file (converted to integer)
/*----------------------------------------------------------------------*/
void SG_File_TXT::addIntValue(std::string name, long double value, std::string comment)
{
	int intvalue = (int)(value);
    mOutputFile << "  " << name << " = " << std::to_string(intvalue) ;
	if (comment!="") mOutputFile << " (" << comment << ")";
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Write a data in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addValue(std::string name, bool value)
{
    mOutputFile << "  " << name << " = " << (value? "yes":"no") << "\n";
}

/*----------------------------------------------------------------------*/
/// Write a data in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addPercentage(std::string name, long double value)
{
    int percentage = int(value*100);
    mOutputFile << "  " << name << " = " << std::to_string(percentage) << "% \n";
}

/*----------------------------------------------------------------------*/
/// Write a data in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addValue(std::string name, char* value, std::string comment)
{
	mOutputFile << "  " << name << " = " << value;
	if (comment!="") mOutputFile << " (" << comment << ")";
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Write a data in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addValue(std::string name, std::string value, std::string comment)
{
	mOutputFile << "  " << name << " = " << value;
	if (comment!="") mOutputFile << " (" << comment << ")";
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Add a section Header in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addSection(std::string name)
{
	mOutputFile << name << ":\n";
}

/*----------------------------------------------------------------------*/
/// Add a section Ender in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::closeSection(std::string name)
{
    (void)name;    // unused parameter
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Add a sub-section in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::subSection()
{
	mOutputFile << "\n";
}

/*----------------------------------------------------------------------*/
/// Add a separator line in the file
/*----------------------------------------------------------------------*/
void SG_File_TXT::addLine()
{
	mOutputFile << "\n********************************************** \n";
}

