/* ------------------------------------------------------------------------- */
// File       : SG_File_XML.h
// Project    : Star Gen
// Author C++ : David de Lorenzo
/* ------------------------------------------------------------------------- */
#ifndef _STARGEN_FILE_XML_H_
#define _STARGEN_FILE_XML_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "SG_File.h"

/* ------------------------------------------------------------------------- */
/// This is a file writer for the solar system description.
/** The output format is an XML file. */
/* ------------------------------------------------------------------------- */
class SG_File_XML : public SG_File
{
	public:

        SG_File_XML(std::string  filename, long seed);
		~SG_File_XML();

    private:

        void addValue     (std::string  name, bool value);
        void addValue     (std::string  name, char* value,        std::string  comment);
        void addValue     (std::string  name, std::string  value, std::string  comment);
        void addValue     (std::string  name, long double value,  std::string  comment);
        void addIntValue  (std::string  name, long double value,  std::string  comment);
        void addPercentage(std::string  name, long double value);
        void addSection   (std::string  name);
        void closeSection (std::string  name);
		void subSection   ();
		void addLine      ();
};


#endif




