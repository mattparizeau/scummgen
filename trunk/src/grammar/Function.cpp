#include "Function.hpp"
#include <sstream>
#include "util/Log.hpp"
#include "Context.hpp"
#include "Declaration.hpp"
#include "Instruction.hpp"
#include "Statement.hpp"

Function::Function(string name, bool thread, BlockStatement *blockS)
{
	_name = name;
	_thread = thread;
	_blockStatement = blockS;
	_id = 0; // This member is filled later
}

void Function::removeLabels()
{
	// First, we get the label instruction addresses and remove the corresponding instructions
	map<uint32_t, uint32_t> labelAddresses;
	vector<Instruction *>::iterator iterator = _instructions.begin();
	while (iterator != _instructions.end())
		if ((*iterator)->getType() == INSTRUCTION_LABEL)
		{
			labelAddresses[(*iterator)->getLabel()] = (*iterator)->getAddress();
			delete (*iterator);
			iterator = _instructions.erase(iterator);
		}
		else
			iterator++;

	// Then, we replace labels by relative offsets
	for (int i = 0; i < _instructions.size(); i++)
		if (_instructions[i]->getType() == INSTRUCTION_VALUE && _instructions[i]->getValue().substr(0, 6) == "LABEL_")
		{
			uint32_t label;
			istringstream iss(_instructions[i]->getValue().substr(6, 1));
			iss >> label;
			// The magic 2 in this statement is because SCUMM jump/if/ifNot instructions
			// don't take the offset following them into account
			int32_t offset = (int32_t)labelAddresses[label] - (int32_t)_instructions[i]->getAddress() - 2;
			ostringstream oss;
			oss << offset;
			_instructions[i]->setValue(oss.str());
		}
}

void Function::displayAssembly()
{
	Log::getInstance().write(LOG_INFO, "Function \"%s\" (id n°%u) assembly:\n", _name.c_str(), _id);
	Log::getInstance().indent();
	for (int i = 0; i < _instructions.size(); i++)
		Log::getInstance().write(LOG_INFO, "%s\n", _instructions[i]->toString().c_str());
	Log::getInstance().unIndent();
}

void Function::compile()
{
	Log::getInstance().write(LOG_INFO, "Compiling function \"%s\"...\n", _name.c_str());
	Log::getInstance().indent();

	Context context(_thread ? CONTEXT_THREAD : CONTEXT_FUNCTION, &_arguments, NULL, -1, -1, 0);
	Context::pushContext(&context);

	// Prepare labels first
	Context::labelCounter++;

	// Compile block statement
	_blockStatement->compile(_instructions);

	// label 0
	_instructions.push_back(new Instruction(0));

	// stopObjectCode instruction
	if (_instructions.empty() || (!_instructions.empty() && _instructions.back()->getOpcodeName() != "stopObjectCode"))
		_instructions.push_back(new Instruction("stopObjectCode"));

	Context::popContext();

	displayAssembly();

	// Replace labels by relative offsets
	removeLabels();

	// Assemble instructions
	for (int i = 0; i < _instructions.size(); i++)
		_instructions[i]->assemble(_byteCode);

	Log::getInstance().unIndent();
}

Function::~Function()
{
	for (int i = 0; i < _arguments.size(); i++)
		delete _arguments[i];
	delete _blockStatement;
	for (int i = 0; i < _instructions.size(); i++)
		delete _instructions[i];
}
