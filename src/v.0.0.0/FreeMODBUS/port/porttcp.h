enum mbtcp_method
{
    MBTCP_METHOD_UNUSED = 0,
	MBTCP_METHOD_USED,
    MBTCP_METHOD_INVALID,
    MBTCP_METHOD_UNKNOWN
};

/**
 * \internal
 * The session structure.
 */
struct mbtcp_session
{
	int socket;
	uint8_t method;

	UCHAR aucTCPBuf[ 256 + 7 ];
	int16_t usTCPBufLen;
	//int16_t usTCPBufPos;
};
