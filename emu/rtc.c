#include "pdp6.h"
#include <time.h>

struct timespec
timesub(struct timespec t1, struct timespec t2)
{
	struct timespec d;
	d.tv_sec = t2.tv_sec - t1.tv_sec;
	d.tv_nsec = t2.tv_nsec - t1.tv_nsec;
	if(d.tv_nsec < 0){
		d.tv_nsec += 1000000000;
		d.tv_sec -= 1;
	}
	return d;
}

struct timespec
timeadd(struct timespec t1, struct timespec t2)
{
	struct timespec s;
	s.tv_sec = t1.tv_sec + t2.tv_sec;
	s.tv_nsec = t1.tv_nsec + t2.tv_nsec;
	if(s.tv_nsec >= 1000000000){
		s.tv_nsec -= 1000000000;
		s.tv_sec += 1;
	}
	return s;
}

/* is t2 after t1 */
int
timeafter(struct timespec t1, struct timespec t2)
{
	if(t1.tv_sec == t2.tv_sec)
		return t1.tv_nsec < t2.tv_nsec;
	return t1.tv_sec < t2.tv_sec;
}

/* is t2 before t1 */
int
timebefore(struct timespec t1, struct timespec t2)
{
	if(t1.tv_sec == t2.tv_sec)
		return t1.tv_nsec > t2.tv_nsec;
	return t1.tv_sec > t2.tv_sec;
}

typedef struct Clockchan Clockchan;
struct Clockchan
{
	int active;
	Channel *c;
	struct timespec start;
	struct timespec timeout;
	struct timespec interval;
	int repeat;
	Clockchan *next;
};
static Clockchan *channels;
Channel *rtcchan;

static Clockchan*
getchan(Channel *c)
{
	Clockchan *cc;
	for(cc = channels; cc; cc = cc->next)
		if(cc->c == c)
			return cc;
	return nil;
}

static void
rtcstart(Channel *c, word interval, int repeat)
{
	Clockchan *cc;

	cc = getchan(c);
	if(cc == nil){
		cc = malloc(sizeof(Clockchan));
		memset(cc, 0, sizeof(Clockchan));
		cc->next = channels;
		channels = cc;
	}

	cc->active = 1;
	cc->c = c;
	clock_gettime(CLOCK_REALTIME, &cc->start);
	cc->interval.tv_sec = interval / 1000000000;
	cc->interval.tv_nsec = interval % 1000000000;
	cc->timeout = timeadd(cc->start, cc->interval);
	cc->repeat = repeat;
}

static void
rtcstop(Channel *c)
{
	Clockchan *cc;

	cc = getchan(c);
	if(cc)
		cc->active = 0;
}

void*
rtcthread(void *p)
{
	struct timespec now;
	Clockchan *cc;
	RtcMsg msg;
	int loss;

	loss = 0;
	for(;;){
		clock_gettime(CLOCK_REALTIME, &now);

		/* Check all channels for a timeout */
		for(cc = channels; cc; cc = cc->next){
			if(!cc->active || timebefore(cc->timeout, now))
				continue;

			/* I *hope* this is ok */
			if(channbsend(cc->c, &loss) == 0)
				;//printf("missed clock\n");
			if(cc->repeat)
				cc->timeout = timeadd(cc->timeout, cc->interval);
			else
				cc->active = 0;
		}

		if(channbrecv(rtcchan, &msg) == 1){
			if(msg.msg == 1)
				rtcstart(msg.c, msg.interval, msg.repeat);
			else
				rtcstop(msg.c);
		}
	}
}


/*
int
main()
{
	long max = 0;
	int n = 0x7FFFFFFF;

	struct timespec interval = { 0, 500000000 };
	struct timespec timeout, now;

	clock_gettime(CLOCK_REALTIME, &timeout);
	timeout = timeadd(timeout, interval);

	while(n--){
		clock_gettime(CLOCK_REALTIME, &now);
		if(timeafter(timeout, now)){
			timeout = timeadd(timeout, interval);
			printf("tick\n");		
		}
	}

	return 0;
}
*/
