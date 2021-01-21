#ifndef __UZMQ_UTIL_H__
#define __UZMQ_UTIL_H__


#define UZMQ_DEBUG_PRINT

#define UZMQ_UNUSED __attribute__((unused))

#define UZMQ_ERROR(format, ...) \
		fprintf(stderr, "[ERROR] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#define UZMQ_INFO(format, ...) \
		fprintf(stdout, "[INFO] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);

#ifdef UZMQ_DEBUG_PRINT
#define UZMQ_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG] %s %d: " format "\n", \
						__FILE__, __LINE__, ##__VA_ARGS__);
#else
#define UZMQ_DEBUG(format, ...)
#endif /* UZMQ_DEBUG_PRINT */

#define zfree(p) do {	\
	if (p) {			\
		free(p);		\
		p = NULL;		\
	} } while (0)



#endif /* __UZMQ_UTIL_H__ */
