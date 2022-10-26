#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "../common/winapi.h"

#define ser_serialise   ser_serialize
#define ser_ser         ser_serialize
#define ser_serial      ser_serialize

#define ser_deserialise ser_deserialize
#define ser_deser       ser_deserialize
#define ser_deserial    ser_deserialize


typedef bool (*ser_fileWriter_t)(void * restrict fileptr, const void * restrict data, size_t numBytes);
typedef bool (*ser_fileReader_t)(void * restrict fileptr, void * restrict data, size_t maxBytes);

bool ser_cFileWriter(void * restrict fileptr, const void * restrict data, size_t numBytes);
bool ser_winFileWriter(void * restrict fileptr, const void * restrict data, size_t numBytes);

bool ser_cFileReader(void * restrict fileptr, void * restrict data, size_t maxBytes);
bool ser_winFileReader(void * restrict fileptr, void * restrict data, size_t maxBytes);


typedef void * (*ser_addressRetreiver_t)(void * restrict objBaseAddr, size_t idx, size_t * objSize);

bool ser_serialize(
	const void * restrict baseaddress,
	size_t objSize,
	size_t numItems,
	size_t indexesPerObj,
	void * restrict fileptr,
	ser_fileWriter_t fWrite,
	ser_addressRetreiver_t addrRet
);

bool ser_deserialize(
	void ** restrict baseaddress,
	size_t * restrict pobjSize,
	size_t indexesPerObj,
	void * restrict fileptr,
	ser_fileReader_t fRead,
	ser_addressRetreiver_t addrRet
);



#endif
