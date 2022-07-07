/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "describe_result.h"
#include <unordered_map>

void DescribeResult::OutputStat(StrDict* varList)
{
	std::unordered_map<std::string, std::string> pairs;
	{
		int i;
		StrRef var, val;

		// Dump out the variables, using the GetVar( x ) interface.
		// Don't display the function, which is only relevant to rpc.
		for (i = 0; varList->GetVar(i, var, val); i++)
		{
			if (var == "func")
				continue;

			pairs[std::string(var.Text())] = val.Text();
		}
	}

	auto findValue = [&pairs](const std::string& key) -> std::string*
	{
		auto iter = pairs.find(key);
		if (iter != pairs.end())
			return &iter->second;
		return nullptr;
	};

	for (;;)
	{
		std::string indexString = std::to_string(m_FileData.size());

		std::string* depotFile = findValue("depotFile" + indexString);
		if (!depotFile)
		{
			break;
		}

		std::string* type = findValue("type" + indexString);
		std::string* revision = findValue("rev" + indexString);
		std::string* action = findValue("action" + indexString);

		m_FileData.push_back(FileData {});
		FileData* fileData = &m_FileData.back();

		fileData->depotFile = *depotFile;
		fileData->revision = *revision;
		fileData->type = *type;
		fileData->action = *action;
		fileData->fileID = (int)m_FileData.size();

		if (!m_FileData.empty() && (m_FileData.size() % 10000) == 0)
		{
			PRINT("Described " << m_FileData.size());
		}
	}
}

int DescribeResult::OutputStatPartial(StrDict* varList)
{
	std::string indexString = std::to_string(m_FileData.size());

	StrPtr* depotFile = varList->GetVar(("depotFile" + indexString).c_str());
	if (!depotFile)
	{
		// Quick exit if the object returned is not a file
		return 0;
	}
	StrPtr* type = varList->GetVar(("type" + indexString).c_str());
	StrPtr* revision = varList->GetVar(("rev" + indexString).c_str());
	StrPtr* action = varList->GetVar(("action" + indexString).c_str());

	m_FileData.push_back(FileData {});
	FileData* fileData = &m_FileData.back();

	fileData->depotFile = depotFile->Text();
	fileData->revision = revision->Text();
	fileData->type = type->Text();
	fileData->action = action->Text();
	fileData->fileID = (int)m_FileData.size();

	return 1;
}

void DescribeResult::OutputText(const char* data, int length)
{
}

void DescribeResult::OutputBinary(const char* data, int length)
{
	OutputText(data, length);
}
