/*
 * Copyright (c) 2022 Salesforce, Inc.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 * For full license text, see the LICENSE.txt file in the repo root or https://opensource.org/licenses/BSD-3-Clause
 */
#include "change_list.h"

#include "p4_api.h"
#include "describe_result.h"
#include "print_result.h"
#include "utils/std_helpers.h"

#include "thread_pool.h"

ChangeList::ChangeList(const std::string& clNumber, const std::string& clDescription, const std::string& userID, const int64_t& clTimestamp)
    : number(clNumber)
    , user(userID)
    , description(clDescription)
    , timestamp(clTimestamp)
{
}

void ChangeList::PrepareDownload()
{
	ChangeList& cl = *this;

	ThreadPool::GetSingleton()->AddJob([&cl](P4API* p4)
	    {
		    const DescribeResult& describe = p4->Describe(cl.number);
		    cl.changedFiles = std::move(describe.GetFileData());
		    {
			    std::unique_lock<std::mutex> lock((*(cl.canDownloadMutex)));
			    *cl.canDownload = true;
		    }
		    PRINT("Prepare download " << cl.number << " files " << cl.changedFiles.size());
			if (cl.changedFiles.empty())
			{
			    ERR("Empty CL " << cl.number);
			}
		    for (FileData& fileData : cl.changedFiles)
		    {
			    fileData.changelist = cl.number;
			}
		    cl.canDownloadCV->notify_all();
	    });
}

void ChangeList::StartDownload(const std::string& depotPath, const int& printBatch, const bool includeBinaries)
{
	ChangeList& cl = *this;

	ThreadPool::GetSingleton()->AddJob([&cl, &depotPath, printBatch, includeBinaries](P4API* p4)
	    {
		    // Wait for describe to finish, if it is still running
		    {
			    std::unique_lock<std::mutex> lock(*(cl.canDownloadMutex));
			    cl.canDownloadCV->wait(lock, [&cl]()
			        { return *(cl.canDownload) == true; });
		    }

		    *cl.filesDownloaded = 0;

		    if (cl.changedFiles.empty())
		    {
			    return;
		    }

		    PRINT("Start download " << cl.number);

		    std::shared_ptr<std::vector<std::string>> printBatchFiles = std::make_shared<std::vector<std::string>>();
		    std::shared_ptr<std::vector<FileData*>> printBatchFileData = std::make_shared<std::vector<FileData*>>();

		    for (int i = 0; i < cl.changedFiles.size(); i++)
		    {
			    FileData& fileData = cl.changedFiles[i];
			    if (p4->IsFileUnderDepotPath(fileData.depotFile, depotPath)
			        && p4->IsFileUnderClientSpec(fileData.depotFile)
			        && (includeBinaries || !p4->IsBinary(fileData.type))
			        && !STDHelpers::Contains(fileData.depotFile, "/.git/") // To avoid adding .git/ files in the Perforce history if any
			        && !STDHelpers::EndsWith(fileData.depotFile, "/.git")) // To avoid adding a .git submodule file in the Perforce history if any
			    {
				    fileData.shouldCommit = true;
				    printBatchFiles->push_back(fileData.depotFile + "#" + fileData.revision);
				    printBatchFileData->push_back(&fileData);
			    }
			    else
			    {
				    (*cl.filesDownloaded)++;
				    cl.commitCV->notify_all();
			    }

			    // Clear the batches if it fits
			    if (printBatchFiles->size() == printBatch)
			    {
				    cl.Flush(printBatchFiles, printBatchFileData);

				    // We let go of the refs held by us and create new ones to queue the next batch
				    printBatchFiles = std::make_shared<std::vector<std::string>>();
				    printBatchFileData = std::make_shared<std::vector<FileData*>>();
				    // Now only the thread job has access to the older batch
			    }
		    }

		    // Flush any remaining files that were smaller in number than the total batch size
		    if (!printBatchFiles->empty())
		    {
			    cl.Flush(printBatchFiles, printBatchFileData);
		    }
	    });
}

void ChangeList::Flush(std::shared_ptr<std::vector<std::string>> printBatchFiles, std::shared_ptr<std::vector<FileData*>> printBatchFileData)
{
	// Share ownership of this batch with the thread job
	ThreadPool::GetSingleton()->AddJob([this, printBatchFiles, printBatchFileData](P4API* p4)
	    {
		    for (auto& e : *printBatchFiles)
		    {
			    PRINT("Printing " << e);
		    }

		    const PrintResult& printData = p4->PrintFiles(*printBatchFiles);

		    for (int i = 0; i < printBatchFiles->size(); i++)
		    {
			    //printBatchFileData->at(i)->contents = std::move(printData.GetPrintData().at(i).contents);
			    printBatchFileData->at(i)->SetContents(printData.GetPrintData().at(i).contents);
		    }

		    (*filesDownloaded) += printBatchFiles->size();

		    commitCV->notify_all();
	    });
}

void ChangeList::WaitForDownload()
{
	std::unique_lock<std::mutex> lock(*commitMutex);
	commitCV->wait(lock, [this]()
	    { return *(filesDownloaded) == (int)changedFiles.size(); });
}

void ChangeList::Clear()
{
	number.clear();
	user.clear();
	description.clear();
	changedFiles.clear();

	filesDownloaded.reset();
	canDownload.reset();
	canDownloadMutex.reset();
	canDownloadCV.reset();
	commitMutex.reset();
	commitCV.reset();
}
