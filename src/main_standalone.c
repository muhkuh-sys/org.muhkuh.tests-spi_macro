#include "main.h"
#include "serial_vectors.h"

#include <string.h>


extern unsigned long parameter_start_address[];
extern char messages_start_address[];
extern char messages_end_address[];


static char *pucStdOutCnt;
static char *pucStdOutMax;


static void StdOutMem_put(unsigned int uiChar)
{
	if( pucStdOutCnt<pucStdOutMax )
	{
		*(pucStdOutCnt++) = (unsigned char)(uiChar&0xffU);
	}
}



static void StdOutMem_flush(void)
{
}


TEST_RESULT_T main_standalone(void);
TEST_RESULT_T main_standalone(void)
{
	const TEST_PARAMETER_T * ptParameter;
	TEST_RESULT_T tResult;
	unsigned char *pucResult;


	/* Write output to memory. */
	pucResult = (unsigned char*)messages_start_address;
	pucStdOutCnt = messages_start_address + 4;
	pucStdOutMax = messages_end_address;
	memset(pucStdOutCnt, 0, (size_t)(pucStdOutMax-pucStdOutCnt));

	tSerialVectors.fn.fnGet = NULL;
	tSerialVectors.fn.fnPeek = NULL;
	tSerialVectors.fn.fnPut = StdOutMem_put;
	tSerialVectors.fn.fnFlush = StdOutMem_flush;


	/* Get the parametert block.
	 * It is appended to the end of the binary.
	 */
	ptParameter = (const TEST_PARAMETER_T*)(parameter_start_address);
	tResult = test_main(ptParameter);
	*pucResult = (unsigned char)(tResult);

	return tResult;
}
