// Operations on the timespec data structure
// Some Linux distributions include bsd/sys/time.h but some don't
// Raspberry Pi Linux didn't seem to have the entire bsd folder in its include path
// So for now, I am only adding the necessary definitions
// These are copied from: https://github.com/openbsd/src/blob/master/sys/sys/time.h

#define	timespecclear(tsp)		(tsp)->tv_sec = (tsp)->tv_nsec = 0

#define	timespecisset(tsp)		((tsp)->tv_sec || (tsp)->tv_nsec)

#define	timespecisvalid(tsp)						\
	((tsp)->tv_nsec >= 0 && (tsp)->tv_nsec < 1000000000L)

#define	timespeccmp(tsp, usp, cmp)					\
	(((tsp)->tv_sec == (usp)->tv_sec) ?				\
	    ((tsp)->tv_nsec cmp (usp)->tv_nsec) :			\
	    ((tsp)->tv_sec cmp (usp)->tv_sec))

#define	timespecadd(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec >= 1000000000L) {			\
			(vsp)->tv_sec++;				\
			(vsp)->tv_nsec -= 1000000000L;			\
		}							\
	} while (0)

#define	timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)
