#include "DIECompileUnit.hpp"

// FOWARD DECLARATION [TODO: REMOVE]
void procmsg(const char* format, ...);

void DIECompileUnit::onAttributeLoaded(const Dwarf_Attribute &attr,
                                       const Dwarf_Half &attr_code,
                                       const Dwarf_Half &form)
{
	switch (attr_code)
	{
	case DW_AT_name:
   	{
		char *cname = 0;
		dwarf_formstring(attr, &cname, 0);
		name = cname;
		procmsg("[DWARF] [DIECompileUnit] Saving attribute: DW_AT_name (%s)\n", cname);
		break;
	}

	case DW_AT_comp_dir:
	{
		char *c_comp_dir = 0;
		dwarf_formstring(attr, &c_comp_dir, 0);
		comp_dir = c_comp_dir;
		procmsg("[DWARF] [DIECompileUnit] Save attribute: DW_AT_comp_dir (%s)\n", c_comp_dir);
		break;
	}

	case DW_AT_low_pc:
	{
		Dwarf_Addr addr;
		dwarf_formaddr(attr, &addr, 0);
		lowpc = addr;
		procmsg("[DWARF] [DIECompileUnit] Saving attribute: DW_AT_low_pc (0x%08x)\n", lowpc);
		break;
	}

	case DW_AT_high_pc:
	{
		switch (form)
		{
		// DWARF 2 implementation
		case DW_FORM_addr:
		{
			Dwarf_Addr addr;
			dwarf_formaddr(attr, &addr, 0);
			highpc = addr;
			break;
		}

		// DWARF 1 implementation
		case DW_FORM_data8:
		{
			Dwarf_Unsigned offset;
			dwarf_formudata(attr, &offset, 0);
			highpc = offset;
			break;
		}
		}
		procmsg("[DWARF] [DIECompileUnit] Saving attribute: DW_AT_high_pc (0x%08x)\n", highpc);
		break;
	}

	default:
		procmsg("[DWARF] [DIECompileUnit] Ignoring attribute...\n");
	}
}

std::string DIECompileUnit::getName()
{
	return name;
}

std::string DIECompileUnit::getCompDir()
{
	return comp_dir;
}