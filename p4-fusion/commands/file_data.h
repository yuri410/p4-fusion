/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#pragma once

#include "common.h"

struct FileData
{
	std::string depotFile;
	std::string revision;
	std::string action;
	std::string type;
	std::vector<char> contents;
	bool shouldCommit = false;

	int fileID;
	std::string changelist;

	std::vector<char> GetContents()
	{
		std::string fn = GetTempFileName();
		FILE* f = fopen(fn.c_str(), "rb");

		fseek(f, 0, SEEK_END);
		long numbytes = ftell(f);
		fseek(f, 0, SEEK_SET);

		std::vector<char> buf = std::vector<char>(numbytes, '\0');
		fread(buf.data(), 1, numbytes, f);
		fclose(f);

		return buf;
	}

	void SetContents(const std::vector<char>& contents)
	{
		std::string fn = GetTempFileName();
		FILE* f = fopen(fn.c_str(), "wb");
		fwrite(contents.data(), 1, contents.size(), f);
		fclose(f);
	}

	std::string GetTempFileName()
	{
		return "Temp_" + changelist + "_" + std::to_string(fileID);
	}

	void Clear()
	{
		depotFile.clear();
		revision.clear();
		action.clear();
		type.clear();
		contents.clear();

		changelist.clear();

		remove(GetTempFileName().c_str());
	}
};
