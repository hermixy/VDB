#include "DebuggingInformationEntry.hpp"

#include "DIESubprogram.hpp"
#include "DIEFormalParameter.hpp"

#include <cstring>

// FOWARD DECLARATION [TODO: REMOVE]
void procmsg(const char* format, ...);

// TODO: Should probably be a helper method...
std::string getDieTagName(const Dwarf_Die &die)
{
	const char *tag_name = 0;
	Dwarf_Half tag;
	Dwarf_Error err;

	// Sets a pointer to the tag of this DIE
	if (dwarf_tag(die, &tag, &err))
		procmsg("[DWARF_ERROR] Error in dwarf_tag!\n");

	// Gets the tag name from the tag pointer
	if (dwarf_get_TAG_name(tag, &tag_name) != DW_DLV_OK)
		procmsg("[DWARF_ERROR] Error in dwarf_get_TAG_name!\n");

	return tag_name;
}



DebuggingInformationEntry::DebuggingInformationEntry(const Dwarf_Debug &dbg,
                                                     const Dwarf_Die &die)
{
	this->dbg = dbg;
	this->die = die;
	
	loadChildren();
}

void DebuggingInformationEntry::loadChildren()
{
	procmsg("Loading children of: %s\n", getTagName().c_str());

	// Check that this DIE has at least 1 child
	Dwarf_Die child_die;
	Dwarf_Error err;
	int result = dwarf_child(die, &child_die, &err);

	// TODO: Perform more extensive error handling here
	if (result != DW_DLV_OK)
	{
		return;
	}

	addChildDie(child_die);

	// Go over all children DIEs
	while (1)
	{
		// Get the next DIE sibling along
		result = dwarf_siblingof(dbg, child_die, &child_die, &err);

		// TODO: Perform more extensive error handling here
		if (result != DW_DLV_OK)
		{
			break; // Done
		}

		addChildDie(child_die);
	}
}

void DebuggingInformationEntry::addChildDie(const Dwarf_Die &child_die)
{
	std::string tag_name = getDieTagName(child_die);

	if (tag_name == "DW_TAG_subprogram")
		children.push_back(std::shared_ptr<DebuggingInformationEntry>(new DIESubprogram(dbg, child_die)));

	else if (tag_name == "DW_TAG_formal_parameter")
		children.push_back(std::shared_ptr<DebuggingInformationEntry>(new DIEFormalParameter(dbg, child_die)));
	
	else
	{
		procmsg("[DWARF] [%s] Ignoring child DIE type: %s\n", getTagName().c_str(), getDieTagName(child_die).c_str());
		return;
	}

	children.back()->loadAttributes();
}

void DebuggingInformationEntry::loadAttributes()
{
	Dwarf_Error err;
	Dwarf_Attribute *attrs;
	Dwarf_Signed attr_count;

	// Get the list of attributes and the list size
	if (dwarf_attrlist(die, &attrs, &attr_count, &err) != DW_DLV_OK)
		procmsg("[DWARF_ERROR] Error in dwarf_attrlist!\n");

	// Loop through all attributes
	for (int i = 0; i < attr_count; i++)
	{
		// Determine the type of the attribute
		Dwarf_Half attr_code;
		if (dwarf_whatattr(attrs[i], &attr_code, &err) != DW_DLV_OK)
			procmsg("[DWARF_ERROR] Error in dwarf_whatattr!\n");

		// Determine the form of the attribute
		Dwarf_Half form;
		if (dwarf_whatform(attrs[i], &form, &err) != DW_DLV_OK)
			procmsg("[DWARF_ERROR] Error in dwarf_whatform!\n");

		// Notify subclasses that an attribute has been found
		onAttributeLoaded(attrs[i], attr_code, form);
	}
}

Dwarf_Die &DebuggingInformationEntry::getInternalDie()
{
	return die;
}

std::string DebuggingInformationEntry::getTagName()
{
	const char *tag_name = 0;
	Dwarf_Half tag;
	Dwarf_Error err;

	// Sets a pointer to the tag of this DIE
	if (dwarf_tag(die, &tag, &err))
		procmsg("[DWARF_ERROR] Error in dwarf_tag!\n");

	// Gets the tag name from the tag pointer
	if (dwarf_get_TAG_name(tag, &tag_name) != DW_DLV_OK)
		procmsg("[DWARF_ERROR] Error in dwarf_get_TAG_name!\n");

	return tag_name;
}

std::vector<std::shared_ptr<DebuggingInformationEntry>> &DebuggingInformationEntry::getChildren()
{
	return children;
}
