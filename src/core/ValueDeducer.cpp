#include "ValueDeducer.hpp"

#include <sys/ptrace.h>

#include <cmath>

#include "dwarf/DIESubrangeType.hpp"
#include "dwarf/DIEMemberType.hpp"

// FOWARD DECLARATION [TODO: REMOVE]
void procmsg(const char* format, ...);

ValueDeducer::ValueDeducer(pid_t target_pid, std::shared_ptr<DwarfDebug> debug_data)
{
	this->target_pid = target_pid;
	this->debug_data = debug_data;
}

std::string ValueDeducer::deduce(uint64_t address, DebuggingInformationEntry &type_die)
{
	if (type_die.getTagName() == "DW_TAG_base_type")
	{
		DIEBaseType *die_base_type = dynamic_cast<DIEBaseType *>(&type_die);
		return deduceBase(address, *die_base_type);
	}
	else if (type_die.getTagName() == "DW_TAG_pointer_type")
	{
		DIEPointerType *die_ptr_type = dynamic_cast<DIEPointerType *>(&type_die);
		return deducePointer(address, *die_ptr_type);
	}
	else if (type_die.getTagName() == "DW_TAG_reference_type")
	{
		DIEReferenceType* die_ref_type = dynamic_cast<DIEReferenceType *>(&type_die);
		return deduceReference(address, *die_ref_type);
	}
	else if (type_die.getTagName() == "DW_TAG_array_type")
	{
		DIEArrayType *die_array_type = dynamic_cast<DIEArrayType *>(&type_die);
		return deduceArray(address, *die_array_type);
	}
	else if (type_die.getTagName() == "DW_TAG_structure_type")
	{
		DIEStructureType *die_struct_type = dynamic_cast<DIEStructureType *>(&type_die);
		return deduceStructure(address, *die_struct_type);
	}
	else if (type_die.getTagName() == "DW_TAG_class_type")
	{
		DIEClassType *die_class_type = dynamic_cast<DIEClassType *>(&type_die);
		return deduceClass(address, *die_class_type);
	}
	else if (type_die.getTagName() == "DW_TAG_const_type")
	{
		DIEConstType *die_const_type = dynamic_cast<DIEConstType *>(&type_die);
		return deduceConst(address, *die_const_type);
	}
	else
	{
		return "Type cannot be deduced";
	}
}

std::string ValueDeducer::deduceBase(uint64_t address, const DIEBaseType &base_die)
{
	// Get the data at the specified target process address
	uint64_t data = ptrace(PTRACE_PEEKDATA, target_pid, address, 0);

	// Use the encoding and byte size to determine the data's type
	switch (base_die.getEncoding())
	{
	//case DW_ATE_address:

	case DW_ATE_boolean:
	{
		bool value = (bool)data;
		return value ? "true" : "false";
	}

	//case DW_ATE_complex_float:

	case DW_ATE_float:
	{
		if (base_die.getByteSize() == 4)
		{
			float value = *((float *)&data);
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 8)
		{
			double value = *((double *)&data);
			return std::to_string(value);
		}
	}

	//case DW_ATE_imaginary_float:

	case DW_ATE_signed:
	{
		if (base_die.getByteSize() == 1)
		{
			char value = (char)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 2)
		{
			short int value = (short int)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 4)
		{
			int value = (int)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 8)
		{
			long int value = (long int)data;
			return std::to_string(value);
		}
	}

	case DW_ATE_signed_char:
	{
		char value = (char)data;
		return std::string(1, value);
	}

	case DW_ATE_unsigned:
	{
		if (base_die.getByteSize() == 1)
		{
			unsigned char value = (unsigned char)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 2)
		{
			short unsigned int value = (short unsigned int)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 4)
		{
			unsigned int value = (unsigned int)data;
			return std::to_string(value);
		}
		else if (base_die.getByteSize() == 8)
		{
			long unsigned int value = (long unsigned int)data;
			return std::to_string(value);
		}
	}

	case DW_ATE_unsigned_char:
	{
		unsigned char value = (unsigned char)data;
		return std::to_string(value);
	}

	//case DW_ATE_packed_decimal:
	
	//case DW_ATE_numeric_string:
	
	//case DW_ATE_edited:
	
	//case DW_ATE_signed_fixed:
	
	//case DW_ATE_unsigned_fixed:
	
	//case DW_ATE_decimal_float:
	}

	return "";
}

