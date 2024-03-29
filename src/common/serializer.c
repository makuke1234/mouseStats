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
	
	const size_t itemNums[2] = { numItems, objSize };
	
	if (!fWrite(fileptr, itemNums, 2 * sizeof(size_t)))
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
				const void * addrToWrite = addrRet(objBase, idx, &sizeToWrite);
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
	size_t * restrict pobjSize,
	size_t indexesPerObj,
	void * restrict fileptr,
	ser_fileReader_t fRead,
	ser_addressRetreiver_t addrRet
)
{
	assert(baseaddress != NULL);
	assert(pobjSize != NULL);
	assert(((indexesPerObj > 0) && (addrRet != NULL)) || (indexesPerObj == 0));
	assert(fileptr != NULL);
	assert(fRead != NULL);
	
	size_t itemNums[2];
	if (!fRead(fileptr, &itemNums, 2 * sizeof(size_t)))
	{
		return false;
	}
	const size_t numItems = itemNums[0];
	*pobjSize = itemNums[1];
	
	if (!(numItems > 0) || !((*pobjSize) > 0))
	{
		return false;
	}
	
	if ((*baseaddress) == NULL)
	{
		*baseaddress = malloc(*pobjSize * numItems);
		if ((*baseaddress) == NULL)
		{
			return false;
		}
	}
	
	if (!indexesPerObj)
	{
		if (!fRead(fileptr, *baseaddress, *pobjSize * numItems))
		{
			return false;
		}
	}
	else
	{
		for (size_t i = 0; i < numItems; ++i)
		{
			void * objBase = (uint8_t *)(*baseaddress) + (i * (*pobjSize) );
			
			// Set everything to zero
			memset(objBase, 0, *pobjSize);
			
			for (size_t idx = 0; idx < indexesPerObj; ++idx)
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
