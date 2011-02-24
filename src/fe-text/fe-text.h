
typedef int (*socket_callback) (void *source, int condition, void *user_data);
typedef int (*timer_callback) (void *user_data);

struct socketeventRec
{
	socket_callback callback;
	void *userdata;
	int sok;
	int tag;
	int rread:1;
	int wwrite:1;
	int eexcept:1;
	int checked:1;
};

typedef struct socketeventRec socketevent;


struct timerRec
{
	timer_callback callback;
	void *userdata;
	int interval;
	int tag;
	guint64 next_call;	/* miliseconds */
};

typedef struct timerRec timerevent;
