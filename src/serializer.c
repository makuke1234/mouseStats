#include "serializer.h"

bool ser_cFileWriter(void * restrict fileptr, const void * restrict data, size_t numBytes)
{
	FILE * fp = (FILE *)fileptr;
	
	return fwrite(data, numBytes, 1, fp) == 1;
}
bool ser_winFileWriter(void * restrict fileptr, const void * restrict data, size_t numBytes)
{
	HANDLE hfile = (HANDLE)fileptr;
	
	DWORD dwBytesWritten;
	return (WriteFile(
		hfile,
		data,
		(DWORD)numBytes,
		&dwBytesWritten,
		NULL
	) != 0) && (dwBytesWritten == numBytes);
}

bool ser_cFileReader(void * restrict fileptr, void * restrict data, size_t maxBytes)
{
	FILE * fp = (FILE *)fileptr;
	
	return fread(data, maxBytes, 1, fp) == 1;
}
bool ser_winFileReader(void * restrict fileptr, void * restrict data, size_t maxBytes)
{
	HANDLE hfile = (HANDLE)fileptr;
	
	DWORD dwBytesRead;
	return (ReadFile(
		hfile,
		data,
		(DWORD)maxBytes,
		&dwBytesRead,
		NULL
	) != 0) && (dwBytesRead == maxBytes);
}


bool ser_serialize(
	const void * restrict baseaddress,
	size_t objSize,
	size_t numItems,
	size_t indexesPerObj,
	void * restrict fileptr,
	ser_fileWriter_t fWrite,
	ser_addressRetreiver_t addrRet
)
{
	assert(baseaddress != NULL);
	assert(objSize > 0);
	assert(numItems > 0);
	assert(((indexesPerObj > 0) && (addrRet != NULL)) || (indexesPerObj == 0));
	assert(fileptr != NULL);
	assert(fWrite != NULL);
	
	if (!fWrite(fileptr, &numItems, sizeof numItems))
	{
		return false;
	}
	
	if (!indexesPerObj)
	{
		if (!fWrite(fileptr, baseaddress, objSize * numItems))
		{
			return false;
		}
	}
	else
	{
		for (size_t i = 0; i < numItems; ++i)
		{
			void * objBase = (uint8_t *)baseaddress + (i * objSize);
			for (size_t idx = 0; idx < indexesPerObj; ++idx)
			{
				size_t sizeToWrite;
				void * addrToWrite = addrRet(objBase, idx, &sizeToWrite);
				assert(addrToWrite != NULL);
				assert(sizeToWrite > 0);
				
				if (!fWrite(fileptr, addrToWrite, sizeToWrite))
				{
					return false;
				}
			}
		}
	}
	
	return true;
}

bool ser_deserialize(
	void ** restrict baseaddress,
	size_t objSize,
	size_t indexesPerObj,
	void * restrict fileptr,
	ser_fileReader_t fRead,
	ser_addressRetreiver_t addrRet
)
{
	assert(baseaddress != NULL);
	assert(objSize > 0);
	assert(((indexesPerObj > 0) && (addrRet != NULL)) || (indexesPerObj == 0));
	assert(fileptr != NULL);
	assert(fRead != NULL);
	
	size_t numItems;
	if (!fRead(fileptr, &numItems, sizeof numItems))
	{
		return false;
	}
	
	assert(numItems > 0);
	
	if ((*baseaddress) == NULL)
	{
		*baseaddress = malloc(objSize * numItems);
		if ((*baseaddress) == NULL)
		{
			return false;
		}
	}
	
	if (!indexesPerObj)
	{
		if (!fRead(fileptr, *baseaddress, objSize * numItems))
		{
			return false;
		}
	}
	else
	{
		for (size_t i = 0; i < numItems; ++i)
		{
			void * objBase = (uint8_t *)(*baseaddress) + (i * objSize);
			
			// Set everything to zero
			memset(objBase, 0, objSize);
			
			for (size_t idx = 0; i < indexesPerObj; ++idx)
			{
				size_t sizeToRead;
				void * addrToRead = addrRet(objBase, idx, &sizeToRead);
				assert(addrToRead != NULL);
				assert(sizeToRead > 0);
				
				if (!fRead(fileptr, addrToRead, sizeToRead))
				{
					return false;
				}
			}
		}
	}
	
	return true;
}
