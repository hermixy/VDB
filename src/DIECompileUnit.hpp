#pragma once

#include "DebuggingInformationEntry.hpp"

#include <dwarf.h>
#include "libdwarf.h"

#include <string>

// Compilation unit DIE
class DIECompileUnit : public DebuggingInformationEntry
{
public:
	DIECompileUnit(Dwarf_Debug dbg, Dwarf_Die die) :
		DebuggingInformationEntry(dbg, die)
	{

	}

private:
	virtual void onAttributeLoaded(Dwarf_Attribute attr, Dwarf_Half attr_code, Dwarf_Half form) override;

	// The name (filename) of the compilation unit
	std::string name;

	// Comilation unit entry address
	unsigned int lowpc;

	unsigned int highpc;
};