std::string ValueDeducer::deducePointer(uint64_t address, const DIEPointerType &pointer_die)
{
	std::shared_ptr<DebuggingInformationEntry> die = debug_data->info()->getDIEByOffset(pointer_die.type_offset);
	if (die == nullptr) return "Error retrieving type pointed to";

	uint64_t new_address = ptrace(PTRACE_PEEKDATA, target_pid, address, 0);
	return deduce(new_address, *die);
}

std::string ValueDeducer::deduceReference(uint64_t address, const DIEReferenceType &ref_die)
{
	std::shared_ptr<DebuggingInformationEntry> die = debug_data->info()->getDIEByOffset(ref_die.type_offset);
	if (die == nullptr) return "Error retrieving reference type";

	uint64_t new_address = ptrace(PTRACE_PEEKDATA, target_pid, address, 0);
	return deduce(new_address, *die);
}

std::string ValueDeducer::deduceArray(uint64_t address, DIEArrayType &array_die)
{
	// Get the type of the array
	std::shared_ptr<DebuggingInformationEntry> die = debug_data->info()->getDIEByOffset(array_die.getTypeOffset());
	if (die == nullptr) return "Error getting array type";
	DIEBaseType *array_base_type_die = dynamic_cast<DIEBaseType *>(die.get());

	// Find the subrange child DIE to determine the upper bound of the array
	uint64_t upper_bound = 0;
	auto child_ptrs = array_die.getChildren();
	for (auto child_ptr : child_ptrs)
	{
		if (child_ptr->getTagName() == "DW_TAG_subrange_type")
		{
			auto child = dynamic_cast<DIESubrangeType *>(child_ptr.get());
			upper_bound = child->getUpperBound();
			break;
		}
	}

	// Deduce the values of the array's elements
	std::string array_string = "{";
	uint64_t array_type_size = array_base_type_die->getByteSize();
	uint64_t array_size = array_type_size * (upper_bound + 1);
	for (uint64_t i = 0; i < array_size; i += array_type_size)
	{
		// Add a comma before adding the next value
		if (i > 0) array_string += ", ";

		// Deduce the array values and add them to the string
		array_string += deduce(address + i, *array_base_type_die);
	}
	array_string += "}";
	return array_string;
}

std::string ValueDeducer::deduceStructure(uint64_t address, DIEStructureType &struct_die)
{
	std::string values = "{";

	// Get the values of all the member variables
	uint64_t counter = 0;
	auto child_ptrs = struct_die.getChildren();
	for (auto child_ptr : child_ptrs)
	{
		if (child_ptr->getTagName() == "DW_TAG_member")
		{
			// Add a comma before adding the next member variable
			if (counter++ > 0) values += ", ";

			// Get the member variable DIE, its base type and its address
			auto member = dynamic_cast<DIEMemberType *>(child_ptr.get());
			auto member_type_die = debug_data->info()->getDIEByOffset(member->getTypeOffset());
			uint64_t member_address = address + member->getDataMemberLocation();

			// Append member variable name and value to the return string
			values += member->getName();
			values += "=";
			values += deduce(member_address, *member_type_die);
		}
	}

	values += "}";

	return values;
}

std::string ValueDeducer::deduceClass(uint64_t address, DIEClassType &class_die)
{
	std::string values = "{";

	// Get the values of all the member variables
	uint64_t counter = 0;
	auto child_ptrs = class_die.getChildren();
	for (auto child_ptr : child_ptrs)
	{
		if (child_ptr->getTagName() == "DW_TAG_member")
		{
			// Add a comma before adding the next member variable
			if (counter++ > 0) values += ", ";

			// Get the member variable DIE, its base type and its address
			auto member = dynamic_cast<DIEMemberType *>(child_ptr.get());
			auto member_type_die = debug_data->info()->getDIEByOffset(member->getTypeOffset());
			uint64_t member_address = address + member->getDataMemberLocation();

			// Append member variable name and value to the return string
			values += member->getName();
			values += "=";
			values += deduce(member_address, *member_type_die);
		}
	}

	values += "}";

	return values;
}

std::string ValueDeducer::deduceConst(uint64_t address, const DIEConstType &const_die)
{
	// Get the type of the array
	std::shared_ptr<DebuggingInformationEntry> die = debug_data->info()->getDIEByOffset(const_die.getTypeOffset());
	if (die == nullptr) return "Error retrieving const variable type";
	return deduce(address, *die);
}