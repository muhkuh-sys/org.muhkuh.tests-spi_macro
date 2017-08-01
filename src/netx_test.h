
#ifndef __NETX_TEST_H__
#define __NETX_TEST_H__

/*-----------------------------------*/

typedef enum TEST_RESULT_ENUM
{
	TEST_RESULT_OK = 0,
	TEST_RESULT_ERROR = 1
} TEST_RESULT_T;

typedef struct TEST_PARAMETER_HEADER_STRUCT
{
	unsigned long ulReturnValue;
	void *pvTestParameter;
	void *pvReturnMessage;
} TEST_PARAMETER_HEADER_T;

/*-----------------------------------*/

#endif  /* __NETX_TEST_H__ */
